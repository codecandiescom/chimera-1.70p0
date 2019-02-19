/* 
 * net.h
 *
 * Copyright (C) 1993, 1994, 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 */

int net_open _ArgProto((char *, int));
void net_close _ArgProto((int));
char *net_gethostname _ArgProto(());
int net_bind _ArgProto((int));
int net_accept _ArgProto((int));
