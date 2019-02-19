/*
 * BookmarkP.h
 *
 * Copyright (C) 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 */

#include "Bookmark.h"

/************************************
 *
 *  Class structure
 *
 ***********************************/


/* New fields for the Bookmark widget class record */
typedef struct _BookmarkClass 
{
  XtPointer extension;
} BookmarkClassPart;

/* Full class record declaration */
typedef struct _BookmarkClassRec
{
  CoreClassPart           core_class;
  CompositeClassPart      composite_class;
  ShellClassPart          shell_class;
  WMShellClassPart        wm_shell_class;
  VendorShellClassPart    vendor_shell_class;
  TransientShellClassPart transient_shell_class;
  BookmarkClassPart       bookmark_class;
} BookmarkClassRec;

extern BookmarkClassRec bookmarkClassRec;

/***************************************
 *
 *  Instance (widget) structure 
 *
 **************************************/

typedef struct _bookmark
{
  char *url;
  char *display;
  struct _bookmark *next;
} Bookmark;

typedef struct _bookmarkgroup
{
  char *name;
  Bookmark *b;
  struct _bookmarkgroup *next;
} BookmarkGroup;

/* New fields for the Bookmark widget record */
typedef struct
{
  /* public */
  char *filename;
  BookmarkGroup *blist;
  char **bgslist;
  char **bslist;
  XtCallbackList callbacks;
  Boolean delayWrite;
  char *bookmarkMessage;
  BookmarkProc bookmarkProc;
  XtPointer bookmarkProcData;
  Boolean pickDestroys;
  String nullURL;
  String noBookmarkProc;
  String noCurrentGroup;
  String noCurrentMark;
  String cantReadBookmarkFile;
  String cantWriteBookmarkFile;
  String badBookmarkFilename;
  String currentGroup;

  /* private */
  XtAppContext appcon;
  BookmarkGroup *cbg; /* current group */
  char *nexturl;
  Widget paned;
  Widget label;
  Widget groupform;
  Widget groupview;
  Widget grouplist;
  Widget groupbox;
  Widget groupopen;
  Widget groupadd;
  Widget groupdel;
  Widget markform;
  Widget markview;
  Widget marklist;
  Widget markbox;
  Widget markopen;
  Widget markadd;
  Widget markdel;
  Widget dismiss;
} BookmarkPart;

   /* Full widget declaration */
typedef struct _BookmarkRec
{
  CorePart        core;
  CompositePart   composite;
  ShellPart       shell;
  WMShellPart     wm;
  VendorShellPart vendor;
  TransientShellPart transient;
  BookmarkPart    bookmark;
} BookmarkRec;


