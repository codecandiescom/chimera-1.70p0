/*
 * fallback.c
 *
 * Copyright (C) 1993, 1994, 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 */ 
#include <stdio.h>

#include "copyright.h"

/*
 * I got sick of this being in main.c
 */
char *fallback_resources[] =
{
  /* Resources for everything */
  "*background:                moccasin",
  "*showGrip:                  false",

  /* resources for general widget types */
  "*Scrollbar.background:      burlywood2",
  "*Command.background:        burlywood2",
  "*Toggle.background:         burlywood2",
  "*Box.orientation:           horizontal",
  "*Label.borderWidth:         0",

  "*Form.resizeable:           true",
  "*Form.borderWidth:          0",

  "*Viewport*allowHoriz:       true",
  "*Viewport*allowVert:        true",

  /* Labels for main window commands */
  "*file.label:                File",
  "*open.label:                Open",
  "*bookmark.label:            Bookmark",
  "*quit.label:                Quit",
  "*reload.label:              Reload",
  "*help.label:                Help",
  "*source.label:              Source",
  "*home.label:                Home",
  "*back.label:                Back",
  "*search.label:              Search",
  "*cancel.label:              Cancel",
  "*deferpix.label:            DeferPix",

  /* resources for specific widgets */
  "*html.height:               600",
  "*html.width:                600",
  "*html.horizontalScrollOnTop: True",
  "*html.activeAnchorBG:       moccasin",
  "*html.anchorUnderlines:     1",
  "*html.visitedAnchorUnderlines: 1",
  "*html.dashedVisitedAnchorUnderlines: true",
  "*html.autoSize:             true",
  "*html.htmlErrorMsgCutoff:   0",

  /* resources for bookmark widgets */
  "*Bookmark*Box.hSpace:       10",
  "*Bookmark*markadd.label:    Add",
  "*Bookmark*markopen.label:   Open",
  "*Bookmark*markdel.label:    Delete",
  "*Bookmark*groupadd.label:   Add Group",
  "*Bookmark*groupdel.label:   Delete Group",
  "*Bookmark*Viewport.height:  150",
  "*Bookmark*Viewport.width:   400",
  "*Bookmark*Viewport.borderWidth: 1",
  "*Bookmark*dismiss.label:    Dismiss",
  "*Bookmark*List.forceColumns: true",
  "*Bookmark*List.defaultColumns: 1",
  "*Bookmark*groupform.defaultDistance: 3",
  "*Bookmark*markform.defaultDistance: 3",
  "*Bookmark*srgroupadd.strreqMessage: Enter Group Name",
  "*Bookmark*srmarkadd.strreqMessage: Enter Bookmark Name",
  "*Bookmark*title:            Bookmarks",

  /* resources for outputsel widgets */
  "*OutputSel*Box.hSpace:      10",
  "*OutputSel*obox.hSpace:     50",
  "*OutputSel*plain.label:     Plain Text",
  "*OutputSel*pretty.label:    Pretty Text",
  "*OutputSel*ps.label:        Postscript",
  "*OutputSel*raw.label:       Raw/HTML",
  "*OutputSel*printer.label:   Printer",
  "*OutputSel*email.label:     Email",
  "*OutputSel*file.label:      File",
  "*OutputSel*ok.label:        OK",
  "*OutputSel*clear.label:     Clear",
  "*OutputSel*dismiss.label:   Dismiss",
  "*OutputSel*ScrollingText.width: 300",
  "*OutputSel*title:           Output Selection",

  /* resources for strreq widgets */
  "*StrReq*Box.hSpace:         10",
  "*StrReq*ok.label:           OK",
  "*StrReq*clear.label:        Clear",
  "*StrReq*dismiss.label:      Dismiss",
  "*StrReq*ScrollingText.width: 300",
  "*filename.strreqMessage:    Enter filename",
  "*filename*title:            Enter filename",
  "*url.strreqMessage:         Enter URL",
  "*url*title:                 Enter URL",
  "*search.strreqMessage:      Enter search",
  "*search*title:              Enter search",

  /* resources for authreq widgets */
  "*AuthReq*Box.hspace:        10",
  "*AuthReq*ok.label:          OK",
  "*AuthReq*clear.label:       Clear",
  "*AuthReq*dismiss.label:     Dismiss",
  "*AuthReq*ScrollingText.width: 300",
  "*AuthReq*ulabel.label:      Username",
  "*AuthReq*plabel.label:      Password",
  "*AuthReq*title:             Authentication",

  "*urllabel.label:            URL   :",
  "*urllabel.left:             ChainLeft",
  "*urllabel.right:            ChainLeft",
  "*urldisplay.left:           ChainLeft",
  "*urldisplay.right:          ChainRight",
  "*urldisplay.fromHoriz:      urllabel",
  "*urldisplay*sensitive:      true",
  "*urldisplay.width:          500",
  "*urldisplay*editType:       edit",

  "*titlelabel.label:          Title :",
  "*titlelabel.left:           ChainLeft",
  "*titlelabel.right:          ChainLeft",
  "*titledisplay.left:         ChainLeft",
  "*titledisplay.right:        ChainRight",
  "*titledisplay.fromHoriz:    titlelabel",
  "*titledisplay*sensitive:    false",
  "*titledisplay.width:        500",
  "*titledisplay*displayCaret: False",
  "*titledisplay*editType:     read",

  "*Label.font: -*-lucidatypewriter-medium-r-normal-*-*-120-*-*-*-*-iso8859-1",
  "*Command.font: -*-lucida-bold-r-normal-sans-*-120-*-*-*-*-iso8859-1",
  "*Toggle.font: -*-lucida-bold-r-normal-sans-*-120-*-*-*-*-iso8859-1",
  "*urldisplay.font:    -*-lucida-bold-r-normal-sans-*-120-*-*-*-*-iso8859-1",
  "*titledisplay.font:  -*-lucida-bold-r-normal-sans-*-120-*-*-*-*-iso8859-1",

  "*header1Font:   -*-helvetica-bold-r-normal-*-*-240-*-*-*-*-iso8859-1",
  "*header2Font:   -*-helvetica-bold-r-normal-*-*-180-*-*-*-*-iso8859-1",
  "*header3Font:   -*-helvetica-bold-r-normal-*-*-140-*-*-*-*-iso8859-1",
  "*header4Font:   -*-helvetica-bold-r-normal-*-*-120-*-*-*-*-iso8859-1",
  "*header5Font:   -*-helvetica-bold-r-normal-*-*-100-*-*-*-*-iso8859-1",
  "*header6Font:   -*-helvetica-bold-r-normal-*-*-80-*-*-*-*-iso8859-1",
  "*font:          -*-helvetica-medium-r-normal-*-*-120-*-*-*-*-iso8859-1",
  "*boldFont:      -*-helvetica-bold-r-normal-*-*-120-*-*-*-*-iso8859-1",
  "*italicFont:    -*-helvetica-medium-o-normal-*-*-120-*-*-*-*-iso8859-1",
  "*addressFont:   -*-helvetica-medium-o-normal-*-*-140-*-*-*-*-iso8859-1",

  NULL /* I'm bitter about this because I have to include stdio.h which
          seems like worthlessness */
};
