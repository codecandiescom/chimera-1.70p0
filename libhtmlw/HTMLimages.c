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

#include <stdio.h>
#include <ctype.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "HTMLP.h"
#include "NoImage.xbm"
#include "DelayedImage.xbm"
#include "AnchoredImage.xbm"

#define IMAGE_BORDER	2

ImageInfo no_image;
ImageInfo delayed_image;
ImageInfo anchored_image;

static int allocation_index[256];

/*
 * Free all the colors in the default colormap that we have allocated so far.
 */
void
FreeColors (dsp, colormap)
Display *dsp;
Colormap colormap;
{
  int i, j;
  unsigned long pix;

  for (i = 0; i < 256; i++)
  {
    if (allocation_index[i])
    {
      pix = (unsigned long) i;
      /*
       * Because X is stupid, we have to Free the color
       * once for each time we've allocated it.
       */
      for (j = 0; j < allocation_index[i]; j++)
      {
	XFreeColors (dsp, colormap, &pix, 1, 0L);
      }
    }
    allocation_index[i] = 0;
  }
}


/*
 * Find the closest color by allocating it, or picking an already allocated
 * color
 */
void
FindColor (dsp, colormap, colr)
Display *dsp;
Colormap colormap;
XColor *colr;
{
  int i, match;
#ifdef MORE_ACCURATE
  double rd, gd, bd, dist, mindist;
#else
  int rd, gd, bd, dist, mindist;
#endif /* MORE_ACCURATE */
  int cindx;
  static XColor def_colrs[256];
  static int have_colors = 0;
  int NumCells;

  NumCells = XDisplayCells(dsp, DefaultScreen(dsp));
  if (NumCells <= 2)
  {
    colr->pixel = (colr->red + colr->green + colr->blue > 98304) ?
      WhitePixel(dsp, DefaultScreen(dsp)) : BlackPixel(dsp, DefaultScreen(dsp)) ;
    XQueryColor(dsp, colormap, colr);
    return;
  }

  match = XAllocColor (dsp, colormap, colr);
  if (match == 0)
  {
    if (!have_colors)
    {
      for (i = 0; i < NumCells; i++)
      {
	def_colrs[i].pixel = i;
      }
      XQueryColors (dsp, colormap, def_colrs, NumCells);
      have_colors = 1;
    }
#ifdef MORE_ACCURATE
    mindist = 196608.0;		/* 256.0 * 256.0 * 3.0 */
    cindx = colr->pixel;
    for (i = 0; i < NumCells; i++)
    {
      rd = (def_colrs[i].red - colr->red) / 256.0;
      gd = (def_colrs[i].green - colr->green) / 256.0;
      bd = (def_colrs[i].blue - colr->blue) / 256.0;
      dist = (rd * rd) +
	(gd * gd) +
	(bd * bd);
      if (dist < mindist)
      {
	mindist = dist;
	cindx = def_colrs[i].pixel;
	if (dist == 0.0)
	{
	  break;
	}
      }
    }
#else
    mindist = 196608;		/* 256 * 256 * 3 */
    cindx = colr->pixel;
    for (i = 0; i < NumCells; i++)
    {
      rd = ((int) (def_colrs[i].red >> 8) -
	    (int) (colr->red >> 8));
      gd = ((int) (def_colrs[i].green >> 8) -
	    (int) (colr->green >> 8));
      bd = ((int) (def_colrs[i].blue >> 8) -
	    (int) (colr->blue >> 8));
      dist = (rd * rd) +
	(gd * gd) +
	(bd * bd);
      if (dist < mindist)
      {
	mindist = dist;
	cindx = def_colrs[i].pixel;
	if (dist == 0)
	{
	  break;
	}
      }
    }
#endif /* MORE_ACCURATE */
    colr->pixel = cindx;
    colr->red = def_colrs[cindx].red;
    colr->green = def_colrs[cindx].green;
    colr->blue = def_colrs[cindx].blue;
  }
  else
  {
    /*
     * Keep a count of how many times we have allocated the
     * same color, so we can properly free them later.
     */
    allocation_index[colr->pixel]++;

    /*
     * If this is a new color, we've actually changed the default
     * colormap, and may have to re-query it later.
     */
    if (allocation_index[colr->pixel] == 1)
    {
      have_colors = 0;
    }
  }
}


static int
highbit (ul)
unsigned long ul;
{
  /*
   * returns position of highest set bit in 'ul' as an integer (0-31),
   * or -1 if none.
   */

  int i;
  for (i = 31; ((ul & 0x80000000) == 0) && i >= 0; i--, ul <<= 1);
  return i;
}



int 
hb (ul)
unsigned long ul;
{
  int i;
  unsigned long testval;

  for (i = 31; i >= -1; i--)
  {
    if (i != -1)
    {
      testval = 1 << i;
      if (testval & ul)
	return i;
    }
  }
  return -1;
}


/*
 * Make an image of appropriate depth for display from image data.
 */
XImage *
MakeImage (dsp, data, width, height, depth, img_info)
Display *dsp;
unsigned char *data;
int width, height;
int depth;
const ImageInfo *img_info;
{
  int linepad, shiftnum;
  int shiftstart, shiftstop, shiftinc;
  int bitsperpixel, scanline_pad;
  int bytesperline;
  int temp;
  int w, h;
  XImage *newimage;
  unsigned char *bit_data, *bitp, *datap;
  Visual *theVisual;
  int bmap_order;
  unsigned long c;
  int rshift, gshift, bshift;

  bitsperpixel = _XGetBitsPerPixel(dsp, depth);
  scanline_pad = _XGetScanlinePad(dsp, depth);

  switch (bitsperpixel)
  {
  case 6:
  case 8:
    bit_data = (unsigned char *) XtMalloc (width * height);
    memcpy(bit_data, data, (width * height));
    bytesperline = width;
    newimage = XCreateImage (dsp,
			     DefaultVisual (dsp, DefaultScreen (dsp)),
			     depth, ZPixmap, 0, (char *) bit_data,
			     width, height, 8, bytesperline);
    break;
  case 1:
  case 2:
  case 4:
    if (BitmapBitOrder (dsp) == LSBFirst)
    {
      shiftstart = 0;
      shiftstop = 8;
      shiftinc = bitsperpixel;
    }
    else
    {
      shiftstart = 8 - bitsperpixel;
      shiftstop = -bitsperpixel;
      shiftinc = -bitsperpixel;
    }
    linepad = scanline_pad - (width % scanline_pad);
    bit_data = (unsigned char *) XtMalloc (((width + linepad) * height)
					   + 1);
    bitp = bit_data;
    datap = data;
    *bitp = 0;
    shiftnum = shiftstart;
    for (h = 0; h < height; h++)
    {
      for (w = 0; w < width; w++)
      {
	temp = *datap++ << shiftnum;
	*bitp = *bitp | temp;
	shiftnum = shiftnum + shiftinc;
	if (shiftnum == shiftstop)
	{
	  shiftnum = shiftstart;
	  bitp++;
	  *bitp = 0;
	}
      }
      for (w = 0; w < linepad; w++)
      {
	shiftnum = shiftnum + shiftinc;
	if (shiftnum == shiftstop)
	{
	  shiftnum = shiftstart;
	  bitp++;
	  *bitp = 0;
	}
      }
    }
    bytesperline = (width + linepad) * bitsperpixel / 8;
    newimage = XCreateImage (dsp,
			     DefaultVisual (dsp, DefaultScreen (dsp)),
			     depth, ZPixmap, 0, (char *) bit_data,
			     (width + linepad), height, scanline_pad,
			     bytesperline);
    break;
    /*
     * WARNING:  This depth 16 code is donated code for 16 bit
     * TrueColor displays.  I have no access to such displays, so I
     * can't really test it.
     * Donated by - andrew@icarus.demon.co.uk
     * 
     * And its totally broken on different 16bit displays
     * This hopefully fixes it.       - DWH 6/10/94 - davidh@use.com
     */
  case 16:
    {
      unsigned long red_mask, green_mask, blue_mask;
      int red_shift, green_shift, blue_shift;
      Visual *visual_info;

      visual_info = DefaultVisual (dsp, DefaultScreen (dsp));

      red_mask = visual_info->red_mask;
      green_mask = visual_info->green_mask;
      blue_mask = visual_info->blue_mask;

      red_shift = 15 - hb (red_mask);
      green_shift = 15 - hb (green_mask);
      blue_shift = 15 - hb (blue_mask);

      bit_data = (unsigned char *) XtMalloc (width * height * 2);
      bitp = bit_data;
      datap = data;
      for (w = (width * height); w > 0; w--)
      {
	temp = (((img_info->reds[(int) *datap] >> red_shift) &
		 red_mask) |
		((img_info->greens[(int) *datap] >> green_shift) &
		 green_mask) |
		((img_info->blues[(int) *datap] >> blue_shift) &
		 blue_mask));

	if (BitmapBitOrder (dsp) == MSBFirst)
	{
	  *bitp++ = (temp >> 8) & 0xff;
	  *bitp++ = temp & 0xff;
	}
	else
	{
	  *bitp++ = temp & 0xff;
	  *bitp++ = (temp >> 8) & 0xff;
	}

	datap++;
      }

      newimage = XCreateImage (dsp,
			       DefaultVisual (dsp, DefaultScreen (dsp)),
			       depth, ZPixmap, 0, (char *) bit_data,
			       width, height, 16, 0);
    }
    break;
  case 24:
  case 32:
    bit_data = (unsigned char *) XtMalloc (width * height * 4);

    theVisual = DefaultVisual (dsp, DefaultScreen (dsp));
    rshift = highbit (theVisual->red_mask) - 7;
    gshift = highbit (theVisual->green_mask) - 7;
    bshift = highbit (theVisual->blue_mask) - 7;
    bmap_order = BitmapBitOrder (dsp);

    bitp = bit_data;
    datap = data;
    for (w = (width * height); w > 0; w--)
    {
      c =
	(((img_info->reds[(int) *datap] >> 8) & 0xff) << rshift) |
	(((img_info->greens[(int) *datap] >> 8) & 0xff) << gshift) |
	(((img_info->blues[(int) *datap] >> 8) & 0xff) << bshift);

      datap++;

      if (bmap_order == MSBFirst)
      {
	*bitp++ = (unsigned char) ((c >> 24) & 0xff);
	*bitp++ = (unsigned char) ((c >> 16) & 0xff);
	*bitp++ = (unsigned char) ((c >> 8) & 0xff);
	*bitp++ = (unsigned char) (c & 0xff);
      }
      else
      {
	*bitp++ = (unsigned char) (c & 0xff);
	*bitp++ = (unsigned char) ((c >> 8) & 0xff);
	*bitp++ = (unsigned char) ((c >> 16) & 0xff);
	*bitp++ = (unsigned char) ((c >> 24) & 0xff);
      }
    }

    newimage = XCreateImage (dsp,
			     DefaultVisual (dsp, DefaultScreen (dsp)),
			     depth, ZPixmap, 0, (char *) bit_data,
			     width, height, 32, 0);
    break;
  default:
    fprintf (stderr, "Don't know how to format image for display of depth %d\n", depth);
    return (NULL);
  }

  return (newimage);
}


int
AnchoredHeight (hw)
HTMLWidget hw;
{
  return ((int) (AnchoredImage_height + IMAGE_BORDER));
}


static const char *const ismapformstring = "ISMAP Form";

char *
IsMapForm (hw)
HTMLWidget hw;
{
  return (XtNewString (ismapformstring));
}


int
IsIsMapForm (hw, href)
HTMLWidget hw;
char *href;
{
  return (href != NULL  &&
	  strcmp (href, ismapformstring) == 0);
}


/*
 * Delayed images are anchored if they aren't already.
 * This accomplishes several things:
 * 1) _HTMLInput (which handles mouse button up events) is only called by
 *     ExtendEnd for button up events in an anchor, and _HTMLInput only
 *     processes elements that look like anchors;
 * 2) a standard anchor-style bounding box will be drawn around the delayed
 *     image icon, so that it's borders are obvious; and
 * 3) the Anchor Display feature will consider the presence of the mouse
 *     cursor in the delayed image area to be worth highlighting.
 *
 * Clicking on a delayed image will cause the image to be loaded (see
 * _HTMLInput() ).
 */

static const char *const delayedhrefstring = "Delayed Image";

char *
DelayedHRef (hw)
HTMLWidget hw;
{
  return (XtNewString (delayedhrefstring));
}


int
IsDelayedHRef (hw, href)
HTMLWidget hw;
char *href;
{
  return (href != NULL  &&
	  strcmp (href, delayedhrefstring) == 0);
}


ImageInfo *
DelayedImageData (anchored)
Boolean anchored;
{
  static Boolean di_inited = False;

  if (di_inited == False)
  {
    anchored_image.delayed = True;
    anchored_image.internal = True;
    anchored_image.fetched = 0;
    anchored_image.width = DelayedImage_width;
    anchored_image.height =
	DelayedImage_height + AnchoredImage_height + IMAGE_BORDER;
    anchored_image.num_colors = 0;
    anchored_image.reds = NULL;
    anchored_image.greens = NULL;
    anchored_image.blues = NULL;
    anchored_image.image_data = NULL;
    anchored_image.image = (Pixmap) NULL;
    anchored_image.name = NULL;
    anchored_image.next = NULL;

    delayed_image.delayed = True;
    delayed_image.internal = True;
    delayed_image.fetched = 0;
    delayed_image.width = DelayedImage_width;
    delayed_image.height = DelayedImage_height;
    delayed_image.num_colors = 0;
    delayed_image.reds = NULL;
    delayed_image.greens = NULL;
    delayed_image.blues = NULL;
    delayed_image.image_data = NULL;
    delayed_image.image = (Pixmap) NULL;
    delayed_image.name = NULL;
    delayed_image.next = NULL;

    di_inited = True;
  }

  return (anchored == True) ?  &anchored_image : &delayed_image;
}


ImageInfo *
NoImageData ()
{
  static Boolean ni_inited = False;

  if (ni_inited == False)
  {
    no_image.delayed = 0;
    no_image.internal = True;
    no_image.fetched = 0;
    no_image.width = NoImage_width;
    no_image.height = NoImage_height;
    no_image.num_colors = 0;
    no_image.reds = NULL;
    no_image.greens = NULL;
    no_image.blues = NULL;
    no_image.image_data = NULL;
    no_image.image = (Pixmap) NULL;
    no_image.name = NULL;
    no_image.next = NULL;

    ni_inited = True;
  }

  return (&no_image);
}


/*
 * Build pixmaps for the internal images
 */
void
BuildInternalImagePixmaps (hw)
HTMLWidget hw;
{
  if (no_image.image == (Pixmap) NULL)
  {
    (void) NoImageData ();	/* make sure noimage has been init'd */
    no_image.image =
	XCreatePixmapFromBitmapData (XtDisplay (hw->html.view),
				     XtWindow (hw->html.view),
				     NoImage_bits,
				     NoImage_width, NoImage_height,
				     hw->html.foreground,
				     hw->core.background_pixel,
				     DefaultDepthOfScreen (XtScreen (hw)));
  }

  if (delayed_image.image == (Pixmap) NULL)
  {
    (void) DelayedImageData (False);  /* make sure delayed_image init'd */
    delayed_image.image =
	XCreatePixmapFromBitmapData (XtDisplay (hw->html.view),
				     XtWindow (hw->html.view),
				     DelayedImage_bits,
				     DelayedImage_width, DelayedImage_height,
				     hw->html.foreground,
				     hw->core.background_pixel,
				     DefaultDepthOfScreen (XtScreen (hw)));
  }

  if (anchored_image.image == (Pixmap) NULL)
  {
    Pixmap pix;

    (void) DelayedImageData (True);  /* make sure anchored_image init'd */
    anchored_image.image =
	XCreatePixmapFromBitmapData (XtDisplay (hw->html.view),
				     XtWindow (hw->html.view),
				     AnchoredImage_bits,
				     AnchoredImage_width, AnchoredImage_height,
				     hw->html.foreground,
				     hw->core.background_pixel,
				     DefaultDepthOfScreen (XtScreen (hw)));
    pix = XCreatePixmap (XtDisplay (hw->html.view),
			 XtWindow (hw->html.view),
			 DelayedImage_width,
			 (DelayedImage_height + AnchoredImage_height +
			    IMAGE_BORDER),
			 DefaultDepthOfScreen (XtScreen (hw)));
    XSetForeground (XtDisplay (hw), hw->html.drawGC,
		    hw->core.background_pixel);
    XFillRectangle (XtDisplay (hw->html.view), pix,
		    hw->html.drawGC, 0, 0,
		    DelayedImage_width,
		    (DelayedImage_height + AnchoredImage_height +
		     IMAGE_BORDER));
    XCopyArea (XtDisplay (hw->html.view),
	       anchored_image.image, pix, hw->html.drawGC,
	       0, 0, AnchoredImage_width, AnchoredImage_height,
	       0, 0);
    XCopyArea (XtDisplay (hw->html.view),
	       delayed_image.image, pix, hw->html.drawGC,
	       0, 0, DelayedImage_width, DelayedImage_height,
	       0, (AnchoredImage_height + IMAGE_BORDER));
    XFreePixmap (XtDisplay (hw->html.view), anchored_image.image);

    anchored_image.image = pix;
  }
}


Pixmap
InfoToImage (hw, img_info)
HTMLWidget hw;
const ImageInfo *img_info;
{
  int i, size;
  int delta, not_right_col, not_last_row;
  Pixmap Img;
  XImage *tmpimage;
  XColor tmpcolr;
  int *Mapping;
  unsigned char *tmpdata;
  unsigned char *ptr;
  unsigned char *ptr2;
  int Vclass;
  XVisualInfo vinfo, *vptr;
  Boolean need_to_dither;
  unsigned long black_pixel;
  unsigned long white_pixel;

  /* find the visual class. */
  vinfo.visualid = XVisualIDFromVisual (DefaultVisual (XtDisplay (hw),
					   DefaultScreen (XtDisplay (hw))));
  vptr = XGetVisualInfo (XtDisplay (hw), VisualIDMask, &vinfo, &i);
  Vclass = vptr->class;
  if (vptr->depth == 1)
  {
    need_to_dither = True;
    black_pixel = BlackPixel (XtDisplay (hw),
			      DefaultScreen (XtDisplay (hw)));
    white_pixel = WhitePixel (XtDisplay (hw),
			      DefaultScreen (XtDisplay (hw)));
  }
  else
  {
    need_to_dither = False;
  }
  XFree ((char *) vptr);

  Mapping = (int *) XtMalloc (img_info->num_colors * sizeof (int));

  for (i = 0; i < img_info->num_colors; i++)
  {
    tmpcolr.red = img_info->reds[i];
    tmpcolr.green = img_info->greens[i];
    tmpcolr.blue = img_info->blues[i];
    tmpcolr.flags = DoRed | DoGreen | DoBlue;
    if ((Vclass == TrueColor) || (Vclass == DirectColor))
    {
      Mapping[i] = i;
    }
    else if (need_to_dither == True)
    {
      Mapping[i] = ((tmpcolr.red >> 5) * 11 +
		    (tmpcolr.green >> 5) * 16 +
		    (tmpcolr.blue >> 5) * 5) / (65504 / 64);
    }
    else
    {
      FindColor (XtDisplay (hw),
		 DefaultColormapOfScreen (XtScreen (hw)),
		 &tmpcolr);
      Mapping[i] = tmpcolr.pixel;
    }
  }

  /*
   * Special case:  For 2 color non-black&white images, instead
   * of 2 dither patterns, we will always drop them to be
   * black on white.
   */
  if ((need_to_dither == True) && (img_info->num_colors == 2))
  {
    if (Mapping[0] < Mapping[1])
    {
      Mapping[0] = 0;
      Mapping[1] = 64;
    }
    else
    {
      Mapping[0] = 64;
      Mapping[1] = 0;
    }
  }

  size = img_info->width * img_info->height;
  if (size == 0)
  {
    tmpdata = NULL;
  }
  else
  {
    tmpdata = (unsigned char *) XtMalloc (size);
  }
  if (tmpdata == NULL)
  {
    tmpimage = NULL;
    Img = (Pixmap) NULL;
  }
  else
  {
    ptr = img_info->image_data;
    ptr2 = tmpdata;

    if (need_to_dither == True)
    {
      int cx, cy;

      for (ptr2 = tmpdata, ptr = img_info->image_data;
	   ptr2 < tmpdata + (size - 1); ptr2++, ptr++)
      {
	*ptr2 = Mapping[(int) *ptr];
      }

      ptr2 = tmpdata;
      for (cy = 0; cy < img_info->height; cy++)
      {
	for (cx = 0; cx < img_info->width; cx++)
	{
	  /*
	   * Assume high numbers are
	   * really negative.
	   */
	  if (*ptr2 > 128)
	  {
	    *ptr2 = 0;
	  }
	  if (*ptr2 > 64)
	  {
	    *ptr2 = 64;
	  }

	  /*
	   * Traditional Floyd-Steinberg
	   */
	  if (*ptr2 < 32)
	  {
	    delta = *ptr2;
	    *ptr2 = black_pixel;
	  }
	  else
	  {
	    delta = *ptr2 - 64;
	    *ptr2 = white_pixel;
	  }
	  if ((not_right_col =
	       (cx < (img_info->width - 1))))
	  {
	    *(ptr2 + 1) += delta * 7 >> 4;
	  }

	  if ((not_last_row =
	       (cy < (img_info->height - 1))))
	  {
	    (*(ptr2 + img_info->width)) +=
	      delta * 5 >> 4;
	  }

	  if (not_right_col && not_last_row)
	  {
	    (*(ptr2 + img_info->width + 1)) +=
	      delta >> 4;
	  }

	  if (cx && not_last_row)
	  {
	    (*(ptr2 + img_info->width - 1)) +=
	      delta * 3 >> 4;
	  }
	  ptr2++;
	}
      }
    }				/* end if (need_to_dither==True) */
    else
    {

      for (i = 0; i < size; i++)
      {
	*ptr2++ = (unsigned char) Mapping[(int) *ptr];
	ptr++;
      }
    }

    tmpimage = MakeImage (XtDisplay (hw), tmpdata,
			  img_info->width, img_info->height,
			  DefaultDepthOfScreen (XtScreen (hw)), img_info);

    /* Caught by Purify; should be OK. */
    XtFree(tmpdata);

    Img = XCreatePixmap (XtDisplay (hw->html.view),
			 XtWindow (hw->html.view),
			 img_info->width, img_info->height,
			 DefaultDepthOfScreen (XtScreen (hw)));
  }

  if ((tmpimage == NULL) || (Img == (Pixmap) NULL))
  {
    if (tmpimage != NULL)
    {
      XDestroyImage (tmpimage);
    }
    if (Img != (Pixmap) NULL)
    {
      XFreePixmap (XtDisplay (hw), Img);
      Img = (Pixmap) NULL;	/* becomes the result returned */
    }
  }
  else
  {
    XPutImage (XtDisplay (hw), Img, hw->html.drawGC, tmpimage, 0, 0,
	       0, 0, img_info->width, img_info->height);
    XDestroyImage (tmpimage);
  }

  /* Caught by Purify; should be OK. */
  XtFree((char *) Mapping);

  return (Img);
}


/*
 * Free the ImageInfo block and the all the resources it owns.
 * Returns the value of pic_data->next as a convenience to the caller.
 */
ImageInfo *
FreeOneImage (hw, pic_data)
HTMLWidget hw;
ImageInfo *pic_data;		/* NULL allowed */
{
  ImageInfo *next;

  /*
   * Don't actually free special built-in images
   * pic_data == NULL is also accepted.
   */
  if (pic_data != NULL  &&
      pic_data->internal == False  &&	 /* the test that ought to work */
      pic_data != &delayed_image  &&	/* paranoia */
      pic_data != &anchored_image &&	/* paranoia */
      pic_data != &no_image)		/* paranoia */
  {
    if (pic_data->image_data != NULL) free_mem ((char *)pic_data->image_data);
    if (pic_data->reds != NULL)       free_mem ((char *)pic_data->reds);
    if (pic_data->greens != NULL)     free_mem ((char *)pic_data->greens);
    if (pic_data->blues != NULL)      free_mem ((char *)pic_data->blues);
    if (pic_data->name != NULL)       free_mem ((char *)pic_data->name);
    if (pic_data->image != (Pixmap) NULL)
	XFreePixmap (XtDisplay (hw), pic_data->image);

    next = pic_data->next;
    free_mem ((char *)pic_data);
  }
  else next = NULL;

  return next;
}


/*
 * Free up all the image data used by the current document.
 */
void
FreeImages (hw)
HTMLWidget hw;
{
  ImageInfo *pic_data = hw->html.image_list;

  while (pic_data != NULL)   pic_data = FreeOneImage (hw, pic_data);

  hw->html.image_list = NULL;
}


/*
 * Search the list of images for one with NAME.
 * Return a pointer to an ImageInfo *, or NULL if not found.
 */
ImageInfo *
IsCurrentImage (list, name)
ImageInfo *list;
const char *const name;
{
  if (list == NULL)
  {
    return (NULL);
  }

  for ( ;  list != NULL;  list = list->next)
  {
    if (strcmp (list->name, name) == 0)
    {
      return list;
    }
  }

  return NULL;
}


/*
 * Do appropriate things to get the specified image or a suitable substitute.
 * If a new real image is obtained, it is added to the image_list.
 * eptr->img_data is created and initialized, and img_data->pic_data is set
 * to whatever image we're going to display.
 * This code does not generate pic_data->image; ImageRefresh() does that.
 */
void
HTMLGetImage (hw, eptr, now)
HTMLWidget hw;
struct ele_rec *eptr;
Boolean now;			/* True: force loading now */
{
  const char *const edata = eptr->edata;
  ImageInfo *pic_data;

  /*
   * Is this image on image_list?
   */
  pic_data = IsCurrentImage (hw->html.image_list, edata);

  if (pic_data == NULL)		/* not on image_list; a new image */
  {
    if (hw->html.resolveImage != NULL)
    {
      /*
       * arg 3:  True = don't load if downloading is required
       */
      pic_data = (* (resolveImageProc) (hw->html.resolveImage) )
	         (hw, edata, (hw->html.delay_images  &&  !now));

      if (pic_data != NULL)	/* got a real image */
      {
	if (pic_data->image_data != NULL)
	{
	  pic_data->delayed = 0;
	  pic_data->name    = XtNewString (edata);
	  pic_data->fetched = True;

	  /*
	   * Add pic_data to head of image_list.  Adding to the head helps
	   * in the common case where ImagePlace() asks for the image twice
	   * in a row because it didn't fit in the first place tried.
	   */
	  pic_data->next = hw->html.image_list;
	  hw->html.image_list = pic_data;
	}
	else			/* well, *that* was useless */
	{
	  (void) FreeOneImage (pic_data);
	  pic_data = NULL;
	}
      }
      /*
       * The inclusion of && !now is optional.  It's arguable whether to
       * give the user another try when delayed image loading fails or
       * leave the user thinking Chimera is broken because it didn't load
       * the image and redisplays the delayed image icon.  I opted for
       * making it clear the load failed.  The user still has other ways
       * to prod Chimera to try again to load the image.
       */
      if (pic_data == NULL  &&	/* don't have an image */
	  hw->html.delay_images  &&  !now)
      {
	/*
	 * Use "delayed image" or "delayed image that's an anchor" box.
	 */
	if (eptr->anchorHRef == NULL)  /* image isn't part of an anchor */
	{
	  pic_data = DelayedImageData (False);
	  /*
	   * Make delayed images look like anchors.
	   * Why?  See DelayedHRef()  (above).
	   */
	  eptr->anchorHRef = DelayedHRef (hw);  /* pseudo-anchor */
	  eptr->fg = hw->html.anchor_fg;  /* just like a real anchor */
	}
	else			/* delayed image that's an anchor */
	{
	  pic_data = DelayedImageData (True);
	}
      }
    }

    if (pic_data == NULL)	/* still nothing?  Give up */
    {
      pic_data = NoImageData ();  /* never returns NULL */
    }
  }

  /*
   * Create the img_data structure to hold any HTML tag qualifiers.
   */
  FreeIMGInfo (eptr->img_data);	 /* free any old img_data; NULL okay */

  eptr->img_data = (IMGInfo *) XtMalloc (sizeof (IMGInfo));
  eptr->img_data->pic_data = pic_data;
  
  /*
   * Note: never exits with eptr->img_data->pic_data non-NULL
   */
}
