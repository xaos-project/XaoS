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
#ifdef __EMX__
#include <float.h>
#include <sys/cdefs.h>
#endif
#ifdef _plan9_
#include <u.h>
#include <libc.h>
#ifndef NULL
#define NULL (void *)0
#endif
#else
#include <stdio.h>
#include <aconfig.h>
#include <math.h>
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#include <config.h>
#endif
#include <fconfig.h>
#include <plane.h>
#include <complex.h>
CONST char *CONST planename[] = {
  "mu",
  "1/mu",
  "1/(mu+0.25)",
  "lambda",
  "1/lambda",
  "1/(lambda-1)",
  "1/(mu-1.40115)",
  NULL
};


void
recalculate (int plane, number_t * x1, number_t * y1)
{
  number_t x = *x1, y = *y1;
  switch (plane)
    {
    case 1:
      {				/* 1/mu */
	number_t t;
	if (myabs (x) + myabs (y) < 0.000001)
	  t = INT_MAX, y = INT_MAX;
	else
	  {

	    c_div (1, 0, x, y, t, y);
	  }
	x = t;
      }
      break;
    case 2:
      {				/* 1/(mu + 0.25) */
	number_t t;
	if (myabs (x) + myabs (y) < 0.000001)
	  t = INT_MAX, y = INT_MAX;
	else
	  {

	    c_div (1, 0, x, y, t, y);
	  }
	x = t;
	x += 0.25;
      }
      break;
    case 3:			/* lambda */
      {
	number_t tr, ti, mr, mi;

	mr = x, mi = y;
	c_pow2 (x, y, tr, ti);
	c_div (tr, ti, 4, 0, x, y);
	c_div (mr, mi, 2, 0, tr, ti);
	c_sub (tr, ti, x, y, mr, mi);
	x = mr, y = mi;
      }
      break;
    case 4:			/* 1/lambda */
      {
	number_t tr, ti, mr, mi;

	c_div (1, 0, x, y, tr, y);
	x = tr;
	mr = x, mi = y;
	c_pow2 (x, y, tr, ti);
	c_div (tr, ti, 4, 0, x, y);
	c_div (mr, mi, 2, 0, tr, ti);
	c_sub (tr, ti, x, y, mr, mi);
	x = mr, y = mi;
      }
      break;
    case 5:			/* 1/(lambda-1) */
      {
	number_t tr, ti, mr, mi;

	c_div (1, 0, x, y, tr, y);
	x = tr + 1;
	mr = x, mi = y;
	c_pow2 (x, y, tr, ti);
	c_div (tr, ti, 4, 0, x, y);
	c_div (mr, mi, 2, 0, tr, ti);
	c_sub (tr, ti, x, y, mr, mi);
	x = mr, y = mi;
      }
      break;
    case 6:
      {				/* 1/(mu + 0.25) */
	number_t t;
	if (myabs (x) + myabs (y) < 0.000001)
	  t = INT_MAX, y = INT_MAX;
	else
	  {

	    c_div (1, 0, x, y, t, y);
	  }
	x = t;
	x -= 1.40115;
      }
      break;
    default:
      break;
    }
  *x1 = x;
  *y1 = y;
}
