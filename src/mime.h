/*
 * mime.h
 *
 * Copyright (C) 1994, 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 */

typedef struct _mailcap
{
  char *ctype;
  char *command;
  struct _mailcap *next;
} MailCap;

typedef struct _mimetype
{
  char *content;
  char *ext;
  struct _mimetype *next;
} MIMEType;

typedef struct _mimefield
{
  char *name;
  char *value;
  int used;
  struct _mimefield *next;
} MIMEField;

char *Ext2Content _ArgProto((MIMEType *, char *));
MIMEType *ReadMIMETypeFiles _ArgProto((char *));
MailCap *ReadMailCapFiles _ArgProto((char *));
int DisplayExternal _ArgProto((char *, char *, char *, MailCap *));
MIMEField *CreateMIMEField _ArgProto((char *));
void DestroyMIMEField _ArgProto((MIMEField *));
char *GetMIMEValue _ArgProto((MIMEField *, char *, int));
time_t ParseExpiresDate _ArgProto((char *));
