/*
 * This file contains code of three conversion filters based at dithering:
 * truecolor conversion filter - it should convert 8bpp, fixedcolor and
 *    bitmaps to truecolor
 * fixedcolor conversion - emulates user palette at 8bpp displays with static
 *    palette
 * bitmap conversion - emulates 8bpp at 1bpp displays.
 *
 * Since this filters share quite a lot code, they are implemented in one
 * file. Internal loops are quire messy, since they are optimized for speed.
 * Let me know about all ideas to make it faster.
 *
 * Note that quite interesting alg. is for preparing dithering table at
 * fixedcolor displays.
 */
#include <config.h>
#ifndef _plan9_
#ifndef NO_MALLOC_H
#include <malloc.h>
#endif
#include <stdio.h>
#include <string.h>
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif
#include <stdlib.h>
#else
#include <u.h>
#include <libc.h>
#include <stdio.h>
#endif
#include <fconfig.h>
#include <filter.h>
#include <xerror.h>
#include <fractal.h>
#include <xthread.h>
#include <archaccel.h>
#define MSIZE 8
static CONST unsigned char matrix[MSIZE][MSIZE] = {
  {0, 192, 48, 240, 12, 204, 60, 252,},
  {128, 64, 176, 112, 140, 76, 188, 124,},
  {32, 224, 16, 208, 44, 236, 28, 220,},
  {160, 96, 144, 80, 172, 108, 156, 92,},
  {8, 200, 56, 248, 4, 196, 52, 244,},
  {136, 72, 184, 120, 132, 68, 180, 116,},
  {40, 232, 24, 216, 36, 228, 20, 212,},
  {168, 104, 152, 88, 164, 100, 148, 84},
};
struct ditherdata
{
  unsigned char table[32][32][32];
  int rmat[MSIZE][MSIZE];
  int gmat[MSIZE][MSIZE];
  int bmat[MSIZE][MSIZE];
  struct palette *palette;
  int active;
};
struct fixeddata
{
  /*6kKb of table */
  unsigned char ctable[8][8][256];
  /*and 32kB */
  unsigned char table[32][32][32];
  /*and 768 bytes... */
  int rmat[MSIZE][MSIZE];
  int gmat[MSIZE][MSIZE];
  int bmat[MSIZE][MSIZE];
  struct palette *palette;
  int forversion;
  int active;
  int fixcolor;
};
struct bitmapdata
{
  struct palette *palette;
  int intensity[256];
  int forversion;
  int active;
  int fixcolor;
};
static int
requirement (struct filter *f, struct requirements *r)
{
  f->req = *r;
  r->nimages = 1;
  r->flags &= ~IMAGEDATA;
  r->supportedmask =
    FIXEDCOLOR | MASK1BPP | MASK3BPP | MASK2BPP | MASK4BPP | BITMAPS;
  return (f->next->action->requirement (f->next, r));
}

#ifdef SFIXEDCOLOR
/* create_rgb_table:
 *  Fills an lookup table with conversion data for the specified
 *  palette. 
 *
 *  Uses alg. similiar to foodfill - it adds one seed per every color in 
 *  palette to its best possition. Then areas around seed are filled by 
 *  same color because it is best aproximation for them, and then areas 
 *  about them etc...
 *
 *  It does just about 80000 tests for distances and this is about 100
 *  times better than normal 256*32000 tests so the caluclation time
 *  is now less than one second at all computers I tested.
 */
#define UNUSED 65535
#define LAST 65532

/* macro add adds to single linked list */
#define add(i)    (next[(i)] == UNUSED ? (next[(i)] = LAST, \
    (first != LAST ? (next[last] = (i)) : (first = (i))), \
    (last = (i))) : 0)

  /* same but w/o checking for first element */
#define add1(i)   (next[(i)] == UNUSED ? (next[(i)] = LAST, \
    next[last] = (i), \
    (last = (i))) : 0)

  /* calculates distance between two colors */
#define dist(a1, a2, a3, b1, b2, b3) \
  (col_diff[0][((a2) - (b2)) & 0x1FF] + \
   col_diff[1][((a1) - (b1)) & 0x1FF] + \
   col_diff[2][((a3) - (b3)) & 0x1FF])

  /* converts r,g,b to position in array and back */
#define pos(r, g, b) \
((((int)(r)) / 8) * 32 * 32 + (((int)(g)) / 8) * 32 + (((int)(b)) / 8))

#define depos(pal, r, g, b) \
  ((b) = ((unsigned int)(pal) & 31) * 8, \
   (g) = (((unsigned int)(pal) >> 5) & 31) * 8, \
   (r) = (((unsigned int)(pal) >> 10) & 31) * 8)

  /* is current color better than pal1? */
#define better(r1, g1, b1, pal1) \
  (((int)dist((r1), (g1), (b1), \
	      (int)(pal1)[0], (int)(pal1)[1], (int)(pal1)[2])) > (int)dist2)
#define better1(r1, g1, b1, pal1) \
  (((int)dist((r1), (g1), (b1), \
	      (int)(pal1)[0], (int)(pal1)[1], (int)(pal1)[2])))

  /* checking of possition */
#define dopos(rp, gp, bp, ts) \
  if ((rp > -1 || r > 0) && (rp < 1 || r < 248) && \
      (gp > -1 || g > 0) && (gp < 1 || g < 248) && \
      (bp > -1 || b > 0) && (bp < 1 || b < 248)) { \
    i = first + rp * 32 * 32 + gp * 32 + bp; \
      if (ts ? data[i] != val : !data[i]) { \
	dist2 = (rp ? col_diff[1][(r+8*rp-(int)pal[val][0]) & 0x1FF] : r2) + \
	  (gp ? col_diff[0][(g+8*gp-(int)pal[val][1]) & 0x1FF] : g2) + \
	  (bp ? col_diff[2][(b+8*bp-(int)pal[val][2]) & 0x1FF] : b2); \
	  if (better((r+8*rp), (g+8*gp), (b+8*bp), pal[data[i]])) { \
	    data[i] = val; \
	      add1 (i); \
	  } \
      } \
  }

static void
create_rgb_table (unsigned char table[32][32][32], struct palette *palette)
{
  int i, curr, r, g, b, val, r2, g2, b2, dist2;
  rgb_t *pal = palette->rgb;
  unsigned short *next;
  unsigned char *data;
  int first = LAST;
  int last = LAST;
  int count = 0;

  next = (unsigned short *) malloc (sizeof (short) * 32 * 32 * 32);
  if (col_diff[0][1] == 0)
    bestfit_init ();

  memset_long (next, 255, sizeof (short) * 32 * 32 * 32);
  memset_long (table, palette->start, sizeof (char) * 32 * 32 * 32);
  depos (32 * 32 * 32 - 1, r, g, b);

  data = (unsigned char *) table;

  /* add starting seeds for foodfill */
  for (i = palette->start + 1; i < palette->end; i++)
    {
      curr = pos ((int) pal[i][0], (int) pal[i][1], (int) pal[i][2]);
      if (next[curr] == UNUSED)
	{
	  data[curr] = (unsigned int) i;
	  add (curr);
	}
    }

  /* main foodfill: two versions of loop for faster growing in blue axis */
  while (first != LAST)
    {
      depos (first, r, g, b);

      /* calculate distance of current color */
      val = data[first];
      r2 = col_diff[1][(((int) pal[val][0]) - (r)) & 0x1FF];
      g2 = col_diff[0][(((int) pal[val][1]) - (g)) & 0x1FF];
      b2 = col_diff[2][(((int) pal[val][2]) - (b)) & 0x1FF];

      /* try to grow to all directions */
      dopos (0, 0, 1, 1);
      dopos (0, 0, -1, 1);
      dopos (1, 0, 0, 1);
      dopos (-1, 0, 0, 1);
      dopos (0, 1, 0, 1);
      dopos (0, -1, 0, 1);

      /* faster growing of blue direction */
      if ((b > 0) && (data[first - 1] == val))
	{
	  b -= 8;
	  first--;
	  b2 = col_diff[2][(((int) pal[val][2]) - (b)) & 0x1ff];

	  dopos (-1, 0, 0, 0);
	  dopos (1, 0, 0, 0);
	  dopos (0, -1, 0, 0);
	  dopos (0, 1, 0, 0);

	  first++;
	}

      /* get next from list */
      i = first;
      first = next[first];
      next[i] = UNUSED;

      /* second version of loop */
      if (first != LAST)
	{
	  depos (first, r, g, b);

	  val = data[first];
	  r2 = col_diff[1][(((int) pal[val][0]) - (r)) & 0x1ff];
	  g2 = col_diff[0][(((int) pal[val][1]) - (g)) & 0x1ff];
	  b2 = col_diff[2][(((int) pal[val][2]) - (b)) & 0x1ff];

	  dopos (0, 0, 1, 1);
	  dopos (0, 0, -1, 1);
	  dopos (1, 0, 0, 1);
	  dopos (-1, 0, 0, 1);
	  dopos (0, 1, 0, 1);
	  dopos (0, -1, 0, 1);

	  if ((b < 248) && (data[first + 1] == val))
	    {
	      b += 8;
	      first++;
	      b2 = col_diff[2][(((int) pal[val][2]) - (b)) & 0x1ff];

	      dopos (-1, 0, 0, 0);
	      dopos (1, 0, 0, 0);
	      dopos (0, -1, 0, 0);
	      dopos (0, 1, 0, 0);

	      first--;
	    }

	  i = first;
	  first = next[first];
	  next[i] = UNUSED;
	}

      count++;
    }
  free (next);
}
static void
checksizes (unsigned char table[32][32][32], int *RESTRICT red,
	    int *RESTRICT green, int *RESTRICT blue)
{
  int r, g, b;
  int color;
  int n, maxn;
  maxn = 0;
  n = 0;
  for (r = 0; r < 32; r++)
    for (g = 0; g < 32; g++)
      {
	color = 512;
	for (b = 0; b < 32; b++)
	  if (color != table[r][g][b])
	    {
	      if (maxn < n)
		maxn = n;
	      color = table[r][g][b];
	      n = 0;
	    }
	  else
	    n++;
      }
  *blue = (maxn) * 8;
  maxn = 0;
  n = 0;
  for (b = 0; b < 32; b++)
    for (g = 0; g < 32; g++)
      {
	color = 512;
	for (r = 0; r < 32; r++)
	  if (color != table[r][g][b])
	    {
	      if (maxn < n)
		maxn = n;
	      color = table[r][g][b];
	      n = 0;
	    }
	  else
	    n++;
      }
  *red = (maxn) * 8;
  maxn = 0;
  n = 0;
  for (b = 0; b < 32; b++)
    for (r = 0; r < 32; r++)
      {
	color = 512;
	for (g = 0; g < 32; g++)
	  if (color != table[r][g][b])
	    {
	      if (maxn < n)
		maxn = n;
	      color = table[r][g][b];
	      n = 0;
	    }
	  else
	    n++;
      }
  *green = (maxn) * 8;
}
#endif /*FIXEDCOLOR */
static int
initialize (struct filter *f, struct initdata *i)
{
  struct ditherdata *s = (struct ditherdata *) f->data;
  struct palette *palette;
  int r, g, b;
  inhermisc (f, i);
  if (i->image->bytesperpixel <= 1)
    {
      int red, green, blue;
      if (!inherimage
	  (f, i, TOUCHIMAGE /*| IMAGEDATA */ , 0, 0, s->palette, 0, 0))
	return 0;
      if (!s->active)
	{
	  palette = clonepalette (f->image->palette);
	  restorepalette (s->palette, palette);
	  destroypalette (palette);
	}
      switch (f->image->palette->type)
	{
	case C256:
	  blue = mkrgb (f->image->palette);
	  red = blue / 256 / 256;
	  green = (blue / 256) & 255;
	  blue &= 255;
	  for (r = 0; r < 32; r++)
	    for (g = 0; g < 32; g++)
	      for (b = 0; b < 32; b++)
		{
		  s->table[r][g][b] =
		    f->image->palette->pixels[((r * red + red / 2) / 32) +
					      ((g * green +
						green / 2) / 32) * blue *
					      red +
					      ((b * blue +
						blue / 2) / 32) * red];
		}
	  for (r = 0; r < MSIZE; r++)
	    for (g = 0; g < MSIZE; g++)
	      {
		s->rmat[r][g] = ((int) matrix[r][g] - 128) * 256 / red / 256;
		s->gmat[r][g] =
		  ((int) matrix[(r + 3) % MSIZE][(g + 6) % MSIZE] -
		   128) * 256 / green / 256;
		s->bmat[r][g] =
		  ((int) matrix[(r + 6) % MSIZE][(g + 3) % MSIZE] -
		   128) * 256 / blue / 256;
	      }
	  break;
#ifdef SFIXEDCOLOR
	case FIXEDCOLOR:
	  create_rgb_table (s->table, f->image->palette);
	  checksizes (s->table, &red, &green, &blue);
	  for (r = 0; r < MSIZE; r++)
	    for (g = 0; g < MSIZE; g++)
	      {
		s->rmat[r][g] = ((int) matrix[r][g] - 128) * red / 256;
		s->gmat[r][g] =
		  ((int) matrix[(r + 3) % MSIZE][(g + 6) % MSIZE] -
		   128) * green / 256;
		s->bmat[r][g] =
		  ((int) matrix[(r + 6) % MSIZE][(g + 3) % MSIZE] -
		   128) * blue / 256;
	      }
	  break;
#endif
#ifdef BITMAPS
	case MBITMAP:
	case LBITMAP:
	case MIBITMAP:
	case LIBITMAP:
	  break;
#endif
	case GRAYSCALE:
	  break;
	default:
	  x_fatalerror ("Unsupported image type. Recompile XaoS.");
	}
      setfractalpalette (f, s->palette);
      s->active = 1;
      f->queue->saveimage = f->childimage;
      return (f->previous->action->initialize (f->previous, i));
    }
  else
    {
      if (s->active)
	{
	  f->image = i->image;
	  palette = clonepalette (s->palette);
	  restorepalette (f->image->palette, palette);
	  destroypalette (palette);
	}
      s->active = 0;
      return (f->previous->action->initialize (f->previous, i));
    }
}
static struct filter *
getinstance (CONST struct filteraction *a)
{
  struct filter *f = createfilter (a);
  struct ditherdata *i = (struct ditherdata *) calloc (1, sizeof (*i));
  i->palette =
    createpalette (0, 65536, TRUECOLOR, 0, 65536, NULL, NULL, NULL, NULL,
		   NULL);
  f->data = i;
  f->name = "Truecolor to 8bpp convertor";
  return (f);
}
static void
convert (void *data, struct taskinfo *task, int r1, int r2)
{
  struct filter *RESTRICT f = (struct filter *) data;
  struct image *RESTRICT img1 = f->childimage;
  struct image *RESTRICT img2 = f->image;
  CONST struct ditherdata *RESTRICT s = (struct ditherdata *) f->data;
  CONST pixel32_t *RESTRICT src, *srcend;
  int r, g, b;
  pixel8_t *RESTRICT dest;
  int i;
  int x = 0;

  for (i = r1; i < r2; i++)
    {
      src = (pixel32_t *) img1->currlines[i];
      dest = img2->currlines[i];
      srcend = src + img1->width;
      x++;
      x = x & (MSIZE - 1);
      for (; src < srcend; src++, dest++)
	{
	  b = *src;
	  g = (b >> 8) & 0xff;
	  r = (b >> 16);
	  b &= 0xff;
	  r += s->rmat[x][(unsigned long) dest & (MSIZE - 1)];
	  g += s->gmat[x][(unsigned long) dest & (MSIZE - 1)];
	  b += s->bmat[x][(unsigned long) dest & (MSIZE - 1)];
	  if (r & (~255))
	    {
	      if (r < 0)
		r = 0;
	      else if (r > 255)
		r = 255;
	    }
	  if (g & (~255))
	    {
	      if (g < 0)
		g = 0;
	      else if (g > 255)
		g = 255;
	    }
	  if (b & (~255))
	    {
	      if (b < 0)
		b = 0;
	      else if (b > 255)
		b = 255;
	    }
	  *dest = s->table[r >> 3][g >> 3][b >> 3];
	}
    }
}

#define intenzity(x) ((int)(((x)&255) * 76 + (((x)>>8)&255) * 151 + (((x)>>16)&255) * 28)>>8)
static void
convertgray (void *data, struct taskinfo *task, int r1, int r2)
{
  struct filter *f = (struct filter *) data;
  struct image *img1 = f->childimage;
  struct image *img2 = f->image;
  pixel32_t *src, *srcend;
  pixel8_t *dest;
  int i;
  int x = 0;
  unsigned char table[256];
  for (i = 0; i < 256; i++)
    table[i] =
      i * (img2->palette->end - img2->palette->start) / 256 +
      img2->palette->start;

  for (i = r1; i < r2; i++)
    {
      src = (pixel32_t *) img1->currlines[i];
      dest = img2->currlines[i];
      srcend = src + img1->width;
      x++;
      x = x & (MSIZE - 1);
      for (; src < srcend; src++, dest++)
	{
	  *dest = table[intenzity (*src)];
	}
    }
}

#ifdef SBITMAPS
#define inten(x) ((int)(((x)&255) * 76 + (((x)>>8)&255) * 151 + (((x)>>16)&255) * 28)>>8)-256
static void
converttbitmap (void *data, struct taskinfo *task, int r1, int r2)
{
  struct filter *f = (struct filter *) data;
  struct image *img1 = f->childimage;
  struct image *img2 = f->image;
  pixel32_t *src, *srcend;
  pixel8_t *dest = NULL;
  int i, y;
  unsigned int mask = 0;
  int x = 0;
  unsigned int val = 0;
  src = (pixel32_t *) img1->currlines[0];
  for (i = r1; i < r2; i++)
    {
      dest = img2->currlines[i];
      x = i & (MSIZE - 1);
      src = (pixel32_t *) img1->currlines[i];
      srcend = src + (img1->width & ~7);
      switch (img2->palette->type)
	{
#ifdef SMBITMAPS
	case MBITMAP:
	  for (; src < srcend; dest++)
	    {
	      if (inten (src[0]) + (int) matrix[x][0] >= 0)
		val = 128;
	      else
		val = 0;
	      if (inten (src[1]) + (int) matrix[x][1] >= 0)
		val |= 64;
	      if (inten (src[2]) + (int) matrix[x][2] >= 0)
		val |= 32;
	      if (inten (src[3]) + (int) matrix[x][3] >= 0)
		val |= 16;
	      if (inten (src[4]) + (int) matrix[x][4] >= 0)
		val |= 8;
	      if (inten (src[5]) + (int) matrix[x][5] >= 0)
		val |= 4;
	      if (inten (src[6]) + (int) matrix[x][6] >= 0)
		val |= 2;
	      if (inten (src[7]) + (int) matrix[x][7] >= 0)
		val |= 1;
	      src += 8;
	      *dest = val;
	    }
	  srcend = (pixel32_t *) img1->currlines[i] + img1->width;
	  if (src != srcend)
	    {
	      y = 0;
	      for (val = 0, mask = 128; src < srcend; mask >>= 1, src++, y++)
		{
		  if (inten (*src) + (int) matrix[x][y] >= 0)
		    val |= mask;
		}
	      if (!mask)
		*dest = val, dest++;
	    }
	  break;
	case MIBITMAP:
	  for (; src < srcend; dest++)
	    {
	      if (inten (src[0]) + (int) matrix[x][0] <= 0)
		val = 128;
	      else
		val = 0;
	      if (inten (src[1]) + (int) matrix[x][1] <= 0)
		val |= 64;
	      if (inten (src[2]) + (int) matrix[x][2] <= 0)
		val |= 32;
	      if (inten (src[3]) + (int) matrix[x][3] <= 0)
		val |= 16;
	      if (inten (src[4]) + (int) matrix[x][4] <= 0)
		val |= 8;
	      if (inten (src[5]) + (int) matrix[x][5] <= 0)
		val |= 4;
	      if (inten (src[6]) + (int) matrix[x][6] <= 0)
		val |= 2;
	      if (inten (src[7]) + (int) matrix[x][7] <= 0)
		val |= 1;
	      src += 8;
	      *dest = val;
	    }
	  srcend = (pixel32_t *) img1->currlines[i] + img1->width;
	  if (src != srcend)
	    {
	      y = 0;
	      for (val = 0, mask = 128; src < srcend; mask >>= 1, src++, y++)
		{
		  if (inten (*src) + (int) matrix[x][y] <= 0)
		    val |= mask;
		}
	      if (!mask)
		*dest = val, dest++;
	    }
	  break;
#endif
#ifdef SLBITMAPS
	case LBITMAP:
	  for (; src < srcend; dest++)
	    {
	      if (inten (src[0]) + (int) matrix[x][0] >= 0)
		val = 1;
	      else
		val = 0;
	      if (inten (src[1]) + (int) matrix[x][1] >= 0)
		val |= 2;
	      if (inten (src[2]) + (int) matrix[x][2] >= 0)
		val |= 4;
	      if (inten (src[3]) + (int) matrix[x][3] >= 0)
		val |= 8;
	      if (inten (src[4]) + (int) matrix[x][4] >= 0)
		val |= 16;
	      if (inten (src[5]) + (int) matrix[x][5] >= 0)
		val |= 32;
	      if (inten (src[6]) + (int) matrix[x][6] >= 0)
		val |= 64;
	      if (inten (src[7]) + (int) matrix[x][7] >= 0)
		val |= 128;
	      src += 8;
	      *dest = val;
	    }
	  srcend = (pixel32_t *) img1->currlines[i] + img1->width;
	  if (src != srcend)
	    {
	      y = 0;
	      for (val = 0, mask = 1; src < srcend; mask <<= 1, src++, y++)
		{
		  if (inten (*src) + (int) matrix[x][y] >= 0)
		    val |= mask;
		}
	      if (!mask)
		*dest = val, dest++;
	    }
	  break;
	case LIBITMAP:
	  for (; src < srcend; dest++)
	    {
	      if (inten (src[0]) + (int) matrix[x][0] <= 0)
		val = 1;
	      else
		val = 0;
	      if (inten (src[1]) + (int) matrix[x][1] <= 0)
		val |= 2;
	      if (inten (src[2]) + (int) matrix[x][2] <= 0)
		val |= 4;
	      if (inten (src[3]) + (int) matrix[x][3] <= 0)
		val |= 8;
	      if (inten (src[4]) + (int) matrix[x][4] <= 0)
		val |= 16;
	      if (inten (src[5]) + (int) matrix[x][5] <= 0)
		val |= 32;
	      if (inten (src[6]) + (int) matrix[x][6] <= 0)
		val |= 64;
	      if (inten (src[7]) + (int) matrix[x][7] <= 0)
		val |= 128;
	      src += 8;
	      *dest = val;
	    }
	  srcend = (pixel32_t *) img1->currlines[i] + img1->width;
	  if (src != srcend)
	    {
	      y = 0;
	      for (val = 0, mask = 1; src < srcend; mask <<= 1, src++, y++)
		{
		  if (inten (*src) + (int) matrix[x][y] <= 0)
		    val |= mask;
		}
	      if (!mask)
		*dest = val, dest++;
	    }
	  break;
#endif
	}
    }
  *dest = val;
}
#endif
static void
destroyinstance (struct filter *f)
{
  struct ditherdata *i = (struct ditherdata *) f->data;
  destroypalette (i->palette);
  free (f->data);
  destroyinheredimage (f);
  free (f);
}
static int
doit (struct filter *f, int flags, int time)
{
  int val;
  struct ditherdata *s = (struct ditherdata *) f->data;
  if (s->active)
    updateinheredimage (f);
  val = f->previous->action->doit (f->previous, flags, time);
  if (s->active)
    {
#ifdef SBITMAPS
      if (f->image->palette->type & BITMAPS)
	{
	  xth_function (converttbitmap, f, f->image->height);
	}
      else
#endif
	{
	  if (f->image->palette->type == GRAYSCALE)
	    xth_function (convertgray, f, f->image->height);
	  else
	    xth_function (convert, f, f->image->height);
	}
      xth_sync ();
    }
  return val;
}

static void
myremovefilter (struct filter *f)
{
  struct ditherdata *s = (struct ditherdata *) f->data;
  struct palette *palette;
  if (s->active)
    {
      palette = clonepalette (s->palette);
      restorepalette (f->image->palette, palette);
      destroypalette (palette);
    }
}


CONST struct filteraction truecolor_filter = {
  "Truecolor emulator",
  "truecolor",
  0,
  getinstance,
  destroyinstance,
  doit,
  requirement,
  initialize,
  convertupgeneric,
  convertdowngeneric,
  myremovefilter,
};



#ifdef SCONVERTORS
static int
myfixedalloccolor (struct palette *p, int init, int r, int g, int b)
{
  struct palette *palette = (struct palette *) p->data;
  fixedalloccolor (p, init, r, g, b);
  return (palette->alloccolor (palette, init, r, g, b));
}
static void
myallocfinished (struct palette *p)
{
  struct palette *palette = (struct palette *) p->data;
  palette->allocfinished (palette);
}
static void
mysetcolor (struct palette *p, int start, int end, rgb_t * rgb)
{
  p->data = &p;
}
#endif
#ifdef SFIXEDCOLOR

static int
initializefixed (struct filter *f, struct initdata *i)
{
  struct fixeddata *s = (struct fixeddata *) f->data;
  struct palette *palette;
  int r, g;
  inhermisc (f, i);
  if (i->image->palette->type == FIXEDCOLOR
      && !(f->req.supportedmask & FIXEDCOLOR))
    {
      int red, green, blue;
      i->image->palette->alloccolor = myfixedalloccolor;
      i->image->palette->allocfinished = myallocfinished;
      i->image->palette->data = s->palette;
      if (!inherimage (f, i, TOUCHIMAGE | IMAGEDATA, 0, 0, s->palette, 0, 0))
	return 0;
      if (s->active == -1)
	{
	  palette = clonepalette (f->image->palette);
	  restorepalette (s->palette, palette);
	  destroypalette (palette);
	}
      create_rgb_table (s->table, f->image->palette);
      checksizes (s->table, &red, &green, &blue);
      for (r = 0; r < MSIZE; r++)
	for (g = 0; g < MSIZE; g++)
	  {
	    s->rmat[r][g] = ((int) matrix[r][g] - 128) * red / 256;
	    s->gmat[r][g] =
	      ((int) matrix[(r + 3) % MSIZE][(g + 6) % MSIZE] -
	       128) * green / 256;
	    s->bmat[r][g] =
	      ((int) matrix[(r + 6) % MSIZE][(g + 3) % MSIZE] -
	       128) * blue / 256;
	  }
      s->palette->data = &s->palette;
      setfractalpalette (f, s->palette);
      s->active = 1;
      f->queue->saveimage = f->childimage;
      f->queue->palettechg = f;
      return (f->previous->action->initialize (f->previous, i));
    }
  else
    {
      if (s->active == 1)
	{
	  s->fixcolor = 1;
	}
      s->active = 0;
      return (f->previous->action->initialize (f->previous, i));
    }
}
static struct filter *
getinstancefixed (CONST struct filteraction *a)
{
  struct filter *f = createfilter (a);
  struct fixeddata *i = (struct fixeddata *) calloc (1, sizeof (*i));
  i->palette =
    createpalette (0, 256, C256, 0, 256, NULL, mysetcolor, NULL, NULL, NULL);
  i->active = -1;
  f->data = i;
  f->name = "Palete emulator";
  return (f);
}
static void
convertfixed (void *data, struct taskinfo *task, int r1, int r2)
{
  struct filter *f = (struct filter *) data;
  struct image *img1 = f->childimage;
  struct image *img2 = f->image;
  struct fixeddata *s = (struct fixeddata *) f->data;
  pixel8_t *src, *srcend;
  pixel8_t *dest;
  int i;
  int x = 0;
  for (i = r1; i < r2; i++)
    {
      src = (pixel8_t *) img1->currlines[i];
      dest = img2->currlines[i];
      srcend = src + img1->width;
      x++;
      x = x & (MSIZE - 1);
      for (; src < srcend; src++, dest++)
	{
	  *dest = s->ctable[x][((unsigned long) dest) & (MSIZE - 1)][*src];
	}
    }
}
static int
doitfixed (struct filter *f, int flags, int time)
{
  int val;
  struct fixeddata *s = (struct fixeddata *) f->data;
  if (s->fixcolor && !s->active)
    {
      struct palette *palette;
      s->fixcolor = 0;
      palette = clonepalette (s->palette);
      restorepalette (f->previous->childimage->palette, palette);
      destroypalette (palette);
    }
  if (s->active)
    updateinheredimage (f);
  if (!(flags & PALETTEONLY))
    val = f->previous->action->doit (f->previous, flags, time);
  else
    val = 0;
  if (s->active)
    {
      if (s->palette->data != NULL)
	{
	  int i, x, y;
	  s->palette->data = NULL;
	  val |= CHANGED;
	  for (i = 0; i < 256; i++)
	    {
	      for (x = 0; x < 8; x++)
		for (y = 0; y < 8; y++)
		  {
		    int r, g, b;
		    r = s->palette->rgb[i][0] + s->rmat[x][y];
		    if (r & (~255))
		      {
			if (r < 0)
			  r = 0;
			else if (r > 255)
			  r = 255;
		      }
		    g = s->palette->rgb[i][1] + s->gmat[x][y];
		    if (g & (~255))
		      {
			if (g < 0)
			  g = 0;
			else if (g > 255)
			  g = 255;
		      }
		    b = s->palette->rgb[i][2] + s->bmat[x][y];
		    if (b & (~255))
		      {
			if (b < 0)
			  b = 0;
			else if (b > 255)
			  b = 255;
		      }
		    s->ctable[x][y][i] = s->table[r >> 3][g >> 3][b >> 3];
		  }
	    }
	}
      xth_function (convertfixed, f, f->image->height);
      xth_sync ();
    }
  return val;
}
static void
myremovefilterfixed (struct filter *f)
{
  struct fixeddata *s = (struct fixeddata *) f->data;
  struct palette *palette;
  if (s->active)
    {
      palette = clonepalette (s->palette);
      restorepalette (f->image->palette, palette);
      destroypalette (palette);
    }
}
static void
destroyinstancefixed (struct filter *f)
{
  struct fixeddata *i = (struct fixeddata *) f->data;
  destroypalette (i->palette);
  free (f->data);
  destroyinheredimage (f);
  free (f);
}
CONST struct filteraction fixedcolor_filter = {
  "Palette emulator",
  "fixedcolor",
  0,
  getinstancefixed,
  destroyinstancefixed,
  doitfixed,
  requirement,
  initializefixed,
  convertupgeneric,
  convertdowngeneric,
  myremovefilterfixed,
};
#endif

#ifdef SBITMAPS
static int
initializebitmap (struct filter *f, struct initdata *i)
{
  struct bitmapdata *s = (struct bitmapdata *) f->data;
  struct palette *palette;
  inhermisc (f, i);
  if ((i->image->palette->type & BITMAPS)
      && !(f->req.supportedmask & BITMAPS))
    {
      i->image->palette->alloccolor = myfixedalloccolor;
      i->image->palette->allocfinished = myallocfinished;
      i->image->palette->data = s->palette;
      if (!inherimage (f, i, TOUCHIMAGE | IMAGEDATA, 0, 0, s->palette, 0, 0))
	return 0;
      if (s->active == -1)
	{
	  palette = clonepalette (f->image->palette);
	  restorepalette (s->palette, palette);
	  destroypalette (palette);
	}
      s->palette->data = &s->palette;
      setfractalpalette (f, s->palette);
      s->active = 1;
      f->queue->saveimage = f->childimage;
      f->queue->palettechg = f;
      return (f->previous->action->initialize (f->previous, i));
    }
  else
    {
      if (s->active == 1)
	{
	  s->fixcolor = 1;
	}
      s->active = 0;
      return (f->previous->action->initialize (f->previous, i));
    }
}
static struct filter *
getinstancebitmap (CONST struct filteraction *a)
{
  struct filter *f = createfilter (a);
  struct bitmapdata *i = (struct bitmapdata *) calloc (1, sizeof (*i));
  i->palette =
    createpalette (0, 256, C256, 0, 256, NULL, mysetcolor, NULL, NULL, NULL);
  i->active = -1;
  f->data = i;
  f->name = "Palete emulator";
  return (f);
}

#define INTENSITY(r,g,b) (r * 30U + g * 59U + b * 11U)/100U-256U
static void
convertbitmap (void *data, struct taskinfo *task, int r1, int r2)
{
  struct filter *f = (struct filter *) data;
  struct image *img1 = f->childimage;
  struct image *img2 = f->image;
  struct bitmapdata *s = (struct bitmapdata *) f->data;
  pixel8_t *src, *srcend;
  pixel8_t *dest = NULL;
  int *intensity = s->intensity;
  int i;
  unsigned int mask = 0;
  int x = 0, y;
  unsigned int val = 0;
  src = (pixel8_t *) img1->currlines[0];
  for (i = r1; i < r2; i++)
    {
      dest = img2->currlines[i];
      x = i & (MSIZE - 1);
      src = (pixel8_t *) img1->currlines[i];
      srcend = src + (img1->width & ~7);
      switch (img2->palette->type)
	{
#ifdef SMBITMAPS
	case MBITMAP:
	  for (; src < srcend; dest++)
	    {
	      if (intensity[src[0]] + (int) matrix[x][0] >= 0)
		val = 128;
	      else
		val = 0;
	      if (intensity[src[1]] + (int) matrix[x][1] >= 0)
		val |= 64;
	      if (intensity[src[2]] + (int) matrix[x][2] >= 0)
		val |= 32;
	      if (intensity[src[3]] + (int) matrix[x][3] >= 0)
		val |= 16;
	      if (intensity[src[4]] + (int) matrix[x][4] >= 0)
		val |= 8;
	      if (intensity[src[5]] + (int) matrix[x][5] >= 0)
		val |= 4;
	      if (intensity[src[6]] + (int) matrix[x][6] >= 0)
		val |= 2;
	      if (intensity[src[7]] + (int) matrix[x][7] >= 0)
		val |= 1;
	      src += 8;
	      *dest = val;
	    }
	  srcend = (pixel8_t *) img1->currlines[i] + img1->width;
	  if (src != srcend)
	    {
	      y = 0;
	      for (val = 0, mask = 128; src < srcend; mask >>= 1, src++, y++)
		{
		  if (intensity[*src] + (int) matrix[x][y] >= 0)
		    val |= mask;
		}
	      *dest = val, dest++;
	    }
	  break;
	case MIBITMAP:
	  for (; src < srcend; dest++)
	    {
	      if (intensity[src[0]] + (int) matrix[x][0] <= 0)
		val = 128;
	      else
		val = 0;
	      if (intensity[src[1]] + (int) matrix[x][1] <= 0)
		val |= 64;
	      if (intensity[src[2]] + (int) matrix[x][2] <= 0)
		val |= 32;
	      if (intensity[src[3]] + (int) matrix[x][3] <= 0)
		val |= 16;
	      if (intensity[src[4]] + (int) matrix[x][4] <= 0)
		val |= 8;
	      if (intensity[src[5]] + (int) matrix[x][5] <= 0)
		val |= 4;
	      if (intensity[src[6]] + (int) matrix[x][6] <= 0)
		val |= 2;
	      if (intensity[src[7]] + (int) matrix[x][7] <= 0)
		val |= 1;
	      src += 8;
	      *dest = val;
	    }
	  srcend = (pixel8_t *) img1->currlines[i] + img1->width;
	  if (src != srcend)
	    {
	      y = 0;
	      for (val = 0, mask = 128; src < srcend; mask >>= 1, src++, y++)
		{
		  if (intensity[*src] + (int) matrix[x][y] <= 0)
		    val |= mask;
		}
	      *dest = val, dest++;
	    }
	  break;
#endif
#ifdef SLBITMAPS
	case LBITMAP:
	  for (; src < srcend; dest++)
	    {
	      if (intensity[src[0]] + (int) matrix[x][0] >= 0)
		val = 1;
	      else
		val = 0;
	      if (intensity[src[1]] + (int) matrix[x][1] >= 0)
		val |= 2;
	      if (intensity[src[2]] + (int) matrix[x][2] >= 0)
		val |= 4;
	      if (intensity[src[3]] + (int) matrix[x][3] >= 0)
		val |= 8;
	      if (intensity[src[4]] + (int) matrix[x][4] >= 0)
		val |= 16;
	      if (intensity[src[5]] + (int) matrix[x][5] >= 0)
		val |= 32;
	      if (intensity[src[6]] + (int) matrix[x][6] >= 0)
		val |= 64;
	      if (intensity[src[7]] + (int) matrix[x][7] >= 0)
		val |= 128;
	      src += 8;
	      *dest = val;
	    }
	  srcend = (pixel8_t *) img1->currlines[i] + img1->width;
	  if (src != srcend)
	    {
	      y = 0;
	      for (val = 0, mask = 1; src < srcend; mask <<= 1, src++, y++)
		{
		  if (intensity[*src] + (int) matrix[x][y] >= 0)
		    val |= mask;
		}
	      *dest = val, dest++;
	    }
	case LIBITMAP:
	  for (; src < srcend; dest++)
	    {
	      if (intensity[src[0]] + (int) matrix[x][0] <= 0)
		val = 1;
	      else
		val = 0;
	      if (intensity[src[1]] + (int) matrix[x][1] <= 0)
		val |= 2;
	      if (intensity[src[2]] + (int) matrix[x][2] <= 0)
		val |= 4;
	      if (intensity[src[3]] + (int) matrix[x][3] <= 0)
		val |= 8;
	      if (intensity[src[4]] + (int) matrix[x][4] <= 0)
		val |= 16;
	      if (intensity[src[5]] + (int) matrix[x][5] <= 0)
		val |= 32;
	      if (intensity[src[6]] + (int) matrix[x][6] <= 0)
		val |= 64;
	      if (intensity[src[7]] + (int) matrix[x][7] <= 0)
		val |= 128;
	      src += 8;
	      *dest = val;
	    }
	  srcend = (pixel8_t *) img1->currlines[i] + img1->width;
	  if (src != srcend)
	    {
	      y = 0;
	      for (val = 0, mask = 1; src < srcend; mask <<= 1, src++, y++)
		{
		  if (intensity[*src] + (int) matrix[x][y] <= 0)
		    val |= mask;
		}
	      *dest = val, dest++;
	    }
	  break;
#endif
	}
    }
}
static int
doitbitmap (struct filter *f, int flags, int time)
{
  int val;
  struct bitmapdata *s = (struct bitmapdata *) f->data;
  if (s->fixcolor && !s->active)
    {
      struct palette *palette;
      s->fixcolor = 0;
      palette = clonepalette (s->palette);
      restorepalette (f->previous->childimage->palette, palette);
      destroypalette (palette);
    }
  if (s->active)
    updateinheredimage (f);
  if (!(flags & PALETTEONLY))
    val = f->previous->action->doit (f->previous, flags, time);
  else
    val = 0;
  if (s->active)
    {
      if (s->palette->data != NULL)
	{
	  int i;
	  for (i = 0; i < 256; i++)
	    s->intensity[i] = INTENSITY (f->childimage->palette->rgb[i][0],
					 f->childimage->palette->rgb[i][1],
					 f->childimage->palette->rgb[i][2]);
	  val |= CHANGED;
	}
      xth_function (convertbitmap, f, f->image->height);
      xth_sync ();
    }
  return val;
}
static void
myremovefilterbitmap (struct filter *f)
{
  struct bitmapdata *s = (struct bitmapdata *) f->data;
  struct palette *palette;
  if (s->active)
    {
      palette = clonepalette (s->palette);
      restorepalette (f->image->palette, palette);
      destroypalette (palette);
    }
}
static void
destroyinstancebitmap (struct filter *f)
{
  struct bitmapdata *i = (struct bitmapdata *) f->data;
  destroypalette (i->palette);
  free (f->data);
  destroyinheredimage (f);
  free (f);
}
CONST struct filteraction bitmap_filter = {
  "Palette emulator",
  "bitmap",
  0,
  getinstancebitmap,
  destroyinstancebitmap,
  doitbitmap,
  requirement,
  initializebitmap,
  convertupgeneric,
  convertdowngeneric,
  myremovefilterbitmap,
};
#endif
