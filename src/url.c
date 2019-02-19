/*
 * url.c
 *
 * Copyright (C) 1993, 1994, 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 */
#include "copyright.h"
#include "options.h"

#include <stdio.h>
#include <ctype.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "common.h"
#include "url.h"

#define MYDUP(a, b)   a = (b != NULL ? alloc_string(b):NULL)

/*
 * EscapeURL
 *
 * Puts escape codes in URLs.  (More complete than it used to be;
 * GN Jan 1997.  We escape all that isn't alphanumeric, "safe" or "extra"
 * as spec'd in RFCs 1738, 1808 and 2068.)
 */
char *
EscapeURL(url, s2p)
unsigned char *url;
int s2p;
{
  unsigned char *cp;
  char *n, *s;
  static char *hex = "0123456789ABCDEF";

  /*
   * use a bit of memory so i don't have to mess around here
   */
  s = n = alloc_mem(strlen(url) * 3 + 2);

  for (cp = url; *cp; cp++, n++)
  {
    if (*cp == ' ' && s2p != 0)
    {
      *n = '+';
    }
    else if (isalnum(*cp) || strchr("$-_.!*'(),", *cp) || (*cp=='+' && !s2p))
    {
      *n = *cp;
    }
    else
    {
      *n = '%';
      n++;
      *n = hex[*cp / 16];
      n++;
      *n = hex[*cp % 16];
    }
  }

  *n = '\0';

  return(s);
}

/*
 * UnescapeURL
 *
 * Converts the escape codes (%xx) into actual characters.  NOT complete.
 * Could do everthing in place I guess.
 */
char *
UnescapeURL(url)
char *url;
{
  char *cp, *n, *s;
  char hex[3];

  s = n = alloc_mem(strlen(url) + 2);
  if (n == NULL)
  {
    return(NULL);
  }

  for (cp = url; *cp; cp++, n++)
  {
    if (*cp == '%')
    {
      cp++;
      if (*cp == '%')
      {
	*n = *cp;
      }
      else
      {
	hex[0] = *cp;
	cp++;
	hex[1] = *cp;
	hex[2] = '\0';
	*n = (char)strtol(hex, NULL, 16);
      }
    }
    else
    {
      *n = *cp;
    }
  }

  *n = '\0';

  return(s);
}

/*
 * MakeURL
 */
char *
MakeURL(up, addanchor)
URLParts *up;
int addanchor;
{
  int len;
  char *u;
  char *delim;
  char *delim2;
  char *filename;
  char *hostname;
  char *protocol;
  char *delim3;
  char *anchor;

  if (up->protocol != NULL && strncmp(up->protocol, "urn", 3) == 0)
  {
    u = alloc_mem(strlen(up->filename) + 5);
    strcpy(u, "urn:");
    strcat(u, up->filename);
    return(u);
  }

  if (NullString(up->protocol)) protocol = "file";
  else protocol = up->protocol;
  
  if (NullString(up->hostname))
  {
    delim = "";
    hostname = "";
  }
  else
  {
    delim = "//";
    hostname = up->hostname;
  }

  if (NullString(up->filename)) filename = "/";
  else filename = up->filename;

  delim2 = "";

  if (up->anchor != NULL && addanchor != 0)
  {
    anchor = up->anchor;
    delim3 = "#";
  }
  else
  {
    anchor = "";
    delim3 = "";
  }

  /*
   * Uck, there was a 'orrible buffer overrun here due to strlen(anchor)
   * not being taken into account -- GN 1997Apr30
   */
  len = strlen(protocol) + strlen(delim) + strlen(hostname) + strlen(delim2) +
        strlen(filename) + strlen(delim3) + strlen(anchor) + 13;
  u = alloc_mem(len);
  if (up->port == 0)
  {
    sprintf (u, "%s:%s%s%s%s%s%s", protocol, delim, hostname, delim2,
	     filename, delim3, anchor);
  }
  else
  {
    /*
     * max 10 digits for the port #...
     * sprintf doesn't let us to specify a maximum field width  (GN 97Apr30)
     */
    sprintf (u, "%s:%s%s:%d%s%s%s%s", protocol, delim, hostname, up->port,
	     delim2, filename, delim3, anchor);
  }

  return(u);
}

/*
 * CreateURLParts
 *
 * Allocate URLParts and initialize to NULLs
 */
static URLParts *
CreateURLParts()
{
  URLParts *up;

  up = (URLParts *)alloc_mem(sizeof(URLParts));
  up->protocol = NULL;
  up->hostname = NULL;
  up->port = 0;
  up->filename = NULL;
  up->anchor = NULL;

  up->method = NULL;
  up->data_type = NULL;

  up->auth_type = NULL;
  up->username = NULL;
  up->password = NULL;
  up->attribute_names = NULL;
  up->attribute_values = NULL;
  up->attribute_count = 0;

  return(up);
}

/*
 * CopyURLAttributes
 */
static void
CopyURLAttributes(d, s)
URLParts *d, *s;
{
  int i;

  if (s->attribute_count > 0)
  {
    d->attribute_names = (char **)alloc_mem(sizeof(char **) *
					    s->attribute_count);

    d->attribute_values = (char **)alloc_mem(sizeof(char **) *
					     s->attribute_count);

    for (i = 0; i < s->attribute_count; i++)
    {
      d->attribute_names[i] = alloc_string(s->attribute_names[i]);
      d->attribute_values[i] = alloc_string(s->attribute_values[i]);
    }
  }

  d->attribute_count = s->attribute_count;

  return;
}

/*
 * MakeURLParts
 */
URLParts *
MakeURLParts(up1, up2)
URLParts *up1, *up2;
{
  URLParts *r;
  char *filename;

  /*
   * Special case for URNs.  Sucks.
   */
  if (up1->protocol != NULL && strncmp(up1->protocol, "urn", 3) == 0)
  {
    return(DupURLParts(up1));
  }

  r = CreateURLParts();

  /*
   * Deal with the protocol.  If it has a protocol then use it, otherwise
   * use the parent's protocol.  If the parent doesn't have a protocol for
   * some spooky reason then use "file".
   */
  if (up1->protocol == NULL)
  {
    if (up2->protocol != NULL) r->protocol = alloc_string(up2->protocol);
    else r->protocol = alloc_string("file");
  }
  else r->protocol = alloc_string(up1->protocol);

  /*
   * Deal with the hostname.  If it has a hostname then use it, otherwise
   * if the protocol used by the parent is the same then use the parent's
   * hostname.
   *
   * The port goes along with the hostname.
   */
  if (up1->hostname == NULL)
  {
    /*
     * r is used here because up1->protocol can be NULL but r->protocol
     * will always have the right thing.
     */
    if (up2->hostname != NULL && up2->protocol != NULL &&
        strcasecmp(r->protocol, up2->protocol) == 0)
    {
      r->hostname = alloc_string(up2->hostname);
      r->port = up2->port;
    }
  }
  else
  {
    r->hostname = alloc_string(up1->hostname);
    r->port = up1->port;
  }

  /*
   * Deal with the filename.
   */
  if (up1->filename == NULL)
  {
    r->filename = alloc_string("/");
  }
  /*
   * This 'else if' used to be 'if', thought I'd change it -- GN97Apr19
   */
  else if (up2->protocol == NULL  ||
	   strcasecmp(r->protocol, up2->protocol) != 0  ||
	   up1->filename[0] == '/')
  {
    r->filename = alloc_string(up1->filename);
  }
  /*
   * Assume that HREF="~luser" is a typo for HREF="/~luser", which is
   * often true but might sometimes be mistaken.
   * (Duh, if everybody had read the rules we should never even see a tilde;
   * they're supposed to appear as %7E's.  NB _I've_ read that rule, and
   * decided to ignore it on my own pages ;^)
   * -- code by WBE, comment by GN 97Apr20
   */
  else if (up1->filename[0] == '~')
  {
    r->filename = alloc_mem(strlen(up1->filename) + 2);
    r->filename[0] = '/';
    strcpy(r->filename + 1, up1->filename);
  }
  else
  {
    /*
     * Handle a relative URL up1 with respect to base up2.
     * This code assumes (wildly, cf. RFC1738) that slashes are field
     * separators, that things between slashes act like UNIX directory
     * names, and that the special `directory names' ".." and "." act
     * the same they would in the UNIX world when they occur at the start
     * of the up1 filename.  None of this is _necessarily_ true  (and for
     * FTP URLs some of this _is_ untrue).
     * [In FTP URLs, slashes _are_ separators, dots _do_ act like that
     * when the FTP host is UNIX-like, and an extra initial slash has
     * extra significance -- but that case was handled safely above.
     * See RFC1738.  NB Chimera currently does NOT implement that correctly,
     * and has a habit of adding spurious slashes in the HREFs it generates
     * within an FTP directory display.]
     *
     * We also do not bother to look for any leftover "."s and ".."s
     * in up2.  Presumably they were exorcised earlier.
     * -- code by WBE, comment by GN 97Apr20
     */
    char *start1, *end2;
    int backcount = 0;
    int len;

    start1 = up1->filename;
    while (*start1 == '.')
    {
      char c = start1[1];
      if (c == '.')		/* matched ".." so far */
      {
	c = start1[2];
	if (c == '/')		/* "../" */
	{
	  start1 += 3;
	  backcount++;
	}
	else if (c == '\0')	/* remaining filename = ".." */
	{
	  start1 += 2;          /* we infer a missing trailing slash */
	  backcount++;
	}
	else break;		/* some file name beginning with ".." */
      }
      else if (c == '/')	/* "./" */
      {
	start1 += 2;		/* just flush it */
      }
      else if (c == '\0')	/* remaining filename = "." */
      {
	++start1;		/* flush it */
      }
      else break;		/* some file name beginning with '.' */
    } /* end while */

    /*
     * delete last name from up2, if any, and then backcount more names and
     * intervening slashes.  We try to be pointing at a slash when we stop.
     */
    end2 = up2->filename + strlen(up2->filename) - 1;  /* last char */
    for ( ;  end2 >= up2->filename;  --end2)
    {
      if (*end2 == '/'  &&  backcount-- == 0)
	  break;
    }
    /*
     * If the URL is a file name (up2->filename == '/'), then make sure
     * r->filename begins with a '/' and don't let relative paths with
     * excess "../"s back up past the root '/'.
     */
    if (end2 < up2->filename  &&  up2->filename[0] == '/')
    {
      end2 = up2->filename;	/* force a leading '/' for file names */
    }
    len = end2 - up2->filename + 1;

    if (len == 0)		/* a non-file URL with no parent part left */
    {
      r->filename = alloc_string(start1);
    }
    /*
     * else combine base part (from up2) with relative part (from up1)
     */
    else
    {
      /*
       * minor optimization for <A HREF="../"> and friends
       */
      if (*start1 == '\0')
      {
	r->filename = alloc_mem(len + 1);
	strncpy(r->filename, up2->filename, len);
	r->filename[len] = '\0';  /* strncpy hasn't terminated it */
      }
      else
      {
	r->filename = alloc_mem(len + strlen(start1) + 1);
	strncpy(r->filename, up2->filename, len);
	strcpy(r->filename + len, start1);
      }
    }
  }

  /*
   * Copy misc. fields.
   */
  MYDUP(r->method, up1->method);
  MYDUP(r->data_type, up1->data_type);
  MYDUP(r->auth_type, up1->auth_type);
  MYDUP(r->username, up1->username);
  MYDUP(r->password, up1->password);
  MYDUP(r->anchor, up1->anchor);
  CopyURLAttributes(r, up1);

  return(r);
}

URLParts *
DupURLParts(up)
URLParts *up;
{
  URLParts *dp;

  dp = CreateURLParts();
  MYDUP(dp->method, up->method);
  MYDUP(dp->protocol, up->protocol);
  MYDUP(dp->hostname, up->hostname);
  dp->port = up->port;

  dp->filename = up->filename != NULL ?
      alloc_string(up->filename):alloc_string("/");

  MYDUP(dp->anchor, up->anchor);

  MYDUP(dp->data_type, up->data_type);

  MYDUP(dp->auth_type, up->auth_type);
  MYDUP(dp->username, up->username);
  MYDUP(dp->password, up->password);

  CopyURLAttributes(dp, up);

  return(dp);
}

/*
 * ParseURL
 *
 * Turns a URL into a URLParts structure
 *
 * The good stuff was written by Rob May <robert.may@rd.eng.bbc.co.uk>
 * and heavily mangled/modified by john to suit his own weird style.
 * Made somewhat smarter (err, completely re-written) by GN 1997May02
 */
URLParts *
ParseURL(url)
char *url;
{
  URLParts *up;
  char *start, *s, *t;
  char *fragmark;             /* '#' fragment marker if any */
  /* NB Fragments  (which the chimera source calls 'anchors' are part
   * of HTML href's but _not_ properly speaking of URLs;  they are handled
   * entirely at the client end and not by the server.
   * Nevertheless we look for them  (this routine should really be called
   * ParseHREF)  and store a fragment identifier separately if we find one.
   * --GN
   */

  up = CreateURLParts();

  /* RFC1738 says spaces in URLs are to be ignored -- GN 1997May02 */
  t = start = (char *)alloc_mem(strlen(url) + 1);
  for (s = url; *s; s++)
    if (!isspace(*s))
      *t++ = *s;
  *t = '\0';

  /* Lousy hack for URNs */
  if (strncasecmp(start, "urn:", 4) == 0)
  {
    up->protocol = alloc_string("urn");
    up->filename = alloc_string(start + 4);
    free_mem(start);
    return(up);
  }
  /* Less lousy hack for URLs which say so */
  if (strncasecmp(start, "url:", 4) == 0)
    s = start + 4;
  else
    s = start;

  /*
   * Check to see if there is a protocol (scheme) name.
   * Matches /^[A-Za-z0-9\+\-\.]+:/ in PERLese.
   */
  for (t = s ; *t; t++)
  {
    if (!isalnum(*t) && *t != '-' && *t != '+' && *t != '.')
      break;
  }
  if (*t == ':')
  {
    up->protocol = alloc_mem(t - s + 1);
    strncpy(up->protocol, s, t - s);
    up->protocol[t - s] = '\0';
    s = ++t;
  }

  /*
   * Check whether this is an 'Internet' URL i.e. the next bit begins
   * with "//".  In this case, what follows up to the next slash ought
   * to parse as "//user:passwd@host.dom.ain:port/" with almost every
   * component optional, and we'll continue later with s pointing at the
   * trailing slash.  If there is no further slash, we'll add one and
   * return.-- None of the fields are supposed to contain any visible
   * (unencoded)  colons, slashes or atsigns.
   */
  if (s[0] == '/'  &&  s[1] == '/')  /* looking at "//" */
  {
    char *atsign;             /* if present, user:passwd precedes it */
    char *colon;              /* colon separators after user or host */
    char *tslash;             /* trailing slash */

    s += 2;
    tslash = strchr(s, '/');
    if (tslash != NULL)
      *tslash = '\0';         /* split the string, we'll undo this later */

    atsign = strchr(s, '@');

    if (atsign != NULL)	      /* a username is present, possibly empty */
    {
      *atsign = '\0';         /* split the string again */
      colon = strchr(s, ':');

      if (colon != NULL)      /* a passwd is also present */
      {
      *colon = '\0';
      up->password = alloc_string(atsign + 1);
      }

      up->username = alloc_string(s);
      s = atsign + 1;
    }

    colon = strchr(s, ':');

    if (colon != NULL)		/* a port is specified */
    {
      *colon = '\0';
      up->port = atoi(colon + 1);
    }

    up->hostname = alloc_string(s);

    if (tslash == NULL)		/* nothing further */
    {
      up->filename = alloc_string("/");
      free_mem(start);
      return(up);
    }

    *tslash = '/';		/* restore the slash */
    s = tslash;			/* and stay there, don't step beyond */
  }

  /*
   * End of special treatment of Internet URLs.  Now s points at what
   * chimera calls the filename part  (if any).
   */
  fragmark = strchr(s, '#');

  if (fragmark != NULL)
  {
    *fragmark = '\0';
    up->anchor = alloc_string(fragmark + 1);
  }

  up->filename = alloc_string(s);  /* everything else goes here */

  free_mem(start);
  return(up);
}


/*
 * DestroyURLParts
 *
 * Destroys a URLParts structure.
 */
void
DestroyURLParts(up)
URLParts *up;
{
  int i;

  if (up->protocol) free_mem(up->protocol);
  if (up->hostname) free_mem(up->hostname);
  if (up->filename) free_mem(up->filename);
  if (up->anchor) free_mem(up->anchor);

  if (up->method) free_mem(up->method);
  if (up->data_type) free_mem(up->data_type);

  if (up->auth_type) free_mem(up->auth_type);
  if (up->username) free_mem(up->username);
  if (up->password) free_mem(up->password);

  if (up->attribute_names != NULL)
  {
    for (i = 0; i < up->attribute_count; i++)
    {
      if (up->attribute_names[i] != NULL) free_mem(up->attribute_names[i]);
    }
    free_mem((char *)up->attribute_names);
  }

  if (up->attribute_values != NULL)
  {
    for (i = 0; i < up->attribute_count; i++)
    {
      if (up->attribute_values[i] != NULL) free_mem(up->attribute_values[i]);
    }
    free_mem((char *)up->attribute_values);
  }

  free_mem((char *)up);

  return;
}

/*
 * IsAbsoluteURL
 *
 * Returns 1 if the URLParts given is absolute, 0 otherwise.
 */
int
IsAbsoluteURL(up)
URLParts *up;
{
  if (up->protocol == NULL) return(0);

  return(1);
}
