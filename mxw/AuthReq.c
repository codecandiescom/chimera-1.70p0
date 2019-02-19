/*
 * AuthReq.c
 *
 * Copyright (C) 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 */

#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/ShellP.h>
#include <X11/Vendor.h>
#include <X11/VendorP.h>

#include <X11/Xaw/XawInit.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Paned.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Form.h>

#include "ScrollText.h"
#include "AuthReqP.h"

#define AREC aw->auth_req

static void AInitialize(), ADestroy(), ARealize();
static Boolean ASetValues();
static void AOKCallback(), AClearCallback(), ADismissCallback();
static void AUsernameReturn(), APasswordReturn();

static char usernameTranslations[] = "<Key>Return: AUsernameReturn() \n ";
static char passwordTranslations[] = "<Key>Return: APasswordReturn() \n ";

#define offset(field) XtOffsetOf(AuthReqRec, field)
static XtResource resources[] =
{
  { XtNauthreqMessage, XtCString, XtRString, sizeof(String),
	offset(auth_req.message), XtRString, (XtPointer)"" },
  { XtNdefaultUsername, XtCString, XtRString, sizeof(String),
	offset(auth_req.defaultUsername), XtRString, (XtPointer)"" },
  { XtNokCallback, XtCCallback, XtRCallback, sizeof(XtPointer),
	offset(auth_req.okCallbacks), XtRCallback, (XtPointer)NULL },
  { XtNdismissCallback, XtCCallback, XtRCallback, sizeof(XtPointer),
	offset(auth_req.dismissCallbacks), XtRCallback, (XtPointer)NULL },
};
#undef offset

static XtActionsRec actionsList[] =
{
  { "AUsernameReturn", AUsernameReturn },
  { "APasswordReturn", APasswordReturn },
};

AuthReqClassRec authReqClassRec =
{
  {
    (WidgetClass) &transientShellClassRec,
    "AuthReq",
    sizeof(AuthReqRec),
    NULL,
    NULL,
    FALSE,
    AInitialize,
    NULL,		
    ARealize,
    actionsList,
    XtNumber(actionsList),
    resources,
    XtNumber(resources),
    NULLQUARK,
    FALSE,
    TRUE,
    FALSE,
    FALSE,
    ADestroy,
    XtInheritResize,
    NULL,
    ASetValues,
    NULL,			
    XtInheritSetValuesAlmost,  
    NULL,			
    NULL,
    XtVersion,
    XtInheritAcceptFocus,
    NULL,
    NULL,
    NULL,
    NULL
  },
  {
    XtInheritGeometryManager,
    XtInheritChangeManaged,
    XtInheritInsertChild,
    XtInheritDeleteChild,
    NULL
  },
  {    
    NULL
  },
  {
    NULL
  },
  {
    NULL
  },
  {
    NULL
  },
  {
    NULL
  },
  
};

/* for public consumption */
WidgetClass authReqWidgetClass = (WidgetClass) &authReqClassRec;

/****************************************************************
 *
 * Private Procedures
 *
 ****************************************************************/

/*
 * Initialize
 */
static void
AInitialize(request, new, xargs, num_args)
Widget request, new;
ArgList xargs;                  /* unused */
Cardinal *num_args;             /* unused */
{
  AuthReqWidget aw = (AuthReqWidget)new;
  Arg args[10];
  int argcnt;
  Widget atext;

  AREC.paned = XtCreateManagedWidget("paned",
				     panedWidgetClass, new,
				     NULL, 0);

  AREC.uform = XtCreateManagedWidget("uform",
				     formWidgetClass, AREC.paned,
				     NULL, 0);

  AREC.ulabel = XtCreateManagedWidget("ulabel",
				      labelWidgetClass, AREC.uform,
				      NULL, 0);

  AREC.username = XtVaCreateManagedWidget("username",
					  scrollingTextWidgetClass, AREC.uform,
					  XtNfromHoriz, AREC.ulabel,
					  NULL);

  atext = XtNameToWidget(AREC.username, "text");
  XtOverrideTranslations(atext,
			 XtParseTranslationTable(usernameTranslations));

  argcnt = 0;
  XtSetArg(args[argcnt], XtNstring, AREC.defaultUsername); argcnt++;
  XtSetArg(args[argcnt], XtNeditType, XawtextEdit); argcnt++;
  XtSetValues(atext, args, argcnt);

  AREC.pform = XtCreateManagedWidget("pform",
				     formWidgetClass, AREC.paned,
				     NULL, 0);

  AREC.plabel = XtCreateManagedWidget("plabel",
				      labelWidgetClass, AREC.pform,
				      NULL, 0);

  AREC.password = XtVaCreateManagedWidget("password",
					  scrollingTextWidgetClass, AREC.pform,
					  XtNfromHoriz, AREC.plabel,
					  NULL);

  atext = XtNameToWidget(AREC.password, "text");
  XtOverrideTranslations(atext,
			 XtParseTranslationTable(passwordTranslations));

  argcnt = 0;
  XtSetArg(args[argcnt], XtNeditType, XawtextEdit); argcnt++;
  XtSetArg(args[argcnt], XtNecho, False); argcnt++;
  XtSetValues(atext, args, argcnt);

  AREC.box = XtCreateManagedWidget("box",
				   boxWidgetClass, AREC.paned,
				   NULL, 0);
  
  AREC.ok = XtCreateManagedWidget("ok",
				  commandWidgetClass, AREC.box,
				  NULL, 0);
  XtAddCallback(AREC.ok, XtNcallback, AOKCallback, (XtPointer)new);
  
  AREC.clear = XtCreateManagedWidget("clear",
				     commandWidgetClass, AREC.box,
				     NULL, 0);
  XtAddCallback(AREC.clear, XtNcallback, AClearCallback, (XtPointer)new);

  AREC.dismiss = XtCreateManagedWidget("dismiss",
				       commandWidgetClass, AREC.box,
				       NULL, 0);
  XtAddCallback(AREC.dismiss, XtNcallback, ADismissCallback, (XtPointer)new);

  return;
}

/*
 * ARealize 
 */
static void
ARealize(w, valueMask, attributes)
Widget w;
XtValueMask *valueMask;
XSetWindowAttributes *attributes;
{
  AuthReqWidget aw = (AuthReqWidget)w;

  (*authReqWidgetClass->core_class.superclass->core_class.realize)
      (w, valueMask, attributes);
  
  XtSetKeyboardFocus(w, XtNameToWidget(AREC.username, "text"));

  return;
}

/*
 * ADestroy
 */
static void
ADestroy(w)
Widget w;
{
  return;
}

/*
 * ASetValues
 */
static Boolean
ASetValues(old, request, new, args, num_args)
Widget old;
Widget request;
Widget new;
ArgList args;
Cardinal *num_args;
{
  return(False);
}

/*
 * AClearCallback
 */
static void
AClearCallback(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  AuthReqWidget aw = (AuthReqWidget)cldata;

  XtVaSetValues(XtNameToWidget(AREC.username, "text"), XtNstring, "", NULL);
  XtVaSetValues(XtNameToWidget(AREC.password, "text"), XtNstring, "", NULL);
  XtSetKeyboardFocus((Widget)aw, XtNameToWidget(AREC.username, "text"));

  return;
}

/*
 * ADoCallback
 */
static void
ADoCallback(aw)
AuthReqWidget aw;
{
  AuthReqReturnStruct ar;

  XtVaGetValues(XtNameToWidget(AREC.username, "text"),
		XtNstring, &ar.username,
		NULL);
  XtVaGetValues(XtNameToWidget(AREC.password, "text"),
		XtNstring, &ar.password,
		NULL);
  XtCallCallbackList((Widget)aw, AREC.okCallbacks, (XtPointer)&ar);

  XtPopdown((Widget)aw);
  XtDestroyWidget((Widget)aw);

  return;
}

/*
 * AOKCallback
 */
static void
AOKCallback(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  ADoCallback((AuthReqWidget)cldata);

  return;
}

/*
 * AUsernameReturn
 */
static void
AUsernameReturn(w, event, params, num_params)
Widget w;
XEvent *event;
String *params;
Cardinal *num_params;
{
  AuthReqWidget aw;
  Widget xw;

  for (xw = XtParent(w);
       !XtIsSubclass(xw, authReqWidgetClass);
       xw = XtParent(xw))
      ;
  aw = (AuthReqWidget)xw;
  XtSetKeyboardFocus(xw, XtNameToWidget(AREC.password, "text"));

  return;
}

/*
 * APasswordReturn
 */
static void
APasswordReturn(w, event, params, num_params)
Widget w;
XEvent *event;
String *params;
Cardinal *num_params;
{
  Widget xw;

  for (xw = XtParent(w);
       !XtIsSubclass(xw, authReqWidgetClass);
       xw = XtParent(xw))
      ;
  ADoCallback((AuthReqWidget)xw);

  return;
}

/*
 * ADismissCallback
 */
static void
ADismissCallback(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  AuthReqWidget aw = (AuthReqWidget)cldata;

  if (AREC.dismissCallbacks == NULL)
  {
    XtPopdown((Widget)cldata);
    XtDestroyWidget((Widget)cldata);
  }
  else XtCallCallbackList((Widget)aw, AREC.dismissCallbacks, NULL);

  return;
}


