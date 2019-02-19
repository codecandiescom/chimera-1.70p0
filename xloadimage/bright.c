/* bright.c
 *
 * miscellaneous colormap altering functions
 *
 * jim frost 10.10.89
 *
 * Copyright 1989, 1991 Jim Frost.
 * See included file "copyright.h" for complete copyright information.
 */
#include <math.h>

#include "common.h"
#include "copyright.h"
#include "image.h"

/* alter an image's brightness by a given percentage
 */

void brighten(image, percent)
     Image        *image;
     unsigned int  percent;
{ int          a;
  unsigned int newrgb;
  float        fperc;
  unsigned int size;
  byte        *destptr;

  goodImage(image, "brighten");
  if (BITMAPP(image)) /* we're AT&T */
    return;

  fperc= (float)percent / 100.0;

  switch (image->type) {
  case IRGB:
    for (a= 0; a < image->rgb.used; a++) {
      newrgb= *(image->rgb.red + a) * fperc;
      if (newrgb > 65535)
	newrgb= 65535;
      *(image->rgb.red + a)= newrgb;
      newrgb= *(image->rgb.green + a) * fperc;
      if (newrgb > 65535)
	newrgb= 65535;
      *(image->rgb.green + a)= newrgb;
      newrgb= *(image->rgb.blue + a) * fperc;
      if (newrgb > 65535)
	newrgb= 65535;
      *(image->rgb.blue + a)= newrgb;
    }
    break;

  case ITRUE:
    size= image->width * image->height * 3;
    destptr= image->data;
    for (a= 0; a < size; a++) {
      newrgb= *destptr * fperc;
      if (newrgb > 255)
	newrgb= 255;
      *(destptr++)= newrgb;
    }
    break;
  }
}

static void
make_gamma( gamma, gammamap )
double gamma;
int gammamap[256];
{
  register int i;

  for ( i = 0; i < 256; i++ )
  {
#ifdef BYTEBUG
    int byteb1;

    byteb1 = (int)(0.5 + 255 * pow( i / 255.0, 1.0/gamma ));
    gammamap[i] = byteb1;
#else
    gammamap[i] = (int)(0.5 + 255 * pow( i / 255.0, 1.0/gamma ));
#endif
  }

  return;
}

void gammacorrect(image, disp_gam)
     Image *image;
     double disp_gam;
{ int          a;
  int gammamap[256];
  unsigned int size;
  byte        *destptr;

  goodImage(image, "gammacorrect");
  if (BITMAPP(image)) /* we're AT&T */
    return;

  make_gamma(disp_gam,gammamap);

  switch (image->type) {
  case IRGB:
    for (a= 0; a < image->rgb.used; a++) {
      *(image->rgb.red + a)= gammamap[(*(image->rgb.red + a))>>8]<<8;
      *(image->rgb.green + a)= gammamap[(*(image->rgb.green + a))>>8]<<8;
      *(image->rgb.blue + a)= gammamap[(*(image->rgb.blue + a))>>8]<<8;
    }
    break;

  case ITRUE:
    size= image->width * image->height * 3;
    destptr= image->data;
    for (a= 0; a < size; a++) {
      *destptr= gammamap[*destptr];
      destptr++;
    }
    break;
  }

}

/* this initializes a lookup table for doing normalization
 */

static void setupNormalizationArray(min, max, array)
     unsigned int min, max;
     byte *array;
{ int a;
  unsigned int new;
  float factor;

  factor= 256.0 / (max - min);
  for (a= min; a <= max; a++) {
    new= (float)(a - min) * factor;
    array[a]= (new > 255 ? 255 : new);
  }
}

/* normalize an image.
 */

Image *normalize(image)
     Image        *image;
{ unsigned int  a, x, y;
  unsigned int  min, max;
  Pixel         pixval;
  Image        *newimage;
  byte         *srcptr, *destptr;
  byte          array[256];

  goodImage(image, "normalize");
  if (BITMAPP(image))
    return(image);

  switch (image->type) {
  case IRGB:
    min= 256;
    max = 0;
    for (a= 0; a < image->rgb.used; a++) {
      byte red, green, blue;

      red= image->rgb.red[a] >> 8;
      green= image->rgb.green[a] >> 8;
      blue= image->rgb.blue[a] >> 8;
      if (red < min)
	min= red;
      if (red > max)
	max= red;
      if (green < min)
	min= green;
      if (green > max)
	max= green;
      if (blue < min)
	min= blue;
      if (blue > max)
	max= blue;
    }
    setupNormalizationArray(min, max, array);

    newimage= newTrueImage(image->width, image->height);
    if (image->title) {
	newimage->title = (char *)alloc_mem(strlen(image->title) + 14);
	sprintf(newimage->title, "%s (normalized)", image->title);
    }
    srcptr= image->data;
    destptr= newimage->data;
    for (y= 0; y < image->height; y++)
      for (x= 0; x < image->width; x++) {
	pixval= memToVal(srcptr, image->pixlen);
	*destptr= array[image->rgb.red[pixval] >> 8];
	*destptr++;
	*destptr= array[image->rgb.green[pixval] >> 8];
	*destptr++;
	*destptr= array[image->rgb.blue[pixval] >> 8];
	*destptr++;
	srcptr += image->pixlen;
      }
    break;

  case ITRUE:
    srcptr= image->data;
    min= 255;
    max= 0;
    for (y= 0; y < image->height; y++)
      for (x= 0; x < image->width; x++) {
	if (*srcptr < min)
	  min= *srcptr;
	if (*srcptr > max)
	  max= *srcptr;
	srcptr++;
	if (*srcptr < min)
	  min= *srcptr;
	if (*srcptr > max)
	  max= *srcptr;
	srcptr++;
	if (*srcptr < min)
	  min= *srcptr;
	if (*srcptr > max)
	  max= *srcptr;
	srcptr++;
      }
    setupNormalizationArray(min, max, array);

    srcptr= image->data;
    for (y= 0; y < image->height; y++)
      for (x= 0; x < image->width; x++) {
	*srcptr= array[*srcptr];
	srcptr++;
	*srcptr= array[*srcptr];
	srcptr++;
	*srcptr= array[*srcptr];
	srcptr++;
      }
    newimage= image;
  }

  return(newimage);
}

/* convert to grayscale
 */

void gray(image)
     Image *image;
{ int a;
  unsigned int size;
  Intensity intensity, red, green, blue;
  byte *destptr;

  goodImage(image, "gray");
  if (BITMAPP(image))
    return;

  switch (image->type) {
  case IRGB:
    for (a= 0; a < image->rgb.used; a++) {
      intensity= colorIntensity(image->rgb.red[a],
				image->rgb.green[a],
				image->rgb.blue[a]);
      image->rgb.red[a]= intensity;
      image->rgb.green[a]= intensity;
      image->rgb.blue[a]= intensity;
    }
    break;

  case ITRUE:
    size= image->width * image->height;
    destptr= image->data;
    for (a= 0; a < size; a++) {
      red= *destptr << 8;
      green= *(destptr + 1) << 8;
      blue= *(destptr + 2) << 8;
      intensity= colorIntensity(red, green, blue) >> 8;
      *(destptr++)= intensity; /* red */
      *(destptr++)= intensity; /* green */
      *(destptr++)= intensity; /* blue */
    }
    break;
  }
}
