/*
 * Bookmark.c
 *
 * Copyright (C) 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 */

#include <stdio.h>
#include <ctype.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/ShellP.h>
#include <X11/Vendor.h>
#include <X11/VendorP.h>

#include <X11/Xaw/XawInit.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Paned.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/List.h>

#include "ScrollText.h"
#include "StrReq.h"
#include "BookmarkP.h"

#define BREC bw->bookmark

#define offset(field) XtOffsetOf(BookmarkRec, field)
static XtResource resources[] =
{
  { XtNfilename, XtCString, XtRString, sizeof(String),
	offset(bookmark.filename), XtRString, (XtPointer)"" },
  { XtNcallback, XtCCallback, XtRCallback, sizeof(XtPointer),
	offset(bookmark.callbacks), XtRPointer, (XtPointer)NULL },
  { XtNdelayWrite, XtCBoolean, XtRBoolean, sizeof(Boolean),
	offset(bookmark.delayWrite), XtRImmediate, (XtPointer)False },
  { XtNbookmarkMessage, XtCString, XtRString, sizeof(String),
	offset(bookmark.bookmarkMessage), XtRString, (XtPointer)"" },
  { XtNbookmarkProc, XtCBookmarkProc, XtRFunction, sizeof(XtPointer),
	offset(bookmark.bookmarkProc), XtRFunction, (XtPointer)NULL },
  { XtNbookmarkProcData, XtCBookmarkProcData, XtRPointer, sizeof(XtPointer),
	offset(bookmark.bookmarkProcData), XtRPointer, (XtPointer)NULL },
  { XtNpickDestroys, XtCBoolean, XtRBoolean, sizeof(Boolean),
	offset(bookmark.pickDestroys), XtRImmediate, (XtPointer)False },
  { XtNcurrentGroup, XtCString, XtRString, sizeof(String),
	offset(bookmark.currentGroup), XtRString, (XtPointer)NULL },
  { XtNnullURL, XtCString, XtRString, sizeof(String),
	offset(bookmark.nullURL), XtRString,
	(XtPointer)"NULL or Empty URL...could not add bookmark." },
  { XtNnoBookmarkProc, XtCString, XtRString, sizeof(String),
	offset(bookmark.noBookmarkProc), XtRString,
	(XtPointer)"Application did not supply a bookmark function." },
  { XtNnoCurrentGroup, XtCString, XtRString, sizeof(String),
	offset(bookmark.noCurrentGroup), XtRString,
	(XtPointer)"There is no current bookmark group." },
  { XtNnoCurrentMark, XtCString, XtRString, sizeof(String),
	offset(bookmark.noCurrentMark), XtRString,
	(XtPointer)"There is no current bookmark." },
  { XtNcantReadBookmarkFile, XtCString, XtRString, sizeof(String),
	offset(bookmark.cantReadBookmarkFile), XtRString,
	(XtPointer)"Cannot read the bookmark file." },
  { XtNcantWriteBookmarkFile, XtCString, XtRString, sizeof(String),
	offset(bookmark.cantWriteBookmarkFile), XtRString,
	(XtPointer)"Cannot write the bookmark file." },
  { XtNbadBookmarkFilename, XtCString, XtRString, sizeof(String),
	offset(bookmark.badBookmarkFilename), XtRString,
	(XtPointer)"Bad bookmark filename." },
};
#undef offset

static void BInitialize(), BDestroy(), BRealize();
static Boolean BSetValues();
static void GroupAddFunc();
static void MarkAddFunc();
static void BGroupDelCallback();
static void BMarkDelCallback();
static void BDismissFunc();
static void BMarkSelect();
static void BGroupListCallback();

static void ReadBookmarkFile();
static void WriteBookmarkFile();
static void MakeBookmarkGroupList();
static void MakeBookmarkList();
static char *GetBookmarkURL();
static Bookmark *CreateBookmark();
static BookmarkGroup *CreateBookmarkGroup();
static void DestroyBookmark();

static char defaultTranslations[] = "";

static char marklistTranslations[] =
"<Btn1Up>(2): Set()  MarkSelect() Unset() \n";

static XtActionsRec actionsList[] =
{
  { "MarkSelect", BMarkSelect },
};

BookmarkClassRec bookmarkClassRec =
{
  {
    (WidgetClass) &transientShellClassRec,
    "Bookmark",
    sizeof(BookmarkRec),
    NULL,
    NULL,
    FALSE,
    BInitialize,
    NULL,		
    BRealize,
    actionsList,
    XtNumber(actionsList),
    resources,
    XtNumber(resources),
    NULLQUARK,
    FALSE,
    TRUE,
    FALSE,
    FALSE,
    BDestroy,
    XtInheritResize,
    NULL,
    BSetValues,
    NULL,			
    XtInheritSetValuesAlmost,  
    NULL,			
    NULL,
    XtVersion,
    NULL,
    defaultTranslations,
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
WidgetClass bookmarkWidgetClass = (WidgetClass) &bookmarkClassRec;

/****************************************************************
 *
 * Private Procedures
 *
 ****************************************************************/

/*
 * BInitialize
 */
static void
BInitialize(request, new, xargs, num_args)
Widget request, new;
ArgList xargs;                  /* unused */
Cardinal *num_args;             /* unused */
{
  BookmarkWidget bw = (BookmarkWidget)new;
  Arg args[1];
  int i;

  BREC.nexturl = NULL;
  BREC.bgslist = NULL;
  BREC.bslist = NULL;

  BREC.appcon = XtWidgetToApplicationContext(new);

  BREC.paned = XtCreateManagedWidget("paned",
				     panedWidgetClass, new,
				     NULL, 0);
  if (BREC.bookmarkMessage != NULL && BREC.bookmarkMessage[0] != '\0')
  {
    XtSetArg(args[0], XtNlabel, BREC.bookmarkMessage);
    BREC.label = XtCreateManagedWidget("label",
				       labelWidgetClass, BREC.paned,
				       args, 1);
  }
  BREC.groupform = XtCreateManagedWidget("groupform",
					 formWidgetClass, BREC.paned,
					 NULL, 0);
  BREC.groupview = XtCreateManagedWidget("groupview",
					 viewportWidgetClass, BREC.groupform,
					 NULL, 0);
  BREC.grouplist = XtCreateManagedWidget("grouplist",
					 listWidgetClass, BREC.groupview,
					 NULL, 0);
  XtAddCallback(BREC.grouplist, XtNcallback,
		BGroupListCallback, (XtPointer)new);

  BREC.groupbox = XtCreateManagedWidget("groupbox",
					boxWidgetClass, BREC.paned,
					NULL, 0);

  BREC.groupadd = XtCreateManagedWidget("groupadd",
					commandWidgetClass, BREC.groupbox,
					NULL, 0);
  XtAddCallback(BREC.groupadd, XtNcallback, GroupAddFunc, (XtPointer)new);

  BREC.groupdel = XtCreateManagedWidget("groupdel",
					commandWidgetClass, BREC.groupbox,
					NULL, 0);
  XtAddCallback(BREC.groupdel, XtNcallback, BGroupDelCallback, (XtPointer)new);

  BREC.markform = XtCreateManagedWidget("markform",
					formWidgetClass, BREC.paned,
					NULL, 0);
  BREC.markview = XtCreateManagedWidget("markview",
					viewportWidgetClass, BREC.markform,
					NULL, 0);
  BREC.marklist = XtCreateManagedWidget("marklist",
					listWidgetClass, BREC.markview,
					NULL, 0);

  XtOverrideTranslations(BREC.marklist,
			 XtParseTranslationTable(marklistTranslations));

  BREC.markbox = XtCreateManagedWidget("markbox",
				    boxWidgetClass, BREC.paned,
				    NULL, 0);
  
  BREC.markadd = XtCreateManagedWidget("markadd",
				       commandWidgetClass, BREC.markbox,
				       NULL, 0);
  XtAddCallback(BREC.markadd, XtNcallback, MarkAddFunc, (XtPointer)new);

  BREC.markdel = XtCreateManagedWidget("markdel",
				       commandWidgetClass, BREC.markbox,
				       NULL, 0);
  XtAddCallback(BREC.markdel, XtNcallback, BMarkDelCallback, (XtPointer)new);

 
  BREC.dismiss = XtCreateManagedWidget("dismiss",
				       commandWidgetClass, BREC.markbox,
				       NULL, 0);
  XtAddCallback(BREC.dismiss, XtNcallback, BDismissFunc, (XtPointer)new);

  ReadBookmarkFile(bw);
  if (BREC.currentGroup != NULL)
  {
    for (BREC.cbg = BREC.blist, i = 0;
	 BREC.cbg != NULL; BREC.cbg = BREC.cbg->next, i++)
    {
      if (BREC.cbg->name != NULL &&
	  strcasecmp(BREC.cbg->name, BREC.currentGroup) == 0) break;
    }
    if (BREC.cbg == NULL) BREC.cbg = BREC.blist;
  }
  else
  {
    BREC.cbg = BREC.blist;
    i = 0;
  }

  MakeBookmarkGroupList(bw);
  MakeBookmarkList(bw);
  XawListHighlight(BREC.grouplist, i);

  return;
}

/*
 * BRealize
 */
static void
BRealize(w, valueMask, attributes)
Widget w;
XtValueMask *valueMask;
XSetWindowAttributes *attributes;
{
  (*bookmarkWidgetClass->core_class.superclass->core_class.realize)
      (w, valueMask, attributes);

  return;
}

/*
 * BDestroy
 */
static void
BDestroy(w)
Widget w;
{
  BookmarkWidget bw = (BookmarkWidget)w;
  Bookmark *bm, *tm;
  BookmarkGroup *bmg, *tmg;

  if (BREC.delayWrite) WriteBookmarkFile((BookmarkWidget)w);

  if (BREC.filename != NULL)     XtFree(BREC.filename);
  if (BREC.currentGroup != NULL) XtFree(BREC.currentGroup);
  if (BREC.nexturl != NULL)      XtFree(BREC.nexturl);
  if (BREC.bgslist != NULL)      XtFree((char *)BREC.bgslist);
  if (BREC.bslist != NULL)       XtFree((char *)BREC.bslist);
  
  for (bmg = BREC.blist; bmg != NULL; )
  {
    if (bmg->name != NULL) XtFree(bmg->name);

    for (bm = bmg->b; bm != NULL; )
    {
      if (bm->url != NULL) XtFree(bm->url);
      if (bm->display != NULL) XtFree(bm->display);
      tm = bm;
      bm = bm->next;
      XtFree((char *)tm);
    }

    tmg = bmg;
    bmg = bmg->next;
    XtFree((char *)tmg);
  }

  return;
}

/*
 * BSetValues
 */
static Boolean
BSetValues(old, request, new, args, num_args)
Widget old;
Widget request;
Widget new;
ArgList args;
Cardinal *num_args;
{
  return(False);
}

/*
 * BDismissFunc
 */
static void
BDismissFunc(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  XtPopdown((Widget)cldata);
  XtDestroyWidget((Widget)cldata);

  return;
}

/*
 * CreateStrReq
 */
static void
CreateStrReq(w, name, defstr, hidden, okcallback, cldata)
Widget w;
char *name;
char *defstr;
Boolean hidden;
XtCallbackProc okcallback;
XtPointer cldata;
{
  Widget nw;
  Arg args[10];
  int argcnt;
  Window rw, cw;
  int rx, ry, wx, wy;
  unsigned int mask;

  XQueryPointer(XtDisplay(w), XtWindow(w),
		&rw, &cw,
		&rx, &ry,
		&wx, &wy,
		&mask);

  argcnt = 0;
  XtSetArg(args[argcnt], XtNdefaultString, defstr); argcnt++;
  XtSetArg(args[argcnt], XtNhiddenText, hidden); argcnt++;
  XtSetArg(args[argcnt], XtNx, rx - 2); argcnt++;
  XtSetArg(args[argcnt], XtNy, ry - 2); argcnt++;
  nw = XtCreatePopupShell(name,
			  strReqWidgetClass, w,
			  args, argcnt);
  XtAddCallback(nw, XtNokCallback, okcallback, cldata);

  XtRealizeWidget(nw);
  XtPopup(nw, XtGrabNone);

  return;
}

/*
 * SRGroupAddCallback
 */
static void
SRGroupAddCallback(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  BookmarkWidget bw = (BookmarkWidget)cldata;

  BREC.cbg = CreateBookmarkGroup((char *)cbdata);
  BREC.cbg->next = BREC.blist;
  BREC.blist = BREC.cbg;

  if (BREC.bgslist != NULL) XtFree((char *)BREC.bgslist);
  if (!BREC.delayWrite) WriteBookmarkFile(bw);
  MakeBookmarkGroupList(bw);
  MakeBookmarkList(bw);

  XtPopdown(w);
  XtDestroyWidget(w);

  return;
}

/*
 * GroupAddFunc
 */
static void
GroupAddFunc(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  CreateStrReq(w, "srgroupadd", "",
	       False, SRGroupAddCallback, (XtPointer)cldata);

  return;
}

/*
 * SRMarkAddCallback
 */
static void
SRMarkAddCallback(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  BookmarkWidget bw = (BookmarkWidget)cldata;
  Bookmark *b;

  if (BREC.cbg != NULL)
  {
    b = CreateBookmark(BREC.nexturl, (char *)cbdata);
    b->next = BREC.cbg->b;
    BREC.cbg->b = b;
    
    if (BREC.bslist != NULL) XtFree((char *)BREC.bslist);
    if (!BREC.delayWrite) WriteBookmarkFile(bw);
  }

  MakeBookmarkList(bw);

  XtPopdown(w);
  XtDestroyWidget(w);

  return;
}

/*
 * MarkAddFunc
 */
static void
MarkAddFunc(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  BookmarkWidget bw = (BookmarkWidget)cldata;

  if (BREC.bookmarkProc != NULL)
  {
    char *display, *url = NULL;

    (BREC.bookmarkProc)(BREC.bookmarkProcData, &display, &url);
    if (url == NULL)
    {
      XtAppWarning(BREC.appcon, BREC.nullURL);
      return;
    }
    if (display == NULL) display = "";

    if (BREC.nexturl != NULL) XtFree(BREC.nexturl);
    BREC.nexturl = url;
    CreateStrReq(w, "srmarkadd", display,
		 False, SRMarkAddCallback, (XtPointer)bw);
    XtFree(display);
  }
  else
  {
    XtAppWarning(BREC.appcon, BREC.noBookmarkProc);
  }

  return;
}

/*
 * BGroupDelCallback
 */
static void
BGroupDelCallback(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  BookmarkWidget bw = (BookmarkWidget)cldata;
  Bookmark *b, *tb;
  BookmarkGroup *tbg;

  if (BREC.cbg != NULL)
  {
    for (b = BREC.cbg->b; b; )
    {
      tb = b;
      b = b->next;
      DestroyBookmark(tb);
    }
    
    for (tbg = BREC.blist; tbg != NULL; tbg = tbg->next)
    {
      if (tbg->next == BREC.cbg) break;
    }

    if (tbg == NULL) BREC.blist = BREC.cbg->next;
    else tbg->next = BREC.cbg->next;
    if (BREC.cbg->name != NULL) XtFree(BREC.cbg->name);
    XtFree((char *)BREC.cbg);
    
    BREC.cbg = BREC.blist;
    if (BREC.bgslist != NULL) XtFree((char *)BREC.bgslist);
    MakeBookmarkGroupList(bw);
    if (!BREC.delayWrite) WriteBookmarkFile(bw);
  }

  MakeBookmarkList(bw);

  return;
}

/*
 * BGroupListCallback
 */
static void
BGroupListCallback(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  XawListReturnStruct *lr = (XawListReturnStruct *)cbdata;
  BookmarkWidget bw = (BookmarkWidget)cldata;

  if (lr != NULL && lr->list_index != -1)
  {
    int i;

    for (i = 0, BREC.cbg = BREC.blist;
	 i < lr->list_index;
	 i++, BREC.cbg = BREC.cbg->next)
	;

    if (BREC.bslist != NULL) XtFree((char *)BREC.bslist);
  }

  MakeBookmarkList(bw);

  return;
}

/*
 * BMarkDelCallback
 */
static void
BMarkDelCallback(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  BookmarkWidget bw = (BookmarkWidget)cldata;
  XawListReturnStruct *lr;
  Bookmark *b, *tb;
  int i;

  lr = XawListShowCurrent(BREC.marklist);
  if (lr != NULL && lr->list_index != -1)
  {
    if (BREC.cbg != NULL)
    {
      for (i = 0, b = BREC.cbg->b, tb = NULL;
	   b != NULL;
	   tb = b, b = b->next, i++)
      {
	if (i == lr->list_index)
	{
	  if (tb == NULL) BREC.cbg->b = b->next;
	  else tb->next = b->next;
	  DestroyBookmark(b);
	  
	  if (BREC.bslist != NULL) XtFree((char *)BREC.bslist);
	  if (!BREC.delayWrite) WriteBookmarkFile(bw);
	  break;
	}
      }
    }
    MakeBookmarkList(bw);
  }
  else
  {
    XtAppWarning(BREC.appcon, BREC.noCurrentMark);
  }

  return;
}

/*
 * BMarkSelect
 */
static void
BMarkSelect(w, event, params, num_params)
Widget w;
XEvent *event;
String *params;
Cardinal *num_params;
{
  BookmarkWidget bw;
  char *url;
  XawListReturnStruct *lr;
  Widget xw;

  for (xw = XtParent(w);
       !XtIsSubclass(xw, bookmarkWidgetClass);
       xw = XtParent(xw))
      ;
  bw = (BookmarkWidget)xw;

  lr = XawListShowCurrent(BREC.marklist);
  if (lr != NULL && lr->list_index != -1)
  {
    if ((url = GetBookmarkURL(bw, lr->list_index)) != NULL)
    {
      url = XtNewString(url);
    }
    XtCallCallbackList((Widget)bw, BREC.callbacks, (XtPointer)url);
    if (url != NULL) XtFree(url);
    if (BREC.pickDestroys)
    {
      XtPopdown((Widget)bw);
      XtDestroyWidget((Widget)bw);
    }
  }
  else
  {
    XtAppWarning(BREC.appcon, BREC.noCurrentMark);
  }

  return;
}

/*
 * DestroyBookmark
 *
 * Deallocate a bookmark
 */
static void
DestroyBookmark(b)
Bookmark *b;
{
  if (b->url) XtFree(b->url);
  if (b->display) XtFree(b->display);
  XtFree((char *)b);

  return;
}

/*
 * CreateBookmark
 */
static Bookmark *
CreateBookmark(url, display)
char *url, *display;
{
  Bookmark *b;

  b = (Bookmark *)XtMalloc(sizeof(Bookmark));
  b->url = XtNewString(url);
  b->display = XtNewString(display);
  b->next = NULL;

  return(b);
}

/*
 * CreateBookmarkGroup
 */
static BookmarkGroup *
CreateBookmarkGroup(name)
char *name;
{
  BookmarkGroup *bg;

  bg = (BookmarkGroup *)XtMalloc(sizeof(BookmarkGroup));
  bg->name = XtNewString(name);
  bg->next = NULL;
  bg->b = NULL;

  return(bg);
}

/*
 * EatSpace
 */
static char *
EatSpace(s)
char *s;
{
  char *cp;

  for (cp = s; *cp; cp++)
  {
    if (!isspace(*cp)) return(cp);
  }
  return(NULL);
}

/*
 * ReadBookmarkFile
 *
 * Read bookmarks from a file.
 */
static void
ReadBookmarkFile(bw)
BookmarkWidget bw;
{
  char *mumble;
  char *filename;
  FILE *fp;
  char *cp;
  char buffer[BUFSIZ];
  char url[BUFSIZ];
  char display[BUFSIZ];
  char group[BUFSIZ];
  int v2 = 0;
  BookmarkGroup *bg = NULL, *tbg = NULL;
  Bookmark *b, *tb = NULL;
  Boolean ispipe;

  if ((filename = EatSpace(BREC.filename)) == NULL)
  {
    XtAppWarning(BREC.appcon, BREC.badBookmarkFilename);
    return;
  }

  if (filename[0] == '|')
  {
    char *format = "%s r";

    mumble = XtMalloc(strlen(filename + 1) + strlen(format) + 1);
    sprintf (mumble, format, filename + 1);
    fp = popen(mumble, "r");
    XtFree(mumble);
    ispipe = True;
  }
  else
  {
    fp = fopen(filename, "r");
    ispipe = False;
  }
  if (fp == NULL)
  {
    BREC.blist = CreateBookmarkGroup("default");
    XtAppWarning(BREC.appcon, BREC.cantReadBookmarkFile);
    return;
  }

  if (fgets(buffer, sizeof(buffer), fp) == NULL)
  {
    if (ispipe) pclose(fp);
    else fclose(fp);
    BREC.blist = CreateBookmarkGroup("default");
    XtAppWarning(BREC.appcon, BREC.cantReadBookmarkFile);
    return;
  }

  if (strncmp(buffer, "chimera_bookmarkv2", 18) == 0) v2 = 1;
  else
  {
    BREC.blist = CreateBookmarkGroup("default");
    sscanf(buffer, "%s %[^\n]", url, display);
    tb = BREC.blist->b = CreateBookmark(url, display);
  }

  while (fgets(buffer, sizeof(buffer), fp))
  {
    if (v2)
    {
      sscanf(buffer, "%s", group);
      if (strcmp(group, "group") == 0)
      {
	cp = (char *)strchr(buffer, ' ');
	if (cp != NULL && *cp != '\0')
	{
	  sscanf(++cp, "%[^\n]", display);
	  bg = CreateBookmarkGroup(display);
	  if (tbg == NULL) BREC.blist = bg;
	  else tbg->next = bg;
	  tbg = bg;
	  tb = NULL;
	}
      }
      else if (strcmp(group, "mark") == 0 && bg != NULL)
      {
	cp = (char *)strchr(buffer, ' ');
	if (cp != NULL && *cp != '\0')
	{
	  sscanf(++cp, "%s %[^\n]", url, display);
	  b = CreateBookmark(url, display);
	  if (bg->b == NULL) bg->b = b;
	  else tb->next = b;
	  tb = b;
	}
      }
    }
    else
    {
      sscanf(buffer, "%s %[^\n]", url, display);
      b = CreateBookmark(url, display);
      tb->next = b;
      tb = b;
    }
  }

  if (ispipe) pclose(fp);
  else fclose(fp);

  if (BREC.blist == NULL) BREC.blist = CreateBookmarkGroup("default");

  return;
}

/*
 * WriteBookmarkFile
 */
static void
WriteBookmarkFile(bw)
BookmarkWidget bw;
{
  FILE *fp;
  Bookmark *b;
  BookmarkGroup *bg;
  char *filename;
  char *mumble;
  Boolean ispipe;

  if ((filename = EatSpace(BREC.filename)) == NULL)
  {
    XtAppWarning(BREC.appcon, BREC.badBookmarkFilename);
    return;
  }

  if (filename[0] == '|')
  {
    char *format = "%s w";

    mumble = XtMalloc(strlen(filename + 1) + strlen(format) + 1);
    sprintf (mumble, format, filename + 1);
    fp = popen(mumble, "w");
    XtFree(mumble);
    ispipe = True;
  }
  else
  {
    fp = fopen(filename, "w");
    ispipe = False;
  }
  if (fp == NULL)
  {
    XtAppWarning(BREC.appcon, BREC.cantWriteBookmarkFile);
    return;
  }

  fprintf (fp, "chimera_bookmarkv2\n");

  for (bg = BREC.blist; bg; bg = bg->next)
  {
    fprintf (fp, "group %s\n", bg->name);
    for (b = bg->b; b; b = b->next)
    {
      fprintf (fp, "mark %s %s\n", b->url, b->display);
    }
    if (feof(fp))
    {
      if (ispipe) pclose(fp);
      else fclose(fp);
      return;
    }
  }

  if (ispipe) pclose(fp);
  else fclose(fp);

  return;
}

/*
 * MakeBookmarkGroupList
 *
 * Makes an array of char pointers to the list of bookmark groups.
 */
static void
MakeBookmarkGroupList(bw)
BookmarkWidget bw;
{
  char **list;
  BookmarkGroup *bg;
  int count, i;

  for (count = 0, bg = BREC.blist; bg; count++, bg = bg->next)
      ;

  if (count == 0)
  {
    list = (char **)XtMalloc(sizeof(char *));
    list[0] = NULL;
    BREC.bgslist = list;
    XawListChange(BREC.grouplist, BREC.bgslist, 0, 0, True);
    return;
  }

  list = (char **)XtMalloc(sizeof(char *) * (count + 1));
  for (bg = BREC.blist, i = 0; bg && i < count; i++, bg = bg->next)
  {
    list[i] = bg->name;
  }
  list[i] = NULL;

  BREC.bgslist = list;
  XawListChange(BREC.grouplist, BREC.bgslist, 0, 0, True);

  return;
}

/*
 * MakeBookmarkList
 *
 * Makes an array of char pointers to the list of bookmarks.
 */
static void
MakeBookmarkList(bw)
BookmarkWidget bw;
{
  int i, count;
  Bookmark *b;
  char **list;

  if (BREC.cbg == NULL)
  {
    if (BREC.currentGroup != NULL)
    {
      XtFree(BREC.currentGroup);
      BREC.currentGroup = NULL;
    }
    XtAppWarning(BREC.appcon, BREC.noCurrentGroup);
    return;
  }

  for (b = BREC.cbg->b, count = 0; b; count++, b = b->next)
      ;
  
  if (count == 0)
  {
    list = (char **)XtMalloc(sizeof(char *));
    list[0] = NULL;
    BREC.bslist = list;
    XawListChange(BREC.marklist, BREC.bslist, 0, 0, True);
    BREC.currentGroup = XtNewString(BREC.cbg->name);
    return;
  }
  
  count += 1;
  list = (char **)XtMalloc(sizeof(char *) * count);
  
  for (i = 0, b = BREC.cbg->b; b && i < count; i++, b = b->next)
  {
    list[i] = b->display;
  }
  list[i] = NULL;
  
  BREC.bslist = list;
  XawListChange(BREC.marklist, BREC.bslist, 0, 0, True);

  BREC.currentGroup = XtNewString(BREC.cbg->name);

  return;
}

/*
 * GetBookmarkURL
 *
 * Returns the URL for a bookmark.
 */
static char *
GetBookmarkURL(bw, index)
BookmarkWidget bw;
int index;
{
  int i;
  Bookmark *b;

  if (BREC.cbg != NULL)
  {
    for (b = BREC.cbg->b, i = 0; b; i++, b = b->next)
    {
      if (i == index) return(b->url);
    }
  }
  
  return(NULL);
}
