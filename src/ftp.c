/*
 * ftp.c
 *
 * Copyright (C) 1993, 1994, 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 */
#include "copyright.h"
#include "options.h"

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "common.h"
#include "url.h"
#include "mime.h"
#include "document.h"
#include "ftp.h"
#include "net.h"
#include "input.h"
#include "stringdb.h"

#define DEFAULT_FTP_PORT 21

static char *ftp_dir _ArgProto((int, char *, int, char *));
static int soak _ArgProto((int, char *, char **));

/*
 * soak
 * 
 * This function is used to "soak up" the multiline BS that some
 * ftp servers spew (not that it is bad to have nice long
 * informational messages its just that I hate them from my
 * programmer's point of view).
 */
static int
soak(s, msg, emsg)
int s;
char *msg;
char **emsg;
{
  char buffer[BUFSIZ];
  int code;
  int rval;
  char *t;
  int tlen, blen, btlen;

  if (msg != NULL)
  {
    if (WriteBuffer(s, msg, strlen(msg)) == -1) return(999);
  }

  t = NULL;
  tlen = 0;
  btlen = 0;
  code = -1;
  while ((rval = ReadLine(s, buffer, sizeof(buffer))) == 0)
  {
    blen = strlen(buffer);
    tlen += blen;
    if (t != NULL) t = (char *)realloc_mem(t, tlen + 1);
    else t = (char *)alloc_mem(tlen + 1);
    memcpy(t + btlen, buffer, blen);
    t[tlen] = '\0';
    btlen = tlen;

    if (code == -1) code = atoi(buffer);
    if (buffer[3] == ' ' && buffer[0] > ' ') break;
  }

  if (emsg != NULL)
  {
    if (t == NULL) *emsg = alloc_string("");
    else *emsg = t;
  }

  if (rval < 0 && code == -1) return(999);

  return(code);
}

/*
 * fixup_ftp_message
 *
 */
static Document *
fixup_ftp_message(ftpmsg)
char *ftpmsg;
{
  char *msg = GetFromStringDB("ftperror");
  char *r;
  Document *d;

  r = alloc_mem(strlen(msg) + strlen(ftpmsg) + 1);
  sprintf (r, msg, ftpmsg);
  free_mem(ftpmsg);
  d = BuildDocument(r, strlen(r), "text/html", 0, 0);
  d->status = DS_ERROR;
  return(d);
}

/*
 * ftp
 *
 * Anonymous FTP interface
 *
 * This is getting to be quite large and that is without all of the
 * necessary error checking being done.
 */
Document *
ftp(up, mtlist)
URLParts *up;
MIMEType *mtlist;
{
  char *emsg;
  char *query;
  char data_hostname[48];
  char *t;
  char *domain;
  char *uname;
  char *filename;
  int s, d;
  int h0, h1, h2, h3, p0, p1, reply, n;
  int data_port;
  int tlen = 0;
  int size;
  char *hostname;
  char *content;
  static char *format = "PASS -%s@%s\r\n"; 
  static char *format2 = "RETR %s\r\n";
  static char *format3 = "CWD %s\r\n";
  static char *format4 = "SIZE %s\r\n";

  filename = up->filename;

  /*
   * Contact the ftp server on the usualy port.  Hardcoding ports is bad.
   */
  hostname = up->hostname != NULL ? up->hostname:"ftp";
  s = net_open(hostname, (up->port == 0 ? DEFAULT_FTP_PORT:up->port));
  if (s < 0) return(NULL);

  /*
   * wait for the connect() to succeed
   */
#ifdef NONBLOCKING_CONNECT
  if (WaitForConnect(s) < 0) return(NULL);
#endif

  /*
   * Take care of the greeting.
   */
  if (soak(s, NULL, &emsg) >= 400) 
  {
    net_close(s);
    return(fixup_ftp_message(emsg));
  }
  free_mem(emsg);

  /*
   * Send the user name
   */
  if (soak(s, "USER anonymous\r\n", &emsg) >= 400)
  {
    net_close(s);
    return(fixup_ftp_message(emsg));
  }
  free_mem(emsg);

  domain = net_gethostname();
  if (domain == NULL)
  {
    net_close(s);
    return(NULL);
  }

  /*
   * Send the password
   */
  if ((uname = NGetFromStringDB("email")) != NULL
      || (uname = getenv("EMAIL")) != NULL)
  {
    query = alloc_mem(strlen(uname) + 9);
    strcpy(query, "PASS -");
    strcat(query, uname);
    strcat(query, "\r\n");
  }
  else
  {
    if ((uname = getenv("USER")) == NULL) uname = "nobody";
    query = alloc_mem(strlen(uname) + 
			   strlen(domain) + strlen(format) + 1);
    sprintf(query, format, uname, domain);
  }

  if (soak(s, query, &emsg) >= 400)
  {
    net_close(s);
    return(fixup_ftp_message(emsg));
  }
  free_mem(emsg);
  free_mem(query);

  /*
   * Set binary transfers
   */
  if (soak(s, "TYPE I\r\n", &emsg) >= 400)
  {
    net_close(s);
    return(fixup_ftp_message(emsg));
  }

  /*
   * Set passive mode and grab the port information
   */
  if (soak(s, "PASV\r\n", &emsg) >= 400)
  {
    net_close(s);
    return(fixup_ftp_message(emsg));
  }

  n = sscanf(emsg, "%d %*[^(] (%d,%d,%d,%d,%d,%d)", &reply, &h0, &h1, &h2, &h3, &p0, &p1);
  if (n != 7 || reply != 227)
  {
    char *msg = GetFromStringDB("ftpweirdness");

    net_close(s);
    free_mem(emsg);
    return(fixup_ftp_message(msg));
  }
  free_mem(emsg);

  sprintf (data_hostname, "%d.%d.%d.%d", h0, h1, h2, h3);

  /*
   * Open a data connection
   */
  data_port = (p0 << 8) + p1;
  d = net_open(data_hostname, data_port);
  if (d < 0)
  {
    net_close(s);
    return(NULL);
  }

  query = alloc_mem(strlen(filename) + strlen(format4) + 1);
  sprintf (query, format4, filename);
  if (soak(s, query, &emsg) == 999)
  {
    net_close(s);
    net_close(d);
    free_mem(query);
    return(fixup_ftp_message(emsg));
  }
  sscanf(emsg, "%d %d", &reply, &size);
  if (reply != 213) size = 0;
  free_mem(query);
  free_mem(emsg);

  /*
   * Try to retrieve the file
   */
  query = alloc_mem(strlen(filename) + strlen(format2) + 1);
  sprintf(query, format2, filename);
  if ((reply = soak(s, query, &emsg)) == 999)
  {    
    net_close(s);
    net_close(d);
    free_mem(query);
    return(fixup_ftp_message(emsg));
  }
  free_mem(query);
  free_mem(emsg);

  /*
   * If the retrieve fails try to treat the file as a directory.
   * If the file is a directory then ask for a listing.
   */
  if (reply >= 400)
  {
    /*
     * Try to read the file as a directory.
     */
    query = alloc_mem(strlen(filename) + strlen(format3) + 1);
    sprintf (query, format3, filename);
    if (soak(s, query, &emsg) >= 400)
    {
      net_close(d);
      net_close(s);
      free_mem(query);
      return(fixup_ftp_message(emsg));
    }
    free_mem(query);
    free_mem(emsg);

    if (soak(s, "NLST\r\n", &emsg) >= 400)
    {
      net_close(s);
      net_close(d);
      return(fixup_ftp_message(emsg));
    }
    free_mem(emsg);

    t = ftp_dir(d, hostname, (up->port == 0 ? DEFAULT_FTP_PORT:up->port),
		filename);
    if (t == NULL) return(NULL);
    content = "text/html";
    tlen = strlen(t);
  }
  else
  {
    /*
     * Read file from the FTP host
     */
    t = ReadBuffer(d, &tlen, size, size);
    if (t == NULL) return(NULL);
    
    content = Ext2Content(mtlist, filename);
    if (content == NULL) content = "text/plain";
  }

  net_close(d);
  net_close(s);
    
  return(BuildDocument(t, tlen, content, 0, 1));
}

static int
ftp_strcmp(a, b)
char **a, **b;
{
  return(strcmp(*a, *b));
}

/*
 * ftp_dir
 *
 * Read directory from FTP server, sort, and display.
 */
static char *
ftp_dir(d, hostname, portno, filename)
int d;
char *hostname;
int portno;
char *filename;
{
  int count;
  int size;
  int rval;
  char **sa;
  char buffer[BUFSIZ];
  char *cp;
  char *f;
  int flen;
  int filelen;
  int entrylen;
  int i;
  char *header = GetFromStringDB("ftpheader");
  static char *entry = "<li> <a href=ftp://%s:%d%s/%s> %s </a>\n";

  count = 0;
  size = 15;
  sa = (char **)alloc_mem(size * sizeof(char *));
  while ((rval = ReadLine(d, buffer, sizeof(buffer))) == 0)
  {
    if (count >= size)
    {
      size *= 4;
      sa = (char **)realloc_mem((char *)sa, size * sizeof(char *));
    }
    for (cp = buffer; *cp; cp++)
    {
      if (isspace(*cp)) break;
    }
    *cp = '\0';
    sa[count] = alloc_string(buffer);
    count += 1;
  }

  if (count == 0)
  {
    free_mem((char *)sa);
    return(NULL);
  }

  qsort(sa, count, sizeof(char *), ftp_strcmp);

  filelen = strlen(filename) + strlen(hostname) + 10;
  entrylen = strlen(entry);

  flen = strlen(header) + 2 * filelen + 1;
  f = (char *)alloc_mem(flen);
  sprintf (f, header, filename, hostname, filename);

  if (filename[0] != '\0' && filename[1] == '\0') filename = "";

  for (i = 0; i < count; i++)
  {
    flen += 2 * strlen(sa[i]) + filelen + entrylen;
    f = (char *)realloc_mem(f, flen);
    sprintf(f + strlen(f), entry, hostname, portno, filename, sa[i], sa[i]);
    free_mem(sa[i]);
  }
  free_mem((char *)sa);

  return(f);
}
