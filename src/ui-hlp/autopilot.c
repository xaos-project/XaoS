
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
#ifdef _plan9_
#include <u.h>
#include <libc.h>
#else
#ifdef __MINGW32__
#include <float.h>
#define isnan _isnan
#endif /* __MINGW32__ */
#include <stdlib.h>
#include <math.h>
#include <config.h>
#include <assert.h>
#include <limits.h>
#endif
#include <fconfig.h>
#define SLARGEITER
#include <filter.h>
#include <zoom.h>
#include "autopilot.h"
#include <ui_helper.h>
#define MINCOUNT 5
#define InSet(i) (i==context->image->palette->pixels[0])
/*Include bitmap depended part first */

#include <c256.h>
#define look1 look18
#define look2 look28
#include "autod.c"

#include <hicolor.h>
#define look1 look116
#define look2 look216
#include "autod.c"

#include <true24.h>
#define look1 look124
#define look2 look224
#include "autod.c"

#include <truecolor.h>
#define look1 look132
#define look2 look232
#include "autod.c"

void
clean_autopilot (uih_context * context)
{
  context->minsize = 1000;
  context->maxsize = 0;
  context->autime = 0;
  context->minlong = 0;
  context->x1 = INT_MAX;
  context->y1 = INT_MAX;
  context->autopilotversion = context->fcontext->version;
}
static void
again (uih_context * context)
{
  context->fcontext->s = context->fcontext->currentformula->v;
  context->fcontext->version++;
  clean_autopilot (context);
}

void
do_autopilot (uih_context * context, int *x, int *y, int *controls,
	      void (*changed) (void), int times)
{
  int c = 0;
  volatile number_t step =
    (context->fcontext->rs.mc -
     context->fcontext->rs.nc) / context->zengine->image->width / 10;
  volatile number_t pos = context->fcontext->rs.mc;
  volatile number_t pos1 = context->fcontext->rs.mc;
  volatile number_t ystep =
    (context->fcontext->rs.mi -
     context->fcontext->rs.ni) / context->zengine->image->height / 10;
  volatile number_t ypos = context->fcontext->rs.mi;
  volatile number_t ypos1 = context->fcontext->rs.mi;
  pos += step;			/*out of precisity check */
  ypos += ystep;
  pos1 -= step;			/*out of precisity check */
  ypos1 -= ystep;
  *x = context->x1;
  *y = context->y1;
  uih_clearwindows (context);
  context->zengine->action->convertup (context->zengine, x, y);
  if ((context->minlong > MINCOUNT && context->c1 == BUTTON3) ||
      !(pos > context->fcontext->rs.mc) ||
      !(ypos > context->fcontext->rs.mi) ||
      (pos1 >= context->fcontext->rs.mc) ||
      (ypos1 >= context->fcontext->rs.mi) ||
      context->fcontext->rs.mc - context->fcontext->rs.nc > 100.0 ||
      isnan (pos) || isnan (ypos) || isnan (context->fcontext->s.cr) ||
      isnan (context->fcontext->s.ci) ||
      isnan (context->fcontext->s.rr - context->fcontext->s.ri) ||
      context->fcontext->s.rr == 0 ||
      context->fcontext->s.ri == 0 ||
      isnan (context->fcontext->rs.mc - context->fcontext->rs.mi) ||
      isnan (context->fcontext->rs.nc - context->fcontext->rs.ni))
    {
      again (context);
      changed ();
    }
  /*Are we waiting for better qualitty? */
  if (!context->c1 && context->zengine->flags & UNCOMPLETTE)
    {
      return;
    }
  assert (changed != NULL);
  if (context->fcontext->version != context->autopilotversion)
    clean_autopilot (context);
  if (context->fcontext->rs.mc - context->fcontext->rs.nc < context->minsize)
    {
      context->minsize = context->fcontext->rs.mc - context->fcontext->rs.nc;
      context->minlong = 0;
    }				/*Oscilating prevention */
  if (context->fcontext->rs.mc - context->fcontext->rs.nc > context->maxsize)
    {
      context->minsize = context->fcontext->rs.mc - context->fcontext->rs.nc;
      context->maxsize = context->fcontext->rs.mc - context->fcontext->rs.nc;
      context->minlong = 0;
    }
  if (context->autime <= 0)
    {
      context->minlong++;
      context->autime = rand () % MAXTIME;
      if (context->zengine->flags & LOWQUALITY)
	{
	  context->c1 = 0;
	}
      else
	{
	  switch (context->zengine->image->bytesperpixel)
	    {
	    case 1:
	      c = look18 (context, *x, *y, RANGE1, NGUESSES);
	      if (!c)
		c = look28 (context, *x, *y, RANGE1, NGUESSES);
	      if (!(rand () % 30))
		c = 0;
	      if (!c)
		c = look18 (context, *x, *y, 10000, NGUESSES1);
	      if (!c)
		c = look18 (context, *x, *y, 10000, NGUESSES2);
	      break;
#ifdef SUPPORT16
	    case 2:
	      c = look116 (context, *x, *y, RANGE1, NGUESSES);
	      if (!c)
		c = look216 (context, *x, *y, RANGE1, NGUESSES);
	      if (!(rand () % 30))
		c = 0;
	      if (!c)
		c = look116 (context, *x, *y, 10000, NGUESSES1);
	      if (!c)
		c = look216 (context, *x, *y, 10000, NGUESSES1);
	      break;
#endif
#ifdef STRUECOLOR24
	    case 3:
	      c = look124 (context, *x, *y, RANGE1, NGUESSES);
	      if (!c)
		c = look224 (context, *x, *y, RANGE1, NGUESSES);
	      if (!(rand () % 30))
		c = 0;
	      if (!c)
		c = look124 (context, *x, *y, 10000, NGUESSES1);
	      if (!c)
		c = look224 (context, *x, *y, 10000, NGUESSES1);
	      break;
#endif
	    case 4:
	      c = look132 (context, *x, *y, RANGE1, NGUESSES);
	      if (!c)
		c = look232 (context, *x, *y, RANGE1, NGUESSES);
	      if (!(rand () % 30))
		c = 0;
	      if (!c)
		c = look132 (context, *x, *y, 10000, NGUESSES1);
	      if (!c)
		c = look232 (context, *x, *y, 10000, NGUESSES1);
	    }
	  if (!c)
	    {
	      if ((context->zengine->flags & UNCOMPLETTE))
		{
		  context->c1 = 0;
		}
	      else
		context->c1 = BUTTON3, context->autime >>= 1;
	    }
	}
    }
  context->autime -= times;
  *x = context->x1;
  *y = context->y1;
  context->zengine->action->convertup (context->zengine, x, y);
/*    printf("%i %i\n",*x,*y); */
  *controls = context->c1;
}
