/*
 * util.h
 *
 * Copyright (C) 1993, 1994, 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 */

/*
 * Just in case.
 */
#ifndef L_tmpnam
#define L_tmpnam 1024
#endif

char *FixFilename _ArgProto((char *));
int SaveData _ArgProto((char *, int, char *));
void StartReaper();
char *mystrtok _ArgProto((char *, int, char **));
char *CreateCommandLine _ArgProto((char *, char *, char *));
char *EscapeHTML _ArgProto((char *));
