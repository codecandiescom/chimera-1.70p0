/*
 * file.h
 *
 * Copyright (C) 1993, 1994, 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 */

typedef struct _convert
{
  char *use;
  char *incontent;
  char *outcontent;
  char *command;
  int (*func)();
  struct _convert *next;
} Convert;

Document *ConvertDocument _ArgProto((Document *, Convert *, char *, char *));
Convert *ReadConvertFiles _ArgProto((char *));
