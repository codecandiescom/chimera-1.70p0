/* substantially modified by WBE, Spring 1997, and by others before me */

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

#include "common.h"

#include <stdio.h>
#include <ctype.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Toggle.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/List.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>

#include "ScrollText.h"

#include "HTMLP.h"

#define STRING String

#define X_NAME		"x"
#define Y_NAME		"y"

#define	W_TEXTFIELD	0
#define	W_CHECKBOX	1
#define	W_RADIOBOX	2
#define	W_PUSHBUTTON	3
#define	W_PASSWORD	4
#define	W_OPTIONMENU	5
#define	W_TEXTAREA	6
#define	W_LIST		7
#define	W_JOT		8
#define	W_HIDDEN	9

extern char *ParseMarkTag ();

void FreeCommaList _ArgProto((char **, int));

static char **ParseCommaList _ArgProto((char *, int *));
static char *MapOptionReturn _ArgProto((char *, char **));
static void setTextSize _ArgProto((Widget, int, int));
static Widget GetAsciiTextWidget _ArgProto((Widget));

#define FONTHEIGHT(font) (font->max_bounds.ascent + font->max_bounds.descent)

static Widget
GetAsciiTextWidget(w)
Widget w;
{
  Widget rw;

  if (XtIsSubclass(w, scrollingTextWidgetClass))
  {
    XtVaGetValues(w, XtNtextWidget, &rw, NULL);
  }
  else
  {
    rw = w;
  }
  return(rw);
}

static void
setTextSize (w, columns, lines)
Widget w;
int columns;
int lines;
{
  XFontStruct *font = NULL;
  Position lm, rm, tm, bm;
  Dimension width, height;
  Widget rw;

  rw = GetAsciiTextWidget(w);

  XtVaGetValues (rw,
		 XtNfont, &font,
		 XtNleftMargin, &lm,
		 XtNrightMargin, &rm,
		 XtNtopMargin, &tm,
		 XtNbottomMargin, &bm,
		 NULL);

  if (font != NULL)
  {
    width = rm + lm + columns * XTextWidth(font, "0", 1);
/*
    width = rm + lm + columns *
	(font->max_bounds.lbearing + font->max_bounds.rbearing);
*/
    height = tm + bm + lines * FONTHEIGHT (font);
  }
  else
  {
    width = 200;
    height = 25;
  }

  XtVaSetValues (w,
		 XtNwidth, width,
		 XtNheight, height,
		 NULL);
}

void
CBListDestroy (w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
  char **string_list, **p;
  int item_count;

  XtVaGetValues (w,
		 XtNlist, &string_list,
		 XtNnumberStrings, &item_count,
		 NULL);

  p = string_list;
  while (item_count > 0)
  {
    XtFree(*p++);
    item_count--;
  }
  if (string_list != NULL) XtFree((char *)string_list);
}


void
CBTextDestroy (w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
  XtFree((char *)client_data);
}


void
CBoption (w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
  Widget menuButton = (Widget) client_data;
  char *label;

  XtVaGetValues (menuButton, XtNlabel, &label, NULL);
  XtVaGetValues (w, XtNlabel, &label, NULL);
  XtVaSetValues (menuButton, XtNlabel, label, NULL);
}


void
AddNewForm (hw, fptr)
HTMLWidget hw;
FormInfo *fptr;
{
  FormInfo *ptr;

  fptr->next = NULL;

  ptr = hw->html.form_list;
  if (ptr == NULL)
  {
    hw->html.form_list = fptr;
  }
  else
  {
    while (ptr->next != NULL)
    {
      ptr = ptr->next;
    }
    ptr->next = fptr;
  }
}


void
FreeOneForm (fptr)
FormInfo *fptr;
{
  if (fptr->action != NULL)      XtFree (fptr->action);
  if (fptr->method != NULL)      XtFree (fptr->method);
  if (fptr->enctype != NULL)     XtFree (fptr->enctype);
  if (fptr->enc_entity != NULL)  XtFree (fptr->enc_entity);
  XtFree ((char *)fptr);
}


void
FreeForms (hw)
HTMLWidget hw;
{
  FormInfo *fptr;

  fptr = hw->html.form_list;
  while (fptr != NULL)
  {
    FormInfo *next = fptr->next;
    FreeOneForm (fptr);
    fptr = next;
  }
  hw->html.form_list = NULL;
}


int
CollectSubmitInfo (w, fptr, name_list, value_list)
Widget w;
FormInfo *fptr;
char ***name_list;
char ***value_list;
{
  HTMLWidget hw = (HTMLWidget) (fptr->hw);
  WbFormCallbackData cbdata;
  WidgetInfo *wptr;
  int cnt;
  Boolean state;

  if (fptr->end == -1)		/* unterminated FORM tag */
  {
    wptr = hw->html.widget_list;
    cnt = 0;
    while (wptr != NULL)
    {
      cnt++;
      wptr = wptr->next;
    }
    cbdata.attribute_count = cnt;
  }
  else
  {
    cbdata.attribute_count = fptr->end - fptr->start;
  }
  cbdata.attribute_names = (char **) XtMalloc (cbdata.attribute_count *
					       sizeof (char *));
  cbdata.attribute_values = (char **) XtMalloc (cbdata.attribute_count *
						sizeof (char *));

  if (fptr->start == 0)
  {
    wptr = hw->html.widget_list;
  }
  else
  {
    wptr = hw->html.widget_list;
    while (wptr != NULL)
    {
      if (wptr->id == fptr->start)
      {
	wptr = wptr->next;
	break;
      }
      wptr = wptr->next;
    }
  }

  cnt = 0;

  while ((wptr != NULL) && (cnt < cbdata.attribute_count))
  {
    if ((wptr->name) && (wptr->type != W_PUSHBUTTON || wptr->w == w))
    {
      Widget child;
      STRING *str_list;
      int list_cnt;
      char *val;
      XawListReturnStruct *currentSelection;

      cbdata.attribute_names[cnt] = wptr->name;
      switch (wptr->type)
      {
        case W_TEXTFIELD:
	  XtVaGetValues (GetAsciiTextWidget(wptr->w), XtNstring,
			 &(cbdata.attribute_values[cnt]),
			 NULL);
	  if ((cbdata.attribute_values[cnt] != NULL) &&
	      (cbdata.attribute_values[cnt][0] == '\0'))
	  {
	    cbdata.attribute_values[cnt] = NULL;
	  }
	  break;
	case W_TEXTAREA:
	  XtVaGetValues (wptr->w, XtNstring,
			 &(cbdata.attribute_values[cnt]),
			 NULL);
	  if ((cbdata.attribute_values[cnt] != NULL) &&
	      (cbdata.attribute_values[cnt][0] == '\0'))
	  {
	    cbdata.attribute_values[cnt] = NULL;
	  }
	  break;
	case W_PASSWORD:
	  XtVaGetValues (GetAsciiTextWidget(wptr->w), XtNstring,
			 &(cbdata.attribute_values[cnt]),
			 NULL);
	  if ((cbdata.attribute_values[cnt] != NULL) &&
	      (cbdata.attribute_values[cnt][0] == '\0'))
	  {
	    cbdata.attribute_values[cnt] = NULL;
	  }
	  break;
	case W_LIST:
	  /*
	   * First get the Widget ID of the proper
	   * list element
	   */
          {
	    WidgetList wl;
	    XtVaGetValues (wptr->w, XtNchildren, &wl, NULL);
	    child = *++wl;
	  }

	  /*
	   * Now get the list of selected items.
	   */
	  currentSelection = XawListShowCurrent (child);
	  list_cnt = currentSelection->list_index == XAW_LIST_NONE ? 0 : 1;
	  str_list = &(currentSelection->string);
	  
	  if (list_cnt == 0)
	  {
	    cnt--;
	    cbdata.attribute_count--;
	  }
	  else
	      /* list_cnt >= 1 */
	  {
	    int j, new_cnt;
	    char **names;
	    char **values;
	    
	    if (list_cnt > 1)
	    {
	      new_cnt = cbdata.attribute_count + list_cnt - 1;
	      names = (char **) XtMalloc (new_cnt * sizeof (char *));
	      values = (char **) XtMalloc (new_cnt * sizeof (char *));
	      for (j = 0; j < cnt; j++)
	      {
		names[j] = cbdata.attribute_names[j];
		values[j] = cbdata.attribute_values[j];
	      }
	      XtFree((char *) cbdata.attribute_names);
	      XtFree((char *) cbdata.attribute_values);
	      cbdata.attribute_names = names;
	      cbdata.attribute_values = values;
	      cbdata.attribute_count = new_cnt;
	    }
	    
	    for (j = 0; j < list_cnt; j++)
	    {
	      cbdata.attribute_names[cnt + j] = wptr->name;
	      val = str_list[j];
	      if ((val != NULL) && (val[0] == '\0'))
	      {
		val = NULL;
	      }
	      else if (val != NULL)
	      {
		val = MapOptionReturn (val, wptr->mapping);
	      }
	      cbdata.attribute_values[cnt + j] = val;
	    }
	    cnt = cnt + list_cnt - 1;
	  }
	  break;
        /*
	 * For an option menu, first get the label gadget
	 * which holds the current value.
	 * Now get the text from that label as a character
	 * string.
	 */
        case W_OPTIONMENU:
	  XtVaGetValues (wptr->w, XtNlabel, &val, NULL);
	  if ((val != NULL) && (val[0] == '\0'))
	  {
	    val = NULL;
	  }
	  else if (val != NULL)
	  {
	    val = MapOptionReturn (val, wptr->mapping);
	  }
	  cbdata.attribute_values[cnt] = val;
	  if ((cbdata.attribute_values[cnt] != NULL) &&
	      (cbdata.attribute_values[cnt][0] == '\0'))
	  {
	    cbdata.attribute_values[cnt] = NULL;
	  }
	  break;
	case W_CHECKBOX:
	case W_RADIOBOX:
	  XtVaGetValues (wptr->w, XtNstate, &state, NULL);
	  if (state)
	  {
	    cbdata.attribute_values[cnt] = wptr->value;
	  }
	  else
	  {
	    cnt--;
	    cbdata.attribute_count--;
	  }
	  break;
        case W_HIDDEN:
	  cbdata.attribute_values[cnt] = wptr->value;
	  break;
	case W_PUSHBUTTON:
	  cbdata.attribute_values[cnt] = wptr->value; /* SKLindsay 240796 */
	  break;
        default:
	  cbdata.attribute_values[cnt] = NULL;
	  break;
      }
      cnt++;
    }
    else
    {
      cbdata.attribute_count--;
    }
    wptr = wptr->next;
  }
  cbdata.attribute_count = cnt;

  *name_list = cbdata.attribute_names;
  *value_list = cbdata.attribute_values;
  return (cbdata.attribute_count);
}


void
ImageSubmitForm (fptr, event, name, x, y)
FormInfo *fptr;
XEvent *event;
char *name;
int x, y;
{
  HTMLWidget hw = (HTMLWidget) (fptr->hw);
  WbFormCallbackData cbdata;
  int i, cnt;
  char **name_list;
  char **value_list;
  char valstr[100];

  cbdata.event = event;
  cbdata.href = fptr->action;
  cbdata.method = fptr->method;
  cbdata.enctype = fptr->enctype;
  cbdata.enc_entity = fptr->enc_entity;

  name_list = NULL;
  value_list = NULL;
  cnt = CollectSubmitInfo (0, fptr, &name_list, &value_list);

  cbdata.attribute_count = cnt + 2;
  cbdata.attribute_names = (char **)XtMalloc (cbdata.attribute_count *
					      sizeof (char *));
  cbdata.attribute_values = (char **)XtMalloc (cbdata.attribute_count *
					       sizeof (char *));
  for (i = 0; i < cnt; i++)
  {
    cbdata.attribute_names[i] = name_list[i];
    cbdata.attribute_values[i] = value_list[i];
  }
  if (name_list != NULL)
  {
    XtFree((char *)name_list);
    name_list = NULL;
  }
  if (value_list != NULL)
  {
    XtFree((char *)value_list);
    value_list = NULL;
  }

  if ((name != NULL) && (name[0] != '\0'))
  {
    cbdata.attribute_names[cnt] = (char *) XtMalloc (sizeof (char) *
						     (strlen (name) +
						      strlen (X_NAME) + 2));
    strcpy (cbdata.attribute_names[cnt], name);
    strcat (cbdata.attribute_names[cnt], ".");
    strcat (cbdata.attribute_names[cnt], X_NAME);
  }
  else
  {
    cbdata.attribute_names[cnt] = XtNewString (X_NAME);
  }
  sprintf (valstr, "%d", x);
  cbdata.attribute_values[cnt] = XtNewString (valstr);

  cnt++;
  if ((name != NULL) && (name[0] != '\0'))
  {
    cbdata.attribute_names[cnt] = (char *) XtMalloc (sizeof (char) *
						     (strlen (name) +
						      strlen (Y_NAME) + 2));
    strcpy (cbdata.attribute_names[cnt], name);
    strcat (cbdata.attribute_names[cnt], ".");
    strcat (cbdata.attribute_names[cnt], Y_NAME);
  }
  else
  {
    cbdata.attribute_names[cnt] = XtNewString (Y_NAME);
  }
  sprintf (valstr, "%d", y);
  cbdata.attribute_values[cnt] = XtNewString (valstr);

  XtCallCallbackList ((Widget)hw, hw->html.form_callback, (XtPointer)&cbdata);
}


void
CBSubmitForm (w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
  FormInfo *fptr = (FormInfo *) client_data;
  HTMLWidget hw = (HTMLWidget) (fptr->hw);
  WbFormCallbackData cbdata;

  cbdata.event = NULL;
  cbdata.href = fptr->action;
  cbdata.method = fptr->method;
  cbdata.enctype = fptr->enctype;
  cbdata.enc_entity = fptr->enc_entity;

  cbdata.attribute_count = CollectSubmitInfo (w,
					      fptr,
					      &cbdata.attribute_names,
					      &cbdata.attribute_values);

  XtCallCallbackList ((Widget) hw, hw->html.form_callback,
		      (XtPointer) & cbdata);
}


/*
 * A radio buttom was toggled on in a form.
 * If there are other radios of the same name, turn them off.
 */
void
CBChangeRadio (w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
  FormInfo *fptr = (FormInfo *) client_data;
  HTMLWidget hw = (HTMLWidget) (fptr->hw);
  WidgetInfo *wptr;
  WidgetInfo *wtmp;
  char *name;
  int cnt, count;
  Boolean state;

  /*
   * Only do stuff when the button is turned on.
   * Don't let the button be turned off, by clicking on
   * it, as that would leave all buttons off.
   */
  XtVaGetValues (w, XtNstate, &state, NULL);
  if (!state)
  {
    XtVaSetValues (w, XtNstate, True, NULL);
    return;
  }

  /*
   * Terminate the form if it was never properly terminated.
   */
  if (fptr->end == -1)		/* unterminated FORM tag */
  {
    wptr = hw->html.widget_list;
    cnt = 0;
    while (wptr != NULL)
    {
      cnt++;
      wptr = wptr->next;
    }
    count = cnt;
  }
  else
  {
    count = fptr->end - fptr->start;
  }

  /*
   * Locate the start of the form.
   */
  if (fptr->start == 0)
  {
    wptr = hw->html.widget_list;
  }
  else
  {
    wptr = hw->html.widget_list;
    while (wptr != NULL)
    {
      if (wptr->id == fptr->start)
      {
	wptr = wptr->next;
	break;
      }
      wptr = wptr->next;
    }
  }

  /*
   * Find the name of the toggle button just pressed.
   */
  name = NULL;
  wtmp = wptr;
  while (wtmp != NULL)
  {
    if (wtmp->w == w)
    {
      name = wtmp->name;
      break;
    }
    wtmp = wtmp->next;
  }

  /*
   * Check for other checked radioboxes of the same name.
   */
  cnt = 0;
  while ((wptr != NULL) && (cnt < count))
  {
    if ((wptr->type == W_RADIOBOX) &&
	(wptr->w != w) &&
	(wptr->name != NULL) &&
	(name != NULL) &&
	(strcasecmp (wptr->name, name) == 0))
    {
      XtVaGetValues (wptr->w, XtNstate, &state, NULL);
      if (state)
      {
	XtVaSetValues (wptr->w, XtNstate, False, NULL);
      }
    }
    cnt++;
    wptr = wptr->next;
  }
}

/*
 * RETURN was hit in a textfield in a form.
 * If this is the only textfield in this form, submit the form.
 */
void
CBActivateField (w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
  FormInfo *fptr = (FormInfo *) client_data;
  HTMLWidget hw = (HTMLWidget) (fptr->hw);
  WidgetInfo *wptr;
  int cnt, count;

  /*
   * Terminate the form if it was never properly terminated.
   */
  if (fptr->end == -1)		/* unterminated FORM tag */
  {
    wptr = hw->html.widget_list;
    cnt = 0;
    while (wptr != NULL)
    {
      cnt++;
      wptr = wptr->next;
    }
    count = cnt;
  }
  else
  {
    count = fptr->end - fptr->start;
  }

  /*
   * Locate the start of the form.
   */
  if (fptr->start == 0)
  {
    wptr = hw->html.widget_list;
  }
  else
  {
    wptr = hw->html.widget_list;
    while (wptr != NULL)
    {
      if (wptr->id == fptr->start)
      {
	wptr = wptr->next;
	break;
      }
      wptr = wptr->next;
    }
  }

  /*
   * Count the textfields in this form.
   */
  cnt = 0;
  while ((wptr != NULL) && (cnt < count))
  {
    if ((wptr->type == W_TEXTFIELD) || (wptr->type == W_PASSWORD))
    {
      cnt++;
    }
    wptr = wptr->next;
  }

  /*
   * If this is the only textfield in this form, submit the form.
   */
  if (cnt == 1) CBSubmitForm (w, client_data, call_data);
}

void
CBResetForm (w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
  FormInfo *fptr = (FormInfo *) client_data;
  HTMLWidget hw = (HTMLWidget) (fptr->hw);
  WidgetInfo *wptr;
  int widget_count, cnt;

  if (fptr->end == -1)		/* unterminated FORM tag */
  {
    wptr = hw->html.widget_list;
    cnt = 0;
    while (wptr != NULL)
    {
      cnt++;
      wptr = wptr->next;
    }
    widget_count = cnt;
  }
  else
  {
    widget_count = fptr->end - fptr->start;
  }

  if (fptr->start == 0)
  {
    wptr = hw->html.widget_list;
  }
  else
  {
    wptr = hw->html.widget_list;
    while (wptr != NULL)
    {
      if (wptr->id == fptr->start)
      {
	wptr = wptr->next;
	break;
      }
      wptr = wptr->next;
    }
  }

  cnt = 0;
  while ((wptr != NULL) && (cnt < widget_count))
  {
    Widget child, atw;
    char *txt = NULL;
    int length = 0;
    Boolean stringInPlace;

    switch (wptr->type)
    {
      case W_TEXTFIELD:
        atw = GetAsciiTextWidget(wptr->w);
        XtVaGetValues (atw,
		       XtNuseStringInPlace, &stringInPlace,
		       XtNlength, &length,
		       NULL);
	if (stringInPlace)
	{
	  XtVaGetValues (atw, XtNstring, &txt, NULL);
	}
	if (wptr->value == NULL)
	{
	  if (stringInPlace)
	  {
	    if (txt) *txt = '\0';
	    XtVaSetValues (atw, XtNstring, txt, NULL);
	  }
	  else
	  {
	    XtVaSetValues (atw, XtNstring, "", NULL);
	  }
	}
	else
	{
	  if (stringInPlace)
	  {
	    strncpy (txt, wptr->value, length);
	    XtVaSetValues (atw, XtNstring, txt, NULL);
	  }
	  else
	  {
	    XtVaSetValues (atw, XtNstring, wptr->value, NULL);
	  }
	}
	break;
      case W_TEXTAREA:
	XtVaSetValues (wptr->w, XtNstring,
		       wptr->value ? wptr->value : "",
		       NULL);
	break;
      case W_PASSWORD:
        atw = GetAsciiTextWidget(wptr->w);
	if (wptr->value == NULL)
	{
	  XtVaSetValues (atw, XtNstring, "", NULL);
	  if (wptr->password != NULL)
	  {
	    XtFree(wptr->password);
	    wptr->password = NULL;
	  }
	}
	else
	{
	  int i, len;
	  
	  if (wptr->password != NULL)
	  {
	    XtFree(wptr->password);
	    wptr->password = NULL;
	  }
	  len = strlen (wptr->value);
	  wptr->password = (char *) XtMalloc (sizeof (char) * (len + 1));
	  for (i = 0; i < len; i++)
	  {
	    wptr->password[i] = '*';
	  }
	  wptr->password[len] = '\0';
	  XtVaSetValues (atw, XtNstring, wptr->password, NULL);
	  strcpy (wptr->password, wptr->value);
	}
	break;
      case W_LIST:
        {
	  char **vlist;
	  int vlist_cnt;
	  STRING *val_list;
	  int i;
	  WidgetList wl;
	  char **string_list;
	  int list_cnt;
	  
	  XtVaGetValues (wptr->w, XtNchildren, &wl, NULL);
	  child = *++wl;
	  XtVaGetValues (child,
			 XtNlist, &string_list,
			 XtNnumberStrings, &list_cnt, NULL);
	  
	  if (wptr->value != NULL)
	  {
	    vlist = ParseCommaList (wptr->value, &vlist_cnt);
	    val_list = (STRING *) XtMalloc (vlist_cnt * sizeof (STRING));
	    XawListUnhighlight (child);
	    for (i = 0; i < vlist_cnt; i++)
	    {
	      val_list[i] = XtNewString (vlist[i]);
	    }
	    FreeCommaList (vlist, vlist_cnt);
	    
	    if (vlist_cnt > 0)
	    {
	      if (vlist_cnt > 1)
	      {
		fprintf (stderr, "HTML: only a single selection allowed!\n");
	      }
	      
	      for (i = 0; i < list_cnt; i++)
	      {
		if (!strcasecmp (string_list[i], val_list[0]))
		{
		  XawListHighlight (child, i);
		  break;
		}
	      }
	    }
	    for (i = 0; i < vlist_cnt; i++)
	    {
	      if (val_list[i] != NULL) XtFree(val_list[i]);
	    }
	    if (val_list != NULL) XtFree((char *) val_list);
	  }
	  else
	  {
	    XawListUnhighlight (child);
	  }
	}
	break;
      /*
       * gack, we saved the widget id of the starting default
       * into the value character pointer, just so we could
       * yank it out here, and restore the default.
       */
      case W_OPTIONMENU:
	if (wptr->value != NULL)
	{
	  Widget hist = (Widget) wptr->value;
	  char *txt;
	  
	  XtVaGetValues (hist, XtNlabel, &txt, NULL);
	  XtVaSetValues (wptr->w, XtNlabel, txt, NULL);
	}
	break;
      case W_CHECKBOX:
      case W_RADIOBOX:
	XtVaSetValues (wptr->w, XtNstate, wptr->checked, NULL);
	break;
      case W_HIDDEN:
	break;
      default:
	break;
    }
    cnt++;
    wptr = wptr->next;
  }
}


void
PrepareFormEnd (hw, w, fptr)
HTMLWidget hw;
Widget w;
FormInfo *fptr;
{
  XtAddCallback (w, XtNcallback, (XtCallbackProc) CBSubmitForm,
		 (XtPointer)fptr);
}


void
PrepareFormReset (hw, w, fptr)
HTMLWidget hw;
Widget w;
FormInfo *fptr;
{
  XtAddCallback (w, XtNcallback, (XtCallbackProc) CBResetForm,
		 (XtPointer)fptr);
}


void
HideWidgets (hw)
HTMLWidget hw;
{
  WidgetInfo *wptr;
  XEvent event;

  wptr = hw->html.widget_list;
  while (wptr != NULL)
  {
    if ((wptr->w != NULL) && (wptr->mapped == True))
    {
      XtSetMappedWhenManaged (wptr->w, False);
      wptr->mapped = False;
    }
    wptr = wptr->next;
  }

  /*
   * Force the exposure events into the queue
   */
  XSync (XtDisplay (hw), False);

  /*
   * Remove all Expose events for the view window
   */
  while (XCheckWindowEvent (XtDisplay (hw->html.view),
			    XtWindow (hw->html.view),
			    ExposureMask, &event) == True)
  {
  }
}


void
MapWidgets (hw)
HTMLWidget hw;
{
  WidgetInfo *wptr;

  wptr = hw->html.widget_list;
  while (wptr != NULL)
  {
    if ((wptr->w != NULL) && (wptr->mapped == False))
    {
      wptr->mapped = True;
      XtSetMappedWhenManaged (wptr->w, True);
    }
    wptr = wptr->next;
  }
}


Boolean
AlreadyChecked (hw, fptr, name)
HTMLWidget hw;
FormInfo *fptr;
char *name;
{
  WidgetInfo *wptr;
  Boolean radio_checked;

  radio_checked = False;
  wptr = hw->html.widget_list;
  while (wptr != NULL)
  {
    if ((wptr->id >= fptr->start) &&
	(wptr->type == W_RADIOBOX) &&
	(wptr->checked == True) &&
	(wptr->name != NULL) &&
	(name != NULL) &&
	(strcasecmp (wptr->name, name) == 0))
    {
      radio_checked = True;
      break;
    }
    wptr = wptr->next;
  }
  return (radio_checked);
}

WidgetInfo *
AddNewWidget (hw, w, type, id, x, y, name, value, mapping, checked)
HTMLWidget hw;
Widget w;
int type;
int id;
int x, y;
char *name;
char *value;
char **mapping;
Boolean checked;
{
  WidgetInfo *wptr;
  Dimension width, height;

  /*
   * Don't want to do GetValues if this is HIDDEN input
   * tag with no widget.
   */
  if (w != NULL)
  {
    Cardinal argcnt;
    Arg arg[10];

    argcnt = 0;
    XtSetArg (arg[argcnt], XtNwidth, &width); argcnt++;
    XtSetArg (arg[argcnt], XtNheight, &height); argcnt++;
    XtGetValues (w, arg, argcnt);
  }
  else
  {
    width = 0;
    height = 0;
  }

  wptr = (WidgetInfo *) XtMalloc (sizeof (WidgetInfo));
  wptr->w = w;
  wptr->type = type;
  wptr->id = id;
  wptr->x = x;
  wptr->y = y;
  wptr->width = width;
  wptr->height = height;
  wptr->name = name;
  wptr->value = value;
  wptr->password = NULL;
  wptr->mapping = mapping;
  wptr->checked = checked;
  wptr->mapped = False;
  wptr->next = NULL;

  if (hw->html.widget_list == NULL)
  {
    hw->html.widget_list = wptr;
  }
  else				/* find tail */
  {
    WidgetInfo *tail = hw->html.widget_list;

    while (tail->next != NULL)   tail = tail->next;

    tail->next = wptr;
  }

  if ((wptr->type == W_PASSWORD) && (wptr->value != NULL))
  {
    wptr->password = XtNewString (wptr->value);
  }

  return (wptr);
}

/*
 * For the various widgets, return their font structures so
 * we can use the font's baseline to place them.
 */
XFontStruct *
GetWidgetFont (hw, wptr)
HTMLWidget hw;
WidgetInfo *wptr;
{
  Widget child;
  XFontStruct *font;

  /*
   * For option menus we have to first get the child that has the
   * font info.
   */
  if (wptr->type == W_OPTIONMENU)
  {
    XtVaGetValues (wptr->w, XtNfont, &font, NULL);
  }
  else
  {
    if (wptr->type == W_LIST)
    {
      WidgetList wl;
      int nc;
      XtVaGetValues (wptr->w, XtNchildren, &wl, XtNnumChildren, &nc, NULL);
      child = *++wl;
      XtVaGetValues (child, XtNfont, &font, NULL);
    }
    else if (wptr->type == W_TEXTFIELD || wptr->type == W_PASSWORD)
    {
      XtVaGetValues (GetAsciiTextWidget(wptr->w), XtNfont, &font, NULL);
    }
    else
    {
      XtVaGetValues (wptr->w, XtNfont, &font, NULL);
    }
  }

  return (font);
}


/*
 * Get the next value in a comma separated list.
 * Also unescape the '\' escaping done in ComposeCommaList
 * and convert the single ''' characters back to '"'
 * characters
 */
char *
NextComma (string)
char *string;
{
  char *tptr;

  tptr = string;
  while (*tptr != '\0')
  {
    if (*tptr == '\\')
    {
      *tptr = '\0';
      strcat (string, (char *) (tptr + 1));
      tptr++;
    }
    else if (*tptr == '\'')
    {
      *tptr = '\"';
      tptr++;
    }
    else if (*tptr == ',')
    {
      return (tptr);
    }
    else
    {
      tptr++;
    }
  }
  return (tptr);
}

/*
 * ParseCommaList
 */
static char **
ParseCommaList (str, count)
char *str;
int *count;
{
  char *str_copy;
  char **list;
  char **tlist;
  char *tptr;
  char *val;
  int i, cnt;
  int max_cnt;

  *count = 0;
  if ((str == NULL) || (*str == '\0')) return ((char **) NULL);

  str_copy = XtNewString (str);
  list = (char **) XtMalloc (50 * sizeof (char *));
  max_cnt = 50;

  /*
   * This loop counts the number of objects
   * in this list.
   * As a side effect, NextComma() unescapes in place so
   * "\\" becomes '\' and "\," becomes ',' and "\"" becomes '"'
   */
  cnt = 0;
  val = str_copy;
  tptr = NextComma (val);
  while (*tptr != '\0')
  {
    if ((cnt + 1) == max_cnt)
    {
      tlist = (char **) XtMalloc ((max_cnt + 50) * sizeof (char *));
      for (i = 0; i < cnt; i++)
      {
	tlist[i] = list[i];
      }
      XtFree((char *) list);
      list = tlist;
      max_cnt += 50;
    }
    *tptr = '\0';
    list[cnt] = XtNewString (val);
    cnt++;

    val = (char *) (tptr + 1);
    tptr = NextComma (val);
  }
  list[cnt] = XtNewString (val);
  cnt++;

  XtFree(str_copy);
  str_copy = NULL;
  tlist = (char **) XtMalloc ((cnt + 1) * sizeof (char *));
  for (i = 0; i < cnt; i++)
  {
    tlist[i] = list[i];
  }
  XtFree((char *) list);
  list = tlist;
  list[cnt] = NULL;

  *count = cnt;
  return (list);
}


/*
 * Compose a single string comma separated list from
 * an array of strings.  Any '\', or ',' in the
 * list are escaped with a prepending '\'.
 * So they become '\\' and '\,'
 * Also we want to allow '"' characters in the list, but
 * they would get eaten by the later parsing code, so we will
 * turn '"' into ''', and turn ''' into '\''
 */
char *
ComposeCommaList (list, cnt)
char **list;
int cnt;
{
  int i;
  char *buf;
  char *tbuf;
  int len, max_len;

  if (cnt == 0) return (XtNewString(""));

  buf = (char *) XtMalloc (sizeof (char) * 1024);
  max_len = 1024;
  len = 0;
  buf[0] = '\0';

  for (i = 0; i < cnt; i++)
  {
    char *option;
    char *tptr;
    int olen;

    option = list[i];
    if (option == NULL)
    {
      olen = 0;
    }
    else
    {
      olen = strlen (option);
    }
    if ((len + (olen * 2)) >= max_len)
    {
      tbuf = XtMalloc(max_len + olen + 1024);
      strcpy(tbuf, buf);
      XtFree(buf);
      buf = tbuf;
      max_len = max_len + olen + 1024;
    }
    tptr = (char *) (buf + len);
    while ((option != NULL) && (*option != '\0'))
    {
      if ((*option == '\\') || (*option == ',') || (*option == '\''))
      {
	*tptr++ = '\\';
	*tptr++ = *option++;
	len += 2;
      }
      else if (*option == '\"')
      {
	*tptr++ = '\'';
	option++;
	len++;
      }
      else
      {
	*tptr++ = *option++;
	len++;
      }
    }
    if (i != (cnt - 1))
    {
      *tptr++ = ',';
      len++;
    }
    *tptr = '\0';
  }

  tbuf = XtNewString(buf);
  XtFree(buf);

  return (tbuf);
}

void
FreeCommaList (list, cnt)
char **list;
int cnt;
{
  int i;

  for (i = 0; i < cnt; i++)
  {
    if (list[i] != NULL) XtFree(list[i]);
  }
  if (list != NULL) XtFree((char *) list);
}


/*
 * Clean up the mucked value field for a TEXTAREA.
 * Unescape the things with '\' in front of them, and transform
 * lone ' back to "
 */
void
UnMuckTextAreaValue (value)
char *value;
{
  char *tptr;

  if ((value == NULL) || (value[0] == '\0'))
  {
    return;
  }

  tptr = value;
  while (*tptr != '\0')
  {
    if (*tptr == '\\')
    {
      *tptr = '\0';
      strcat (value, (char *) (tptr + 1));
      tptr++;
    }
    else if (*tptr == '\'')
    {
      *tptr = '\"';
      tptr++;
    }
    else
    {
      tptr++;
    }
  }
}


static char *
MapOptionReturn (val, mapping)
char *val;
char **mapping;
{
  int cnt;

  if (mapping == NULL)
  {
    return (val);
  }

  cnt = 0;
  while (mapping[cnt] != NULL)
  {
    if (strcasecmp (mapping[cnt], val) == 0)
    {
      return (mapping[cnt + 1]);
    }
    cnt += 2;
  }
  return (val);
}


char **
MakeOptionMappings (list1, list2, list_cnt)
char **list1;
char **list2;
int list_cnt;
{
  int i, cnt;
  char **list;

  /*
   * pass through to see how many mappings we have.
   */
  cnt = 0;
  for (i = 0; i < list_cnt; i++)
  {
    if ((list2[i] != NULL) && (*list2[i] != '\0'))
    {
      cnt++;
    }
  }

  if (cnt == 0) return (NULL);

  list = (char **) XtMalloc (((2 * cnt) + 1) * sizeof (char *));

  cnt = 0;
  for (i = 0; i < list_cnt; i++)
  {
    if ((list2[i] != NULL) && (*list2[i] != '\0'))
    {
      list[cnt] = XtNewString (list1[i]);
      list[cnt + 1] = XtNewString (list2[i]);
      cnt += 2;
    }
  }
  list[cnt] = NULL;

  return (list);
}


/*
 * MakeTextAreaWidget
 */
static WidgetInfo *
MakeTextAreaWidget (name, hw, text, x, y, id, fptr)
char *name;
HTMLWidget hw;
char *text;
int x, y;
int id;
FormInfo *fptr;			/* not used */
{
  Widget w;
  char **list;
  int list_cnt;
  int rows, cols;
  char *tptr;
  char *value;
  Cardinal argcnt;
  Arg arg[30];

  /*
   * If there is no SIZE, look for ROWS and COLS
   * directly.
   * SIZE is COLUMNS,ROWS parse the list
   */
  rows = -1;
  cols = -1;
  tptr = ParseMarkTag (text, MT_INPUT, "SIZE");
  if (tptr == NULL)
  {
    tptr = ParseMarkTag (text, MT_INPUT, "ROWS");
    if (tptr != NULL)
    {
      rows = atoi (tptr);
      XtFree(tptr);
      tptr = NULL;
    }
    tptr = ParseMarkTag (text, MT_INPUT, "COLS");
    if (tptr != NULL)
    {
      cols = atoi (tptr);
      XtFree(tptr);
      tptr = NULL;
    }
  }
  else
  {
    list = ParseCommaList (tptr, &list_cnt);
    XtFree(tptr);
    tptr = NULL;

    if (list_cnt == 1)
    {
      cols = atoi (list[0]);
    }
    else if (list_cnt > 1)
    {
      cols = atoi (list[0]);
      rows = atoi (list[1]);
    }
    FreeCommaList (list, list_cnt);
  }

  argcnt = 0;
  XtSetArg (arg[argcnt], XtNx, x); argcnt++;
  XtSetArg (arg[argcnt], XtNy, y); argcnt++;
  XtSetArg (arg[argcnt], XtNeditType, XawtextEdit); argcnt++;
  if ((value = ParseMarkTag (text, MT_INPUT, "VALUE")) != NULL)
  {
    UnMuckTextAreaValue (value);
    XtSetArg (arg[argcnt], XtNstring, value); argcnt++;
  }
  w = XtCreateWidget (name,
		      asciiTextWidgetClass, hw->html.view,
		      arg, argcnt);
  setTextSize (w, cols > 0 ? cols : 20, rows > 0 ? rows : 1);

  XtSetMappedWhenManaged (w, False);
  XtManageChild (w);

  return (AddNewWidget (hw, w, W_TEXTAREA, id, x, y, name, value, NULL, 0));
}

/*
 * MakeTextFieldWidget
 */
static WidgetInfo *
MakeTextFieldWidget (name, hw, text, x, y, id, fptr)
char *name;
HTMLWidget hw;
char *text;
int x, y;
int id;
FormInfo *fptr;			/* not used */
{
  char **list;
  int list_cnt;
  int rows, cols;
  char *tptr;
  int type;
  char *value;
  int maxlength;
  Cardinal argcnt;
  Arg arg[30];
  Widget w;

  /*
   * SIZE can be either COLUMNS or COLUMNS,ROWS
   * we assume COLUMNS,ROWS and parse the list
   */
  tptr = ParseMarkTag (text, MT_INPUT, "SIZE");
  list = ParseCommaList (tptr, &list_cnt);
  if (tptr != NULL)
  {
    XtFree(tptr);
    tptr = NULL;
  }

  /*
   * If only COLUMNS specified, or SIZE not specified
   * assume a TEXTFIELD
   * Otherwise a TEXTAREA.
   */
  if (list_cnt <= 1)
  {
    type = W_TEXTFIELD;
    if (list_cnt == 1) cols = atoi (list[0]);
    else cols = -1;
  }
  else
  {
    type = W_TEXTAREA;
    cols = atoi (list[0]);
    rows = atoi (list[1]);
  }
  /*
   * Now that we have cols, and maybe rows, free the list
   */
  FreeCommaList (list, list_cnt);

  /*
   * Grab the starting value of the text here.
   * NULL if none.
   */
  value = ParseMarkTag (text, MT_INPUT, "VALUE");
  if (value == NULL) value = XtNewString ("");

  /*
   * For textfields parse maxlength and
   * set up the widget.
   */
  if (type == W_TEXTFIELD)
  {
    maxlength = -1;
    tptr = ParseMarkTag (text, MT_INPUT, "MAXLENGTH");
    if (tptr != NULL)
    {
      maxlength = atoi (tptr);
      XtFree(tptr);
      tptr = NULL;
    }

    argcnt = 0;
    XtSetArg (arg[argcnt], XtNx, x); argcnt++;
    XtSetArg (arg[argcnt], XtNy, y); argcnt++;
    w = XtCreateWidget (name,
			scrollingTextWidgetClass, hw->html.view,
			arg, argcnt);

    /*
     * Set args for asciitext
     */
    argcnt = 0;
    if (maxlength > 0)
    {
      XtSetArg (arg[argcnt], XtNlength, maxlength); argcnt++;
      XtAddCallback (w, XtNdestroyCallback, CBTextDestroy, (XtPointer)value);
    }
    XtSetArg (arg[argcnt], XtNstring, value); argcnt++;
    XtSetArg (arg[argcnt], XtNeditType, XawtextEdit); argcnt++;
    XtSetValues (GetAsciiTextWidget(w), arg, argcnt);

    cols = cols > 0 ? cols:20;
    rows = 1;
  }
  else
  {
    /*
     * Else this is a TEXTAREA.  Maxlength is ignored,
     * and we set up the scrolled window
     */
    argcnt = 0;
    XtSetArg (arg[argcnt], XtNx, x); argcnt++;
    XtSetArg (arg[argcnt], XtNy, y); argcnt++;
    XtSetArg (arg[argcnt], XtNeditType, XawtextEdit); argcnt++;
    if (value != NULL)
    {
      XtSetArg (arg[argcnt], XtNstring, value); argcnt++;
    }
    w = XtCreateWidget (name,
			asciiTextWidgetClass, hw->html.view,
			arg, argcnt);
    cols = cols > 0 ? cols : 20;
    rows = rows > 0 ? rows : 1;
  }

  setTextSize (w, cols, rows);

  XtSetMappedWhenManaged (w, False);
  XtManageChild (w);

  return (AddNewWidget (hw, w, type, id, x, y, name, value, NULL, False));
}

static WidgetInfo *
MakePasswordWidget (name, hw, text, x, y, id, fptr)
char *name;
HTMLWidget hw;
char *text;
int x, y;
int id;
FormInfo *fptr;			/* not used */
{
  Arg arg[30];
  int size, maxlength;
  Widget w;
  Cardinal argcnt;
  char *tptr;
  char *value;
  Pixel bg;

  value = ParseMarkTag (text, MT_INPUT, "VALUE");
  if (value == NULL) value = XtNewString("");

  size = -1;
  maxlength = -1;

  tptr = ParseMarkTag (text, MT_INPUT, "SIZE");
  if (tptr != NULL)
  {
    size = atoi (tptr);
    XtFree(tptr);
    tptr = NULL;
  }

  tptr = ParseMarkTag (text, MT_INPUT, "MAXLENGTH");
  if (tptr != NULL)
  {
    maxlength = atoi (tptr);
    XtFree(tptr);
    tptr = NULL;
  }

  argcnt = 0;
  XtSetArg (arg[argcnt], XtNx, x); argcnt++;
  XtSetArg (arg[argcnt], XtNy, y); argcnt++;
  w = XtCreateWidget (name,
		      scrollingTextWidgetClass, hw->html.view,
		      arg, argcnt);

  argcnt = 0;
  if (maxlength > 0)
  {
    XtAddCallback (w, XtNdestroyCallback, CBTextDestroy, (XtPointer) value);
    XtSetArg (arg[argcnt], XtNlength, maxlength); argcnt++;
  }

  XtVaGetValues (w, XtNbackground, &bg, NULL);

  XtSetArg (arg[argcnt], XtNforeground, bg); argcnt++;
  XtSetArg (arg[argcnt], XtNstring, value); argcnt++;
  XtSetArg (arg[argcnt], XtNeditType, XawtextEdit); argcnt++;
  XtSetValues (GetAsciiTextWidget(w), arg, argcnt);

  setTextSize (w, size < 1 ? 20 : size, 1);

  XtSetMappedWhenManaged (w, False);
  XtManageChild (w);

  return (AddNewWidget (hw, w, W_PASSWORD, id, x, y,
			name, value, NULL, False));
}

/*
 * MakeCheckWidget
 */
static WidgetInfo *
MakeCheckWidget (name, hw, text, x, y, id, fptr)
char *name;
HTMLWidget hw;
char *text;
int x, y;
int id;
FormInfo *fptr;			/* not used */
{
  Boolean checked;
  char *value;
  Cardinal argcnt;
  char *tptr;
  Arg arg[30];
  Widget w;

  value = ParseMarkTag (text, MT_INPUT, "VALUE");
  if (value == NULL) value = XtNewString ("on");

  tptr = ParseMarkTag (text, MT_INPUT, "CHECKED");

  argcnt = 0;
  XtSetArg (arg[argcnt], XtNx, x); argcnt++;
  XtSetArg (arg[argcnt], XtNy, y); argcnt++;
  if (tptr != NULL)
  {
    XtSetArg (arg[argcnt], XtNstate, True); argcnt++;
    checked = True;
    XtFree(tptr);
    tptr = NULL;
  }
  XtSetArg (arg[argcnt], XtNlabel, ""); argcnt++;
  w = XtCreateWidget (name,
		      toggleWidgetClass, hw->html.view,
		      arg, argcnt);
  XtSetMappedWhenManaged (w, False);
  XtManageChild (w);

  return (AddNewWidget (hw, w, W_CHECKBOX, id, x, y,
			name, value, NULL, checked));
}

/*
 * MakeHiddenWidget
 */
static WidgetInfo *
MakeHiddenWidget (name, hw, text, x, y, id, fptr)
char *name;
HTMLWidget hw;
char *text;
int x, y;
int id;
FormInfo *fptr;			/* not used */
{
  char *value;

  value = ParseMarkTag (text, MT_INPUT, "VALUE");
  if (value == NULL) XtNewString("");

  return (AddNewWidget (hw, NULL, W_HIDDEN, id, x, y,
			name, value, NULL, False));
}

/*
 * MakeRadioBoxWidget
 */
static WidgetInfo *
MakeRadioBoxWidget (name, hw, text, x, y, id, fptr)
char *name;
HTMLWidget hw;
char *text;
int x, y;
int id;
FormInfo *fptr;
{
  char *value;
  Arg arg[30];
  char *tptr;
  Cardinal argcnt;
  Widget w;
  Boolean checked;

  value = ParseMarkTag (text, MT_INPUT, "VALUE");
  if (value == NULL) value = XtNewString ("on");

  /*
   * Only one checked radio button with the
   * same name per form
   */
  tptr = ParseMarkTag (text, MT_INPUT, "CHECKED");
  if ((tptr != NULL) && (AlreadyChecked (hw, fptr, name) == True))
  {
    XtFree (tptr);
    tptr = NULL;
  }

  argcnt = 0;
  XtSetArg (arg[argcnt], XtNx, x); argcnt++;
  XtSetArg (arg[argcnt], XtNy, y); argcnt++;
  if (tptr != NULL)
  {
    XtSetArg (arg[argcnt], XtNstate, True); argcnt++;
    checked = True;
    XtFree (tptr);
    tptr = NULL;
  }
  XtSetArg (arg[argcnt], XtNlabel, ""); argcnt++;
  w = XtCreateWidget (name,
		      toggleWidgetClass, hw->html.view,
		      arg, argcnt);
  XtAddCallback (w, XtNcallback,
		 (XtCallbackProc) CBChangeRadio, (XtPointer)fptr);
  XtSetMappedWhenManaged (w, False);
  XtManageChild (w);

  return (AddNewWidget (hw, w, W_RADIOBOX, id, x, y,
			name, value, NULL, checked));
}

/*
 * MakeSubmitWidget
 */
static WidgetInfo *
MakeSubmitWidget (name, hw, text, x, y, id, fptr)
char *name;
HTMLWidget hw;
char *text;
int x, y;
int id;
FormInfo *fptr;
{
  char *value;
  Cardinal argcnt;
  Arg arg[30];
  Widget w;

  value = ParseMarkTag (text, MT_INPUT, "VALUE");
  if ((value == NULL) || (*value == '\0'))
  {
    value = XtNewString ("Submit Query");
  }

  argcnt = 0;
  XtSetArg (arg[argcnt], XtNx, x); argcnt++;
  XtSetArg (arg[argcnt], XtNy, y); argcnt++;
  if (value != NULL)
  {
    XtSetArg (arg[argcnt], XtNlabel, value); argcnt++;
  }
  w = XtCreateWidget (name != NULL ? name:"submit",
		      commandWidgetClass, hw->html.view,
		      arg, argcnt);
  XtSetMappedWhenManaged (w, False);
  XtManageChild (w);
  PrepareFormEnd (hw, w, fptr);

  return (AddNewWidget (hw, w, W_PUSHBUTTON, id, x, y,
			name, value, NULL, False));
}

/*
 * MakeSelectWidget
 */
static WidgetInfo *
MakeSelectWidget (name, hw, text, x, y, id, fptr)
char *name;
HTMLWidget hw;
char *text;
int x, y;
int id;
FormInfo *fptr;			/* not used */
{
  int type;
  Widget w;
  STRING label;
  Widget scroll;
  Widget pulldown, button, hist;
  char *options;
  char *returns;
  char **list;
  int list_cnt;
  char **vlist;
  int vlist_cnt;
  char **ret_list;
  int return_cnt;
  int i, mult, size;
  char *tptr;
  char *value;
  char **mapping;
  Cardinal argcnt;
  Arg arg[30];

  tptr = ParseMarkTag (text, MT_INPUT, "HINT");
  if (tptr != NULL)
  {
    if (strcasecmp(tptr, "list") == 0) type = W_LIST;
    else if (strcasecmp (tptr, "menu") == 0) type =  W_OPTIONMENU;
    XtFree(tptr);
    tptr = NULL;
  }
  else type = -1;

  size = 5;
  tptr = ParseMarkTag (text, MT_INPUT, "SIZE");
  if (tptr != NULL)
  {
    size = atoi (tptr);
    if ((size > 1) && (type == -1)) type = W_LIST;
    XtFree (tptr);
    tptr = NULL;
  }

  mult = 0;
  tptr = ParseMarkTag (text, MT_INPUT, "MULTIPLE");
  if (tptr != NULL)
  {
    if (type == -1) type = W_LIST;
    mult = 1;
    XtFree (tptr);
    tptr = NULL;
  }

  if (type == -1)
  {
    type = W_OPTIONMENU;
  }

  label = NULL;
  hist = NULL;
  value = ParseMarkTag (text, MT_INPUT, "VALUE");
  options = ParseMarkTag (text, MT_INPUT, "OPTIONS");
  list = ParseCommaList (options, &list_cnt);
  if (options != NULL)
  {
    XtFree (options);
    options = NULL;
  }

  returns = ParseMarkTag (text, MT_INPUT, "RETURNS");
  ret_list = ParseCommaList (returns, &return_cnt);
  if (returns != NULL)
  {
    XtFree (returns);
    returns = NULL;
  }

  /*
   * If return_cnt is less than list_cnt, the user made
   * a serious error.  Try to recover by padding out
   * ret_list with NULLs
   */
  if (list_cnt > return_cnt)
  {
    int rcnt;
    char **rlist;

    rlist = (char **) XtMalloc (list_cnt * sizeof (char *));
    for (rcnt = 0; rcnt < return_cnt; rcnt++)
    {
      rlist[rcnt] = ret_list[rcnt];
    }
    for (rcnt = return_cnt; rcnt < list_cnt; rcnt++)
    {
      rlist[rcnt] = NULL;
    }
    if (ret_list != NULL) XtFree ((char *) ret_list);
    ret_list = rlist;
  }

  vlist = ParseCommaList (value, &vlist_cnt);

  if (size > list_cnt) size = list_cnt;
  if (size < 1) size = 1;

  mapping = MakeOptionMappings (list, ret_list, list_cnt);

  if (type == W_OPTIONMENU)
  {
    XFontStruct *font;
    Dimension maxWidth = 0, width, iW;

    argcnt = 0;
    XtSetArg (arg[argcnt], XtNx, x); argcnt++;
    XtSetArg (arg[argcnt], XtNy, y); argcnt++;
    XtSetArg (arg[argcnt], XtNlabel, list[0]); argcnt++;
    w = XtCreateWidget (name,
			menuButtonWidgetClass, hw->html.view,
			arg, argcnt);
    pulldown = XtCreatePopupShell ("menu",
				   simpleMenuWidgetClass, w,
				   NULL, 0);

    for (i = 0; i < list_cnt; i++)
    {
      char bname[30];

      sprintf (bname, "Button%d", (i + 1));
      argcnt = 0;
      XtSetArg (arg[argcnt], XtNlabel, list[i]); argcnt++;
      button = XtCreateWidget (bname,
			       smeBSBObjectClass, pulldown,
                               arg, argcnt);
      XtManageChild (button);

      XtAddCallback (button, XtNcallback,
		     CBoption, (XtPointer) w);

      if (i == 0)
      {
	XtVaGetValues (w,
		       XtNfont, &font,
		       XtNinternalWidth, &iW,
		       NULL);
      }

      width = XTextWidth (font, list[i], strlen (list[i]));

      if (width > maxWidth) maxWidth = width;

      if ((vlist_cnt > 0) && (vlist[0] != NULL) &&
	  (strcasecmp (vlist[0], list[i]) == 0))
      {
	hist = button;
	XtVaSetValues (w,
		       XtNlabel, list[i],
		       NULL);
      }

      /*
       * Start hist out as the first button
       * so that if the user didn't set a
       * default we always default to the
       * first element.
       */
      if ((i == 0) && (hist == NULL))
      {
	hist = button;
      }
    }

    XtVaSetValues (w, XtNwidth, maxWidth + (4 * iW), NULL);

    FreeCommaList (vlist, vlist_cnt);
    if (value != NULL)
    {
      XtFree (value);
      value = NULL;
    }

    if (hist != NULL)
    {
      /*
       * A gaggage.  Value is used to later
       * restore defaults.  For option menu
       * this means we need to save a child
       * widget id as opposed to the
       * character string everyone else uses.
       */
      value = (char *) hist;
    }
  }
  else /* type == W_LIST */
  {
    STRING *string_list;
    STRING *val_list;

    if (list_cnt < 1)
    {
      if (size < 1) size = 1;

      list_cnt = vlist_cnt = size;

      string_list = (STRING *) XtMalloc (list_cnt * sizeof (STRING));
      for (i = 0; i < list_cnt; i++)
      {
	string_list[i] = XtNewString ("");
      }

      val_list = (STRING *) XtMalloc (vlist_cnt * sizeof (STRING));
      for (i = 0; i < vlist_cnt; i++)
      {
	val_list[i] = XtNewString ("");
      }
    }
    else
    {
      if ((!mult) && (vlist_cnt > 1))
      {
	XtFree (value);
	value = XtNewString (vlist[0]);
      }

      string_list = (STRING *) XtMalloc (list_cnt * sizeof (STRING));
      for (i = 0; i < list_cnt; i++)
      {
	string_list[i] = XtNewString (list[i]);
      }

      val_list = (STRING *) XtMalloc (vlist_cnt * sizeof (STRING));
      for (i = 0; i < vlist_cnt; i++)
      {
	val_list[i] = XtNewString (vlist[i]);
      }

      FreeCommaList (list, list_cnt);
      FreeCommaList (vlist, vlist_cnt);
    }

    argcnt = 0;
    XtSetArg (arg[argcnt], XtNx, x); argcnt++;
    XtSetArg (arg[argcnt], XtNy, y); argcnt++;
    XtSetArg (arg[argcnt], XtNallowVert, True); argcnt++;
    /*
     * I pulled these numbers out of my butt.  john.
     */
    XtSetArg (arg[argcnt], XtNwidth, 200); argcnt++;
    XtSetArg (arg[argcnt], XtNheight, 100); argcnt++;
    scroll = XtCreateWidget ("Scroll",
			     viewportWidgetClass, hw->html.view,
                             arg, argcnt);

    argcnt = 0;
    XtSetArg (arg[argcnt], XtNdefaultColumns, 1); argcnt++;
    w = XtCreateWidget (name,
			listWidgetClass, scroll,
                        arg, argcnt);
    XtManageChild (w);

    XtAddCallback (w, XtNdestroyCallback, CBListDestroy, NULL);

    XawListChange (w, string_list, list_cnt, 0, True);

    if (vlist_cnt > 0)
    {
      if (vlist_cnt > 1)
      {
	fprintf (stderr, "HTML: only a single selection allowed!\n");
      }

      for (i = 0; i < list_cnt; i++)
      {
	if (!strcasecmp (string_list[i], val_list[0]))
	{
	  XawListHighlight (w, i);
	  break;
	}
      }
    }

    if (size > list_cnt) size = list_cnt;
    if (size > 1)
    {
      XFontStruct *font = NULL;
      Dimension h, width, s;

      XtVaGetValues (w, XtNfont, &font,
		     XtNinternalHeight, &h,
		     XtNwidth, &width,
		     XtNrowSpacing, &s,
		     NULL);
      XtVaSetValues (scroll,
		     XtNheight,
		     h + size * (s + FONTHEIGHT (font)),
		     XtNwidth, width + 20,
		     NULL);
    }

    w = scroll;

    for (i = 0; i < vlist_cnt; i++)
    {
      if (val_list[i] != NULL) XtFree(val_list[i]);
    }
    if (val_list != NULL) XtFree((char *) val_list);
  }

  XtSetMappedWhenManaged (w, False);
  XtManageChild (w);

  return (AddNewWidget (hw, w, type, id, x, y,
			name, value, mapping, False));
}

/*
 * MakeResetWidget
 */
static WidgetInfo *
MakeResetWidget (name, hw, text, x, y, id, fptr)
char *name;
HTMLWidget hw;
char *text;
int x, y;
int id;
FormInfo *fptr;
{
  char *value;
  Cardinal argcnt;
  Arg arg[30];
  Widget w;

  value = ParseMarkTag (text, MT_INPUT, "VALUE");
  if ((value == NULL) || (*value == '\0'))
  {
    value = XtNewString ("Reset");
  }

  argcnt = 0;
  XtSetArg (arg[argcnt], XtNx, x); argcnt++;
  XtSetArg (arg[argcnt], XtNy, y); argcnt++;
  if (value != NULL)
  {
    XtSetArg (arg[argcnt], XtNlabel, value); argcnt++;
  }
  w = XtCreateWidget (name,
		      commandWidgetClass, hw->html.view,
		      arg, argcnt);
  XtSetMappedWhenManaged (w, False);
  XtManageChild (w);
  PrepareFormReset (hw, w, fptr);

  return (AddNewWidget (hw, w, W_PUSHBUTTON, id, x, y,
			name, value, NULL, False));
}

/*
 * MakePushButtonWidget
 */
static WidgetInfo *
MakePushButtonWidget (name, hw, text, x, y, id, fptr)
char *name;
HTMLWidget hw;
char *text;
int x, y;
int id;
FormInfo *fptr;			/* not used */
{
  char *value;
  Cardinal argcnt;
  Arg arg[30];
  Widget w;

  value = ParseMarkTag (text, MT_INPUT, "VALUE");

  argcnt = 0;
  XtSetArg (arg[argcnt], XtNx, x); argcnt++;
  XtSetArg (arg[argcnt], XtNy, y); argcnt++;
  if (value != NULL)
  {
    XtSetArg (arg[argcnt], XtNlabel, value); argcnt++;
  }
  w = XtCreateWidget (name,
		      commandWidgetClass, hw->html.view,
		      arg, argcnt);
  XtSetMappedWhenManaged (w, False);
  XtManageChild (w);

  return (AddNewWidget (hw, w, W_PUSHBUTTON, id, x, y,
			name, value, NULL, False));
}

/*
 * Move the widget to a new x,y location and update the WidgetInfo x,y.
 */
void
MoveWidget (wp, x, y)
WidgetInfo *wp;
int x,y;
{
    wp->x = x;
    wp->y = y;

    /*
     * Don't want to SetValues if type HIDDEN which has no widget.
     */
    if (wp->w != NULL)
    {
      Arg arg[10];
      Cardinal argcnt;

      XtUnmanageChild (wp->w);
      argcnt = 0;
      XtSetArg (arg[argcnt], XtNx, x); argcnt++;
      XtSetArg (arg[argcnt], XtNy, y); argcnt++;
      XtSetValues (wp->w, arg, argcnt);
      XtManageChild (wp->w);
    }
}


/*
 * Make the appropriate widget for this tag, and fill in an
 * WidgetInfo structure and return it.
 */
WidgetInfo *
MakeWidget (hw, text, x, y, id, fptr)
HTMLWidget hw;
char *text;
int x, y;
int id;
FormInfo *fptr;
{
  WidgetInfo *wlist;
  WidgetInfo *wptr;

  for (wlist = hw->html.widget_list;  wlist != NULL;  wlist = wlist->next)
  {
    if (wlist->id == id)
      break;
  }

  /*
   * If this widget is not on the list, we have never
   * used it before.  Create it now.
   */
  if (wlist == NULL)
  {
    int i;
    char *name;
    char *type_str;
    static struct typefunc
    {
      char *name;
      WidgetInfo *(*func) ();
    }
    typefunclist[] =
    {
      { "checkbox", MakeCheckWidget },
      { "hidden", MakeHiddenWidget },
      { "radio", MakeRadioBoxWidget },
      { "submit", MakeSubmitWidget },
      { "reset", MakeResetWidget },
      { "button", MakePushButtonWidget },
      { "select", MakeSelectWidget },
      { "textarea", MakeTextAreaWidget },
      { "password", MakePasswordWidget },
      { NULL, NULL },
    };

    name = ParseMarkTag (text, MT_INPUT, "NAME");
    type_str = ParseMarkTag (text, MT_INPUT, "TYPE");
    if (type_str == NULL)
    {
      wptr = MakeTextFieldWidget (name, hw, text, x, y, id, fptr);
    }
    else
    {
      wptr = NULL;
      for (i = 0; typefunclist[i].name != NULL; i++)
      {
	if (strcasecmp (type_str, typefunclist[i].name) == 0)
	{
	  wptr = (typefunclist[i].func) (name, hw, text, x, y, id, fptr);
	  break;
	}
      }
      if (wptr == NULL)		/* this could be a disaster */
      {
	wptr = MakeTextFieldWidget (name, hw, text, x, y, id, fptr);
      }
    }
    if (type_str != NULL)
    {
      XtFree(type_str);
      type_str = NULL;
    }
  }
  else
    /*
     * We found this widget on the list of already created widgets.
     * Move it to its new location.
     */
  {
    MoveWidget (wlist, x, y);
    wptr = wlist;
  }

  return (wptr);
}


void
WidgetRefresh (hw, eptr)
HTMLWidget hw;
struct ele_rec *eptr;
{
  if ((eptr->widget_data != NULL) && (eptr->widget_data->mapped == False) &&
      (eptr->widget_data->w != NULL))
  {
    eptr->widget_data->mapped = True;
    XtSetMappedWhenManaged (eptr->widget_data->w, True);
  }
}
