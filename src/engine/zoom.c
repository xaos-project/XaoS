/* 
 *     XaoS, a fast portable realtime fractal zoomer 
 *                  Copyright (C) 1996,1997 by
 *
 *      Jan Hubicka          (hubicka@paru.cas.cz)
 *      Thomas Marsh         (tmarsh@austin.ibm.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*#define DRAW */
#include <config.h>
#include <fconfig.h>
#ifdef _plan9_
#include <u.h>
#include <libc.h>
#include <stdio.h>
#else
#include <stdlib.h>
#include <stdio.h>
#ifndef _MAC
#ifndef NO_MALLOC_H
#include <malloc.h>
#endif
#endif
#ifdef __EMX__
#include <float.h>
#include <sys/cdefs.h>
#endif
#include <aconfig.h>
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#include <math.h>
#include <string.h>
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif
#ifndef _plan9_
/*#undef NDEBUG */
#include <assert.h>
#endif
#endif
#define SLARGEITER
#include <filter.h>
#include <zoom.h>
#include <archaccel.h>
#include <complex.h>		/*for myabs */
#include <plane.h>
#include <btrace.h>
#include <xthread.h>
#include <xerror.h>
#include "calculate.h"		/*an inlined calulate function */

#ifdef HAVE_GETTEXT
#include <libintl.h>
#else
#define gettext(STRING) STRING
#endif

#define ASIZE 16
#define ALIGN(x) (((x)+ASIZE-1)&(~(ASIZE-1)))
static int nsymetrized;
unsigned char *tmpdata, *tmpdata1;
struct realloc_s
{
  number_t possition;
  number_t price;
  unsigned int plus;
  int recalculate;
  int symto;
  int symref;
  int dirty;
}
#ifdef __GNUC__
__attribute__ ((aligned (32)))
#endif
  ;
typedef struct realloc_s realloc_t;


typedef struct zoom_context
{
  number_t *xpos, *ypos;
  int newcalc;
  int forversion;
  int forpversion;
  realloc_t *reallocx, *reallocy;
  int uncomplette;
  int changed;
}
zoom_context;

struct filltable
{
  int from;
  int to;
  int length;
  int end;
};
#define getzcontext(f) ((zoom_context *)((f)->data))
#define getfcontext(f) ((f)->fractalc)

#define callwait() if(cfilter.wait_function!=NULL) cfilter.wait_function(&cfilter);
#define tcallwait() if(!xth_nthread(task)&&cfilter.wait_function!=NULL) cfilter.wait_function(&cfilter);
#define setuncomplette(i) (getzcontext(&cfilter)->uncomplette=i)
#define incuncomplette() (getzcontext(&cfilter)->uncomplette++)
#define setchanged(i) (getzcontext(&cfilter)->changed=i)


zoom_context czoomc;
struct filter cfilter;
#ifdef STATISTICS
static int tocalculate = 0, avoided = 0;
static int nadded = 0, nsymetry = 0, nskipped = 0;
int nperi = 0;
#endif

#ifdef _NEVER_
#define rdtsc() ({unsigned long long time; asm __volatile__ ("rdtsc":"=A"(time)); time; })
#define startagi() ({asm __volatile__ ("rdmsr ; andw 0xfe00,%%ax ; orw 0x1f, %%ax ; wrmsr; "::"c"(22):"ax","dx"); })
#define countagi() ({unsigned long long count; asm __volatile__ ("rdmsr":"=A"(count):"c"(12)); count;})
#define cli() ({asm __volatile__ ("cli");})
#define sti() ({asm __volatile__ ("sti");})
#else
#define rdtsc() 0
#define cli() 0
#define sti() 0
#define startagi() 0
#define countagi() 0
#endif

#ifndef USE_i386ASM
static void
moveoldpoints (void *data1, struct taskinfo *task, int r1, int r2)
REGISTERS (0);
     static void fillline_8 (int line) REGISTERS (0);
     static void fillline_16 (int line) REGISTERS (0);
     static void fillline_24 (int line) REGISTERS (0);
     static void fillline_32 (int line) REGISTERS (0);
#endif

/*first of all inline driver section */
/*If you think this way is ugly, I must agree. Please let me know
 *about better one that allows to generate custom code for 8,16,24,32
 *bpp modes and use of static variables
 */
#include <c256.h>
#define fillline fillline_8
#define dosymetry2 dosymetry2_8
#define calcline calcline_8
#define calccolumn calccolumn_8
#include "zoomd.c"

#include <truecolor.h>
#define fillline fillline_32
#define dosymetry2 dosymetry2_32
#define calcline calcline_32
#define calccolumn calccolumn_32
#include "zoomd.c"

#include <true24.h>
#define fillline fillline_24
#define dosymetry2 dosymetry2_24
#define calcline calcline_24
#define calccolumn calccolumn_24
#include "zoomd.c"

#include <hicolor.h>
#define fillline fillline_16
#define dosymetry2 dosymetry2_16
#define calcline calcline_16
#define calccolumn calccolumn_16
#include "zoomd.c"

#define calcline(a) drivercall(cimage,calcline_8(a),calcline_16(a),calcline_24(a),calcline_32(a));
#define calccolumn(a) drivercall(cimage,calccolumn_8(a),calccolumn_16(a),calccolumn_24(a),calccolumn_32(a));


     struct dyn_data
     {
       int price;
       struct dyn_data *previous;
     };

#define FPMUL 64		/*Let multable fit into pentium cache */
#define RANGES 2		/*shift equal to x*RANGE */
#define RANGE 4

#define DSIZEHMASK (0x7)	/*mask equal to x%(DSIZE) */
#define DSIZE (2*RANGE)
#define DSIZES (RANGES+1)	/*shift equal to x*DSIZE */


#define adddata(n,i) (dyndata+(((n)<<DSIZES)+(((i)&(DSIZEHMASK)))))
#define getbest(i) (dyndata+((size)<<DSIZES)+(i))
#define nosetadd ((size*2)<<DSIZES)
#ifndef DEBUG
#define CHECKPOS(pos)
#else
#define CHECKPOS(pos) (assert((pos)>=dyndata),assert((pos)<dyndata+(size)+((size)<<DSIZES)))
#endif

#ifdef __POWERPC__
#  undef USE_MULTABLE
#else
#  define USE_MULTABLE 1
#endif

#ifdef USE_MULTABLE
#define PRICE(i,i1) mulmid[(i)-(i1)]
#else
#define PRICE(i,i1) (((i)-(i1)) * ((i)-(i1)))
#endif
#define NEWPRICE (FPMUL*FPMUL*(RANGE)*(RANGE))

#define NOSETMASK ((unsigned int)0x80000000)
#define END NULL
#define MAXPRICE INT_MAX
/*static int dynsize = (int)sizeof (struct dyn_data);*/
#ifndef INT_MIN
#define INT_MIN (- INT_MAX - 1)
#endif
#define IRANGE FPMUL*RANGE

#ifdef USE_MULTABLE
     static int multable[RANGE * FPMUL * 2];
     static int *mulmid;
#endif

/*Functions looks trought rows/columns marked for calculation and tries to use
 *some symetrical one instead
 */

/*FIXME should be threaded...but thread overhead should take more work than
 *do it in one, since it is quite simple and executes just in case fractal
 *on the screen is symetrical and it is quite rare case...who knows
 */
     static void		/*INLINE */
      
       preparesymetries (register realloc_t * realloc, CONST int size,
			 register int symi, number_t sym, number_t step)
{
  register int i;
  register int istart = 0;
  number_t fy, ftmp;
  realloc_t *r = realloc, *reallocs;

  sym *= 2;
  i = 2 * symi - size;
  if (i < 0)
    i = 0;
  realloc += i;

  for (; i <= symi; i++, realloc++)
    {				/*makes symetries */
      int j, min = 0;
      number_t dist = NUMBER_BIG, tmp1;

      if (realloc->symto != -1)
	continue;

      fy = realloc->possition;
      realloc->symto = 2 * symi - i;

      if (realloc->symto >= size - RANGE)
	realloc->symto = size - RANGE - 1;

      dist = RANGE * step;
      min = RANGE;
#ifndef NDEBUG
      if (realloc->symto < 0 || realloc->symto >= size)
	{
	  x_fatalerror ("Internal error #22-1 %i", realloc->symto);
	  assert (0);
	}
#endif
      reallocs = &r[realloc->symto];
      j =
	(realloc->symto - istart >
	 RANGE) ? -RANGE : (-realloc->symto + istart);

      if (realloc->recalculate)
	{
	  for (; j < RANGE && realloc->symto + j < size - 1; j++)
	    {
	      ftmp = sym - (reallocs + j)->possition;
	      if ((tmp1 = myabs (ftmp - fy)) < dist)
		{
		  if ((realloc == r || ftmp > (realloc - 1)->possition) &&
		      (ftmp < (realloc + 1)->possition))
		    {
		      dist = tmp1;
		      min = j;
		    }
		}
	      else if (ftmp < fy)
		break;
	    }

	}
      else
	{
	  for (; j < RANGE && realloc->symto + j < size - 1; j++)
	    {
	      if (!realloc->recalculate)
		continue;
	      ftmp = sym - (reallocs + j)->possition;
	      if ((tmp1 = myabs (ftmp - fy)) < dist)
		{
		  if ((realloc == r || ftmp > (realloc - 1)->possition) &&
		      (ftmp < (realloc + 1)->possition))
		    {
		      dist = tmp1;
		      min = j;
		    }
		}
	      else if (ftmp < fy)
		break;
	    }
	}
      realloc->symto += min;

      if (min == RANGE || realloc->symto <= symi ||
	  (reallocs = reallocs + min)->symto != -1 || reallocs->symref != -1)
	{
	  realloc->symto = -1;
	  continue;
	}

      if (!realloc->recalculate)
	{
	  realloc->symto = -1;
	  if (reallocs->symto != -1 || !reallocs->recalculate)
	    continue;
	  reallocs->plus = realloc->plus;
	  reallocs->symto = i;
	  nsymetrized++;
	  istart = realloc->symto - 1;
	  reallocs->dirty = 1;
	  realloc->symref = (int) (reallocs - r);
	  STAT (nadded -= reallocs->recalculate);
	  reallocs->recalculate = 0;
	  reallocs->possition = sym - realloc->possition;
	}
      else
	{
	  if (reallocs->symto != -1)
	    {
	      realloc->symto = -1;
	      continue;
	    }
	  istart = realloc->symto - 1;
	  STAT (nadded -= realloc->recalculate);
	  nsymetrized++;
	  realloc->dirty = 1;
	  realloc->plus = reallocs->plus;
	  realloc->recalculate = 0;
	  reallocs->symref = i;
	  realloc->possition = sym - reallocs->possition;
	}
      STAT (nsymetry++);

#ifndef NDEBUG
      if (realloc->symto < -1 || realloc->symto >= size)
	{
	  x_fatalerror ("Internal error #22 %i", realloc->symto);
	  assert (0);
	}
      if (reallocs->symto < -1 || reallocs->symto >= size)
	{
	  x_fatalerror ("Internal error #22-2 %i", reallocs->symto);
	  assert (0);
	}
#endif
    }

}
static /*INLINE */ void
newpossitions (realloc_t * realloc, unsigned int size, number_t begin1,
	       number_t end1, CONST number_t * fpos, int yend)
{
  realloc_t *rs, *re, *rend;
  number_t step = size / (end1 - begin1);
  number_t start;
  number_t end;
  rend = realloc + size;
  rs = realloc - 1;
  re = realloc;
  while (rs < rend - 1)
    {
      re = rs + 1;
      if (re->recalculate)
	{
	  while (re < rend && re->recalculate)
	    re++;

	  if (re == rend)
	    end = end1;
	  else
	    end = re->possition;

	  if (rs == realloc - 1)
	    {
	      start = begin1;
	      if (start > end)
		start = end;
	    }
	  else
	    start = rs->possition;

	  if (re == rend && start > end)
	    end = start;

	  if (re - rs == 2)
	    end = (end - start) * 0.5;
	  else
	    end = ((number_t) (end - start)) / (re - rs);


	  switch (yend)
	    {
	    case 1:
	      for (rs++; rs < re; rs++)
		{
		  start += end, rs->possition = start;
		  rs->price =
		    1 / (1 + myabs (fpos[rs - realloc] - start) * step);
		}
	      break;
	    case 2:
	      for (rs++; rs < re; rs++)
		{
		  start += end, rs->possition = start;
		  rs->price = (myabs (fpos[rs - realloc] - start) * step);
		  if (rs == realloc || rs == rend - 1)
		    rs->price *= 500;
		}
	      break;
	    default:
	      for (rs++; rs < re; rs++)
		{
		  start += end, rs->possition = start;
		  rs->price = (number_t) 1;
		}
	      break;
	    }
	}
      rs = re;
    }
}

/* This is the main reallocation algorithm described in xaos.info
 * It is quite complex since many loops are unrooled and uses custom
 * fixedpoint
 *
 * Takes approx 30% of time so looking for way to do it threaded.
 * Let me know :)
 */
static /*INLINE */ void
mkrealloc_table (CONST number_t * RESTRICT fpos, realloc_t * RESTRICT realloc,
		 CONST unsigned int size, CONST number_t begin,
		 CONST number_t end, number_t sym, unsigned char *tmpdata)
{
  unsigned int i;
  int counter;
  unsigned int ps, ps1 = 0, pe;
  register unsigned int p;
  int bestprice = MAXPRICE;
  realloc_t *r = realloc;
  struct dyn_data *RESTRICT dyndata;
  int yend, y;
  register struct dyn_data **RESTRICT best;
  struct dyn_data **RESTRICT best1, **tmp;
  register int *RESTRICT pos;
  number_t step, tofix;
  int symi = -1;
  unsigned int lastplus = 0;
  struct dyn_data *RESTRICT data;
  register struct dyn_data *previous = NULL, *bestdata = NULL;
  register int myprice;
#ifdef STATISTICS
  nadded = 0, nsymetry = 0, nskipped = 0;
#endif

  pos = (int *) tmpdata;
  best = (struct dyn_data **) (tmpdata + ALIGN ((size + 2) * sizeof (int)));
  best1 =
    (struct dyn_data **) (tmpdata + ALIGN ((size + 2) * sizeof (int)) +
			  ALIGN (size * sizeof (struct dyn_data **)));
  dyndata =
    (struct dyn_data *) (tmpdata + ALIGN ((size + 2) * sizeof (int)) +
			 2 * ALIGN (size * sizeof (struct dyn_data **)));

  tofix = size * FPMUL / (end - begin);
  pos[0] = INT_MIN;
  pos++;
  for (counter = (int) size - 1; counter >= 0; counter--)
    {
      pos[counter] = (int) ((fpos[counter] - begin) * tofix);	/*first convert everything into fixedpoint */
      if (counter < (int) size - 1 && pos[counter] > pos[counter + 1])
	/*Avoid processing of missordered rows.
	   They should happend because of limited
	   precisity of FP numbers */
	pos[counter] = pos[counter + 1];
    }
  pos[size] = INT_MAX;
  step = (end - begin) / (number_t) size;
  if (begin > sym || sym > end)	/*calculate symetry point */
    symi = -2;
  else
    {
      symi = (int) ((sym - begin) / step);

    }

  ps = 0;
  pe = 0;
  y = 0;

  /* This is first pass that fills table dyndata, that holds information
   * about all ways algorithm thinks about. Correct way is discovered at
   * end by looking backward and determining witch way algorithm used to
   * calculate minimal value*/

  for (i = 0; i < size; i++, y += FPMUL)
    {
      bestprice = MAXPRICE;
      p = ps;			/*just inicialize parameters */

      tmp = best1;
      best1 = best;
      best = tmp;

      yend = y - IRANGE;
      if (yend < -FPMUL)	/*do no allow lines outside screen */
	yend = -FPMUL;

      while (pos[p] <= yend)	/*skip lines out of range */
	p++;
#ifdef _UNDEFINED_
      if (pos[p - 1] > yend)	/*warning...maybe this is the bug :) */
	p--, assert (0);
#endif
      ps1 = p;
      yend = y + IRANGE;

      /*First try case that current line will be newly calculated */

      /*Look for best way how to connect previous lines */
      if (ps != pe && p > ps)
	{			/*previous point had lines */
	  assert (p >= ps);
	  if (p < pe)
	    {
	      previous = best[p - 1];
	      CHECKPOS (previous);
	    }
	  else
	    previous = best[pe - 1];
	  CHECKPOS (previous);
	  myprice = previous->price;	/*find best one */
	}
      else
	{
	  if (i > 0)
	    {			/*previous line had no lines */
	      previous = getbest (i - 1);
	      myprice = previous->price;
	    }
	  else
	    previous = END, myprice = 0;
	}

      data = getbest (i);	/*find store possition */
      myprice += NEWPRICE;
      bestdata = data;
      data->previous = previous;
      bestprice = myprice;	/*calculate best available price */
      data->price = myprice;	/*store data */
      assert (bestprice >= 0);	/*FIXME:tenhle assert muze FAILIT! */
#ifdef _UNDEFINED_
      if (yend > end + FPMUL)	/*check bounds */
	yend = end + FPMUL;
#endif
      data = adddata (p, i);	/*calcualte all lines good for this y */

      /* Now try all acceptable connection and calculate best possibility
       * with this connection
       */
      if (ps != pe)
	{			/*in case that previous had also possitions */
	  int price1 = INT_MAX;
	  /*At first line of previous interval we have only one possibility
	   *don't connect previous line at all.
	   */
	  if (p == ps)
	    {			/*here we must skip previous point */
	      if (pos[p] != pos[p + 1])
		{
		  previous = getbest (i - 1);
		  myprice = previous->price;
		  myprice += PRICE (pos[p], y);	/*store data */
		  if (myprice < bestprice)
		    {		/*calcualte best */
		      bestprice = myprice, bestdata = data;
		      data->price = myprice;
		      data->previous = previous;
		    }
		}
	      assert (bestprice >= 0);
	      assert (myprice >= 0);
	      best1[p] = bestdata;
	      data += DSIZE;
	      p++;
	    }

	  previous = NULL;
	  price1 = myprice;
	  while (p < pe)
	    {			/*this is area where intervals of current point and previous one are crossed */
	      if (pos[p] != pos[p + 1])
		{
		  if (previous != best[p - 1])
		    {

		      previous = best[p - 1];
		      CHECKPOS (previous);
		      price1 = myprice = previous->price;

		      /*In case we found revolutional point, we should think
		       *about changing our gusesses in last point too - don't
		       *connect it at all, but use this way instead*/
		      if (myprice + NEWPRICE < bestprice)	/*true in approx 2/3 of cases */
			{
			  bestprice = myprice + NEWPRICE, bestdata =
			    data - DSIZE;
			  (bestdata)->price = bestprice;
			  (bestdata)->previous = previous + nosetadd;
			  best1[p - 1] = bestdata;
			}
		    }
		  else
		    myprice = price1;

		  myprice += PRICE (pos[p], y);	/*calculate price of new connection */

		  if (myprice < bestprice)	/*2/3 of cases */
		    {		/*if it is better than previous, store it */
		      bestprice = myprice, bestdata = data;
		      data->price = myprice;
		      data->previous = previous;
		    }
		  else if (pos[p] > y)
		    {
		      best1[p] = bestdata;
		      data += DSIZE;
		      p++;
		      break;
		    }

		}

	      assert (myprice >= 0);
	      assert (bestprice >= 0);	/*FIXME:tenhle assert FAILI! */

	      best1[p] = bestdata;
	      data += DSIZE;
	      p++;
	    }
	  while (p < pe)
	    {			/*this is area where intervals of current point and previous one are crossed */
#ifdef DEBUG
	      if (pos[p] != pos[p + 1])
		{
		  if (previous != best[p - 1])
		    {
		      x_fatalerror ("Missoptimization found!");
		    }
		}
#endif
#ifdef _UNDEFINED_
	      /* Experimental results show, that probability for better approximation
	       * in this area is extremly low. Maybe it never happends. 
	       * I will have to think about it a bit more... It seems to have
	       * to do something with meaning of universe and god... no idea
	       * why it is true.
	       *
	       * Anyway it don't seems to worth include the expensive tests
	       * here.
	       */
	      if (pos[p] != pos[p + 1])
		{
		  if (previous != best[p - 1])
		    {

		      previous = best[p - 1];
		      CHECKPOS (previous);
		      myprice = previous->price;

		      /*In case we found revolutional point, we should think
		       *about changing our gusesses in last point too - don't
		       *connect it at all, but use this way instead*/
		      if (myprice + NEWPRICE < bestprice)	/*true in approx 2/3 of cases */
			{
			  bestprice = myprice + NEWPRICE, bestdata =
			    data - DSIZE;
			  (bestdata)->price = bestprice;
			  (bestdata)->previous = previous + nosetadd;
			  best1[p - 1] = bestdata;
			}
		      myprice += PRICE (pos[p], y);	/*calculate price of new connection */
		      if (myprice < bestprice)
			{	/*if it is better than previous, store it */
			  bestprice = myprice, bestdata = data;
			  data->price = myprice;
			  data->previous = previous;
			}
		    }
		}
#endif
	      assert (myprice >= 0);
	      assert (bestprice >= 0);	/*FIXME:tenhle assert FAILI! */

	      best1[p] = bestdata;
	      data += DSIZE;
	      p++;
	    }

	  /* OK...we passed crossed area. All next areas have same previous
	   * situation so our job is easier
	   * So find the best solution once for all od them
	   */
	  if (p > ps)
	    {
	      previous = best[p - 1];	/*find best one in previous */
	      CHECKPOS (previous);
	      price1 = previous->price;
	    }
	  else
	    {
	      previous = getbest (i - 1);
	      price1 = previous->price;
	    }

	  /* Since guesses for "revolutional point" was allways one
	   * step back, we need to do last one*/
	  if (price1 + NEWPRICE < bestprice && p > ps1)
	    {
	      myprice = price1 + NEWPRICE;
	      bestprice = myprice, bestdata = data - DSIZE;
	      (bestdata)->price = myprice;
	      (bestdata)->previous = previous + nosetadd;
	      best1[p - 1] = bestdata;
	      myprice -= NEWPRICE;
	    }

	  while (pos[p] < yend)
	    {
	      if (pos[p] != pos[p + 1])
		{
		  myprice = price1;
		  myprice += PRICE (pos[p], y);	/*store data */
		  if (myprice < bestprice)
		    {		/*calcualte best */
		      bestprice = myprice, bestdata = data;
		      data->price = myprice;
		      data->previous = previous;
		    }
		  else if (pos[p] > y)
		    break;
		}

	      assert (bestprice >= 0);
	      assert (myprice >= 0);

	      best1[p] = bestdata;
	      data += DSIZE;
	      p++;
	    }
	  while (pos[p] < yend)
	    {
	      best1[p] = bestdata;
	      p++;
	    }
	}
      else
	{
	  /* This is second case - previous y was not mapped at all.
	   * Situation is simplier now, since we know that behind us is
	   * large hole and our decisions don't affect best solution for
	   * previous problem. Se we have just one answer
	   * Situation is similiar to latest loop in previous case
	   */
	  int myprice1;		/*simplified loop for case that previous
				   y had no lines */
	  if (pos[p] < yend)
	    {
	      if (i > 0)
		{
		  previous = getbest (i - 1);
		  myprice1 = previous->price;
		}
	      else
		previous = END, myprice1 = 0;
	      while (pos[p] < yend)
		{
		  if (pos[p] != pos[p + 1])
		    {
		      myprice = myprice1 + PRICE (pos[p], y);
		      if (myprice < bestprice)
			{
			  data->price = myprice;
			  data->previous = previous;
			  bestprice = myprice, bestdata = data;
			}
		      else if (pos[p] > y)
			break;
		    }
		  assert (bestprice >= 0);
		  assert (myprice >= 0);
		  best1[p] = bestdata;
		  p++;
		  data += DSIZE;
		}
	      while (pos[p] < yend)
		{
		  best1[p] = bestdata;
		  p++;
		}
	    }
	}
/*previous = ps; *//*store possitions for next loop */
      ps = ps1;
      ps1 = pe;
      pe = p;
    }


  assert (bestprice >= 0);

  realloc = realloc + size;
  yend = (int) ((begin > fpos[0]) && (end < fpos[size - 1]));

  if (pos[0] > 0 && pos[size - 1] < (int) size * FPMUL)
    yend = 2;



  /*This part should be made threaded quite easily...but does it worth
   *since it is quite simple loop 0...xmax
   */
  for (i = size; i > 0;)
    {				/*and finally traces the path */
      struct dyn_data *bestdata1;
      realloc--;
      i--;
      realloc->symto = -1;
      realloc->symref = -1;
      bestdata1 = bestdata->previous;

      if (bestdata1 >= dyndata + nosetadd
	  || bestdata >= dyndata + ((size) << DSIZES))
	{
	  if (bestdata1 >= dyndata + nosetadd)
	    bestdata1 -= nosetadd;

	  realloc->recalculate = 1;
	  STAT (nadded++);
	  realloc->dirty = 1;
	  lastplus++;

	  if (lastplus >= size)
	    lastplus = 0;

	  realloc->plus = lastplus;

	}
      else
	{
	  p = ((unsigned int) (bestdata - dyndata)) >> DSIZES;
	  assert (p >= 0 && p < size);
	  realloc->possition = fpos[p];
	  realloc->plus = p;
	  realloc->dirty = 0;
	  realloc->recalculate = 0;
	  lastplus = p;
	}
      bestdata = bestdata1;
    }



  newpossitions (realloc, size, begin, end, fpos, yend);
  realloc = r;
  if (symi <= (int) size && symi >= 0)
    {
      preparesymetries (r, (int) size, symi, sym, step);
    }


  STAT (printf
	("%i added %i skipped %i mirrored\n", nadded, nskipped, nsymetry));
  STAT (nadded2 += nadded;
	nskipped2 += nskipped;
	nsymetry2 += nsymetry);
}

struct movedata
{
  unsigned int size;
  unsigned int start;
  unsigned int plus;
};
int avgsize;
/* 
 * this function prepares fast moving table for moveoldpoints
 * see xaos.info for details. It is not threaded since it is quite
 * fast.
 */
static /*INLINE */ void
preparemoveoldpoints (void)
{
  struct movedata *data, *sizend;
  realloc_t *rx, *rx1, *rend1;
  int sum = 0, num = 0;
  int plus1 = 0;

  data = (struct movedata *) tmpdata;
  for (rx = czoomc.reallocx, rend1 = rx + cimage.width; rx < rend1; rx++)
    if ((rx->dirty) && plus1 < cimage.width + 1)
      plus1++;
    else
      break;
  data->start = czoomc.reallocx->plus;
  data->size = 0;
  data->plus = plus1;
  rend1--;
  while (rend1->dirty)
    {
      if (rend1 == czoomc.reallocx)
	return;
      rend1--;
    }
  rend1++;
  for (; rx < rend1; rx++)
    {
      if ((rx->dirty || rx->plus == data->start + data->size))
	data->size++;
      else
	{
	  if (data->size)
	    {
	      plus1 = 0;
	      rx1 = rx - 1;
	      while (rx1 > czoomc.reallocx && rx1->dirty)
		plus1++, data->size--, rx1--;
	      if (!(data->start + data->size < (unsigned int) cimage.width)
		  && !rx->dirty)
		{
		  int i;
		  if (rx == rend1)
		    break;
		  for (i = 0; rx->dirty && rx < rend1; rx++)
		    i++;
		  data++;
		  data->plus = plus1;
		  data->size = (unsigned int) i;
		  data->start = rx->plus - i;
		}
	      else
		{
		  sum += data->size;
		  num++;
		  data++;
		  data->plus = plus1;
		  data->start = rx->plus;
		}
	    }
	  else
	    data->start = rx->plus;
	  assert (rx->plus >= 0 && rx->plus < (unsigned int) cimage.width);
	  data->size = 1;
	}

    }
  if (data->size)
    {
      sizend = data + 1;
      sum += data->size;
      rx1 = rx - 1;
      while (rx1 > czoomc.reallocx && rx1->dirty)
	data->size--, rx1--;
      num++;
    }
  else
    sizend = data;
  sizend->size = 0;
  if (cimage.bytesperpixel != 1)
    {
      sum *= cimage.bytesperpixel;
      for (data = (struct movedata *) tmpdata; data < sizend; data++)
	{
	  data->plus *= cimage.bytesperpixel;
	  data->size *= cimage.bytesperpixel;
	  data->start *= cimage.bytesperpixel;
	}
    }
  if (num)
    avgsize = sum / num;
}

#ifndef USE_i386ASM
static /*INLINE */ void
moveoldpoints (void /*@unused@ */ *data1, struct taskinfo /*@unused@ */ *task,
	       int r1, int r2)
{
  struct movedata *data;
  register unsigned char *vline, *vbuff;
  realloc_t *ry, *rend;
  int i = r1;

  for (ry = czoomc.reallocy + r1, rend = czoomc.reallocy + r2; ry < rend;
       ry++, i++)
    {
      if (!ry->dirty)
	{
	  assert (ry->plus >= 0 && ry->plus < (unsigned int) cimage.height);
	  vbuff = cimage.currlines[i];
	  vline = cimage.oldlines[ry->plus];
	  for (data = (struct movedata *) tmpdata; data->size; data++)
	    {
	      vbuff += data->plus;
	      memcpy (vbuff, vline + data->start, (size_t) data->size),
		vbuff += data->size;
	    }
	}
    }
}
#endif
/* This function prepares fast filling tables for fillline */
static /*INLINE */ int
mkfilltable (void)
{
  int vsrc;
  int pos;
  realloc_t *rx, *r1, *r2, *rend, *rend2;
  int n = 0;
  int num = 0;
  struct filltable *tbl = (struct filltable *) tmpdata;

  pos = 0;
  vsrc = 0;

  rx = czoomc.reallocx;
  while (rx > czoomc.reallocx && rx->dirty)
    rx--;
  for (rend = czoomc.reallocx + cimage.width, rend2 =
       czoomc.reallocx + cimage.width; rx < rend; rx++)
    {
      if (rx->dirty)
	{
	  r1 = rx - 1;
	  for (r2 = rx + 1; r2->dirty && r2 < rend2; r2++);
	  while (rx->dirty && rx < rend2)
	    {
	      n = (int) (r2 - rx);
	      assert (n > 0);
	      if (r2 < rend2
		  && (r1 < czoomc.reallocx
		      || rx->possition - r1->possition >
		      r2->possition - rx->possition))
		vsrc = (int) (r2 - czoomc.reallocx), r1 = r2;
	      else
		{
		  vsrc = (int) (r1 - czoomc.reallocx);
		  if (vsrc < 0)
		    goto end;
		}
	      pos = (int) (rx - czoomc.reallocx);
	      assert (pos >= 0 && pos < cimage.width);
	      assert (vsrc >= 0 && vsrc < cimage.width);

	      tbl[num].length = n;
	      tbl[num].to = pos * cimage.bytesperpixel;
	      tbl[num].from = vsrc * cimage.bytesperpixel;
	      tbl[num].end =
		tbl[num].length * cimage.bytesperpixel + tbl[num].to;
	      /*printf("%i %i %i %i\n",num,tbl[num].length, tbl[num].to, tbl[num].from); */
	      while (n)
		{
		  rx->possition = czoomc.reallocx[vsrc].possition;
		  rx->dirty = 0;
		  rx++;
		  n--;
		}
	      num++;
	    }			/*while rx->dirty */
	}			/*if rx->dirty */
    }				/*for czoomc */
end:
  tbl[num].length = 0;
  tbl[num].to = pos;
  tbl[num].from = vsrc;
  return num;
}

static /*INLINE */ void
filly (void /*@unused@ *//*@null@ */ *data,
       struct taskinfo /*@unused@ */ *task, int rr1, int rr2)
{
  register unsigned char **vbuff = cimage.currlines;
  realloc_t *ry, *r1, *r2, *rend, *rend2, *rs = NULL;
  int linesize = cimage.width * cimage.bytesperpixel;

  ry = czoomc.reallocy + rr1;

  ry = czoomc.reallocy + rr1;
  while (ry > czoomc.reallocy && ry->dirty > 0)
    ry--;
  for (rend = czoomc.reallocy + rr2, rend2 = czoomc.reallocy + cimage.height;
       ry < rend; ry++)
    {
      if (ry->dirty > 0)
	{
	  incuncomplette ();
	  r1 = ry - 1;
	  for (r2 = ry + 1; r2->dirty > 0 && r2 < rend2; r2++);
#ifdef _UNDEFINED_
	  if (r2 >= rend && (rr2 != cimage.height || ry == 0))
#else
	  if (r2 >= rend2 && (rr2 != cimage.height || ry == 0))
#endif
	    return;
	  while (ry->dirty > 0 && ry < rend2)
	    {
	      if (r1 < czoomc.reallocy)
		{
		  rs = r2;
		  if (r2 >= rend2)
		    return;
		}
	      else if (r2 >= rend2)
		rs = r1;
	      else if (ry->possition - r1->possition <
		       r2->possition - ry->possition)
		rs = r1;
	      else
		rs = r2;
	      if (!rs->dirty)
		{
		  drivercall (cimage,
			      fillline_8 (rs - czoomc.reallocy),
			      fillline_16 (rs - czoomc.reallocy),
			      fillline_24 (rs - czoomc.reallocy),
			      fillline_32 (rs - czoomc.reallocy));
		  ry->dirty = -1;
		}
	      memcpy (vbuff[ry - czoomc.reallocy],
		      vbuff[rs - czoomc.reallocy], (size_t) linesize);
	      ry->possition = rs->possition;
	      ry->dirty = -1;
	      ry++;
	    }
	}
      if (ry < rend && !ry->dirty)
	{
	  drivercall (cimage,
		      fillline_8 (ry - czoomc.reallocy),
		      fillline_16 (ry - czoomc.reallocy),
		      fillline_24 (ry - czoomc.reallocy),
		      fillline_32 (ry - czoomc.reallocy));
	  ry->dirty = -1;
	}
    }
}
static void
fill (void)
{
  if (cfilter.interrupt)
    {
      cfilter.pass = "reducing resolution";
      mkfilltable ();
      xth_function (filly, NULL, cimage.height);
    }
  xth_sync ();
}

static /*INLINE */ void
calculatenew (void /*@unused@ */ *data, struct taskinfo /*@unused@ */ *task,
	      int /*@unused@ */ r1, int /*@unused@ */ r2)
{
  int s;
  int i, y;
  realloc_t *rx, *ry, *rend;
  int range = cfractalc.range * 2;
  int positions[16];
  int calcpositions[16];
  /*int s3; */
  if (range < 1)
    range = 1;
  if (range > 16)
    range = 16;
  memset (positions, 0, sizeof (positions));
  calcpositions[0] = 0;
  positions[0] = 1;
  for (s = 1; s < range;)
    {
      for (i = 0; i < range; i++)
	{
	  if (!positions[i])
	    {
	      for (y = i; y < range && !positions[y]; y++);
	      positions[(y + i) / 2] = 1;
	      calcpositions[s++] = (y + i) / 2;
	    }
	}
    }

  if (!xth_nthread (task))
    {
      STAT (tocalculate = 0);
      STAT (avoided = 0);
      cfilter.pass = gettext ("Solid guessing 1");
      cfilter.max = 0;
      cfilter.pos = 0;
    }

  /* We don't need to wory about race conditions here, since only
   * problem that should happend is incorrectly counted number
   * of lines to do...
   *
   * I will fix that problem later, but I think that this information
   * should be quite useless at multithreaded systems so it should
   * be a bit inaccurate. Just need to take care in percentage
   * displayers that thinks like -100% or 150% should happend
   */
  if (!xth_nthread (task))
    {
      for (ry = czoomc.reallocy, rend = ry + cimage.height; ry < rend; ry++)
	{
	  if (ry->recalculate)
	    cfilter.max++;
	}
      for (rx = czoomc.reallocx, rend = rx + cimage.width; rx < rend; rx++)
	{
	  if (rx->recalculate)
	    {
	      cfilter.max++;
	    }
	}
    }
  tcallwait ();
  for (s = 0; s < range; s++)
    {
      for (ry = czoomc.reallocy + calcpositions[s], rend =
	   czoomc.reallocy + cimage.height; ry < rend; ry += range)
	{
	  xth_lock (0);
	  if (ry->recalculate == 1)
	    {
	      ry->recalculate = 2;
	      xth_unlock (0);
	      setchanged (1);
	      ry->dirty = 0;
	      calcline (ry);
	      cfilter.pos++;
#ifndef DRAW
	      tcallwait ();
#endif
	      if (cfilter.interrupt)
		{
		  break;
		}
	    }
	  else
	    {
	      xth_unlock (0);
	    }
	}			/*for ry */
      for (rx = czoomc.reallocx + calcpositions[s], rend =
	   czoomc.reallocx + cimage.width; rx < rend; rx += range)
	{
	  xth_lock (1);
	  if (rx->recalculate == 1)
	    {
	      rx->recalculate = 2;
	      xth_unlock (1);
	      setchanged (1);
	      rx->dirty = 0;
	      calccolumn (rx);
	      cfilter.pos++;
#ifndef DRAW
	      tcallwait ();
#endif
	      if (cfilter.interrupt)
		{
		  return;
		}
	    }
	  else
	    {
	      xth_unlock (1);
	    }
	}
    }
  STAT (printf
	("Avoided caluclating of %i points from %i and %2.2f%% %2.2f%%\n",
	 avoided, tocalculate, 100.0 * (avoided) / tocalculate,
	 100.0 * (tocalculate - avoided) / cimage.width / cimage.height));
  STAT (avoided2 += avoided;
	tocalculate2 += tocalculate;
	frames2 += 1);
}

static void
addprices (realloc_t * r, realloc_t * r2)
REGISTERS (3);
     static void addprices (realloc_t * r, realloc_t * r2)
{
  realloc_t *r3;
  while (r < r2)
    {
      r3 = r + (((unsigned int) (r2 - r)) >> 1);
      r3->price = (r2->possition - r3->possition) * (r3->price);
      if (r3->symref != -1)
	r3->price = r3->price / 2;
      addprices (r, r3);
      r = r3 + 1;
    }
}

/* We can't do both symetryies (x and y) in one loop at multithreaded
 * systems,since we need to take care to points at the cross of symetrized
 * point/column
 */
static /*INLINE */ void
dosymetry (void /*@unused@ */ *data, struct taskinfo /*@unused@ */ *task,
	   int r1, int r2)
{
  unsigned char **vbuff = cimage.currlines + r1;
  realloc_t *ry, *rend;
  int linesize = cimage.width * cimage.bytesperpixel;

  for (ry = czoomc.reallocy + r1, rend = czoomc.reallocy + r2; ry < rend;
       ry++)
    {
      assert (ry->symto >= 0 || ry->symto == -1);
      if (ry->symto >= 0)
	{
	  assert (ry->symto < cimage.height);
	  if (!czoomc.reallocy[ry->symto].dirty)
	    {
	      memcpy (*vbuff, cimage.currlines[ry->symto], (size_t) linesize);
	      ry->dirty = 0;
	    }
	}
      vbuff++;
    }
}

/*Well, clasical simple quicksort. Should be faster than library one
 *because of reduced number of function calls :)
 */
static INLINE void
myqsort (realloc_t ** start, realloc_t ** end)
{
  number_t med;
  realloc_t **left = start, **right = end - 1;
  while (1)
    {

      /*Quite strange caluclation of median, but should be
       *as good as Sedgewick middle of three method and is faster*/
      med = ((*start)->price + (*(end - 1))->price) * 0.5;

      /*Avoid one comparsion */
      if (med > (*start)->price)
	{
	  realloc_t *tmp;
	  tmp = *left;
	  *left = *right;
	  *right = tmp;
	}
      right--;
      left++;

      while (1)
	{
	  realloc_t *tmp;

	  while (left < right && (*left)->price > med)
	    left++;
	  while (left < right && med > (*right)->price)
	    right--;

	  if (left < right)
	    {
	      tmp = *left;
	      *left = *right;
	      *right = tmp;
	      left++;
	      right--;
	    }
	  else
	    break;
	}
      if (left - start > 1)
	myqsort (start, left);
      if (end - right <= 2)
	return;
      left = start = right;
      right = end - 1;
    }
}
static int tocalcx, tocalcy;
static void
processqueue (void *data, struct taskinfo /*@unused@ */ *task,
	      int /*@unused@ */ r1, int /*@unused@ */ r2)
{
  realloc_t **tptr = (realloc_t **) data, **tptr1 = (realloc_t **) tmpdata;
  realloc_t *r, *end;
  end = czoomc.reallocx + cimage.width;

  while (tptr1 < tptr
	 && (!cfilter.interrupt || tocalcx == cimage.width
	     || tocalcy == cimage.height))
    {
      xth_lock (0);
      r = *tptr1;
      if (r != NULL)
	{
	  *tptr1 = NULL;
	  xth_unlock (0);
	  cfilter.pos++;
	  if (tocalcx < cimage.width - 2 && tocalcy < cimage.height - 2)
	    cfilter.readyforinterrupt = 1;
	  tcallwait ();
	  if (r >= czoomc.reallocx && r < end)
	    {
	      r->dirty = 0;
	      tocalcx--;
	      calccolumn (r);
	    }
	  else
	    {
	      r->dirty = 0;
	      tocalcy--;
	      calcline (r);
	    }
	}
      else
	{
	  xth_unlock (0);
	}
      tptr1++;
    }
}

/*
 * Another long unthreaded code. It seems to be really long and
 * ugly, but believe or not it takes just about 4% of calculation time,
 * so why to worry about? :)
 *
 * This code looks for columns/lines to calculate, adds them into queue,
 * sorts it in order of significancy and then calls parrel processqueue,
 * that does the job.
 */
static void
calculatenewinterruptible (void)
{
  realloc_t *r, *r2, *end, *end1;
  realloc_t **table, **tptr;

  /*tptr = table = (realloc_t **) malloc (sizeof (*table) * (cimage.width + cimage.height)); */
  tptr = table = (realloc_t **) tmpdata;
  end = czoomc.reallocx + cimage.width;
  tocalcx = 0, tocalcy = 0;

  STAT (tocalculate = 0);
  STAT (avoided = 0);

  cfilter.pass = gettext ("Solid guessing");

  for (r = czoomc.reallocx; r < end; r++)
    if (r->dirty)
      tocalcx++, setchanged (1);

  for (r = czoomc.reallocx; r < end; r++)
    {
      if (r->recalculate)
	{
	  for (r2 = r; r2->recalculate && r2 < end; r2++)
	    *(tptr++) = r2;
	  if (r2 == end)
	    /*(r2 - 1)->price = 0, */ r2--;
	  addprices (r, r2);
	  r = r2;
	}
    }

  end1 = czoomc.reallocy + cimage.height;

  for (r = czoomc.reallocy; r < end1; r++)
    if (r->dirty)
      tocalcy++, setchanged (1);

  for (r = czoomc.reallocy; r < end1; r++)
    {
      if (r->recalculate)
	{
	  for (r2 = r; r2->recalculate && r2 < end1; r2++)
	    *(tptr++) = r2;
	  if (r2 == end1)
	    /*(r2 - 1)->price = 0, */ r2--;
	  addprices (r, r2);
	  r = r2;
	}
    }
  if (table != tptr)
    {

      if (tptr - table > 1)
	myqsort (table, tptr);

      cfilter.pos = 0;
      cfilter.max = (int) (tptr - table);
      cfilter.incalculation = 1;
      callwait ();

      xth_function (processqueue, tptr, 1);

      callwait ();
    }

  cfilter.pos = 0;
  cfilter.max = 0;
  cfilter.pass = "Procesing symetries";
  cfilter.incalculation = 0;
  callwait ();

  xth_sync ();
  if (nsymetrized)
    {
      xth_function (dosymetry, NULL, cimage.height);
      xth_sync ();
      drivercall (cimage,
		  xth_function (dosymetry2_8, NULL, cimage.width),
		  xth_function (dosymetry2_16, NULL, cimage.width),
		  xth_function (dosymetry2_24, NULL, cimage.width),
		  xth_function (dosymetry2_32, NULL, cimage.width));
      xth_sync ();
    }
  if (cfilter.interrupt)
    {
      cfilter.pass = "reducing resolution";
      mkfilltable ();
      xth_function (filly, NULL, cimage.height);
    }
  xth_sync ();

  STAT (printf
	("Avoided caluclating of %i points from %i and %2.2f%% %2.2f%%\n",
	 avoided, tocalculate, 100.0 * (avoided) / tocalculate,
	 100.0 * (tocalculate - avoided) / cimage.width / cimage.height));
  STAT (avoided2 += avoided;
	tocalculate2 += tocalculate;
	frames2 += 1);
}
static void
init_tables (struct filter *f)
{
  int i;
  zoom_context *c = getzcontext (f);

  /*c->dirty = 2; */
  for (i = 0; i < f->image->width + 1; i++)
    c->xpos[i] =
      (-f->fractalc->rs.nc + f->fractalc->rs.mc) + f->fractalc->rs.mc;
  for (i = 0; i < f->image->height + 1; i++)
    c->ypos[i] =
      (-f->fractalc->rs.ni + f->fractalc->rs.mi) + f->fractalc->rs.mi;
}


static int
alloc_tables (struct filter *f)
{
  zoom_context *c = getzcontext (f);
  c->xpos = (number_t *) malloc ((f->image->width + 8) * sizeof (*c->xpos));
  if (c->xpos == NULL)
    return 0;
  c->ypos = (number_t *) malloc ((f->image->height + 8) * sizeof (*c->ypos));
  if (c->ypos == NULL)
    {
      free ((void *) c->xpos);
      return 0;
    }
  c->reallocx =
    (realloc_t *) malloc (sizeof (realloc_t) * (f->image->width + 8));
  if (c->reallocx == NULL)
    {
      free ((void *) c->xpos);
      free ((void *) c->ypos);
      return 0;
    }
  c->reallocy =
    (realloc_t *) malloc (sizeof (realloc_t) * (f->image->height + 8));
  if (c->reallocy == NULL)
    {
      free ((void *) c->xpos);
      free ((void *) c->ypos);
      free ((void *) c->reallocx);
      return 0;
    }
  return 1;
}

static void
free_tables (struct filter *f)
{
  zoom_context *c = getzcontext (f);
  if (c->xpos != NULL)
    free ((void *) c->xpos), c->xpos = NULL;
  if (c->ypos != NULL)
    free ((void *) c->ypos), c->ypos = NULL;
  if (c->reallocx != NULL)
    free ((void *) c->reallocx), c->reallocx = NULL;
  if (c->reallocy != NULL)
    free ((void *) c->reallocy), c->reallocy = NULL;
}
static void
free_context (struct filter *f)
{
  zoom_context *c;
  c = getzcontext (f);
  free_tables (f);
  free ((void *) c);
  f->data = NULL;
}

static zoom_context *
make_context (void)
{
  zoom_context *new_ctxt;

  new_ctxt = (zoom_context *) calloc (1, sizeof (zoom_context));
  if (new_ctxt == NULL)
    return NULL;
  new_ctxt->forversion = -1;
  new_ctxt->newcalc = 1;
  new_ctxt->reallocx = NULL;
  new_ctxt->reallocy = NULL;
  new_ctxt->xpos = NULL;
  new_ctxt->ypos = NULL;
  new_ctxt->uncomplette = 0;
  return (new_ctxt);
}
static void
startbgmkrealloc (void /*@unused@ */ *data,
		  struct taskinfo /*@unused@ */ *task, int /*@unused@ */ r1,
		  int /*@unused@ */ r2)
{
  mkrealloc_table (czoomc.ypos, czoomc.reallocy, (unsigned int) cimage.height,
		   cfractalc.rs.ni, cfractalc.rs.mi, cursymetry.ysym,
		   tmpdata1);
}
static int
do_fractal (struct filter *f, int flags, int /*@unused@ */ time)
{
  number_t *posptr;
  int maxres;
  int size;
  int rflags = 0;
  realloc_t *r, *rend;

  f->image->flip (f->image);
  cfilter = *f;
  set_fractalc (f->fractalc, f->image);

  if (getzcontext (f)->forversion != f->fractalc->version ||
      getzcontext (f)->newcalc ||
      getzcontext (f)->forpversion != f->image->palette->version)
    {
      clear_image (f->image);
      free_tables (f);
      if (!alloc_tables (f))
	return 0;
      init_tables (f);
      getzcontext (f)->newcalc = 0;
      getzcontext (f)->forversion = getfcontext (f)->version;
      getzcontext (f)->forpversion = f->image->palette->version;
      czoomc = *getzcontext (f);
      if (BTRACEOK && !(flags & INTERRUPTIBLE))
	{
	  boundarytraceall (czoomc.xpos, czoomc.ypos);
	  f->flags &= ~ZOOMMASK;
	  return CHANGED | (cfilter.interrupt ? UNCOMPLETTE : 0);
	}
    }
  else
    rflags |= INEXACT;

  czoomc = *getzcontext (f);

  setuncomplette (0);
  setchanged (0);

  maxres = cimage.width;
  if (maxres < cimage.height)
    maxres = cimage.height;
  size =
    ALIGN ((maxres) * (DSIZE + 1) * (int) sizeof (struct dyn_data)) +
    2 * ALIGN (maxres * (int) sizeof (struct dyn_data **)) +
    ALIGN ((maxres + 2) * (int) sizeof (int));
#ifdef HAVE_ALLOCA
  tmpdata = (unsigned char *) alloca (size);
#else
  tmpdata = (unsigned char *) malloc (size);
#endif
  if (tmpdata == NULL)
    {
      x_error
	("XaoS fatal error:Could not allocate memory for temporary data of size %i. "
	 "I am unable to handle this problem so please resize to smaller window.",
	 size);
      return 0;
    }
  if (nthreads != 1)
    {
#ifdef HAVE_ALLOCA
      tmpdata1 = (unsigned char *) alloca (size);
#else
      tmpdata1 = (unsigned char *) malloc (size);
#endif
      if (tmpdata1 == NULL)
	{
	  x_error
	    ("XaoS fatal error:Could not allocate memory for temporary data of size %i. "
	     "I am unable to handle this problem so please resize to smaller window",
	     size);
	  return 0;
	}
    }
  else
    tmpdata1 = tmpdata;

  cfilter.incalculation = 0;
  cfilter.readyforinterrupt = 0;
  cfilter.interrupt = 0;

  nsymetrized = 0;
  cfilter.max = 0;
  cfilter.pos = 0;
  cfilter.pass = "Making y realloc table";
  xth_bgjob (startbgmkrealloc, NULL);

  cfilter.pass = "Making x realloc table";
  mkrealloc_table (czoomc.xpos, czoomc.reallocx, (unsigned int) cimage.width,
		   cfractalc.rs.nc, cfractalc.rs.mc, cursymetry.xsym,
		   tmpdata);

  callwait ();

  cfilter.pass = "Moving old points";
  callwait ();
  preparemoveoldpoints ();
  xth_sync ();
#ifdef _NEVER_
  {
    static long long sum2, sum;
    cli ();
    startagi ();
    sum -= rdtsc ();
    sum2 -= countagi ();
    xth_function (moveoldpoints, NULL, cimage.height);
    sum += rdtsc ();
    sum2 += countagi ();
    sti ();
    printf ("%i %i\n", (int) sum, (int) sum2);
  }
#else
  xth_function (moveoldpoints, NULL, cimage.height);
#endif

  cfilter.pass = "Starting calculation";
  callwait ();
  xth_sync ();
  if (flags & INTERRUPTIBLE)
    calculatenewinterruptible ();
  else
    {
      xth_function (calculatenew, NULL, 1);
      if (cfilter.interrupt)
	{
	  getzcontext (f)->uncomplette = 1;
	}
      cfilter.pos = 0;
      cfilter.max = 0;
      cfilter.pass = "Procesing symetries";
      callwait ();
      xth_sync ();
      if (nsymetrized)
	{
	  xth_function (dosymetry, NULL, cimage.height);
	  xth_sync ();
	  drivercall (cimage,
		      xth_function (dosymetry2_8, NULL, cimage.width),
		      xth_function (dosymetry2_16, NULL, cimage.width),
		      xth_function (dosymetry2_24, NULL, cimage.width),
		      xth_function (dosymetry2_32, NULL, cimage.width));
	  xth_sync ();
	}
      if (getzcontext (f)->uncomplette)
	{
	  fill ();
	}
    }
  for (r = czoomc.reallocx, posptr = czoomc.xpos, rend =
       czoomc.reallocx + cimage.width; r < rend; r++, posptr++)
    {
      *posptr = r->possition;
    }
  for (r = czoomc.reallocy, posptr = czoomc.ypos, rend =
       czoomc.reallocy + cimage.height; r < rend; r++, posptr++)
    {
      *posptr = r->possition;
    }
#ifdef STATISTICS
  STAT (printf ("Statistics: frames %i\n"
		"mkrealloctable: added %i, symetry %i\n"
		"calculate loop: tocalculate %i avoided %i\n"
		"calculate:calculated %i inside %i\n"
		"iters inside:%i iters outside:%i periodicty:%i\n", frames2,
		nadded2, nsymetry2, tocalculate2, avoided2, ncalculated2,
		ninside2, niter2, niter1, nperi));
#endif
  f->flags &= ~ZOOMMASK;
  if (getzcontext (f)->uncomplette)
    rflags |= UNCOMPLETTE, f->flags |= UNCOMPLETTE;
  if (getzcontext (f)->uncomplette > (cimage.width + cimage.height) / 2)
    f->flags |= LOWQUALITY;
  if (getzcontext (f)->changed)
    rflags |= CHANGED;
#ifndef HAVE_ALLOCA
  free (tmpdata);
  if (nthreads != 1)
    free (tmpdata1);
#endif
  return rflags;
}


static struct filter *
getinstance (CONST struct filteraction *a)
{
  struct filter *f = createfilter (a);
  f->data = make_context ();
  f->name = "Zooming engine";
  return (f);
}
static void
destroyinstance (struct filter *f)
{
  free_context (f);
  free (f);
}
static int
requirement (struct filter *f, struct requirements *r)
{
  r->nimages = 2;
  r->supportedmask =
    C256 | TRUECOLOR | TRUECOLOR24 | TRUECOLOR16 | LARGEITER | SMALLITER |
    GRAYSCALE;
  r->flags = IMAGEDATA | TOUCHIMAGE;
  return (f->next->action->requirement (f->next, r));
}

static int
initialize (struct filter *f, struct initdata *i)
{
#ifdef USE_MULTABLE
  if (!multable[0])
    {
      int i;
      mulmid = multable + RANGE * FPMUL;
      for (i = -RANGE * FPMUL; i < RANGE * FPMUL; i++)
	mulmid[i] = i * i;
    }
#endif
  inhermisc (f, i);
  if (i->image != f->image || datalost (f, i))
    getzcontext (f)->forversion = -1, f->image = i->image;
  f->imageversion = i->image->version;
  return (1);
}

CONST struct filteraction zoom_filter = {
  "XaoS's zooming engine",
  "zoom",
  0,
  getinstance,
  destroyinstance,
  do_fractal,
  requirement,
  initialize,
  convertupgeneric,
  convertdowngeneric,
  NULL,
};
