/*
 * stringdb.h
 *
 * Copyright (C) 1993, 1994, 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 */

typedef struct _stringdb
{
  char *name;
  char *value;
  struct _stringdb *next;
} StringDB;

void AddListToStringDB _ArgProto((StringDB *));
void AddToStringDB _ArgProto((char *, char *));
char *GetFromStringDB _ArgProto((char *));
char *NGetFromStringDB _ArgProto((char *));
