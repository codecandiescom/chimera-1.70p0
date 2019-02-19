/*
 * net.c
 *
 * Copyright (C) 1993, 1994, 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 */
#include "copyright.h"

#include "options.h"

#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>

#if ((defined(SYSV) || defined(SVR4)) && defined(sun))
#include <sys/file.h>
#endif
 
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef USE_LONGJMPS
#include <setjmp.h>
#endif

#include "common.h"
#include "util.h"
#include "net.h"

#if defined(SYSV) || defined(SVR4)
# define bzero(dst,len) memset(dst,0,len)
# define bcopy(src,dst,len) memcpy(dst,src,len)
#endif /*SYSV*/

#ifdef TERM
#include "termnet.h"
#endif

extern int DisplayConnectStatus _ArgProto((int, char *));
				/* defined in src/main.c */

#ifdef USE_LONGJMPS
jmp_buf lj_env;			/* Global Variables Are Evil ;^) */
 /*
  * handle_alrm
  * SIGALRM handler
  */
void
handle_alrm()
{
  signal(SIGALRM, SIG_IGN);
  alarm(0);
  longjmp(lj_env,1);
}
#endif

/* #define DNS_CACHE_PROFILE 1*/

/*
 * net_open
 *
 * open network connection to a host.  the regular version.
 *
 * DNS mini-cache:  We keep track of the two most recently visited
 * hosts  (names and IP addresses),  subject to an expiry timeout
 * of about 20 minutes to prevent us from using stale addresses.
 * This should substantially cut down on the number of DNS lookups
 * required.-- I was tempted to hard-code localhost=127.0.0.1 into
 * the source as a default cache entry but I resisted...
 *
 * DNS lookups:  If we can use setjmp/longjmp, we first try running
 * gethostbyname with a 5 seconds alarm ticking in the background,
 * recovering via a longjmp if the alarm goes off first.  If we can't,
 * or if it does and we haven't yet received a user Cancel, we try
 * forking and letting the gethostbyname run in the child until it
 * succeeds or returns failure or times out on its own, whilst doing
 * a slow Cancel-polling loop in the parent.
 *
 * If the fork fails, we fall back to doing a blocking lookup.
 *
 * NB when a proxy is in use, we'd normally find it fast, and wait for
 * _it_ doing the remote DNS lookup after sending our request.  This
 * is readily handled by the normal download/DisplayTransferStatus code.
 * --GN 1997Apr30
 */
#define ttl 1024              /* cache entry time-to-live in seconds */

int
net_open(hostname, port)
char *hostname;
int port;
{
  int s;
  struct sockaddr_in addr;
  static struct sockaddr_in addr_mru, addr_2mru;
  static char *host_mru = NULL, *host_2mru = NULL;
  char *dotted_quad;
  static char *dqud_mru = NULL, *dqud_2mru = NULL;
  time_t now;
  static time_t when_mru = 0, when_2mru = 0;
  struct hostent *hp;
  extern int errno;
  char *dq_hn;			/* dotted quad plus hostname */
  int child;
  int forked = 0;
  int canceled = 0;
#ifdef DNS_CACHE_PROFILE
  static int dns_hit = 0, dns_miss = 0;
#endif

  now = time((time_t *)0);
                              /* cache: expiry */
  if (host_mru != (char *)NULL && (unsigned long)(now - when_mru) > ttl)
  {
#ifdef DNS_CACHE_PROFILE
    printf("DNS stale mru %s purged\n", host_mru);
#endif
    free_mem(host_mru);
    free_mem(dqud_mru);
    host_mru = dqud_mru = (char *)NULL;
  }
  if (host_2mru != (char *)NULL && (unsigned long)(now - when_2mru) > ttl)
  {
#ifdef DNS_CACHE_PROFILE
    printf("DNS stale 2mru %s purged\n", host_2mru);
#endif
    free_mem(host_2mru);
    free_mem(dqud_2mru);
    host_2mru = dqud_2mru = (char *)NULL;
  }
                              /* cache: mru lookup attempt */
  if (host_mru != NULL && strcmp(hostname, host_mru) == 0)
  {                           /* found */
    bcopy((char *)&addr_mru, (char *)&addr, sizeof(addr));
    dotted_quad = dqud_mru;   /* copy pointer not string */
#ifdef DNS_CACHE_PROFILE
    if ((++dns_hit + dns_miss)%20 == 0)
    {
      printf("DNS hit rate: %4.1f\n", (5.*dns_hit)/((dns_hit+dns_miss)/20));
    }
#endif
  }
                              /* cache: 2mru lookup attempt */
  else if (host_2mru != NULL && strcmp(hostname, host_2mru) == 0)
  {
    char *host_tmru;
    time_t when_tmru;

    bcopy((char *)&addr_2mru, (char *)&addr, sizeof(addr));
    dotted_quad = dqud_2mru;

    when_tmru = when_2mru;    /* swap mru / 2mru around */
    host_tmru = host_2mru;
    host_2mru = host_mru;
    bcopy((char *)&addr_mru, (char *)&addr_2mru, sizeof(addr));
    dqud_2mru = dqud_mru;
    when_2mru = when_mru;
    host_mru = host_tmru;
    bcopy((char *)&addr, (char *)&addr_mru, sizeof(addr));
    dqud_mru = dotted_quad;
    when_mru = when_tmru;
#ifdef DNS_CACHE_PROFILE
    if ((++dns_hit + dns_miss)%20 == 0)
    {
      printf("DNS hit rate: %4.1f\n", (5.*dns_hit)/((dns_hit+dns_miss)/20));
    }
#endif
  }
  else                                /* attempt DNS lookup */
  {
    if (DisplayConnectStatus(0, hostname) == 1)
      return(-1);

#ifdef DNS_CACHE_PROFILE
                              /* count failed lookups as misses */
    if ((++dns_miss + dns_hit)%20 == 0)
    {
      printf("DNS hit rate: %4.1f\n", (5.*dns_hit)/((dns_hit+dns_miss)/20));
    }
#endif

    bzero((char *)&addr, sizeof(addr));

  /* fix by Jim Rees so that numeric addresses are dealt with */
    if ((addr.sin_addr.s_addr = inet_addr(hostname)) == -1)
    {
                              /* begin DNS ceremony */
#ifdef USE_LONGJMPS
      signal(SIGALRM, &handle_alrm);
      alarm(5);
      if (setjmp(lj_env) == 0)
      {
      hp = (struct hostent *)gethostbyname(hostname);
      alarm(0);               /* turn it off */
      signal(SIGALRM, SIG_IGN);
      if (hp == NULL) return(-1);
      }
      else                    /* alarm went off, longjmp hopped */
      {
      int pfd[2];             /* pipe fd's */

      if (DisplayConnectStatus(0, hostname) == 1)
        return(-1);

      if (pipe(pfd) == 0 && (child = (int)fork()) < 0)
      {                       /* fork failure for whatever reason */
        close(pfd[0]); close(pfd[1]);
        if ((hp = (struct hostent *)gethostbyname(hostname)) == NULL)
          return(-1);         /* do it with a blocking call */
      }
      else if (child == 0)
      {                       /* child: run gethostbyname to bitter end */
        int len;
        close(pfd[0]);
        if ((hp = (struct hostent *)gethostbyname(hostname)) == NULL)
          exit(1);
        bcopy(hp->h_addr, (char *)&(addr.sin_addr), hp->h_length);
        dotted_quad = inet_ntoa(addr.sin_addr); /* for passing it back */
        if ((len = strlen(dotted_quad)) > 254)
          len = 255;          /* paranoia */
        write(pfd[1], (const char *)dotted_quad, len);
        write(pfd[1], "", 1); /* write a 0 byte */
        close(pfd[1]);
        exit(0);
      }
      else
      {                       /* parent: exercise DisplayConnectStatus */
                              /* we could do a select on the pipe but I'm
                                 too lazy to figure out how to ;^) --GN */
        int rstat;
        char *t;

        forked = 1;
        close(pfd[1]);
        fcntl(pfd[0], F_SETFL, O_NONBLOCK);
        t = dotted_quad = alloc_mem(256);
        while (1)
        {
          if ((rstat = read(pfd[0], t , 16)) < 0 && errno == EAGAIN)
          {
            sleep(3);
            if (DisplayConnectStatus(0, hostname) == 1)
            {
              close(pfd[0]);
              return(-1);     /* DON't signal the child or it'll erase
                                 our cache files... */
            }
          }
          else if (rstat > 0)
            t += rstat;
          else
            break;
        }
        close(pfd[0]);
        if (t > dotted_quad && *--t == '\0')
        {
          if ((addr.sin_addr.s_addr = inet_addr(dotted_quad)) == -1)
            return(-1);
        }
        else
          return(-1);
      }
      }
#else  /* do without alarm, setjmp and longjmp */
      int pfd[2];		/* pipe fd's */

      if (DisplayConnectStatus(0, hostname) == 1)
	return(-1);

      if (pipe(pfd) == 0 && (child = (int)fork()) < 0)
      {				/* fork failure for whatever reason */
	close(pfd[0]); close(pfd[1]);
	if ((hp = (struct hostent *)gethostbyname(hostname)) == NULL)
	  return(-1);		/* do it with a blocking call */
      }
      else if (child == 0)
      {				/* child: run gethostbyname to bitter end */
	int len;
	close(pfd[0]);
	if ((hp = (struct hostent *)gethostbyname(hostname)) == NULL)
	  exit(1);
	bcopy(hp->h_addr, (char *)&(addr.sin_addr), hp->h_length);
	dotted_quad = inet_ntoa(addr.sin_addr); /* for passing it back */
	if ((len = strlen(dotted_quad)) > 254)
	  len = 255;		/* paranoia */
	write(pfd[1], (const char *)dotted_quad, len);
	write(pfd[1], "", 1);	/* write a 0 byte */
	close(pfd[1]);
	exit(0);
      }
      else
      {				/* parent: exercise DisplayConnectStatus */
				/* we could do a select on the pipe but I'm
				   too lazy to figure out how to ;^) --GN */
	int rstat;
	char *t;

	forked = 1;
	close(pfd[1]);
	fcntl(pfd[0], F_SETFL, O_NONBLOCK);
	t = dotted_quad = alloc_mem(256);
	while (1)
	{
        if ((rstat = read(pfd[0], t , 4)) < 0 && errno == EAGAIN)
	  {
	    sleep(3);
	    if (DisplayConnectStatus(0, hostname) == 1)
	    {
	      return(-1);	/* DON't signal the child or it'll erase
				   our cache files... */
	    }
	  }
	  else if (rstat > 0)
	    t += rstat;
	  else
	    break;
	}
	close(pfd[0]);
	if (t > dotted_quad && *--t == '\0')
	{
	  if ((addr.sin_addr.s_addr = inet_addr(dotted_quad)) == -1)
	    return(-1);
	}
	else
	  return(-1);
      }
#endif
                              /* end DNS ceremony, extract address */
      if (!forked)
      bcopy(hp->h_addr, (char *)&(addr.sin_addr), hp->h_length);
    }

    if (!forked)
      dotted_quad = inet_ntoa(addr.sin_addr); /* for displaying it */

                              /* now cache the result */
    if (host_mru != NULL)
    {                         /* don't overwrite a valid entry with a stale
                                 one */
      if (host_2mru != NULL)
      {
#ifdef DNS_CACHE_PROFILE
      printf("DNS pushing out %s\n", host_2mru);
#endif
      free_mem(host_2mru);
      free_mem(dqud_2mru);
      host_2mru = dqud_2mru = (char *)NULL;
      }
      host_2mru = host_mru;
      bcopy((char *)&addr_mru, (char *)&addr_2mru, sizeof(addr));
      dqud_2mru = dqud_mru;
      when_2mru = when_mru;
    }
#ifdef DNS_CACHE_PROFILE
    printf("DNS caching %s = %s\n", hostname, dotted_quad);
#endif
    host_mru = alloc_string(hostname);
    bcopy((char *)&addr, (char *)&addr_mru, sizeof(addr));
    dqud_mru = alloc_string(dotted_quad);
    when_mru = now;
  } /* end DNS lookup branch */

  dq_hn = alloc_mem(strlen(dotted_quad) + strlen(hostname) + 4);
  sprintf(dq_hn, "%s (%s)", dotted_quad, hostname);

  if (DisplayConnectStatus(1, dq_hn) == 1)
    canceled = 1;
  free_mem(dq_hn);
  if (canceled) return(-1);

  addr.sin_family = AF_INET;
  addr.sin_port = htons((unsigned short)port);

  s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (s < 0) return(-1);

#ifndef SOCKS 
#ifdef NONBLOCKING_CONNECT
  fcntl(s, F_SETFL, FNDELAY);
#endif

  if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
#else
  if (Rconnect(s, &addr, sizeof(addr)) < 0)
#endif
  {
    if (errno != EINPROGRESS) return(-1);
  }
  
  return(s);
}

/*
 * net_bind
 *
 * open a socket, bind a port to it, and listen for connections.
 */
int
net_bind(port)
int port;
{
  int s;
  struct sockaddr_in addr;

  s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (s < 0) return(-1);

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons((unsigned short)port);
  
  if (bind(s, (struct sockaddr *)&addr, sizeof(addr))) return(-1);

  if (listen(s, 1) < 0) return(-1);

  return(s);
}

/*
 * net_close
 *
 * close a network connection
 *
 */
void
net_close(s)
int s;
{
  close(s);

  return;
}

/*
 * net_gethostname
 *
 * Returns the full domain name of the local host.  Thanks Jim.
 */
char *
net_gethostname()
{
#ifdef MAXHOSTNAMELEN		/* from <sys/param.h> */
  char myname[MAXHOSTNAMELEN+1];
#else
  char myname[100];
#endif
  struct hostent *hp;
  static char domain[BUFSIZ];

  if (gethostname(myname, sizeof(myname)) < 0) return(NULL);
  if ((hp = (struct hostent *)gethostbyname(myname)) == NULL) return(NULL);
  
  if (strlen(hp->h_name) < sizeof(domain))
  {
    strcpy(domain, hp->h_name);
    return(domain);
  }

  return(NULL);
}

/*
 * net_accept
 *
 */
int
net_accept(s)
int s;
{
  struct sockaddr_in sock;
  int len = sizeof(sock);

  return(accept(s, (struct sockaddr *)&sock, &len));
}
