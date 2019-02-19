/*
 * input.h
 *
 * Copyright (C) 1994, 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 */

int WaitForConnect _ArgProto((int));
int WriteBuffer _ArgProto((int, char *, int));
char *ReadBuffer _ArgProto((int, int *, int, int));
int ReadLine _ArgProto((int, char *, int));
char *ReadPipe _ArgProto((char *, char *, char *, int, char **, int *));
int PipeCommand _ArgProto((char *, int *, int));
