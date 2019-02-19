/*
 * StrReq.h
 *
 * Copyright (C) 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 */

/* Resources:

 Name		     Class		RepType		Default Value
 ----		     -----		-------		-------------

*/

#define XtNdefaultString "defaultString"
#define XtNstrreqMessage "strreqMessage"
#define XtNhiddenText "hiddenText"
#define XtNokCallback "okCallback"
#define XtNdismissCallback "dismissCallback"

extern WidgetClass     strReqWidgetClass;

typedef struct _StrReqClassRec   *StrReqWidgetClass;
typedef struct _StrReqRec        *StrReqWidget;
