/*
 * dmem.c
 *
 * Copyright (C) 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 */
#include "copyright.h"
#include "options.h"

/*
 * This cheeseball bit of code is just a frontend to the Xt memory
 * allocation stuff so that non-X code doesn't have to deal with
 * X definitions.
 */

#include <X11/Intrinsic.h>

#include "common.h"

/*
 * free_mem
 */
void
free_mem(m)
char *m;
{
  XtFree(m);
  return;
}

/*
 * calloc_mem
 */
char *
calloc_mem(len, elen)
int len, elen;
{
  return(XtCalloc(len, elen));
}

/*
 * alloc_mem
 */
char *
alloc_mem(len)
int len;
{
  return(XtMalloc(len));
}

/*
 * realloc_mem
 */
char *
realloc_mem(s, len)
char *s;
int len;
{
  return(XtRealloc(s, len));
}

/*
 * alloc_string
 */
char *
alloc_string(str)
char *str;
{
  return(XtNewString(str));
}
