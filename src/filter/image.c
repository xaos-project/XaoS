#ifndef _plan9_
#include <string.h>
#include <fconfig.h>
#include <assert.h>
#ifdef NO_MALLOC_H
#include <stdlib.h>
#else
#include <malloc.h>
#endif
#include <stdio.h>
#else
#include <u.h>
#include <libc.h>
#include <stdio.h>
#include <fconfig.h>
#endif
#include <filter.h>
void
flipgeneric (struct image *img)
{
  pixel_t **line;
  assert (img->nimages == 2);
  img->currimage ^= 1;
  line = img->currlines;
  img->currlines = img->oldlines;
  img->oldlines = line;
}

int
bytesperpixel (int type)
{
  switch (type)
    {
    case MBITMAP:
    case LBITMAP:
    case LIBITMAP:
    case MIBITMAP:
      return 0;
    case SMALLITER:
    case FIXEDCOLOR:
    case GRAYSCALE:
    case C256:
      return 1;
    case LARGEITER:
    case TRUECOLOR16:
      return 2;
    case TRUECOLOR24:
      return 3;
    case TRUECOLOR:
      return 4;
    default:
      assert (0);
      return 0;
    }
}
struct image *
create_image_lines (int width, int height,
		    int nimages, pixel_t ** lines1, pixel_t ** lines2,
		    struct palette *palette,
		    void (*flip) (struct image * img), int flags,
		    float pixelwidth, float pixelheight)
{
  int i;
  static int version = 1;
  struct image *img = (struct image *) calloc (1, sizeof (*img));
  if (img == NULL)
    return NULL;
  if (flip == NULL)
    flip = flipgeneric;
  img->width = width;
  img->height = height;
  img->nimages = nimages;
  img->bytesperpixel = bytesperpixel (palette->type);
  img->palette = palette;
  img->currimage = 0;
  img->flip = flip;
  img->flags = flags;
  img->version = version;
  version += 65535;
  img->currlines = lines1;
  img->oldlines = lines2;
  img->pixelwidth = pixelwidth;
  img->pixelheight = pixelheight;
  if (lines1 != NULL && (nimages != 2 || lines2 != NULL))
    {
      img->scanline = (int) (lines1[1] - lines1[0]);
      if (img->scanline < 0)
	img->scanline = -1;
      else
	{
	  for (i = 0; i < height; i++)
	    {
	      if (lines1[0] - lines1[i] != img->scanline * i)
		{
		  img->scanline = -1;
		  break;
		}
	      if (nimages == 2 && lines2[0] - lines2[i] != img->scanline * i)
		{
		  img->scanline = -1;
		  break;
		}
	    }
	}
    }
  else
    img->scanline = -1;
  return (img);
}
struct image *
create_image_cont (int width, int height, int scanlinesize,
		   int nimages, pixel_t * buf1, pixel_t * buf2,
		   struct palette *palette, void (*flip) (struct image * img),
		   int flags, float pixelwidth, float pixelheight)
{
  struct image *img =
    create_image_lines (width, height, nimages, NULL, NULL, palette, flip,
			flags, pixelwidth, pixelheight);
  int i;
  if (img == NULL)
    {
      return NULL;
    }
  if ((img->currlines =
       (pixel_t **) malloc (sizeof (*img->currlines) * height)) == NULL)
    {
      free (img);
      return NULL;
    }
  if (nimages == 2)
    {
      if ((img->oldlines =
	   (pixel_t **) malloc (sizeof (*img->oldlines) * height)) == NULL)
	{
	  free (img->currlines);
	  free (img);
	  return NULL;
	}
    }
  for (i = 0; i < img->height; i++)
    {
      img->currlines[i] = buf1;
      buf1 += scanlinesize;
    }
  if (nimages == 2)
    for (i = 0; i < img->height; i++)
      {
	img->oldlines[i] = buf2;
	buf2 += scanlinesize;
      }
  img->flags |= FREELINES;
  img->scanline = scanlinesize;
  return (img);
}
struct image *
create_image_mem (int width, int height, int nimages, struct palette *palette,
		  float pixelwidth, float pixelheight)
{
  unsigned char *data = (unsigned char *) calloc (((width + 3) & ~3) * height,
						  bytesperpixel
						  (palette->type));
  unsigned char *data1 =
    (unsigned char *) (nimages == 2 ? calloc (((width + 3) & ~3) * height,
					      bytesperpixel (palette->type)) :
		       NULL);
  struct image *img;
  if (data == NULL)
    {
#ifdef DEBUG
      printf ("Image:out of memory\n");
#endif
      return (NULL);
    }
  if (nimages == 2 && data1 == NULL)
    {
      free (data);
#ifdef DEBUG
      printf ("Image:out of memory2\n");
#endif
      return NULL;
    }
  img =
    create_image_cont (width, height,
		       ((width + 3) & ~3) * bytesperpixel (palette->type),
		       nimages, data, data1, palette, NULL, 0, pixelwidth,
		       pixelheight);
  if (img == NULL)
    {
      free (data);
      if (data1 != NULL)
	free (data1);
      return NULL;
    }
  img->flags |= FREEDATA;
  return (img);
}
struct image *
create_subimage (struct image *simg, int width, int height, int nimages,
		 struct palette *palette, float pixelwidth, float pixelheight)
{
  int size = height * bytesperpixel (palette->type);
  int i;
  int shift1 = 0, shift2 = 0;
  struct image *img;
  if (size > simg->height * simg->bytesperpixel || height > simg->height
      || (nimages == 2 && simg->nimages == 1))
    return (create_image_mem
	    (width, height, nimages, palette, pixelwidth, pixelheight));
  nimages = simg->nimages;
  img =
    create_image_lines (width, height, nimages, NULL, NULL, palette, NULL, 0,
			pixelwidth, pixelheight);
  if (img == NULL)
    return NULL;
  if ((img->currlines =
       (pixel_t **) malloc (sizeof (*img->currlines) * height)) == NULL)
    {
      free (img);
      return NULL;
    }
  if (nimages == 2)
    {
      if ((img->oldlines =
	   (pixel_t **) malloc (sizeof (*img->oldlines) * height)) == NULL)
	{
	  free (img->currlines);
	  free (img);
	  return NULL;
	}
    }
  shift1 = simg->height - img->height;
  shift2 =
    simg->width * simg->bytesperpixel - img->width * img->bytesperpixel;
  for (i = 0; i < img->height; i++)
    {
      img->currlines[i] = simg->currlines[i + shift1] + shift2;
    }
  if (nimages == 2)
    for (i = 0; i < img->height; i++)
      {
	img->oldlines[i] = simg->oldlines[i + shift1] + shift2;
      }
  img->flags |= FREELINES;
  img->currimage = simg->currimage;
  return (img);
}

void
destroy_image (struct image *img)
{
  if (img->flags & FREEDATA)
    {
      free (*img->currlines);
      if (img->nimages == 2)
	free (*img->oldlines);
    }
  if (img->flags & FREELINES)
    {
      free (img->currlines);
      if (img->nimages == 2)
	free (img->oldlines);
    }
  free (img);
}

void
clear_image (struct image *img)
{
  int i;
  int color = img->palette->pixels[0];
  int width = img->width * img->bytesperpixel;
  if (img->palette->npreallocated)
    color = img->palette->index[0];
  if (!width)
    {
      width = (img->width + 7) / 8;
      if (color)
	color = 255;
    }
  for (i = 0; i < img->height; i++)
    memset (img->currlines[i], color, width);
}
