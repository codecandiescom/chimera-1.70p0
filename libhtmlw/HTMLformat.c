/* Nearly completely rewritten by WBE, Spring 1997, for Chimera 1.70. */

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

#ifdef TIMING
#include <sys/time.h>
struct timeval Tv;
struct timezone Tz;
#endif

#include <stdio.h>
#include <ctype.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "HTMLP.h"

/* export */ int errors_in_HTML_document;

#define Max(a,b) (((a) > (b)) ?  (a) : (b))

/*
 * I need my own is ispunct function because I need a closing paren
 * immediately after a word to act like punctuation.
 */
#define	MY_ISPUNCT(val)	(ispunct((int)(val)) || ((val) == ')'))

#define INDENT_SPACES	2
#define IMAGE_BORDER	2

#define D_NONE		0
#define D_TITLE		1
#define D_TEXT		2
#define D_OLIST		3
#define D_ULIST		4

#define ALIGN_BOTTOM	0
#define ALIGN_MIDDLE	1
#define ALIGN_TOP	2


extern struct ele_rec *AddEle ();
extern void FreeLineList ();
extern void FreeObjList ();
extern int SwapElements ();
extern struct ele_rec **MakeLineList ();
extern char *ParseMarkTag ();
extern char *MaxTextWidth ();
extern char *IsMapForm ();
extern char *DelayedHRef ();
extern int IsDelayedHRef ();
extern int AnchoredHeight ();
extern struct mark_up *HTMLParse ();
extern ImageInfo *NoImageData ();
extern Pixmap InfoToImage ();
extern void clean_white_space ();
extern void WidgetRefresh ();
extern void MoveWidget (/*WidgetInfo *wp, x, y*/);
extern WidgetInfo *MakeWidget ();
extern XFontStruct *GetWidgetFont ();
extern void AddNewForm ();
extern void FreeOneForm ();
extern void PrepareFormEnd ();
extern char *ComposeCommaList ();
extern void FreeCommaList ();

/*
 * To allow arbitrary nesting of lists
 */
typedef struct dtype_rec
{
  int type;			/* D_NONE, D_TITLE, D_TEXT, D_OLIST, D_ULIST */
  int count;
  int compact;
  struct dtype_rec *next;
}
DescRec;


/*
 * To allow arbitrary nesting of font changes
 */
typedef struct font_rec
{
  XFontStruct *font;
  struct font_rec *next;
}
FontRec;

static DescRec BaseDesc;
static DescRec *DescType;
static DescRec *ListData;
static FontRec FontBase;
static FontRec *FontStack;
static XFontStruct *currentFont;
static XFontStruct *saveFont;
static unsigned long Fg;
static unsigned long Bg;
static int Width;
static int MaxWidth;		/* actually, max. X position used */
static int ElementId;
static int WidgetId;
static int LineNumber;
static int LineHeight;		/* max ascent + max descent around BaseLine */
static int LineBottom;		/* BaseLine + max descent below BaseLine */
static int BaseLine;		/* Y offset of text baseline on this line */
static int TextIndent;
static int MarginW;
static int Special;
static int Preformat;
static int PF_LF_State;		/* Pre-formatted linefeed state.  Hack for bad HTMLs */
static int NeedSpace;
static Boolean DashedUnderlines;
static Boolean Strikeout;
static Boolean EmptyRegion;	/* see AddFElement, LineFeed */
static int Underlines;
static int CharsInLine;
static int IndentLevel;
static struct ele_rec *Current;
static char *AnchorText;
static char *SpecialsText;
static char *TextAreaBuf;
static struct mark_up *Last;
static FormInfo *CurrentForm;
static SelectInfo *CurrentSelect;
static struct ele_rec *html_error_line_elements;

/* variable size structure; current size indicated by maxcols */
typedef struct tbl_info {
    struct tbl_info *next, *prev;
    int TextIndent, oldWidth;	/* saved and restored state */
    int outerCellMaxX;
    int cellpadding, Xmax, Ymax, tbl_x0, rows_y0;
    enum { TBL_OPEN, TBL_ROW_OPEN, TBL_ITEM_OPEN } state;
    struct ele_rec *head;	/* head->next = first element in table */
    unsigned short curcol, nextcol, ncols, spans, maxcols;
    char *caption;		/* set if <caption>text</caption> seen */
# define INITIAL_TBL_COLS 32
    int col_x0[INITIAL_TBL_COLS+1];  /* saved column starting points */
    /* col_x0 is a variable length array; XtRealloc used to expand */
 } TBL_INFO;
static TBL_INFO *CurrentTable;
static unsigned short CurrentTblColumn;
static int TableDepth;		/* diagnostic counter */
static struct ele_rec *PrevContextEnd;
static int CellMaxX;
#define UNCERTAIN_X -20000	/* set x from prev element's x+width */

/*
 * Turned out we were taking WAY too much time mallocing and freeing
 * memory when composing the lines into elements.  So this inelegant
 * method minimizes all that.
 */
#define COMP_LINE_BUF_LEN	1024
static char *CompLine = NULL;
static int CompLineLen = 0;
static char *CompWord = NULL;
static int CompWordLen = 0;

/*
 * HaveRealBaseLine
 *
 * If BaseLine is still -100 ("not set yet"), set it and return False.
 * If it is already set, return True.
 */

static Boolean
HaveRealBaseLine (int baseline)
{
  if (BaseLine >= 0) return True;  /* already set to something real */

  BaseLine = baseline;
  if (LineBottom == 0)
  {
    LineBottom = LineHeight - baseline;
  }
  else
  {
    /*
     * It is possible (with the first item in a line being a
     * top aligned image) for LineBottom to have already been set.
     * It now needs to be corrected as we set BaseLine for real.
     */
    LineBottom = Max(LineHeight, LineBottom) - baseline;
  }

  return False;
}


/*
 * Add a format descriptor of the specified type to the logical end of the
 * formatted element linked list.
 * Sets 'Current' to point to the new element.
 * Type-specific as well as type-independent initialization is done.
 */
static void
AddFElement (hw, type, fp, x, y, edata)
HTMLWidget hw;
int type;
XFontStruct *fp;
int x, y;
char *edata;
{
  struct ele_rec *eptr;
  char *saved_edata;
  int len, saved_edata_len;
  int baseline = (fp != NULL) ?  fp->max_bounds.ascent : LineHeight;

  EmptyRegion = False;		/* there's about to be at least 1 element */

/* if (type == E_TEXT)  printf (" %d/[%d+%d,%d]'%s'%p ", LineNumber, y,LineHeight,x, edata, fp); else printf (" %d/[%d,%d]%c ", LineNumber, y,x, "0tbLiWh"[type]); /*DEBUG*/

  /*
   * If the linked list of elements is empty, or if we've reached
   * the "physical" end of the list, we have to create a new element and
   * add it to the tail of the list.  If the "logical" end (as defined
   * by Current) precedes the "physical" end (as defined by ->next), then
   * recycle one of the existing elements instead.  Doing that may also
   * allow recycling of previously allocated string space.
   */
  if (hw->html.formatted_elements == NULL  ||
      (Current != NULL  &&  Current->next == NULL))
  {				/* no old ones that could be recycled */
    eptr = (struct ele_rec *) XtMalloc (sizeof(struct ele_rec));
    if (eptr == NULL)
    {
      fprintf (stderr, "AddFElement: memory alloc failed\n");
      exit (1);
    }
    Current = AddEle (&(hw->html.formatted_elements), Current, eptr);
    saved_edata = NULL;
    saved_edata_len = 0;
  }
  else				/* reuse an existing element */
  {
				/* both these just tested non-NULL */
    eptr = (Current == NULL) ? hw->html.formatted_elements : Current->next;
    Current = eptr;

    saved_edata = eptr->edata;
    saved_edata_len = eptr->edata_len;
    if (eptr->anchorHRef != NULL)  XtFree((char *) eptr->anchorHRef);
    if (eptr->anchorName != NULL)  XtFree((char *) eptr->anchorName);
    FreeIMGInfo (eptr->img_data);  /* accepts NULL */
  }

  eptr->type = type;
  eptr->img_data = NULL;
  eptr->widget_data = NULL;
  eptr->font = fp;
  eptr->alignment = ALIGN_BOTTOM;
  eptr->selected = False;
  eptr->internal = False;	/* unused mechanism, but possibly useful */
  eptr->strikeout = Strikeout;
  eptr->x = x;
  eptr->y = y;
  eptr->tblcol = CurrentTblColumn;
  eptr->y_offset = 0;
  eptr->width = 0;
  eptr->line_number = LineNumber;
  eptr->line_height = LineHeight;
  eptr->fg = Fg;
  eptr->bg = Bg;
  eptr->underline_number = Underlines;
  eptr->dashed_underline = DashedUnderlines;
  eptr->indent_level = IndentLevel;
  eptr->anchorHRef = NULL;
  eptr->anchorName = NULL;
  eptr->edata = NULL;
  eptr->edata_len = 0;

  /* saved_edata will be freed at the bottom if it's still non-NULL */

  switch (type)
  {
  case E_TEXT:
    eptr->ele_id = ++ElementId;	 /* get a unique element ID */

    len = strlen (edata) + 1;
    if (saved_edata != NULL  &&  len <= saved_edata_len)
    {
      strcpy (saved_edata, edata);
      eptr->edata = saved_edata;
      saved_edata = NULL;	/* so it won't get freed below */
    }
    else
    {
      eptr->edata = XtNewString (edata);
    }
    eptr->edata_len = len;

    /*
     * if this is an anchor, puts its href and name values into the element.
     */
    if (AnchorText != NULL)
    {
      eptr->anchorHRef = ParseMarkTag (AnchorText, MT_ANCHOR, AT_HREF);
      eptr->anchorName = ParseMarkTag (AnchorText, MT_ANCHOR, AT_NAME);
    }
    break;

  case E_BULLET:
    eptr->ele_id = ElementId;
    eptr->underline_number = 0;	 /* Bullets can't be underlined! */

    if (HaveRealBaseLine (baseline)  &&  baseline < BaseLine)
    {
      eptr->y_offset = BaseLine - baseline;
    }
    break;

  case E_HRULE:
    eptr->ele_id = ++ElementId;	 /* get a unique element ID */
    eptr->underline_number = 0;	 /* HRules can't be underlined! */

    if (HaveRealBaseLine (baseline)  &&  baseline < BaseLine)
    {
      eptr->y_offset = BaseLine - baseline;
    }
    break;

  case E_LINEFEED:
    eptr->ele_id = ElementId;
    eptr->underline_number = 0;	 /* Linefeeds can't be underlined! */

    /* BaseLine and LineBottom were set before AddFElement called */

    /*
     * Linefeeds have to use at least the maximum line height.
     * Deal with bad Lucida descents.
     */
#ifdef NO_EXTRA_FILLS
    eptr->line_height = eptr->font->ascent + eptr->font->descent;
#else
    eptr->line_height = LineHeight;
#endif /* NO_EXTRA_FILLS */

    if (eptr->line_height < BaseLine + LineBottom)
    {
      eptr->line_height = BaseLine + LineBottom;
    }

    /*
     * If this linefeed is part of a broken anchor put
     * its href and name values into the element
     * so we can reconnect it when activated.
     * If it's at the beginning of an anchor, don't put
     * the href in and change the color back.
     */
    if (AnchorText != NULL)
    {
      char *tptr;

      tptr = ParseMarkTag (AnchorText, MT_ANCHOR, AT_HREF);
      if (eptr->prev != NULL
	  && (eptr->prev->anchorHRef == NULL  ||
	      tptr == NULL  ||
	      strcasecmp (eptr->prev->anchorHRef, tptr) != 0 ) )
      {
	if (tptr) XtFree(tptr);
	eptr->anchorHRef = NULL;
	eptr->anchorName = NULL;
	eptr->fg = hw->html.foreground;
      }
      else
      {
	eptr->anchorHRef = tptr;
	eptr->anchorName = ParseMarkTag (AnchorText, MT_ANCHOR, AT_NAME);
      }
    }
    break;

  case E_IMAGE:
    eptr->ele_id = ++ElementId;	 /* get a unique element ID */
    eptr->underline_number = 0;	 /* Images can't be underlined! */

    if (edata != NULL)
    {
      len = strlen (edata) + 1;
      if (saved_edata != NULL  &&  len <= saved_edata_len)
      {
	strcpy (saved_edata, edata);
	eptr->edata = saved_edata;
	saved_edata = NULL;	/* so it won't get freed below */
      }
      else
      {
	eptr->edata = XtNewString (edata);
      }
      eptr->edata_len = len;
    }

    /*
     * If this image is part of an anchor put its href and name values into
     * the element so we can reconnect it when activated.
     * Must precede call to HTMLGetImage.
     */
    if (AnchorText != NULL)
    {
      eptr->anchorHRef = ParseMarkTag (AnchorText, MT_ANCHOR, AT_HREF);
      eptr->anchorName = ParseMarkTag (AnchorText, MT_ANCHOR, AT_NAME);
    }

    HTMLGetImage (hw, eptr, 0);	 /* sets img_data & pic_data non-NULL */
    break;

  case E_WIDGET:
    eptr->ele_id = ++ElementId;	 /* get a unique element ID */
    eptr->underline_number = 0;	 /* Widgets can't be underlined! */
    WidgetId++;

    /*
     * If this widget is part of an anchor put
     * its href and name values into the element
     * so we can reconnect it when activated.
     */
    if (AnchorText != NULL)
    {
      eptr->anchorHRef = ParseMarkTag (AnchorText, MT_ANCHOR, AT_HREF);
      eptr->anchorName = ParseMarkTag (AnchorText, MT_ANCHOR, AT_NAME);
    }

    /*
     * Widget stuff
     */
    eptr->widget_data = MakeWidget (hw, edata,
				    (x + IMAGE_BORDER), (y + IMAGE_BORDER),
				    WidgetId, CurrentForm);

    /*
     * I have no idea what to do if we can't create the
     * widget.  It probably means we are so messed up we
     * will soon be crashing.
     */
    if (eptr->widget_data == NULL)
    {
      fprintf (stderr, "AddFElement: Could not make widget.\n");
    }

    break;

  default:
    fprintf (stderr, "AddFElement:  Unknown type %d\n", type);
    eptr->ele_id = ElementId;
    break;
  }

  if (saved_edata != NULL)   XtFree ((char *) saved_edata);
}



/*
 * Change our drawing font 
 */
void
NewFont (fp)
XFontStruct *fp;
{
  /*
   * Deal with bad Lucida descents.
   */
  if (fp->descent > fp->max_bounds.descent)
  {
    LineHeight = fp->max_bounds.ascent + fp->descent;
  }
  else
  {
    LineHeight = fp->max_bounds.ascent + fp->max_bounds.descent;
  }
}


/*
 * Place a linefeed at the end of a line.
 * Create and add the element record for it.
 */
static void
LinefeedPlace (hw, x, y)
HTMLWidget hw;
int *x, *y;
{
  /*
   * At the end of every line check if we have a new MaxWidth
   */
  if (CurrentTable)
  {
    if (CellMaxX < *x)   CellMaxX = *x;
  }
  else
  {
    if (MaxWidth < (*x + hw->html.margin_width))
    {
      MaxWidth = *x + hw->html.margin_width;
    }
  }

  /*
   * make LFs have a minimum height of LineHeight
   */
  if (BaseLine < 0)   BaseLine = 0;

  if (BaseLine + LineBottom < LineHeight)
  {
    LineBottom = LineHeight - BaseLine;
  }

  AddFElement (hw, E_LINEFEED, currentFont, *x, *y, (char *) NULL);

  if (CurrentTable)
  {
    struct ele_rec *tptr = Current->prev;
    Current->width = - CurrentTable->nextcol;  /* marker; see Table_Close */
    /* if previous item is HRule and in same context, float Current->x */
    if (tptr != NULL  &&  tptr != PrevContextEnd  &&
	tptr->type == E_HRULE  &&  tptr->width == Current->width)
    {
      Current->x = UNCERTAIN_X;
    }
  }
  /* (else) non-table LFs have width=0, meaning "infinitely far right" */
/* printf (" LF@[%d,%d-%d] ", *y, *x, *x + Current->width); /*DEBUG*/

  *x = TextIndent;
  *y = *y + BaseLine + LineBottom;

  CharsInLine = 0;
  NeedSpace = 0;
  LineBottom = 0;
  BaseLine = -100;
  PrevContextEnd = Current;

  if (!CurrentTable)   LineNumber++;
}


/*
 * We have encountered a line break.  Increment the line counter,
 * and move down some space.
 */
void
LineFeed (hw, x, y)
HTMLWidget hw;
int *x, *y;
{
  /*
   * Manipulate linefeed state for special pre-formatted linefeed
   * hack for broken HTMLs
   */
  if (Preformat)
  {
    switch (PF_LF_State)
    {
      /*
       * First soft linefeed
       */
    case 0:
      PF_LF_State = 1;
      break;
      /*
       * Collapse multiple soft linefeeds within a pre
       */
    case 1:
      return;
      break;
      /*
       * Ignore soft linefeeds after hard linefeeds
       * within a pre
       */
    case 2:
      return;
      break;
    default:
      PF_LF_State = 1;
      break;
    }
  }
  /*
   * For formatted documents there are 3 linefeed states.
   * 0 = in the middle of a line.
   * 1 = at left margin
   * 2 = at left margin with blank line above
   */
  else
  {
    if (++PF_LF_State > 2)   PF_LF_State = 2;
  }

  /*
   * No blank lines allowed at the start of a region.
   */
  if (!EmptyRegion  &&
      (CurrentTable == NULL  ||  CurrentTable->state == TBL_ITEM_OPEN))
  {
    LinefeedPlace (hw, x, y);
  }
}


/*
 * We want to make sure that future text starts at the left margin.
 * But if we are already there, don't put in a new line.
 */
void
ConditionalLineFeed (hw, x, y, state)
HTMLWidget hw;
int *x, *y;
int state;
{
  if (PF_LF_State < state)
  {
    /*
     * If this funtion is being used to insert a blank line,
     * we need to look at the percentVerticalSpace resource
     * to see how high to make the line.
     */
    if ((state == 2) && (hw->html.percent_vert_space > 0))
    {
      int l_height;

      l_height = LineHeight;
      LineHeight = LineHeight * hw->html.percent_vert_space / 100;
      LineFeed (hw, x, y);
      LineHeight = l_height;
    }
    else
    {
      LineFeed (hw, x, y);
    }
  }
}


/*
 * Hack to make broken HTMLs within pre-formatted text have nice
 * looking linefeeds.
 */
void
HardLineFeed (hw, x, y)
HTMLWidget hw;
int *x, *y;
{
  /*
   * Manipulate linefeed state for special pre-formatted linefeed
   * hack for broken HTMLs
   */
  if (Preformat)
  {
    switch (PF_LF_State)
    {
      /*
       * First hard linefeed
       */
    case 0:
      PF_LF_State = 2;
      break;
      /*
       * Previous soft linefeed should have been ignored, so
       * ignore this hard linefeed, but set state like it
       * was not ignored.
       */
    case 1:
      PF_LF_State = 2;
      return;
      break;
      /*
       * Honor multiple hard linefeeds.
       */
    case 2:
      break;
    default:
      PF_LF_State = 2;
      break;
    }
  }
  /*
   * No blank lines allowed at the start of a region.
   */
  if (!EmptyRegion  &&
      (CurrentTable == NULL  ||  CurrentTable->state == TBL_ITEM_OPEN))
  {
    LinefeedPlace (hw, x, y);
  }
}


static void
AdjustBaseLine ()
{
  int baseline = Current->font->max_bounds.ascent;

/* printf (" ABL baseline= %d, Baseline= %d, LB= %d, LH= %d\n", baseline, BaseLine, LineBottom, LineHeight); /*DEBUG*/

  if (!HaveRealBaseLine (baseline))
  {
    /* this section intentionally left blank */
  }
  else if (baseline <= BaseLine)
  {
    Current->y_offset = BaseLine - baseline;

    if (LineBottom < LineHeight - baseline)
    {
      LineBottom = LineHeight - baseline;
    }
  }
  else
  {
    struct ele_rec *eptr;
    int incy;

    incy = baseline - BaseLine;
    BaseLine = baseline;

    /*
     * Go back over this line and move everything down a little.
     */
    for (eptr = Current->prev;  eptr != PrevContextEnd;  eptr = eptr->prev)
    {
      eptr->y_offset += incy;
    }

    if (LineBottom < LineHeight - baseline)
    {
      LineBottom = LineHeight - baseline;
    }
  }
}


/*
 * Place the bullet at the beginning of an unnumbered
 * list item.  Create and add the element record for it.
 */
void
BulletPlace (hw, x, y)
HTMLWidget hw;
int *x, *y;
{
  int width = hw->html.font->max_bounds.lbearing
            + hw->html.font->max_bounds.rbearing;

  /*
   * Save the font's line height, and set your own for this
   * element.  Restore the fonts height when done.
   * Deal with bad Lucida descents.
   */
  int savedLineHeight = LineHeight;

  LineHeight = hw->html.font->max_bounds.ascent
      + Max (hw->html.font->descent, hw->html.font->max_bounds.descent);

  /*
   * For historical reasons, bullets aren't between *x and *x + width
   * like every other object, but at *x - width to *x.  As a result, the
   * usual  *x += width  isn't needed.  However, set
   * Current->x = *x - width, and Current->width = width so that the
   * refresh, locate, and other code can properly locate the bullet.
   */
   
  AddFElement (hw, E_BULLET, hw->html.font, *x - width, *y, (char *) NULL);

  LineHeight = savedLineHeight;

  Current->width = width;

  /*
   * This should really be here, but it is a hack for headers on list
   * elements to work if we leave it out
   PF_LF_State = 0;
   */

  /*
   * Bullets are drawn from *x - width to *x - width/2, leaving an "en"s
   * worth of space between it and whatever follows, so there's no need
   * to add yet more space.
   */
  NeedSpace = 0;
}


/*
 * Place a horizontal rule across the page or table entry.
 * Create and add the element record for it.
 */
void
HRulePlace (hw, x, y, width)
HTMLWidget hw;
int *x, *y;
unsigned int width;
{
  *x = (CurrentTable == NULL) ?  hw->html.margin_width : TextIndent;
  AddFElement (hw, E_HRULE, currentFont, *x, *y, (char *) NULL);
  if (CurrentTable == NULL)
  {
    Current->width = width - 2 * hw->html.margin_width;
    if (Current->width < 0)   Current->width = 0;
    *x += Current->width;
  }
  else
  {
    *x = CurrentTable->col_x0[CurrentTable->nextcol]  /* may be -1 */
	 - CurrentTable->cellpadding - 1;
    if (*x < CellMaxX)   *x = CellMaxX;
    Current->width = - CurrentTable->nextcol;  /* marker; see Table_Close */
  }
  NeedSpace = 1;		/* in case we ever do <hr width= > */
  PF_LF_State = 0;
}


/*
 * Place the number at the beginning of a numbered list item.
 * Create and add the element record for it.
 */
void
ListNumberPlace (hw, x, y, val)
HTMLWidget hw;
int *x, *y;
int val;
{
  int width, my_x;
  int dir, ascent, descent;
  XCharStruct all;
  char buf[20];

  sprintf (buf, "%d.", val);

  width = hw->html.font->max_bounds.lbearing
      + hw->html.font->max_bounds.rbearing;

  XTextExtents (currentFont, buf, strlen (buf), &dir, &ascent, &descent, &all);
  my_x = *x - (width / 2) - all.width;

  /*
   * Add a space after the list number here rather than using NeedSpace
   */
  width = strlen (buf);
  buf[width] = ' ';
  buf[width + 1] = '\0';

  AddFElement (hw, E_TEXT, currentFont, my_x, *y, buf);
  AdjustBaseLine ();
  CharsInLine = CharsInLine + strlen (buf);

  NeedSpace = 0;		/* because the space was added above */
  /*
   * This should really be here, but it is a hack for headers on list
   * elements to work if we leave it out
   PF_LF_State = 0;
   */
}


/*
 * Place a piece of pre-formatted text. Add an element record for it.
 */
static void
PreformatPlace (hw, text, x, y, width)
HTMLWidget hw;
char *text;			/* not const; some whitespace changed to 040 */
int *x, *y;
unsigned int width;
{
  const char *start;
  const char *end;
  char *ptr;
  char tchar;
  int tab_count, char_cnt;
  int dir, ascent, descent;
  XCharStruct all;
  char *line;
  int line_x;

  line_x = *x;
  line = CompLine;
  if (line != NULL)
  {
    line[0] = '\0';
  }
  end = text;
  while (*end != '\0')
  {
    tab_count = 0;
    char_cnt = CharsInLine;
    /*
     * make start and end point to one word.  A word is either
     * a lone linefeed, or all whitespace before a word, plus
     * the text of the word itself.
     */
    start = end;
    /*
     * Throw out carriage returns and form-feeds
     */
    if ((*end == '\r') || (*end == '\f'))
    {
      start++;
      end++;
    }
    else if (*end == '\n')
    {
      end++;
      char_cnt++;
    }
    else
    {
      /*
       * Should be only spaces and tabs here, so if it
       * is not a tab, make it a space.
       * Break on linefeeds, they must be done separately
       */
      while ((unsigned) *end < 128  &&  isspace (*end))
      {
	if (*end == '\n')
	{
	  break;
	}
	else if (*end == '\t')
	{
	  tab_count++;
	  char_cnt = ((char_cnt / 8) + 1) * 8;
	}
	else
	{
	  *((char *)end) = ' ';	 /* alters the argument 'text'! */
	  char_cnt++;
	}
	end++;
      }
      while ((unsigned) *end > 127  ||  (!isspace (*end)  &&  *end != '\0'))
      {
	end++;
	char_cnt++;
      }
    }

    /*
     * Add the word to the end of this line, or insert
     * a linefeed if the word is a lone linefeed.
     * Tabs expand to 8 spaces.
     */
    if (start != end)
    {
      int tlen = char_cnt + 1;

      if (tlen > CompWordLen)
      {
	CompWordLen += COMP_LINE_BUF_LEN;
	if (tlen > CompWordLen) CompWordLen = tlen;
	if (CompWord != NULL) XtFree(CompWord);
	CompWord = (char *)XtMalloc(CompWordLen);
      }
      ptr = CompWord;

      /*
       * If we have any tabs, expand them into spaces.
       */
      if (tab_count)
      {
	char *p1;
	const char *p2;
	int i, new;

	char_cnt = CharsInLine;
	p1 = ptr;
	p2 = start;
	while (p2 != end)
	{
	  if (*p2 == '\t')
	  {
	    new = ((char_cnt / 8) + 1) * 8;
	    for (i = 0; i < (new - char_cnt); i++)
	    {
	      *p1++ = ' ';
	    }
	    p2++;
	    char_cnt = new;
	  }
	  else
	  {
	    *p1++ = *p2++;
	    char_cnt++;
	  }
	}
	*p1 = '\0';
      }
      else
      {
	*ptr = '\0';
	strncat (ptr, start, end - start);
      }

#ifdef ASSUME_FIXED_WIDTH_PRE
      all.width = currentFont->max_bounds.width * strlen (ptr);
#else
      XTextExtents (currentFont, ptr, strlen (ptr), &dir,
		    &ascent, &descent, &all);
#endif /* ASSUME_FIXED_WIDTH_PRE */

      if (*start == '\n')
      {
	if ((line != NULL) && (line[0] != '\0'))
	{
	  AddFElement (hw, E_TEXT, currentFont, line_x, *y, line);
	  /*
	   * Save width here to avoid an XTextExtents call later.
	   */
	  Current->width = *x - line_x + 1;

	  AdjustBaseLine ();
	  PF_LF_State = 0;

	  line[0] = '\0';
	}

	HardLineFeed (hw, x, y);
	line_x = *x;
	/*NeedSpace = 0;	/* done by HardLineFeed */
      }
      else
      {
	char *tptr;
	int tlen;

	if (line == NULL)
	{
	  tlen = strlen (ptr) + 1;
	}
	else
	{
	  tlen = strlen (line) + strlen (ptr) + 1;
	}
	if (tlen > CompLineLen)
	{
	  CompLineLen += COMP_LINE_BUF_LEN;
	  if (tlen > CompLineLen)
	  {
	    CompLineLen = tlen;
	  }
	  tptr = (char *)XtMalloc(CompLineLen);
	  if (CompLine != NULL)
	  {
	    strcpy (tptr, CompLine);
	    XtFree(CompLine);
	  }
	  else
	  {
	    tptr[0] = '\0';
	  }
	  CompLine = tptr;
	}
	line = CompLine;

	strcat (line, ptr);

	*x = *x + all.width;
	CharsInLine = CharsInLine + strlen (ptr);
	NeedSpace = 1;
      }
    }
  }
  if ((line != NULL) && (line[0] != '\0'))
  {
    AddFElement (hw, E_TEXT, currentFont, line_x, *y, line);
    /*
     * Save width here to avoid an XTextExtents call later.
     */
    Current->width = *x - line_x + 1;

    AdjustBaseLine ();
    PF_LF_State = 0;
    line[0] = '\0';
  }
}


/*
 * IfNeedSpaceDo
 *
 * Inserts NeedSpace spaces by adding a new text element.
 * Any spaces inserted will NOT be part of an anchor.
 */

static void
IfNeedSpaceDo (hw, x, y)
HTMLWidget hw;
int *x, *y;
{
  if (!Preformat  &&  NeedSpace < 0)
  {
    int dir, ascent, descent;
    XCharStruct all;
    char *const realAnchorText = AnchorText;
    char *spc = (NeedSpace == -2) ?  "  " : " ";

    XTextExtents (currentFont, spc, strlen(spc), &dir,
		  &ascent, &descent, &all);

    AnchorText = NULL;
    AddFElement (hw, E_TEXT, currentFont, *x, *y, spc);
    AnchorText = realAnchorText;

    Current->underline_number = 0;  /* don't underline these spaces */
    Current->width = all.width;

    AdjustBaseLine ();
    *x += all.width;
    CharsInLine = CharsInLine + strlen (spc);
    PF_LF_State = 0;
    NeedSpace = 0;
  }
}


/*
 * Format and place a piece of text. Add an element record for it.
 */
static void
FormatPlace (hw, text, x, y, xmax)
HTMLWidget hw;
const char *text;
int *x, *y;
unsigned int xmax;
{
  const char *start, *end;
#ifdef DOUBLE_SPACE_AFTER_PUNCT
  char tchar2;
#endif /* DOUBLE_SPACE_AFTER_PUNCT */
  int stripped_space;
  int added_space;
  int double_space;
  int dir, ascent, descent;
  XCharStruct all;
  char *line;
  int line_x0;
  int marginw = (CurrentTable != NULL) ?  0 : MarginW;

  line_x0 = *x;
  line = CompLine;
  if (line != NULL)
  {
    line[0] = '\0';
  }
  end = text;
  while (*end != '\0')
  {
    /*
     * Make start and end point to one word.
     * Set flag if we removed any leading white space.
     * Set flag if we add any leading white space.
     */
    stripped_space = 0;
    added_space = 0;
    start = end;
    /*
     * the only reason I can see for the  < 128  test is
     * to make \240 work for &nbsp.  (WBE)
     */
    while (*start != '\0'  &&  isspace (*start)  &&  (unsigned) *start < 128)
    {
      start++;
    }
    stripped_space = (start != end);
    if (stripped_space  &&  NeedSpace > 0)  NeedSpace = -NeedSpace;

    end = start;
    while ((unsigned) *end > 127  ||  *end != '\0'  &&  !isspace (*end))
    {
      end++;
    }

    /*
     * Add the word to the end of this line, or insert
     * a linefeed and put the word at the start of the next line.
     */
    if (start != end)
    {
      int nobreak;
      int tlen;
      char *ptr;

      /*
       * nobreak is a horrible hack that specifies special
       * conditions where line breaks are just not allowed
       */
      nobreak = 0;

      /*
       * Malloc temp space if needed, leave room for
       * 2 spaces and an end of string char
       */
      tlen = end - start + 3;
      if (tlen > CompWordLen)
      {
	CompWordLen += COMP_LINE_BUF_LEN;
	if (tlen > CompWordLen) CompWordLen = tlen;
	if (CompWord != NULL) XtFree(CompWord);
	CompWord = (char *)XtMalloc(CompWordLen);
      }
      ptr = CompWord;

      if (NeedSpace < 0  ||  (NeedSpace > 0  &&  Current->type != E_TEXT))
      {
	if (NeedSpace == 2  ||  NeedSpace == -2)
	{
	  strcpy (ptr, "  ");	/* put spaces at start of composing area */
	  added_space = 2;
	}
	else
	{
	  strcpy (ptr, " ");	/* put spaces at start of composing area */
	  added_space = 1;
	}
      }
      else
      {
	strcpy (ptr, "");
      }
      strncat (ptr, start, end - start);  /* append non-whitespace string */

#ifdef DOUBLE_SPACE_AFTER_PUNCT
      /*
       * If this text ends in '.', '!', or '?' we need
       * to set up the addition of two spaces after it.
       */
      tchar2 = ptr[strlen (ptr) - 1];
      double_space = (tchar2 == '.') || (tchar2 == '!') || (tchar2 == '?');
#else
      double_space = 0;
#endif /* DOUBLE_SPACE_AFTER_PUNCT */

      /*
       * Horrible hack for punctuation following
       * font changes to not go on the next line.
       */
      if ((MY_ISPUNCT (*ptr)) && (added_space == 0))
      {
	char *tptr;

	/*
	 * Take into account whole streams of punctuation.
	 */
	nobreak = 1;
	tptr = ptr;
	while ((*tptr != '\0') && (MY_ISPUNCT (*tptr)))
	{
	  tptr++;
	}
	if (*tptr != '\0')
	{
	  nobreak = 0;
	}
      }

      /*
       * No linebreaks if this whole line is just too long.
       */
      if (*x == TextIndent)
      {
	nobreak = 1;
      }

      XTextExtents (currentFont, ptr, strlen (ptr), &dir,
		    &ascent, &descent, &all);

      if (nobreak  ||  (*x + all.width + marginw <= xmax))
      {
	char *tptr;
	int tlen;

	/*
	 * If we do nothing, any added leading spaces will have the same
	 * anchorHRef as the text after them because AnchorText is already
	 * set.  If the spaces are going to be between two parts of the
	 * same anchor, fine.  Otherwise put the spaces in a separate text
	 * element so that they won't be underlined or highlighted.
	 */
	if (added_space  &&  AnchorText != NULL  &&
	    (line == NULL  ||  line[0] == '\0') )
	{
	  char *tptr = ParseMarkTag (AnchorText, MT_ANCHOR, AT_HREF);

	  if (Current == NULL  ||  Current->anchorHRef == NULL  ||
	      tptr == NULL  ||  strcmp (tptr, Current->anchorHRef) != 0)
	  {
	    IfNeedSpaceDo (hw, x, y);
	    ptr += added_space;	 /* flush leading spaces from to-add string */
	    added_space = 0;	/* no longer adding spaces here */
	    line_x0 = *x;	/* fix origin of remaining line */
	    XTextExtents (currentFont, ptr, strlen (ptr), &dir,
			  &ascent, &descent, &all);  /* recompute width */
	  }
	  if (tptr != NULL) XtFree (tptr);
	}

	if (line == NULL)
	{
	  tlen = strlen (ptr) + 1;
	}
	else
	{
	  tlen = strlen (line) + strlen (ptr) + 1;
	}
	if (tlen > CompLineLen)
	{
	  CompLineLen += COMP_LINE_BUF_LEN;
	  if (tlen > CompLineLen)
	  {
	    CompLineLen = tlen;
	  }
	  tptr = (char *)XtMalloc(CompLineLen);
	  if (CompLine != NULL)
	  {
	    strcpy (tptr, CompLine);
	    XtFree(CompLine);
	  }
	  else
	  {
	    tptr[0] = '\0';
	  }
	  CompLine = tptr;
	}
	line = CompLine;

	strcat (line, ptr);
      }
      else
      {
	char *tptr;
	int tlen;

	/*
	 * Push out old line, if any
	 */
	if (line != NULL  &&  line[0] != '\0')
	{
	  AddFElement (hw, E_TEXT, currentFont, line_x0, *y, line);
	  /*
	   * Save width here to avoid an XTextExtents call later.
	   */
	  Current->width = *x - line_x0 + 1;

	  AdjustBaseLine ();
	  PF_LF_State = 0;

	  line[0] = '\0';
	}

	/*
	 * Add a LF after the old line and start composing another line
	 */
	LineFeed (hw, x, y);
	line_x0 = *x;

	/*
	 * If we added leading spaces, remove them
	 * since we are at the beginning of a new line
	 */
	ptr += added_space;

	XTextExtents (currentFont, ptr, strlen(ptr), &dir,
		      &ascent, &descent, &all);	 /* *x += all.width (below) */

	if (line == NULL)
	{
	  tlen = strlen (ptr) + 1;
	}
	else
	{
	  tlen = strlen (line) + strlen (ptr) + 1;
	}
	if (tlen > CompLineLen)
	{
	  CompLineLen += COMP_LINE_BUF_LEN;
	  if (tlen > CompLineLen)
	  {
	    CompLineLen = tlen;
	  }
	  tptr = (char *)XtMalloc(CompLineLen);
	  if (CompLine != NULL)
	  {
	    strcpy (tptr, CompLine);
	    XtFree(CompLine);
	  }
	  else
	  {
	    tptr[0] = '\0';
	  }
	  CompLine = tptr;
	}
	line = CompLine;

	strcat (line, ptr);	/* append new text to composition line */
      }

      /*
       * Set NeedSpace for one or 2 spaces based on
       * whether we are after a '.', '!', or '?'
       * or not.
       */
      NeedSpace = double_space ?  2 : 1;

      *x += all.width;
    }
    /*
     * No more words to add.
     * If there is trailing whitespace on a non-empty line, it's faster
     * to add it to the current element than to put it off and maybe have
     * to add another element.  This may cause some non-empty lines to
     * have spaces on the end.
     */
    else if (stripped_space  &&  line != NULL  &&  line[0] != '\0')
	/* code below relies on the above line != NULL test */
    {
      char *tptr;
      char *spc;
      int tlen;

      spc = (NeedSpace == -2) ?  "  " : " ";  /* stripped_space ==> <0 */

      XTextExtents (currentFont, spc, strlen (spc), &dir,
		    &ascent, &descent, &all);  /* for *x += all.width below */

      /*
       * Will adding this space force a line break?
       */
      if ((*x + all.width + marginw) <= xmax)	/* no, so add it */
      {
	tlen = strlen (line) + strlen (spc) + 1;
	if (tlen > CompLineLen)
	{
	  CompLineLen += COMP_LINE_BUF_LEN;
	  if (tlen > CompLineLen)
	  {
	    CompLineLen = tlen;
	  }
	  tptr = (char *)XtMalloc(CompLineLen);
	  strcpy (tptr, CompLine);  /* CompLine MUST be !NULL if line !NULL */
	  XtFree(CompLine);
	  CompLine = tptr;
	}
	line = CompLine;

	strcat (line, spc);

	*x += all.width;
	NeedSpace = 0;
      }
      /*
       * Else adding this space would force a linefeed, so give up and
       * exit with NeedSpace still < 0 indicating want, and saw, space.
       * This will prevent text or punctuation following a font change
       * from getting concatenated or treated specially.
       */
    }

  }  /* end while *end != '\0' */

  if (line != NULL  &&  line[0] != '\0')
  {
    AddFElement (hw, E_TEXT, currentFont, line_x0, *y, line);
    /*
     * Save width here to avoid an XTextExtents call later.
     */
    Current->width = *x - line_x0 + 1;

    AdjustBaseLine ();
    PF_LF_State = 0;
    line[0] = '\0';
  }
}


/*
 * Place an image.  Add an element record for it.
 */
static void
ImagePlace (hw, taginfo, x, y, width)
HTMLWidget hw;
const char *taginfo;
int *x, *y;
unsigned int width;		/* = right hand margin */
{
  struct ele_rec *const savedCurrent = Current;
  char *tptr;
  char *spc;
  int dx, dy;			/* image width and height, incl. borders */
  int extra;
  int marginw = (CurrentTable != NULL) ?  0 : MarginW;

  /*
   * Note:  NeedSpace = 0 if Current->prev = PrevContextEnd
   */
  IfNeedSpaceDo (hw, x, y);

  /*
   * Place the image in order to find out its dimensions.
   * If it doesn't fit on the line, back up Current and start over.
   */

  tptr = ParseMarkTag (taginfo, MT_IMAGE, "SRC");
  AddFElement (hw, E_IMAGE, currentFont, *x, *y, tptr);
  /*
   * AddFElement promises pic_data will be something usable.
   */

  if (hw->html.border_images == True  ||  Current->anchorHRef != NULL)
  {
    extra = 2 * IMAGE_BORDER;
  }
  else
  {
    extra = 0;
  }

  dx = Current->img_data->pic_data->width + extra;
  dy = Current->img_data->pic_data->height + extra;

  /*
   * If the image is too wide for the line and
   * if this isn't the beginning of the local context,
   * start over, insert a LF, and place the image again.
   */
  if (!Preformat  &&
      *x + dx + marginw > width  &&
      Current->prev != PrevContextEnd)
  {
    Current = savedCurrent;	/* unplaces image and any added spaces */
    LineFeed (hw, x, y);
    AddFElement (hw, E_IMAGE, currentFont, *x, *y, tptr);
  }

  /*
   * Clean up parsed SRC string
   */
  if (tptr != NULL) XtFree(tptr);

  /*
   * Yank out the name field, and stick it in text.
   * We may use this for ALT to at some later date.
   */
  tptr = ParseMarkTag (taginfo, MT_IMAGE, "NAME");
  Current->img_data->text = tptr;

  /*
   * Check if this image has the ISMAP attribute, so we know the
   * x,y coordinates of the image click are important.
   * Due to a special case (see below), this code can actually
   * change the size, or anchor status of the image, thus we MUST
   * do it before we muck with the Baseline and stuff.
   */
  Current->img_data->fptr = NULL;
  tptr = ParseMarkTag (taginfo, MT_IMAGE, "ISMAP");
  if (tptr != NULL)
  {
    XtFree(tptr);
    Current->img_data->ismap = 1;
    /*
     * SUPER SPECIAL CASE!  (Thanks Marc)
     * If you have an ISMAP image inside a form,
     * and that form doesn't already have an HREF
     * by being inside an anchor,
     * [and being a DelayedHRef is considered no HRef]
     * then clicking in that image will submit the form,
     * adding the x,y coordinates of the click as part
     * of the list of name/value pairs.
     */
    if (CurrentForm != NULL  &&
	(Current->anchorHRef == NULL  ||
	 IsDelayedHRef (hw, Current->anchorHRef)))
    {
      Current->img_data->fptr = CurrentForm;
      Current->anchorHRef = IsMapForm (hw);
      Current->fg = hw->html.anchor_fg;
    }
  }
  else
  {
    Current->img_data->ismap = 0;
  }

  /*
   * Check if this image will be top aligned
   */
  tptr = ParseMarkTag (taginfo, MT_IMAGE, "ALIGN");
  if (tptr != NULL && strcasecmp (tptr, "TOP") == 0)
  {
    Current->alignment = ALIGN_TOP;
  }
  else if (tptr != NULL && strcasecmp (tptr, "MIDDLE") == 0)
  {
    Current->alignment = ALIGN_MIDDLE;
  }
  else
  {
    Current->alignment = ALIGN_BOTTOM;
  }
  /*
   * Clean up parsed ALIGN string
   */
  if (tptr != NULL)
  {
    XtFree(tptr);
    tptr = NULL;
  }

  /*
   * Advance x position, and check the max
   * line height.  We need to follow this
   * image with a space.
   */
  if (BaseLine == -100)   BaseLine = 0;

  *x += dx;

  if (Current->alignment == ALIGN_TOP)
  {
    /*Current->y_offset = 0;	/* already done by AddFElement */
  }
  else if (Current->alignment == ALIGN_MIDDLE)
  {
    int baseline = dy / 2;

    if (baseline <= BaseLine)
    {
      Current->y_offset = BaseLine - baseline;
/* printf (" [%d,%d]I(m): ý=%d (%d-%d) ", Current->y,Current->x, Current->y_offset, BaseLine, baseline); /*DEBUG*/
    }
    else
    {
      struct ele_rec *eptr;
      int incy;

      /*Current->y_offset = 0;	/* already done by AddFElement */

      incy = baseline - BaseLine;
      BaseLine = baseline;

      /*
       * Go back over this line and move everything down a little.
       */
      for (eptr = Current->prev;  eptr != PrevContextEnd;  eptr = eptr->prev)
      {
	eptr->y_offset += incy;
/* printf (" [%d,%d]I(m) set [%d,%d]ý+=%d ", Current->y,Current->x, eptr->y, eptr->x, incy); /*DEBUG*/
      }
    }
  }
  else			/* align == ALIGN_BOTTOM */
  {
    if (dy <= BaseLine)
    {
      Current->y_offset = BaseLine - dy;
/* printf (" [%d,%d]I(b): ý=%d (%d-%d) ", Current->y,Current->x, Current->y_offset, BaseLine, dy); /*DEBUG*/
    }
    else
    {
      struct ele_rec *eptr;
      int incy;

      incy = dy - BaseLine;
      BaseLine = dy;

      /*
       * Go back over this line and move everything down a little.
       */
      for (eptr = Current->prev;  eptr != PrevContextEnd;  eptr = eptr->prev)
      {
	eptr->y_offset += incy;
/* printf (" [%d,%d]I(b) set [%d,%d]ý+=%d ", Current->y,Current->x, eptr->y, eptr->x, incy); /*DEBUG*/
      }
    }
  }

  if (LineBottom < dy - BaseLine)   LineBottom = dy - BaseLine;

  if (BaseLine == 0)   BaseLine = -100;

  Current->width = dx;
  Current->line_height = dy;

  PF_LF_State = 0;
  NeedSpace = 1;
}


/*
 * Place a Widget.  Add an element record for it.
 */
static void
WidgetPlace (hw, taginfo, x, y, width)
HTMLWidget hw;
const char *taginfo;
int *x, *y;
unsigned int width;
{
  struct ele_rec *const savedCurrent = Current;
  int marginw = (CurrentTable != NULL) ?  0 : MarginW;

  /*
   * Note:  NeedSpace = 0 if Current->prev = PrevContextEnd
   */
  IfNeedSpaceDo (hw, x, y);

  /*
   * Place the widget in order to find out its dimensions.
   * If it doesn't fit on the line, back up Current and start over.
   */

  AddFElement (hw, E_WIDGET, currentFont, *x, *y, (char *)taginfo);

  if (Current->widget_data != NULL  &&  !Preformat)
  {
    int extra = 2 * IMAGE_BORDER;
    int dx = Current->widget_data->width + extra;

    /*
     * If the image is too wide for the line and
     * if this isn't the beginning of the local context,
     * start over, insert a LF, and place the image again.
     */
    if (*x + dx + marginw > width  &&
	Current->prev != PrevContextEnd)
    {
      Current = savedCurrent;	/* unplaces widget and any added spaces */
      LineFeed (hw, x, y);
      WidgetId--;
      AddFElement (hw, E_WIDGET, currentFont, *x, *y, (char *)taginfo);
    }
  }

  /*
   * Adjust x position, BaseLine, LineBottom, and y_offset appropriately.
   */
  if (Current->widget_data != NULL)
  {
    XFontStruct *fp;
    int baseline;

    int extra = 2 * IMAGE_BORDER;
    int dy = Current->widget_data->height + extra;

    /*
     * Find the font used in this widget.  Then find its baseline
     */
    fp = GetWidgetFont (hw, Current->widget_data);
    if (fp == NULL)
    {
      baseline = dy;
    }
    /*
     * If no font, the baseline is the bottom of the widget
     */
    else
    {
      int border = (dy - (fp->max_bounds.ascent + fp->max_bounds.descent));
      baseline = (border / 2) + fp->max_bounds.ascent;
    }

    if (BaseLine == -100)	/* the special unset baseline value */
    {
      BaseLine = baseline;
      /*
       * If linebottom isn't set (=0), set it to
       * whatever of the height is below the baseline.
       *
       * Else, it is possible that a linebottom has been
       * set even when we have no baseline yet (like if
       * the first item in the line was a top aligned image)
       * It now needs to be corrected as we set a real BaseLine.
       */
      LineBottom = Max(dy, LineBottom) - baseline;
    }
    /*
     * Else we already have a baseline, and it is greater that
     * the baseline for this widget.
     */
    else if (baseline <= BaseLine)
    {
      Current->y_offset = BaseLine - baseline;

      /*
       * Our line bottom may be greater than the old one.
       */
      if (LineBottom < dy - baseline)   LineBottom = dy - baseline;
    }
    else
      /*
       * Else we have a new baseline greater than the old baseline.
       */
    {
      struct ele_rec *eptr;
      int incy;

      /*
       * Figure out how much to move all the old stuff
       */
      incy = baseline - BaseLine;
      BaseLine = baseline;

      /*
       * Go back over this line and move everything down a little.
       */
      for (eptr = Current->prev;  eptr != PrevContextEnd;  eptr = eptr->prev)
      {
	eptr->y_offset += incy;
/* printf (" [%d,%d]W set [%d,%d]y+=%d ", Current->y,Current->x, eptr->y, eptr->x, incy); /*DEBUG*/
      }

      /*
       * Our line bottom may be greater than the old one.
       */
      if (LineBottom < dy - baseline)
      {
	LineBottom = dy - baseline;
      }
    }

    /*
     * Advance the X position.
     */
    *x += Current->widget_data->width + extra;
  }
  PF_LF_State = 0;
  NeedSpace = 1;		/* need to follow widget with a space */
}


static void
PushFont (font)
XFontStruct *font;
{
  FontRec *fptr;

/* printf (" PushFont %p ", font); /*DEBUG*/

  fptr = (FontRec *)XtMalloc(sizeof(FontRec));
  fptr->font = font;
  fptr->next = FontStack;
  FontStack = fptr;
}


static XFontStruct *
PopFont ()
{
  XFontStruct *font;
  FontRec *fptr;

  if (FontStack->next != NULL)
  {
    fptr = FontStack;
    FontStack = FontStack->next;
    font = fptr->font;
/* printf (" PopFont %p ", font); /*DEBUG*/
    XtFree((char *) fptr);
  }
  else
  {
#ifdef VERBOSE
    fprintf (stderr, "Warning, popping empty font stack!\n");
#endif
    font = FontStack->font;
/* printf (" EmptyPop to %p ", font); /*DEBUG*/
  }

  return (font);
}


/*
 * DisplayMessage
 *
 * Add display elements for text MSG in font FONT.
 * Designed for captions and short messages alone on their line.
 *
 * XMAX is the left hand edge to use (caller subtracts margin if needed).
 * CENTER=True requests centering of the text between *x and XMAX.
 * The original font is restored before returning.
 * If MSG is longer than XMAX - *x, MSG will be left-indented.
 */

static void
DisplayMessage (hw, x, y, msg, font, xmax, center)
HTMLWidget hw;
int *x, *y;
const char *msg;
XFontStruct *font;
unsigned int xmax;
Boolean center;
{
  /*
   * switch to specified font
   */
  PushFont (currentFont);
  NewFont (font);
  currentFont = font;

  if (center)
  {
    /*
     * Simulate align=center for the typical 1-line message,
     * since align=center isn't supported yet.  It will lose
     * somewhat because FormatPlace will do whitespace conversion,
     * but the code below has the advantage of simplicity.
     *
     * Helpfully, XTextExtents seems to return Sum(width of each
     * character) and seems to consider control charaters (e.g. LF)
     * as being of zero length.  Thus stray LFs in the source text
     * don't affect the width calculation!
     */
    int dir, nascent, descent, margin;
    XCharStruct all;

    XTextExtents (font, msg, strlen(msg), &dir, &nascent, &descent, &all);
    margin = xmax - *x - all.width;
    if (margin < 0)  margin = 0;
    *x += margin / 2;
  }

  FormatPlace (hw, msg, x, y, xmax);

  font = PopFont ();
  NewFont (font);
  currentFont = font;
}


/*
 * We've just terminated the current OPTION.
 * Put it in the proper place in the SelectInfo structure.
 * Move option_buf into options, and maybe copy into
 * value if is_value is set.
 */
static void
ProcessOption (sptr)
SelectInfo *sptr;
{
  int i, cnt;
  char **tarray;

  clean_white_space (sptr->option_buf);
  tarray = sptr->options;
  cnt = sptr->option_cnt + 1;
  sptr->options = (char **)XtMalloc(sizeof(char *) * cnt);
  for (i = 0; i < (cnt - 1); i++)
  {
    sptr->options[i] = tarray[i];
  }
  if (tarray != NULL)
  {
    XtFree((char *) tarray);
    tarray = NULL;
  }
  sptr->options[cnt - 1] = sptr->option_buf;
  sptr->option_cnt = cnt;

  tarray = sptr->returns;
  cnt = sptr->option_cnt;
  sptr->returns = (char **)XtMalloc(sizeof(char *) * cnt);
  for (i = 0; i < (cnt - 1); i++)
  {
    sptr->returns[i] = tarray[i];
  }
  if (tarray != NULL)
  {
    XtFree((char *) tarray);
    tarray = NULL;
  }
  sptr->returns[cnt - 1] = sptr->retval_buf;

  if (sptr->is_value)
  {
    tarray = sptr->value;
    cnt = sptr->value_cnt + 1;
    sptr->value = (char **)XtMalloc(sizeof(char *) * cnt);
    for (i = 0; i < (cnt - 1); i++)
    {
      sptr->value[i] = tarray[i];
    }
    if (tarray != NULL)
    {
      XtFree((char *) tarray);
      tarray = NULL;
    }
    sptr->value[cnt - 1] = XtNewString(sptr->option_buf);
    sptr->value_cnt = cnt;
  }
}


/*
 * Horrible code for the TEXTAREA element.  Escape '\' and ''' by
 * putting a '\' in front of them, then replace all '"' with '''.
 * This lets us safely put the resultant value between double quotes.
 */
char *
TextAreaAddValue (value, text)
char *value;
char *text;
{
  int extra;
  char *buf;
  char *bptr;
  char *tptr;

  if ((text == NULL) || (text[0] == '\0')) return (value);

  extra = 0;
  tptr = text;
  while (*tptr != '\0')
  {
    if (*tptr == '\\')
    {
      extra++;
    }
    else if (*tptr == '\'')
    {
      extra++;
    }
    tptr++;
  }

  buf = (char *)XtMalloc (strlen(text) + strlen(value) + extra + 1);
  strcpy(buf, value);

  tptr = text;
  bptr = (char *) (buf + strlen (value));
  while (*tptr != '\0')
  {
    if ((*tptr == '\\') || (*tptr == '\''))
    {
      *bptr++ = '\\';
      *bptr++ = *tptr++;
    }
    else if (*tptr == '\"')
    {
      *bptr++ = '\'';
      tptr++;
    }
    else
    {
      *bptr++ = *tptr++;
    }
  }
  *bptr = '\0';

  XtFree(value);

  return (buf);
}

/*
 * A table column needs to be pushed rightward.  This is done by determining
 * which columns need to be moved, then adding deltax0 to all elements
 * between Current and CurrentTable->head whose tblcol says they're in one
 * of those columns.
 */
static void
WidenTableColumn (col, deltax0)
int col, deltax0;
{
  struct ele_rec *eptr;
  int i, first, last;

    /* move all columns from col to ncols or first "-1" (unfixed) */
    first = CurrentTable->ncols;
    for (last = col;  last <= first;  ++last)
	if (CurrentTable->col_x0[last] < 0)  break;
    first = col;

    /* adjust the col_x0[] boundaries */
    for (i = first;  i < last;  ++i)
	CurrentTable->col_x0[i] += deltax0;

    /* adjust the relevant element records */
    if (first < CurrentTable->ncols)
    {
      for (eptr = Current;  eptr != CurrentTable->head;  eptr = eptr->prev)
      {
	if (eptr->tblcol >= first  &&  eptr->tblcol < last)
	{
	  eptr->x += deltax0;
	}
      }
    }
    CurrentTable->Xmax += deltax0;
}

/* #define TABLE_DEBUG 1 */

static void
Table_Open (hw, opstr, x, y)
HTMLWidget hw;
char *opstr;
int *x, *y;
{
  TBL_INFO *newtable = (TBL_INFO *) XtMalloc (sizeof (TBL_INFO));
  int n;
  char *tptr;

    newtable->prev = CurrentTable;
    newtable->maxcols = INITIAL_TBL_COLS;
    Width -= hw->html.margin_width;
    newtable->oldWidth = Width;

    /* table's "width=" spec is currently ignored except to determine
     * if the outermost table will fit on the current line
     */
    if (CurrentTable == NULL)
    {
      Boolean wrap = TRUE;

      /*
       * if width specified, will table fit in remaining space?
       */
      tptr = ParseMarkTag (opstr, MT_TABLE, "width");
      if (tptr != NULL)
      {
	char c;
	int w = -1;

	if (sscanf (tptr, " %d%c", &w, &c) == 2  &&  c == '%')
	{
	  /* "width=x%" (of usable screen width) */
	  w = w * (Width - hw->html.margin_width) / 100;
	}
	/* wrap if no useful size specified or it won't fit */
	wrap = (w <= 0  ||  (*x + w + MarginW) >= Width) && (*x > TextIndent);

	XtFree (tptr);
      }
      if (wrap)  LineFeed (hw, x, y, 1);  /* requires CurrentTable==NULL! */
    }
    else
    {
      CurrentTable->next = newtable;
      ConditionalLineFeed (hw, x, y, 1);
      newtable->outerCellMaxX = CellMaxX;
    }

    CurrentTable = newtable;
    TableDepth++;

    CurrentTable->state = TBL_OPEN;
    CurrentTable->tbl_x0 = *x;
    CurrentTable->Xmax = *x;
    CurrentTable->head = Current;
    CurrentTable->ncols = 0;
    CurrentTable->nextcol = 0;	/* for LineFeedPlace and HRulePlace */
    CurrentTable->TextIndent = TextIndent;
    CurrentTable->caption = NULL;

    /* interpret cellpadding as N spaces, and spaces as 4 wide */
    tptr = ParseMarkTag (opstr, MT_TABLE, "cellpadding");
    if (tptr != NULL)
    {
      int pad = 1;		/* default is cellpadding=1 (space) */
      sscanf (tptr, " %d", &pad);
      n = pad;
      XtFree(tptr);
    }
    else n = -1;
    /* for now, cellpadding = padding + default cellspacing (3) */
    CurrentTable->cellpadding = ((n < 0) ?  0 : n*4) + 3;

#ifdef TABLE_DEBUG
 printf ("\n%s D%d @[%d,%d]\n", opstr, TableDepth, *y, *x); /*DEBUG*/
#endif
}


static void
Table_Close (hw, opstr, x, y)
HTMLWidget hw;
char *opstr;
int *x, *y;
{
#ifdef TABLE_DEBUG
 printf ("%s %d\n", opstr, TableDepth); /*DEBUG*/
#endif
   if (CurrentTable != NULL)
   {
     struct ele_rec *eptr;
     int col, prevx0, pad = CurrentTable->cellpadding;
     TBL_INFO *prev = CurrentTable->prev;

     /*
      * Sanity enforcement: force columns to be in monotonically increasing
      * positions, to protect against "inventive" use of colspan
      */
     prevx0 = CurrentTable->col_x0[0];
     for (col = 1;  col <= CurrentTable->ncols;  ++col)
     {
       int deltax, x0 = CurrentTable->col_x0[col];

       if (x0 < 0)		/* column boundary was never set */
       {
	 x0 = prevx0 + pad;
	 deltax = 0;
       }
       else
       {
	 deltax = prevx0 + pad - x0;
       }
       if (deltax > 0)
       {
	 WidenTableColumn (col, deltax);
/* if (col < CurrentTable->ncols)  printf ("\n crazy widen: moving col %d at %d + %d\n", col+1, x0, deltax); /*DEBUG*/
	 prevx0 = x0 + deltax;
       }
       else prevx0 = x0;
     }

     CurrentTblColumn = prev ?  prev->curcol : -1;

     /*
      * Replace markers in LF and HRule elements with actual length,
      * also change all tblcols to column # of outer table
      */
     for (eptr = Current;  eptr != CurrentTable->head;  eptr = eptr->prev)
     {
       int w;
       if ( (eptr->type == E_LINEFEED  ||  eptr->type == E_HRULE)
	    &&  (w = eptr->width) < 0 )
       {
	 w = -w;
	 if (w > CurrentTable->ncols)
	 {
	   /*
	    * If this happens, someone's installed a bug.  Complain.
	    */
	   fprintf (stderr, "\a\nL#%d [%d,%d]: Bug in %s Table item width fixup: %d > %d\n\a",
		    eptr->line_number, eptr->y + eptr->y_offset, eptr->x,
		    ((eptr->type == E_HRULE) ? "HR" : "LF"),
		    w, CurrentTable->ncols);
	   w = CurrentTable->ncols;
	 }
	 if (eptr->type == E_HRULE)
	 {
	   w = CurrentTable->col_x0[w] - eptr->x - pad;
	   if (w < 0)  w = 0;
	 }
	 else
	 {
	   w = CurrentTable->col_x0[w] - 3;  /* end of LF region */
	   if (eptr->x == UNCERTAIN_X)
	   {
	     /* almost all the cell width goes to the HRule */
	     eptr->x = w - (pad - 3);  /* where the HRule will end */
	     w = pad - 3;
	   }
	   else  w -= eptr->x;
	   if (w <= 0)  w = 1;  /* no width=0 for LFs in tables */
	 }
	 eptr->width = w;
       }
       eptr->tblcol = CurrentTblColumn;
     }

     if (CurrentTable->caption != NULL)
     {
       XtFree (CurrentTable->caption);
     }

     if (prev == NULL)		/* exiting outermost table */
     {
       /*
	* Check if we have a new MaxWidth
	*/
       if ((CurrentTable->Xmax + hw->html.margin_width) > MaxWidth)
       {
	 MaxWidth = CurrentTable->Xmax + hw->html.margin_width;
       }

       /*
	* Rather than use MoveWidget every time WidenColumn gets called,
	* we wait until exiting the outermost table and then move the
	* widgets just once to their final x,y location.
	*/
       for (eptr = Current;  eptr != CurrentTable->head;  eptr = eptr->prev)
       {
	 if (eptr->type == E_WIDGET  &&  eptr->widget_data != NULL)
	 {
	   MoveWidget (eptr->widget_data,
		       (eptr->x + IMAGE_BORDER),
		       (eptr->y + eptr->y_offset + IMAGE_BORDER));
	 }
       }
     }
     else			/* exiting an inner table */
     {
       /* set CellMaxX for cell containing this now-complete table */
       CellMaxX = CurrentTable->Xmax;
       if (CurrentTable->outerCellMaxX > CellMaxX)
       {
	 CellMaxX = CurrentTable->outerCellMaxX;
       }
     }

     *x = CurrentTable->tbl_x0;
     NeedSpace = 0;
     LineBottom = 3;
     BaseLine = 0;
     Width = CurrentTable->oldWidth + hw->html.margin_width;
     TextIndent = CurrentTable->TextIndent;
     XtFree ((char *)CurrentTable);
     CurrentTable = prev;
     --TableDepth;
   }
}


static void
Table_Row_Open (hw, opstr, x, y)
HTMLWidget hw;
char *opstr;
int *x, *y;
{
    *x =  CellMaxX =  CurrentTable->tbl_x0;
    CurrentTable->col_x0[0] = *x;
    CurrentTable->curcol = 0;	/* probably useless, but doesn't hurt */
    CurrentTable->nextcol = 0;
    if (CurrentTable->ncols > 0)  *y += 3;  /* table row vertical separation */
    CurrentTable->rows_y0 = *y;
    CurrentTable->Ymax = *y;
    CharsInLine = 0;
    CurrentTable->state = TBL_ROW_OPEN;
#ifdef TABLE_DEBUG
 printf("%s D%d, L#%d @[%d,%d]\n", opstr, TableDepth, LineNumber, *y, *x); /*DEBUG*/
#endif
}


static void
Table_Row_Close (hw, opstr, x, y)
HTMLWidget hw;
char *opstr;
int *x, *y;
{
  int loc;

    if (CurrentTable->Xmax < *x)        CurrentTable->Xmax = *x;
    if (CurrentTable->Xmax < CellMaxX)  CurrentTable->Xmax = CellMaxX;
    loc = CurrentTable->Xmax + CurrentTable->cellpadding;
    /*
     * Technically, the line below should use nextcol instead of ncols,
     * but use of ncols often produces a slightly nicer result.
     */
    if (CurrentTable->col_x0[CurrentTable->ncols] < loc)
	CurrentTable->col_x0[CurrentTable->ncols] = loc;

    TextIndent = CurrentTable->tbl_x0;  /* start of row */
    /*
     * LF, if this is the outermost table and we actually need one
     */
    if (CurrentTable->prev == NULL  &&  CurrentTable->Xmax > TextIndent)
    {
      LineFeed (hw, x, y);
      LineNumber++;   
    }
    if (*y < CurrentTable->Ymax)  *y = CurrentTable->Ymax;
    CurrentTable->state = TBL_OPEN;
#ifdef TABLE_DEBUG
 printf ("\n%s D%d, L#%d, *y=%d\n", opstr, TableDepth, LineNumber, *y); /*DEBUG*/
#endif
}


static void
Table_Item_Open (hw, opstr, x, y)
HTMLWidget hw;
char *opstr;
int *x, *y;
{
  int sx, loc, w, span, nextcol;
  int mycol = CurrentTable->nextcol;  /* advance to next column */
  int myx0 = CurrentTable->col_x0[mycol];
  const int pad = CurrentTable->cellpadding;
  char *tptr;

    CurrentTable->curcol = mycol;
    CurrentTblColumn = mycol;
    *y = CurrentTable->rows_y0;
    loc = CellMaxX + CurrentTable->cellpadding;
    if (mycol > 0  &&  loc > myx0)
    {
      int deltax = loc - myx0;

      if (myx0 > 0  &&  mycol < CurrentTable->ncols)
      {
	/* retroactively adjust everything */
	WidenTableColumn (mycol, deltax);
/* printf ("widen1: col %d moved from %d --> %d\n", mycol+1, myx0, loc); /*DEBUG*/
      }
      else
      {
	/*
	 * Would like to give new columns a minimum width of 40, but that
	 * could cause problems if previous <td> used colspan= and no
	 * width= and if the spanned columns later turn out to have
	 * an assigned width= <40.
	 */
	CurrentTable->col_x0[mycol] = loc;
      }
      myx0 = loc;
    }
    *x = myx0;
    CellMaxX = myx0;
    TextIndent = myx0;
    BaseLine = -100;
    LineBottom = 0;
    NeedSpace = 0;
    PF_LF_State = 1;
    PrevContextEnd = Current;
    EmptyRegion = True;		/* suppresses leading linefeeds */

    span = 1;
    tptr = ParseMarkTag (opstr, MT_TABLE_ITEM, "colspan");
    if (tptr != NULL)
    {
      sscanf (tptr, " %d", &span);
      if (span <= 0  ||  span > 40)   span = 1;	 /* ignore unlikely values */
      XtFree (tptr);
    }
    nextcol = mycol + span;
    if (nextcol > CurrentTable->ncols)
    {
      int i = CurrentTable->ncols;

      CurrentTable->ncols = nextcol;
      if (nextcol > CurrentTable->maxcols)
      {
	/* make room for at least another 25 columns */
	int newsize = CurrentTable->maxcols + 25;
	if (newsize < nextcol + 10)  newsize = nextcol + 10;
	CurrentTable->maxcols = newsize;

	newsize = sizeof(TBL_INFO)
	          + (sizeof(CurrentTable->col_x0[0])
		     * (newsize - INITIAL_TBL_COLS));
	CurrentTable = (TBL_INFO *) XtRealloc ((char *)CurrentTable, newsize);
	if (CurrentTable->prev != NULL)
	    CurrentTable->prev->next = CurrentTable;
      }
      /* mark newly created, spanned columns, as having unknown x0 */
      while (++i <= nextcol)  CurrentTable->col_x0[i] = -1;
    }

    /*
     * Decide what Width to use for this cell.
     */

    /* default end of column if no width specified */
    sx = CurrentTable->ncols;	/* #columns in table so far */
    if (nextcol < sx)		/* if not last column of this row */
    {
      w = CurrentTable->oldWidth - myx0;  /* remaining width */
      Width = myx0 + w * span / (sx - mycol);  /* share of remaining width */
      if (Width > CurrentTable->oldWidth)
      {
	Width = CurrentTable->oldWidth;
      }
    }
    else Width = CurrentTable->oldWidth;  /* at last column (so far) */

    w = CurrentTable->col_x0[nextcol];
/* printf ("T%d, c%d: %d + (%d-%d) * %d/%d = %d vs %d\n", TableDepth, mycol+1, myx0, CurrentTable->oldWidth, CurrentTable->tbl_x0, span, sx, Width, w - pad); /*DEBUG*/
    if (w - pad > Width)   Width = w - pad;

    w -= myx0;			/* is <0 for new cols */
    tptr = ParseMarkTag (opstr, MT_TABLE_ITEM, "width");
    if (tptr != NULL)
    {
      char c;

      if (sscanf(tptr, " %d%c", &w, &c) == 2  &&  c == '%')
      {
	/* "width=x%" (of total available width) */
	w = (CurrentTable->oldWidth - CurrentTable->tbl_x0 - pad) * w/100;
      }
      XtFree (tptr);
      w -= pad;			/* cellpadding comes out of requested width */
      if (w > 0)		/* sscanf may have failed */
      {
	int nextx0;
	/* enforce right hand edge */
	Width = myx0 + w;

	/* finally, set end of this column / start of following column */
	loc = myx0 + w + pad;	/* nominal start of following column */
	nextx0 = CurrentTable->col_x0[nextcol];
	if (nextx0 < 0)
	    CurrentTable->col_x0[nextcol] = loc;

#if 1  /* 1: adjust on spec now; 0: wait until something actually runs over */
	else if (loc > nextx0)
	{
	  int deltax = loc - nextx0;
	  /* retroactively adjust everything */
	  WidenTableColumn (nextcol, deltax);
/* printf ("widen2: col %d moved from %d --> %d\n", nextcol+1, nextx0, loc); /*DEBUG*/
	}
#endif
      }
    }
    /* increase Width for indeterminate width cells that begin off screen */
    if (Width <= myx0)  Width = myx0 + 100;

    CurrentTable->state = TBL_ITEM_OPEN;
    CurrentTable->nextcol = nextcol;

#ifdef TABLE_DEBUG
 printf("%s D%d#%d*%d @[%d,%d-%d]  W%d\n", opstr, TableDepth, mycol+1, span, *y, *x, CurrentTable->col_x0[nextcol], w); /*DEBUG*/
#endif
}


static void
Table_Item_Close (hw, opstr, x, y)
HTMLWidget hw;
char *opstr;
int *x, *y;
{
  int max_y = *y + LineBottom + Max(BaseLine, 0);
  int w;

    LineBottom = 0;
    BaseLine = -100;
    if (CurrentTable->Ymax < max_y)   CurrentTable->Ymax = max_y;
    if (*x > CellMaxX)
    {
      CellMaxX = *x;
    }
    else if (*x < CellMaxX)
    {
      *x = CellMaxX;
    }

#ifdef TABLE_DEBUG
 printf ("\n%s @[%d,%d] D%d, Ymax %d, L#%d\n", opstr, *y, *x, TableDepth, CurrentTable->Ymax, LineNumber); /*DEBUG*/
#endif
    /* suppress spurious formatted output of whitespace after </td> */
    NeedSpace = 0;
    PF_LF_State = 0;
    PrevContextEnd = Current;
    CurrentTable->state = TBL_ROW_OPEN;
}

/*
 * EnsureTableItemOpen
 *
 * CurrentTable != NULL and the caller wants to create some kind of element
 * that ought to be inside a table cell.  Do an implicit <TD> if the author
 * seems to have left one out.
 */
static void
EnsureTableItemOpen (hw, x, y)
HTMLWidget hw;
int *x, *y;
{
  switch (CurrentTable->state)
  {
    case TBL_OPEN:
      Table_Row_Open (hw, "+TR", x, y);
      ++errors_in_HTML_document;
      /* no break */
    case TBL_ROW_OPEN:
      Table_Item_Open (hw, "+TD", x, y);
      ++errors_in_HTML_document;
      break;
  }
}

/*
 * Make necessary changes to formatting, based on the type of the
 * parsed HTML text we are formatting.
 * Some calls create elements that are added to the formatted element list.
 */
void
TriggerMarkChanges (hw, mptr, x, y)
HTMLWidget hw;
struct mark_up *mptr;
int *x, *y;
{
  struct mark_up *mark;
  XFontStruct *font;
  int type, width;
  
  mark = mptr;
  type = mark->type;
  font = NULL;

  /*
   * While Special is true, ignore all elements except the matching end
   *   tags and ordinary text.
   * Let text through because it's the "value" of the special tag.
   * Let TITLE through so we can hit the </title>.
   * Let SELECT through so we can hit the </select>.
   * Let OPTION through because that's part of handling SELECTs.
   * Let TEXTAREA through so we can hit the </TEXTAREA>.
   * Let CAPTION through so we can hit the </caption>.
   */
  if (Special  &&
      (type != M_NONE)  &&  (type != M_TITLE)  && (type != M_CAPTION)  &&
      (type != M_SELECT)  &&  (type != M_OPTION)  &&  (type != M_TEXTAREA) )
  {
    return;
  }

  switch (type)
  {
    /*
     * Place the text.  Different functions based on whether it
     * is pre-formatted or not.
     */
  case M_NONE:
    if ((Special) && (CurrentSelect == NULL) && (TextAreaBuf == NULL))
    {
      /*
       * This code works for all special <TYPE>text</TYPE> items, since
       * they can't overlap.  Currently used for: TITLE, CAPTION.
       * End tag processing must either move this string somewhere else
       * for safe keeping or copy and free the memory.
       */
      if (SpecialsText == NULL)
      {
	SpecialsText = XtNewString (mptr->text);
      }
      else
      {
	char *tptr;

	tptr = (char *)XtMalloc(strlen(SpecialsText) + strlen(mptr->text) + 1);
	strcpy (tptr, SpecialsText);
	strcat (tptr, mptr->text);
	XtFree (SpecialsText);
	SpecialsText = tptr;
      }
    }
    else if ((Special) && (CurrentSelect != NULL))
    {
      if (CurrentSelect->option_buf != NULL)
      {
	char *tptr;

	tptr = (char *)XtMalloc(strlen (CurrentSelect->option_buf) +
				strlen (mptr->text) + 1);
	strcpy (tptr, CurrentSelect->option_buf);
	strcat (tptr, mptr->text);
	XtFree (CurrentSelect->option_buf);
	CurrentSelect->option_buf = tptr;
      }
    }
    else if ((Special) && (TextAreaBuf != NULL))
    {
      TextAreaBuf = TextAreaAddValue (TextAreaBuf, mptr->text);
    }
    else
    {
      if (CurrentTable)
      {
	char *cp;
	for (cp = mptr->text;  *cp != '\0';  ++cp)
	{
	  if (isgraph (*cp))	/* if any visible chars in string */
	  {
	    EnsureTableItemOpen (hw, x, y);
	    break;
	  }
	}
	if (*cp == '\0'  &&  CurrentTable->state != TBL_ITEM_OPEN)
	{
	  break;		/* flush whitespace between table cells */
	}
      }

      if (Preformat)
      {
	PreformatPlace (hw, mptr->text, x, y, Width);
      }
      else
      {
	FormatPlace (hw, mptr->text, x, y, Width);
      }
    }
    break;

    /*
     * Titles are just set into the widget for retrieval by XtGetValues().
     */
  case M_TITLE:
    if (mark->is_end)
    {
      Special = 0;
      if (hw->html.title != NULL)
      {
	XtFree (SpecialsText);	/* keep first one, flush others */
      }
      else
      {
	hw->html.title = SpecialsText;
      }
      SpecialsText = NULL;
    }
    else
    {
      if (SpecialsText != NULL)
      {
	XtFree (SpecialsText);
      }
      SpecialsText = NULL;
      Special = 1;
    }
    break;

    /*
     * Feel free to expand the code if captions on other items are allowed
     */
  case M_CAPTION:
    if (mark->is_end)
    {
      if (SpecialsText != NULL)
      {
	if (CurrentTable != NULL)  /* a table caption */
	{
	  if (CurrentTable->caption != NULL)  /* table has >1 caption?? */
	  {
	    XtFree (CurrentTable->caption);  /* free the old one */
	  }
	  CurrentTable->caption = SpecialsText;  /* keep the last one */
	}
	else XtFree (SpecialsText);  /* else discard the caption */

	SpecialsText = NULL;
      }
      Special = 0;
    }
    else
    {
      if (SpecialsText != NULL)
      {
	XtFree (SpecialsText);
      }
      SpecialsText = NULL;
      Special = 1;
    }
    break;

    /*
     * Formatting commands just change the current font.
     */
  case M_CODE:
  case M_SAMPLE:
  case M_KEYBOARD:
  case M_FIXED:
    if (mark->is_end)
    {
      font = PopFont ();
    }
    else
    {
      PushFont (currentFont);
      font = hw->html.fixed_font;
    }
    break;

  case M_STRONG:
  case M_BOLD:
    if (mark->is_end)
    {
      font = PopFont ();
    }
    else
    {
      PushFont (currentFont);
      if (currentFont == hw->html.fixed_font ||
	  currentFont == hw->html.fixeditalic_font)
	font = hw->html.fixedbold_font;
      else if (currentFont == hw->html.plain_font ||
	       currentFont == hw->html.plainitalic_font)
	font = hw->html.plainbold_font;
      else
	font = hw->html.bold_font;
    }
    break;

  case M_EMPHASIZED:
  case M_VARIABLE:
  case M_CITATION:
  case M_ITALIC:
    if (mark->is_end)
    {
      font = PopFont ();
    }
    else
    {
      PushFont (currentFont);
      if (currentFont == hw->html.fixed_font ||
	  currentFont == hw->html.fixedbold_font)
	font = hw->html.fixeditalic_font;
      else if (currentFont == hw->html.plain_font ||
	       currentFont == hw->html.plainbold_font)
	font = hw->html.plainitalic_font;
      else
	font = hw->html.italic_font;
    }
    break;
    /*
     * Strikeout means draw a line through the text.
     * Right now we just set a boolean flag which gets shoved
     * in the element record for all elements in the
     * strikeout zone.
     */

  case M_STRIKEOUT:
    if (mark->is_end)
    {
      Strikeout = False;
    }
    else
    {
      Strikeout = True;
    }
    break;
    /*
     * Headers are preceded and followed by a linefeed,
     * and they change the font.
     */

  case M_HEADER_1:
    ConditionalLineFeed (hw, x, y, 1);
    if (mark->is_end)
    {
      font = PopFont ();
      NewFont (font);
      currentFont = font;
      ConditionalLineFeed (hw, x, y, 2);
    }
    else
    {
      ConditionalLineFeed (hw, x, y, 2);
      PushFont (currentFont);
      font = hw->html.header1_font;
    }
    break;

  case M_HEADER_2:
    ConditionalLineFeed (hw, x, y, 1);
    if (mark->is_end)
    {
      font = PopFont ();
      NewFont (font);
      currentFont = font;
      ConditionalLineFeed (hw, x, y, 2);
    }
    else
    {
      ConditionalLineFeed (hw, x, y, 2);
      PushFont (currentFont);
      font = hw->html.header2_font;
    }
    break;

  case M_HEADER_3:
    ConditionalLineFeed (hw, x, y, 1);
    if (mark->is_end)
    {
      font = PopFont ();
      NewFont (font);
      currentFont = font;
      ConditionalLineFeed (hw, x, y, 2);
    }
    else
    {
      ConditionalLineFeed (hw, x, y, 2);
      PushFont (currentFont);
      font = hw->html.header3_font;
    }
    break;

  case M_HEADER_4:
    ConditionalLineFeed (hw, x, y, 1);
    if (mark->is_end)
    {
      font = PopFont ();
      NewFont (font);
      currentFont = font;
      ConditionalLineFeed (hw, x, y, 2);
    }
    else
    {
      ConditionalLineFeed (hw, x, y, 2);
      PushFont (currentFont);
      font = hw->html.header4_font;
    }
    break;

  case M_HEADER_5:
    ConditionalLineFeed (hw, x, y, 1);
    if (mark->is_end)
    {
      font = PopFont ();
      NewFont (font);
      currentFont = font;
      ConditionalLineFeed (hw, x, y, 2);
    }
    else
    {
      ConditionalLineFeed (hw, x, y, 2);
      PushFont (currentFont);
      font = hw->html.header5_font;
    }
    break;

  case M_HEADER_6:
    ConditionalLineFeed (hw, x, y, 1);
    if (mark->is_end)
    {
      font = PopFont ();
      NewFont (font);
      currentFont = font;
      ConditionalLineFeed (hw, x, y, 2);
    }
    else
    {
      ConditionalLineFeed (hw, x, y, 2);
      PushFont (currentFont);
      font = hw->html.header6_font;
    }
    break;
    /*
     * Anchors change the text color, and may set
     * underlining attributes.
     * No linefeeds, so they can be imbedded anywhere.
     */

  case M_ANCHOR:
    if (mark->is_end)
    {
      Fg = hw->html.foreground;
      Underlines = 0;
      DashedUnderlines = False;
      AnchorText = NULL;
    }
    else
    {
      char *tptr;

      /*
       * Only change the color of anchors with
       * HREF tags, because other anchors are
       * not active.
       */
      tptr = ParseMarkTag (mark->start, MT_ANCHOR, AT_HREF);
      if (tptr != NULL)
      {
	/*
	 * Have we visited it before?
	 */
	if (hw->html.previously_visited_test != NULL)
	{
	  if ((*(visitTestProc)
	       (hw->html.previously_visited_test))  (hw, tptr))
	  {
	    Fg = hw->html.visitedAnchor_fg;
	    Underlines = hw->html.num_visitedAnchor_underlines;
	    DashedUnderlines = hw->html.dashed_visitedAnchor_lines;
	  }
	  else
	  {
	    Fg = hw->html.anchor_fg;
	    Underlines = hw->html.num_anchor_underlines;
	    DashedUnderlines = hw->html.dashed_anchor_lines;
	  }
	}
	else			/* only get here if user set resource to 0 */
	{
	  Fg = hw->html.anchor_fg;
	  Underlines = hw->html.num_anchor_underlines;
	  DashedUnderlines = hw->html.dashed_anchor_lines;
	}

	XtFree(tptr);
	tptr = NULL;
      }
      AnchorText = mark->start;
    }
    break;

    /*
     * Just insert a linefeed, or ignore if this is prefomatted
     * text because the <P> will be followed by a linefeed.
     */
  case M_PARAGRAPH:
    ConditionalLineFeed (hw, x, y, 1);
    ConditionalLineFeed (hw, x, y, 2);
    break;

    /*
     * Just insert the image for now
     */
  case M_IMAGE:
    if (!mark->is_end)
    {
      if (CurrentTable)
      {
	EnsureTableItemOpen (hw, x, y);
      }
      ImagePlace (hw, mptr->start, x, y, Width);
    }
    break;
    /*
     * Can only be inside a SELECT tag.
     */

  case M_OPTION:
    if (CurrentSelect != NULL)
    {
      char *tptr;

      if (CurrentSelect->option_buf != NULL)
      {
	ProcessOption (CurrentSelect);
      }
      CurrentSelect->option_buf = XtNewString("");

      /*
       * Check if this option starts selected
       */
      tptr = ParseMarkTag (mark->start, MT_OPTION, "SELECTED");
      if (tptr != NULL)
      {
	CurrentSelect->is_value = 1;
	XtFree(tptr);
	tptr = NULL;
      }
      else
      {
	CurrentSelect->is_value = 0;
      }

      /*
       * Check if this option has a different return value field.
       */
      tptr = ParseMarkTag (mark->start, MT_OPTION, "VALUE");
      if (tptr != NULL)
      {
	if (*tptr != '\0')
	{
	  CurrentSelect->retval_buf = tptr;
	}
	else
	{
	  CurrentSelect->retval_buf = NULL;
	  XtFree(tptr);
	  tptr = NULL;
	}
      }
      else
      {
	CurrentSelect->retval_buf = NULL;
      }
    }
    break;

    /*
     * Special INPUT tag.  Allows an option menu or
     * a scrolled list.
     * Due to a restriction in SGML, this can't just be a 
     * subset of the INPUT markup.  However, I can treat it
     * that way to avoid duplicating code.
     * As a result I combine SELECT and OPTION into a faked
     * up INPUT mark.
     */
  case M_SELECT:
    if (CurrentForm != NULL)
    {
      if ((mark->is_end) && (CurrentSelect != NULL))
      {
	int len;
	char *buf, *options, *returns, *value;

	if (CurrentSelect->option_buf != NULL)
	{
	  ProcessOption (CurrentSelect);
	}

	options = ComposeCommaList(CurrentSelect->options,
				   CurrentSelect->option_cnt);
	returns = ComposeCommaList(CurrentSelect->returns,
				   CurrentSelect->option_cnt);
	value = ComposeCommaList(CurrentSelect->value,
				 CurrentSelect->value_cnt);
	FreeCommaList(CurrentSelect->options, CurrentSelect->option_cnt);
	FreeCommaList(CurrentSelect->returns, CurrentSelect->option_cnt);
	FreeCommaList(CurrentSelect->value, CurrentSelect->value_cnt);

	/*
	 * Construct a fake INPUT tag in buf.
	 */
	len = strlen (MT_INPUT)
	    + strlen (options)
	    + strlen (returns)
	    + strlen (value)
	    + strlen (" type=select options=\"\" returns=\"\" value=\"\"");

	buf = (char *)XtMalloc(len + strlen (CurrentSelect->mptr->start) + 1);
	strcpy (buf, MT_INPUT);
	strcat (buf, " type=select");
	strcat (buf, " options=\"");
	strcat (buf, options);
	strcat (buf, "\" returns=\"");
	strcat (buf, returns);
	strcat (buf, "\" value=\"");
	strcat (buf, value);
	strcat (buf, "\"");
	strcat (buf, (char *)(CurrentSelect->mptr->start + strlen(MT_SELECT)));

	if (CurrentTable)
	{
	  EnsureTableItemOpen (hw, x, y);
	}
	WidgetPlace (hw, buf, x, y, Width);

	XtFree(buf);     buf = NULL;

	XtFree(options); options = NULL;
	XtFree(returns); returns = NULL;
	XtFree(value);   value = NULL;

	XtFree((char *) CurrentSelect);
	CurrentSelect = NULL;
	Special = 0;
      }
      else if ((!mark->is_end) && (CurrentSelect == NULL))
      {
	CurrentSelect = (SelectInfo *)XtMalloc(sizeof (SelectInfo));
	CurrentSelect->hw = (Widget) hw;
	CurrentSelect->mptr = mptr;
	CurrentSelect->option_cnt = 0;
	CurrentSelect->returns = NULL;
	CurrentSelect->retval_buf = NULL;
	CurrentSelect->options = NULL;
	CurrentSelect->option_buf = NULL;
	CurrentSelect->value_cnt = 0;
	CurrentSelect->value = NULL;
	CurrentSelect->is_value = -1;
	Special = 1;
      }
    }
    break;

    /*
     * TEXTAREA is a replacement for INPUT type=text size=rows,cols
     * because SGML will not allow an arbitrary length value
     * field.
     */
  case M_TEXTAREA:
    if (CurrentForm != NULL)
    {
      if ((mark->is_end) && (TextAreaBuf != NULL))
      {
	char *buf = (char *)XtMalloc(strlen(TextAreaBuf) + 2);

	/*
	 * Finish composing the fake INPUT tag.
	 */
	strcpy(buf, TextAreaBuf);
	strcat(buf, "\"");

	if (CurrentTable)
	{
	  EnsureTableItemOpen (hw, x, y);
	}
	WidgetPlace (hw, buf, x, y, Width);

	XtFree(buf);
	XtFree(TextAreaBuf);  TextAreaBuf = NULL;
	Special = 0;
      }
      else if ((!mark->is_end) && (TextAreaBuf == NULL))
      {
	char *buf;
	int len;

	/*
	 * Construct  the start of
	 * a fake INPUT tag.
	 */
	len = strlen(MT_INPUT) + strlen(" type=textarea value=\"\"");
	buf = (char *)XtMalloc(len + strlen(mark->start) + 1);
	strcpy (buf, MT_INPUT);
	strcat (buf, (char *)(mark->start + strlen (MT_TEXTAREA)));
	strcat (buf, " type=textarea value=\"");

	TextAreaBuf = buf;
	Special = 1;
      }
    }
    break;

    /*
     * Just insert the widget.
     * Can only occur inside a FORM tag.
     * Special case the type=image stuff to become a special
     * IMG tag.
     */
  case M_INPUT:
    if (CurrentForm != NULL)
    {
      char *tptr = ParseMarkTag (mptr->start, MT_INPUT, "TYPE");

      if (tptr != NULL  &&  strcasecmp (tptr, "image") == 0)
      {
	XtFree(tptr);
	tptr = (char *)XtMalloc(strlen (mptr->start) +
				strlen (" ISMAP") +
				strlen (MT_IMAGE) -
				strlen (MT_INPUT) + 1);
	strcpy (tptr, MT_IMAGE);
	strcat (tptr, (char *)(mptr->start + strlen (MT_INPUT)));
	strcat (tptr, " ISMAP");
	if (CurrentTable)
	{
	  EnsureTableItemOpen (hw, x, y);
	}
	ImagePlace (hw, tptr, x, y, Width);

	XtFree(tptr);
	tptr = NULL;
      }
      /*
       * Hidden inputs have no element associated with them, just a widget
       * record.  If nothing displayed, don't need to EnsureTableItemOpen.
       */
      else if (tptr != NULL  &&  strcasecmp (tptr, "hidden") == 0)
      {
	XtFree(tptr);
	tptr = NULL;
	WidgetId++;
	(void) MakeWidget (hw, mptr->start, x, y, WidgetId, CurrentForm);
      }
      else
      {
	if (tptr != NULL)
        {
	  XtFree(tptr);
	  tptr = NULL;
	}
	if (CurrentTable)
	{
	  EnsureTableItemOpen (hw, x, y);
	}
	WidgetPlace (hw, mptr->start, x, y, Width);
      }
    }
    break;

    /*
     * Fillout forms.  Cannot be nested.
     */
  case M_FORM:
    ConditionalLineFeed (hw, x, y, 1);
    if ((mark->is_end) && (CurrentForm != NULL))
    {
      CurrentForm->end = WidgetId;
      ConditionalLineFeed (hw, x, y, 2);
      if (CurrentTable)
      {
	EnsureTableItemOpen (hw, x, y);
      }
      AddNewForm (hw, CurrentForm);
      CurrentForm = NULL;
    }
    else if ((!mark->is_end) && (CurrentForm == NULL))
    {
      ConditionalLineFeed (hw, x, y, 2);
      CurrentForm = (FormInfo *)XtMalloc(sizeof (FormInfo));
      CurrentForm->next = NULL;
      CurrentForm->hw = (Widget)hw;
      CurrentForm->action = ParseMarkTag(mark->start, MT_FORM, "ACTION");
      CurrentForm->method = ParseMarkTag(mark->start, MT_FORM, "METHOD");
      CurrentForm->enctype = ParseMarkTag(mark->start, MT_FORM, "ENCTYPE");
      CurrentForm->enc_entity = ParseMarkTag(mark->start,
					     MT_FORM, "ENCENTITY");
      CurrentForm->start = WidgetId;
      CurrentForm->end = -1;
    }
    break;

    /*
     * Addresses are just like headers.  A linefeed before and
     * after, and change the font.
     */
  case M_ADDRESS:
    ConditionalLineFeed (hw, x, y, 1);
    if (mark->is_end)
    {
      font = PopFont();
    }
    else
    {
      PushFont (currentFont);
      font = hw->html.address_font;
    }
    break;

    /*
     * Blockquotes increase the margin width.
     * They cannot be nested.
     */
  case M_BLOCKQUOTE:
    ConditionalLineFeed (hw, x, y, 1);
    if (mark->is_end)
    {
      MarginW = hw->html.margin_width;
      /*
       * Only unindent if we think we indented
       * when we started the blockquote
       */
      if (TextIndent <= (2 * MarginW))
      {
	TextIndent = MarginW;
      }
      ConditionalLineFeed (hw, x, y, 2);
      /*
       * The linefeed should have set x to TextIndent
       * but since it is conditional, it might not
       * have, so check it here.
       */
      if (*x > TextIndent)
      {
	*x = TextIndent;
      }
    }
    else
    {
      MarginW = 2 * hw->html.margin_width;
      /*
       * Only indent if the current indent
       * is less than what we want.
       */
      if (TextIndent < MarginW)
      {
	TextIndent = MarginW;
      }
      ConditionalLineFeed (hw, x, y, 2);
      /*
       * The linefeed should have set x to TextIndent
       * but since it is conditional, it might not
       * have, so check it here.
       */
      if (*x < TextIndent)
      {
	*x = TextIndent;
      }
    }
    break;

    /*
     * Plain text.  A single pre-formatted chunk of text
     * in its own font.
     */
  case M_PLAIN_TEXT:
    if (mark->is_end)
    {
      Preformat = 0;
      /*
       * Properly convert the Linefeed state
       * variable from preformat to formatted
       * state.
       */
      if (!CurrentTable  ||  CurrentTable->state == TBL_ITEM_OPEN)
      {
	if (PF_LF_State == 2)
	{
	  PF_LF_State = 1;
	}
	else
	{
	  PF_LF_State = 0;
	}
	ConditionalLineFeed (hw, x, y, 1);
      }
      font = PopFont ();
      NewFont (font);
      currentFont = font;
      ConditionalLineFeed (hw, x, y, 2);
    }
    else
    {
      ConditionalLineFeed (hw, x, y, 1);
      ConditionalLineFeed (hw, x, y, 2);
      Preformat = 1;
      PF_LF_State = 0;
      PushFont (currentFont);
      font = hw->html.plain_font;
    }
    break;

    /*
     * Listing text.  A different pre-formatted chunk of text
     * in its own font.
     */
  case M_LISTING_TEXT:
    if (mark->is_end)
    {
      Preformat = 0;
      /*
       * Properly convert the Linefeed state
       * variable from preformat to formatted
       * state.
       */
      if (!CurrentTable  ||  CurrentTable->state == TBL_ITEM_OPEN)
      {
	if (PF_LF_State == 2)
	{
	  PF_LF_State = 1;
	}
	else
	{
	  PF_LF_State = 0;
	}
	ConditionalLineFeed (hw, x, y, 1);
      }
      font = PopFont ();
      NewFont (font);
      currentFont = font;
      ConditionalLineFeed (hw, x, y, 2);
    }
    else
    {
      ConditionalLineFeed (hw, x, y, 1);
      ConditionalLineFeed (hw, x, y, 2);
      Preformat = 1;
      PF_LF_State = 0;
      PushFont (currentFont);
      font = hw->html.listing_font;
    }
    break;

    /*
     * Plain text.  The rest of the text is pre-formatted.
     * There is no end for this mark.
     */
  case M_PLAIN_FILE:
    ConditionalLineFeed (hw, x, y, 1);
    ConditionalLineFeed (hw, x, y, 2);
    Preformat = 1;
    PF_LF_State = 0;
    PushFont (currentFont);
    font = hw->html.plain_font;
    break;

    /*
     * Numbered lists, Unnumbered lists, Menus.
     * Currently also lump directory listings into this.
     * Save state for each indent level.
     * Change the value of the TxtIndent (can be nested)
     * Linefeed at the end of the list.
     */
  case M_NUM_LIST:
  case M_UNUM_LIST:
  case M_MENU:
  case M_DIRECTORY:
    ConditionalLineFeed (hw, x, y, 1);
    width = hw->html.font->max_bounds.width;
#if 0
    /*
     * If this is the outermost level of indentation,
     * add another linefeed for more white space.
     */
    if (TextIndent <= MarginW  ||
	(mark->is_end  &&
	 TextIndent - ((INDENT_SPACES + 1) * width) <= MarginW ))
#else
    /*
     * WBE: no, don't add leading LFs for outermost level.
     * I don't care for it.
     */
    if (mark->is_end  &&
	TextIndent - ((INDENT_SPACES + 1) * width) <= MarginW )
#endif
    {
      ConditionalLineFeed (hw, x, y, 2);
    }
    if (mark->is_end)
    {
      TextIndent = TextIndent - ((INDENT_SPACES + 1) * width);
      if (TextIndent < MarginW)
      {
	TextIndent = MarginW;
      }
      IndentLevel--;
      if (IndentLevel < 0)
      {
	IndentLevel = 0;
      }

      /*
       * Restore the old state if there is one
       */
      if (ListData->next != NULL)
      {
	DescRec *dptr;

	dptr = ListData;
	ListData = ListData->next;
	XtFree((char *) dptr);
	dptr = NULL;
      }
    }
    else
    {
      DescRec *dptr;

      dptr = (DescRec *)XtMalloc(sizeof(DescRec));
      /*
       * Save the old state, and start anew
       */
      if (type == M_NUM_LIST)
      {
	dptr->type = D_OLIST;
	dptr->count = 1;
      }
      else
      {
	dptr->type = D_ULIST;
	dptr->count = 0;
      }
      dptr->next = ListData;
      ListData = dptr;

      TextIndent = TextIndent + ((INDENT_SPACES + 1) * width);
      IndentLevel++;
    }
    *x = TextIndent;
    break;

    /*
     * Place the bullet element at the beginning of this item.
     */
  case M_LIST_ITEM:
    if (!mark->is_end)
    {
      ConditionalLineFeed (hw, x, y, 1);
      /*
       * for ordered/numbered lists
       * put numbers in place of bullets.
       */
      if (CurrentTable)
      {
	EnsureTableItemOpen (hw, x, y);
      }
      if (ListData->type == D_OLIST)
      {
	ListNumberPlace (hw, x, y, ListData->count);
	ListData->count++;
      }
      else
      {
	BulletPlace (hw, x, y);
      }
    }
    break;

  case M_TABLE_HEADER:
  case M_TABLE_ITEM:
    if (CurrentTable != NULL)	/* else ignore it */
    {
      if (mark->is_end)
      {
/* if (CurrentTable->state != TBL_ITEM_OPEN)  printf ("<%s> but in state %d?\n", mark->end, CurrentTable->state); /*DEBUG*/
	if (CurrentTable->state == TBL_ITEM_OPEN)
	    Table_Item_Close (hw, mark->end, x, y);
	/* if state isn't TBL_ITEM_OPEN, it's too late now to fix things */
      }
      else {
	switch (CurrentTable->state)
	{
	  case TBL_ITEM_OPEN:	/* assume </td> was left out */
	    Table_Item_Close (hw, "+/TD", x, y);
	    break;
	  case TBL_OPEN:	/* assume <tr> was left out */
	    Table_Row_Open (hw, "+TR", x, y);
	    ++errors_in_HTML_document;
	    break;
	}
	Table_Item_Open (hw, mark->start, x, y);
      }
    }
    break;

  case M_TABLE_ROW:
    if (CurrentTable != NULL)	/* else ignore it */
    {
      if (mark->is_end)
      {
	if (CurrentTable->state == TBL_ITEM_OPEN)
	    Table_Item_Close (hw, "+/TD", x, y);
	if (CurrentTable->state == TBL_ROW_OPEN)
	    Table_Row_Close (hw, mark->end, x, y);
	/* else <table></tr> or </tr></tr>, so just ignore this close */
	else ++errors_in_HTML_document;
      }
      else
      {
	if (CurrentTable->state == TBL_ITEM_OPEN)
	{
	  Table_Item_Close (hw, "+/TD", x, y);
	  ++errors_in_HTML_document;
	}
	if (CurrentTable->state == TBL_ROW_OPEN)
	    Table_Row_Close (hw, "+/TR", x, y);
	Table_Row_Open (hw, mark->start, x, y);
      }
    }
    break;

  case M_TABLE:
    if (mark->is_end)
    {
      if (CurrentTable != NULL)	 /* else ignore it */
      {
	if (CurrentTable->state == TBL_ITEM_OPEN)
	    Table_Item_Close (hw, "+/TD", x, y);
	if (CurrentTable->state == TBL_ROW_OPEN)
	    Table_Row_Close (hw, "+/TR", x, y);

	/*
	 * Handling captions here instead of in Table_Close is done so that
	 * captions are only rendered if the table is properly closed.
	 */
	if (CurrentTable->caption != NULL)
	{
	  char fake_mark_text[40];

	  sprintf (fake_mark_text,
		   "*TD align=center colspan=%d", CurrentTable->ncols);

	  Table_Row_Open (hw, "*TR", x, y);
	  Table_Item_Open (hw, fake_mark_text, x, y);

	  /*
	   * It'd be nice if caption font size = table's font size - 1.
	   * Another option would be to add a new table caption font
	   * resource and use that font.
	   * For now, using <h5> is a passable substitute.
	   */
	  DisplayMessage (hw, x, y,
			  CurrentTable->caption,
			  hw->html.header5_font,
			  CurrentTable->col_x0[CurrentTable->ncols]
			    - CurrentTable->cellpadding,
			  True);

	  Table_Item_Close (hw, "*/TD", x, y);
	  Table_Row_Close (hw, "*/TR", x, y);
	}  /* end if have a caption */

	Table_Close (hw, mark->end, x, y);
      }
    }
    else
    {
      if (CurrentTable != NULL)
      {
	if (CurrentTable->state == TBL_OPEN)
	{
	  Table_Row_Open (hw, "+TR", x, y);
	  ++errors_in_HTML_document;
	}
	if (CurrentTable->state == TBL_ROW_OPEN)
	{
	  Table_Item_Open (hw, "+TD", x, y);
	  ++errors_in_HTML_document;
	}
      }
      Table_Open (hw, mark->start, x, y);
    }
    break;

    /*
     * Description lists
     */
  case M_DESC_LIST:
    ConditionalLineFeed (hw, x, y, 1);
    ConditionalLineFeed (hw, x, y, 2);
    width = hw->html.font->max_bounds.width;
    if (mark->is_end)
    {
      if (DescType->type == D_TEXT)
      {
	TextIndent -= (INDENT_SPACES + 1) * width;
	if (TextIndent < MarginW)
	{
	  TextIndent = MarginW;
	}
      }
      /*
       * Restore the old state if there is one
       */
      if (DescType->next != NULL)
      {
	DescRec *dptr;

	dptr = DescType;
	DescType = DescType->next;
	XtFree((char *) dptr);
	dptr = NULL;
	/*
	 * If the old state had forced an
	 * indent, outdent it now.
	 */
	if (DescType->type == D_TITLE)
	{
	  TextIndent -= (INDENT_SPACES + 1) * width;
	  if (TextIndent < MarginW)
	  {
	    TextIndent = MarginW;
	  }
	}
      }
    }
    else
    {
      DescRec *dptr;
      char *tptr;

      dptr = (DescRec *)XtMalloc(sizeof (DescRec));
      /*
       * Check is this is a compact list
       */
      tptr = ParseMarkTag(mark->start, MT_DESC_LIST, "COMPACT");
      if (tptr != NULL)
      {
	XtFree(tptr);
	tptr = NULL;
	dptr->compact = 1;
      }
      else
      {
	dptr->compact = 0;
      }
      /*
       * Description list stared after a title needs
       * a forced indentation here
       */
      if (DescType->type == D_TITLE)
      {
	TextIndent += (INDENT_SPACES + 1) * width;
      }
      /*
       * Save the old state, and start anew
       */
      dptr->type = D_TITLE;
      dptr->next = DescType;
      DescType = dptr;
    }
    *x = TextIndent;
    break;

  case M_DESC_TITLE:
    ConditionalLineFeed (hw, x, y, 1);
    width = hw->html.font->max_bounds.width;
    /*
     * Special hack.  Don't indent again for
     * multiple <dt>'s in a row.
     */
    if (DescType->type == D_TEXT)
    {
      TextIndent -= (INDENT_SPACES + 1) * width;
      if (TextIndent < MarginW)
      {
	TextIndent = MarginW;
      }
    }
    DescType->type = D_TITLE;
    *x = TextIndent;
    break;

  case M_DESC_TEXT:
    width = hw->html.font->max_bounds.width;

    /*
     * For a compact list we want to stay on the same
     * line if there is room and we are the first line
     * after a title.
     */
    if ((DescType->compact) && (DescType->type == D_TITLE) &&
	(*x < (TextIndent + (INDENT_SPACES * width))))
    {
      NeedSpace = 0;
    }
    else
    {
      ConditionalLineFeed (hw, x, y, 1);
    }

    /*
     * Special hack.  Don't indent again for
     * multiple <dd>'s in a row.
     */
    if (DescType->type == D_TITLE)
    {
      TextIndent += (INDENT_SPACES + 1) * width;
    }
    DescType->type = D_TEXT;
    *x = TextIndent;
    break;

  case M_PREFORMAT:
    if (mark->is_end)
    {
      Preformat = 0;
      /*
       * Properly convert the Linefeed state
       * variable from preformat to formatted
       * state.
       */
      if (!CurrentTable  ||  CurrentTable->state == TBL_ITEM_OPEN)
      {
	if (PF_LF_State == 2)
	{
	  PF_LF_State = 1;
	}
	else
	{
	  PF_LF_State = 0;
	}
	ConditionalLineFeed (hw, x, y, 1);
      }
      if (saveFont != NULL)
      {
	hw->html.font = saveFont;
	saveFont = NULL;
      }
      font = PopFont ();
      NewFont (font);
      currentFont = font;
      ConditionalLineFeed (hw, x, y, 2);
    }
    else
    {
      ConditionalLineFeed (hw, x, y, 1);
      ConditionalLineFeed (hw, x, y, 2);
      Preformat = 1;
      PF_LF_State = 2;
      if (saveFont == NULL)
      {
	saveFont = hw->html.font;
	hw->html.font = hw->html.plain_font;
      }
      PushFont (currentFont);
      font = hw->html.font;
    }
    break;

    /*
     * Now with forms, <INDEX> is the same as:
     * <FORM>
     * <HR>
     * This is a searchable index.  Enter search keywords:
     * <INPUT NAME="isindex">
     * <HR>
     * </FORM>
     * Also, <INDEX> will take an ACTION tag to specify a
     * different URL to submit the query to.
     */
  case M_INDEX:
    hw->html.is_index = True;
    /*
     * No index inside a form
     */
    if (CurrentForm == NULL)
    {
      /*
       * Start the form
       */
      ConditionalLineFeed (hw, x, y, 1);
      ConditionalLineFeed (hw, x, y, 2);
      CurrentForm = (FormInfo *)XtMalloc(sizeof (FormInfo));
      CurrentForm->next = NULL;
      CurrentForm->hw = (Widget) hw;
      CurrentForm->action = NULL;
      CurrentForm->action = ParseMarkTag(mark->start,  MT_INDEX, "ACTION");
      CurrentForm->method = ParseMarkTag(mark->start, MT_INDEX, "METHOD");
      CurrentForm->enctype = ParseMarkTag(mark->start, MT_INDEX, "ENCTYPE");
      CurrentForm->enc_entity = ParseMarkTag(mark->start, MT_INDEX,
					     "ENCENTITY");
      CurrentForm->start = WidgetId;
      CurrentForm->end = -1;

      /*
       * Horizontal rule
       */
      ConditionalLineFeed (hw, x, y, 1);
      if (CurrentTable)
      {
	EnsureTableItemOpen (hw, x, y);
      }
      HRulePlace (hw, x, y, Width);
      ConditionalLineFeed (hw, x, y, 1);

      FormatPlace (hw, "This is a searchable index.  Enter search keywords: ",
		   x, y, Width);
      WidgetPlace (hw, "input SIZE=25 NAME=\"isindex\"", x, y, Width);

#ifdef ISINDEX_SUBMIT
      FormatPlace (hw, ";\n press this button to submit the query: ",
		   x, y, Width);
      WidgetPlace (hw, "input TYPE=\"submit\"", x, y, Width);

      FormatPlace (hw, ".\n", x, y, Width);
#endif /* ISINDEX_SUBMIT */

      /*
       * Horizontal rule
       */
      ConditionalLineFeed (hw, x, y, 1);
      HRulePlace (hw, x, y, Width);
      ConditionalLineFeed (hw, x, y, 1);

      /*
       * Close the form
       */
      ConditionalLineFeed (hw, x, y, 1);
      CurrentForm->end = WidgetId;
      ConditionalLineFeed (hw, x, y, 2);
      AddNewForm (hw, CurrentForm);
      CurrentForm = NULL;
    }
    break;
  case M_HRULE:
    ConditionalLineFeed (hw, x, y, 1);
    if (CurrentTable)
    {
      EnsureTableItemOpen (hw, x, y);
    }
    HRulePlace (hw, x, y, Width);
    ConditionalLineFeed (hw, x, y, 1);
    break;
  case M_LINEBREAK:
    LineFeed (hw, x, y);
    break;
  default:
    break;
  }
  if ((font != NULL) && (font != currentFont))
  {
    NewFont (font);
    currentFont = font;
  }
}


/*
 * Format all the objects in the passed Widget's
 * parsed object list to fit the locally global Width.
 * Passes in the x,y coords of where to start placing the
 * formatted text.
 * Returns the ending x,y in same variables.
 * Title objects aren't formatted, but they are saved away in hw->html.title
 *
 * The locally global variables are assumed to have been initialized
 * before this function was called.
 */
void
FormatChunk (hw, x, y)
HTMLWidget hw;
int *x, *y;
{
  struct mark_up *mptr;

  /*
   * Format all objects
   */
  mptr = hw->html.html_objects;
/*  Last = NULL;	/* not used for anything */
  while (mptr != NULL)
  {
    TriggerMarkChanges (hw, mptr, x, y);
    /*
     * Save last non-text mark
     */
#if 0 /* Last isn't used anywhere */
    if (mptr->type != M_NONE)
    {
      Last = mptr;
    }
#endif
    mptr = mptr->next;
  }

  /*
   * Implicitly perform as many </table>s as required.
   *
   * I don't know what the SGML spec says, but putting tables in the header,
   * body, or footer area without properly closing them before the region ends,
   * would seem to be doing something wrong.  That's why this code is here
   * instead of in FormatAll.
   */
  while (CurrentTable != NULL)
  {
    if (CurrentTable->state == TBL_ITEM_OPEN)
	Table_Item_Close (hw, "+/TD", x, y);
    if (CurrentTable->state == TBL_ROW_OPEN)
	Table_Row_Close (hw, "+/TR", x, y);
    Table_Close (hw, "+/TABLE", x, y);
    ++errors_in_HTML_document;
  }
}


/*
 * Called by the widget to format all the objects in the
 * parsed object list to fit its current window size.
 * Returns the max_height of the entire document.
 * Title objects are ignored, and not formatted.
 */
int
FormatAll (hw, Fwidth)
HTMLWidget hw;
int *Fwidth;
{
  int x, y;
  int width;
#ifdef TIMING
  gettimeofday (&Tv, &Tz);
  fprintf (stderr, "FormatAll enter (%d.%d)\n", Tv.tv_sec, Tv.tv_usec);
#endif

  width = *Fwidth;
  MaxWidth = width;

  /*
   * Clear the is_index flag
   */
  hw->html.is_index = False;

  /*
   * Initialize local variables, some from the widget
   */
  MarginW = hw->html.margin_width;
  Fg = hw->html.foreground;
  Bg = hw->core.background_pixel;
  Underlines = 0;
  DashedUnderlines = False;
  Width = width;
  TextIndent = MarginW;
  ElementId = 0;
  WidgetId = 0;
  LineNumber = 1;
  LineBottom = 0;
  BaseLine = -100;
  CharsInLine = 0;
  IndentLevel = 0;
  Special = 0;
  Preformat = 0;
  PF_LF_State = 0;
  NeedSpace = 0;
  Strikeout = False;
  AnchorText = NULL;
  DescType = &BaseDesc;
  ListData = &BaseDesc;
  DescType->type = D_NONE;
  DescType->count = 0;
  DescType->compact = 0;
  DescType->next = NULL;
  CurrentForm = NULL;
  CurrentSelect = NULL;
  TextAreaBuf = NULL;
  TableDepth = 0;
  CurrentTblColumn = -1;
  SpecialsText = NULL;		/* will be NULL again before exiting */
  errors_in_HTML_document = 0;

  /*
   * Free the old TBL_INFO blocks if there are any.
   */
  while (CurrentTable != NULL)
  {
    TBL_INFO *prev = CurrentTable->prev;
    XtFree((char *)CurrentTable);
    CurrentTable = prev;
  }

  /*
   * Free the old title, if there is one.
   */
  if (hw->html.title != NULL)
  {
    XtFree(hw->html.title);
    hw->html.title = NULL;
  }

  /*
   * Allocated object handling policies:	(WBE, 97Apr25)
   *
   * hw->html.formatted_elements policy:
   *   It's zero at the beginning of time;
   *   Starting with Current = NULL (below) will recycle the list elements;
   *   More elements are added if needed;
   *   Excess elements are freed (see FreeLineList calls below);
   *
   * Allocated object in the elements:
   *   HTML tag-specific use of an image is kept in eptr->img_data.
   *     AddFElement allocates and frees (IMGInfo *)img_data blocks.
   *   General image information is kept in (ImageInfo *) blocks on
   *     hw->html.image_list.  HTMLGetImage and NoImage return (ImageInfo *)
   *     pointers which are generally placed in eptr->img_data->pic_data.
   *     FreeImages frees the resources in image_list when the current
   *     document changes.
   *   Widgets are kept in hw->html.widget_list by widget ID, so it's safe
   *     to clear widget pointers in the element records.  When reparsing
   *     the same document, widget IDs will match up.  HTMLFreeWidgetInfo
   *     gets called to delete all widgets on widget_list when switching to
   *     a new document.
   *   Text in edata, anchorHRef, and anchorName is freed by FreeLineList
   *     and by AddFElement.
   */

  /*
   * Clear any previous selections
   */
  hw->html.select_start = NULL;
  hw->html.select_end = NULL;
  hw->html.new_start = NULL;
  hw->html.new_end = NULL;

  /*
   * Set up a starting font, and starting x, y, position
   */
  NewFont (hw->html.font);
  currentFont = hw->html.font;
  saveFont = NULL;
  FontStack = &FontBase;
  FontStack->font = hw->html.font;

  x = TextIndent;
  y = hw->html.margin_height;

  /*
   * Start a null element list, to be filled in as we go.
   */
  Current = NULL;   PrevContextEnd = Current;
  EmptyRegion = True;

  /*
   * If we have parsed special header text, fill it in now.
   */
  if (hw->html.html_header_objects != NULL)
  {
    struct mark_up *msave = hw->html.html_objects;
    hw->html.html_objects = hw->html.html_header_objects;
    FormatChunk (hw, &x, &y);

    if (saveFont != NULL)
    {
      hw->html.font = saveFont;
      saveFont = NULL;
    }
    NewFont (hw->html.font);
    currentFont = hw->html.font;

    ConditionalLineFeed (hw, &x, &y, 1);

    hw->html.html_objects = msave;
  }

  /*
   * Format all objects for width
   */
  FormatChunk (hw, &x, &y);

  /*
   * If we have parsed special footer text, fill it in now.
   */
  if (hw->html.html_footer_objects != NULL)
  {
    struct mark_up *msave;

    if (saveFont != NULL)
    {
      hw->html.font = saveFont;
      saveFont = NULL;
      ++errors_in_HTML_document;
    }
    NewFont (hw->html.font);
    currentFont = hw->html.font;

    Preformat = 0;
    PF_LF_State = 0;
    NeedSpace = 0;

    ConditionalLineFeed (hw, &x, &y, 1);

    msave = hw->html.html_objects;
    hw->html.html_objects = hw->html.html_footer_objects;
    FormatChunk (hw, &x, &y);

    hw->html.html_objects = msave;
  }

  /*
   * Restore the proper font from unterminated preformatted text
   * sequences.
   */
  if (saveFont != NULL)
  {
    hw->html.font = saveFont;
    saveFont = NULL;
    ++errors_in_HTML_document;
  }

  /*
   * If document didn't properly pair its font changes, there may be
   * stuff left on the font stack.  Free it.
   */
  while (FontStack->next != NULL)
  {
    (void) PopFont ();
    if (Preformat)  Preformat = 0;
    else ++errors_in_HTML_document;
  }

  /*
   * Free any unclaimed SpecialsText
   */
  if (SpecialsText != NULL)
  {
    XtFree(SpecialsText);
    SpecialsText = NULL;
    ++errors_in_HTML_document;
  }

  /*
   * Free any unfinished TextAreaBuf
   */
  if (TextAreaBuf != NULL)
  {
    XtFree(TextAreaBuf);
    TextAreaBuf = NULL;
    ++errors_in_HTML_document;
  }

  /*
   * Check for unterminated anchor
   */
  if (AnchorText != NULL)
  {
    Fg = hw->html.foreground;
    Underlines = 0;
    DashedUnderlines = False;
    AnchorText = NULL;
    ++errors_in_HTML_document;
  }

  /*
   * Free contents of any unterminated Forms
   */
  if (CurrentForm != NULL)
  {
#if 0
    if (CurrentForm->action != NULL)  XtFree (CurrentForm->action);
    if (CurrentForm->method != NULL)  XtFree (CurrentForm->method);
    if (CurrentForm->enctype != NULL)  XtFree (CurrentForm->enctype);
    if (CurrentForm->enc_entity != NULL)  XtFree (CurrentForm->enc_entity);
    XtFree ((char *)CurrentForm);
#else
    FreeOneForm (CurrentForm);
#endif
    CurrentForm = NULL;
    ++errors_in_HTML_document;
  }
  if (CurrentSelect != NULL)
  {
    FreeCommaList(CurrentSelect->options, CurrentSelect->option_cnt);
    FreeCommaList(CurrentSelect->returns, CurrentSelect->option_cnt);
    FreeCommaList(CurrentSelect->value, CurrentSelect->value_cnt);
    if (CurrentSelect->retval_buf != NULL)
	XtFree (CurrentSelect->retval_buf);
    if (CurrentSelect->option_buf != NULL)
	XtFree (CurrentSelect->option_buf);
    XtFree ((char *)CurrentSelect);
    CurrentSelect = NULL;
    ++errors_in_HTML_document;
  }

  /*
   * Ensure a linefeed after the final element of the document.
   */
  ConditionalLineFeed (hw, &x, &y, 1);

  /*
   * End of document.
   * LineNumber is 1..N, doc_line_count is 0..N-1 (#lines and array index),
   * so convert.
   */
  hw->html.doc_line_count = LineNumber - 1;

  /*
   * Free any existing HTML error line elements
   */
  FreeLineList (html_error_line_elements);

  if (hw->html.html_errmsg_cutoff >= 0  &&
      errors_in_HTML_document > hw->html.html_errmsg_cutoff)
  {
    char msg[60];

    html_error_line_elements = Current;

    sprintf (msg, "%d HTML violation%s in document", errors_in_HTML_document,
	     (errors_in_HTML_document == 1) ? "" : "s");

    DisplayMessage (hw, &x, &y, msg, hw->html.header5_font, Width, False);
    LineFeed (hw, &x, &y);

    if (html_error_line_elements != NULL)  /* Current previously non-NULL */
    {
      html_error_line_elements = html_error_line_elements->next;
      html_error_line_elements->prev->next = NULL;  /* disconnect them */
    }
    else if (Current != NULL)	/* foo. HTML errors in a blank document! */
    {
      html_error_line_elements = hw->html.formatted_elements;
      hw->html.formatted_elements = Current->next;  /* extras freed below */
      Current = NULL;		/* so code below will understand */
    }
    /* else msg & LF didn't get displayed??  Out of memory?  No action. */
  }
  else html_error_line_elements = NULL;

  /*
   * Free any extra elements on the element list.
   */
  if (Current != NULL)
  {
    FreeLineList (Current->next);  /* NULL as arg okay */
    Current->next = NULL;
  }
  else if (hw->html.formatted_elements != NULL)
  {
    FreeLineList (hw->html.formatted_elements);
    hw->html.formatted_elements = NULL;
  }

  /*
   * Add the bottom margin to the max height.
   */
  y = y + hw->html.margin_height;

  /*
   * Make the line array indexed into the element list
   * and store it into the widget
   */
  hw->html.full_line_count = --LineNumber;  /* LineNumber now = #lines */
  if (hw->html.line_array != NULL) XtFree((char *) hw->html.line_array);
  hw->html.line_array =
      MakeLineList (hw->html.formatted_elements, LineNumber);
  if (LineNumber > hw->html.doc_line_count)
  {
    hw->html.line_array[LineNumber - 1] = html_error_line_elements;
  }

  /*
   * If the passed-in MaxWidth was wrong, correct it.
   */
  if (MaxWidth != width)
  {
    *Fwidth = MaxWidth;
  }

#ifdef TIMING
  gettimeofday (&Tv, &Tz);
  fprintf (stderr, "FormatAll exit (%d.%d)\n", Tv.tv_sec, Tv.tv_usec);
#endif
  return (y);
}


/*
 * Redraw a linefeed.
 * Basically a filled rectangle at the end of a line.
 */
void
LinefeedRefresh (hw, eptr)
HTMLWidget hw;
struct ele_rec *eptr;
{
  int x1, y1;
  unsigned int width, height;

  x1 = eptr->x;
  if (x1 < -20000)
  {
    fprintf (stderr, "\aTABLE BUG!  LF refresh with x=%d\n\a", x1);
    return;			/* who knows where it should have been */
  }

  width = eptr->width;		/* > 0 if inside a table */
  if (width < 0)
  {
    if (x1 > (int) hw->core.width)
    {
      width = 0;
    }
    else
    {
      width = hw->core.width - x1;
    }
  }
#ifdef NO_EXTRA_FILLS
  /*
   * The actual height of the rectangle to fill is strange, based
   * on a difference between eptr->font->(ascent/descent) and
   * eptr->font->max_bounds.(ascent/descent) which I don't quite
   * understand. But it works.
   * Deal with bad Lucida descents.
   */

#ifdef SHORT_LINEFEEDS
  y1 = eptr->y + eptr->y_offset + eptr->font->max_bounds.ascent
       - eptr->font->ascent;
  height = eptr->font->ascent + eptr->font->descent;
#else
  y1 = eptr->y + eptr->font->max_bounds.ascent - eptr->font->ascent;
  height = eptr->line_height;
#endif /* SHORT_LINEFEEDS */

#else /* undef NO_EXTRA_FILLS */

#ifdef SHORT_LINEFEEDS
  y1 = eptr->y + eptr->y_offset;
  if (eptr->font->descent > eptr->font->max_bounds.descent)
  {
    height = eptr->font->max_bounds.ascent + eptr->font->descent;
  }
  else
  {
    height = eptr->font->max_bounds.ascent + eptr->font->max_bounds.descent;
  }
#else
  y1 = eptr->y;
  height = eptr->line_height;
#endif /* SHORT_LINEFEEDS */

#endif /* NO_EXTRA_FILLS */

  x1 = x1 - hw->html.scroll_x;
  y1 = y1 - hw->html.scroll_y;

  if (eptr->selected == True)
  {
    XSetForeground (XtDisplay (hw), hw->html.drawGC, eptr->fg);
  }
  else
  {
    XSetForeground (XtDisplay (hw), hw->html.drawGC, eptr->bg);
  }
  XFillRectangle (XtDisplay (hw->html.view), XtWindow (hw->html.view),
		  hw->html.drawGC,
		  x1, y1, width, height);
}


/*
 * Redraw part of a formatted text element, in the passed fg and bg
 */
void
PartialRefresh (hw, eptr, start_pos, end_pos, fg, bg, sip)
HTMLWidget hw;
struct ele_rec *eptr;
int start_pos, end_pos;
unsigned long fg, bg;
int sip;			/* selection-in-progress --GN 1997May05 */
{
  int ascent;
  char *tdata;
  int tlen;
  int x, y, width;
  int partial;

  XSetFont (XtDisplay (hw), hw->html.drawGC, eptr->font->fid);
  ascent = eptr->font->max_bounds.ascent;
  width = -1;
  partial = 0;

  if (start_pos != 0)
  {
    int dir, nascent, descent;
    XCharStruct all;

#ifdef ASSUME_FIXED_WIDTH_PRE
    if (eptr->font == hw->html.plain_font)
    {
      all.width = eptr->font->max_bounds.width * start_pos;
    }
    else
    {
      XTextExtents (eptr->font, (char *) eptr->edata,
		    start_pos, &dir, &nascent, &descent, &all);
    }
#else
    XTextExtents (eptr->font, (char *) eptr->edata,
		  start_pos, &dir, &nascent, &descent, &all);
#endif /* ASSUME_FIXED_WIDTH_PRE */
    x = eptr->x + all.width;
    tdata = (char *) (eptr->edata + start_pos);
    partial = 1;
  }
  else
  {
    x = eptr->x;
    tdata = (char *) eptr->edata;
  }

  if (end_pos != (eptr->edata_len - 2))
  {
    tlen = end_pos - start_pos + 1;
    partial = 1;
  }
  else
  {
    tlen = eptr->edata_len - start_pos - 1;
  }

  y = eptr->y + eptr->y_offset;

  x = x - hw->html.scroll_x;
  y = y - hw->html.scroll_y;

#ifndef NO_EXTRA_FILLS
  {
    int dir, nascent, descent;
    XCharStruct all;
    int height;

    /*
     * May be safe to use the cached full width of this
     * string, and thus avoid a call to XTextExtents
     */
    if ((!partial) && (eptr->width != 0))
    {
      all.width = eptr->width;
    }
    else
    {
#ifdef ASSUME_FIXED_WIDTH_PRE
      if (eptr->font == hw->html.plain_font)
      {
	all.width = eptr->font->max_bounds.width * tlen;
      }
      else
      {
	XTextExtents (eptr->font, (char *) tdata,
		      tlen, &dir, &nascent, &descent, &all);
      }
#else
      XTextExtents (eptr->font, (char *) tdata,
		    tlen, &dir, &nascent, &descent, &all);
#endif /* ASSUME_FIXED_WIDTH_PRE */
    }

    XSetForeground (XtDisplay (hw), hw->html.drawGC, bg);

    height = (eptr->font->max_bounds.ascent - eptr->font->ascent);
    if (height > 0)
    {
      XFillRectangle (XtDisplay (hw->html.view),
		      XtWindow (hw->html.view),
		      hw->html.drawGC, x, y,
		      (unsigned int) all.width, (unsigned int) height);
    }
    height = (eptr->font->max_bounds.descent - eptr->font->descent);
    if (height > 0)
    {
      XFillRectangle (XtDisplay (hw->html.view),
		      XtWindow (hw->html.view),
		      hw->html.drawGC, x,
		      (int) (y + eptr->font->max_bounds.ascent +
			     eptr->font->descent),
		      (unsigned int) all.width, (unsigned int) height);
    }
    width = all.width;
  }
#endif /* NO_EXTRA_FILLS */

  XSetForeground (XtDisplay (hw), hw->html.drawGC, fg);
  XSetBackground (XtDisplay (hw), hw->html.drawGC, bg);

  if (sip > 0)
    XDrawImageString (XtDisplay (hw->html.view), XtWindow (hw->html.view),
		      hw->html.drawGC,
		      x, y + ascent,
		      (char *) tdata, tlen);
  if (sip != 1)
    XDrawString (XtDisplay (hw->html.view), XtWindow (hw->html.view),
		      hw->html.drawGC,
		      x, y + ascent,
		      (char *) tdata, tlen);

  if (eptr->underline_number)
  {
    int i, ly;

    if (eptr->dashed_underline)
    {
      XSetLineAttributes (XtDisplay (hw), hw->html.drawGC, 1,
			  LineDoubleDash, CapButt, JoinBevel);
    }
    else
    {
      XSetLineAttributes (XtDisplay (hw), hw->html.drawGC, 1,
			  LineSolid, CapButt, JoinBevel);
    }

    if (width == -1)
    {
      int dir, nascent, descent;
      XCharStruct all;

#ifdef ASSUME_FIXED_WIDTH_PRE
      if (eptr->font == hw->html.plain_font)
      {
	all.width = eptr->font->max_bounds.width * tlen;
      }
      else
      {
	XTextExtents (eptr->font, (char *) tdata,
		      tlen, &dir, &nascent, &descent, &all);
      }
#else
      XTextExtents (eptr->font, (char *) tdata,
		    tlen, &dir, &nascent, &descent, &all);
#endif /* ASSUME_FIXED_WIDTH_PRE */
      width = all.width;
    }

    ly = (int) (y + eptr->font->max_bounds.ascent +
		eptr->font->descent - 1);

    for (i = 0; i < eptr->underline_number; i++)
    {
      XDrawLine (XtDisplay (hw->html.view),
		 XtWindow (hw->html.view), hw->html.drawGC,
		 x, ly, (int) (x + width), ly);
      ly -= 2;
    }
  }

  if (eptr->strikeout == True)
  {
    int ly;

    XSetLineAttributes (XtDisplay (hw), hw->html.drawGC, 1,
			LineSolid, CapButt, JoinBevel);

    if (width == -1)
    {
      int dir, nascent, descent;
      XCharStruct all;

#ifdef ASSUME_FIXED_WIDTH_PRE
      if (eptr->font == hw->html.plain_font)
      {
	all.width = eptr->font->max_bounds.width * tlen;
      }
      else
      {
	XTextExtents (eptr->font, (char *) tdata,
		      tlen, &dir, &nascent, &descent, &all);
      }
#else
      XTextExtents (eptr->font, (char *) tdata,
		    tlen, &dir, &nascent, &descent, &all);
#endif /* ASSUME_FIXED_WIDTH_PRE */
      width = all.width;
    }

    ly = (int) (y + eptr->font->max_bounds.ascent +
		eptr->font->descent - 1);
    ly = ly - ((hw->html.font->max_bounds.ascent +
		hw->html.font->descent) / 2);

    XDrawLine (XtDisplay (hw->html.view), XtWindow (hw->html.view),
	       hw->html.drawGC,
	       x, ly, (int) (x + width), ly);
  }
}


/*
 * Redraw a formatted text element
 */
void
TextRefresh (hw, eptr, start_pos, end_pos, sip)
HTMLWidget hw;
struct ele_rec *eptr;
int start_pos, end_pos;
int sip;
{
  if (eptr->selected == False)
  {
    PartialRefresh (hw, eptr, start_pos, end_pos,
		    eptr->fg, eptr->bg, sip);
  }
  else if ((start_pos >= eptr->start_pos) && (end_pos <= eptr->end_pos))
  {
    PartialRefresh (hw, eptr, start_pos, end_pos,
		    eptr->bg, eptr->fg, sip | 2);
  }
  else
  {
    if (start_pos < eptr->start_pos)
    {
      PartialRefresh (hw, eptr, start_pos, eptr->start_pos - 1,
		      eptr->fg, eptr->bg, sip);
      start_pos = eptr->start_pos;
    }
    if (end_pos > eptr->end_pos)
    {
      PartialRefresh (hw, eptr, eptr->end_pos + 1, end_pos,
		      eptr->fg, eptr->bg, sip);
      end_pos = eptr->end_pos;
    }
    PartialRefresh (hw, eptr, start_pos, end_pos,
		    eptr->bg, eptr->fg, sip | 2);
  }
}


/*
 * Redraw a formatted bullet element
 */
void
BulletRefresh (hw, eptr)
HTMLWidget hw;
struct ele_rec *eptr;
{
  int width, line_height;
  int x1, y1;

  width = eptr->width;
  line_height = eptr->line_height;

  x1 = eptr->x;
  y1 = eptr->y + eptr->y_offset + (line_height / 2) - (width / 4);
  x1 -= hw->html.scroll_x;
  y1 -= hw->html.scroll_y;
  XSetFont (XtDisplay (hw), hw->html.drawGC, eptr->font->fid);
  XSetForeground (XtDisplay (hw), hw->html.drawGC, eptr->fg);
  XSetBackground (XtDisplay (hw), hw->html.drawGC, eptr->bg);
  if (eptr->indent_level < 2)
  {
    XFillArc (XtDisplay (hw->html.view), XtWindow (hw->html.view),
	      hw->html.drawGC,
	      x1, y1, (width / 2), (width / 2), 0, 23040);
  }
  else if (eptr->indent_level == 2)
  {
    XSetLineAttributes (XtDisplay (hw), hw->html.drawGC, 1,
			LineSolid, CapButt, JoinBevel);
    XDrawRectangle (XtDisplay (hw->html.view),
		    XtWindow (hw->html.view), hw->html.drawGC,
		    x1, y1, (width / 2), (width / 2));
  }
  else				/* else (eptr->indent_level > 2) */
  {
    XSetLineAttributes (XtDisplay (hw), hw->html.drawGC, 1,
			LineSolid, CapButt, JoinBevel);
    XDrawArc (XtDisplay (hw->html.view), XtWindow (hw->html.view),
	      hw->html.drawGC,
	      x1, y1, (width / 2), (width / 2), 0, 23040);
  }
}


/*
 * Redraw a formatted horizontal rule element
 */
void
HRuleRefresh (hw, eptr)
HTMLWidget hw;
struct ele_rec *eptr;
{
  int width, height;
  int x1, y1;

  width = eptr->width;
  if (width < 0)
  {
    /*
     * If this ever happens, somebody's introduced a bug.  Complain.  (WBE)
     */
    fprintf (stderr, "\aBad width (%d) in <hr>\n\a", width);
    width = 0;
  }

  x1 = eptr->x - hw->html.scroll_x;
  y1 = eptr->y - hw->html.scroll_y;
  height = eptr->line_height;

  /* blank out area */
  XSetForeground (XtDisplay (hw), hw->html.drawGC, eptr->bg);
  XFillRectangle (XtDisplay (hw->html.view), XtWindow (hw->html.view),
		  hw->html.drawGC, x1, y1, width, height);
  y1 += (height / 2) - 1;

  XSetLineAttributes (XtDisplay (hw), hw->html.drawGC, 1,
		      LineSolid, CapButt, JoinBevel);
  /* changing the GC back and forth is not the most efficient way.... */
  XSetForeground (XtDisplay (hw), hw->html.drawGC, eptr->fg);
  XDrawLine (XtDisplay (hw->html.view), XtWindow (hw->html.view),
	     hw->html.drawGC,
	     x1, y1, (int) (x1 + width), y1);
  XDrawLine (XtDisplay (hw->html.view), XtWindow (hw->html.view),
	     hw->html.drawGC,
	     x1, y1 + 1, (int) (x1 + width), y1 + 1);
}


/*
 * Redraw a formatted image element.
 * The color of the image border reflects whether it is an active anchor
 * or not.
 */
void
ImageRefresh (hw, eptr)
HTMLWidget hw;
struct ele_rec *eptr;
{
  if (eptr->img_data != NULL)	/* if img_data is non-NULL, pic_data is too */
  {
    int x, y, extra;
    ImageInfo *pic_data = eptr->img_data->pic_data;

    x = eptr->x;
    y = eptr->y + eptr->y_offset;

    if (hw->html.border_images == True  || eptr->anchorHRef != NULL)
    {
      extra = IMAGE_BORDER;
    }
    else
    {
      extra = 0;
    }

    x = x - hw->html.scroll_x;
    y = y - hw->html.scroll_y;

    XSetForeground (XtDisplay (hw), hw->html.drawGC, eptr->fg);
    XSetBackground (XtDisplay (hw), hw->html.drawGC, eptr->bg);
    XFillRectangle (XtDisplay (hw->html.view),
		    XtWindow (hw->html.view), hw->html.drawGC,
		    x, y,
		    (pic_data->width + (2 * extra)),
		    extra);
    XFillRectangle (XtDisplay (hw->html.view),
		    XtWindow (hw->html.view), hw->html.drawGC,
		    x,
		    (y + pic_data->height + extra),
		    (pic_data->width + (2 * extra)),
		    extra);
    XFillRectangle (XtDisplay (hw->html.view),
		    XtWindow (hw->html.view), hw->html.drawGC,
		    x, y,
		    extra,
		    (pic_data->height + (2 * extra)));
    XFillRectangle (XtDisplay (hw->html.view),
		    XtWindow (hw->html.view), hw->html.drawGC,
		    (x + pic_data->width + extra),
		    y,
		    extra,
		    (pic_data->height + (2 * extra)));

    /*
     * Pixmap creation at the X server was put off until now to make sure
     * we had a window.  If it hasn't been created already, make the
     * Pixmap now.
     */
    if (pic_data->image == (Pixmap) NULL)
    {
      if (pic_data->image_data != NULL)
      {
	/*
	 * All real images do this the first time through
	 */
	pic_data->image = InfoToImage (hw, pic_data);
	if (pic_data->image == (Pixmap) NULL)  /* failed? */
	{
	  pic_data = NoImageData ();  /* give up, use noimage icon */
	}
      }
      /*
       * First use of one of the internal images?
       */
      if (pic_data->image == (Pixmap) NULL  &&  pic_data->internal)
      {
	BuildInternalImagePixmaps (hw);  /* updates their pic_data->image */
      }
#if 0
      if (pic_data->image == (Pixmap) NULL  &&  !pic_data->internal)
      {
	/*
	 * Once upon a time there was code here that said if
	 * pic_data->fetched, call resolveDelayedImage (hw, eptr->edata),
	 * copy ismap and fptr to the new img_data, and free the old one.
	 * The explanation below was offered for why this was being done.
	 * I simply don't believe it can happen given the current code.
	 * -WBE  97May3
	 *
	 * Could be that the user opened another window, and the Pixmap was
	 * freed, and then they overflowed the cache, and the XImage data
	 * was freed.  If this image was ever successfully fetched, try
	 * again before giving up.
	 */
      }
#endif
    }
    /* pic_data->image is now non-NULL unless out of memory or worse */

    if (pic_data->image != (Pixmap) NULL)
    {
      XCopyArea (XtDisplay (hw->html.view),
		 pic_data->image,
		 XtWindow (hw->html.view), hw->html.drawGC, 0, 0,
		 pic_data->width, pic_data->height,
		 (x + extra),
		 (y + extra));

    }
    if (pic_data->delayed   &&  /* if delayed and "has real anchor" */
	eptr->anchorHRef != NULL  &&
	!IsDelayedHRef (hw, eptr->anchorHRef)  &&
	!IsIsMapForm (hw, eptr->anchorHRef) )
    {
      XSetForeground (XtDisplay (hw), hw->html.drawGC,
		      eptr->fg);
      XFillRectangle (XtDisplay (hw->html.view),
		      XtWindow (hw->html.view), hw->html.drawGC,
		      x, (y + AnchoredHeight (hw)),
		      (pic_data->width + (2 * extra)),
		      extra);
    }
  }
}

/*
 * RefreshTextRange -- apparently not used anywhere
 * --GN 1997May05
 */
void
RefreshTextRange (hw, start, end, sip)
HTMLWidget hw;
struct ele_rec *start;
struct ele_rec *end;
int sip;
{
  struct ele_rec *eptr;

  eptr = start;
  while ((eptr != NULL) && (eptr != end))
  {
    if (eptr->type == E_TEXT)
    {
      TextRefresh (hw, eptr, 0, (eptr->edata_len - 2), sip);
    }
    eptr = eptr->next;
  }
  if (eptr != NULL)
  {
    if (eptr->type == E_TEXT)
    {
      TextRefresh (hw, eptr,  0, (eptr->edata_len - 2), sip);
    }
  }
}


/*
 * Refresh all elements on a single line into the widget's window
 */
void
PlaceLine (hw, line, x0, y0, xmax, ymax)
HTMLWidget hw;
int line, x0, y0, xmax, ymax;
{
  struct ele_rec *eptr;

  /*
   * Item list for this line
   */
  eptr = hw->html.line_array[line];

  while ((eptr != NULL) && (eptr->line_number == (line + 1)))
  {
    if ( !(ymax < eptr->y  ||
	   xmax < eptr->x  ||
	   y0 > (eptr->y + eptr->y_offset + eptr->line_height)  ||
	   x0 > (eptr->x + eptr->width) ))
	switch (eptr->type)
	{
	  case E_TEXT:
	    TextRefresh (hw, eptr,  0, (eptr->edata_len - 2), 0);
	    break;
	  case E_BULLET:
	    BulletRefresh (hw, eptr);
	    break;
	  case E_HRULE:
	    HRuleRefresh (hw, eptr);
	    break;
	  case E_LINEFEED:
	    LinefeedRefresh (hw, eptr);
	    break;
	  case E_IMAGE:
	    ImageRefresh (hw, eptr);
	    break;
	  case E_WIDGET:
	    WidgetRefresh (hw, eptr);
	    break;
	}
    eptr = eptr->next;
  }
}


/*
 * Locate the element (if any) at the passed location in the widget.
 * If OEP is NULL, [x,y] must be wholly inside displayed element.  (This
 *    is suitable for anchor-related matches.)
 * If OEP is non-NULL, return the element before or after [x,y]
 *    visually if no exact match.  (This is suitable for Selecting text.)
 * If there is no corresponding element, return NULL.
 * If an element is found, also return the position of the character the
 *    cursor is at in *pos.
 */
struct ele_rec *
LocateElement (hw, x, y, pos, oep)
HTMLWidget hw;
int x, y;
int *pos;
struct ele_rec *oep;
{
  struct ele_rec *eptr;
  struct ele_rec *rptr;
  int i, start, end, line, guess;

  x = x + hw->html.scroll_x;
  y = y + hw->html.scroll_y;

/* printf (" find %d,%d:", y, x); /*DEBUG*/

  /*
   * Narrow the search down to a 2 line range
   * before beginning to search element by element
   */
  start = -1;
  end = -1;

  /*
   * Heuristic to guess the starting line.  Assumes lines of just text;
   * overestimates if there are large objects or tables.  Still, the larger
   * the document, the more likely this estimate will speed up the search.
   */
  guess = y / (hw->html.font->max_bounds.ascent +
	       hw->html.font->max_bounds.descent);
  if (guess > (hw->html.doc_line_count - 1))
  {
    guess = hw->html.doc_line_count - 1;
  }
  while (guess > 0)
  {
    if ((hw->html.line_array[guess] != NULL) &&
	(hw->html.line_array[guess]->y <= y))
    {
      break;
    }
    guess--;
  }
  if (guess < 0)
  {
    guess = 0;
  }

  for (i = guess; i < hw->html.doc_line_count; i++)
  {
    if (hw->html.line_array[i] == NULL)
    {
      continue;
    }
    else if (hw->html.line_array[i]->y <= y)
    {
      start = i;
      continue;
    }
    else
    {
      end = i;
      break;
    }
  }

  /*
   * Search may have already failed, or it may be a one line
   * range.
   */
  if ((start == -1) && (end == -1))
  {
    return (NULL);
  }
  else if (start == -1)
  {
    start = end;
  }
  else if (end == -1)
  {
    end = start;
  }

  /*
   * Search element by element; for now we only search
   * text elements, images, and linefeeds.
   */
  for (rptr = NULL, eptr = hw->html.line_array[start];
       (eptr != NULL)  &&  (eptr->line_number <= (end + 1));
       eptr = eptr->next)
  {
    int tx1, tx2, ty1, ty2;
    /*
     * If matching on logically closest appropriate item, always
     * match last element.
     */
    if (oep != NULL  &&  eptr->next == NULL)
    {
      rptr = eptr;
      break;
    }

    if (eptr->internal == True)   continue;

    tx1 = eptr->x;
    tx2 = tx1 + eptr->width;
    ty1 = eptr->y + eptr->y_offset;
    ty2 = ty1 + eptr->line_height;

    if (eptr->type == E_TEXT  ||  eptr->type == E_IMAGE)
    {
      if (x >= tx1  &&  x <= tx2  &&  y >= ty1  &&  y <= ty2)
      {
	rptr = eptr;
	break;			/* exact match; exit */
      }
    }
    else if (eptr->type == E_LINEFEED)
    {
      int w = eptr->width;

      if (eptr->next != NULL)	/* if there's a next line */
      {
	int tx3 = eptr->next->x,  /* start of next line */
	    ty3 = eptr->next->y;

	/* extend this LF's region to top of following line */
	if (ty3 > ty2  &&  (tx3 < tx2  ||  w == 0))  ty2 = ty3;
      }
      /*
       * If (x,y) to the right of this LF, stop.
       * Previously included the region to left of next line:
       *   (x < tx3  &&  y >= ty3  &&  y <= ty3 + eptr->next->line_height)
       * but doesn't any more.
       */
      if (y >= ty1  &&  x >= tx1  &&  (w == 0  || x <= tx2)  &&  y <= ty2)
      {
	rptr = eptr;
	break;			/* exact match; exit */
      }
    }
#if 0
    else			/* make tx2 and ty2 something valid  */
    {
      tx2 = tx1 + eptr->width;
      ty2 = ty1;
    }
#endif
    /*
     * Deal with the case of an outer table cell containing a [table with
     * a multi-line column] followed by anything else
     * when seeking a best approximate (end of column) match.
     */
    if (rptr != NULL  &&  eptr->x <= x  &&  ty1 <= y)   rptr = NULL;

    /*
     * If we've just checked the last item that had any chance of matching,
     * and it didn't match, stop now.
     */
    if (x < tx2  &&  y < ty2)
    {
      if (oep != NULL  &&  rptr == NULL)
      {
	rptr = eptr;
	/*
	 * The cursor is logically prior to the current element.
	 * If doing a "forward" select (current element ID > passed ID),
	 * back up to the first preceding element of a matchable type.
	 */
	if (oep->ele_id < rptr->ele_id)
	    do {
	      rptr = rptr->prev;
	      } while (rptr != NULL  &&
		       rptr->type != E_TEXT  &&
		       rptr->type != E_LINEFEED &&
		       rptr->type != E_IMAGE);
      }
      break;			/* always stop; rptr set if oep non-NULL */
    }
    /*
     * If matching on logically closest appropriate item, match on last
     * item in the current table column if X is to the left of the first
     * item in the next column.  If rptr already set, we're at the bottom
     * of a column to the right of the cursor, so it's not as good a match
     * as the previous rptr.
     */
    if (oep != NULL  &&  rptr == NULL  &&
	eptr->next->y < eptr->y  &&  x < eptr->next->x - 4)
    {
      rptr = eptr;
      /*
       * The cursor is logically after the current element.
       * If doing a "backward" select (current element ID < passed ID),
       * advance to the first following element of a matchable type.
       */
      if (rptr->ele_id < oep->ele_id)
	  do {
	    rptr = rptr->next;
	    } while (rptr != NULL  &&
		     rptr->type != E_TEXT  &&
		     rptr->type != E_LINEFEED &&
		     rptr->type != E_IMAGE);
      /* inexact match; keep searching */
    }
  }

  /*
   * If we matched an element, locate the exact character position within
   * that element (if it's a text element).
   */
  if (rptr != NULL)
  {

/* printf (" matched %d/[%d,%d]-%c'%s' ", rptr->line_number, rptr->y + rptr->y_offset,rptr->x, "0tbLiWh"[rptr->type], (rptr->type == E_TEXT ?  rptr->edata : ""));  fflush(stdout); /*DEBUG*/

    if (rptr->type != E_TEXT  ||  x < rptr->x)
    {
      *pos = 0;
    }
    else
    {
      int dir, ascent, descent;
      XCharStruct all;
      int epos;

      /*
       * Start assuming fixed width font.  The real position should
       * always be <= to this, but just in case, start at the end
       * of the string if it is not.
       */
      epos = ((x - rptr->x) / rptr->font->max_bounds.width) + 1;
      if (epos >= rptr->edata_len - 1)
      {
	epos = rptr->edata_len - 2;
      }
      XTextExtents (rptr->font, (char *) rptr->edata,
		    (epos + 1), &dir, &ascent, &descent, &all);
      if (x > (int) (rptr->x + all.width))
      {
	epos = rptr->edata_len - 3;
      }
      else
      {
	epos--;
      }

      while (epos >= 0)
      {
	XTextExtents (rptr->font, (char *) rptr->edata,
		      (epos + 1), &dir, &ascent, &descent, &all);
	if ((int) (rptr->x + all.width) <= x)
	{
	  break;
	}
	epos--;
      }
      epos++;
      *pos = epos;
    }
  }
  return (rptr);
}

struct expstring {
  char *s;			/* an expandable string */
  int len, maxlen, incr;
} static const null_expstring = { NULL, 0, 0, 150 };


/*
 * Subroutine for ParseTextToString.  Append s2 to end of ep->s, reallocating
 * ep->s if needed.  Update the associated current length and maximum length.
 */
static void
strcpy_or_grow (ep, s2)
struct expstring *const ep;
const char *const s2;
{
  int newlen, s2len;

  if (s2 == NULL)  return;

  s2len = strlen (s2);
  newlen = s2len + ep->len;	/* not incl. final NUL */
  if (newlen + 1 >= ep->maxlen)	 /* incl. final NUL */
  {
    int maxlen = (newlen / ep->incr + 1) * ep->incr;
    if (ep->s != NULL)
    {
      ep->s = (char *) XtRealloc (ep->s, maxlen * sizeof (char));
    }
    else
    {
      ep->s = (char *) XtMalloc (maxlen * sizeof (char));
    }
    ep->maxlen = maxlen;
  }

  if (s2len > 0)   memcpy ((char *) (ep->s + ep->len), s2, s2len + 1);

  ep->len = newlen;
}


/*
 * ToCol: Add tabs and spaces to end of line to advance to specified column
 */
static void
ToCol (epp, atcol, newcol)
struct expstring *epp;
int atcol, newcol;
{
  int tabcol = (newcol & ~7) - 1;

  while (atcol < tabcol)
  {
    strcpy_or_grow (epp, "\t");
    atcol = (atcol | 7) + 1;
  }
  if (atcol < newcol)
      strcpy_or_grow (epp, "        " + 8 + atcol - newcol);
}


/*
 * Parse all the formatted text elements from start to end into an ascii
 * text string, and return it.
 * If "hw" is supplied, text is prettied up to show headers and the like.
 * space_width and lmargin tell us how many spaces to indent lines.
 */
char *
ParseTextToString (hw, startp, endp, start_pos, end_pos, em)
HTMLWidget hw;			/* doubles as "pretty" boolean */
struct ele_rec *startp;
struct ele_rec *endp;		/* NULL means through end */
int start_pos, end_pos;
int em;				/* max. width of characters in this font */
{
  const Boolean pretty = (hw != NULL);
  const int en = (em + 1) / 2;	/* "en" is a standard printing measure */
  const int height = 10 + 2;	/* a small font (10pt + 1pt for spacing) */
  struct expstring result = null_expstring;
  int i, epos, maxlines, xmin, lineadj;
  struct L {
    int xend, lastcol;		/* logical end of line */
    struct expstring content;
    } *ldp, *ldesc;
  struct ele_rec *start, *end, *last, *bol;
  Boolean needLF;

  if (startp == NULL)
  {
    return NULL;
  }

  result.incr = 1024;		/* use bigger increments */

  if (SwapElements (startp, endp, start_pos, end_pos))
  {
    start = endp;
    end = startp;
    epos = start_pos;
    start_pos = end_pos;
    end_pos = epos;
  }
  else
  {
    start = startp;
    end = endp;
  }

  if (pretty)
  {
    /*
     * We need to know if we should consider the indentation or bullet
     * that might be just before the first selected element to also be
     * selected.  This current hack looks to see if they selected the
     * whole line, and assumes if they did, they also wanted the beginning.
     *
     * If we are at the beginning of the list, or the beginning of
     * a line, or just behind a bullet, assume this is the start of
     * a line that we may want to include the indent for.
     */
    if ((start_pos == 0) &&
	((start->prev == NULL) || (start->prev->type == E_BULLET) ||
	 (start->prev->line_number != start->line_number)))
    {
      struct ele_rec *eptr = start;
      while ((eptr != NULL) && (eptr != end) &&
	     (eptr->type != E_LINEFEED))
      {
	eptr = eptr->next;
      }
      if ((eptr != NULL) && (eptr->type == E_LINEFEED))
      {
	if ((start->prev != NULL) &&
	    (start->prev->type == E_BULLET))
	{
	  start = start->prev;
	}
      }
    }
  }

  last = (end != NULL) ?  end->next : NULL;

  xmin = start->x;
  for (bol = start->next;  bol != last;  bol = bol->next)
  {
    if (bol->x < xmin)   xmin = bol->x;
  }

  needLF = False;
  for (bol = start;  bol != last;  )
  {
    struct ele_rec *eolptr, *eptr = bol;

    /*
     * Pass 1: compute actual #lines on next logical line by
     * finding (ymax - ymin) / height
     */
    int ymin = eptr->y + eptr->y_offset,
	ymax = ymin,
	curline = eptr->line_number;
    while (eptr != last)
    {
      Boolean wantit = False;
      eptr = eptr->next;
      if (eptr == NULL)
      {
	break;
      }
      if (eptr == last  ||  eptr->line_number != curline)
      {
	eptr = eptr->prev;
	break;
      }
      if (eptr->internal == True)   continue;

      switch (eptr->type) {
	/* ?maybe calculate height as Min(height, font.ascend+descend)? */
	case E_TEXT:      wantit = True;     break;
	case E_LINEFEED:  wantit = True;     break;
	case E_BULLET:    wantit = pretty;   break;
	}
      if (wantit == True)
      {
	int y = eptr->y + eptr->y_offset;
	if (y < ymin)       ymin = y;
	else if (y > ymax)  ymax = y;
      }
    }
    eolptr = (eptr != NULL) ?  eptr->next : NULL;

    if (ymax == ymin)
	maxlines = 1;
    else
	maxlines = (ymax - ymin) / height + 1 + 1;  /* extra lines */
    maxlines += maxlines;	/* double, in case everything's underlined! */

    ldesc = (struct L *) XtMalloc (sizeof(struct L) * maxlines);
    for (i = 0, ldp = ldesc;  i < maxlines;  ++ldp, ++i)
    {
      ldp->xend = xmin;
      ldp->lastcol = 0;
      ldp->content = null_expstring;
    }

    /*
     * Pass 2: place text in array of lines
     */
    lineadj = 0;
    for (eptr = bol;  eptr != eolptr;  eptr = eptr->next)
    {
      char *newtext1 = NULL, *newtext2 = NULL;
      char tchar, *tend = NULL;

      if (eptr->prev != NULL  &&  eptr->y < eptr->prev->y)
      {
	lineadj = 0;
      }

      if (eptr->internal == True)  continue;

      switch (eptr->type) {
	case E_LINEFEED:
	  newtext1 = "\n";
	  break;

	case E_BULLET:
	  if (pretty)  newtext1 = "* ";	 /* WBE: I prefer this to "o " */
	  break;

	case E_TEXT:
	  newtext1 = eptr->edata + ((eptr == start) ? start_pos : 0);
	  if (eptr == end)
	  {
	    tend = (char *) (eptr->edata + end_pos + 1);
	    tchar = *tend;
	    *tend = '\0';
	  }
	  if (pretty  &&  *newtext1 != '\0')
	  {
	    char lchar;
	    if      (eptr->font == hw->html.header1_font)   lchar = '*';
	    else if (eptr->font == hw->html.header2_font)   lchar = '=';
	    else if (eptr->font == hw->html.header3_font)   lchar = '+';
	    else if (eptr->font == hw->html.header4_font)   lchar = '-';
	    else if (eptr->font == hw->html.header5_font)   lchar = '~';
	    else if (eptr->font == hw->html.header6_font)   lchar = '.';
	    else lchar = '\0';
	    if (lchar != '\0')
	    {
	      int n = strlen (newtext1);
	      char *p = XtMalloc ((n+2) * sizeof(char));
	      newtext2 = p;
	      for ( ;  n > 0;  --n)   *p++ = lchar;
/*	      *p = '\n';	/* optional, safe */
	      *p = '\0';
	    }
	  }
	  break;
      }
      if (newtext1 != NULL)
      {
	int line = lineadj + ((eptr->y + eptr->y_offset) - ymin) / height;
	if (line >= maxlines -1)
	{
	  fprintf (stderr, "\aProblem in conversion to string: L%d? (>%d)\n\a",
		   line, maxlines - 2);
	  fflush (stderr); 
	  line = maxlines - 2;
	}
	ldp = ldesc + line;

	if (*newtext1 == '\n')
	{
	  int len = ldp->content.len;
	  if (len == 0  ||  ldp->content.s[len - 1] != '\n')
	  {
	    strcpy_or_grow (&ldp->content, newtext1);
	  }
	}
	else if (*newtext1 != '\0')  /* >0 chars to add to end of line */
	{
	  /*
	   * If elements are more than a few pixel apart, they're
	   * probably not related -- use absolute column positioning to get
	   * table items to line up (most of the time).
	   * If elements have little space between them, assume they're
	   * related and insert a space if needed.
	   */
	  int col1;
	  int len = eptr->x - ldp->xend + 1;  /* spacing between elements */

	  while (len < 0  &&  line < maxlines - 2)
	  {
	    ++line, ++lineadj, ++ldp;
	    len = eptr->x - ldp->xend + 1;
	  }
	  col1 = (len <= en) ? ldp->lastcol : ((eptr->x - xmin) / en);

	  if (newtext2 != NULL  &&  col1 < ldp[1].lastcol)
	  {
	    col1 = ldp[1].lastcol;
	  }

	  len = ldp->content.len;
	  if (len > 0  &&  ldp->content.s[len - 1] == '\n')
	  {
	    ldp->content.len = len - 1;  /* flush mid-line LFs (tables) */
	  }

	  ToCol (&ldp->content, ldp->lastcol, col1);
	  ldp->xend = eptr->x + eptr->width;
	  ldp->lastcol = col1 + strlen (newtext1);
	  strcpy_or_grow (&ldp->content, newtext1);
	  if (newtext2 != NULL)
	  {
	    len = ldp[1].content.len;
	    if (len > 0  &&  ldp[1].content.s[len - 1] == '\n')
	    {
	      ldp[1].content.len = len - 1;
	    }
	    ToCol (&ldp[1].content, ldp[1].lastcol, col1);
	    ldp[1].xend = ldp->xend;
	    ldp[1].lastcol = ldp->lastcol;
	    strcpy_or_grow (&ldp[1].content, newtext2);
	    XtFree (newtext2);
	  }
	}
	if (tend != NULL)	/* restore the string we clobbered earlier */
	    *tend = tchar;
      }
    }
    /*
     * finally, copy the lines to 'result'
     */
    for (i = 0, ldp = ldesc;  i < maxlines;  ++ldp, ++i)
    {
      if (ldp->content.s != NULL)
      {
	int len;
	if (needLF)
	{
	  strcpy_or_grow (&result, "\n");
	}
	strcpy_or_grow (&result, ldp->content.s);

	/*
	 * Programming note: Sure, you could change TableRowClose to always
	 * add a Linefeed, but then you'd be paying the cost of building and
	 * processing those element records ALL the time.  This way, the
	 * cost is only paid when printing or selecting, not when viewing.
	 */
	len = ldp->content.len;
	needLF = (len == 0  ||  ldp->content.s[len - 1] != '\n');

	XtFree (ldp->content.s);
      }
    }
    XtFree ((char *)ldesc);

    bol = eolptr;
  }

  return result.s;
}

/*
 * Find the preferred width of a parsed HTML document
 * Currently unformatted plain text, unformatted listing text, plain files
 * and preformatted text require special width.
 * Preferred width = (width of longest plain text line in document) *
 *      (width of that text's font)
 */
int
DocumentWidth (hw, list)
HTMLWidget hw;
struct mark_up *list;
{
  struct mark_up *mptr;
  int plain_text;
  int listing_text;
  int pcnt, lcnt, pwidth, lwidth;
  int width;
  char *ptr;

  /*
   * Loop through object list looking at the plain, preformatted,
   * and listing text
   */
  width = 0;
  pwidth = 0;
  lwidth = 0;
  plain_text = 0;
  listing_text = 0;
  mptr = list;
  while (mptr != NULL)
  {
    /*
     * All text blocks between the starting and ending
     * plain and pre text markers are plain text blocks.
     * Manipulate flags so we recognize these blocks.
     */
    if ((mptr->type == M_PLAIN_TEXT) ||
	(mptr->type == M_PLAIN_FILE) ||
	(mptr->type == M_PREFORMAT))
    {
      if (mptr->is_end)
      {
	plain_text--;
	if (plain_text < 0)
	{
	  plain_text = 0;
	}
      }
      else
      {
	plain_text++;
      }
      pcnt = 0;
      lcnt = 0;
    }
    /*
     * All text blocks between the starting and ending
     * listing markers are listing text blocks.
     */
    else if (mptr->type == M_LISTING_TEXT)
    {
      if (mptr->is_end)
      {
	listing_text--;
	if (listing_text < 0)
	{
	  listing_text = 0;
	}
      }
      else
      {
	listing_text++;
      }
      lcnt = 0;
      pcnt = 0;
    }
    /*
     * If this is a plain text block, add to line length.
     * Find the Max of all line lengths.
     */
    else if ((plain_text) && (mptr->type == M_NONE))
    {
      ptr = mptr->text;
      while ((ptr != NULL) && (*ptr != '\0'))
      {
	ptr = MaxTextWidth (ptr, &pcnt);
	if (pcnt > pwidth)
	{
	  pwidth = pcnt;
	}
      }
    }
    /*
     * If this is a listing text block, add to line length.
     * Find the Max of all line lengths.
     */
    else if ((listing_text) && (mptr->type == M_NONE))
    {
      ptr = mptr->text;
      while ((ptr != NULL) && (*ptr != '\0'))
      {
	ptr = MaxTextWidth (ptr, &lcnt);
	if (lcnt > lwidth)
	{
	  lwidth = lcnt;
	}
      }
    }
    mptr = mptr->next;
  }
  width = pwidth * hw->html.plain_font->max_bounds.width;
  lwidth = lwidth * hw->html.listing_font->max_bounds.width;
  if (lwidth > width)
  {
    width = lwidth;
  }
  return (width);
}
