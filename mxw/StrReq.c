/*
 * StrReq.c
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
#include "StrReqP.h"

static char defaultTranslations[] = "<Key>Return: SRPressReturn() \n ";

#define SRREC srw->str_req

#define offset(field) XtOffsetOf(StrReqRec, field)
static XtResource resources[] =
{
  { XtNstrreqMessage, XtCString, XtRString, sizeof(String),
	offset(str_req.message), XtRString, (XtPointer)"" },
  { XtNdefaultString, XtCString, XtRString, sizeof(String),
	offset(str_req.defaultString), XtRString, (XtPointer)"" },
  { XtNhiddenText, XtCBoolean, XtRBoolean, sizeof(Boolean),
	offset(str_req.hiddenText), XtRImmediate, (XtPointer)False },
  { XtNokCallback, XtCCallback, XtRCallback, sizeof(XtPointer),
	offset(str_req.okCallbacks), XtRCallback, (XtPointer)NULL },
  { XtNdismissCallback, XtCCallback, XtRCallback, sizeof(XtPointer),
	offset(str_req.dismissCallbacks), XtRCallback, (XtPointer)NULL },
};
#undef offset

static void SRInitialize(), SRRealize(), SRDestroy();
static Boolean SRSetValues();
static void SRPressReturn();
static void OKFunc(), ClearFunc(), DismissFunc();

static XtActionsRec actionsList[] =
{
  { "SRPressReturn", SRPressReturn }
};

StrReqClassRec strReqClassRec =
{
  {
    (WidgetClass) &transientShellClassRec,
    "StrReq",
    sizeof(StrReqRec),
    NULL,
    NULL,
    FALSE,
    SRInitialize,
    NULL,		
    SRRealize,
    actionsList,
    XtNumber(actionsList),
    resources,
    XtNumber(resources),
    NULLQUARK,
    FALSE,
    TRUE,
    FALSE,
    FALSE,
    SRDestroy,
    XtInheritResize,
    NULL,
    SRSetValues,
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
WidgetClass strReqWidgetClass = (WidgetClass) &strReqClassRec;

/****************************************************************
 *
 * Private Procedures
 *
 ****************************************************************/

/*
 * SRPressReturn
 */
static void
SRPressReturn(w, event, params, num_params)
Widget w;
XEvent *event;
String *params;
Cardinal *num_params;
{
  StrReqWidget srw;
  String str;
  Widget xw;

  for (xw = XtParent(w);
       !XtIsSubclass(xw, strReqWidgetClass);
       xw = XtParent(xw))
      ;
  srw = (StrReqWidget)xw;
  XtVaGetValues(w, XtNstring, &str, NULL);
  XtCallCallbackList(xw, SRREC.okCallbacks, (XtPointer)str);

  return;
}

/*
 * Initialize
 */
static void
SRInitialize(request, new, xargs, num_args)
Widget request, new;
ArgList xargs;                  /* unused */
Cardinal *num_args;             /* unused */
{
  StrReqWidget srw = (StrReqWidget)new;
  Arg args[10];
  int argcnt;
  Widget atext;
  Pixel bg;

  SRREC.paned = XtCreateManagedWidget("paned",
			       panedWidgetClass, new,
			       NULL, 0);

  SRREC.form = XtCreateManagedWidget("form",
			      formWidgetClass, SRREC.paned,
			      NULL, 0);

  argcnt = 0;
  if (SRREC.message != NULL && SRREC.message[0] != '\0')
  {
    XtSetArg(args[0], XtNlabel, SRREC.message);
    SRREC.label = XtCreateManagedWidget("label",
					labelWidgetClass, SRREC.form,
					args, 1);
    XtSetArg(args[argcnt], XtNfromVert, SRREC.label); argcnt++;
  }

  SRREC.field = XtCreateManagedWidget("field",
			       scrollingTextWidgetClass, SRREC.form,
			       args, argcnt);

  atext = XtNameToWidget(SRREC.field, "text");

  XtOverrideTranslations(atext,
			 XtParseTranslationTable(defaultTranslations));

  argcnt = 0;
  XtSetArg(args[argcnt], XtNstring, SRREC.defaultString); argcnt++;
  XtSetArg(args[argcnt], XtNeditType, XawtextEdit); argcnt++;
  if (SRREC.hiddenText)
  {
    XtVaGetValues(SRREC.field, XtNbackground, &bg, NULL);
    XtSetArg(args[argcnt], XtNforeground, bg); argcnt++;
  }
  XtSetValues(atext, args, argcnt);

  SRREC.box = XtCreateManagedWidget("box",
			     boxWidgetClass, SRREC.paned,
			     NULL, 0);
  
  SRREC.ok = XtCreateManagedWidget("ok",
			    commandWidgetClass, SRREC.box,
			    NULL, 0);
  XtAddCallback(SRREC.ok, XtNcallback, OKFunc, (XtPointer)new);
  
  SRREC.clear = XtCreateManagedWidget("clear",
			       commandWidgetClass, SRREC.box,
			       NULL, 0);
  XtAddCallback(SRREC.clear, XtNcallback, ClearFunc, (XtPointer)new);

  SRREC.dismiss = XtCreateManagedWidget("dismiss",
				 commandWidgetClass, SRREC.box,
				 NULL, 0);
  XtAddCallback(SRREC.dismiss, XtNcallback, DismissFunc, (XtPointer)new);

  return;
}

/*
 * SRRealize
 */
static void
SRRealize(w, valueMask, attributes)
Widget w;
XtValueMask *valueMask;
XSetWindowAttributes *attributes;
{
  StrReqWidget srw = (StrReqWidget)w;

  (*strReqWidgetClass->core_class.superclass->core_class.realize)
      (w, valueMask, attributes);

  XtSetKeyboardFocus(w, XtNameToWidget(SRREC.field, "text"));

  return;
}


/*
 * SRDestroy
 */
static void
SRDestroy(w)
Widget w;
{
  return;
}

/*
 * SRSetValues
 */
static Boolean
SRSetValues(old, request, new, args, num_args)
Widget old;
Widget request;
Widget new;
ArgList args;
Cardinal *num_args;
{
  return(False);
}

/*
 * ClearFunc
 */
static void
ClearFunc(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  StrReqWidget srw = (StrReqWidget)cldata;

  XtVaSetValues(XtNameToWidget(SRREC.field, "text"), XtNstring, "", NULL);

  return;
}

/*
 * OKFunc
 */
static void
OKFunc(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  StrReqWidget srw = (StrReqWidget)cldata;
  String str;

  XtVaGetValues(XtNameToWidget(SRREC.field, "text"), XtNstring, &str, NULL);
  XtCallCallbackList((Widget)srw, SRREC.okCallbacks, (XtPointer)str);

  return;
}

/*
 * DismissFunc
 */
static void
DismissFunc(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  StrReqWidget srw = (StrReqWidget)cldata;

  if (SRREC.dismissCallbacks == NULL)
  {
    XtPopdown((Widget)cldata);
    XtDestroyWidget((Widget)cldata);
    XFlush(XtDisplay((Widget)cldata));
  }
  else XtCallCallbackList((Widget)srw, SRREC.dismissCallbacks, NULL);

  return;
}


