/*
 * local.h
 *
 * Copyright (C) 1993, 1994, 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 */

void ReadIndexFilenames _ArgProto((char *));
Document *file _ArgProto((URLParts *, MIMEType *));
Document *telnet _ArgProto((URLParts *, MIMEType *));
Document *tn3270 _ArgProto((URLParts *, MIMEType *));
