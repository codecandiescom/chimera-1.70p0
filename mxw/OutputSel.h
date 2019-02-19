/*
 * OutputSel.h
 *
 * Copyright (C) 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 */

/* Resources:

 Name		     Class		RepType		Default Value
 ----		     -----		-------		-------------

*/

#define XtNdefaultPrinter "defaultPrinter"
#define XtNdefaultFilename "defaultFilename"
#define XtNdefaultEmail "defaultEmail"
#define XtNokCallback "okCallback"
#define XtNdismissCallback "dismissCallback"
#define XtNoutputType "outputType"
#define XtNoutputDevice "outputDevice"

typedef struct _outputselcallbackinfo
{
  char *str;
  int type;
  int device;
} OutputSelCallbackInfo;

extern WidgetClass     outputSelWidgetClass;

typedef struct _OutputSelClassRec   *OutputSelWidgetClass;
typedef struct _OutputSelRec        *OutputSelWidget;
