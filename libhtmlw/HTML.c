/* substantially modified by WBE, Spring 1997. */

/****************************************************************************
 * NCSA Mosaic for the X Window System                                      *
 * Software Development Group                                               *
 * National Center for Supercomputing Applications                          *
 * University of Illinois at Urbana-Champaign                               *
 * 605 E. Springfield, Champaign IL 61820                                   *
 * mosaic@ncsa.uiuc.edu                                                     *
 *                                                                          *
 * Copyright (C) 1993, Board of Trustees of the University of Illinois      *
 *                                                                          *
 * NCSA Mosaic software, both binary and source (hereafter, Software) is    *
 * copyrighted by The Board of Trustees of the University of Illinois       *
 * (UI), and ownership remains with the UI.                                 *
 *                                                                          *
 * The UI grants you (hereafter, Licensee) a license to use the Software    *
 * for academic, research and internal business purposes only, without a    *
 * fee.  Licensee may distribute the binary and source code (if released)   *
 * to third parties provided that the copyright notice and this statement   *
 * appears on all copies and that no charge is associated with such         *
 * copies.                                                                  *
 *                                                                          *
 * Licensee may make derivative works.  However, if Licensee distributes    *
 * any derivative work based on or derived from the Software, then          *
 * Licensee will (1) notify NCSA regarding its distribution of the          *
 * derivative work, and (2) clearly notify users that such derivative       *
 * work is a modified version and not the original NCSA Mosaic              *
 * distributed by the UI.                                                   *
 *                                                                          *
 * Any Licensee wishing to make commercial use of the Software should       *
 * contact the UI, c/o NCSA, to negotiate an appropriate license for such   *
 * commercial use.  Commercial use includes (1) integration of all or       *
 * part of the source code into a product for sale or license by or on      *
 * behalf of Licensee to third parties, or (2) distribution of the binary   *
 * code or source code to third parties that need it to utilize a           *
 * commercial product sold or licensed by or on behalf of Licensee.         *
 *                                                                          *
 * UI MAKES NO REPRESENTATIONS ABOUT THE SUITABILITY OF THIS SOFTWARE FOR   *
 * ANY PURPOSE.  IT IS PROVIDED "AS IS" WITHOUT EXPRESS OR IMPLIED          *
 * WARRANTY.  THE UI SHALL NOT BE LIABLE FOR ANY DAMAGES SUFFERED BY THE    *
 * USERS OF THIS SOFTWARE.                                                  *
 *                                                                          *
 * By using or copying this Software, Licensee agrees to abide by the       *
 * copyright law and all other applicable laws of the U.S. including, but   *
 * not limited to, export control laws, and the terms of this license.      *
 * UI shall have the right to terminate this license immediately by         *
 * written notice upon Licensee's breach of, or non-compliance with, any    *
 * of its terms.  Licensee may be held legally responsible for any          *
 * copyright infringement that is caused or encouraged by Licensee's        *
 * failure to abide by the terms of this license.                           *
 *                                                                          *
 * Comments and questions are welcome and can be sent to                    *
 * mosaic-x@ncsa.uiuc.edu.                                                  *
 ****************************************************************************/

#include <stdio.h>

#include "HTMLP.h"
#include "DrawingArea.h"
#include <X11/Xmu/StdSel.h>
#include <X11/Xaw/Scrollbar.h>
#include <X11/cursorfont.h>

#define	MARGIN_DEFAULT		20
#define	CLICK_TIME		500
#define	SELECT_THRESHOLD	3
#define	MAX_UNDERLINES		3
#define DEFAULT_INCREMENT       18

#ifndef ABS
#  define ABS(x)  ( ((x) > 0) ? (x) : (-(x)) )
#endif

#define	W_TEXTFIELD	0
#define	W_CHECKBOX	1
#define	W_RADIOBOX	2
#define	W_PUSHBUTTON	3
#define	W_PASSWORD	4
#define	W_OPTIONMENU	5


extern int FormatAll ();
extern int DocumentWidth ();
extern void PlaceLine ();
extern void TextRefresh ();
extern void ImageRefresh ();
extern void LinefeedRefresh ();
extern void RefreshTextRange (); /* not actually used --GN */
extern void FreeColors ();
extern void FreeImages ();
extern void HideWidgets ();
extern void MapWidgets ();
extern int SwapElements ();
extern int ElementLessThan ();
extern int IsDelayedHRef ();
extern int IsIsMapForm ();
extern int AnchoredHeight ();
extern char *ParseMarkTag ();
extern char *ParseTextToString ();
extern char *ParseTextToPSString ();
extern struct mark_up *HTMLParse ();
extern struct ele_rec *LocateElement ();
extern struct ele_rec **MakeLineList ();
extern void FreeHRefs ();
extern struct ref_rec *AddHRef ();
extern ImageInfo *NoImageData ();
extern void ImageSubmitForm ();
extern void SetTitle();
extern void SetDeferPixButtonState ();

static void SelectStart ();
static void ExtendStart ();
static void ExtendAdjust ();
static void ExtendEnd ();
static void TrackMotion ();
static Boolean ConvertSelection ();
static void LoseSelection ();
static void SelectionDone ();
static void ScrollUp ();
static void ScrollDown ();


#ifdef _NO_PROTO
static void		CallLinkCallbacks ();
static void		DiscardCurPageObjects ();
static XtGeometryResult GeometryManager ();
static Dimension	HbarHeight ();
static void		_HTMLInput ();
static void		Initialize ();
static void		Redisplay ();
static void		Resize ();
static Boolean		SetValues ();
static Dimension	VbarWidth ();
static void		ViewRedisplay ();
static void		ViewClearAndRefresh ();
#else /* _NO_PROTO */
static void	CallLinkCallbacks (HTMLWidget hw);
static void	DiscardCurPageObjects (HTMLWidget old, HTMLWidget new);
static XtGeometryResult
		GeometryManager (Widget w, XtWidgetGeometry * request,
				 XtWidgetGeometry * reply);
static Dimension  HbarHeight (HTMLWidget hw);
static void	_HTMLInput (Widget w, XEvent * event, String * params,
			    Cardinal * num_params);
static void	Initialize (HTMLWidget request, HTMLWidget new);
static void	Redisplay (HTMLWidget hw, XEvent * event, Region region);
static void	Resize (HTMLWidget hw);
static Boolean	SetValues (HTMLWidget current, HTMLWidget request,
			   HTMLWidget new);
static Dimension  VbarWidth (HTMLWidget hw);
static void	ViewRedisplay (HTMLWidget hw, int x, int y, int width,
			       int height);
static void	ViewClearAndRefresh (HTMLWidget hw);
#endif /* _NO_PROTO */


/*
 * Default translations.
 * Selection of text, and activate anchors.
 * If motif, add manager translations.
 */
static char defaultTranslations[] =
" \
<Btn1Down>:	select-start() \n\
<Btn1Motion>:	extend-adjust() \n\
<Btn1Up>:	extend-end(PRIMARY, CUT_BUFFER0) \n\
<Btn2Down>:	select-start() \n\
<Btn2Motion>:	extend-adjust() \n\
<Btn2Up>:	extend-end(PRIMARY, CUT_BUFFER0) \n\
<Btn3Down>:	extend-start()\n\
<Btn3Motion>:	extend-adjust()\n\
<Btn3Up>:	extend-end(PRIMARY, CUT_BUFFER0) \n\
<Motion>:       track-motion()\n\
<Leave>:        track-motion()\n\
<FocusOut>:     track-motion()\n\
<Expose>:       track-motion()\n\
<Key>space:     scroll-up(\"Halfscreen\")\n\
<Key>b:         scroll-down(\"Halfscreen\")\n\
<Key>j:         scroll-up(\"Oneline\")\n\
<Key>k:         scroll-down(\"Oneline\")\n\
<Key>Down:      scroll-up(\"Oneline\")\n\
<Key>Up:        scroll-down(\"Oneline\")\n\
";
/*
 * Changes above by Satoshi ASAMI <asami@cory.EECS.Berkeley.EDU>
 */


static XtActionsRec actionsList[] =
{
  {"select-start", (XtActionProc) SelectStart},
  {"extend-start", (XtActionProc) ExtendStart},
  {"extend-adjust", (XtActionProc) ExtendAdjust},
  {"extend-end", (XtActionProc) ExtendEnd},
  {"track-motion", (XtActionProc) TrackMotion},
  {"HTMLInput", (XtActionProc) _HTMLInput},
  {"scroll-up", (XtActionProc) ScrollUp},
  {"scroll-down", (XtActionProc) ScrollDown},
};

/*
 * For some reason, in Motif1.2/X11R5 the actionsList above gets corrupted
 * when the parent HTML widget is created.  This means we can't use
 * it later with XtAppAddActions to add to the viewing area.
 * So, we make a spare copy here to use with XtAppAddActions.
 */
static XtActionsRec SpareActionsList[] =
{
  {"select-start", (XtActionProc) SelectStart},
  {"extend-start", (XtActionProc) ExtendStart},
  {"extend-adjust", (XtActionProc) ExtendAdjust},
  {"extend-end", (XtActionProc) ExtendEnd},
  {"track-motion", (XtActionProc) TrackMotion},
  {"HTMLInput", (XtActionProc) _HTMLInput},
  {"scroll-up", (XtActionProc) ScrollUp},
  {"scroll-down", (XtActionProc) ScrollDown},
};


/*
 *  Resource definitions for HTML widget
 */

static XtResource resources[] =
{
  {
    XtNborderWidth,
    XtCBorderWidth, XtRDimension, sizeof (Dimension),
    XtOffset (HTMLWidget, core.border_width),
    XtRImmediate, (XtPointer) 0
  },
  {
    WbNverticalScrollBarPos,
    WbCVerticalScrollBarPos, XtRInt, sizeof (int),
    XtOffset (HTMLWidget, html.scroll_y),
    XtRInt, 0
  },
  {
    WbNhorizontalScrollBarPos,
    WbCHorizontalScrollBarPos, XtRInt, sizeof (int),
    XtOffset (HTMLWidget, html.scroll_x),
    XtRInt, 0
  },
  {
    WbNmarginWidth,
    WbCMarginWidth, XtRDimension, sizeof (Dimension),
    XtOffset (HTMLWidget, html.margin_width),
    XtRImmediate, (caddr_t) MARGIN_DEFAULT
  },
  {
    WbNmarginHeight,
    WbCMarginHeight, XtRDimension, sizeof (Dimension),
    XtOffset (HTMLWidget, html.margin_height),
    XtRImmediate, (caddr_t) MARGIN_DEFAULT
  },
  {
    WbNanchorCallback,
    XtCCallback, XtRCallback, sizeof (XtCallbackList),
    XtOffset (HTMLWidget, html.anchor_callback),
    XtRImmediate, (caddr_t) NULL
  },
  {
    WbNlinkCallback,
    XtCCallback, XtRCallback, sizeof (XtCallbackList),
    XtOffset (HTMLWidget, html.link_callback),
    XtRImmediate, (caddr_t) NULL
  },
  {
    WbNsubmitFormCallback,
    XtCCallback, XtRCallback, sizeof (XtCallbackList),
    XtOffset (HTMLWidget, html.form_callback),
    XtRImmediate, (caddr_t) NULL
  },
  {
    WbNtext,
    WbCText, XtRString, sizeof (char *),
    XtOffset (HTMLWidget, html.raw_text),
    XtRString, (char *) NULL
  },
  {
    WbNheaderText,
    WbCHeaderText, XtRString, sizeof (char *),
    XtOffset (HTMLWidget, html.header_text),
    XtRString, (char *) NULL
  },
  {
    WbNfooterText,
    WbCFooterText, XtRString, sizeof (char *),
    XtOffset (HTMLWidget, html.footer_text),
    XtRString, (char *) NULL
  },
  {
    WbNtitleText,
    WbCTitleText, XtRString, sizeof (char *),
    XtOffset (HTMLWidget, html.title),
    XtRString, (char *) NULL
  },
  {
    XtNforeground,
    XtCForeground, XtRPixel, sizeof (Pixel),
    XtOffset (HTMLWidget, html.foreground),
    XtRString, "Black"
  },
  {
    WbNanchorUnderlines,
    WbCAnchorUnderlines, XtRInt, sizeof (int),
    XtOffset (HTMLWidget, html.num_anchor_underlines),
    XtRString, "0"
  },
  {
    WbNvisitedAnchorUnderlines,
    WbCVisitedAnchorUnderlines, XtRInt, sizeof (int),
    XtOffset (HTMLWidget, html.num_visitedAnchor_underlines),
    XtRString, "0"
  },
  {
    WbNdashedAnchorUnderlines,
    WbCDashedAnchorUnderlines, XtRBoolean, sizeof (Boolean),
    XtOffset (HTMLWidget, html.dashed_anchor_lines),
    XtRString, "False"
  },
  {
    WbNdashedVisitedAnchorUnderlines,
    WbCDashedVisitedAnchorUnderlines, XtRBoolean, sizeof (Boolean),
    XtOffset (HTMLWidget, html.dashed_visitedAnchor_lines),
    XtRString, "False"
  },
  {
    WbNanchorColor,
    XtCForeground, XtRPixel, sizeof (Pixel),
    XtOffset (HTMLWidget, html.anchor_fg),
    XtRString, "blue2"
  },
  {
    WbNvisitedAnchorColor,
    XtCForeground, XtRPixel, sizeof (Pixel),
    XtOffset (HTMLWidget, html.visitedAnchor_fg),
    XtRString, "purple4"
  },
  {
    WbNactiveAnchorFG,
    XtCBackground, XtRPixel, sizeof (Pixel),
    XtOffset (HTMLWidget, html.activeAnchor_fg),
    XtRString, "Red"
  },
  {
    WbNactiveAnchorBG,
    XtCForeground, XtRPixel, sizeof (Pixel),
    XtOffset (HTMLWidget, html.activeAnchor_bg),
    XtRString, "White"
  },
  {
    WbNpercentVerticalSpace,
    WbCPercentVerticalSpace, XtRInt, sizeof (int),
    XtOffset (HTMLWidget, html.percent_vert_space),
    XtRString, "90"
  },
  {
    WbNhtmlErrorMsgCutoff,
    WbCHTMLErrorMsgCutoff, XtRInt, sizeof (int),
    XtOffset (HTMLWidget, html.html_errmsg_cutoff),
    XtRString, "0"
  },
  {
    WbNimageBorders,
    WbCImageBorders, XtRBoolean, sizeof (Boolean),
    XtOffset (HTMLWidget, html.border_images),
    XtRString, "False"
  },
  {
    WbNdelayImageLoads,
    WbCDelayImageLoads, XtRBoolean, sizeof (Boolean),
    XtOffset (HTMLWidget, html.delay_images),
    XtRString, "False"
  },
  {
    WbNfancySelections,
    WbCFancySelections, XtRBoolean, sizeof (Boolean),
    XtOffset (HTMLWidget, html.fancy_selections),
    XtRString, "False"
  },
  {
    WbNisIndex,
    WbCIsIndex, XtRBoolean, sizeof (Boolean),
    XtOffset (HTMLWidget, html.is_index),
    XtRString, "False"
  },
  {
    WbNview,
    WbCView, XtRWidget, sizeof (Widget),
    XtOffset (HTMLWidget, html.view),
    XtRImmediate, NULL
  },
  {
    WbNverticalScrollBar,
    WbCVerticalScrollBar, XtRWidget, sizeof (Widget),
    XtOffset (HTMLWidget, html.vbar),
    XtRImmediate, NULL
  },
  {
    WbNhorizontalScrollBar,
    WbCHorizontalScrollBar, XtRWidget, sizeof (Widget),
    XtOffset (HTMLWidget, html.hbar),
    XtRImmediate, NULL
  },
  {
    WbNverticalScrollOnRight,
    WbCVerticalScrollOnRight, XtRBoolean, sizeof (Boolean),
    XtOffset (HTMLWidget, html.vbar_right),
    XtRString, "False"
  },
  {
    WbNhorizontalScrollOnTop,
    WbCHorizontalScrollOnTop, XtRBoolean, sizeof (Boolean),
    XtOffset (HTMLWidget, html.hbar_top),
    XtRString, "False"
  },
  {
    XtNfont,
    XtCFont, XtRFontStruct, sizeof (XFontStruct *),
    XtOffset (HTMLWidget, html.font),
    XtRString, "-adobe-times-medium-r-normal-*-14-*-*-*-*-*-*-*"
  },
  {
    WbNitalicFont,
    WbCItalicFont, XtRFontStruct, sizeof (XFontStruct *),
    XtOffset (HTMLWidget, html.italic_font),
    XtRString, "-adobe-times-medium-i-normal-*-14-*-*-*-*-*-*-*"
  },
  {
    WbNboldFont,
    WbCBoldFont, XtRFontStruct, sizeof (XFontStruct *),
    XtOffset (HTMLWidget, html.bold_font),
    XtRString, "-adobe-times-bold-r-normal-*-14-*-*-*-*-*-*-*"
  },
  {
    WbNfixedFont,
    WbCFixedFont, XtRFontStruct, sizeof (XFontStruct *),
    XtOffset (HTMLWidget, html.fixed_font),
    XtRString, "-adobe-courier-medium-r-normal-*-14-*-*-*-*-*-*-*"
  },
  {
    WbNfixedboldFont,
    WbCFixedboldFont, XtRFontStruct, sizeof (XFontStruct *),
    XtOffset (HTMLWidget, html.fixedbold_font),
    XtRString, "-adobe-courier-bold-r-normal-*-14-*-*-*-*-*-*-*"
  },
  {
    WbNfixeditalicFont,
    WbCFixeditalicFont, XtRFontStruct, sizeof (XFontStruct *),
    XtOffset (HTMLWidget, html.fixeditalic_font),
    XtRString, "-adobe-courier-medium-o-normal-*-14-*-*-*-*-*-*-*"
  },
  {
    WbNheader1Font,
    WbCHeader1Font, XtRFontStruct, sizeof (XFontStruct *),
    XtOffset (HTMLWidget, html.header1_font),
    XtRString, "-adobe-times-bold-r-normal-*-24-*-*-*-*-*-*-*"
  },
  {
    WbNheader2Font,
    WbCHeader2Font, XtRFontStruct, sizeof (XFontStruct *),
    XtOffset (HTMLWidget, html.header2_font),
    XtRString, "-adobe-times-bold-r-normal-*-18-*-*-*-*-*-*-*"
  },
  {
    WbNheader3Font,
    WbCHeader3Font, XtRFontStruct, sizeof (XFontStruct *),
    XtOffset (HTMLWidget, html.header3_font),
    XtRString, "-adobe-times-bold-r-normal-*-17-*-*-*-*-*-*-*"
  },
  {
    WbNheader4Font,
    WbCHeader4Font, XtRFontStruct, sizeof (XFontStruct *),
    XtOffset (HTMLWidget, html.header4_font),
    XtRString, "-adobe-times-bold-r-normal-*-14-*-*-*-*-*-*-*"
  },
  {
    WbNheader5Font,
    WbCHeader5Font, XtRFontStruct, sizeof (XFontStruct *),
    XtOffset (HTMLWidget, html.header5_font),
    XtRString, "-adobe-times-bold-r-normal-*-12-*-*-*-*-*-*-*"
  },
  {
    WbNheader6Font,
    WbCHeader6Font, XtRFontStruct, sizeof (XFontStruct *),
    XtOffset (HTMLWidget, html.header6_font),
    XtRString, "-adobe-times-bold-r-normal-*-10-*-*-*-*-*-*-*"
  },
  {
    WbNaddressFont,
    WbCAddressFont, XtRFontStruct, sizeof (XFontStruct *),
    XtOffset (HTMLWidget, html.address_font),
    XtRString, "-adobe-times-medium-i-normal-*-14-*-*-*-*-*-*-*"
  },
  {
    WbNplainFont,
    WbCPlainFont, XtRFontStruct, sizeof (XFontStruct *),
    XtOffset (HTMLWidget, html.plain_font),
    XtRString, "-adobe-courier-medium-r-normal-*-14-*-*-*-*-*-*-*"
  },
  {
    WbNplainboldFont,
    WbCPlainboldFont, XtRFontStruct, sizeof (XFontStruct *),
    XtOffset (HTMLWidget, html.plainbold_font),
    XtRString, "-adobe-courier-bold-r-normal-*-14-*-*-*-*-*-*-*"
  },
  {
    WbNplainitalicFont,
    WbCPlainitalicFont, XtRFontStruct, sizeof (XFontStruct *),
    XtOffset (HTMLWidget, html.plainitalic_font),
    XtRString, "-adobe-courier-medium-o-normal-*-14-*-*-*-*-*-*-*"
  },
  {
    WbNlistingFont,
    WbCListingFont, XtRFontStruct, sizeof (XFontStruct *),
    XtOffset (HTMLWidget, html.listing_font),
    XtRString, "-adobe-courier-medium-r-normal-*-12-*-*-*-*-*-*-*"
  },
  {
    WbNpreviouslyVisitedTestFunction,
    WbCPreviouslyVisitedTestFunction, XtRPointer,
    sizeof (XtPointer),
    XtOffset (HTMLWidget, html.previously_visited_test),
    XtRImmediate, (caddr_t) NULL
  },
  {
    WbNresolveImageFunction,
    WbCResolveImageFunction, XtRPointer,
    sizeof (XtPointer),
    XtOffset (HTMLWidget, html.resolveImage),
    XtRImmediate, (caddr_t) NULL
  },
  {
    WbNpointerMotionCallback,
    WbCPointerMotionCallback, XtRPointer,
    sizeof (XtPointer),
    XtOffset (HTMLWidget, html.pointer_motion_callback),
    XtRImmediate, (caddr_t) NULL
  },

};



HTMLClassRec htmlClassRec =
{
  {				/* core class fields */
    (WidgetClass) & constraintClassRec,		/* superclass */
    "HTML",			/* class_name */
    sizeof (HTMLRec),		/* widget_size */
    NULL,			/* class_initialize */
    NULL,			/* class_part_init */
    FALSE,			/* class_inited */
    (XtInitProc) Initialize,	/* initialize */
    NULL,			/* initialize_hook */
    XtInheritRealize,		/* realize */
    actionsList,		/* actions */
    XtNumber (actionsList),	/* num_actions */
    resources,			/* resources */
    XtNumber (resources),	/* num_resources */
    NULLQUARK,			/* xrm_class */
    TRUE,			/* compress_motion */
    FALSE,			/* compress_exposure */
    TRUE,			/* compress_enterlv */
    FALSE,			/* visible_interest */
    NULL,			/* destroy */
    (XtWidgetProc) Resize,	/* resize */
    (XtExposeProc) Redisplay,	/* expose */
    (XtSetValuesFunc) SetValues, /* set_values */
    NULL,			/* set_values_hook */
    XtInheritSetValuesAlmost,	/* set_values_almost */
    NULL,			/* get_values_hook */
    NULL,			/* accept_focus */
    XtVersion,			/* version */
    NULL,			/* callback_private */
    defaultTranslations,	/* tm_table */
    XtInheritQueryGeometry,	/* query_geometry */
    XtInheritDisplayAccelerator, /* display_accelerator */
    NULL,			/* extension */
  },

  {				/* composite_class fields */
    (XtGeometryHandler) GeometryManager, /* geometry_manager */
    NULL,			/* change_managed */
    XtInheritInsertChild,	/* insert_child */
    XtInheritDeleteChild,	/* delete_child */
    NULL,			/* extension */
  },

  {				/* constraint_class fields */
    NULL,			/* resource list */
    0,				/* num resources */
    0,				/* constraint size */
    NULL,			/* init proc */
    NULL,			/* destroy proc */
    NULL,			/* set values proc */
    NULL,			/* extension */
  },

  {				/* html_class fields */
    0				/* none */
  }
};

WidgetClass htmlWidgetClass = (WidgetClass) & htmlClassRec;

static Cursor in_anchor_cursor = (Cursor) NULL;


/*
 * SetDelayedImageLoading()  and  ToggleDelayedImageLoading()
 *
 * Set or toggle hw->html.delay_images
 * Called from main.c.
 */
void
SetDelayedImageLoading (hw, defer)  /* used when DeferPix button present */
HTMLWidget hw;
Boolean defer;
{
  hw->html.delay_images = defer ?  True : False;
}


void
ToggleDelayedImageLoading (hw)	/* used when DeferPix button absent */
HTMLWidget hw;
{
  hw->html.delay_images = !hw->html.delay_images;
}


/*
 * Process an expose event in the View (or drawing area).  This
 * can be a regular expose event, or perhaps a GraphicsExpose Event.
 */
static void
DrawExpose (w, data, event)
Widget w;
caddr_t data;
XEvent *event;
{
  XExposeEvent *ExEvent = (XExposeEvent *) event;
  HTMLWidget hw = (HTMLWidget) data;
  int x, y;
  int width, height;

  if ((event->xany.type != Expose) && (event->xany.type != GraphicsExpose))
  {
    return;
  }

  /*
   * Make sure we have a valid GC to draw with.
   */
  if (hw->html.drawGC == NULL)
  {
    unsigned long valuemask;
    XGCValues values;

    values.function = GXcopy;
    values.plane_mask = AllPlanes;
    values.foreground = hw->html.foreground;
    values.background = hw->core.background_pixel;

    valuemask = GCFunction | GCPlaneMask | GCForeground | GCBackground;

    hw->html.drawGC = XCreateGC (XtDisplay (hw), XtWindow (hw),
				 valuemask, &values);
  }

  x = ExEvent->x;
  y = ExEvent->y;
  width = (int) ExEvent->width;
  height = (int) ExEvent->height;

#ifdef DEBUG
  DebugHook (x, y, width, height);
#endif

  ViewRedisplay (hw, x, y, width, height);
}


void
ScrollWidgets (hw)
HTMLWidget hw;
{
  WidgetInfo *wptr;
  int xval, yval;

  xval = hw->html.scroll_x;
  yval = hw->html.scroll_y;
  wptr = hw->html.widget_list;
  while (wptr != NULL)
  {
    if (wptr->w != NULL)
    {
      Widget w;
      int x, y;

      w = wptr->w;
      x = wptr->x;
      y = wptr->y;
      XtMoveWidget (w, (x - xval), (y - yval));
    }
    wptr = wptr->next;
  }
}


/*
 * Set the Athena Scrollbar's thumb position properly.
 */
static void
setScrollBar (sb, topPosition, totalLength, currentLength)
Widget sb;
int topPosition;
int totalLength, currentLength;
{
  float top = (float) topPosition / (float) (totalLength);
  float shown = (float) currentLength / (float) (totalLength);

#if !defined(ultrix)
#if defined(XRELEASE)
#if XRELEASE > 4
  XawScrollbarSetThumb (sb, top, shown);
#endif
#endif
#else
  XawScrollbarSetThumb (sb, top, shown);
#endif
}


/*
 * Either the vertical or hortizontal scrollbar has been moved
 */
void
ScrollToPos (w, hw, value)
Widget w;
HTMLWidget hw;
int value;
{
  /*
   * Special code incase the scrollbar is "moved" before we have a window
   * (if we have a GC we have a window)
   */
  if (hw->html.drawGC == NULL)
  {
    if (w == hw->html.vbar)
    {
      hw->html.scroll_y = value;
    }
    else if (w == hw->html.hbar)
    {
      hw->html.scroll_x = value;
    }
    return;
  }

  /*
   * get our widgets out of the way (No Expose events)
   HideWidgets(hw);
   */

  /*
   * If we've moved the vertical scrollbar
   */
  if (w == hw->html.vbar)
  {
    /*
     * We've scrolled down. Copy up the untouched part of the
     * window.  Then Clear and redraw the new area
     * exposed.
     */
    if (value > hw->html.scroll_y)
    {
      int dy;

      dy = value - hw->html.scroll_y;
      if (dy > hw->html.view_height)
      {
	hw->html.scroll_y = value;
	XClearArea (XtDisplay (hw->html.view),
		    XtWindow (hw->html.view),
		    0, 0,
		    hw->html.view_width,
		    hw->html.view_height, False);
	ViewRedisplay (hw,
		       0, 0,
		       hw->html.view_width,
		       hw->html.view_height);
      }
      else
      {
	XCopyArea (XtDisplay (hw->html.view),
		   XtWindow (hw->html.view),
		   XtWindow (hw->html.view),
		   hw->html.drawGC, 0, dy,
		   hw->html.view_width,
		   hw->html.view_height - dy,
		   0, 0);
	hw->html.scroll_y = value;
	XClearArea (XtDisplay (hw->html.view),
		    XtWindow (hw->html.view),
		    0, (int) hw->html.view_height - dy,
		    hw->html.view_width, dy, False);
	ViewRedisplay (hw,
		       0, (int) hw->html.view_height - dy,
		       hw->html.view_width, dy);
      }
    }
    /*
     * We've scrolled up. Copy down the untouched part of the
     * window.  Then Clear and redraw the new area
     * exposed.
     */
    else if (value < hw->html.scroll_y)
    {
      int dy;

      dy = hw->html.scroll_y - value;
      if (dy > hw->html.view_height)
      {
	hw->html.scroll_y = value;
	XClearArea (XtDisplay (hw->html.view),
		    XtWindow (hw->html.view),
		    0, 0,
		    hw->html.view_width,
		    hw->html.view_height, False);
	ViewRedisplay (hw,
		       0, 0,
		       hw->html.view_width,
		       hw->html.view_height);
      }
      else
      {
	XCopyArea (XtDisplay (hw->html.view),
		   XtWindow (hw->html.view),
		   XtWindow (hw->html.view),
		   hw->html.drawGC, 0, 0,
		   hw->html.view_width,
		   hw->html.view_height - dy,
		   0, dy);
	hw->html.scroll_y = value;
	XClearArea (XtDisplay (hw->html.view),
		    XtWindow (hw->html.view),
		    0, 0,
		    hw->html.view_width, dy, False);
	ViewRedisplay (hw,
		       0, 0,
		       hw->html.view_width, dy);
      }
    }
  }
  /*
   * Else we've moved the horizontal scrollbar
   */
  else if (w == hw->html.hbar)
  {
    /*
     * We've scrolled right. Copy left the untouched part of the
     * window.  Then Clear and redraw the new area
     * exposed.
     */
    if (value > hw->html.scroll_x)
    {
      int dx;

      dx = value - hw->html.scroll_x;
      if (dx > hw->html.view_width)
      {
	hw->html.scroll_x = value;
	XClearArea (XtDisplay (hw->html.view),
		    XtWindow (hw->html.view),
		    0, 0,
		    hw->html.view_width,
		    hw->html.view_height, False);
	ViewRedisplay (hw,
		       0, 0,
		       hw->html.view_width,
		       hw->html.view_height);
      }
      else
      {
	XCopyArea (XtDisplay (hw->html.view),
		   XtWindow (hw->html.view),
		   XtWindow (hw->html.view),
		   hw->html.drawGC, dx, 0,
		   hw->html.view_width - dx,
		   hw->html.view_height,
		   0, 0);
	hw->html.scroll_x = value;
	XClearArea (XtDisplay (hw->html.view),
		    XtWindow (hw->html.view),
		    (int) hw->html.view_width - dx, 0,
		    dx, hw->html.view_height, False);
	ViewRedisplay (hw,
		       (int) hw->html.view_width - dx, 0,
		       dx, hw->html.view_height);
      }
    }
    /*
     * We've scrolled left. Copy right the untouched part of the
     * window.  Then Clear and redraw the new area
     * exposed.
     */
    else if (value < hw->html.scroll_x)
    {
      int dx;

      dx = hw->html.scroll_x - value;
      if (dx > hw->html.view_width)
      {
	hw->html.scroll_x = value;
	XClearArea (XtDisplay (hw->html.view),
		    XtWindow (hw->html.view),
		    0, 0,
		    hw->html.view_width,
		    hw->html.view_height, False);
	ViewRedisplay (hw,
		       0, 0,
		       hw->html.view_width,
		       hw->html.view_height);
      }
      else
      {
	XCopyArea (XtDisplay (hw->html.view),
		   XtWindow (hw->html.view),
		   XtWindow (hw->html.view),
		   hw->html.drawGC, 0, 0,
		   hw->html.view_width - dx,
		   hw->html.view_height,
		   dx, 0);
	hw->html.scroll_x = value;
	XClearArea (XtDisplay (hw->html.view),
		    XtWindow (hw->html.view),
		    0, 0,
		    dx, hw->html.view_height, False);
	ViewRedisplay (hw,
		       0, 0,
		       dx, hw->html.view_height);
      }
    }
  }

  /*
   * Move the now hidden widgets
   * Flush any Copy'ed or Cleared text first.
   XFlush(XtDisplay(hw));
   */
  ScrollWidgets (hw);

  /*
   * Remap the widgets to their new location
   MapWidgets(hw);
   */
}


/*
 * Either the vertical or hortizontal scrollbar has been moved
 */
void
ScrollMove (w, client_data, call_data)
Widget w;
caddr_t client_data;
caddr_t call_data;
{
  float scrollDir = (int) call_data < 0 ? -0.3 : 0.3;
  HTMLWidget hw = (HTMLWidget) client_data;
  int value, maxv;
  int totalLength, currentLength;

  if (w == hw->html.vbar)
  {
    totalLength = hw->html.doc_height;
    currentLength = hw->html.view_height;
/*
   value = hw->html.scroll_y + scrollDir * currentLength; 
 */
    /*
     * patch by Satoshi Asami <asami@cs.berkeley.edu>
     */
    if ((int) call_data == 1 || (int) call_data == -1)
      value = hw->html.scroll_y + scrollDir * currentLength;
    else
      value = hw->html.scroll_y + (int) call_data;
  }
  else
  {
    totalLength = hw->html.doc_width;
    currentLength = hw->html.view_width;
    value = hw->html.scroll_x + scrollDir * currentLength;
  }

  /*
   * If resizing of the document has made the scroll value
   * greater than the max, we want to hold it at the max.
   *
   * Moved this from someplace else in the code to keep the scrollbar
   * from going too far. john.
   */
  maxv = totalLength - currentLength;
  if (value > maxv)
    value = maxv;
  if (value < 0)
    value = 0;

  setScrollBar (w, value, totalLength, currentLength);
  ScrollToPos (w, hw, value);
}


void
JumpMove (w, client_data, call_data)
Widget w;
caddr_t client_data;
caddr_t call_data;
{
  HTMLWidget hw = (HTMLWidget) client_data;
  int value = (int) (*(float *) call_data *
		     (w == hw->html.vbar ?
		      hw->html.doc_height :
		      hw->html.doc_width));
  ScrollToPos (w, hw, value);
}


/*
 * Create the horizontal and vertical scroll bars.
 * Size them later.
 */
static void
CreateScrollbars (hw)
HTMLWidget hw;
{
  Arg arg[20];
  Cardinal argcnt;
  XtTranslations trans;

  /*
   * If the user hasn't provided a viewing area Widget (which they
   * should not most of the time) make a drawing area to use.
   */
  if (hw->html.view == NULL)
  {
    argcnt = 0;
    XtSetArg (arg[argcnt], XxNwidth, 10);
    argcnt++;
    XtSetArg (arg[argcnt], XxNheight, 10);
    argcnt++;
    hw->html.view = XtCreateWidget ("View",
				    drawingAreaWidgetClass,
				    (Widget) hw, arg, argcnt);
    XtManageChild (hw->html.view);
  }

  /*
   * For the view widget catch all Expose and GraphicsExpose
   * events.  Replace its translations with ours, and make
   * sure all the actions are in order.
   */
  XtAddEventHandler ((Widget) hw->html.view, ExposureMask, True,
		     (XtEventHandler) DrawExpose, (caddr_t) hw);
  /*
   * As described previoisly, for some reason with Motif1.2/X11R5
   * the list actionsList is corrupted when we get here,
   * so we have to use the special copy SpareActionsList
   */
  XtAppAddActions (XtWidgetToApplicationContext (hw->html.view),
		   SpareActionsList, XtNumber (SpareActionsList));
  trans = XtParseTranslationTable (defaultTranslations);
  argcnt = 0;
  XtSetArg (arg[argcnt], XtNtranslations, trans);
  argcnt++;
  XtSetValues (hw->html.view, arg, argcnt);

  /*
   * If the user hasn't provided a vertical scrollbar (which they
   * should not most of the time) make one.
   */
  if (hw->html.vbar == NULL)
  {
    argcnt = 0;
    XtSetArg (arg[argcnt], XtNorientation, XtorientVertical);
    argcnt++;
    hw->html.vbar = XtCreateWidget ("Vbar", scrollbarWidgetClass,
				    (Widget) hw, arg, argcnt);
    XtManageChild (hw->html.vbar);
  }

  /*
   * Add callbacks to catch scrollbar changes
   */
  XtAddCallback (hw->html.vbar, XtNjumpProc,
		 (XtCallbackProc) JumpMove, (caddr_t) hw);
  XtAddCallback (hw->html.vbar, XtNscrollProc,
		 (XtCallbackProc) ScrollMove, (caddr_t) hw);

  /*
   * If the user hasn't provided a horizontal scrollbar (which they
   * should not most of the time) make one.
   */
  if (hw->html.hbar == NULL)
  {
    argcnt = 0;
    XtSetArg (arg[argcnt], XtNorientation, XtorientHorizontal);
    argcnt++;
    hw->html.hbar = XtCreateWidget ("Hbar", scrollbarWidgetClass,
				    (Widget) hw, arg, argcnt);
    XtManageChild (hw->html.hbar);
  }

  /*
   * Add callbacks to catch scrollbar changes
   */
  XtAddCallback (hw->html.hbar, XtNjumpProc,
		 (XtCallbackProc) JumpMove, (caddr_t) hw);
  XtAddCallback (hw->html.hbar, XtNscrollProc,
		 (XtCallbackProc) ScrollMove, (caddr_t) hw);
}


/*
 * Return the width of the vertical scrollbar
 */
static Dimension
VbarWidth (hw)
HTMLWidget hw;
{
  Arg arg[4];
  Cardinal argcnt;
  Dimension width;

  width = 0;
  if (hw->html.vbar != NULL)
  {
    argcnt = 0;
    XtSetArg (arg[argcnt], XxNwidth, &width);
    argcnt++;
    XtGetValues (hw->html.vbar, arg, argcnt);
  }

  return (width);
}


/*
 * Return the height of the horizontal scrollbar
 */
static Dimension
HbarHeight (hw)
HTMLWidget hw;
{
  Arg arg[4];
  Cardinal argcnt;
  Dimension height;

  height = 0;
  if (hw->html.hbar != NULL)
  {
    argcnt = 0;
    XtSetArg (arg[argcnt], XxNheight, &height);
    argcnt++;
    XtGetValues (hw->html.hbar, arg, argcnt);
  }

  return (height);
}


/*
 * Resize and set the min and max values of the scrollbars.  Position viewing
 * area based on scrollbar locations.
 */
static void
ConfigScrollBars (hw)
HTMLWidget hw;
{
  int vx, vy;

  /*
   * Move and size the viewing area
   */
  vx = vy = 0;
  if ((hw->html.use_vbar == True) && (hw->html.vbar_right == False))
  {
    vx += VbarWidth (hw);
  }
  if ((hw->html.use_hbar == True) && (hw->html.hbar_top == True))
  {
    vy += HbarHeight (hw);
  }
  XtMoveWidget (hw->html.view, vx, vy);
  XtResizeWidget (hw->html.view, hw->html.view_width, hw->html.view_height,
		  hw->html.view->core.border_width);

  /*
   * Set up vertical scrollbar
   */
  if (hw->html.use_vbar == True)
  {
    int maxv;
    int ss;

    /*
     * Size the vertical scrollbar to the height of
     * the viewing area
     */
    XtResizeWidget (hw->html.vbar, hw->html.vbar->core.width,
		    hw->html.view_height + (2 * 0),
		    hw->html.vbar->core.border_width);

    /*
     * Set the slider size to be the percentage of the
     * viewing area that the viewing area is of the
     * document area.  Or set it to 1 if that isn't possible.
     */
    if (hw->html.doc_height == 0)
    {
      ss = 1;
    }
    else
    {
#ifdef DEBUG
      fprintf (stderr, "view_height %d, doc_height %d\n",
	       hw->html.view_height, hw->html.doc_height);
#endif
#ifdef NOT_RIGHT
      /*
         Eric -- your previous equation wasn't doing it.
         This isn't either... 
       */
      ss =
	(int) ((float) hw->html.view_height *
	       ((float) hw->html.view_height /
		(float) (hw->html.doc_height - (int) hw->html.view_height)));
      if (ss > hw->html.view_height)
      {
	ss = hw->html.view_height;
      }
#endif
      /*
         Added by marca: this produces results *very* close (~1 pixel)
         to the original scrolled window behavior. 
       */
      ss = hw->html.view_height;
    }
    if (ss < 1)
    {
      ss = 1;
    }
#ifdef DEBUG
    fprintf (stderr, "computed ss to be %d\n", ss);
#endif

    /*
     * If resizing of the document has made scroll_y
     * greater than the max, we want to hold it at the max.
     */
    maxv = hw->html.doc_height - (int) hw->html.view_height;
    if (maxv < 0)
    {
      maxv = 0;
    }
    if (hw->html.scroll_y > maxv)
    {
      hw->html.scroll_y = maxv;
    }

    /*
     * Prevent the Motif max value and slider size
     * from going to zero, which is illegal
     */
    maxv = maxv + ss;
    if (maxv < 1)
    {
      maxv = 1;
    }

    /*
     * Motif will not allow the actual value to be equal to
     * its max value.  Adjust accordingly.
     * Since we might decrease scroll_y, cap it at zero.
     */
    if (hw->html.scroll_y >= maxv)
    {
      hw->html.scroll_y = maxv - 1;
    }
    if (hw->html.scroll_y < 0)
    {
      hw->html.scroll_y = 0;
    }

    setScrollBar (hw->html.vbar,
		  hw->html.scroll_y,
		  hw->html.doc_height,
		  hw->html.view_height);

#ifdef DEBUG
    XtVaGetValues (hw->html.vbar, XmNsliderSize, &ss, NULL);
    fprintf (stderr, "real slider size %d\n", ss);
#endif
  }

  /*
   * Set up horizontal scrollbar
   */
  if (hw->html.use_hbar == True)
  {
    int maxv;
    int ss;

    /*
     * Size the horizontal scrollbar to the width of
     * the viewing area
     */
    XtResizeWidget (hw->html.hbar,
		    hw->html.view_width,
		    hw->html.hbar->core.height,
		    hw->html.hbar->core.border_width);

    /*
     * Set the slider size to be the percentage of the
     * viewing area that the viewing area is of the
     * document area.  Or set it to 1 if that isn't possible.
     */
    if (hw->html.doc_width == 0)
    {
      ss = 1;
    }
    else
    {
#ifdef NOT_RIGHT
      ss = hw->html.view_width *
	hw->html.view_width / hw->html.doc_width;
      if (ss > hw->html.view_width)
      {
	ss = hw->html.view_width;
      }
#endif
      /*
         Added by marca: this produces results *very* close (~1 pixel)
         to the original scrolled window behavior. 
       */
      ss = hw->html.view_width;
    }
    if (ss < 1)
    {
      ss = 1;
    }

    /*
     * If resizing of the document has made scroll_x
     * greater than the max, we want to hold it at the max.
     */
    maxv = hw->html.doc_width - (int) hw->html.view_width;
    if (maxv < 0)
    {
      maxv = 0;
    }
    if (hw->html.scroll_x > maxv)
    {
      hw->html.scroll_x = maxv;
    }

    /*
     * Prevent the Motif max value and slider size
     * from going to zero, which is illegal
     */
    maxv = maxv + ss;
    if (maxv < 1)
    {
      maxv = 1;
    }

    /*
     * Motif will not allow the actual value to be equal to
     * its max value.  Adjust accordingly.
     * Since we might decrease scroll_x, cap it at zero.
     */
    if (hw->html.scroll_x >= maxv)
    {
      hw->html.scroll_x = maxv - 1;
    }
    if (hw->html.scroll_x < 0)
    {
      hw->html.scroll_x = 0;
    }

    setScrollBar (hw->html.hbar,
		  hw->html.scroll_x,
		  hw->html.doc_width,
		  hw->html.view_width);
  }

#ifdef DEBUG
  {
    int ss;
    XtVaGetValues (hw->html.vbar, XmNsliderSize, &ss, NULL);
    fprintf (stderr, "real slider size %d\n", ss);
  }
#endif
}


/*
 * Reformat the window and scrollbars.
 * May be called because of a changed document, or because of a changed
 * window size.
 */
static void
ReformatWindow (hw)
HTMLWidget hw;
{
  int temp;
  int new_width;
  Dimension swidth, sheight;
  Dimension st;

  /*
   * Find the current scrollbar sizes, and shadow thickness.
   * Format the document to the current window width.
   * (assume a vertical scrollbar)
   */
  swidth = VbarWidth (hw);
  sheight = HbarHeight (hw);
  st = 0;
  if (hw->core.width <= swidth)
  {
    hw->core.width = swidth + 10;
  }
  new_width = hw->core.width - swidth - (2 * st);
  temp = FormatAll (hw, &new_width);

  /*
   * If we need the vertical scrollbar, place and manage it,
   * and store the current viewing area width.
   */
  if (temp > hw->core.height - sheight)
  {
    hw->html.use_vbar = True;
    if (hw->html.vbar_right == True)
    {
      XtMoveWidget (hw->html.vbar,
		    (hw->core.width - swidth), 0);
    }
    else
    {
      XtMoveWidget (hw->html.vbar, 0, 0);
    }
    XtManageChild (hw->html.vbar);
    hw->html.view_width = hw->core.width - swidth - (2 * st);
  }
  /*
   * Else we were wrong to assume a vertical scrollbar.
   * Remove it, and reformat the document to the wider width.
   * Save the new width as the current viewing area width.
   */
  else
  {
    hw->html.use_vbar = False;
    XtUnmanageChild (hw->html.vbar);
    hw->html.scroll_y = 0;
    new_width = hw->core.width - (2 * st);
    temp = FormatAll (hw, &new_width);
    hw->html.view_width = hw->core.width - (2 * st);
    /*
       fake out later horizontal scrollbars 
     */
    swidth = 0;
  }

  /*
   * Calculate the actual max width and height of the complete
   * formatted document.
   * The max width may exceed the preformatted width due to special
   * factors in the formatting of the widget.
   * Use the max of the 2 here, but leave max_pre_width unchanged
   * for future formatting calls.
   */
  /*
   * new_width includes the margins, and hw->html.max_pre_width
   * does not, fix that here.
   */
  new_width = new_width - (2 * hw->html.margin_width);
  if (hw->html.max_pre_width > new_width)
  {
    new_width = hw->html.max_pre_width;
  }
  /*
   * If the maximum width derives from a formatted, as opposed to
   * unformatted piece of text, allow a 20% of margin width slop
   * over into the margin to cover up a minor glick with terminating
   * punctuation after anchors at the end of the line.
   */
  else
  {
    new_width = new_width - (20 * hw->html.margin_width / 100);
  }

  hw->html.doc_height = temp;
  hw->html.doc_width = new_width + (2 * hw->html.margin_width);
  if (hw->html.view_width > hw->html.doc_width)
  {
    hw->html.doc_width = hw->html.view_width;
  }

  /*
   * If we need a horizontal scrollbar,
   * place it and manage it.  Save the height of the current
   * viewing area.
   */
  if (hw->html.doc_width > hw->html.view_width)
  {
    hw->html.use_hbar = True;
    if (hw->html.hbar_top == True)
    {
      if (hw->html.use_vbar == True)
      {
	XtMoveWidget (hw->html.vbar,
		      hw->html.vbar->core.x, sheight);
      }

      if (hw->html.vbar_right == True)
      {
	XtMoveWidget (hw->html.hbar, 0, 0);
      }
      else
      {
	XtMoveWidget (hw->html.hbar, swidth, 0);
      }
    }
    else
    {
      if (hw->html.vbar_right == True)
      {
	XtMoveWidget (hw->html.hbar, 0,
		      (hw->core.height - sheight));
      }
      else
      {
	XtMoveWidget (hw->html.hbar, swidth,
		      (hw->core.height - sheight));
      }
    }
    XtManageChild (hw->html.hbar);
    hw->html.view_height = hw->core.height - sheight - (2 * st);
  }
  /*
   * Else we don't need a horizontal scrollbar.
   * Remove it and save the current viewing area height.
   */
  else
  {
    hw->html.use_hbar = False;
    XtUnmanageChild (hw->html.hbar);
    hw->html.scroll_x = 0;
    hw->html.view_height = hw->core.height - (2 * st);
  }

  /*
   * Configure the scrollbar min, max, and slider sizes
   */
#ifdef DEBUG
  fprintf (stderr, "calling in ReformatWindow\n");
#endif
  ConfigScrollBars (hw);
}


/*
 * We're a happy widget.  We let any child move or resize themselves
 * however they want, we don't care.
 */
static XtGeometryResult
GeometryManager (w, request, reply)
Widget w;
XtWidgetGeometry *request;
XtWidgetGeometry *reply;
{
  reply->x = request->x;
  reply->y = request->y;
  reply->width = request->width;
  reply->height = request->height;
  reply->border_width = request->border_width;
  reply->request_mode = request->request_mode;
  return (XtGeometryYes);
}


/*
 * Initialize is called when the widget is first initialized.
 * Check to see that all the starting resources are valid.
 */
static void
Initialize (request, new)
HTMLWidget request;
HTMLWidget new;
{
  /*
   *    Make sure height and width are not zero.
   */
  if (new->core.width == 0)
  {
    new->core.width = new->html.margin_width << 1;
  }
  if (new->core.width == 0)
  {
    new->core.width = 10;
  }
  if (new->core.height == 0)
  {
    new->core.height = new->html.margin_height << 1;
  }
  if (new->core.height == 0)
  {
    new->core.height = 10;
  }

  /*
   *    Make sure the underline numbers are within bounds.
   */
  if (new->html.num_anchor_underlines < 0)
  {
    new->html.num_anchor_underlines = 0;
  }
  if (new->html.num_anchor_underlines > MAX_UNDERLINES)
  {
    new->html.num_anchor_underlines = MAX_UNDERLINES;
  }
  if (new->html.num_visitedAnchor_underlines < 0)
  {
    new->html.num_visitedAnchor_underlines = 0;
  }
  if (new->html.num_visitedAnchor_underlines > MAX_UNDERLINES)
  {
    new->html.num_visitedAnchor_underlines = MAX_UNDERLINES;
  }

  /*
   * Initialize the document's object lists and properties
   */
  new->html.formatted_elements = NULL;
  new->html.my_visited_hrefs = NULL;
  new->html.image_list = NULL;
  new->html.widget_list = NULL;
  new->html.form_list = NULL;
  new->html.line_array = NULL;
  new->html.full_line_count = 0;
  new->html.doc_line_count = 0;

  /*
   * Parse the raw text with the HTML parser.
   * (This code used to precede formatted_elements = NULL above, but calling
   *  HTMLParse and then discarding whatever it produced looked like a memory
   *  leak as well as bad policy.  -WBE 97May04)
   */
  new->html.html_objects = HTMLParse (NULL, request->html.raw_text);
  CallLinkCallbacks (new);
  new->html.html_header_objects = HTMLParse (NULL, request->html.header_text);
  new->html.html_footer_objects = HTMLParse (NULL, request->html.footer_text);

  /*
   * Find the max width of a preformatted
   * line in this document.
   */
  new->html.max_pre_width = DocumentWidth (new, new->html.html_objects);

  /*
   * Create the scrollbars.
   * Find their dimensions and then decide which scrollbars you
   * will need, and what the dimensions of the viewing area are.
   * Start assuming a vertical scrollbar and a horizontal one.
   * Then remove vertical if short enough, and remove horizontal
   * if narrow enough.
   */
  CreateScrollbars (new);
  new->html.scroll_x = 0;
  new->html.scroll_y = 0;
  ReformatWindow (new);

  /*
   * Initialize private widget resources
   */
  new->html.drawGC = NULL;
  new->html.select_start = NULL;
  new->html.select_end = NULL;
  new->html.sel_start_pos = 0;
  new->html.sel_end_pos = 0;
  new->html.new_start = NULL;
  new->html.new_end = NULL;
  new->html.new_start_pos = 0;
  new->html.new_end_pos = 0;
  new->html.active_anchor = NULL;
  new->html.press_x = 0;
  new->html.press_y = 0;

  new->html.cached_tracked_ele = NULL;

  /*
     Initialize cursor used when pointer is inside anchor. 
   */
  if (in_anchor_cursor == (Cursor) NULL)
    in_anchor_cursor = XCreateFontCursor (XtDisplay (new), XC_hand2);

  SetDeferPixButtonState (new->html.delay_images);
  return;
}


#ifdef DEBUG
void
DebugHook (x, y, width, height)
int x, y, width, height;
{
  fprintf (stderr, "Redrawing (%d,%d) %dx%d\n", x, y, width, height);
}
#endif


/*
 * This is called by redisplay.  It is passed a rectangle
 * in the viewing area, and it redisplays that portion of the
 * underlying document area.
 */
static void
ViewRedisplay (hw, x, y, width, height)
HTMLWidget hw;
int x, y;
int width, height;
{
  int sx, sy;
  int doc_x, doc_y;
  int i, start, end, guess;

  /*
   * Use scrollbar values to map from view space to document space.
   */
  sx = sy = 0;
  if (hw->html.use_vbar == True)
  {
    sy += hw->html.scroll_y;
  }
  if (hw->html.use_hbar == True)
  {
    sx += hw->html.scroll_x;
  }

  doc_x = x + sx;
  doc_y = y + sy;

  /*
   * Find the lines that overlap the exposed area.
   */
  start = 0;
  end = hw->html.full_line_count - 1;

  /*
   * Heuristic to speed up redraws by guessing at the starting line.
   */
  guess = doc_y / (hw->html.font->max_bounds.ascent +
		   hw->html.font->max_bounds.descent);
  if (guess > end)
  {
    guess = end;
  }
  while (guess > 0)
  {
    if ((hw->html.line_array[guess] != NULL) &&
	(hw->html.line_array[guess]->y < doc_y))
    {
      break;
    }
    guess--;
  }
  if (guess < start)
  {
    guess = start;
  }

  for (i = guess; i < hw->html.full_line_count; i++)
  {
    if (hw->html.line_array[i] == NULL)
    {
      continue;
    }

    if (hw->html.line_array[i]->y < doc_y)
    {
      start = i;
    }
    if (hw->html.line_array[i]->y > (doc_y + height))
    {
      end = i - 1;
      break;
    }
  }

  /*
   * If we have a GC draw the lines that overlap the exposed area.
   */
  if (hw->html.drawGC != NULL)
  {
    for (i = start; i <= end; i++)
    {
      PlaceLine (hw, i, doc_x, doc_y, doc_x + width, doc_y + height);
    }
#ifdef EXTRA_FLUSH
    XFlush (XtDisplay (hw));
#endif
  }
}


static void
ViewClearAndRefresh (hw)
HTMLWidget hw;
{
#ifdef FAKE_EXPOSE
  /*
   * This is some scary shit.  The first argument in DrawExpose is
   * not used so 0 is OK but I am still getting queasy.  The reason
   * this is being done is because there are problems on some machines
   * with refreshes.  This may be bad because at this point there
   * may not be a window.  This seemed to be the easiest way to solve it
   * but I will try to figure out a better way. john.
   */
  if (hw->html.drawGC == NULL)
  {
    XEvent xe;

    xe.xany.type = Expose;

    DrawExpose (0, (caddr_t) hw, &xe);
  }
#endif

  /*
   * Only refresh if we have a window already
   * (if we have a GC we have a window).
   */
  if (hw->html.drawGC != NULL)
  {
    XClearArea (XtDisplay (hw->html.view), XtWindow (hw->html.view),
		0, 0, 0, 0, False);
    ViewRedisplay (hw, 0, 0,
		   hw->html.view_width, hw->html.view_height);
    /*
     * This is a fake deal to make an Expose event to call Redisplay
     * to redraw the shadow around the view area
     */
    XClearArea (XtDisplay (hw), XtWindow (hw),
		0, 0, 1, 1, True);
  }
}


/*
 * The Redisplay function is what you do with an expose event.
 * Right now we call user callbacks, and then call the CompositeWidget's
 * Redisplay routine.
 */
static void
Redisplay (hw, event, region)
HTMLWidget hw;
XEvent *event;
Region region;
{
  int dx, dy;

  dx = dy = 0;
  if ((hw->html.use_vbar == True) && (hw->html.vbar_right == False))
  {
    dx += VbarWidth (hw);
  }
  if ((hw->html.use_hbar == True) && (hw->html.hbar_top == True))
  {
    dy += HbarHeight (hw);
  }

  return;
}


/*
 * Resize is called when the widget changes size.
 * Mostly any resize causes a reformat, except for the special case
 * where the width doesn't change, and the height doesn't change
 * enought to affect the vertical scrollbar.
 * It is too complex to guess exactly what needs to be redrawn, so refresh the
 * whole window on any resize.
 */
static void
Resize (hw)
HTMLWidget hw;
{
  int tempw;
  Dimension swidth, sheight;
  Dimension st;

  /*
   * Find the new width of the viewing area.
   */
  swidth = VbarWidth (hw);
  sheight = HbarHeight (hw);
  st = 0;
  if (hw->core.width <= swidth)
  {
    hw->core.width = swidth + 10;
  }

  if (hw->html.use_vbar == True)
  {
    tempw = hw->core.width - swidth - (2 * st);
  }
  else
  {
    tempw = hw->core.width - (2 * st);
    /*
       fool positioning of horz scrollbar later 
     */
    swidth = 0;
  }

  /*
   * Special case where we don't have to reformat to a new width.
   * The width has not changed, and the height has not changed
   * significantly to change the state of the vertical scrollbar.
   */
  if (tempw == hw->html.view_width  &&
      (hw->html.use_vbar == True)  ==
          (hw->core.height - sheight - 2 * st < hw->html.doc_height) )
  {
    /*
     * Super special case where the size of the window hasn't
     * changed at ALL!
     */
    if ((hw->html.use_hbar == True  &&
	 hw->html.view_height == hw->core.height - sheight - 2 * st)  ||
	(hw->html.use_hbar == False  &&
	 hw->html.view_height == hw->core.height - 2 * st) )
    {
      return;
    }

    if (hw->html.use_hbar == True)
    {
      if (hw->html.hbar_top == True)
      {
	if (hw->html.vbar_right == True)
	{
	  XtMoveWidget (hw->html.hbar, 0, 0);
	}
	else
	{
	  XtMoveWidget (hw->html.hbar, swidth, 0);
	}
      }
      else
      {
	if (hw->html.vbar_right == True)
	{
	  XtMoveWidget (hw->html.hbar, 0,
			(hw->core.height - sheight));
	}
	else
	{
	  XtMoveWidget (hw->html.hbar, swidth,
			(hw->core.height - sheight));
	}
      }
      hw->html.view_height = hw->core.height - sheight - 2 * st;
    }
    else
    {
      hw->html.view_height = hw->core.height - 2 * st;
    }
#ifdef DEBUG
    fprintf (stderr, "calling in Resize\n");
#endif
    ConfigScrollBars (hw);
    ScrollWidgets (hw);
    ViewClearAndRefresh (hw);
  }
  /*
   * Otherwise we have to do a full reformat on every resize.
   */
  else
  {
    ReformatWindow (hw);	/* if this tries to load an image ... */
    SetTitle (NULL);		/* ... this is needed to restore the title */
    ScrollWidgets (hw);
    ViewClearAndRefresh (hw);
  }

#ifdef DEBUG
  {
    int ss;
    XtVaGetValues (hw->html.vbar, XmNsliderSize, &ss, NULL);
    fprintf (stderr, "leaving; slider size %d\n", ss);
  }
#endif

  return;
}


/*
 * Find the complete text for this the anchor that aptr is a part of
 * and set it into the selection.
 */
static void
FindSelectAnchor (hw, aptr)
HTMLWidget hw;
struct ele_rec *aptr;
{
  struct ele_rec *eptr;

  eptr = aptr;
  while ((eptr->prev != NULL) &&
	 (eptr->prev->anchorHRef != NULL) &&
	 (strcmp (eptr->prev->anchorHRef, eptr->anchorHRef) == 0))
  {
    eptr = eptr->prev;
  }
  hw->html.select_start = eptr;
  hw->html.sel_start_pos = 0;

  eptr = aptr;
  while ((eptr->next != NULL) &&
	 (eptr->next->anchorHRef != NULL) &&
	 (strcmp (eptr->next->anchorHRef, eptr->anchorHRef) == 0))
  {
    eptr = eptr->next;
  }
  hw->html.select_end = eptr;
  hw->html.sel_end_pos = eptr->edata_len - 2;
}


/*
 * Set as active all elements in the widget that are part of the anchor
 * in the widget's start ptr.
 */
static void
SetAnchor (hw)
HTMLWidget hw;
{
  struct ele_rec *eptr;
  struct ele_rec *start;
  struct ele_rec *end;
  unsigned long fg, bg;
  unsigned long old_fg, old_bg;

  eptr = hw->html.active_anchor;
  if ((eptr == NULL) || (eptr->anchorHRef == NULL))
  {
    return;
  }
  fg = hw->html.activeAnchor_fg;
  bg = hw->html.activeAnchor_bg;

  FindSelectAnchor (hw, eptr);

  start = hw->html.select_start;
  end = hw->html.select_end;

  eptr = start;
  while ((eptr != NULL) && (eptr != end))
  {
    if (eptr->type == E_TEXT)
    {
      old_fg = eptr->fg;
      old_bg = eptr->bg;
      eptr->fg = fg;
      eptr->bg = bg;
      TextRefresh (hw, eptr,
		   0, (eptr->edata_len - 2), 1);
      eptr->fg = old_fg;
      eptr->bg = old_bg;
    }
    else if (eptr->type == E_IMAGE)
    {
      old_fg = eptr->fg;
      old_bg = eptr->bg;
      eptr->fg = fg;
      eptr->bg = bg;
      ImageRefresh (hw, eptr);
      eptr->fg = old_fg;
      eptr->bg = old_bg;
    }
    /*
     * Linefeeds in anchor spanning multiple lines should NOT
     * be highlighted!
     else if (eptr->type == E_LINEFEED)
     {
     old_fg = eptr->fg;
     old_bg = eptr->bg;
     eptr->fg = fg;
     eptr->bg = bg;
     LinefeedRefresh(hw, eptr);
     eptr->fg = old_fg;
     eptr->bg = old_bg;
     }
     */
    eptr = eptr->next;
  }
  if (eptr != NULL)
  {
    if (eptr->type == E_TEXT)
    {
      old_fg = eptr->fg;
      old_bg = eptr->bg;
      eptr->fg = fg;
      eptr->bg = bg;
      TextRefresh (hw, eptr,
		   0, (eptr->edata_len - 2), 1);
      eptr->fg = old_fg;
      eptr->bg = old_bg;
    }
    else if (eptr->type == E_IMAGE)
    {
      old_fg = eptr->fg;
      old_bg = eptr->bg;
      eptr->fg = fg;
      eptr->bg = bg;
      ImageRefresh (hw, eptr);
      eptr->fg = old_fg;
      eptr->bg = old_bg;
    }
    /*
     * Linefeeds in anchor spanning multiple lines should NOT
     * be highlighted!
     else if (eptr->type == E_LINEFEED)
     {
     old_fg = eptr->fg;
     old_bg = eptr->bg;
     eptr->fg = fg;
     eptr->bg = bg;
     LinefeedRefresh(hw, eptr);
     eptr->fg = old_fg;
     eptr->bg = old_bg;
     }
     */
  }
}


/*
 * Draw selection for all elements in the widget
 * from start to end.
 */
static void
DrawSelection (hw, start, end, start_pos, end_pos)
HTMLWidget hw;
struct ele_rec *start;
struct ele_rec *end;
int start_pos, end_pos;
{
  struct ele_rec *eptr;
  int epos;

  if ((start == NULL) || (end == NULL))
  {
    return;
  }

  /*
   * Keep positions within bounds (allows us to be sloppy elsewhere)
   */
  if (start_pos < 0)
  {
    start_pos = 0;
  }
  if (start_pos >= start->edata_len - 1)
  {
    start_pos = start->edata_len - 2;
  }
  if (end_pos < 0)
  {
    end_pos = 0;
  }
  if (end_pos >= end->edata_len - 1)
  {
    end_pos = end->edata_len - 2;
  }

  if (SwapElements (start, end, start_pos, end_pos))
  {
    eptr = start;
    start = end;
    end = eptr;
    epos = start_pos;
    start_pos = end_pos;
    end_pos = epos;
  }

  eptr = start;
  while ((eptr != NULL) && (eptr != end))
  {
    int p1, p2;

    if (eptr == start)
    {
      p1 = start_pos;
    }
    else
    {
      p1 = 0;
    }
    p2 = eptr->edata_len - 2;

    if (eptr->type == E_TEXT)
    {
      eptr->selected = True;
      eptr->start_pos = p1;
      eptr->end_pos = p2;
      TextRefresh (hw, eptr, p1, p2, 1);
    }
    else if (eptr->type == E_LINEFEED)
    {
      eptr->selected = True;
      LinefeedRefresh (hw, eptr);
    }
    eptr = eptr->next;
  }
  if (eptr != NULL)
  {
    int p1, p2;

    if (eptr == start)
    {
      p1 = start_pos;
    }
    else
    {
      p1 = 0;
    }

    if (eptr == end)
    {
      p2 = end_pos;
    }
    else
    {
      p2 = eptr->edata_len - 2;
    }

    if (eptr->type == E_TEXT)
    {
      eptr->selected = True;
      eptr->start_pos = p1;
      eptr->end_pos = p2;
      TextRefresh (hw, eptr, p1, p2, 1);
    }
    else if (eptr->type == E_LINEFEED)
    {
      eptr->selected = True;
      LinefeedRefresh (hw, eptr);
    }
  }
}


/*
 * Set selection for all elements in the widget's
 * start to end list.
 */
static void
SetSelection (hw)
HTMLWidget hw;
{
  struct ele_rec *start;
  struct ele_rec *end;
  int start_pos, end_pos;

  start = hw->html.select_start;
  end = hw->html.select_end;
  start_pos = hw->html.sel_start_pos;
  end_pos = hw->html.sel_end_pos;
  DrawSelection (hw, start, end, start_pos, end_pos);
}


/*
 * Erase the selection from start to end
 */
static void
EraseSelection (hw, start, end, start_pos, end_pos)
HTMLWidget hw;
struct ele_rec *start;
struct ele_rec *end;
int start_pos, end_pos;
{
  struct ele_rec *eptr;
  int epos;

  if ((start == NULL) || (end == NULL))
  {
    return;
  }

  /*
   * Keep positions within bounds (allows us to be sloppy elsewhere)
   */
  if (start_pos < 0)
  {
    start_pos = 0;
  }
  if (start_pos >= start->edata_len - 1)
  {
    start_pos = start->edata_len - 2;
  }
  if (end_pos < 0)
  {
    end_pos = 0;
  }
  if (end_pos >= end->edata_len - 1)
  {
    end_pos = end->edata_len - 2;
  }

  if (SwapElements (start, end, start_pos, end_pos))
  {
    eptr = start;
    start = end;
    end = eptr;
    epos = start_pos;
    start_pos = end_pos;
    end_pos = epos;
  }

  eptr = start;
  while ((eptr != NULL) && (eptr != end))
  {
    int p1, p2;

    if (eptr == start)
    {
      p1 = start_pos;
    }
    else
    {
      p1 = 0;
    }
    p2 = eptr->edata_len - 2;

    if (eptr->type == E_TEXT)
    {
      eptr->selected = False;
      TextRefresh (hw, eptr, p1, p2, 1);
    }
    else if (eptr->type == E_LINEFEED)
    {
      eptr->selected = False;
      LinefeedRefresh (hw, eptr);
    }
    eptr = eptr->next;
  }
  if (eptr != NULL)
  {
    int p1, p2;

    if (eptr == start)
    {
      p1 = start_pos;
    }
    else
    {
      p1 = 0;
    }

    if (eptr == end)
    {
      p2 = end_pos;
    }
    else
    {
      p2 = eptr->edata_len - 2;
    }

    if (eptr->type == E_TEXT)
    {
      eptr->selected = False;
      TextRefresh (hw, eptr, p1, p2, 1);
    }
    else if (eptr->type == E_LINEFEED)
    {
      eptr->selected = False;
      LinefeedRefresh (hw, eptr);
    }
  }
}


/*
 * Clear the current selection (if there is one)
 */
static void
ClearSelection (hw)
HTMLWidget hw;
{
  struct ele_rec *start;
  struct ele_rec *end;
  int start_pos, end_pos;

  start = hw->html.select_start;
  end = hw->html.select_end;
  start_pos = hw->html.sel_start_pos;
  end_pos = hw->html.sel_end_pos;
  EraseSelection (hw, start, end, start_pos, end_pos);

  if ((start == NULL) || (end == NULL))
  {
    hw->html.select_start = NULL;
    hw->html.select_end = NULL;
    hw->html.sel_start_pos = 0;
    hw->html.sel_end_pos = 0;
    hw->html.active_anchor = NULL;
    return;
  }

  hw->html.select_start = NULL;
  hw->html.select_end = NULL;
  hw->html.sel_start_pos = 0;
  hw->html.sel_end_pos = 0;
  hw->html.active_anchor = NULL;
}


/*
 * Clear from active all elements in the widget that are part of the anchor.
 * (These have already been previously set into the start and end of the
 * selection.)
 */
static void
UnsetAnchor (hw)
HTMLWidget hw;
{
  struct ele_rec *eptr;

  /*
   * Clear any activated images
   */
  eptr = hw->html.select_start;
  while ((eptr != NULL) && (eptr != hw->html.select_end))
  {
    if (eptr->type == E_IMAGE)
    {
      ImageRefresh (hw, eptr);
    }
    eptr = eptr->next;
  }
  if ((eptr != NULL) && (eptr->type == E_IMAGE))
  {
    ImageRefresh (hw, eptr);
  }

  /*
   * Clear the activated anchor
   */
  ClearSelection (hw);
}


/*
 * Erase the old selection, and draw the new one in such a way
 * that advantage is taken of overlap, and there is no obnoxious
 * flashing.
 */
static void
ChangeSelection (hw, start, end, start_pos, end_pos)
HTMLWidget hw;
struct ele_rec *start;
struct ele_rec *end;
int start_pos, end_pos;
{
  struct ele_rec *old_start;
  struct ele_rec *old_end;
  struct ele_rec *new_start;
  struct ele_rec *new_end;
  struct ele_rec *eptr;
  int epos;
  int new_start_pos, new_end_pos;
  int old_start_pos, old_end_pos;

  old_start = hw->html.new_start;
  old_end = hw->html.new_end;
  old_start_pos = hw->html.new_start_pos;
  old_end_pos = hw->html.new_end_pos;
  new_start = start;
  new_end = end;
  new_start_pos = start_pos;
  new_end_pos = end_pos;

  if ((new_start == NULL) || (new_end == NULL))
  {
    return;
  }

  if ((old_start == NULL) || (old_end == NULL))
  {
    DrawSelection (hw, new_start, new_end,
		   new_start_pos, new_end_pos);
    return;
  }

  if (SwapElements (old_start, old_end, old_start_pos, old_end_pos))
  {
    eptr = old_start;
    old_start = old_end;
    old_end = eptr;
    epos = old_start_pos;
    old_start_pos = old_end_pos;
    old_end_pos = epos;
  }

  if (SwapElements (new_start, new_end, new_start_pos, new_end_pos))
  {
    eptr = new_start;
    new_start = new_end;
    new_end = eptr;
    epos = new_start_pos;
    new_start_pos = new_end_pos;
    new_end_pos = epos;
  }

  /*
   * Deal with all possible intersections of the 2 selection sets.
   *
   ********************************************************
   *                      *                               *
   *      |--             *            |--                *
   * old--|               *       new--|                  *
   *      |--             *            |--                *
   *                      *                               *
   *      |--             *            |--                *
   * new--|               *       old--|                  *
   *      |--             *            |--                *
   *                      *                               *
   ********************************************************
   *                      *                               *
   *      |----           *              |--              *
   * old--|               *         new--|                *
   *      | |--           *              |                *
   *      |-+--           *            |-+--              *
   *        |             *            | |--              *
   *   new--|             *       old--|                  *
   *        |--           *            |----              *
   *                      *                               *
   ********************************************************
   *                      *                               *
   *      |---------      *            |---------         *
   *      |               *            |                  *
   *      |      |--      *            |      |--         *
   * new--| old--|        *       old--| new--|           *
   *      |      |--      *            |      |--         *
   *      |               *            |                  *
   *      |---------      *            |---------         *
   *                      *                               *
   ********************************************************
   *
   */
  if ((ElementLessThan (old_end, new_start, old_end_pos, new_start_pos)) ||
      (ElementLessThan (new_end, old_start, new_end_pos, old_start_pos)))
  {
    EraseSelection (hw, old_start, old_end,
		    old_start_pos, old_end_pos);
    DrawSelection (hw, new_start, new_end,
		   new_start_pos, new_end_pos);
  }
  else if ((ElementLessThan (old_start, new_start,
			     old_start_pos, new_start_pos)) &&
	   (ElementLessThan (old_end, new_end, old_end_pos, new_end_pos)))
  {
    if (new_start_pos != 0)
    {
      EraseSelection (hw, old_start, new_start,
		      old_start_pos, new_start_pos - 1);
    }
    else
    {
      EraseSelection (hw, old_start, new_start->prev,
		      old_start_pos, new_start->prev->edata_len - 2);
    }
    if (old_end_pos < (old_end->edata_len - 2))
    {
      DrawSelection (hw, old_end, new_end,
		     old_end_pos + 1, new_end_pos);
    }
    else
    {
      DrawSelection (hw, old_end->next, new_end,
		     0, new_end_pos);
    }
  }
  else if ((ElementLessThan (new_start, old_start,
			     new_start_pos, old_start_pos)) &&
	   (ElementLessThan (new_end, old_end, new_end_pos, old_end_pos)))
  {
    if (old_start_pos != 0)
    {
      DrawSelection (hw, new_start, old_start,
		     new_start_pos, old_start_pos - 1);
    }
    else
    {
      DrawSelection (hw, new_start, old_start->prev,
		     new_start_pos, old_start->prev->edata_len - 2);
    }
    if (new_end_pos < (new_end->edata_len - 2))
    {
      EraseSelection (hw, new_end, old_end,
		      new_end_pos + 1, old_end_pos);
    }
    else
    {
      EraseSelection (hw, new_end->next, old_end,
		      0, old_end_pos);
    }
  }
  else if ((ElementLessThan (new_start, old_start,
			     new_start_pos, old_start_pos)) ||
	   (ElementLessThan (old_end, new_end, old_end_pos, new_end_pos)))
  {
    if ((new_start != old_start) || (new_start_pos != old_start_pos))
    {
      if (old_start_pos != 0)
      {
	DrawSelection (hw, new_start, old_start,
		       new_start_pos, old_start_pos - 1);
      }
      else
      {
	DrawSelection (hw, new_start, old_start->prev,
		       new_start_pos,
		       old_start->prev->edata_len - 2);
      }
    }
    if ((old_end != new_end) || (old_end_pos != new_end_pos))
    {
      if (old_end_pos < (old_end->edata_len - 2))
      {
	DrawSelection (hw, old_end, new_end,
		       old_end_pos + 1, new_end_pos);
      }
      else
      {
	DrawSelection (hw, old_end->next, new_end,
		       0, new_end_pos);
      }
    }
  }
  else
  {
    if ((old_start != new_start) || (old_start_pos != new_start_pos))
    {
      if (new_start_pos != 0)
      {
	EraseSelection (hw, old_start, new_start,
			old_start_pos, new_start_pos - 1);
      }
      else
      {
	EraseSelection (hw, old_start, new_start->prev,
			old_start_pos,
			new_start->prev->edata_len - 2);
      }
    }
    if ((new_end != old_end) || (new_end_pos != old_end_pos))
    {
      if (new_end_pos < (new_end->edata_len - 2))
      {
	EraseSelection (hw, new_end, old_end,
			new_end_pos + 1, old_end_pos);
      }
      else
      {
	EraseSelection (hw, new_end->next, old_end,
			0, old_end_pos);
      }
    }
  }
}


static void
SelectStart (w, event, params, num_params)
Widget w;
XEvent *event;
String *params;			/*
				   unused 
				 */
Cardinal *num_params;		/*
				   unused 
				 */
{
  HTMLWidget hw = (HTMLWidget) XtParent (w);
  XButtonPressedEvent *BuEvent = (XButtonPressedEvent *) event;
  struct ele_rec *eptr;
  int epos;

  if (XtClass (XtParent (w)) != htmlWidgetClass)
  {
    return;
  }

#ifdef NOT_RIGHT
  XUndefineCursor (XtDisplay (hw->html.view), XtWindow (hw->html.view));
#endif
  XUndefineCursor (XtDisplay (hw), XtWindow (hw));

  /*
   * Because X sucks, we can get the button pressed in the window, but
   * released out of the window.  This will highlight some text, but
   * never complete the selection.  Now on the next button press we
   * have to clean up this mess.
   */
  EraseSelection (hw, hw->html.new_start, hw->html.new_end,
		  hw->html.new_start_pos, hw->html.new_end_pos);

  /*
   * We want to erase the currently selected text, but still save the
   * selection internally in case we don't create a new one.
   */
  EraseSelection (hw, hw->html.select_start, hw->html.select_end,
		  hw->html.sel_start_pos, hw->html.sel_end_pos);
  hw->html.new_start = hw->html.select_start;
  hw->html.new_end = NULL;
  hw->html.new_start_pos = hw->html.sel_start_pos;
  hw->html.new_end_pos = 0;

  eptr = LocateElement (hw, BuEvent->x, BuEvent->y, &epos, hw->html.new_start);

  hw->html.press_x = BuEvent->x;
  hw->html.press_y = BuEvent->y;
  hw->html.but_press_time = BuEvent->time;

  if (eptr != NULL)
  {
    /*
     * If this is an anchor assume for now we are activating it
     * and not selecting it.
     */
    if (eptr->anchorHRef != NULL  &&  eptr->type != E_LINEFEED)
    {
      hw->html.active_anchor = eptr;
      SetAnchor (hw);
      return;
    }
    /*
     * Else if we are on an image we can't select text so
     * pretend we got eptr==NULL, and exit here.
     */
    else if (eptr->type == E_IMAGE)
    {
      /* fall into deselect code below */
    }
    /*
     * Else if we used button2, we can't select text, so exit here.
     */
    else if (BuEvent->button == Button2)
    {
      return;
    }
    /*
     * Else a single click will not select a new object
     * but it will prime that selection on the next mouse
     * move.
     * Ignore special internal text
     */
    else if (eptr->internal == False)
    {
      hw->html.new_start = eptr;
      hw->html.new_start_pos = epos;
      hw->html.new_end = NULL;
      hw->html.new_end_pos = 0;
      return;
    }
  }

  /*
   * deselect
   */
  hw->html.new_start = NULL;
  hw->html.new_end = NULL;
  hw->html.new_start_pos = 0;
  hw->html.new_end_pos = 0;
}


static void
ExtendStart (w, event, params, num_params)
Widget w;
XEvent *event;
String *params;			/*
				   unused 
				 */
Cardinal *num_params;		/*
				   unused 
				 */
{
  HTMLWidget hw = (HTMLWidget) XtParent (w);
  XButtonPressedEvent *BuEvent = (XButtonPressedEvent *) event;
  struct ele_rec *eptr;
  struct ele_rec *start, *end;
  struct ele_rec *old_start, *old_end;
  int old_start_pos, old_end_pos;
  int start_pos, end_pos;
  int epos;

  if (XtClass (XtParent (w)) != htmlWidgetClass)
  {
    return;
  }

  eptr = LocateElement (hw, BuEvent->x, BuEvent->y, &epos, hw->html.new_start);

  /*
   * Ignore IMAGE elements.
   */
  if ((eptr != NULL) && (eptr->type == E_IMAGE))
  {
    eptr = NULL;
  }

  /*
   * Ignore NULL elements.
   * Ignore special internal text
   * documents.
   */
  if ((eptr != NULL) && (eptr->internal == False))
  {
    old_start = hw->html.new_start;
    old_start_pos = hw->html.new_start_pos;
    old_end = hw->html.new_end;
    old_end_pos = hw->html.new_end_pos;
    if (hw->html.new_start == NULL)
    {
      hw->html.new_start = hw->html.select_start;
      hw->html.new_start_pos = hw->html.sel_start_pos;
      hw->html.new_end = hw->html.select_end;
      hw->html.new_end_pos = hw->html.sel_end_pos;
    }
    else
    {
      hw->html.new_end = eptr;
      hw->html.new_end_pos = epos;
    }

    if (SwapElements (hw->html.new_start, hw->html.new_end,
		      hw->html.new_start_pos, hw->html.new_end_pos))
    {
      if (SwapElements (eptr, hw->html.new_end,
			epos, hw->html.new_end_pos))
      {
	start = hw->html.new_end;
	start_pos = hw->html.new_end_pos;
	end = eptr;
	end_pos = epos;
      }
      else
      {
	start = hw->html.new_start;
	start_pos = hw->html.new_start_pos;
	end = eptr;
	end_pos = epos;
      }
    }
    else
    {
      if (SwapElements (eptr, hw->html.new_start,
			epos, hw->html.new_start_pos))
      {
	start = hw->html.new_start;
	start_pos = hw->html.new_start_pos;
	end = eptr;
	end_pos = epos;
      }
      else
      {
	start = hw->html.new_end;
	start_pos = hw->html.new_end_pos;
	end = eptr;
	end_pos = epos;
      }
    }

    if (start == NULL)
    {
      start = eptr;
      start_pos = epos;
    }

    if (old_start == NULL)
    {
      hw->html.new_start = hw->html.select_start;
      hw->html.new_end = hw->html.select_end;
      hw->html.new_start_pos = hw->html.sel_start_pos;
      hw->html.new_end_pos = hw->html.sel_end_pos;
    }
    else
    {
      hw->html.new_start = old_start;
      hw->html.new_end = old_end;
      hw->html.new_start_pos = old_start_pos;
      hw->html.new_end_pos = old_end_pos;
    }
    ChangeSelection (hw, start, end, start_pos, end_pos);
    hw->html.new_start = start;
    hw->html.new_end = end;
    hw->html.new_start_pos = start_pos;
    hw->html.new_end_pos = end_pos;
  }
  else
  {
    if (hw->html.new_start == NULL)
    {
      hw->html.new_start = hw->html.select_start;
      hw->html.new_start_pos = hw->html.sel_start_pos;
      hw->html.new_end = hw->html.select_end;
      hw->html.new_end_pos = hw->html.sel_end_pos;
    }
  }
  hw->html.press_x = BuEvent->x;
  hw->html.press_y = BuEvent->y;
}


static void
ExtendAdjust (w, event, params, num_params)
Widget w;
XEvent *event;
String *params;			/*
				   unused 
				 */
Cardinal *num_params;		/*
				   unused 
				 */
{
  HTMLWidget hw = (HTMLWidget) XtParent (w);
  XPointerMovedEvent *MoEvent = (XPointerMovedEvent *) event;
  struct ele_rec *eptr;
  struct ele_rec *start, *end;
  int start_pos, end_pos;
  int epos;

  if (XtClass (XtParent (w)) != htmlWidgetClass)
  {
    return;
  }

  /*
   * Very small mouse motion immediately after button press
   * is ignored.
   */
  if ((ABS ((hw->html.press_x - MoEvent->x)) <= SELECT_THRESHOLD) &&
      (ABS ((hw->html.press_y - MoEvent->y)) <= SELECT_THRESHOLD))
  {
    return;
  }

  /*
   * If we have an active anchor and we got here, we have moved the
   * mouse too far.  Deactivate anchor, and prime a selection.
   * If the anchor is internal text, don't
   * prime a selection.
   */
  if (hw->html.active_anchor != NULL)
  {
    eptr = hw->html.active_anchor;
    UnsetAnchor (hw);
    if (eptr->internal == False)
    {
      hw->html.new_start = NULL;
      hw->html.new_start_pos = 0;
      hw->html.new_end = NULL;
      hw->html.new_end_pos = 0;
    }
  }

  /*
   * If we used button2, we can't select text, so
   * clear selection and exit here.
   */
  if ((MoEvent->state & Button1Mask) == 0)
  {
    hw->html.select_start = NULL;
    hw->html.select_end = NULL;
    hw->html.sel_start_pos = 0;
    hw->html.sel_end_pos = 0;
    hw->html.new_start = NULL;
    hw->html.new_end = NULL;
    hw->html.new_start_pos = 0;
    hw->html.new_end_pos = 0;
    return;
  }

  eptr = LocateElement (hw, MoEvent->x, MoEvent->y, &epos, hw->html.new_start);

  /*
   * If we are on an image pretend we are nowhere
   * and just return.
   */
  if ((eptr != NULL) && (eptr->type == E_IMAGE))
  {
    return;
  }

  /*
   * Ignore NULL items.
   * Ignore if the same as last selected item and position.
   * Ignore special internal text
   */
  if ((eptr != NULL) &&
      ((eptr != hw->html.new_end) || (epos != hw->html.new_end_pos)) &&
      (eptr->internal == False))
  {
    start = hw->html.new_start;
    start_pos = hw->html.new_start_pos;
    end = eptr;
    end_pos = epos;
    if (start == NULL)
    {
      start = eptr;
      start_pos = epos;
    }

    ChangeSelection (hw, start, end, start_pos, end_pos);
    hw->html.new_start = start;
    hw->html.new_end = end;
    hw->html.new_start_pos = start_pos;
    hw->html.new_end_pos = end_pos;
  }
}


static void
ExtendEnd (w, event, params, num_params)
Widget w;
XEvent *event;
String *params;
Cardinal *num_params;
{
  HTMLWidget hw = (HTMLWidget) XtParent (w);
  XButtonReleasedEvent *BuEvent = (XButtonReleasedEvent *) event;
  struct ele_rec *eptr;
  struct ele_rec *start, *end;
  Atom *atoms;
  int i, buffer;
  int start_pos, end_pos;
  int epos;
  char *text;

  if (XtClass (XtParent (w)) != htmlWidgetClass)
  {
    return;
  }

  eptr = LocateElement (hw, BuEvent->x, BuEvent->y, &epos,
			hw->html.select_start);

  /*
   * If we just released button one or two, and we are on an object,
   * and we have an active anchor, and we are on the active anchor,
   * and if we haven't waited too long...  activate that anchor.
   */
  if (((BuEvent->button == Button1) || (BuEvent->button == Button2)) &&
      (eptr != NULL) &&
      (hw->html.active_anchor != NULL) &&
      (eptr == hw->html.active_anchor) &&
      ((BuEvent->time - hw->html.but_press_time) < CLICK_TIME))
  {
    _HTMLInput (w, event, params, num_params);
    return;
  }
  else if (hw->html.active_anchor != NULL)
  {
    start = hw->html.active_anchor;
    UnsetAnchor (hw);
    if (start->internal == False)
    {
      hw->html.new_start = eptr;
      hw->html.new_start_pos = epos;
      hw->html.new_end = NULL;
      hw->html.new_end_pos = 0;
    }
  }

  /*
   * If we used button2, we can't select text, so clear
   * selection and exit here.
   */
  if (BuEvent->button == Button2)
  {
    hw->html.new_start = hw->html.select_start;
    hw->html.new_end = NULL;
    hw->html.new_start_pos = hw->html.sel_start_pos;
    hw->html.new_end_pos = 0;
    return;
  }

  /*
   * If we are on an image, pretend we are nowhere
   * and NULL out the eptr
   */
  if ((eptr != NULL) && (eptr->type == E_IMAGE))
  {
    eptr = NULL;
  }

  /*
   * If button released on a NULL item, take the last non-NULL
   * item that we highlighted.
   */
  if ((eptr == NULL) && (hw->html.new_end != NULL))
  {
    eptr = hw->html.new_end;
    epos = hw->html.new_end_pos;
  }

  if ((eptr != NULL) && (eptr->internal == False) &&
      (hw->html.new_end != NULL))
  {
    start = hw->html.new_start;
    start_pos = hw->html.new_start_pos;
    end = eptr;
    end_pos = epos;
    if (start == NULL)
    {
      start = eptr;
      start_pos = epos;
    }
    ChangeSelection (hw, start, end, start_pos, end_pos);
    hw->html.select_start = start;
    hw->html.sel_start_pos = start_pos;
    hw->html.select_end = end;
    hw->html.sel_end_pos = end_pos;
    SetSelection (hw);
    hw->html.new_start = NULL;
    hw->html.new_end = NULL;
    hw->html.new_start_pos = 0;
    hw->html.new_end_pos = 0;

    atoms = (Atom *) XtMalloc (*num_params * sizeof (Atom));
    if (atoms == NULL)
    {
      fprintf (stderr, "cannot allocate atom list\n");
      return;
    }
    XmuInternStrings (XtDisplay ((Widget) hw), params, *num_params, atoms);
    hw->html.selection_time = BuEvent->time;
    for (i = 0; i < *num_params; i++)
    {
      switch (atoms[i])
      {
      case XA_CUT_BUFFER0:
	buffer = 0;
	break;
      case XA_CUT_BUFFER1:
	buffer = 1;
	break;
      case XA_CUT_BUFFER2:
	buffer = 2;
	break;
      case XA_CUT_BUFFER3:
	buffer = 3;
	break;
      case XA_CUT_BUFFER4:
	buffer = 4;
	break;
      case XA_CUT_BUFFER5:
	buffer = 5;
	break;
      case XA_CUT_BUFFER6:
	buffer = 6;
	break;
      case XA_CUT_BUFFER7:
	buffer = 7;
	break;
      default:
	buffer = -1;
	break;
      }
      if (buffer >= 0)
      {
	text = ParseTextToString ((hw->html.fancy_selections == True)? hw:NULL,
				  hw->html.select_start,
				  hw->html.select_end,
				  hw->html.sel_start_pos,
				  hw->html.sel_end_pos,
				  hw->html.font->max_bounds.width);
	XStoreBuffer (XtDisplay ((Widget) hw),
		      text, strlen (text), buffer);
	if (text != NULL)
	{
	  XtFree(text);
	}
      }
      else
      {
	XtOwnSelection ((Widget) hw, atoms[i],
			BuEvent->time,
			(XtConvertSelectionProc) ConvertSelection,
			(XtLoseSelectionProc) LoseSelection,
			(XtSelectionDoneProc) SelectionDone);
      }
    }
    XtFree ((char *) atoms);
  }
  else if (eptr == NULL)
  {
    hw->html.select_start = NULL;
    hw->html.sel_start_pos = 0;
    hw->html.select_end = NULL;
    hw->html.sel_end_pos = 0;
    hw->html.new_start = NULL;
    hw->html.new_start_pos = 0;
    hw->html.new_end = NULL;
    hw->html.new_end_pos = 0;
  }
}


#define LEAVING_ANCHOR(hw) \
  hw->html.cached_tracked_ele = NULL; \
  (*(pointerTrackProc) \
    (hw->html.pointer_motion_callback))(hw, ""); \
  XUndefineCursor (XtDisplay (hw), XtWindow (hw));

/*
 * KNOWN PROBLEM: We never get LeaveNotify or FocusOut events,
 * despite the fact we've requested them.  Bummer. 
 */
static void
TrackMotion (w, event, params, num_params)
Widget w;
XEvent *event;
String *params;			/*
				   unused 
				 */
Cardinal *num_params;		/*
				   unused 
				 */
{
  HTMLWidget hw = (HTMLWidget) XtParent (w);
  struct ele_rec *eptr;
  int epos, x, y;

  if (XtClass (XtParent (w)) != htmlWidgetClass)
  {
    return;
  }

  if (!hw->html.pointer_motion_callback)
    return;

  if (event->type == MotionNotify)
  {
    x = ((XMotionEvent *) event)->x;
    y = ((XMotionEvent *) event)->y;
  }
  else if (event->type == LeaveNotify || event->type == FocusOut ||
	   event->type == Expose)
  {
    /*
     * Wipe out. 
     */
    if (hw->html.cached_tracked_ele)
    {
      LEAVING_ANCHOR (hw);
    }
    return;
  }
  else
  {
    return;
  }

  eptr = LocateElement (hw, x, y, &epos, NULL);

  if (eptr != NULL  &&  eptr->type == E_LINEFEED)   eptr = NULL;

  /*
   * We're hitting a new anchor if eptr exists and
   * eptr != cached tracked element and anchorHRef != NULL. 
   */
  if (eptr != NULL && eptr != hw->html.cached_tracked_ele &&
      eptr->anchorHRef != NULL)
  {
    hw->html.cached_tracked_ele = eptr;
    (*(pointerTrackProc)
     (hw->html.pointer_motion_callback)) (hw, eptr->anchorHRef);
    XDefineCursor (XtDisplay (hw), XtWindow (hw), in_anchor_cursor);
  }
  /*
   * We're leaving an anchor if the element stops being found or if
   * a cached element exists and we're not entering a new anchor. 
   */
  else if (eptr == NULL  ||
	   hw->html.cached_tracked_ele != NULL  &&  eptr->anchorHRef == NULL)
  {
    LEAVING_ANCHOR (hw);
  }

  return;
}

static void
ScrollUp (w, event, params, num_params)
Widget w;
XEvent *event;
String *params;			/*
				   unused 
				 */
Cardinal *num_params;		/*
				   unused 
				 */
{
  HTMLWidget hw = (HTMLWidget) XtParent (w);

  if (XtClass (XtParent (w)) == htmlWidgetClass)
  {
    /*
     * Added by Satoshi ASAMI <asami@cory.EECS.Berkeley.EDU>
     */
    if (*num_params == 1)
    {
      if (strcmp (params[0], "Oneline") == 0)
	ScrollMove (hw->html.vbar, (caddr_t) hw, DEFAULT_INCREMENT);
      else
	ScrollMove (hw->html.vbar, (caddr_t) hw, 1);
    }
  }
}

static void
ScrollDown (w, event, params, num_params)
Widget w;
XEvent *event;
String *params;			/*
				   unused 
				 */
Cardinal *num_params;		/*
				   unused 
				 */
{
  HTMLWidget hw = (HTMLWidget) XtParent (w);

  if (XtClass (XtParent (w)) == htmlWidgetClass)
  {
    /*
     * Added by Satoshi ASAMI <asami@cory.EECS.Berkeley.EDU>
     */
    if (*num_params == 1)
    {
      if (strcmp (params[0], "Oneline") == 0)
	ScrollMove (hw->html.vbar, (caddr_t) hw, -DEFAULT_INCREMENT);
      else
	ScrollMove (hw->html.vbar, (caddr_t) hw, -1);
    }
  }
}


/*
 * Process mouse input to the HTML widget.
 * Currently only processes an anchor-activate when Button1 is pressed
 */
static void
_HTMLInput (w, event, params, num_params)
Widget w;
XEvent *event;
String *params;			/* unused */
Cardinal *num_params;		/* unused */
{
  HTMLWidget hw = (HTMLWidget) XtParent (w);
  struct ele_rec *eptr;
  WbAnchorCallbackData cbdata;
  int epos;

  if (XtClass (XtParent (w)) != htmlWidgetClass)
  {
    return;
  }

  if (event->type == ButtonRelease)
  {
    eptr = LocateElement (hw, event->xbutton.x, event->xbutton.y,
			  &epos, NULL);

    if (eptr != NULL  &&  eptr->type == E_LINEFEED)   eptr = NULL;

    if (eptr != NULL)
    {
      if (eptr->anchorHRef != NULL)
      {
	char *tptr, *ptr;

	/*
	 * Save the anchor text, replace newlines with spaces.
	 */
	tptr = ParseTextToString (NULL,
				  hw->html.select_start, hw->html.select_end,
				 hw->html.sel_start_pos, hw->html.sel_end_pos,
				  hw->html.font->max_bounds.width);
	ptr = tptr;
	while ((ptr != NULL) && (*ptr != '\0'))
	{
	  if (*ptr == '\n')
	  {
	    *ptr = ' ';
	  }
	  ptr++;
	}

	/*
	 * Clear the activated anchor
	 */
	UnsetAnchor (hw);
#ifdef EXTRA_FLUSH
	XFlush (XtDisplay (hw));
#endif

	/*
	 * If user clicked on a not-really-anchored delayed image,
	 * or on a delayed ISMAP image (which usurps eptr->anchorHRef for
	 *     an IsMapForm identifier string when inside a FORM),
	 * or on a delayed image that has a real anchor,
	 * then try to load the image now and redraw things with the result.
	 */
	if (IsDelayedHRef (hw, eptr->anchorHRef)  /* delayed image only */
	    ||
	    (eptr->img_data != NULL   &&
	     eptr->img_data->pic_data != NULL  &&  /* unnecessarily cautious */
	     eptr->img_data->pic_data->delayed  &&
	     (eptr->img_data->ismap /* delayed ISMAP img, possibly IsMapForm */
	      ||
	      AnchoredHeight (hw) < (event->xbutton.y + hw->html.scroll_y
				       - (eptr->y + eptr->y_offset))
	      ) ) )
	{
	  HTMLGetImage (hw, eptr, 1);  /* load image, delay disabled */

	  ReformatWindow (hw);
	  ScrollWidgets (hw);
	  ViewClearAndRefresh (hw);
	}
	else if ((eptr->img_data != NULL) &&
		 (eptr->img_data->ismap) &&
		 /*(eptr->anchorHRef != NULL) &&*/ /* already tested above */
		 (IsIsMapForm (hw, eptr->anchorHRef)))
	{
	  int form_x, form_y;

	  form_x = event->xbutton.x + hw->html.scroll_x - eptr->x;
	  form_y = event->xbutton.y + hw->html.scroll_y - eptr->y;

	  ImageSubmitForm (eptr->img_data->fptr, event,
			   eptr->img_data->text,
			   form_x, form_y);
	}
	else
	{
	  /*
	   * The following is a hack to send the
	   * selection location along with the HRef
	   * for images.  This allows you to
	   * point at a location on a map and have
	   * the server send you the related document.
	   * Tony Sanders, April 1993 <sanders@bsdi.com>
	   */
	  int alloced = 0;
	  char *buf = eptr->anchorHRef;
	  if (eptr->type == E_IMAGE  &&
	      eptr->img_data  &&  eptr->img_data->ismap)
	  {
	    buf = (char *) XtMalloc (strlen (eptr->anchorHRef) + 256);
	    alloced = 1;
	    sprintf (buf, "%s?%d,%d",
		     eptr->anchorHRef,
		     event->xbutton.x + hw->html.scroll_x - eptr->x,
		     event->xbutton.y + hw->html.scroll_y - eptr->y);
	  }
	  /*
	   * XXX: should call a media dependent
	   * function that decides how to munge the
	   * HRef.  For example mpeg data will want
	   * to know on what frame the event occured.
	   *
	   * cddata.href = *(eptr->eventf)(eptr, event);
	   */
	  cbdata.event = event;
	  cbdata.element_id = eptr->ele_id;
	  cbdata.href = buf;
	  /* cbdata.href = eptr->anchorHRef; */
	  cbdata.text = tptr;
	  XtCallCallbackList ((Widget) hw,
			      hw->html.anchor_callback,
			      (XtPointer) & cbdata);
	  if (alloced) XtFree (buf);
	  if (tptr != NULL) XtFree (tptr);
	}
      }
    }
  }
}


/*
 * SetValues is called by XtSetValues when XtSetValues is used to change
 * resources in this widget.
 */
static Boolean
SetValues (current, request, new)
HTMLWidget current;
HTMLWidget request;
HTMLWidget new;
{
  int reformatted;

  /*
   *    Make sure the underline numbers are within bounds.
   */
  if (request->html.num_anchor_underlines < 0)
  {
    new->html.num_anchor_underlines = 0;
  }
  if (request->html.num_anchor_underlines > MAX_UNDERLINES)
  {
    new->html.num_anchor_underlines = MAX_UNDERLINES;
  }
  if (request->html.num_visitedAnchor_underlines < 0)
  {
    new->html.num_visitedAnchor_underlines = 0;
  }
  if (request->html.num_visitedAnchor_underlines > MAX_UNDERLINES)
  {
    new->html.num_visitedAnchor_underlines = MAX_UNDERLINES;
  }

  reformatted = 0;
  if ((request->html.raw_text != current->html.raw_text) ||
      (request->html.header_text != current->html.header_text) ||
      (request->html.footer_text != current->html.footer_text))
  {
    DiscardCurPageObjects (current, new);

    /*
     * Parse the raw text with the HTML parser
     */
    new->html.html_objects =
	HTMLParse (current->html.html_objects, request->html.raw_text);
    CallLinkCallbacks (new);

    new->html.html_header_objects =
	HTMLParse (current->html.html_header_objects,
		   request->html.header_text);

    new->html.html_footer_objects =
	HTMLParse (current->html.html_footer_objects,
		   request->html.footer_text);

    /*
     * Redisplay for the changed data.
     */
    {
      new->html.scroll_x = 0;
      new->html.scroll_y = 0;
      new->html.max_pre_width = DocumentWidth (new,
					       new->html.html_objects);
      ReformatWindow (new);
      ViewClearAndRefresh (new);
      reformatted = 1;
    }

    /*
     * Clear any previous selection
     */
    new->html.select_start = NULL;
    new->html.select_end = NULL;
    new->html.sel_start_pos = 0;
    new->html.sel_end_pos = 0;
    new->html.new_start = NULL;
    new->html.new_end = NULL;
    new->html.new_start_pos = 0;
    new->html.new_end_pos = 0;
    new->html.active_anchor = NULL;
  }
  else if (request->html.font            != current->html.font  ||
	   request->html.italic_font     != current->html.italic_font  ||
	   request->html.bold_font       != current->html.bold_font  ||
	   request->html.fixed_font      != current->html.fixed_font  ||
	   request->html.fixedbold_font  != current->html.fixedbold_font  ||
	   request->html.fixeditalic_font!= current->html.fixeditalic_font  ||
	   request->html.header1_font    != current->html.header1_font  ||
	   request->html.header2_font    != current->html.header2_font  ||
	   request->html.header3_font	 != current->html.header3_font  ||
	   request->html.header4_font    != current->html.header4_font  ||
	   request->html.header5_font 	 != current->html.header5_font  ||
	   request->html.header6_font 	 != current->html.header6_font  ||
	   request->html.address_font 	 != current->html.address_font  ||
	   request->html.plain_font      != current->html.plain_font  ||
	   request->html.plainbold_font  != current->html.plainbold_font  ||
	   request->html.plainitalic_font!= current->html.plainitalic_font  ||
	   request->html.listing_font    != current->html.listing_font  ||
	   request->html.activeAnchor_fg != current->html.activeAnchor_fg  ||
	   request->html.activeAnchor_bg != current->html.activeAnchor_bg  ||
	   request->html.anchor_fg       != current->html.anchor_fg  ||
	   request->html.visitedAnchor_fg!= current->html.visitedAnchor_fg  ||
	   request->html.dashed_anchor_lines
	            != current->html.dashed_anchor_lines  ||
	   request->html.dashed_visitedAnchor_lines
	            != current->html.dashed_visitedAnchor_lines  ||
	   request->html.num_anchor_underlines
	            != current->html.num_anchor_underlines  ||
	   request->html.num_visitedAnchor_underlines
	            != current->html.num_visitedAnchor_underlines 
	  )
  {
    if (request->html.plain_font   != current->html.plain_font  ||
	request->html.listing_font != current->html.listing_font)
    {
      new->html.max_pre_width = DocumentWidth (new, new->html.html_objects);
    }

    ReformatWindow (new);
    ScrollWidgets (new);
    ViewClearAndRefresh (new);
    reformatted = 1;
  }

  /*
   * Image borders have been changed
   */
  if (request->html.border_images != current->html.border_images)
  {
    ReformatWindow (new);
    ScrollWidgets (new);
    ViewClearAndRefresh (new);
    reformatted = 1;
  }

  /*
   * vertical space has been changed
   */
  if (request->html.percent_vert_space != current->html.percent_vert_space)
  {
    ReformatWindow (new);
    ScrollWidgets (new);
    ViewClearAndRefresh (new);
    reformatted = 1;
  }

  return (False);
}


/*
 * Go through the parsed marks and for all the <LINK> tags in the document
 * call the LinkCallback.
 */
static void
CallLinkCallbacks (hw)
HTMLWidget hw;
{
  struct mark_up *mptr;
  LinkInfo l_info;

  mptr = hw->html.html_objects;
  while (mptr != NULL)
  {
    if (mptr->type == M_BASE)
    {
      l_info.href = ParseMarkTag (mptr->start, MT_BASE,
				  "HREF");
      l_info.role = ParseMarkTag (mptr->start, MT_BASE,
				  "ROLE");
      XtCallCallbackList ((Widget) hw, hw->html.link_callback,
			  (XtPointer) & l_info);
      if (l_info.href != NULL)
      {
	XtFree (l_info.href);
      }
      if (l_info.role != NULL)
      {
	XtFree (l_info.role);
      }
    }
    mptr = mptr->next;
  }
}


static Boolean
ConvertSelection (w, selection, target, type, value, length, format)
Widget w;
Atom *selection, *target, *type;
caddr_t *value;
unsigned long *length;
int *format;
{
  Display *d = XtDisplay (w);
  HTMLWidget hw = (HTMLWidget) w;
  char *text;

  if (hw->html.select_start == NULL)
  {
    return False;
  }

  if (*target == XA_TARGETS (d))
  {
    Atom *targetP;
    Atom *std_targets;
    unsigned long std_length;
    XmuConvertStandardSelection (w, hw->html.selection_time,
			 selection, target, type, (caddr_t *) & std_targets,
				 &std_length, format);

    *length = std_length + 5;
    *value = (caddr_t) XtMalloc (sizeof (Atom) * (*length));
    targetP = *(Atom **) value;
    *targetP++ = XA_STRING;
    *targetP++ = XA_TEXT (d);
    *targetP++ = XA_COMPOUND_TEXT (d);
    *targetP++ = XA_LENGTH (d);
    *targetP++ = XA_LIST_LENGTH (d);

    memcpy ((char *)targetP, (char *) std_targets, sizeof (Atom) * std_length);
    XtFree ((char *) std_targets);
    *type = XA_ATOM;
    *format = 32;
    return True;
  }

  if (*target == XA_STRING || *target == XA_TEXT (d) ||
      *target == XA_COMPOUND_TEXT (d))
  {
    if (*target == XA_COMPOUND_TEXT (d))
    {
      *type = *target;
    }
    else
    {
      *type = XA_STRING;
    }
    text = ParseTextToString ((hw->html.fancy_selections == True) ? hw : NULL,
			      hw->html.select_start, hw->html.select_end,
			      hw->html.sel_start_pos, hw->html.sel_end_pos,
			      hw->html.font->max_bounds.width);
    *value = text;
    *length = strlen (*value);
    *format = 8;
    return True;
  }

  if (*target == XA_LIST_LENGTH (d))
  {
    *value = XtMalloc (4);
    if (sizeof (long) == 4)
    {
      *(long *) *value = 1;
    }
    else
    {
      long temp = 1;
      memcpy((char *)*value, ((char *) &temp) + sizeof (long) - 4, 4);
    }
    *type = XA_INTEGER;
    *length = 1;
    *format = 32;
    return True;
  }

  if (*target == XA_LENGTH (d))
  {
    text = ParseTextToString ((hw->html.fancy_selections == True) ? hw : NULL,
			      hw->html.select_start, hw->html.select_end,
			      hw->html.sel_start_pos, hw->html.sel_end_pos,
			      hw->html.font->max_bounds.width);
    *value = XtMalloc (4);
    if (sizeof (long) == 4)
    {
      *(long *) *value = strlen (text);
    }
    else
    {
      long temp = strlen (text);
      memcpy ((char *)*value, ((char *) &temp) + sizeof (long) - 4, 4);
    }
    XtFree (text);
    *type = XA_INTEGER;
    *length = 1;
    *format = 32;
    return True;
  }

  if (XmuConvertStandardSelection (w, hw->html.selection_time, selection,
				   target, type, value, length, format))
  {
    return True;
  }

  return False;
}


static void
LoseSelection (w, selection)
Widget w;
Atom *selection;
{
  HTMLWidget hw = (HTMLWidget) w;

  ClearSelection (hw);
}


static void
SelectionDone (w, selection, target)
Widget w;
Atom *selection, *target;
{
  /*
   * empty proc so Intrinsics know we want to keep storage 
   */
}


/*
 *************************************************************************
 ******************************* PUBLIC FUNCTIONS ************************
 *************************************************************************
 */


/*
 * Convenience function to return the text of the HTML document as a plain
 * ascii text string.
 * This function allocates memory for the returned string, that it is up
 * to the user to free.
 * Extra option flags "pretty" text to be returned.
 * when pretty is two or larger, Postscript is returned. The font used is
 * encoded in the pretty parameter:
 * pretty = 2: Times
 * pretty = 3: Helvetica
 * pretty = 4: New century schoolbook
 * pretty = 5: Lucida Bright
 */
char *
HTMLGetText (w, pretty)
Widget w;
int pretty;
{
  HTMLWidget hw = (HTMLWidget) w;
  char *text;
  char *tptr, *buf;
  struct ele_rec *start;
  struct ele_rec *end;

  text = NULL;
  start = hw->html.formatted_elements;
#if 0  /* WBE: this gets my vote for Most Useless Original Code.  :-) */
  end = start;
  while (end != NULL)
  {
    end = end->next;
  }
#endif
  end = NULL;			/* WBE: its replacement */

  if (pretty >= 2)
  {
    tptr = ParseTextToPSString (hw, start, end, 0, 0,
				hw->html.font->max_bounds.width,
				hw->html.margin_width, pretty - 2);
  }
  else
  {
    tptr = ParseTextToString (pretty ? hw : NULL, start, end, 0, 0,
			      hw->html.font->max_bounds.width);
  }
  if (tptr != NULL)
  {
    if (text == NULL)
    {
      text = tptr;
    }
    else
    {
      buf = (char *) XtMalloc (strlen (text) +
			       strlen (tptr) + 1);
      strcpy (buf, text);
      strcat (buf, tptr);
      XtFree (text);
      XtFree (tptr);
      text = buf;
    }
  }
  return (text);
}


/*
 * Convenience function to return the element id of the element
 * nearest to the x,y coordinates passed in.
 * If there is no element there, return the first element in the
 * line we are on.  If we are on no line, either return the
 * beginning, or the end of the document.
 */
#if 0				/* not used */
/*extern int	 HTMLPositionToId _ArgProto((Widget w, int x, int y));*/
int
HTMLPositionToId (w, x, y)
Widget w;
int x, y;
{
  HTMLWidget hw = (HTMLWidget) w;
  int i;
  int epos;
  struct ele_rec *eptr;

  eptr = LocateElement (hw, x, y, &epos, NULL);
  if (eptr == NULL)
  {
    x = x + hw->html.scroll_x;
    y = y + hw->html.scroll_y;
    eptr = hw->html.line_array[0];
    for (i = 0; i < hw->html.doc_line_count; i++)
    {
      if (hw->html.line_array[i] == NULL)
      {
	continue;
      }
      else if (hw->html.line_array[i]->y <= y)
      {
	eptr = hw->html.line_array[i];
	continue;
      }
      else
      {
	break;
      }
    }
  }

  /*
   * 0 means the very top of the document.  We put you there for
   * unfound elements.
   * We also treat the special case when the scrollbar is at the
   * absolute top.
   */
  if ((eptr == NULL) || (hw->html.scroll_y == 0))
  {
    return (0);
  }
  else
  {
    return (eptr->ele_id);
  }
}
#endif


/*
 * Convenience function to return the position of the element
 * based on the element id passed in.
 * Function returns 1 on success and fills in x,y pixel values.
 * If there is no such element, x=0, y=0 and -1 is returned.
 */
#if 0				/* not used */
/*extern int	 HTMLIdToPosition _ArgProto((Widget w, int element_id,
					     int *x, int *y));*/
int
HTMLIdToPosition (w, element_id, x, y)
Widget w;
int element_id;
int *x, *y;
{
  HTMLWidget hw = (HTMLWidget) w;
  struct ele_rec *start;
  struct ele_rec *eptr;

  eptr = NULL;
  start = hw->html.formatted_elements;
  while (start != NULL)
  {
    if (start->ele_id == element_id)
    {
      eptr = start;
      break;
    }
    start = start->next;
  }

  if (eptr == NULL)
  {
    *x = 0;
    *y = 0;
    return (-1);
  }
  else
  {
    *x = eptr->x;
    *y = eptr->y;
    return (1);
  }
}
#endif

/*
 * Convenience function to position the element
 * based on the element id passed at the top of the viewing area.
 * A passed in id of 0 means goto the top.
 */
void
HTMLGotoId (w, element_id)
Widget w;
int element_id;
{
  HTMLWidget hw = (HTMLWidget) w;
  struct ele_rec *start;
  struct ele_rec *eptr;
  int newy;

  /*
   * If we have no scrollbar, just return.
   */
  if (hw->html.use_vbar == False)
  {
    return;
  }

  /*
   * Find the element corrsponding to the id passed in.
   */
  eptr = NULL;
  start = hw->html.formatted_elements;
  while (start != NULL)
  {
    if (start->ele_id == element_id)
    {
      eptr = start;
      break;
    }
    start = start->next;
  }

  /*
   * No such element, do nothing.
   */
  if ((element_id != 0) && (eptr == NULL))
  {
    return;
  }

  if (element_id == 0)
  {
    newy = 0;
  }
  else
  {
    newy = eptr->y - (int) hw->html.view_height / 2;
  }
  if (newy < 0)
  {
    newy = 0;
  }
  if (newy > (hw->html.doc_height - (int) hw->html.view_height / 2))
  {
    newy = hw->html.doc_height - (int) hw->html.view_height / 2;
  }
  if (newy < 0)
  {
    newy = 0;
  }
  ScrollToPos (hw->html.vbar, hw, newy);
  ScrollToPos (hw->html.hbar, hw, 0);
  setScrollBar (hw->html.vbar, newy,
		hw->html.doc_height,
		hw->html.view_height);
}


/*
 * Convenience function to return the position of the anchor
 * based on the anchor NAME passed.
 * Function returns 1 on success and fills in x,y pixel values.
 * If there is no such element, x=0, y=0 and -1 is returned.
 */
int
HTMLAnchorToPosition (w, name, x, y)
Widget w;
char *name;
int *x, *y;
{
  HTMLWidget hw = (HTMLWidget) w;
  struct ele_rec *start;
  struct ele_rec *eptr;

  eptr = NULL;
  start = hw->html.formatted_elements;
  while (start != NULL)
  {
    if ((start->anchorName) &&
	(strcmp (start->anchorName, name) == 0))
    {
      eptr = start;
      break;
    }
    start = start->next;
  }

  if (eptr == NULL)
  {
    *x = 0;
    *y = 0;
    return (-1);
  }
  else
  {
    *x = eptr->x;
    *y = eptr->y;
    return (1);
  }
}


/*
 * Convenience function to return the element id of the anchor
 * based on the anchor NAME passed.
 * Function returns id on success.
 * If there is no such element, 0 is returned.
 */
int
HTMLAnchorToId (w, name)
Widget w;
char *name;
{
  HTMLWidget hw = (HTMLWidget) w;
  struct ele_rec *start;

  /*
   * Find the passed anchor name
   */
  start = hw->html.formatted_elements;
  for ( ;  start != NULL;  start = start->next)
  {
    if (start->anchorName  &&  strcmp (start->anchorName, name) == 0)
    {
      return (start->ele_id);
    }
  }

  return 0;
}


/*
 * Convenience function to return the HREFs of all active anchors in the
 * document.
 * Function returns an array of strings and fills num_hrefs passed.
 * If there are no HREFs NULL returned.
 */
char **
HTMLGetHRefs (w, num_hrefs)
Widget w;
int *num_hrefs;
{
  HTMLWidget hw = (HTMLWidget) w;
  int cnt;
  struct ele_rec *start;
  struct ele_rec *list;
  struct ele_rec *eptr;
  char **harray;

  list = NULL;
  cnt = 0;
  /*
   * Construct a linked list of all the different hrefs, counting
   * them as we go.
   */
  start = hw->html.formatted_elements;
  for ( ;  start != NULL;  start = start->next)
  {
    /*
     * This one has an HREF
     */
    if (start->anchorHRef != NULL)
    {
      /*
       * Check to see if we already have this HREF in our list.
       */
      for (eptr = list;  eptr != NULL;  eptr = eptr->next)
      {
	if (strcmp (eptr->anchorHRef, start->anchorHRef) == 0)
	{
	  break;		/* match found;  exit with eptr != NULL */
	}
      }
      /*
       * If this HREF is new and isn't an internal reference, add it.
       */
      if (eptr == NULL  &&  start->internal == False)
      {
	eptr = (struct ele_rec *)XtMalloc (sizeof (struct ele_rec));
	eptr->anchorHRef = start->anchorHRef;
	eptr->next = list;	/* add to head of list */
	list = eptr;
	cnt++;
      }
    }
  }
  *num_hrefs = cnt;

  if (cnt == 0)
  {
    return (NULL);
  }
  else
  {
    harray = (char **) XtMalloc (sizeof (char *) * cnt);
    while (list != NULL)
    {
      harray[--cnt] = XtNewString (list->anchorHRef);
      eptr = list;
      list = list->next;
      XtFree ((char *) eptr);
    }
    return (harray);
  }
}


/*
 * Convenience function to return the SRCs of all images in the
 * document.
 * Function returns an array of strings and fills num_srcs passed.
 * If there are no SRCs NULL returned.
 */
char **
HTMLGetImageSrcs (w, num_srcs)
Widget w;
int *num_srcs;
{
  HTMLWidget hw = (HTMLWidget) w;
  struct mark_up *mptr;
  int cnt;
  char *tptr;
  char **harray;

  cnt = 0;
  mptr = hw->html.html_objects;
  while (mptr != NULL)
  {
    if (mptr->type == M_IMAGE)
    {
      tptr = ParseMarkTag (mptr->start, MT_IMAGE, "SRC");
      if ((tptr != NULL) && (*tptr != '\0'))
      {
	cnt++;
	XtFree (tptr);
      }
    }
    mptr = mptr->next;
  }

  if (cnt == 0)
  {
    *num_srcs = 0;
    return (NULL);
  }
  else
  {
    *num_srcs = cnt;
    harray = (char **) XtMalloc (sizeof (char *) * cnt);
    mptr = hw->html.html_objects;
    cnt = 0;
    while (mptr != NULL)
    {
      if (mptr->type == M_IMAGE)
      {
	tptr = ParseMarkTag (mptr->start, MT_IMAGE, "SRC");
	if ((tptr != NULL) && (*tptr != '\0'))
	{
	  harray[cnt] = tptr;
	  cnt++;
	}
      }
      mptr = mptr->next;
    }
    return (harray);
  }
}


/*
 * Convenience function to return the link information
 * for all the <LINK> tags in the document.
 * Function returns an array of LinkInfo structures and fills
 * num_links passed.
 * If there are no LINKs NULL returned.
 */
LinkInfo *
HTMLGetLinks (w, num_links)
Widget w;
int *num_links;
{
  HTMLWidget hw = (HTMLWidget) w;
  struct mark_up *mptr;
  int cnt;
  char *tptr;
  LinkInfo *larray;

  cnt = 0;
  mptr = hw->html.html_objects;
  while (mptr != NULL)
  {
    if (mptr->type == M_BASE)
    {
      cnt++;
    }
    mptr = mptr->next;
  }

  if (cnt == 0)
  {
    *num_links = 0;
    return (NULL);
  }
  else
  {
    *num_links = cnt;
    larray = (LinkInfo *) XtMalloc (sizeof (LinkInfo) * cnt);
    mptr = hw->html.html_objects;
    cnt = 0;
    while (mptr != NULL)
    {
      if (mptr->type == M_BASE)
      {
	tptr = ParseMarkTag (mptr->start, MT_BASE, "HREF");
	larray[cnt].href = tptr;
	tptr = ParseMarkTag (mptr->start, MT_BASE, "ROLE");
	larray[cnt].role = tptr;
	cnt++;
      }
      mptr = mptr->next;
    }
    return (larray);
  }
}



static void
HTMLFreeImageInfo (w)
Widget w;
{
  HTMLWidget hw = (HTMLWidget) w;

  FreeColors (XtDisplay (w), DefaultColormapOfScreen (XtScreen (w)));
  FreeImages (hw);
}


static void
HTMLFreeWidgetInfo (w)
Widget w;
{
  HTMLWidget hw = (HTMLWidget) w;
  WidgetInfo *wptr = hw->html.widget_list;
  WidgetInfo *tptr;

  hw->html.widget_list = NULL;

  while (wptr != NULL)
  {
    tptr = wptr;
    wptr = wptr->next;
    if (tptr->w != NULL)
    {
      /*
       * This is REALLY DUMB, but X generates an expose event
       * for the destruction of the Widget, even if it isn't
       * mapped at the time it is destroyed.
       * So I move the invisible widget to -1000,-1000
       * before destroying it, to avoid a visible flash.
       */
      XtMoveWidget (tptr->w, -1000, -1000);
      XtDestroyWidget (tptr->w);
    }
    /*
     * Once upon a time, the code said:
     *
     *   if ((tptr->value != NULL) && (tptr->type != W_OPTIONMENU))
     *      XtFree (tptr->value);
     *
     * However, that doesn't work.  In particular, several widgets use
     * callbacks to CBTextDestroy to free the string in tptr->value.  I can't
     * tell easily here which widgets clean up value and which, if any,
     * don't.  I've opted for the following new policy: X/widget stuff should
     * clean up 'value' itself; this code will only clean up value if no
     * widget was created.  That may create a memory leak, but any such leak
     * will be relatively small (since the value strings tend to be small),
     * and it's much better than multiply freeing one.  -WBE 97Apr30
     */
    else if (tptr->value != NULL)  XtFree (tptr->value);

    if (tptr->name != NULL)  XtFree (tptr->name);

    XtFree ((char *) tptr);
  }
}


/*
 * DiscardCurPageObjects
 *
 * Delete, discard, free, clean up, etc. any allocated resources associated
 * with the currently displayed document.  This should be called before
 * switching to a new page, and should NOT be called for resizing,
 * reformatting, reparsing, etc. of the current page.
 *
 * NOTE: There is an assumption in much of the code that as long as it
 * stays on the same page (excluding reloading), it will see the same HTML
 * in the same order and thus create exactly the same images and widgets
 * in exactly the same order, though possibly in different locations.  By
 * keeping the objects around, one speeds up reformatting, but then one has
 * to make sure to discard everything when switching to a new page.  That's
 * where DiscardCurPageObjects comes in.
 */
static void
DiscardCurPageObjects (old, new)
HTMLWidget old, new;		/* may be the same */
{
  /*
   * Free up the old visited href list.
   */
  FreeHRefs (old->html.my_visited_hrefs);
  new->html.my_visited_hrefs = NULL;

  /*
   * Free any old colors, pixmaps, and widgets
   */
  HTMLFreeImageInfo ((Widget)old);
  HTMLFreeWidgetInfo ((Widget)old);

  /*
   * Free up old Forms descriptors after freeing Widgets
   */
  FreeForms (old);
  new->html.form_list = NULL;
}


/*
 * Convenience function to redraw all active anchors in the
 * document.
 * Can also pass a new predicate function to check visited
 * anchors.  If NULL passed for function, uses default predicate
 * function.
 */
void
HTMLRetestAnchors (w, testFunc)
Widget w;
visitTestProc testFunc;
{
  HTMLWidget hw = (HTMLWidget) w;
  struct ele_rec *start;

  if (testFunc == NULL)
  {
    testFunc = (visitTestProc) hw->html.previously_visited_test;
  }

  /*
   * Search all elements
   */
  start = hw->html.formatted_elements;
  for ( ;  start != NULL;  start = start->next)
  {
    if ((start->internal == True) || (start->anchorHRef == NULL))
    {
      continue;
    }

    if (testFunc != NULL)
    {
      if ((*testFunc) (hw, start->anchorHRef))
      {
	start->fg = hw->html.visitedAnchor_fg;
	start->underline_number = hw->html.num_visitedAnchor_underlines;
	start->dashed_underline = hw->html.dashed_visitedAnchor_lines;
      }
      else
      {
	start->fg = hw->html.anchor_fg;
	start->underline_number = hw->html.num_anchor_underlines;
	start->dashed_underline = hw->html.dashed_anchor_lines;
      }
    }
    else
    {
      start->fg = hw->html.anchor_fg;
      start->underline_number = hw->html.num_anchor_underlines;
      start->dashed_underline = hw->html.dashed_anchor_lines;
    }

    /*
     * Since the element may have changed, redraw it
     */
    switch (start->type)
    {
      case E_TEXT:
        TextRefresh (hw, start, 0, (start->edata_len - 2), 0);
	break;
      case E_IMAGE:
	ImageRefresh (hw, start);
	break;
      case E_BULLET:
	BulletRefresh (hw, start);
	break;
      case E_LINEFEED:
	LinefeedRefresh (hw, start);
	break;
    }
  }
}


void
HTMLClearSelection (w)
Widget w;
{
  LoseSelection (w, NULL);
}


/*
 * Set the current selection based on the ElementRefs passed in.
 * Both refs must be valid.
 */
void
HTMLSetSelection (w, start, end)
Widget w;
ElementRef *start;
ElementRef *end;
{
  HTMLWidget hw = (HTMLWidget) w;
  int found;
  struct ele_rec *eptr;
  struct ele_rec *e_start;
  struct ele_rec *e_end;
  int start_pos, end_pos;
  Atom *atoms;
  int i, buffer;
  char *text;
  char *params[2];

  /*
   * If the starting position is not valid, fail the selection
   */
  if ((start->id > 0) && (start->pos >= 0))
  {
    found = 0;
    for (eptr = hw->html.formatted_elements; eptr != NULL; eptr = eptr->next)
    {
      if (eptr->ele_id == start->id)
      {
	e_start = eptr;
	start_pos = start->pos;
	found = 1;
	break;
      }
    }
    if (!found)
    {
      return;
    }
  }

  /*
   * If the ending position is not valid, fail the selection
   */
  if ((end->id > 0) && (end->pos >= 0))
  {
    found = 0;
    for (eptr = hw->html.formatted_elements; eptr != NULL; eptr = eptr->next)
    {
      if (eptr->ele_id == end->id)
      {
	e_end = eptr;
	end_pos = end->pos;
	found = 1;
	break;
      }
    }
    if (!found)
    {
      return;
    }
  }

  LoseSelection (w, NULL);

  /*
   * We expect the ElementRefs came from HTMLSearchText, so we know
   * that the end_pos is one past what we want to select.
   */
  end_pos = end_pos - 1;

  /*
   * Sanify the position data
   */
  if ((start_pos > 0) && (start_pos >= e_start->edata_len - 1))
  {
    start_pos = e_start->edata_len - 2;
  }
  if ((end_pos > 0) && (end_pos >= e_end->edata_len - 1))
  {
    end_pos = e_end->edata_len - 2;
  }

  hw->html.select_start = e_start;
  hw->html.sel_start_pos = start_pos;
  hw->html.select_end = e_end;
  hw->html.sel_end_pos = end_pos;
  SetSelection (hw);
  hw->html.new_start = NULL;
  hw->html.new_end = NULL;
  hw->html.new_start_pos = 0;
  hw->html.new_end_pos = 0;

  /*
   * Do all the gunk from the end of the ExtendEnd function
   */
  params[0] = "PRIMARY";
  params[1] = "CUT_BUFFER0";
  atoms = (Atom *) XtMalloc (2 * sizeof (Atom));
  if (atoms == NULL)
  {
    fprintf (stderr, "cannot allocate atom list\n");
    return;
  }
  XmuInternStrings (XtDisplay ((Widget) hw), params, 2, atoms);
  hw->html.selection_time = CurrentTime;
  for (i = 0; i < 2; i++)
  {
    switch (atoms[i])
    {
    case XA_CUT_BUFFER0:
      buffer = 0;
      break;
    case XA_CUT_BUFFER1:
      buffer = 1;
      break;
    case XA_CUT_BUFFER2:
      buffer = 2;
      break;
    case XA_CUT_BUFFER3:
      buffer = 3;
      break;
    case XA_CUT_BUFFER4:
      buffer = 4;
      break;
    case XA_CUT_BUFFER5:
      buffer = 5;
      break;
    case XA_CUT_BUFFER6:
      buffer = 6;
      break;
    case XA_CUT_BUFFER7:
      buffer = 7;
      break;
    default:
      buffer = -1;
      break;
    }
    if (buffer >= 0)
    {
      text = ParseTextToString ((hw->html.fancy_selections == True) ? hw:NULL,
				hw->html.select_start,
				hw->html.select_end,
				hw->html.sel_start_pos,
				hw->html.sel_end_pos,
				hw->html.font->max_bounds.width);
      XStoreBuffer (XtDisplay ((Widget) hw),
		    text, strlen (text), buffer);
      XtFree (text);
    }
    else
    {
      XtOwnSelection ((Widget) hw, atoms[i], CurrentTime,
		      (XtConvertSelectionProc) ConvertSelection,
		      (XtLoseSelectionProc) LoseSelection,
		      (XtSelectionDoneProc) SelectionDone);
    }
  }
  XtFree ((char *) atoms);
}


/*
 * Convenience function to return the text of the HTML document as a single
 * white space separated string, with pointers to the various start and
 * end points of selections.
 * This function allocates memory for the returned string, that it is up
 * to the user to free.
 */
char *
HTMLGetTextAndSelection (w, startp, endp, insertp)
Widget w;
char **startp;
char **endp;
char **insertp;
{
  HTMLWidget hw = (HTMLWidget) w;
  int length;
  char *text;
  char *tptr;
  struct ele_rec *eptr;
  struct ele_rec *sel_start;
  struct ele_rec *sel_end;
  struct ele_rec *insert_start;
  int start_pos, end_pos, insert_pos;

  if (SwapElements (hw->html.select_start, hw->html.select_end,
		    hw->html.sel_start_pos, hw->html.sel_end_pos))
  {
    sel_end = hw->html.select_start;
    end_pos = hw->html.sel_start_pos;
    sel_start = hw->html.select_end;
    start_pos = hw->html.sel_end_pos;
  }
  else
  {
    sel_start = hw->html.select_start;
    start_pos = hw->html.sel_start_pos;
    sel_end = hw->html.select_end;
    end_pos = hw->html.sel_end_pos;
  }

  insert_start = hw->html.new_start;
  insert_pos = hw->html.new_start_pos;
  *startp = NULL;
  *endp = NULL;
  *insertp = NULL;

  length = 0;
  for (eptr = hw->html.formatted_elements;  eptr != NULL;  eptr = eptr->next)
  {
    /*
     * Skip the special internal text
     */
    if (eptr->internal == True)
    {
      continue;
    }

    if (eptr->type == E_TEXT)
    {
      length = length + eptr->edata_len - 1;
    }
    else if (eptr->type == E_LINEFEED)
    {
      length = length + 1;
    }
  }

  text = (char *) XtMalloc (length + 1);
  if (text == NULL)
  {
    fprintf (stderr, "No space for return string\n");
    return (NULL);
  }
  strcpy (text, "");

  tptr = text;

  
  for (eptr = hw->html.formatted_elements;  eptr != NULL;  eptr = eptr->next)
  {
    /*
     * Skip the special internal text
     */
    if (eptr->internal == True)
    {
      continue;
    }

    if (eptr->type == E_TEXT)
    {
      if (eptr == sel_start)
      {
	*startp = (char *) (tptr + start_pos);
      }

      if (eptr == sel_end)
      {
	*endp = (char *) (tptr + end_pos);
      }

      if (eptr == insert_start)
      {
	*insertp = (char *) (tptr + insert_pos);
      }

      strcat (text, (char *) eptr->edata);
      tptr = tptr + eptr->edata_len - 1;
    }
    else if (eptr->type == E_LINEFEED)
    {
      if (eptr == sel_start)
      {
	*startp = tptr;
      }

      if (eptr == sel_end)
      {
	*endp = tptr;
      }

      if (eptr == insert_start)
      {
	*insertp = tptr;
      }

      strcat (text, " ");
      tptr = tptr + 1;
    }
  }
  return (text);
}


/*
 * HTMLSetText
 *
 * Convenience function to set the raw text into the widget.  If the
 * page's contents might change, do the full set of DiscardCurPageObjects,
 * reparse, reload images, and reformat.
 *
 * If any text pointer is passed in as NULL that text is unchanged;
 * if a text pointer points to "", that text is set to NULL.
 */
void
HTMLSetText (w, text, header_text, footer_text)
Widget w;
char *text;
char *header_text;
char *footer_text;
{
  HTMLWidget hw = (HTMLWidget) w;
  struct ele_rec *start;
  struct ele_rec *eptr;

  if (text == NULL  &&  header_text == NULL  &&  footer_text == NULL)
  {
    return;
  }

  DiscardCurPageObjects (hw, hw);

  if (text != NULL)
  {
    hw->html.raw_text = (*text != '\0') ?  text : NULL;

    /*
     * Parse the raw text with the HTML parser
     */
    hw->html.html_objects = HTMLParse (hw->html.html_objects,
				       hw->html.raw_text);
    CallLinkCallbacks (hw);
  }

  if (header_text != NULL)
  {
    hw->html.header_text = (*header_text != '\0') ?  header_text : NULL;

    /*
     * Parse the header text with the HTML parser
     */
    hw->html.html_header_objects = HTMLParse (hw->html.html_header_objects,
					      hw->html.header_text);
  }
  if (footer_text != NULL)
  {
    hw->html.footer_text = (*footer_text != '\0') ?  footer_text : NULL;

    /*
     * Parse the footer text with the HTML parser
     */
    hw->html.html_footer_objects = HTMLParse (hw->html.html_footer_objects,
					      hw->html.footer_text);
  }

  /*
   * Reformat the new text
   */
  hw->html.max_pre_width = DocumentWidth (hw, hw->html.html_objects);
  ReformatWindow (hw);

  /*
   * This code previously tried to position to the top of the document.
   * Now we let the scrollbar code remember the location and position
   * things appropriately, since that's nicer when resizing the window.
   */
#if 0
  hw->html.scroll_x = 0;
  hw->html.scroll_y = 0;
#endif

#ifdef DEBUG
  fprintf (stderr, "calling in HTMLSetText\n");
#endif

  ConfigScrollBars (hw);
  ScrollWidgets (hw);

  /*
   * Display the new text
   */
  ViewClearAndRefresh (hw);

  /*
   * Clear any previous selection
   */
  hw->html.select_start = NULL;
  hw->html.select_end = NULL;
  hw->html.sel_start_pos = 0;
  hw->html.sel_end_pos = 0;
  hw->html.new_start = NULL;
  hw->html.new_end = NULL;
  hw->html.new_start_pos = 0;
  hw->html.new_end_pos = 0;
  hw->html.active_anchor = NULL;

  hw->html.cached_tracked_ele = NULL;
}


/*
 * To use faster TOLOWER as set up in HTMLparse.c
 */
#ifdef NOT_ASCII
#define TOLOWER(x)      (tolower(x))
#else
extern char map_table[];
#define TOLOWER(x)      (map_table[x])
#endif /* NOT_ASCII */


/*
 * Convenience function to search the text of the HTML document as a single
 * white space separated string. Linefeeds are converted into spaces.
 *
 * Takes a pattern, pointers to the start and end blocks to store the
 * start and end of the match into.  Start is also used as the location to
 * start the search from for incremental searching.  If start is an invalid
 * position (id = 0),  default start is the beginning of the document for
 * forward searching, and the end of the document for backwards searching.
 * The backward and caseless parameters I hope are self-explanatory.
 *
 * returns 1 on success
 *      (and the start and end positions of the match).
 * returns -1 otherwise (and start and end are unchanged).
 */
int
HTMLSearchText (w, pattern, m_start, m_end, backward, caseless)
Widget w;
char *pattern;
ElementRef *m_start;
ElementRef *m_end;
int backward;
int caseless;
{
  HTMLWidget hw = (HTMLWidget) w;
  int found, equal;
  char *match;
  char *tptr;
  char *mptr;
  char cval;
  struct ele_rec *eptr;
  int s_pos;
  struct ele_rec *s_eptr;
  ElementRef s_ref, e_ref;
  ElementRef *start, *end;

  /*
   * If bad parameters are passed, just fail the search
   */
  if ((pattern == NULL) || (*pattern == '\0') ||
      (m_start == NULL) || (m_end == NULL))
  {
    return (-1);
  }

  /*
   * If we are caseless, make a lower case copy of the pattern to
   * match to use in compares.
   *
   * Remember to free this before returning
   */
  if (caseless)
  {
    match = (char *) XtMalloc (strlen (pattern) + 1);
    tptr = pattern;
    mptr = match;
    while (*tptr != '\0')
    {
      *mptr = (char) TOLOWER ((int) *tptr);
      mptr++;
      tptr++;
    }
    *mptr = '\0';
  }
  else
  {
    match = pattern;
  }

  /*
   * Slimy coding.  I later decided I didn't want to change start and
   * end if the search failed.  Rather than changing all the code,
   * I just copy it into locals here, and copy it out again if a match
   * is found.
   */
  start = &s_ref;
  end = &e_ref;
  start->id = m_start->id;
  start->pos = m_start->pos;
  end->id = m_end->id;
  end->pos = m_end->pos;

  /*
   * Find the user specified start position.
   */
  if (start->id > 0)
  {
    found = 0;
    for (eptr = hw->html.formatted_elements; eptr != NULL; eptr = eptr->next)
    {
      if (eptr->ele_id == start->id)
      {
	s_eptr = eptr;
	found = 1;
	break;
      }
    }
    /*
     * Bad start position, fail them out.
     */
    if (!found)
    {
      if (caseless)
      {
	XtFree (match);
      }
      return (-1);
    }
    /*
     * Sanify the start position
     */
    s_pos = start->pos;
    if (s_pos >= s_eptr->edata_len - 1)
    {
      s_pos = s_eptr->edata_len - 2;
    }
    if (s_pos < 0)
    {
      s_pos = 0;
    }
  }
  else
  {
    /*
     * Default search starts at end for backward, and
     * beginning for forwards.
     */
    if (backward)
    {
      s_eptr = hw->html.formatted_elements;
      while (s_eptr->next != NULL)
      {
	s_eptr = s_eptr->next;
      }
      s_pos = s_eptr->edata_len - 2;
    }
    else
    {
      s_eptr = hw->html.formatted_elements;
      s_pos = 0;
    }
  }

  if (backward)
  {
    char *mend;

    /*
     * Save the end of match here for easy end to start searching
     */
    mend = match;
    while (*mend != '\0')
    {
      mend++;
    }
    if (mend > match)
    {
      mend--;
    }
    found = 0;
    equal = 0;
    mptr = mend;

    if (s_eptr != NULL)
    {
      eptr = s_eptr;
    }
    else
    {
      eptr = hw->html.formatted_elements;
      while (eptr->next != NULL)
      {
	eptr = eptr->next;
      }
    }

    for ( ;  eptr != NULL;  eptr = eptr->prev)
    {
      /*
       * Skip the special internal text
       */
      if (eptr->internal == True)
      {
	continue;
      }

      if (eptr->type == E_TEXT)
      {
	tptr = (char *) (eptr->edata + eptr->edata_len - 2);
	if (eptr == s_eptr)
	{
	  tptr = (char *) (eptr->edata + s_pos);
	}
	while (tptr >= eptr->edata)
	{
	  if (equal)
	  {
	    if (caseless)
	    {
	      cval = (char) TOLOWER ((int) *tptr);
	    }
	    else
	    {
	      cval = *tptr;
	    }
	    while ((mptr >= match) && (tptr >= eptr->edata) &&
		   (cval == *mptr))
	    {
	      tptr--;
	      mptr--;
	      if (tptr >= eptr->edata)
	      {
		if (caseless)
		{
		  cval = (char) TOLOWER ((int) *tptr);
		}
		else
		{
		  cval = *tptr;
		}
	      }
	    }
	    if (mptr < match)
	    {
	      found = 1;
	      start->id = eptr->ele_id;
	      start->pos = (int)(tptr - eptr->edata + 1);
	      break;
	    }
	    else if (tptr < eptr->edata)
	    {
	      break;
	    }
	    else
	    {
	      equal = 0;
	    }
	  }
	  else
	  {
	    mptr = mend;
	    if (caseless)
	    {
	      cval = (char) TOLOWER ((int) *tptr);
	    }
	    else
	    {
	      cval = *tptr;
	    }
	    while ((tptr >= eptr->edata) && (cval != *mptr))
	    {
	      tptr--;
	      if (tptr >= eptr->edata)
	      {
		if (caseless)
		{
		  cval = (char) TOLOWER ((int) *tptr);
		}
		else
		{
		  cval = *tptr;
		}
	      }
	    }
	    if ((tptr >= eptr->edata) &&
		(cval == *mptr))
	    {
	      equal = 1;
	      end->id = eptr->ele_id;
	      end->pos = (int)(tptr - eptr->edata + 1);
	    }
	  }
	}
      }
      /*
       * Linefeeds match to single space characters.
       */
      else if (eptr->type == E_LINEFEED)
      {
	if (equal)
	{
	  if (*mptr == ' ')
	  {
	    mptr--;
	    if (mptr < match)
	    {
	      found = 1;
	      start->id = eptr->ele_id;
	      start->pos = 0;
	    }
	  }
	  else
	  {
	    equal = 0;
	  }
	}
	else
	{
	  mptr = mend;
	  if (*mptr == ' ')
	  {
	    equal = 1;
	    end->id = eptr->ele_id;
	    end->pos = 0;
	    mptr--;
	    if (mptr < match)
	    {
	      found = 1;
	      start->id = eptr->ele_id;
	      start->pos = 0;
	    }
	  }
	}
      }
      if (found)
      {
	break;
      }
    }
  }
  else
  /* forward */
  {
    found = 0;
    equal = 0;
    mptr = match;

    eptr = (s_eptr != NULL) ?   s_eptr  :  hw->html.formatted_elements;

    for ( ;  eptr != NULL;  eptr = eptr->next)
    {
      /*
       * Skip the special internal text
       */
      if (eptr->internal == True)
      {
	continue;
      }

      if (eptr->type == E_TEXT)
      {
	tptr = eptr->edata;
	if (eptr == s_eptr)
	{
	  tptr = (char *) (tptr + s_pos);
	}
	while (*tptr != '\0')
	{
	  if (equal)
	  {
	    if (caseless)
	    {
	      cval = (char) TOLOWER ((int) *tptr);
	    }
	    else
	    {
	      cval = *tptr;
	    }
	    while ((*mptr != '\0') && (cval == *mptr))
	    {
	      tptr++;
	      mptr++;
	      if (caseless)
	      {
		cval = (char) TOLOWER ((int) *tptr);
	      }
	      else
	      {
		cval = *tptr;
	      }
	    }
	    if (*mptr == '\0')
	    {
	      found = 1;
	      end->id = eptr->ele_id;
	      end->pos = (int)(tptr - eptr->edata);
	      break;
	    }
	    else if (*tptr == '\0')
	    {
	      break;
	    }
	    else
	    {
	      equal = 0;
	    }
	  }
	  else
	  {
	    mptr = match;
	    if (caseless)
	    {
	      cval = (char) TOLOWER ((int) *tptr);
	    }
	    else
	    {
	      cval = *tptr;
	    }
	    while ((*tptr != '\0') && (cval != *mptr))
	    {
	      tptr++;
	      if (caseless)
	      {
		cval = (char) TOLOWER ((int) *tptr);
	      }
	      else
	      {
		cval = *tptr;
	      }
	    }
	    if (cval == *mptr)
	    {
	      equal = 1;
	      start->id = eptr->ele_id;
	      start->pos = (int)(tptr - eptr->edata);
	    }
	  }
	}
      }
      else if (eptr->type == E_LINEFEED)
      {
	if (equal)
	{
	  if (*mptr == ' ')
	  {
	    mptr++;
	    if (*mptr == '\0')
	    {
	      found = 1;
	      end->id = eptr->ele_id;
	      end->pos = 0;
	    }
	  }
	  else
	  {
	    equal = 0;
	  }
	}
	else
	{
	  mptr = match;
	  if (*mptr == ' ')
	  {
	    equal = 1;
	    start->id = eptr->ele_id;
	    start->pos = 0;
	    mptr++;
	    if (*mptr == '\0')
	    {
	      found = 1;
	      end->id = eptr->ele_id;
	      end->pos = 0;
	    }
	  }
	}
      }
      if (found)
      {
	break;
      }
    }
  }

  if (found)
  {
    m_start->id = start->id;
    m_start->pos = start->pos;
    m_end->id = end->id;
    m_end->pos = end->pos;
  }

  if (caseless)
  {
    XtFree (match);
  }

  if (found)
  {
    return (1);
  }
  else
  {
    return (-1);
  }
}
