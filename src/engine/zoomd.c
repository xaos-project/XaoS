#ifndef UNSUPPORTED

/*  this two routines implements solid guessing. They are almost same. One
 *  caluclates lines, second rows.
 *
 *  The heruistic is as follows:
 *
 *  ---1------6------5-------  (vbuffu)
 *     |      |      |
 *  ===7======X======8=======  (vbuff1)
 *     |      |      |     
 *  ---2------3------4-------  (vbuffd)
 *  distdown  rx   distup
 *
 *  -- and | means calculated lines. == is current line, names are pointers to
 *  them. Note that naming is quite confusing, because it is same in lines and
 *  rows.
 *
 *  we do solid guessing as folows: 
 *  |distl-vbuff1| < range
 *  |distr-vbuff1| < range
 *  the distance of distup and distdown is not limited, because we already
 *  have exact enought guesses 3 and 6
 *
 *  points 1 2 3 4 5 6 8 must be the same (point 8 is not yet calculated)
 *
 */
static void
calcline (realloc_t * RESTRICT ry)
REGISTERS (3);
     static void calcline (realloc_t * RESTRICT ry)
{
  number_t y;
  int range = cfractalc.range;
  realloc_t *RESTRICT rx, *rend, *rend1, *ryl, *ryr;
  int distl, distr, distup, distdown;
  cpixel_t *RESTRICT vbuff, *RESTRICT vbuffu, *RESTRICT vbuffd;
  cpixeldata_t inset = (cpixeldata_t) cpalette.pixels[0];
  cpixeldata_t c;
  cppixel_t *vbuff1 = (cpixel_t **) cimage.currlines + (ry - czoomc.reallocy);
  assert (ry >= czoomc.reallocy);
  assert (ry < czoomc.reallocy + cimage.height);
  y = ry->possition;
  rend = ry - range - 1;
  if (czoomc.reallocy > rend)
    rend = czoomc.reallocy;
  for (ryl = ry - 1; rend <= ryl && ryl->dirty; ryl--);
  distl = (int) (ryl - ry);
  rend = ry + range;
  if (czoomc.reallocy + cimage.height < rend)
    rend = czoomc.reallocy + cimage.height;
  for (ryr = ry + 1; rend > ryr && ryr->dirty; ryr++);
  distr = (int) (ryr - ry);
  rend = czoomc.reallocy + cimage.height;
  if (ryr == czoomc.reallocy + cimage.height || ryl < czoomc.reallocy
      || ryr->dirty || ryl->dirty)
    {
      for (rx = czoomc.reallocx, vbuff = *vbuff1,
	   rend1 = czoomc.reallocx + cimage.width; rx < rend1; rx++)
	{
	  if (!rx->dirty)
	    {
	      STAT (tocalculate++);
	      p_set (vbuff,
		     (cpixeldata_t) calculate (rx->possition, y,
					       cfractalc.periodicity));
#ifdef DRAW
	      vga_setcolor (0xff0000);
	      vga_drawpixel (rx - czoomc.reallocx, ry - czoomc.reallocy);
#endif
	    }
	  p_inc (vbuff, 1);
	}
    }
  else
    {
      distup = INT_MAX / 2;
      distdown = 0;
      for (rx = czoomc.reallocx,
	   vbuff = vbuff1[0], vbuffu = vbuff1[distl], vbuffd = vbuff1[distr],
	   rend1 = czoomc.reallocx + cimage.width; rx < rend1; rx++)
	{
	  assert (rx < czoomc.reallocx + cimage.width);
	  assert (rx >= czoomc.reallocx);
	  if (!rx->dirty)
	    {
	      STAT (tocalculate++);
	      if (distdown <= 0)
		{
		  for (ryr = rx + 1; ryr < rend1 && ryr->dirty; ryr++);
		  distdown = (int) (ryr - rx);
		  if (ryr == rend1)
		    distdown = INT_MAX / 2;
		}
	      if (distdown < INT_MAX / 4 && distup < INT_MAX / 4 &&
		  (p_get (vbuffu) == (c = p_get (vbuffd)) &&
		   c == p_getp (vbuff, -distup) &&
		   c == p_getp (vbuffu, -distup) &&
		   c == p_getp (vbuffu, distdown) &&
		   c == p_getp (vbuffd, distdown) &&
		   c == p_getp (vbuffd, -distup)))
		{
		  p_set (vbuff, c);
		  STAT (avoided++);
		}
	      else
		{
		  if (cfractalc.periodicity &&
		      distdown < INT_MAX / 4 && distup < INT_MAX / 4 &&
		      (p_get (vbuffu) != inset &&
		       p_get (vbuffd) != inset &&
		       p_getp (vbuff, -distup) != inset &&
		       p_getp (vbuffu, -distup) != inset &&
		       p_getp (vbuffu, +distdown) != inset &&
		       p_getp (vbuffd, -distup) != inset &&
		       p_getp (vbuffd, +distdown) != inset))
		    p_set (vbuff,
			   (cpixeldata_t) calculate (rx->possition, y, 0));
		  else
		    p_set (vbuff,
			   (cpixeldata_t) calculate (rx->possition, y,
						     cfractalc.periodicity));
#ifdef DRAW
		  vga_setcolor (0xffffff);
		  vga_drawpixel (rx - czoomc.reallocx, ry - czoomc.reallocy);
#endif
		}
	      distup = 0;
	    }
	  p_inc (vbuff, 1);
	  p_inc (vbuffu, 1);
	  p_inc (vbuffd, 1);
	  distdown--;
	  distup++;
	}
    }
  ry->recalculate = 0;
  ry->dirty = 0;
}
static void
calccolumn (realloc_t * RESTRICT rx)
REGISTERS (3);
     static void calccolumn (realloc_t * RESTRICT rx)
{
  number_t x;
  int range = cfractalc.range;
  realloc_t *RESTRICT ry, *rend, *rend1, *rxl, *rxr;
  int pos, distl, distr, distup, distdown;
  cpixeldata_t c;
  cpixeldata_t inset = (cpixeldata_t) cpalette.pixels[0];
  cppixel_t *RESTRICT vbuff;
  pos = (int) (rx - czoomc.reallocx);
  assert (pos >= 0);
  assert (pos < cimage.width);
  rend = rx - range + 1;
  if (czoomc.reallocx > rend)
    rend = czoomc.reallocx;
  for (rxl = rx - 1; rend <= rxl && rxl->dirty; rxl--);
  distl = (int) (rx - rxl);
  rend = rx + range;
  if (czoomc.reallocx + cimage.width < rend)
    rend = czoomc.reallocx + cimage.width;
  for (rxr = rx + 1; rxr < rend && rxr->dirty; rxr++);
  distr = (int) (rxr - rx);
  x = rx->possition;
  rend = czoomc.reallocx + cimage.width;
  if (rxr >= czoomc.reallocx + cimage.width || rxl < czoomc.reallocx
      || rxr->dirty || rxl->dirty)
    {
      for (ry = czoomc.reallocy, vbuff =
	   (cppixel_t *) cimage.currlines, rend1 =
	   czoomc.reallocy + cimage.height; ry < rend1; ry++, vbuff++)
	{
	  if (!ry->dirty)
	    {
	      STAT (tocalculate++);
	      p_setp ((*vbuff), pos,
		      (cpixeldata_t) calculate (x, ry->possition,
						cfractalc.periodicity));
#ifdef DRAW
	      vga_setcolor (0xff0000);
	      vga_drawpixel (rx - czoomc.reallocx, ry - czoomc.reallocy);
#endif
	    }
	}
    }
  else
    {
      distl = pos - distl;
      distr = pos + distr;
      assert (distl >= 0);
      assert (distr < cimage.width);
      distup = INT_MAX / 2;
      distdown = 0;
      for (ry = czoomc.reallocy, vbuff =
	   (cppixel_t *) cimage.currlines, rend1 =
	   czoomc.reallocy + cimage.height; ry < rend1; ry++)
	{
	  /*if (ry->symto == -1) { */
	  assert (ry < czoomc.reallocy + cimage.height);
	  if (!ry->dirty)
	    {
	      STAT (tocalculate++);
	      if (distdown <= 0)
		{
		  for (rxr = ry + 1; rxr < rend1 && rxr->dirty; rxr++);
		  distdown = (int) (rxr - ry);
		  if (rxr == rend1)
		    distdown = INT_MAX / 2;
		}
	      if (distdown < INT_MAX / 4 && distup < INT_MAX / 4 &&
		  (p_getp (vbuff[0], distl) == (c = p_getp (vbuff[0], distr))
		   && p_getp (vbuff[-distup], distl) == c
		   && p_getp (vbuff[-distup], distr) == c
		   && p_getp (vbuff[-distup], pos) == c
		   && p_getp (vbuff[distdown], distr) == c
		   && p_getp (vbuff[distdown], distl) == c))
		{
		  STAT (avoided++);
		  p_setp (vbuff[0], pos, c);
		}
	      else
		{
		  if (cfractalc.periodicity &&
		      distdown < INT_MAX / 4 && distup < INT_MAX / 4 &&
		      (p_getp (vbuff[0], distl) != inset &&
		       p_getp (vbuff[0], distr) != inset &&
		       p_getp (vbuff[distdown], distr) != inset &&
		       p_getp (vbuff[distdown], distl) != inset &&
		       p_getp (vbuff[-distup], distl) != inset &&
		       p_getp (vbuff[-distup], pos) != inset &&
		       p_getp (vbuff[-distup], distr) != inset))
		    p_setp (vbuff[0], pos,
			    (cpixeldata_t) calculate (x, ry->possition, 0));
		  else
		    p_setp (vbuff[0], pos,
			    (cpixeldata_t) calculate (x, ry->possition,
						      cfractalc.periodicity));
#ifdef DRAW
		  vga_setcolor (0xffffff);
		  vga_drawpixel (rx - czoomc.reallocx, ry - czoomc.reallocy);
#endif
		}
	      distup = 0;
	    }
	  vbuff++;
	  distdown--;
	  distup++;
	}
    }
  rx->recalculate = 0;
  rx->dirty = 0;
}

static /*INLINE */ void
dosymetry2 (void /*@unused@ */ *data, struct taskinfo /*@unused@ */ *task,
	    int r1, int r2)
{
  cpixel_t **vbuff = (cpixel_t **) cimage.currlines;
  realloc_t *rx, *rend;
  cpixel_t **vend = (cpixel_t **) cimage.currlines + cimage.height;
  for (rx = czoomc.reallocx + r1, rend = czoomc.reallocx + r2; rx < rend;
       rx++)
    {
      assert (rx->symto >= 0 || rx->symto == -1);
      if (rx->symto >= 0)
	{
	  assert (rx->symto < cimage.width);
	  if (!czoomc.reallocx[rx->symto].dirty)
	    {
	      int pos = (int) (rx - czoomc.reallocx);
	      int pos1 = rx->symto;
	      vbuff = (cpixel_t **) cimage.currlines;
	      for (; vbuff < vend; vbuff++)
		p_copy (vbuff[0], pos, vbuff[0], pos1);
	      rx->dirty = 0;
	    }
	}
    }
}

#ifndef USE_i386ASM
/*
 * Fill - bitmap depended part.
 *
 * This function is called, when calculation was interrupted because of
 * timeout. It fills uncalculated rows by nearest one
 *
 * This function is very time critical in higher resultions I am shooting
 * for.
 */
#ifndef __GNUC__
#undef bpp1
#endif
#ifndef __i386__
#undef bpp1
#endif
#undef bpp1

static INLINE void
fillline (int line)
{
  register char *RESTRICT vbuff = cimage.currlines[line];
  CONST struct filltable *RESTRICT table = (struct filltable *) tmpdata;
  while (table->length)
    {
      register cpixeldata_t s = p_get ((cpixel_t *) (vbuff + table->from));
      register cpixel_t *vcurr = (cpixel_t *) (vbuff + table->to);
#ifdef bpp1
      memset (vcurr, s, table->length);
#else
      register cpixel_t *vend = (cpixel_t *) (vbuff + table->end);
      while (vcurr < vend)
	{
	  p_set (vcurr, s);
	  p_inc (vcurr, 1);
	}
#endif
      table++;
    }
}
#endif
#endif
#undef dosymetry2
#undef calcline
#undef calccolumn
#undef fillline
#undef rend
