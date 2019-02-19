/*
 * AuthReq.h
 *
 * Copyright (C) 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 */

/* Resources:

 Name		     Class		RepType		Default Value
 ----		     -----		-------		-------------

*/

#define XtNdefaultUsername "defaultUsername"
#define XtNauthreqMessage "authreqMessage"
#define XtNokCallback "okCallback"
#define XtNdismissCallback "dismissCallback"

typedef struct _authreqreturnstruct
{
  String username;
  String password;
} AuthReqReturnStruct;

extern WidgetClass     authReqWidgetClass;

typedef struct _AuthReqClassRec   *AuthReqWidgetClass;
typedef struct _AuthReqRec        *AuthReqWidget;
