/*
 * inline.c
 *
 * Copyright (C) 1993, 1994, 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 */
#include "copyright.h"
#include "options.h"

#include <stdio.h>
#include <sys/types.h>

#include <X11/Intrinsic.h>

#include "HTML.h"

#include "common.h"
#include "image.h"

#include "url.h"
#include "mime.h"
#include "document.h"
#include "convert.h"
#include "util.h"
#include "inline.h"


extern Image *gifLoad();
extern Image *xbitmapLoad();
extern Image *xpixmapLoad();
extern Image *pbmLoad();
extern void gammacorrect();

/*
 * AllocImageInfo
 */
static ImageInfo *
AllocImageInfo()
{
  ImageInfo *iinfo;

  iinfo = (ImageInfo *)alloc_mem(sizeof(ImageInfo));
  iinfo->internal = 0;
  iinfo->delayed = 0;
  iinfo->fetched = 0;
  iinfo->image = 0;
  iinfo->name = 0;

  return(iinfo);
}

/*
 * ConvertRGBImageToImageInfo
 */
static ImageInfo *
ConvertRGBImageToImageInfo(img)
Image *img;
{
  ImageInfo *iinfo;
  int csize;
  int dsize;

  iinfo = AllocImageInfo();

  iinfo->width = img->width;
  iinfo->height = img->height;

  csize = sizeof(unsigned short) * img->rgb.used;
  iinfo->reds = (unsigned short *)alloc_mem(csize);
  iinfo->greens = (unsigned short *)alloc_mem(csize);
  iinfo->blues = (unsigned short *)alloc_mem(csize);
  iinfo->num_colors = img->rgb.used;
  memcpy(iinfo->reds, img->rgb.red, csize);
  memcpy(iinfo->greens, img->rgb.green, csize);
  memcpy(iinfo->blues, img->rgb.blue, csize);

  dsize = img->width * img->height * sizeof(unsigned char);
  iinfo->image_data = (unsigned char *)alloc_mem(dsize);

  memcpy(iinfo->image_data, img->data, dsize);

  return(iinfo);
}

/*
 * ConvertTrueImageToImageInfo
 */
static ImageInfo *
ConvertTrueImageToImageInfo(img)
Image *img;
{
  ImageInfo *iinfo = NULL;
  int csize;
  int dsize;
  int i, j;
  int n;
  int ccount = 0;
  int maxcount = 256;
  unsigned short xr, xg, xb;
  int k = 0;

  iinfo = AllocImageInfo();

  iinfo->width = img->width;
  iinfo->height = img->height;

  dsize = img->width * img->height * sizeof(unsigned char);
  iinfo->image_data = (unsigned char *)alloc_mem(dsize);

  csize = sizeof(unsigned short) * maxcount;
  iinfo->reds = (unsigned short *)alloc_mem(csize);
  iinfo->greens = (unsigned short *)alloc_mem(csize);
  iinfo->blues = (unsigned short *)alloc_mem(csize);

  n = img->width * img->height * 3;

  for (i = 0; i < n; i += 3)
  {
    xr = (unsigned short)(img->data[i]) * 0xffff / 0xff;
    xg = (unsigned short)(img->data[i + 1]) * 0xffff / 0xff;
    xb = (unsigned short)(img->data[i + 2]) * 0xffff / 0xff;
    for (j = 0; j < ccount; j++)
    {
      if (iinfo->reds[j] == xr &&
	  iinfo->greens[j] == xg &&
	  iinfo->blues[j] == xb)
      {
	iinfo->image_data[k++] = j;
	break;
      }
    }
    if (j == ccount)
    {
      if (ccount == maxcount)
      {
	iinfo->image_data[k++] = 0;
      }
      else
      {
	iinfo->reds[j] = xr;
	iinfo->greens[j] = xg;
	iinfo->blues[j] = xb;
	iinfo->image_data[k++] = j;
	ccount++;
      }
    }
  }

  iinfo->num_colors = ccount;

  return(iinfo);
}

/*
 * CreateImageInfo
 *
 * Creates an Image structure from image data.
 */
ImageInfo *
CreateImageInfo(w, d, clist, path, stype, maxcolors, bg, gamma)
Widget w;
Document *d;
Convert *clist;
char *path;
int stype;
int maxcolors;
XColor *bg;
double gamma;
{
  Image *img, *nimg;
  RGBColor rgbc;
  Document *x;
  char *content;
  char *data;
  int datalen;
  ImageInfo *iinfo = NULL;	/* unless proven otherwise */

  if (d->ptext != NULL)
  {
    data = d->ptext;
    datalen = d->plen;
    content = d->pcontent;
  }
  else
  {
    data = d->text;
    datalen = d->len;
    content = d->content;
  }

  rgbc.red = bg->red;
  rgbc.green = bg->green;
  rgbc.blue = bg->blue;

  if (strcasecmp(content, "image/gif") == 0)
      img = gifLoad(data, datalen, &rgbc);
  else if (strcasecmp(content, "image/x-xbitmap") == 0 ||
	   strcasecmp(content, "image/xbm") == 0 ||
	   strcasecmp(content, "image/x-xbm") == 0)
  {
    img = xbitmapLoad(data, datalen, &rgbc);
  }
  else if (strcasecmp(content, "image/x-xpixmap") == 0 ||
	   strcasecmp(content, "image/xpm") == 0 ||
	   strcasecmp(content, "image/x-xpm") == 0)
  {
    img = xpixmapLoad(XtDisplay(w), DefaultScreen(XtDisplay(w)),
		      data, datalen, &rgbc);
  }
  else if (strcasecmp(content, "image/x-portable-anymap") == 0 ||
	   strcasecmp(content, "image/x-portable-bitmap") == 0 ||
	   strcasecmp(content, "image/x-portable-graymap") == 0 ||
	   strcasecmp(content, "image/x-portable-pixmap") == 0)
  {
    img = pbmLoad(data, datalen, &rgbc);
  }
  else
  {
    if (d->ptext != NULL) return(NULL);

    x = ConvertDocument(d, clist, "inline", path);
    if (x != NULL)
    {
      if (x->ptext != NULL &&
	  x->pcontent != NULL && strncasecmp(x->pcontent, "image/", 6) == 0)
      {
        iinfo = CreateImageInfo (w, x, clist, path, stype, maxcolors,
				 bg, gamma);  /* recursive call */
      }
      if (x != d) DestroyDocument(x);
    }

    return(iinfo);
  }

  if (img != NULL)
  {
    if (img->type != IBAD)
    {
      if (img->rgb.used < 3 )
      {
	int i;
	
	for(i = 0; i < img->rgb.used; ++i)
        {
	  if ( RGB_TO_TRUE (*(img->rgb.red + i),
			    *(img->rgb.green + i),
			    *(img->rgb.blue + i) ) == 0xffffff)
	  {
	    *(img->rgb.red + i) = bg->red;
	    *(img->rgb.green + i) = bg->green;
	    *(img->rgb.blue + i) = bg->blue;
	    break;
	  }
	}
      }

      if ((stype == MONO_DISPLAY || maxcolors == 2) && img->type != IBITMAP)
      {
	nimg = dither(img, 0);
	freeImage(img);
	img = nimg;
      }
      else if (img->rgb.used > maxcolors)
      {
	nimg = reduce(img, maxcolors, 0);
	freeImage(img);
	img = nimg;
      }

      if (img->type == IBITMAP)
      {
	nimg = expand(img);
	freeImage(img);
	img = nimg;
	gamma = 0.0;
      }

      if (img != NULL)
      {
	if (gamma > (double)0.0) gammacorrect(img, gamma);
	if (img->type == ITRUE)
	    iinfo = ConvertTrueImageToImageInfo(img);
	else if (img->type == IRGB)
	    iinfo = ConvertRGBImageToImageInfo(img);
	freeImage(img);
      }
    }
    else
    {
      free_mem((char *)img); /* probably need to free more than this. */
    }
  }

  return(iinfo);
}
