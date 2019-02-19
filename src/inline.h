/*
 * inline.h
 *
 * Copyright (C) 1993, 1994, 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 */

#define COLOR_DISPLAY 0
#define GRAY_DISPLAY 1
#define MONO_DISPLAY 2

ImageInfo *CreateImageInfo _ArgProto((Widget, Document *, Convert *, char *, int, int, XColor *, double));
