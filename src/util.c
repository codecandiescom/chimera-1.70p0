/*
 * util.c
 *
 * Copyright (C) 1993, 1994, 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 */
#include "copyright.h"

#include "options.h"

#include <stdio.h>
#include <pwd.h>
#include <errno.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <sys/param.h>

#include <ctype.h>

#include "common.h"
#include "util.h"

/*
 * FixFilename
 *
 * The only thing this does right now is to handle the '~' stuff.
 */
char *
FixFilename(filename)
char *filename;
{
  struct passwd *p;
  char *cp, *cp2;
  char username[BUFSIZ];
  char *fname;
  char *home;
  static char newfilename[MAXPATHLEN + 1];

  if (filename[0] == '~') fname = filename;
  else if (filename[0] == '/' && filename[1] == '~') fname = filename + 1;
  else fname = NULL;

  if (fname != NULL)
  {
    if (fname[1] == '/')
    {
      if ((home = getenv("HOME")) == NULL) return(NULL);
      cp = fname + 1;
    }
    else
    {
      for (cp = fname + 1, cp2 = username; *cp && *cp != '/'; cp++, cp2++)
      {
	*cp2 = *cp;
      }
      *cp2 = '\0';

      p = getpwnam(username);
      if (p == NULL) return(NULL);
      home = p->pw_dir;
    }

    if (strlen(home) + strlen(cp) > MAXPATHLEN) return(NULL);
    strcpy(newfilename, home);
    strcat(newfilename, cp);
  }
  else
  {
    strcpy(newfilename, filename);
  }

  return(newfilename);
}

/*
 * SaveData
 *
 * Save some memory to a file.  Returns 0 if successful, -1 otherwise.
 */
int
SaveData(data, datalen, filename)
char *data;
int datalen;
char *filename;
{
  FILE *fp;

  if (filename[0] == '|') fp = popen(filename + 1, "w");
  else fp = fopen(filename, "w");
  if (fp == NULL) return(-1);

  if (fwrite(data, 1, datalen, fp) < datalen)
  {
    if (filename[0] == '|') pclose(fp);
    else
    {
      fclose(fp);
      unlink(filename);
    }
  }

  if (filename[0] == '|') pclose(fp);
  else fclose(fp);

  return(0);
}

/*
 * ReapChild
 *
 * This code grabs the status from an exiting child process.  We really
 * don't care about the status, we just want to keep zombie's from
 * cropping up.
 */
static void
ReapChild()
{
#if defined(WNOHANG) && !defined(SYSV) && !defined(SVR4)
  int pid;
#endif
  extern int errno;
  int old_errno = errno;

 /*
  * It would probably be better to use the POSIX mechanism here,but I have not 
  * checked into it.  This gets us off the ground with SYSV.  RSE@GMI
  */
#if defined(WNOHANG) && !defined(SYSV) && !defined(SVR4)
  union wait st;

  do 
  {
    errno = 0;
    pid = wait3(&st, WNOHANG, 0);
  }
  while (pid <= 0 && errno == EINTR);
#else
  int st;

  wait(&st);
#endif
  StartReaper();
  errno = old_errno;

  return;
}

/*
 * StartReaper
 *
 * This code inits the code which reaps child processes that were
 * fork'd off for external viewers.
 */
void
StartReaper()
{
#ifdef SIGCHLD
  signal(SIGCHLD, ReapChild);
#else
  signal(SIGCLD, ReapChild);
#endif

  return;
}

/*
 * mystrtok
 *
 */
char *
mystrtok(s, c, cdr)
char *s;
int c;
char **cdr;
{
  char *cp, *cp2;
  static char str[BUFSIZ];

  if (s == NULL) return(NULL);

  for (cp = s, cp2 = str; ; cp++)
  {
    if (*cp == '\0') break;
    else if (*cp == c)
    {
      for (cp++; *cp == c; )
      {
	cp++;
      }
      break;
    }
    else *cp2++ = *cp;
  }

  *cp2 = '\0';

  if (*cp == '\0') *cdr = NULL;
  else *cdr = cp;

  return(str);
}

/*
 * CreateCommandLine
 */
char *
CreateCommandLine(command, path, filename)
char *command, *path, *filename;
{
  char *cmdline;

  cmdline = alloc_mem(strlen(command) + strlen(filename) + 1);
  sprintf (cmdline, command, filename);

  if (path != NULL && path[0] != '\0')
  {
#ifdef NOSEMICOLON
    char *format = "PATH=$PATH:%s %s";
#else
    char *format = "PATH=$PATH:%s; export PATH; %s";
#endif
    char *t = cmdline;

    cmdline = alloc_mem(strlen(format) + strlen(path) + strlen(t) + 1);
    sprintf (cmdline, format, path, t);
    free_mem(t);
  }

  return(cmdline);
}

/*
 * EscapeHTML
 *
 * Allocates a new string, and copies text into it, escaping all HTML
 * active characters. The string should be freed with free()
 *
 * This uses a two-pass algorithm.
 * Contributed by David.Robinson@ast.cam.ac.uk
 */
char *
EscapeHTML(text)
char *text;
{
  int j, i;
  char *ret;
    
  /* count length and number of active characters in string */
  j = 0;
  for (i = 0; text[i] != '\0'; i++)
  {
    if (text[i] == '<' || text[i] == '&' || text[i] == '>') j++;
  }

  /* conservative; allow 4 _extra_ characters for each escape sequence */
  ret = (char *)alloc_mem(i + 1 + j*4);

  j = 0;
  for (i=0; text[i] != '\0'; i++)
  {
    if (text[i] == '<')
    {   
      memcpy(&ret[j], "&lt;", 4);
      j += 4;
    }
    else if (text[i] == '>')
    {
      memcpy(&ret[j], "&gt;", 4);
      j += 4;
    }
    else if (text[i] == '&')
    {
      memcpy(&ret[j], "&amp;", 5);
      j += 5;
    }
    else ret[j++] = text[i];
  }
  ret[j] = '\0';
  return ret;
}
