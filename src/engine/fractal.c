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
/*#define STATISTICS */
#include <aconfig.h>
#include <string.h>
#include <config.h>
#include <fconfig.h>
#ifdef _plan9_
#include <u.h>
#include <libc.h>
#include <stdio.h>
#else
#include <stdio.h>
#ifndef _MAC
#ifndef NO_MALLOC_H
#include <malloc.h>
#endif
#endif
#ifndef _plan9_
#include <limits.h>
#include <assert.h>
#include <math.h>
#endif
#endif
#ifdef __EMX__
#include <float.h>
#include <sys/cdefs.h>
#endif
#include <filter.h>
#include <complex.h>
#include <plane.h>
#include "../include/timers.h"
#ifdef __GNUC__
#ifdef __i386__
#ifndef PC_64
#include <i386/ctrl87.h>
#endif
#endif
#endif
#ifdef __alpha__
#ifdef __linux__
#include <asm/fpu.h>
#endif
#endif
#ifndef M_PI
#define M_PI 3.1415
#endif
#include <xerror.h>

struct symetryinfo2 cursymetry;
struct palette cpalette;
struct image cimage;
struct fractal_context cfractalc;
struct formula cformula;

static symetry2 sym_lines[100];


static void
precalculate_rotation (fractal_context * c)
{
  c->sin = sin ((c->angle) * M_PI / 180);
  c->cos = cos ((c->angle) * M_PI / 180);
}
static void
recalc_view (fractal_context * c)
{
  number_t
    xs = c->s.rr, ys = c->s.ri * c->windowwidth / c->windowheight,
    xc = c->s.cr, yc = c->s.ci, size;
  precalculate_rotation (c);
  rotate (*c, xc, yc);
  /*assert(c->s.rr >= 0);
     assert(c->s.ri >= 0); */

  xs = myabs (xs);		/*do not crash in owerflowing cases */
  ys = myabs (ys);
  if (xs > ys)
    size = xs;
  else
    size = ys;
  c->rs.nc = xc - size / 2;
  c->rs.mc = xc + size / 2;
  c->rs.ni = yc - size * c->windowheight / c->windowwidth / 2;
  c->rs.mi = yc + size * c->windowheight / c->windowwidth / 2;
  if (c->rs.nc > c->rs.mc)
    xc = c->rs.nc, c->rs.nc = c->rs.mc, c->rs.mc = xc;
  if (c->rs.ni > c->rs.mi)
    xc = c->rs.ni, c->rs.ni = c->rs.mi, c->rs.mi = xc;
}
static void
set_view (fractal_context * c, CONST vinfo * s)
{
  c->s = *s;
  recalc_view (c);
}

/*FIXME most of this code is obsolette */
static void			/*INLINE */
combine_methods (void)
{
#ifdef __UNDEFINED__
  int i, j;
#endif
  int angle = (int) cfractalc.angle;
  CONST struct symetryinfo *s1 =
    cfractalc.currentformula->out + cfractalc.coloringmode, *s2 =
    cfractalc.currentformula->in + cfractalc.incoloringmode;
  if (angle < 0)
    {
      angle = 360 - ((-angle) % 360);
    }
  else
    angle %= 360;
  if (cfractalc.mandelbrot != cfractalc.currentformula->mandelbrot ||
      cfractalc.bre || cfractalc.bim)
    {
      cursymetry.xsym = (number_t) INT_MAX;
      cursymetry.ysym = (number_t) INT_MAX;
      cursymetry.nsymetries = 0;
      return;
    }
#ifdef __UNDEFINED__
  cursymetry.xmul = cimage.width / (cfractalc.rs.mc - cfractalc.rs.nc);
  cursymetry.ymul = cimage.height / (cfractalc.rs.mi - cfractalc.rs.ni);
  cursymetry.xdist = (cfractalc.rs.mc - cfractalc.rs.nc) / cimage.width / 6;
  cursymetry.ydist = (cfractalc.rs.mi - cfractalc.rs.ni) / cimage.height / 6;
#endif
  if (s1->xsym == s2->xsym)
    cursymetry.xsym = s1->xsym;
  else
    cursymetry.xsym = (number_t) INT_MAX;
  if (s1->ysym == s2->ysym)
    cursymetry.ysym = s1->ysym;
  else
    cursymetry.ysym = (number_t) INT_MAX;
  switch (cfractalc.plane)
    {
    case P_PARABOL:
      cursymetry.xsym = (number_t) INT_MAX;
      break;
    case P_LAMBDA:
      if (cursymetry.xsym == 0 && cursymetry.ysym == 0)
	cursymetry.xsym = (number_t) 1;
      else
	cursymetry.xsym = (number_t) INT_MAX;
      break;
    case P_INVLAMBDA:
      cursymetry.xsym = (number_t) INT_MAX;
      break;
    case P_TRANLAMBDA:
      if (cursymetry.xsym != 0 || cursymetry.ysym != 0)
	cursymetry.xsym = (number_t) INT_MAX;
      break;
    case P_MEREBERG:
      cursymetry.xsym = (number_t) INT_MAX;
      break;
    }
  cursymetry.symetry = sym_lines;
  cursymetry.nsymetries = 0;
  if ((number_t) angle == cfractalc.angle)
    {
      switch (angle)
	{
	case 0:
	  break;
	case 180:
	  cursymetry.xsym = -cursymetry.xsym;
	  cursymetry.ysym = -cursymetry.ysym;
	  break;
	case 90:
	  {
	    number_t tmp = cursymetry.xsym;
	    cursymetry.xsym = -cursymetry.ysym;
	    cursymetry.ysym = tmp;
	  }
	  break;
	case 210:
	  {
	    number_t tmp = cursymetry.xsym;
	    cursymetry.xsym = cursymetry.ysym;
	    cursymetry.ysym = -tmp;
	  }
	  break;
	default:
	  cursymetry.xsym = (number_t) INT_MAX;
	  cursymetry.ysym = (number_t) INT_MAX;
	}
    }
  else
    {
      cursymetry.xsym = (number_t) INT_MAX;
      cursymetry.ysym = (number_t) INT_MAX;
    }
  if (cursymetry.xsym == -(number_t) INT_MAX)
    cursymetry.xsym = (number_t) INT_MAX;
  if (cursymetry.ysym == -(number_t) INT_MAX)
    cursymetry.ysym = (number_t) INT_MAX;
}

void
update_view (fractal_context * context)
{
  set_view (context, &context->s);
}

void
set_fractalc (fractal_context * context, struct image *img)
{
  update_view (context);
  precalculate_rotation (context);
  cfractalc = *context;		/*its better to copy often accesed data into fixed memory locations */
  cpalette = *img->palette;
  cimage = *img;
  cformula = *context->currentformula;

  if (cfractalc.maxiter < 1)
    cfractalc.maxiter = 1;

  if (cfractalc.bailout < 0)
    cfractalc.bailout = 0;

  if (cfractalc.periodicity)
    {
      if (!cformula.hasperiodicity || cfractalc.incoloringmode
	  || !cfractalc.mandelbrot)
	cfractalc.periodicity = 0;
      else if (!cfractalc.plane)
	cfractalc.periodicity_limit =
	  (context->rs.mc - context->rs.nc) / (double) img->width;
      else
	{
	  int x, y;
	  number_t xstep =
	    ((context->rs.mc - context->rs.nc) / (double) img->width);
	  number_t ystep =
	    ((context->rs.mc - context->rs.nc) / (double) img->height);
	  number_t xstep2 = ((context->rs.mc - context->rs.nc) / 5);
	  number_t ystep2 = ((context->rs.mc - context->rs.nc) / 5);

	  for (x = 0; x < 5; x++)
	    for (y = 0; y < 5; y++)
	      {
		number_t x1 = context->rs.mc + xstep2 * x;
		number_t y1 = context->rs.mi + ystep2 * y;
		number_t x2 = context->rs.mc + xstep2 * x + xstep;
		number_t y2 = context->rs.mi + ystep2 * y + ystep;

		recalculate (cfractalc.plane, &x1, &y1);
		recalculate (cfractalc.plane, &x2, &y2);

		x1 = myabs (x2 - x1);
		y1 = myabs (y2 - y1);

		if (x == y && x == 0)
		  cfractalc.periodicity_limit = x1;
		if (cfractalc.periodicity > x1)
		  cfractalc.periodicity_limit = x1;
		if (cfractalc.periodicity > y1)
		  cfractalc.periodicity_limit = y1;
	      }
	}
    }

  combine_methods ();

  if (cursymetry.xsym == (number_t) INT_MAX)
    cursymetry.xsym = cfractalc.rs.mc + INT_MAX;

  if (cursymetry.ysym == (number_t) INT_MAX)
    cursymetry.ysym = cfractalc.rs.mi + INT_MAX;

#ifndef SLOWFUNCPTR
  if (cfractalc.coloringmode == 9 && cformula.smooth_calculate != NULL
      && (cpalette.type & (TRUECOLOR | TRUECOLOR16 | TRUECOLOR24 | GRAYSCALE |
			   LARGEITER)))
    {
      cfractalc.calculate[0] = cformula.smooth_calculate;
      if (cformula.smooth_calculate_periodicity && cfractalc.periodicity)
	cfractalc.calculate[1] = cformula.smooth_calculate_periodicity;
      else
	cfractalc.calculate[1] = cformula.smooth_calculate;
    }
  else
    {
      cfractalc.calculate[0] = cformula.calculate;
      if (cformula.calculate_periodicity && cfractalc.periodicity)
	cfractalc.calculate[1] = cformula.calculate_periodicity;
      else
	cfractalc.calculate[1] = cformula.calculate;
    }
#endif

}




void
set_formula (fractal_context * c, int num)
{
  assert (num < nformulas);
  assert (num >= 0);
  if (num >= nformulas)
    num = 0;
  if (c->currentformula != formulas + num)
    {
      c->currentformula = formulas + num;
      c->version++;
    }
  if (c->mandelbrot != c->currentformula->mandelbrot)
    {
      c->mandelbrot = c->currentformula->mandelbrot;
      c->version++;
    }
  if (c->currentformula->pre != c->pre)
    {
      c->pre = c->currentformula->pre;
      if (!c->mandelbrot)
	c->version++;
    }
  if (c->currentformula->pim != c->pim)
    {
      c->pim = c->currentformula->pim;
      if (!c->mandelbrot)
	c->version++;
    }
  if (c->angle)
    {
      c->angle = 0;
      c->version++;
    }
  if (c->s.cr != c->currentformula->v.cr ||
      c->s.ci != c->currentformula->v.ci ||
      c->s.rr != c->currentformula->v.rr ||
      c->s.ri != c->currentformula->v.ri)
    {
      c->s = c->currentformula->v;
      c->version++;
    }
  if (c->bre && c->bim)
    {
      c->bre = c->bim = 0;
      if (c->mandelbrot)
	c->version++;
    }
}


void
fractalc_resize_to (fractal_context * c, float wi, float he)
{
  c->windowwidth = wi;
  c->windowheight = he;
  recalc_view (c);
  return;
}



fractal_context *
make_fractalc (CONST int formula, float wi, float he)
{
  fractal_context *new_ctxt;

#ifndef __BEOS__
#ifdef __GNUC__
#ifdef __i386__
#ifndef NOASSEMBLY
  _control87 (PC_64 | MCW_EM | MCW_RC, MCW_PC | MCW_EM | MCW_RC);
#endif
#endif
#endif
#endif
#ifdef __alpha__
#ifdef __linux__
  extern void ieee_set_fp_control (unsigned long);
  /* ieee_set_fp_control(IEEE_TRAP_ENABLE_INV); */
  ieee_set_fp_control (0UL);	/* ignore everything possible */
#endif
#endif
#ifdef _plan9_
  {
    unsigned long fcr = 0;	/*getfcr(); */
    fcr |= FPRNR | FPPEXT;
    /*fcr &= ~(FPINEX | FPOVFL | FPUNFL | FPZDIV); */
    setfcr (fcr);
  }
#endif
  new_ctxt = (fractal_context *) calloc (1, sizeof (fractal_context));
  if (new_ctxt == NULL)
    return 0;
  new_ctxt->windowwidth = wi;
  new_ctxt->periodicity = 1;
  new_ctxt->windowheight = he;
  new_ctxt->maxiter = DEFAULT_MAX_ITER;
  new_ctxt->bailout = DEFAULT_BAILOUT;
  new_ctxt->coloringmode = 0;
  new_ctxt->intcolor = 1;
  new_ctxt->outtcolor = 1;
  new_ctxt->slowmode = 0;
  new_ctxt->range = 3;
  new_ctxt->angle = 0;
  set_formula (new_ctxt, formula);
  return (new_ctxt);
}

void
free_fractalc (fractal_context * c)
{
  free (c);
}

#ifdef NOASSEMBLY
#define rdtsc() 0
#else
#define rdtsc() ({unsigned long time; asm __volatile__ ("rdtsc":"=a"(time)); time; })
#endif

void
speed_test (fractal_context * c, struct image *img)
{
  unsigned int sum;
  tl_timer *t;
  int time;
  unsigned int i;
  set_fractalc (c, img);
  t = tl_create_timer ();
  cfractalc.maxiter = 100;
#ifdef SLOWFUNCPTR
  i = calculateswitch (0.0, 0.0, 0.0, 0.0, 0);
#else
  cfractalc.currentformula->calculate (0.0, 0.0, 0.0, 0.0);
  if (cfractalc.currentformula->calculate_periodicity != NULL)
    cfractalc.currentformula->calculate_periodicity (0.0, 0.0, 0.0, 0.0);
  if (cfractalc.currentformula->smooth_calculate != NULL)
    cfractalc.currentformula->smooth_calculate (0.0, 0.0, 0.0, 0.0);
  if (cfractalc.currentformula->smooth_calculate_periodicity != NULL)
    cfractalc.currentformula->smooth_calculate_periodicity (0.0, 0.0, 0.0,
							    0.0);
#endif
  cfractalc.maxiter = 20000000;

  tl_update_time ();
  tl_reset_timer (t);
  /*sum = rdtsc (); */
#ifdef SLOWFUNCPTR
  i = calculateswitch (0.0, 0.0, 0.0, 0.0, 0);
#else
  i = cfractalc.currentformula->calculate (0.0, 0.0, 0.0, 0.0);
#endif
  /*sum -= rdtsc ();
     printf ("%f\n", (double) (-sum) / cfractalc.maxiter); */
  tl_update_time ();
  time = tl_lookup_timer (t);
  x_message ("Result:%i Formulaname:%s Time:%i Mloops per sec:%.2f",
	     (int) i,
	     cfractalc.currentformula->name[0], time,
	     cfractalc.maxiter / (double) time);

#ifndef SLOWFUNCPTR


  if (cfractalc.currentformula->smooth_calculate != NULL)
    {
      tl_update_time ();
      tl_reset_timer (t);
      i = cfractalc.currentformula->smooth_calculate (0.0, 0.0, 0.0, 0.0);
      tl_update_time ();
      time = tl_lookup_timer (t);
      x_message ("Result:%i Formulaname:%s Time:%i Mloops per sec:%.2f",
		 (int) i,
		 cfractalc.currentformula->name[0],
		 time, cfractalc.maxiter / (double) time);
    }

#endif

  tl_free_timer (t);
}
