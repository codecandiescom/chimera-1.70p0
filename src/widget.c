/*
 * widget.c
 *
 * Copyright (C) 1993, 1994, 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * Note that much of this code was actually written by Jim.Rees@umich.edu.
 * The messed up parts were written by john.
 *
 * See copyright.h for details.
 */
#include "copyright.h"
#include "options.h"

#include <stdio.h>
#include <sys/types.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <X11/Xaw/Form.h>
#include <X11/Xaw/Paned.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xaw/List.h>
#include <X11/Xaw/Scrollbar.h>
#include <X11/Xaw/Toggle.h>

#include <X11/Xaw/Cardinals.h>

#include "ScrollText.h"

#ifndef __STDC__
#define _NO_PROTO 1
#endif

#include "HTML.h"

#include "common.h"
#include "url.h"
#include "mime.h"
#include "document.h"
#include "convert.h"
#include "widget.h"

static struct ButtonTable {
  char *name;
  Widget w;
  int wc;			/* 0: command widget;  1: toggle widget */
  void (*cb)();
} ButtonTable[] = {
  {"quit",      NULL, 0, Quit},
  {"open",	NULL, 0, OpenAction},
  {"home",	NULL, 0, Home},
  {"back",	NULL, 0, Back},
  {"source",	NULL, 0, Source},
  {"reload",	NULL, 0, Reload},
  {"file",      NULL, 0, FileAction},
  {"help",	NULL, 0, Help},
  {"cancel",    NULL, 0, Cancel},
  {"bookmark",	NULL, 0, BookmarkAction},
  {"search",    NULL, 0, SearchAction},
  {"deferpix",  NULL, 1, DeferPix},
  {NULL,        NULL, 0, NULL},
};

static void
AddButtons(r, box, list)
AppResources *r;
Widget box;
char *list;
{
  char name[256];
  struct ButtonTable *btp;

  list = alloc_string(list);

  while (sscanf(list, " %[^,]", name) == 1)
  {
    /*
     * Find the listed button and create its widget
     */
    for (btp = &ButtonTable[0]; btp->name != NULL; btp++)
    {
      if (!strcasecmp(btp->name, name))
      {
	WidgetClass wc =
	    (btp->wc == 0) ? commandWidgetClass : toggleWidgetClass;
	btp->w = XtCreateManagedWidget(btp->name,
				       wc,
				       box,
				       NULL, ZERO);
	XtAddCallback(btp->w, XtNcallback, btp->cb, r);
	break;
      }
    }

    /*
     * Fill in the entry in AppResources for the widget.
     */
    if (!strcasecmp(name, "open"))
      r->load = btp->w;
    else if (!strcasecmp(name, "home"))
    {
      r->home = btp->w;
      XtSetSensitive(r->home, False);
    }
    else if (!strcasecmp(name, "back"))
    {
      r->back = btp->w;
      XtSetSensitive(r->back, False);
    }
    else if (!strcasecmp(name, "source"))
      r->source = btp->w;
    else if (!strcasecmp(name, "reload"))
      r->reload = btp->w;
    else if (!strcasecmp(name, "file"))
      r->file = btp->w;
    else if (!strcasecmp(name, "help"))
      r->help = btp->w;
    else if (!strcasecmp(name, "bookmark"))
      r->bookmark = btp->w;
    else if (!strcasecmp(name, "cancel"))
      r->cancel = btp->w;
    else if (!strcasecmp(name, "search"))
      r->search = btp->w;
    else if (!strcasecmp(name, "quit"))
      r->quit = btp->w;
    else if (!strcasecmp(name, "deferpix"))
      r->deferpix = btp->w;

    /*
     * Skip to the next comma-delimited item in the list
     */
    while (*list && *list != ',')
      list++;
    if (*list == ',')
      list++;
  }

  return;
}

/*
 * CreateWidgets
 *
 * Create the command and box widgets.
 */
void
CreateWidgets(r)
AppResources *r;
{
  Widget paned, box, form;
  Arg args[5];

  /*
   * Main window pane
   */
  paned = XtCreateManagedWidget("paned",
                                panedWidgetClass, r->toplevel, 
                                NULL, ZERO);


  /*
   * Button pane(s)
   */
  if (r->button1Box && *r->button1Box)
  {
    box = XtCreateManagedWidget("box1", boxWidgetClass, paned, NULL, ZERO);
    AddButtons(r, box, r->button1Box);
  }

  if (r->button2Box && *r->button2Box)
  {
    box = XtCreateManagedWidget("box3", boxWidgetClass, paned, NULL, ZERO);
    AddButtons(r, box, r->button2Box);
  }


  /*
   * Third pane. Title display.
   */
  if (r->showTitle)
  {
    form = XtCreateManagedWidget("box5",
				formWidgetClass, paned,
				NULL, ZERO);
    
    XtCreateManagedWidget("titlelabel",
			  labelWidgetClass, form,
			  NULL, ZERO);
    
    r->titledisplay = XtCreateManagedWidget("titledisplay",
					    scrollingTextWidgetClass, form,
					    NULL, ZERO);
    r->titledisplay = XtNameToWidget(r->titledisplay, "text");
  }
  else
  {
    r->titledisplay = 0;
  }

  /*
   * Fourth pane.  URL display.
   */
  if (r->showURL)
  {
    form = XtCreateManagedWidget("box4",
				formWidgetClass, paned,
				NULL, ZERO);
    
    XtCreateManagedWidget("urllabel",
			  labelWidgetClass, form,
			  NULL, ZERO);
    
    r->urldisplay = XtCreateManagedWidget("urldisplay",
					  scrollingTextWidgetClass, form,
					  NULL, ZERO);
    r->urldisplay = XtNameToWidget(r->urldisplay, "text");
  }
  else
  {
    r->urldisplay = 0;
  }

  /*
   * rwmcm: Sixth pane, the URL anchor display
   */
  if (r->anchorDisplay)
  {
    XtSetArg(args[0], XtNjustify, XtJustifyLeft);
    XtSetArg(args[1], XtNlabel, " ");
    XtSetArg(args[2], XtNskipAdjust, True);
    r->anchordisplay=XtCreateManagedWidget("anchordisplay",
                         labelWidgetClass, paned,
                         args, 3);
  }

  /*
   * Fifth pane, the HTML viewing area
   */
  r->w = XtCreateManagedWidget("html",
                              htmlWidgetClass, paned,
                              NULL, ZERO);
  XtAddCallback(r->w, WbNanchorCallback, Anchor, r);
  if (r->anchorDisplay)
  {
    XtSetArg(args[0], WbNpointerMotionCallback, AnchorURLDisplay);
    XtSetValues(r->w, args, 1);
  }

  /*
   * Set some callbacks for the HTML widget.
   */
  XtSetArg(args[0], WbNresolveImageFunction, ImageResolve);
  XtSetArg(args[1], WbNpreviouslyVisitedTestFunction, VisitTest);
  XtSetValues(r->w, args, 2);

  XtAddCallback(r->w, WbNlinkCallback, LinkCB, (XtPointer)0);
  XtAddCallback(r->w, WbNsubmitFormCallback, SubmitForm, (XtPointer)0);

  return;
}


