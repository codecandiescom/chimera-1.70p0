/*
 * input.c
 *
 * Copyright (C) 1994, 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 */
#include "copyright.h"
#include "options.h"

#include <stdio.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <signal.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/types.h>
#include <errno.h>

#ifndef sco
#if defined(SYSV) || defined(SVR4)
#include <sys/select.h>
#endif
#endif

#include "common.h"
#include "util.h"
#include "net.h"
#include "input.h"
#include "stringdb.h"

extern int DisplayTransferStatus _ArgProto((int, int, int));
extern int DisplayConnectStatus _ArgProto((int, char *));
				/* these are defined in src/main.c */

int response_bytecnt = 0;	/* used by back-to-back ReadLine calls */

/*
 * WaitForConnect
 */
int
WaitForConnect(d)
int d;
{
  int rval;
  fd_set writefds;
  struct timeval to;
  extern int errno;

  while (1)
  { 
    FD_ZERO(&writefds);
    FD_SET(d, &writefds);
    to.tv_sec = NET_WAIT_TO_SECS;
    to.tv_usec = NET_WAIT_TO_USECS;

    if ((rval = select(d + 1, (fd_set *)0, &writefds, (fd_set *)0, &to)) > 0)
    {    
      if (FD_ISSET(d, &writefds)) break;
    }
    else if (rval < 0 && errno != EINTR) return(-1);
				/* following says "Connecting..."--GN */
    if (DisplayConnectStatus(1, NULL) == 1) return(-1);
  }
				/* and "Connected" */
  if (DisplayConnectStatus(2, NULL) == 1) return(-1);
  return(0);
}

/*
 * WriteBuffer
 */
int
WriteBuffer(d, buffer, bufferlen)
int d;
char *buffer;
int bufferlen;
{
  int rval;
  int writelen;
  fd_set writefds;
  struct timeval to;
  int i;
  extern int errno;

  response_bytecnt = 0;		/* sanity */

  for (i = 0; i < bufferlen; i += writelen)
  {
    FD_ZERO(&writefds);
    FD_SET(d, &writefds);
    to.tv_sec = NET_WAIT_TO_SECS;
    to.tv_usec = NET_WAIT_TO_USECS;

    if ((rval = select(d + 1, (fd_set *)0, &writefds, (fd_set *)0, &to)) > 0)
    {    
      if (FD_ISSET(d, &writefds))
      {
	writelen = write(d, buffer + i, bufferlen - i);
	if (writelen < 0)
	{
	  if (errno != EAGAIN) break;
	}
      }
    }
    else if (rval < 0 && errno != EINTR) return(-1);
				/* and this says "Connected, sending req..." */
    if (DisplayConnectStatus(3, NULL) == 1) return(-1);
  }
				/* and "Sent request...." */
  if (DisplayConnectStatus(4, NULL) == 1) return(-1);
  return(0);
}

/*
 * ReadLine
 *
 * Read a line from a descriptor.  This is really inefficient but it was
 * easy to do.  What is needed is an fselect() so the stdio stuff can be
 * used.
 */
int
ReadLine(d, buffer, bufferlen)
int d;
char *buffer;
int bufferlen;
{
  fd_set readfds;
  struct timeval to;
  int i = 0;
  int rval;
  extern int errno;

  if (bufferlen == 0) return(-1);

  while (1)
  {
    if (i == bufferlen - 1)
    {
      buffer[i] = '\0';
      return(0);
    }

    FD_ZERO(&readfds);
    FD_SET(d, &readfds);
    to.tv_sec = NET_WAIT_TO_SECS;
    to.tv_usec = NET_WAIT_TO_USECS;

    if ((rval = select(d + 1, &readfds, (fd_set *)0, (fd_set *)0, &to)) > 0)
    {    
      if (FD_ISSET(d, &readfds))
      {
	rval = read(d, buffer + i, 1);
	if (rval == -1) return(-1);
	else if (rval > 0)
	{
	  if (buffer[i] == '\n')
	  {
	    buffer[i + 1] = '\0';
	    if (DisplayTransferStatus(response_bytecnt, -1, 1) == 1)
	      return(-1);
	    return(0);
	  }
	}
	else return(-1);
	i++;
	response_bytecnt++;
      }
    }
    else if (rval < 0 && errno != EINTR) break;
				/* always say "Reading response...": --GN */
    if (DisplayTransferStatus(response_bytecnt, -1, 0) == 1) break;
  }

  return(-1);
}

/*
 * ReadBuffer
 *
 * Reads data from a stream and makes a buffer for it.  The buffer is 1
 * byte larger than it needs to be to accomodate a NULL byte.
 *
 * It reads up to max bytes from the file descriptor s.  If max is zero then
 * it reads until it can't read no more.  The number of bytes read  (not
 * counting the NULL byte we've added)  is returned in *len.
 *
 * If bmax is zero then DisplayTransferStatus won't display the
 * "out of %d loaded".  If bmax is negative it'll display "Reading response".
 *
 * ReadBuffer also bumps response_bytecnt (because we use it to read the
 * first 8 bytes of an HTTP response).
 */
char *
ReadBuffer(d, len, max, bmax)
int d;
int *len;
int max;
int bmax;
{
  int blen = 0;
  int tlen = 0;
  int btlen = 0;
  int barp;
  char buffer[BUFSIZ];
  char *t = NULL;
  int rval;
  int readlen;
  fd_set readfds;
  struct timeval to;

  for (readlen = max; max == 0 || readlen > 0; readlen -= blen)
  {
    FD_ZERO(&readfds);
    FD_SET(d, &readfds);
    to.tv_sec = NET_WAIT_TO_SECS;
    to.tv_usec = NET_WAIT_TO_USECS;
    blen = 0;

    if ((rval = select(d + 1, &readfds, (fd_set *)0, (fd_set *)0, &to)) > 0)
    {    
      if (FD_ISSET(d, &readfds))
      {
	barp = (readlen > sizeof(buffer) || max == 0
		? sizeof(buffer):readlen);
	blen = read(d, buffer, barp);
	if (blen <= 0) break;
	tlen += blen;
	if (t) t = (char *)realloc_mem(t, tlen + 1);
	else t = (char *)alloc_mem(tlen + 1);
	memcpy(t + btlen, buffer, blen);
	btlen = tlen;
      }
    }
    else if (rval < 0 && errno != EINTR)
    {
      if (t != NULL) free_mem(t);
      return(NULL);
    }

    if (DisplayTransferStatus(tlen, bmax, 0) == 1)
    {
      if (t != NULL) free_mem(t);
      return(NULL);
    }
  }

  if (t != NULL)
  {
    t[tlen] = '\0';
    *len = tlen;
    response_bytecnt += tlen;
  }

  if (DisplayTransferStatus(tlen, bmax, 1) == 1) /* force update */
  {
    if (t != NULL) free_mem(t);
    return(NULL);
  }

  return(t);
}

/*
 * ReadPipe
 */
char *
ReadPipe(command, path, in, inlen, out, outlen)
char *command;
char *path;
char *in;
int inlen;
char **out;
int *outlen;
{
  char filename[L_tmpnam + 1];
  char *cmdline;
  char *data;
  int datalen;
  int fd[2];
  char *emsg = NULL;

  if (SaveData(in, inlen, tmpnam(filename)) == -1)
  {
    return(alloc_string(GetFromStringDB("nosave")));
  }

  cmdline = CreateCommandLine(command, path, filename);

  if (PipeCommand(cmdline, fd, 1) != 0)
  {
    char *msg = GetFromStringDB("noexec");

    emsg = alloc_mem(strlen(msg) + strlen(cmdline) + 1);
    strcpy(emsg, msg);
    strcat(emsg, cmdline);
    free_mem(cmdline);
    unlink(filename);
    return(emsg);
  }

  data = ReadBuffer(fd[1], &datalen, 0, 0);
  if (data == NULL)
  {
    char *msg = GetFromStringDB("nopipedata");
    
    emsg = alloc_mem(strlen(msg) + strlen(cmdline) + 1);
    strcpy(emsg, msg);
    strcat(emsg, cmdline);
  }
  else
  {
    *out = data;
    *outlen = datalen;
  }

  close(fd[0]);
  close(fd[1]);

  free_mem(cmdline);
  unlink(filename);

  return(emsg);
}

/*
 * PipeCommand
 *
 * fork and exec to get a program running and supply it with
 * a stdin, stdout, stderr so that we can talk to it.
 */
int
PipeCommand(command, fd, usesystem)
char *command;
int *fd;
int usesystem;
{
  int pout[2];
  int pin[2];
  int pid;


  if (pipe(pout) == -1) return(-1);

  if (pipe(pin) == -1)
  {
    close(pout[0]);
    close(pout[1]);
    return(-1);
  }

  pid = (int)fork();
  if (pid == -1)
  {
    return(-1);
  }
  else if (pid == 0)
  {
#ifdef CHILD_STDERR
    int perr;
    char *filename;

    filename = FixFilename(CHILD_STDERR);
    if (filename == NULL) filename = "/dev/null";

    perr = open(filename, O_WRONLY | O_CREAT, 0);
    if (perr >= 0)
    {
      dup2(perr, 2);
      close(perr);
    }
#endif

    if (pout[1] != 1)
    {
      dup2(pout[1], 1);
      close(pout[1]);
    }

    if (pin[0] != 0)
    {
      dup2(pin[0], 0);
      close(pin[0]);
    }

    signal(SIGPIPE, SIG_DFL);

    if (usesystem)
    {
      system(command);
      exit(0);
    }
    else
    {
      execl(command, command, (char *)0);
      exit(1);
    }
  }
  else
  {
    close(pout[1]);
    close(pin[0]);
  }

  fd[0] = pin[1];
  fd[1] = pout[0];

  return(0);
}
