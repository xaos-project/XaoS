#ifndef UNSUPPORTED
REGISTERS (3)
     static void
       tracecolor (int xstart, int ystart, int xend, int yend, register int
		   x, register int y)
{
  int dir = RIGHT, fill = 0;
  register unsigned char *calc;
  int peri = 0;
  cpixeldata_t c = (cpixeldata_t) calculatepixel (x, y, 0);
  cpixeldata_t w = (cpixeldata_t) 0;
  cpixeldata_t inset = (cpixeldata_t) cpalette.pixels[0];
  putpixel (x, y, c);
  calc = calculated + x + y * CALCWIDTH;
  *calc = (unsigned char) 1;
  while (x > xstart && getpixel (x - 1, y) == c)
    x--, calc--;
  *calc = (unsigned char) 2;
  if (c == inset)
    peri = 1;
  do
    {
      if (!fill && !*calc)
	{
	  *calc = (unsigned char) 1;
	  putpixel (x, y, c);
	}
      switch (dir)
	{
	case RIGHT:
	  if (y > ystart)
	    {
	      if (!*(calc - CALCWIDTH))
		{
		  w = (cpixeldata_t) calculatepixel (x, y - 1, peri);
		  putpixel (x, y - 1, w);
		  *(calc - CALCWIDTH) = (unsigned char) 1;
		}
	      else
		w = getpixel (x, y - 1);
	      if (w == c)
		{
		  dir = UP;
		  calc -= CALCWIDTH;
		  y--;
		  break;
		}
	    }

	  if (x < xend)
	    {
	      if (!*(calc + 1))
		{
		  w = (cpixeldata_t) calculatepixel (x + 1, y, peri);
		  putpixel (x + 1, y, w);
		  *(calc + 1) = (unsigned char) 1;
		}
	      else
		w = getpixel (x + 1, y);
	      if (w == c)
		{
		  calc++;
		  x++;
		  break;
		}
	    }

	  if (y < yend)
	    {
	      if (!*(calc + CALCWIDTH))
		{
		  w = (cpixeldata_t) calculatepixel (x, y + 1, peri);
		  putpixel (x, y + 1, w);
		  *(calc + CALCWIDTH) = (unsigned char) 1;
		}
	      else
		w = getpixel (x, y + 1);
	      if (w == c)
		{
		  dir = DOWN;
		  calc += CALCWIDTH;
		  y++;
		  break;
		}
	    }

	  if (*calc == (unsigned char) 2)
	    {
	      *calc = (unsigned char) 1;
	      return;
	    }

	  dir = LEFT;
	  x--;
	  calc--;
	  break;

	case LEFT:
	  if (y < yend)
	    {
	      if (!*(calc + CALCWIDTH))
		{
		  w = (cpixeldata_t) calculatepixel (x, y + 1, peri);
		  putpixel (x, y + 1, w);
		  *(calc + CALCWIDTH) = (unsigned char) 1;
		}
	      else
		w = getpixel (x, y + 1);
	      if (w == c)
		{
		  dir = DOWN;
		  calc += CALCWIDTH;
		  y++;
		  break;
		}
	    }

	  if (x > xstart)
	    {
	      if (!*(calc - 1))
		{
		  w = (cpixeldata_t) calculatepixel (x - 1, y, peri);
		  putpixel (x - 1, y, w);
		  *(calc - 1) = (unsigned char) 1;
		}
	      else
		w = getpixel (x - 1, y);
	      if (w == c)
		{
		  calc--;
		  x--;
		  break;
		}
	    }

	  if (y > ystart)
	    {
	      if (!*(calc - CALCWIDTH))
		{
		  w = (cpixeldata_t) calculatepixel (x, y - 1, peri);
		  putpixel (x, y - 1, w);
		  *(calc - CALCWIDTH) = (unsigned char) 1;
		}
	      else
		w = getpixel (x, y - 1);
	      if (w == c)
		{
		  dir = UP;
		  calc -= CALCWIDTH;
		  y--;
		  break;
		}
	    }


	  dir = RIGHT;
	  x++;
	  calc++;
	  break;

	case UP:
	  if (fill)
	    {
	      unsigned char *calc1;
	      cpixel_t *pixel1;
	      calc1 = calc + 1;
	      pixel1 = p_add ((cpixel_t *) cimage.currlines[y], x + 1);
	      while (pixel1 <= p_add ((cpixel_t *) cimage.currlines[y], xend))
		{
		  if (!*calc1)
		    *calc1 = (unsigned char) 1, p_set (pixel1, c);
		  else if (p_get (pixel1) != c)
		    break;
		  p_inc (pixel1, 1);
		  calc1++;
		}
	    }
	  if (x > xstart)
	    {
	      if (!*(calc - 1))
		{
		  w = (cpixeldata_t) calculatepixel (x - 1, y, peri);
		  putpixel (x - 1, y, w);
		  *(calc - 1) = (unsigned char) 1;
		}
	      w = getpixel (x - 1, y);
	      if (w == c)
		{
		  dir = LEFT;
		  calc--;
		  x--;
		  break;
		}
	    }

	  if (y > ystart)
	    {
	      if (!*(calc - CALCWIDTH))
		{
		  w = (cpixeldata_t) calculatepixel (x, y - 1, peri);
		  putpixel (x, y - 1, w);
		  *(calc - CALCWIDTH) = (unsigned char) 1;
		}
	      w = getpixel (x, y - 1);
	      if (w == c)
		{
		  calc -= CALCWIDTH;
		  y--;
		  break;
		}
	    }

	  if (x < xend)
	    {
	      if (!*(calc + 1))
		{
		  w = (cpixeldata_t) calculatepixel (x + 1, y, peri);
		  putpixel (x + 1, y, w);
		  *(calc + 1) = (unsigned char) 1;
		}
	      else
		w = getpixel (x + 1, y);
	      if (w == c)
		{
		  dir = RIGHT;
		  calc++;
		  x++;
		  break;
		}
	    }

	  dir = DOWN;
	  y++;
	  calc += CALCWIDTH;
	  break;
	case DOWN:
	  if (x < xend)
	    {
	      if (!*(calc + 1))
		{
		  w = (cpixeldata_t) calculatepixel (x + 1, y, peri);
		  putpixel (x + 1, y, w);
		  *(calc + 1) = (unsigned char) 1;
		}
	      else
		w = getpixel (x + 1, y);
	      if (w == c)
		{
		  dir = RIGHT;
		  calc++;
		  x++;
		  break;
		}
	    }

	  if (y < yend)
	    {
	      if (!*(calc + CALCWIDTH))
		{
		  w = (cpixeldata_t) calculatepixel (x, y + 1, peri);
		  putpixel (x, y + 1, w);
		  *(calc + CALCWIDTH) = (unsigned char) 1;
		}
	      else
		w = getpixel (x, y + 1);
	      if (w == c)
		{
		  dir = DOWN;
		  calc += CALCWIDTH;
		  y++;
		  break;
		}
	    }

	  if (x > xstart)
	    {
	      if (!*(calc - 1))
		{
		  w = (cpixeldata_t) calculatepixel (x - 1, y, peri);
		  putpixel (x - 1, y, w);
		  *(calc - 1) = (unsigned char) 1;
		}
	      else
		w = getpixel (x - 1, y);
	      if (w == c)
		{
		  dir = LEFT;
		  calc--;
		  x--;
		  break;
		}
	    }

	  dir = UP;
	  calc -= CALCWIDTH;
	  y--;
	  break;

	}
      if (*calc == (unsigned char) 2)
	{
	  if (fill)
	    {
	      *calc = (unsigned char) 1;
	      return;
	    }
	  fill = 1;
	  dir = RIGHT;
	}
    }
  while (1);
}

#ifndef SLOWCACHESYNC
#ifndef nthreads
#define ethreads 1
REGISTERS (3)
     static INLINE void
       tracepoint (int xp, int yp, int dir, unsigned int color, int xstart,
		   int xend, int ystart, int yend)
{
  unsigned char *calc;
  cpixeldata_t mycolor;
  int i, lookdir;
  unsigned int c;
  int x, y;
  int periodicity = (dir & 8) != 0;
  dir &= ~8;
  calc = calculated + xp + yp * CALCWIDTH;

  if (!(*calc & (CALCULATED | CALCULATING)))
    {
      *calc |= CALCULATING;
      mycolor = (cpixeldata_t) calculatepixel (xp, yp, periodicity);
      putpixel (xp, yp, mycolor);
      *calc |= CALCULATED;
      *calc &= ~CALCULATING;
    }
  else
    {
      if (*calc & CALCULATING)
	{
	  /*Bad luck..some other procesor is working with out pixel :) try
	   *later.*/
	  addstack (xp, yp, dir, color, periodicity);
	  return;
	}
      mycolor = getpixel (xp, yp);
    }

  while (1)
    {
      periodicity = (mycolor == inset || color == inset);
      lookdir = turnright (dir);
      for (i = 0; i < 3; i++)
	{
	  x = xp + dirrections[lookdir][0];
	  y = yp + dirrections[lookdir][1];
	  if (x >= xstart && x <= xend && y >= ystart && y <= yend)
	    {
	      calc = calculated + x + y * CALCWIDTH;
	      if (!(*calc & (CALCULATED | CALCULATING)))
		{
		  *calc |= CALCULATING;
		  c = calculatepixel (x, y, periodicity);
		  putpixel (x, y, c);
		  *calc |= CALCULATED;
		  *calc &= ~CALCULATING;
		}
	      else
		{
		  if (*calc & CALCULATING)
		    {
		      /*Bad luck..some other procesor is working with out pixel :) try
		       *later.*/
		      addstack (xp, yp, dir, color, periodicity);
		      return;
		    }
		  c = getpixel (x, y);
		}
	      if (c == mycolor)
		break;
	      if (c != color)
		{
		  int dir2 = turnright (lookdir);
		  int mask = (1 << dir2) + (1 << turnright (dir2));
		  if (!(*calc & mask))
		    {
		      addstack (x, y, dir2, mycolor, periodicity);
		    }
		  color = c;
		}
	    }
	  lookdir = turnleft (lookdir);
	}
      x = xp + dirrections[lookdir][0];
      y = yp + dirrections[lookdir][1];
      if (x >= xstart && x <= xend && y >= ystart && y <= yend)
	{
	  calc = calculated + x + y * CALCWIDTH;
	  if (!(*calc & (1 << lookdir)))
	    {
	      *calc |= (1 << lookdir);
	      if (size < 10)
		{
		  addstack (x, y, lookdir, color, periodicity);
		  return;
		}
	      else
		{
		  xp = x;
		  yp = y;
		  dir = lookdir;
		  calc = calculated + xp + yp * CALCWIDTH;
		}
	    }
	  else
	    return;
	}
      else
	return;
    }
}
static void
queue (void *data, struct taskinfo *task, int r1, int r2)
{
  int x, y, d, c;
  int pos = 0;

  while (1)
    {
      int nstack;
      xth_lock (0);
      while (!size)		/*Well stack is empty. */
	{
	  if (exitnow)		/*Possibly everything is done now.. */
	    {
	      xth_unlock (0);
	      return;
	    }
	  if (nwaiting == nthreads - 1)	/*We are last working CPU */
	    {
	      exitnow = 1;	/*So we should exit now */
	      xth_wakeup (0);	/*Wake up all waiting tasks */
	      xth_unlock (0);
	      return;		/*and exit :) */
	    }
	  nwaiting++;		/*We are not latest task. */
	  xth_sleep (0, 0);	/*Wait until other task will push some data */
	  nwaiting--;
	  if (exitnow)		/*Evrything is done now? */
	    {
	      xth_unlock (0);
	      return;
	    }
	}
      nstack = xth_nthread (task);
      while (!sizes[nstack])
	if (nstack != nthreads - 1)
	  nstack++;
	else
	  nstack = 0;
      sizes[nstack]--;
      size--;
      pos++;
      if (pos >= sizes[nstack])
	pos = 0;
      x = starts[nstack][pos >> PAGESHIFT][pos & (PAGESIZE - 1)].x;
      y = starts[nstack][pos >> PAGESHIFT][pos & (PAGESIZE - 1)].y;
      d = starts[nstack][pos >> PAGESHIFT][pos & (PAGESIZE - 1)].direction;
      c = starts[nstack][pos >> PAGESHIFT][pos & (PAGESIZE - 1)].color;
      /* Well stack currently is queue. Should have better results at
       * SMP, since has tendency trace all ways at time, so (I believe)
       * should avoid some cache conflict and situation where queue is
       * empty. At the other hand, makes queue bigger and needs following
       * copy:
       */
      starts[nstack][pos >> PAGESHIFT][pos & (PAGESIZE - 1)] =
	starts[nstack][sizes[nstack] >> PAGESHIFT][sizes[nstack] &
						   (PAGESIZE - 1)];
      xth_unlock (0);
      tracepoint (x, y, d, c, xstart, xend, ystart, yend);
    }
}
static void
bfill (void *dat, struct taskinfo *task, int r1, int r2)
{
  int y;
  cpixel_t *pos, *end;
  unsigned char *data;
  r1 += ystart + 1;
  r2 += ystart + 1;
  for (y = r1; y < r2; y++)
    {
      pos = p_add ((cpixel_t *) cimage.currlines[y], xstart + 1);
      end = p_add ((cpixel_t *) cimage.currlines[y], xend);
      data = calculated + xstart + y * CALCWIDTH + 1;
      for (; pos < end; p_inc (pos, 1), data++)
	{
	  if (!*data)
	    p_copy (pos, 0, pos, -1);
	}
    }
}

#undef ethreads
#endif
#endif
static void
dosymetries (int x1, int x2, int y1, int y2, int xsym, int cx1, int cx2)
{
  if (cx1 != x1)
    {
      register int y;
      register cpixel_t *xx1, *xx2;
      for (y = y1; y <= y2; y++)
	{
	  xx1 = p_add ((cpixel_t *) cimage.currlines[y], x1);
	  xx2 = p_add ((cpixel_t *) cimage.currlines[y], 2 * xsym - x1);
	  while (xx1 < xx2)
	    {
	      p_copy (xx1, 0, xx2, 0);
	      p_inc (xx1, 1);
	      p_inc (xx2, -1);
	    }
	}
    }
  if (cx2 != x2)
    {
      register int y;
      register cpixel_t *xx1, *xx2;
      for (y = y1; y <= y2; y++)
	{
	  xx1 = p_add ((cpixel_t *) cimage.currlines[y], x2);
	  xx2 = p_add ((cpixel_t *) cimage.currlines[y], 2 * xsym - x2);
	  while (xx1 > xx2)
	    {
	      p_copy (xx1, 0, xx2, 0);
	      p_inc (xx1, -1);
	      p_inc (xx2, 1);
	    }
	}
    }
}
#endif

#undef dosymetries
#undef tracepoint
#undef tracecolor
#undef queue
#undef bfill
