#ifndef UNSUPPORTED
static inline void
drawchar (struct image *img, int x, int y, int fgcolor, unsigned char letter)
{
  int fontwidth = (RWIDTH (letter) + 7) / 8;
  CONST unsigned char *bitmap = &DATA[letter * HEIGHT * fontwidth];
  cpixel_t *current;
  int yend = y + HEIGHT;
  if (y < 0)
    bitmap -= y, y = 0;
  if (yend > img->height)
    yend = img->height;
  for (; y < yend; y++)
    {
      int b = *(bitmap++);
      int i = (1 << (RWIDTH (letter) - 1));
      if (fontwidth == 2)
	{
	  b <<= 8;
	  b |= *bitmap++;
	}
      current = (cpixel_t *) img->currlines[y];
      p_inc (current, x);
      while (i)
	{
	  if (i & b)
	    {
	      p_set (current, fgcolor);
	    }
	  i >>= 1;
	  p_inc (current, 1);
	}
    }
}
static inline void
hline (struct image *img, int x, int y, int length, int fgcolor)
{
  cpixel_t *current = (cpixel_t *) img->currlines[y], *end =
    (cpixel_t *) img->currlines[y];
  p_inc (current, x);
  p_inc (end, x + length);
#ifdef bpp1
  memset (current, fgcolor, end - current + 1);
#else
  while (current <= end)
    {
      p_set (current, fgcolor);
      p_inc (current, 1);
    }
#endif
}
static inline void
vline (struct image *img, int x, int y, int length, int fgcolor)
{
  length += y;
  while (y <= length)
    {
      cpixel_t *current = (cpixel_t *) img->currlines[y];
      p_inc (current, x);
      p_set (current, fgcolor);
      y++;
    }
}
static inline void
rectangle (struct image *img, int x, int y, int width, int height,
	   int fgcolor)
{
  height += y;
  while (y < height)
    hline (img, x, y, width - 1, fgcolor), y++;
}
static inline char *
savevline (struct image *img, int x, int y, int length)
{
  cpixel_t *saved = (cpixel_t *) malloc (length * bpp + bpp), *s = saved;
  length += y;
  while (y <= length)
    {
      cpixel_t *current = (cpixel_t *) img->currlines[y];
      p_copy (s, 0, current, x);
      p_inc (s, 1);
      y++;
    }
  return (char *) saved;
}
static inline void
restorevline (struct image *img, char *saved, int x, int y, int length)
{
  cpixel_t *s = (cpixel_t *) saved;
  length += y;
  while (y <= length)
    {
      cpixel_t *current = (cpixel_t *) img->currlines[y];
      p_copy (current, x, s, 0);
      p_inc (s, 1);
      y++;
    }
}

static inline char *
saveline (struct image *img, int x, int y, int x2, int y2)
{
  int dx = x2 - x;
  int dy = y2 - y;
  int ady = abs (dy);
  if (dx < ady)
    {
      cpixel_t *saved = (cpixel_t *) malloc ((ady + 1) * bpp * 2), *s = saved;
      int plus = (dx << 16) / ady;
      if (dy < 0)
	{
	  int dy = (x << 16) /*| (65536 / 2) */ ;
	  ady = y;
	  while (ady >= y2)
	    {
	      cpixel_t *current = (cpixel_t *) img->currlines[ady];
	      p_inc (current, (dy >> 16));
	      p_copy (s, 0, current, 0);
	      p_copy (s, 1, current, 1);
	      p_inc (s, 2);
	      dy += plus;
	      ady--;
	    }
	}
      else
	{
	  int dy = (x << 16) /*| (65536 / 2) */ ;
	  ady = y;
	  while (ady <= y2)
	    {
	      cpixel_t *current = (cpixel_t *) img->currlines[ady];
	      p_inc (current, (dy >> 16));
	      p_copy (s, 0, current, 0);
	      p_copy (s, 1, current, 1);
	      p_inc (s, 2);
	      dy += plus;
	      ady++;
	    }
	}
      return ((char *) saved);
    }
  else
    {
      cpixel_t *saved = (cpixel_t *) malloc ((dx + 1) * bpp * 2), *s = saved;
      int plus = (dy << 16) / dx;
      ady = x;
      dy = (y << 16);
      while (ady <= x2)
	{
	  cpixel_t *current = (cpixel_t *) img->currlines[dy >> 16];
	  p_copy (s, 0, current, ady);
	  current = (cpixel_t *) img->currlines[(dy >> 16) + 1];
	  p_copy (s, 1, current, ady);
	  p_inc (s, 2);
	  dy += plus;
	  ady++;
	}
      return ((char *) saved);
    }
}
static inline void
restoreline (struct image *img, char *saved, int x, int y, int x2, int y2)
{
  int dx = x2 - x;
  int dy = y2 - y;
  int ady = abs (dy);
  if (dx < ady)
    {
      cpixel_t *s = (cpixel_t *) saved;
      int plus = (dx << 16) / ady;
      if (dy < 0)
	{
	  int dy = (x << 16) /*| (65536 / 2) */ ;
	  ady = y;
	  while (ady >= y2)
	    {
	      cpixel_t *current = (cpixel_t *) img->currlines[ady];
	      p_inc (current, (dy >> 16));
	      p_copy (current, 0, s, 0);
	      p_copy (current, 1, s, 1);
	      p_inc (s, 2);
	      dy += plus;
	      ady--;
	    }
	}
      else
	{
	  int dy = (x << 16) /*| (65536 / 2) */ ;
	  ady = y;
	  while (ady <= y2)
	    {
	      cpixel_t *current = (cpixel_t *) img->currlines[ady];
	      p_inc (current, (dy >> 16));
	      p_copy (current, 0, s, 0);
	      p_copy (current, 1, s, 1);
	      p_inc (s, 2);
	      dy += plus;
	      ady++;
	    }
	}
    }
  else
    {
      cpixel_t *s = (cpixel_t *) saved;
      int plus = (dy << 16) / dx;
      ady = x;
      dy = (y << 16);
      while (ady <= x2)
	{
	  cpixel_t *current = (cpixel_t *) img->currlines[dy >> 16];
	  p_copy (current, ady, s, 0);
	  current = (cpixel_t *) img->currlines[(dy >> 16) + 1];
	  p_copy (current, ady, s, 1);
	  p_inc (s, 2);
	  dy += plus;
	  ady++;
	}
    }
}

#ifdef bpp1
#define myinterpol(a,b,n) intergray(a,b,n)
#else
#define myinterpol(a,b,n) interpol(a,b,n,rmask,gmask,bmask)
#endif
static inline void
line (struct image *img, int x, int y, int x2, int y2, int color)
{
  int dx = x2 - x;
  int dy = y2 - y;
  int ady = abs (dy);
#ifndef bpp1
  int rmask = img->palette->info.truec.rmask;
  int gmask = img->palette->info.truec.gmask;
  int bmask = img->palette->info.truec.bmask;
#endif
#ifdef bpp1
  if (img->palette->type &= (C256 | FIXEDCOLOR))
    {
      if (dx < ady)
	{
	  int plus = (dx << 16) / ady;
	  if (dy < 0)
	    {
	      int dy = (x << 16) | (65536 / 2);
	      ady = y;
	      while (ady >= y2)
		{
		  cpixel_t *current = (cpixel_t *) img->currlines[ady];
		  p_inc (current, (dy >> 16));
		  p_set (current, color);
		  dy += plus;
		  ady--;
		}
	    }
	  else
	    {
	      int dy = (x << 16) | (65536 / 2);
	      ady = y;
	      while (ady <= y2)
		{
		  cpixel_t *current = (cpixel_t *) img->currlines[ady];
		  p_inc (current, (dy >> 16));
		  p_set (current, color);
		  dy += plus;
		  ady++;
		}
	    }
	}
      else
	{
	  int plus = (dy << 16) / dx;
	  ady = x;
	  dy = (y << 16) | (65536 / 2);
	  while (ady <= x2)
	    {
	      cpixel_t *current = (cpixel_t *) img->currlines[dy >> 16];
	      p_setp (current, ady, color);
	      dy += plus;
	      ady++;
	    }
	}
      return;
    }
#endif

  if (dx < ady)
    {
      int plus = (dx << 16) / ady;
      if (dy < 0)
	{
	  int dy = (x << 16);
	  ady = y;
	  while (ady >= y2)
	    {
	      cpixel_t *current = (cpixel_t *) img->currlines[ady];
	      p_inc (current, (dy >> 16));
	      p_set (current,
		     myinterpol (p_get (current), color,
				 ((dy & 65535) >> 8)));
	      p_setp (current, 1,
		      myinterpol (color, p_getp (current, 1),
				  ((dy & 65535) >> 8)));
	      dy += plus;
	      ady--;
	    }
	}
      else
	{
	  int dy = (x << 16);
	  ady = y;
	  while (ady <= y2)
	    {
	      cpixel_t *current = (cpixel_t *) img->currlines[ady];
	      p_inc (current, (dy >> 16));
	      p_set (current,
		     myinterpol (p_get (current), color,
				 ((dy & 65535) >> 8)));
	      p_setp (current, 1,
		      myinterpol (color, p_getp (current, 1),
				  ((dy & 65535) >> 8)));
	      dy += plus;
	      ady++;
	    }
	}
    }
  else
    {
      int plus = (dy << 16) / dx;
      ady = x;
      dy = (y << 16);
      while (ady <= x2)
	{
	  cpixel_t *current = (cpixel_t *) img->currlines[dy >> 16];
	  p_setp (current, ady,
		  myinterpol (p_getp (current, ady), color,
			      ((dy & 65535) >> 8)));
	  current = (cpixel_t *) img->currlines[(dy >> 16) + 1];
	  p_setp (current, ady,
		  myinterpol (color, p_getp (current, ady),
			      ((dy & 65535) >> 8)));
	  dy += plus;
	  ady++;
	}
    }
}

#undef myinterpol
#endif

#undef drawchar
#undef hline
#undef vline
#undef rectangle
#undef line
#undef restoreline
#undef saveline
#undef savevline
#undef restorevline
