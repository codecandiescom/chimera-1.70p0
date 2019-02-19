/*
 * document.h
 *
 * Copyright (C) 1993, 1994, 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 */

#define DS_OK 0
#define DS_NEEDS_AUTH 1
#define DS_ERROR 2
#define DS_REDIRECT 3
#define DS_NOTHING 4

typedef struct _document
{
  URLParts *up;         /* the URL in parts */
  char *text;           /* the stuff in the document */
  int len;              /* the len of the data */
  char *content;        /* content-type might be NULL */
  char *ptext;          /* processed data (after pipe) */
  int plen;             /* length of processed data */
  char *pcontent;       /* processed content-type */
  int from_cache;       /* 1 if fetched from the cache */
  int cache;            /* 1 if the document should be cached */
  int status;           /* document status */
  char *auth_realm;     /* authentication realm */
  char *auth_type;      /* authentication type */
  MIMEField *mflist;    /* list of MIME fields for document */
} Document;

typedef struct _protocol
{
  char *proto;
  char *command;
  struct _protocol *next;
} Protocol;

Protocol *ReadProtocolFiles _ArgProto((char *));
Document *LoadDocument _ArgProto((URLParts *, Protocol *, MIMEType *, int));
void DestroyDocument _ArgProto((Document *));
Document *CreateDocument _ArgProto((void));
Document *BuildDocument _ArgProto((char *, int, char *, int, int));
void ParseMIMEField _ArgProto((Document *, MIMEField *));
char *BuildDocumentInfo _ArgProto((Document *));

