/*
 * auth.c
 *
 * Copyright (C) 1993, 1994, 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 */
#include "copyright.h"
#include "options.h"

#include "common.h"
#include "auth.h"

/*
 * GetAuthInfo
 *
 * Reads authentication info from a file.
 */
void
GetAuthInfo(filelist)
char *filelist;
{
  return;
}

/*
 * IsAuthentic
 *
 * Returns a non-zero value if the document the name and auth
 * info is correct.
 */
int
IsAuthentic(name, info)
char *name;
char *info;
{
  return(0);
}

/*
 * IsKAuthentic
 *
 * Returns a non-zero value if the ticket is valid
 */
int
IsKAuthentic(kdata)
char *kdata;
{
  return(0);
}
