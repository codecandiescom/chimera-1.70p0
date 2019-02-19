/*
 * url.h
 *
 * Copyright (C) 1993, 1994, 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 */

/*
 * Some of these fields should not be here.  It was convienent.
 *
 * protocol://username:password@hostname:port/filename#anchor
 */
typedef struct
{
  char *protocol;
  char *hostname;
  int port;
  char *filename;
  char *anchor;
  char *method;
  char **attribute_names;
  char **attribute_values;
  int attribute_count;
  char *data_type;
  char *auth_type;
  char *username;
  char *password;
} URLParts;

#define NullString(s)	(s == NULL || *s == '\0')

/*
 * Prototypes
 */

char *FixURL _ArgProto((char *));
char *MakeURL _ArgProto((URLParts *, int));
URLParts *MakeURLParts _ArgProto((URLParts *, URLParts *));
URLParts *ParseURL _ArgProto((char *));
void DestroyURLParts _ArgProto((URLParts *));
void StripSearchURL _ArgProto((char *));
void AddVisitedURL _ArgProto((char *));
int IsVisitedURL _ArgProto((char *));
char *UnescapeURL _ArgProto((char *));
char *EscapeURL _ArgProto((unsigned char *, int));
URLParts *DupURLParts _ArgProto((URLParts *));
int IsAbsoluteURL _ArgProto((URLParts *));
