/*
 * OutputSel.c
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
#include <X11/Xaw/Toggle.h>
#include <X11/Xaw/Paned.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Form.h>

#include "ScrollText.h"
#include "OutputSelP.h"

#define OSREC osw->output_sel

static char fieldTranslations[] = "<Key>Return: OSPressReturn() \n ";

#define offset(field) XtOffsetOf(OutputSelRec, field)
static XtResource resources[] =
{
  { XtNdefaultPrinter, XtCString, XtRString, sizeof(String),
	offset(output_sel.defaultPrinter), XtRString, (XtPointer)"" },
  { XtNdefaultFilename, XtCString, XtRString, sizeof(String),
	offset(output_sel.defaultFilename), XtRString, (XtPointer)"" },
  { XtNdefaultEmail, XtCString, XtRString, sizeof(String),
	offset(output_sel.defaultEmail), XtRString, (XtPointer)"" },
  { XtNokCallback, XtCCallback, XtRCallback, sizeof(XtPointer),
	offset(output_sel.okCallbacks), XtRCallback, (XtPointer)NULL },
  { XtNdismissCallback, XtCCallback, XtRCallback, sizeof(XtPointer),
	offset(output_sel.dismissCallbacks), XtRCallback, (XtPointer)NULL },
  { XtNoutputType, XtCIndex, XtRInt, sizeof(int),
	offset(output_sel.outputType), XtRImmediate, (XtPointer)0 },
  { XtNoutputDevice, XtCIndex, XtRInt, sizeof(int),
	offset(output_sel.outputDevice), XtRImmediate, (XtPointer)0 },
};
#undef offset

static void OSInitialize(), OSDestroy(), OSRealize();
static Boolean OSSetValues();
static void OSPressReturn(), OKFunc(), ClearFunc();
static void TTCallback(), OTCallback();
static void DismissFunc();

static XtActionsRec actionsList[] =
{
  { "OSPressReturn", OSPressReturn }
};

OutputSelClassRec outputSelClassRec =
{
  {
    (WidgetClass) &transientShellClassRec,
    "OutputSel",
    sizeof(OutputSelRec),
    NULL,
    NULL,
    FALSE,
    OSInitialize,
    NULL,		
    OSRealize,
    actionsList,
    XtNumber(actionsList),
    resources,
    XtNumber(resources),
    NULLQUARK,
    FALSE,
    TRUE,
    FALSE,
    FALSE,
    OSDestroy,
    XtInheritResize,
    NULL,
    OSSetValues,
    NULL,			
    XtInheritSetValuesAlmost,  
    NULL,			
    NULL,
    XtVersion,
    NULL,
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
WidgetClass outputSelWidgetClass = (WidgetClass) &outputSelClassRec;

/****************************************************************
 *
 * Private Procedures
 *
 ****************************************************************/

/*
 * OSPressReturn
 */
static void
OSPressReturn(w, event, params, num_params)
Widget w;
XEvent *event;
String *params;
Cardinal *num_params;
{
  OutputSelWidget osw;
  Widget xw;
  OutputSelCallbackInfo osci;

  for (xw = XtParent(w);
       !XtIsSubclass(xw, outputSelWidgetClass);
       xw = XtParent(xw))
      ;
  osw = (OutputSelWidget)xw;

  XtVaGetValues(XtNameToWidget(OSREC.field, "text"),
		XtNstring, &osci.str,
		NULL);
  osci.type = OSREC.outputType;
  osci.device = OSREC.outputDevice;
  XtCallCallbackList(xw, OSREC.okCallbacks, (XtPointer)&osci);

  return;
}

/*
 * GetFieldStr
 */
static char *
GetFieldStr(osw)
OutputSelWidget osw;
{
  if (OSREC.outputDevice == 0) return(OSREC.defaultPrinter);
  else if (OSREC.outputDevice == 1) return(OSREC.defaultFilename);
  return(OSREC.defaultEmail);
}

/*
 * OSInitialize
 */
static void
OSInitialize(request, new, xargs, num_args)
Widget request, new;
ArgList xargs;                  /* unused */
Cardinal *num_args;             /* unused */
{
  OutputSelWidget osw = (OutputSelWidget)new;
  Arg args[10];
  int argcnt;
  Widget atext;
  char *str;
  int i;
  static char *tnames[] =
  {
    "plain", "pretty", "ps", "raw"
  };
  static char *onames[] =
  {
    "printer", "file", "email",
  };

  if (OSREC.outputDevice > 2 || OSREC.outputDevice < 0) OSREC.outputDevice = 0;
  if (OSREC.outputType > 3 || OSREC.outputType < 0) OSREC.outputType = 0;

  OSREC.paned = XtCreateManagedWidget("paned",
				      panedWidgetClass, new,
				      NULL, 0);

  OSREC.tbox = XtCreateManagedWidget("tbox",
				     boxWidgetClass, OSREC.paned,
				     NULL, 0);

  for (i = 0; i < XtNumber(tnames); i++)
  {
    if (i == OSREC.outputType) XtSetArg(args[0], XtNstate, True);
    else XtSetArg(args[0], XtNstate, False);

    OSREC.types[i] = XtCreateManagedWidget(tnames[i],
					   toggleWidgetClass, OSREC.tbox,
					   args, 1);
    XtAddCallback(OSREC.types[i], XtNcallback, TTCallback, (XtPointer)new);
  }

  OSREC.obox = XtCreateManagedWidget("obox",
				     boxWidgetClass, OSREC.paned,
				     NULL, 0);

  for (i = 0; i < XtNumber(onames); i++)
  {
    if (i == OSREC.outputType) XtSetArg(args[0], XtNstate, True);
    else XtSetArg(args[0], XtNstate, False);

    OSREC.outs[i] = XtCreateManagedWidget(onames[i],
					  toggleWidgetClass, OSREC.obox,
					  args, 1);
    XtAddCallback(OSREC.outs[i], XtNcallback, OTCallback, (XtPointer)new);
  }

  OSREC.form = XtCreateManagedWidget("form",
				     formWidgetClass, OSREC.paned,
				     NULL, 0);

  OSREC.field = XtCreateManagedWidget("field",
				      scrollingTextWidgetClass, OSREC.form,
				      NULL, 0);

  atext = XtNameToWidget(OSREC.field, "text");

  XtOverrideTranslations(atext,
			 XtParseTranslationTable(fieldTranslations));

  str = GetFieldStr(osw);

  argcnt = 0;
  XtSetArg(args[argcnt], XtNstring, str); argcnt++;
  XtSetArg(args[argcnt], XtNeditType, XawtextEdit); argcnt++;
  XtSetValues(atext, args, argcnt);

  OSREC.box = XtCreateManagedWidget("box",
				    boxWidgetClass, OSREC.paned,
				    NULL, 0);
  
  OSREC.ok = XtCreateManagedWidget("ok",
				   commandWidgetClass, OSREC.box,
				   NULL, 0);
  XtAddCallback(OSREC.ok, XtNcallback, OKFunc, (XtPointer)new);
  
  OSREC.clear = XtCreateManagedWidget("clear",
				      commandWidgetClass, OSREC.box,
				      NULL, 0);
  XtAddCallback(OSREC.clear, XtNcallback, ClearFunc, (XtPointer)new);

  OSREC.dismiss = XtCreateManagedWidget("dismiss",
					commandWidgetClass, OSREC.box,
					NULL, 0);
  XtAddCallback(OSREC.dismiss, XtNcallback, DismissFunc, (XtPointer)new);

  return;
}

/*
 * OSRealize
 */
static void
OSRealize(w, valueMask, attributes)
Widget w;
XtValueMask *valueMask;
XSetWindowAttributes *attributes;
{
  OutputSelWidget osw = (OutputSelWidget)w;

  (*outputSelWidgetClass->core_class.superclass->core_class.realize)
      (w, valueMask, attributes);

  XtSetKeyboardFocus(w, OSREC.field);

  return;
}

/*
 * OSDestroy
 */
static void
OSDestroy(w)
Widget w;
{
  return;
}

/*
 * OSSetValues
 */
static Boolean
OSSetValues(old, request, new, args, num_args)
Widget old;
Widget request;
Widget new;
ArgList args;
Cardinal *num_args;
{
  return(False);
}

/*
 * OTCallback
 */
static void
OTCallback(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  OutputSelWidget osw = (OutputSelWidget)cldata;
  int i;
  Boolean state;

  XtVaGetValues(w, XtNstate, &state, NULL);
  if (state == False)
  {
    XtVaSetValues(w, XtNstate, True, NULL);
    return;
  }

  for (i = 0; i < 3; i++)
  {
    if (OSREC.outs[i] != w)
    {
      XtVaSetValues(OSREC.outs[i], XtNstate, False, NULL);
    }
    else OSREC.outputDevice = i;
  }
  
  XtVaSetValues(XtNameToWidget(OSREC.field, "text"),
		XtNstring, GetFieldStr(osw),
		NULL);

  return;
}

/*
 * TTCallback
 */
static void
TTCallback(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  OutputSelWidget osw = (OutputSelWidget)cldata;
  int i;
  Boolean state;

  XtVaGetValues(w, XtNstate, &state, NULL);
  if (state == False)
  {
    XtVaSetValues(w, XtNstate, True, NULL);
    return;
  }

  for (i = 0; i < 4; i++)
  {
    if (OSREC.types[i] != w)
    {
      XtVaSetValues(OSREC.types[i], XtNstate, False, NULL);
    }
    else OSREC.outputType = i;
  }

  return;
}

/*
 * ClearFunc
 */
static void
ClearFunc(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  OutputSelWidget osw = (OutputSelWidget)cldata;

  XtVaSetValues(XtNameToWidget(OSREC.field, "text"), XtNstring, "", NULL);

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
  OutputSelWidget osw = (OutputSelWidget)cldata;
  OutputSelCallbackInfo osci;

  XtVaGetValues(XtNameToWidget(OSREC.field, "text"),
		XtNstring, &osci.str,
		NULL);
  osci.type = OSREC.outputType;
  osci.device = OSREC.outputDevice;
  XtCallCallbackList((Widget)osw, OSREC.okCallbacks, (XtPointer)&osci);

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
  OutputSelWidget osw = (OutputSelWidget)cldata;

  if (OSREC.dismissCallbacks != NULL)
  {
    XtCallCallbackList((Widget)osw, OSREC.dismissCallbacks, NULL);
  }
  else
  {
    XtPopdown((Widget)osw);
    XtDestroyWidget((Widget)osw);
  }

  return;
}
