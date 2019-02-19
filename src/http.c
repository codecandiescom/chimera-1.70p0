/*
 * http.c
 *
 * Copyright (C) 1993, 1994, 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 */
#include "copyright.h"
#include "options.h"

#include <stdio.h>
#include <sys/types.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "common.h"
#include "util.h"
#include "url.h"
#include "mime.h"
#include "document.h"
#include "http.h"
#include "net.h"
#include "input.h"
#include "auth.h"
#include "stringdb.h"

#define DEFAULT_HTTP_PORT 80

/*
 * ECToDS
 */
static void
ECToDS(d, error_code)
Document *d;
int error_code;
{
  if (error_code >= 200 && error_code < 300)
  {
    if (error_code == 204) d->status = DS_NOTHING;
    else d->status = DS_OK;
  }
  else if (error_code >= 300 && error_code < 400)
  {
    if (error_code == 304) d->status = DS_NOTHING;
    else if (error_code == 302)
    {
      d->status = DS_REDIRECT;
      d->cache = 0;
    }
    else d->status = DS_REDIRECT;
  }
  else if (error_code >= 400 && error_code < 500)
  {
    if (error_code == 401) d->status = DS_NEEDS_AUTH;
    else d->status = DS_ERROR;
  }
  else
  {
    d->status = DS_ERROR;
  }
  return;
}

/*
 * ParseHTTPRequest
 */
Document *
ParseHTTPRequest(s)
int s;
{
  Document *d;
  char buffer[BUFSIZ];
  int error_code = 0;
  int tlen;
  int header_end = 0;
  MIMEField *m;

  d = CreateDocument();

  /*
   * Read HTTP request command.
   */
  if (ReadLine(s, buffer, sizeof(buffer)) == -1)
  {
    DestroyDocument(d);
    return(NULL);
  }

/*
 * Don't care what this is right now.
 *
  if (sscanf(buffer, "%s %s %s", command, url, proto) != 3)
  {
    ;
  }
*/

  /*
   * Read the MIME header.
   */
  while (1)
  {
    if (ReadLine(s, buffer, sizeof(buffer)) != 0) break;

    /*
     * End of header?
     */
    if (buffer[0] == '\r' || buffer[0] == '\n')
    {
      header_end = 1;
      break;
    }

    if ((m = CreateMIMEField(buffer)) != NULL)
    {
      ParseMIMEField(d, m);
      m->next = d->mflist;
      d->mflist = m;
    }
  }

  if (!header_end)
  {
    char *msg = GetFromStringDB("abort");

    DestroyDocument(d);

    return(BuildDocument(msg, strlen(msg), "text/html", 1, 0));
  }

  /*
   * This has to be done because a number of servers return the wrong
   * content-length.
   */
  d->text = ReadBuffer(s, &tlen, 0, d->len);
  d->len = tlen;
  ECToDS(d, error_code);

  return(d);
}

/*
 * ParseHTTPResponse
 *
 * This function sorts out the standard HTTP header (which looks like
 * a MIME header) and sorts out the error code and all that jazz.
 * This is a whole bunch of hard-coded MIME string fields and stuff
 * in here.
 */
Document *
ParseHTTPResponse(s)
int s;
{
  Document *d;
  char buffer[BUFSIZ];
  char *t;
  int error_code = 0;
  int tlen;
  int header_end = 0;
  MIMEField *m;

  d = CreateDocument();

				/* We only need 7 characters here, but... */
  t = ReadBuffer(s, &tlen, 8, -8); /* say "Awaiting/Reading response"--GN */
  if (t == NULL) return(NULL);
  
  if (strncmp(t, "HTTP/1.", 7) != 0)
  {				/* non-HTTP/1.* transmission */
    char *x;
    int xlen;
    
    x = ReadBuffer(s, &xlen, 0, 0);
    if (x == NULL)
    {
      d->text = t;
      d->len = tlen;
    }
    else
    {
      d->len = tlen + xlen;
      d->text = alloc_mem(d->len + 1);
      strcpy(d->text, t);
      strcpy(d->text + tlen, x);
      free_mem(t);
      free_mem(x);
    }
    return(d);
  }
    
  free_mem(t);
  
  if (ReadLine(s, buffer, sizeof(buffer)) == -1)
  {
    DestroyDocument(d);
    return(NULL);
  }
  
  sscanf (buffer, "%d", &error_code);

  /*
   * Read the MIME header.
   */
  while (1)
  {
    if (ReadLine(s, buffer, sizeof(buffer)) != 0) break;

    /*
     * End of header?
     */
    if (buffer[0] == '\r' || buffer[0] == '\n')
    {
      header_end = 1;
      break;
    }

    if ((m = CreateMIMEField(buffer)) != NULL)
    {
      ParseMIMEField(d, m);
      m->next = d->mflist;
      d->mflist = m;
    }
  }

  if (!header_end)
  {
    char *msg = GetFromStringDB("abort");

    DestroyDocument(d);

    return(BuildDocument(msg, strlen(msg), "text/html", 1, 0));
  }

  /*
   * This has to be done because a number of servers return the wrong
   * content-length.
   */
  d->text = ReadBuffer(s, &tlen, 0, d->len);
  d->len = tlen;
  ECToDS(d, error_code);

  return(d);
}

/*
 * Formats for HTTP/1.0 requests, for talking to helper applications
 * (\n-terminated lines)  or to an httpd  (\r\n-terminated lines).
 * In the latter case, we now send a Host header so multi-homed HTTP/1.1
 * sites have a chance of figuring out what we want of them.  In the
 * former case, the formats have a dummy %s for convenience. --GN 1997May01
 */

static char *format_get = "\
GET %s HTTP/1.0\n\
%s\
User-Agent: %s\n\
Accept: */*\n\
%s\
%s\
\n";

static char *format_get2 = "\
GET %s?%s HTTP/1.0\n\
%s\
User-Agent: %s\n\
Accept: */*\n\
%s\
%s\
\n";

static char *format_post = "\
POST %s HTTP/1.0\n\
%s\
User-Agent: %s\n\
Accept: */*\n\
Content-length: %d\n\
Content-type: %s\n\
%s\
%s\
\n\
%s\
\n";

static char *format_get_crlf = "\
GET %s HTTP/1.0\r\n\
Host: %s\r\n\
User-Agent: %s\r\n\
Accept: */*\r\n\
%s\
%s\
\r\n";

static char *format_get2_crlf = "\
GET %s?%s HTTP/1.0\r\n\
Host: %s\r\n\
User-Agent: %s\r\n\
Accept: */*\r\n\
%s\
%s\
\r\n";

static char *format_post_crlf = "\
POST %s HTTP/1.0\r\n\
Host: %s\r\n\
User-Agent: %s\n\
Accept: */*\r\n\
Content-length: %d\r\n\
Content-type: %s\r\n\
%s\
%s\
\r\n\
%s\
\r\n";

/*
 * crap for uuencode
 */
static char six2pr[64] =
{
    'A','B','C','D','E','F','G','H','I','J','K','L','M',
    'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    'a','b','c','d','e','f','g','h','i','j','k','l','m',
    'n','o','p','q','r','s','t','u','v','w','x','y','z',
    '0','1','2','3','4','5','6','7','8','9','+','/'
};

/*
 * uuencode
 *
 * This is used to uuencode a string.  It pisses me off that this is even
 * needed.  The WWW guys LIKE complexity and huge code because it keeps
 * them employed (ah wild speculation is good!).
 *
 * I snarfed this code from some version of libwww.
 *
 * ACKNOWLEDGEMENT:
 *      This code is taken from rpem distribution, and was originally
 *      written by Mark Riordan.
 *
 * AUTHORS:
 *      MR      Mark Riordan    riordanmr@clvax1.cl.msu.edu
 *      AL      Ari Luotonen    luotonen@dxcern.cern.ch
 *
 */
static int
uuencode(bufin, nbytes, bufcoded)
unsigned char *bufin;
unsigned int nbytes;
char *bufcoded;
{
  /* ENC is the basic 1 character encoding function to make a char printing */
#define ENC(c) six2pr[c]
  
  register char *outptr = bufcoded;
  unsigned int i;
  
  for (i=0; i<nbytes; i += 3) 
  {
    *(outptr++) = ENC(*bufin >> 2);            /* c1 */
    *(outptr++) = ENC(((*bufin << 4) & 060) | ((bufin[1] >> 4) & 017)); /*c2*/
    *(outptr++) = ENC(((bufin[1] << 2) & 074) | ((bufin[2] >> 6) & 03));/*c3*/
    *(outptr++) = ENC(bufin[2] & 077);         /* c4 */
    
    bufin += 3;
  }
  
  /* If nbytes was not a multiple of 3, then we have encoded too
   * many characters.  Adjust appropriately.
   */
  if (i == nbytes + 1)
  {
    /* There were only 2 bytes in that last group */
    outptr[-1] = '=';
  }
  else if (i == nbytes + 2)
  {
    /* There was only 1 byte in that last group */
    outptr[-1] = '=';
    outptr[-2] = '=';
  }
  *outptr = '\0';

  return(outptr - bufcoded);
}

/*
 * build_auth_info
 *
 * Build the string needed for authorization.
 */
static char *
build_auth_info(type, username, password)
char *type;
char *username;
char *password;
{
  char *garbage;
  char *auth_info;
  static char *format = "Authorization: %s %s\n";

  garbage = alloc_mem(strlen(username) +
		      1 +
		      strlen(password != NULL ? password:"") +
		      1);
  strcpy(garbage, username);
  if (password != NULL) strcat(garbage, ":");
  if (password != NULL) strcat(garbage, password);

  if (strcmp(type, "Basic") == 0)
  {
    char *t;

    t = alloc_mem(strlen(garbage) * 2);
    uuencode(garbage, strlen(garbage), t);
    free_mem(garbage);
    garbage = t;;
  }

  auth_info = alloc_mem(strlen(format) +
			strlen(type) +
			strlen(garbage) + 1);
  sprintf (auth_info, format,
	   type,
	   garbage);

  return(auth_info);
}

/*
 * GetHTTPRequestData
 */
static char *
GetHTTPRequestData(up)
URLParts *up;
{
  char **vlist;
  char **nlist;
  int count;
  char *finfo = NULL;
  char *sep = "";
  int i;
  char *n, *v;
  int flen;
  int oflen;

  nlist = up->attribute_names;
  vlist = up->attribute_values;
  count = up->attribute_count;

  if (nlist != NULL && vlist != NULL)
  {
    /*
     * If there is only one attribute and it is named isindex then
     * treat this like a regular searchable index.  Otherwise
     * do form stuff.
     */
    if (count == 1 && strcasecmp("isindex", nlist[0]) == 0)
    {
      if (vlist[0] == NULL) finfo = alloc_string("");
      else finfo = EscapeURL((unsigned char *)vlist[0], 1);
    }
    else
    {
      finfo = NULL;
      flen = 0;
      for (i = 0; i < count; i++)
      {
	if (nlist[i] == NULL) continue;
	
	n = EscapeURL((unsigned char *)nlist[i], 0);
	
	if (vlist[i] != NULL) v = EscapeURL((unsigned char *)vlist[i], 1);
	else v = alloc_string("");

	/*
	 * length of URL + length of attribute name + length of
	 * attribute value + ampersand/question mark + equal sign + NULL
	 */
	oflen = flen;
	flen = flen + strlen(n) + strlen(v) + strlen(sep) + 1;
	if (finfo != NULL) finfo = realloc_mem(finfo, flen + 1);
	else finfo = alloc_mem(flen + 1);
	sprintf (finfo + oflen, "%s%s=%s", sep, n, v); 
	sep = "&";

	free_mem(n);
	free_mem(v);
      }
    }
  }

  return(finfo);
}

/* 
 * build_request
 *
 * Make an HTTP request string
 * Host header added in crlf mode, GN 1997May01
 */
static char *
build_request(up, filename, extra_header, crlf)
URLParts *up;
char *filename;
char *extra_header;
int crlf;
{
  char *data_type;
  char *query;
  char *auth_info = NULL;
  char *format;
  char *request_data;
  char *host_port;

  if (up->data_type == NULL) data_type = "application/x-www-form-urlencoded";
  else data_type = up->data_type;

  if (filename == NULL) filename = up->filename;
  if (extra_header == NULL) extra_header = "";

  if (up->auth_type != NULL)
  {
    if (up->username != NULL)
    {
      auth_info = build_auth_info(up->auth_type, up->username, up->password);
    }
  }
  if (auth_info == NULL) auth_info = alloc_string("");

  if (crlf != 1)
  {
    host_port = alloc_string("");
  }
  else if (up->port != 0 && up->port != DEFAULT_HTTP_PORT)
  {                           /* assume ports have at most 10 digits ;^\ */
    host_port = alloc_mem(strlen(up->hostname) + 12);
    sprintf(host_port, "%s:%d", up->hostname, up->port);
  }
  else
  {
    host_port = alloc_string(up->hostname);
  }

  if (up->attribute_count > 0)
  {
    if ((request_data = GetHTTPRequestData(up)) == NULL)
    {
      request_data = alloc_string("");
    }

    if (strcasecmp(up->method, "GET") == 0) /* GET ! */
    {
      format = crlf == 1 ? format_get2_crlf:format_get2;
      query = alloc_mem(strlen(filename) +
			strlen(format) +
			strlen(request_data) +
                      strlen(host_port) +
			strlen(extra_header) +
			strlen(auth_info) + 
			strlen(USER_AGENT) + 1);
      sprintf (query,
	       format,
	       filename,
	       request_data,
             host_port,
	       USER_AGENT,
	       extra_header,
	       auth_info);
    }
    else if (strcasecmp(up->method, "POST") == 0) /* POST ! */
    {
      format = crlf == 1 ? format_post_crlf:format_post;
      query = alloc_mem(strlen(filename) +
			strlen(format) +
			strlen(request_data) +
                      strlen(host_port) +
			strlen(data_type) +
			strlen(extra_header) +
			strlen(auth_info) +
			strlen(USER_AGENT) + 1);
      sprintf (query,
	       format,
	       filename,
             host_port,
	       USER_AGENT,
	       strlen(request_data),
	       data_type,
	       extra_header,
	       auth_info,
	       request_data);
    }
    else query = NULL;

    free_mem(request_data);
  }
  else /* regular GET */
  {
    format = crlf == 1 ? format_get_crlf:format_get;
    query = alloc_mem(strlen(filename) +
		      strlen(format) +
                    strlen(host_port) +
		      strlen(extra_header) +
		      strlen(auth_info) + 
		      strlen(USER_AGENT) + 1);
    sprintf (query,
	     format,
	     filename,
           host_port,
	     USER_AGENT,
	     extra_header,
	     auth_info);
  }

  free_mem(auth_info);
  free_mem(host_port);

  return(query);
}

/*
 * http_request
 *
 * Sends an HTTP request
 */
int
http_request(fd, up, type, reload)
int fd;
URLParts *up;
int type;
int reload;
{
  char *query;
  char *extra_header;

  if (type == 0)
  {
    if (reload != 0) extra_header = "Pragma: no-cache\n";
    else extra_header = "";

    query = build_request(up, NULL, extra_header, 1);
  }
  else if (type == 1)
  {
    char *filename;

    if (reload != 0) extra_header = "Pragma: no-cache\n";
    else extra_header = "";

    filename = MakeURL(up, 0);
    query = build_request(up, filename, extra_header, 1);
    free_mem(filename);
  }
  else
  {
    char *url;
    static char *format = "\
URI: %s\n\
X-protocol: %s\n\
X-hostname: %s\n\
X-port: %d\n\
X-filename: %s\n";

    url = MakeURL(up, 0);
    extra_header = alloc_mem(strlen(url) * 5 + strlen(format) + 1);
    sprintf (extra_header,
	     format,
	     url,
	     up->protocol != NULL ? up->protocol:"",
	     up->hostname != NULL ? up->hostname:"",
	     up->port,
	     up->filename != NULL ? up->filename:"/");

    query = build_request(up, NULL, extra_header, 0);
    free_mem(extra_header);
    free_mem(url);
  }

  if (query == NULL) return(-1);

  if (WriteBuffer(fd, query, strlen(query)) < 0)
  {
    free_mem(query);

    return(-1);
  }

  free_mem(query);

  return(0);
}

/*
 * http_main
 *
 * Talk to an HTTP server.
 */
static Document *
http_main(up, rup, mtlist, reload)
URLParts *up;
URLParts *rup;
MIMEType *mtlist;
int reload;
{
  Document *d;
  int s;
  char *hostname;

  /*
   * Start chitchatting with the HTTP server.
   */
  hostname = up->hostname != NULL ? up->hostname:"www";
  s = net_open(hostname, up->port == 0 ? DEFAULT_HTTP_PORT:up->port);
  if (s < 0) return(NULL);

  if (http_request(s, rup != NULL ? rup:up, rup != NULL ? 1:0, reload) == -1)
  {
    net_close(s);
    return(NULL);
  }

  d = ParseHTTPResponse(s);

  net_close(s);

  if (d == NULL) return(NULL);
  else if (d->content == NULL)
  {
    char *content;

    if ((content = Ext2Content(mtlist, up->filename)) == NULL)
    {
      content = "text/html";
    }
    d->content = alloc_string(content);
  }

  if (up->attribute_count > 0) d->cache = 0;

  return(d);
}

/*
 * http
 *
 * Frontend for regular HTTP
 */
Document *
http(up, mtlist)
URLParts *up;
MIMEType *mtlist;
{
  return(http_main(up, NULL, mtlist, 0));
}

/*
 * http_proxy
 *
 * Frontend for proxy HTTP
 */
Document *
http_proxy(up, rup, mtlist, reload)
URLParts *up;
URLParts *rup;
MIMEType *mtlist;
int reload;
{
  return(http_main(up, rup, mtlist, reload));
}
