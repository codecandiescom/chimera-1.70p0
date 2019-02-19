/*
 * Bookmark.h
 *
 * Copyright (C) 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 */

/* Resources:

 Name		     Class		RepType		Default Value
 ----		     -----		-------		-------------

*/

#define XtNfilename "filename"
#define XtNbookmarkProc "bookmarkProc"
#define XtNdelayWrite "delayWrite"
#define XtNbookmarkMessage "bookmarkMessage"
#define XtNbookmarkProcData "bookmarkProcData"
#define XtNpickDestroys "pickDestroys"
#define XtNnullURL "nullURL"
#define XtNnoBookmarkProc "noBookmarkProc"
#define XtNnoCurrentGroup "noCurrentGroup"
#define XtNnoCurrentMark "noCurrentMark"
#define XtNcantReadBookmarkFile "cantReadBookmarkFile"
#define XtNcantWriteBookmarkFile "cantWriteBookmarkFile"
#define XtNbadBookmarkFilename "badBookmarkFilename"
#define XtNcurrentGroup "currentGroup"

#define XtCBookmarkProc "BookmarkProc"
#define XtCBookmarkProcData "BookmarkProcData"

typedef void (*BookmarkProc)(
#if NeedFunctionPrototypes
    XtPointer,
    char **,               /* default bookmark name */
    char **                /* url */
#endif
);

extern WidgetClass     bookmarkWidgetClass;

typedef struct _BookmarkClassRec   *BookmarkWidgetClass;
typedef struct _BookmarkRec        *BookmarkWidget;
