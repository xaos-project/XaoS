
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
#include <stdio.h>
#else
/* Hello reader!
 * code you are looking at is dangerous for both you and your hardware! PLEASE
 * CLOSE THIS FILE UNLESS YOU REALY KNOW WHAT YOU ARE DOING.
 *
 * Main purpose of this file is to generate optimal caluclation loops for
 * various formulas/algorithms. It heavily includes docalc.c - set of
 * caluclation loops, that then uses macros instad of formulas. This lets me
 * to change calculation loops easily. At the other hand it looks very ugly.
 * You have been warned :)
 */

#ifndef _MAC
#include <aconfig.h>
#endif

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <math.h>
#ifdef __EMX__
#include <float.h>
#include <sys/cdefs.h>
#endif
#include <stdio.h>
#endif /*plan9 */
#include <archaccel.h>
#include <config.h>
#include <complex.h>
#include <filter.h>
#include <fractal.h>
#include "julia.h"
#ifndef M_PI
#define M_PI 3.1415
#endif

#ifdef SLOWFUNCPTR
#define FUNCTYPE INLINE
#else
#define FUNCTYPE
#endif


CONST char *CONST incolorname[] = {
  "0",
  "zmag",
  "Decomposition-like",
  "real/imag",
  "abs(abs(c)-abs(r))",
  "cos(mag)",
  "mag*cos(real^2)",
  "sin(real^2-imag^2)",
  "atan(real*imag*creal*cimag)",
  "squares",
  "True-color",
  NULL
};

CONST char *CONST outcolorname[] = {
  "iter",
  "iter+real",
  "iter+imag",
  "iter+real/imag",
  "iter+real+imag+real/imag",
  "binary decomposition",
  "biomorphs",
  "potential",
  "color decomposition",
  "smooth",
  "True-color",
  NULL
};
CONST char *CONST tcolorname[] = {
  "black",
  "re*im sin(re^2) angle",
  "sin(re) sin(im) sin(square)",
  "hsv",
  "hsv2",
  "cos(re^c) cos(im^2) cos(square)",
  "abs(re^2) abs(im^2) abs(square)",
  "re*im re*re im*im",
  "abs(im*cim) abs(re*cre) abs(re*cim)",
  "abs(re*im-csqr) abs(re^2-csqr) abs(im^2-csqr)",
  "angle angle2 angle",
  "Disable truecolor colouring",
  NULL
};

#define SHIFT 8
#define SMUL 256

#define __GNUC__EGCS
/* i386 fp comparsions are incredibly slow. We get much better results when we
   do it in integer unit.  This trick works well for numbers>0*/
#ifdef __GNUC__EGCS
#ifdef __i386__121

/* Use union to be alias-analysis correct.  */
typedef union
{
  unsigned int *i;
  float *f;
}
fpint;
#define less_than_4(x) ({float tmp=(x); fpint ptr; ptr.f=&tmp;*ptr.i<0x40800000U;})
#define less_than_0(x) ({float tmp=(x); fpint ptr; ptr.f=&tmp;*ptr.i&0x80000000U;})
#define greater_then_1Em6(x) ({float tmp=(x); fpint ptr; ptr.f=&tmp;*ptr.i>(unsigned int)0x358637bdU;})
#define abs_less_than(x,y) ({float tmp=(x), tmp2=(y); fpint ptr, ptr2; ptr.f=&tmp; ptr2.f=&tmp2;(*ptr.i&~0x80000000U)<*ptr2.i;})
#define greater_than(x,y) ({float tmp=(x), tmp2=(y); fpint ptr, ptr2; ptr.f=&tmp; ; ptr2.f=&tmp2;*ptr.i>*ptr2.i;})
#endif
#endif
#ifndef less_than_4
#define less_than_0(x) ((x)<0)
#define less_than_4(x) ((x)<cfractalc.bailout)
#define greater_then_1Em6(n) ((n)>1E-6)
#define abs_less_than(x,y) (myabs(x)<y)
#define greater_than(x,y) ((x)>(y))
#endif



#define PERIINOUTPUT() STAT(nperi++;ninside2++);return(cpalette.pixels[0])

#define OUTOUTPUT() STAT(niter2+=iter);return(!cfractalc.coloringmode?cpalette.pixels[(iter%(cpalette.size-1))+1]:color_output(zre,zim,iter))
#define INOUTPUT() STAT(niter1+=iter;ninside2++);return(cfractalc.incoloringmode?incolor_output(zre,zim,pre,pim,iter):cpalette.pixels[0])

#define OUTPUT() if(iter>=(unsigned int)cfractalc.maxiter)\
                { \
                  if(cfractalc.incoloringmode==10) return(truecolor_output(zre,zim,pre,pim,cfractalc.intcolor,1)); \
		  INOUTPUT();  \
                } \
		else  { \
                  if(cfractalc.coloringmode==10) return(truecolor_output(zre,zim,pre,pim,cfractalc.outtcolor,0)); \
		  OUTOUTPUT(); \
		}

#define SMOOTHOUTPUT() {PRESMOOTH;zre+=0.000001;szmag+=0.000001; \
                        iter=(int)(((cfractalc.maxiter-iter)*256+log((double)(cfractalc.bailout/(szmag)))/log((double)((zre)/(szmag)))*256)); \
                        if (iter < 0) {\
                          iter = (((unsigned int)(cpalette.size - 1)) << 8) - ((-iter) % (((unsigned int)(cpalette.size - 1)) << 8))-1; \
                          if (iter < 0) iter=0; \
			} \
                        iter %= ((unsigned int)(cpalette.size - 1)) << 8; \
 \
                        if ((cpalette.type & (C256 | SMALLITER)) || !(iter & 255)) \
                          return (cpalette.pixels[1 + (iter >> 8)]); \
                        { \
                          unsigned int i1, i2; \
                          i1 = cpalette.pixels[1 + (iter >> 8)]; \
                          if ((iter >> 8) == (unsigned int)(cpalette.size - 2)) \
                            i2 = cpalette.pixels[1]; \
                          else \
                            i2 = cpalette.pixels[2 + (iter >> 8)]; \
                          iter &= 255; \
                          return (interpoltype (cpalette, i2, i1, iter)); \
                        } \
                      }

static INLINE void
hsv_to_rgb (int h, int s, int v, int *red, int *green, int *blue)
  CONSTF;
     static INLINE void
       hsv_to_rgb (int h, int s, int v, int *red, int *green, int *blue)
{
  int hue;
  int f, p, q, t;

  if (s == 0)
    {
      *red = v;
      *green = v;
      *blue = v;
    }
  else
    {
      h %= 256;
      if (h < 0)
	h += 256;
      hue = h * 6;

      f = hue & 255;
      p = v * (256 - s) / 256;
      q = v * (256 - (s * f) / 256) >> 8;
      t = v * (256 * 256 - (s * (256 - f))) >> 16;

      switch ((int) (hue / 256))
	{
	case 0:
	  *red = v;
	  *green = t;
	  *blue = p;
	  break;
	case 1:
	  *red = q;
	  *green = v;
	  *blue = p;
	  break;
	case 2:
	  *red = p;
	  *green = v;
	  *blue = t;
	  break;
	case 3:
	  *red = p;
	  *green = q;
	  *blue = v;
	  break;
	case 4:
	  *red = t;
	  *green = p;
	  *blue = v;
	  break;
	case 5:
	  *red = v;
	  *green = p;
	  *blue = q;
	  break;
	}
    }
}

static unsigned int
truecolor_output (number_t zre, number_t zim, number_t pre, number_t pim,
		  int mode, int inset)
     CONSTF REGISTERS (3);
     static unsigned int
       truecolor_output (number_t zre, number_t zim, number_t pre,
			 number_t pim, int mode, int inset)
{
  /*WARNING r and b fiends are swapped for HISTORICAL REASONS (BUG :) */
  int r = 0, g = 0, b = 0, w = 0;

  switch (mode)
    {
    case 0:
      break;
    case 1:
      b =
	(int) ((sin ((double) atan2 ((double) zre, (double) zim) * 20) + 1) *
	       127);
      w = (int) ((sin ((double) zim / zre)) * 127);
      r = (int) ((int) (zre * zim));
      g = (int) ((sin ((double) (zre * zre) / 2) + 1) * 127);
      break;
    case 2:
      if (!inset)
	{
	  r = (int) ((sin ((double) zre * 2) + 1) * 127);
	  g = (int) ((sin ((double) zim * 2) + 1) * 127);
	  b = (int) ((sin ((double) (zim * zim + zre * zre) / 2) + 1) * 127);
	}
      else
	{
	  r = (int) ((sin ((double) zre * 50) + 1) * 127);
	  g = (int) ((sin ((double) zim * 50) + 1) * 127);
	  b = (int) ((sin ((double) (zim * zim + zre * zre) * 50) + 1) * 127);
	}
      w = (int) ((sin ((double) zim / zre)) * 127);
      break;
    case 3:
      if (inset)
	hsv_to_rgb ((int) (atan2 ((double) zre, (double) zim) * 256 / M_PI),
		    (int) ((sin ((double) (zre * 50)) + 1) * 128),
		    (int) ((sin ((double) (zim * 50)) + 1) * 128), &r, &g,
		    &b);
      else
	hsv_to_rgb ((int) (atan2 ((double) zre, (double) zim) * 256 / M_PI),
		    (int) ((sin ((double) zre) + 1) * 128),
		    (int) ((sin ((double) zim) + 1) * 128), &r, &g, &b);
      break;
    case 4:
      if (inset)
	hsv_to_rgb ((int)
		    (sin ((double) (zre * zre + zim * zim) * 0.1) * 256),
		    (int) (sin (atan2 ((double) zre, (double) zim) * 10) *
			   128 + 128),
		    (int) ((sin ((double) (zre + zim) * 10)) * 65 + 128), &r,
		    &g, &b);
      else
	hsv_to_rgb ((int)
		    (sin ((double) (zre * zre + zim * zim) * 0.01) * 256),
		    (int) (sin (atan2 ((double) zre, (double) zim) * 10) *
			   128 + 128),
		    (int) ((sin ((double) (zre + zim) * 0.3)) * 65 + 128), &r,
		    &g, &b);
      break;
    case 5:
      {
	if (!inset)
	  {
	    r = (int) (cos ((double) myabs (zre * zre)) * 128) + 128;
	    g = (int) (cos ((double) myabs (zre * zim)) * 128) + 128;
	    b =
	      (int) (cos ((double) myabs (zim * zim + zre * zre)) * 128) +
	      128;
	  }
	else
	  {
	    r = (int) (cos ((double) myabs (zre * zre) * 10) * 128) + 128;
	    g = (int) (cos ((double) myabs (zre * zim) * 10) * 128) + 128;
	    b =
	      (int) (cos ((double) myabs (zim * zim + zre * zre) * 10) *
		     128) + 128;
	  }
      }
      break;
    case 6:
      {
	if (!inset)
	  {
	    r = (int) (zre * zim * 64);
	    g = (int) (zre * zre * 64);
	    b = (int) (zim * zim * 64);
	  }
	else
	  r = (int) (zre * zim * 256);
	g = (int) (zre * zre * 256);
	b = (int) (zim * zim * 256);
      }
      break;
    case 7:
      {
	if (!inset)
	  {
	    r = (int) ((zre * zre + zim * zim - pre * pre - pim * pim) * 16);
	    g = (int) ((zre * zre * 2 - pre * pre - pim * pim) * 16);
	    b = (int) ((zim * zim * 2 - pre * pre - pim * pim) * 16);
	  }
	else
	  {
	    r = (int) ((zre * zre + zim * zim - pre * pre - pim * pim) * 256);
	    g = (int) ((zre * zre * 2 - pre * pre - pim * pim) * 256);
	    b = (int) ((zim * zim * 2 - pre * pre - pim * pim) * 256);
	  }
      }
      break;
    case 8:
      {
	if (!inset)
	  {
	    r = (int) ((myabs (zim * pim)) * 64);
	    g = (int) ((myabs (zre * pre)) * 64);
	    b = (int) ((myabs (zre * pim)) * 64);
	  }
	else
	  {
	    r = (int) ((myabs (zim * pim)) * 256);
	    g = (int) ((myabs (zre * pre)) * 256);
	    b = (int) ((myabs (zre * pim)) * 256);
	  }
      }
      break;
    case 9:
      {
	if (!inset)
	  {
	    r = (int) ((myabs (zre * zim - pre * pre - pim * pim)) * 64);
	    g = (int) ((myabs (zre * zre - pre * pre - pim * pim)) * 64);
	    b = (int) ((myabs (zim * zim - pre * pre - pim * pim)) * 64);
	  }
	else
	  {
	    r = (int) ((myabs (zre * zim - pre * pre - pim * pim)) * 256);
	    g = (int) ((myabs (zre * zre - pre * pre - pim * pim)) * 256);
	    b = (int) ((myabs (zim * zim - pre * pre - pim * pim)) * 256);
	  }
      }
      break;
    case 10:
      {
	r = (int) (atan2 ((double) zre, (double) zim) * 128 / M_PI) + 128;
	g = (int) (atan2 ((double) zre, (double) zim) * 128 / M_PI) + 128;
	b = (int) (atan2 ((double) zim, (double) zre) * 128 / M_PI) + 128;
      }
      break;
    }

  r += w;
  g += w;
  b += w;
  if (r < 0)
    r = 0;
  else if (r > 255)
    r = 255;
  if (g < 0)
    g = 0;
  else if (g > 255)
    g = 255;
  if (b < 0)
    b = 0;
  else if (b > 255)
    b = 255;

  switch (cpalette.type)
    {
    case GRAYSCALE:
      return ((unsigned int) (r * 76 + g * 151 + b * 29) *
	      (cpalette.end - cpalette.start) >> 16) + cpalette.start;
    case TRUECOLOR:
    case TRUECOLOR24:
    case TRUECOLOR16:
      r >>= cpalette.info.truec.bprec;
      g >>= cpalette.info.truec.gprec;
      b >>= cpalette.info.truec.rprec;
      return ((r << cpalette.info.truec.bshift) +
	      (g << cpalette.info.truec.gshift) +
	      (b << cpalette.info.truec.rshift));
    }

  return cpalette.pixels[inset];
}

#ifdef __alpha__
#define __TEST__
#endif
static unsigned int
color_output (number_t zre, number_t zim, unsigned int iter)
     CONSTF REGISTERS (3);
     static unsigned int
       color_output (number_t zre, number_t zim, unsigned int iter)
{
  int i;
  iter <<= SHIFT;
  i = iter;

  switch (cfractalc.coloringmode)
    {
    case 9:
      break;
    case 1:			/* real */
      i = (int) (iter + zre * SMUL);
      break;
    case 2:			/* imag */
      i = (int) (iter + zim * SMUL);
      break;
    case 3:			/* real / imag */
#ifdef __TEST__
      if (zim != 0)
#endif
	i = (int) (iter + (zre / zim) * SMUL);
      break;
    case 4:			/* all of the above */
#ifdef __TEST__
      if (zim != 0)
#endif
	i = (int) (iter + (zre + zim + zre / zim) * SMUL);
      break;
    case 5:
      if (zim > 0)
	i = ((cfractalc.maxiter << SHIFT) - iter);
      break;
    case 6:
      if (myabs (zim) < 2.0 || myabs (zre) < 2.0)
	i = ((cfractalc.maxiter << SHIFT) - iter);
      break;
    case 7:
      zre = zre * zre + zim * zim;
#ifdef __TEST__
      if (zre < 1 || !i)
	i = 0;
      else
#endif
	i = (int) (sqrt (log ((double) zre) / i) * 256 * 256);
      break;
    default:
    case 8:
      i =
	(int) ((atan2 ((double) zre, (double) zim) / (M_PI + M_PI) + 0.75) *
	       20000);
      break;
    }

  if (i < 0)
    {
      i =
	(((unsigned int) (cpalette.size - 1)) << 8) -
	((-i) % (((unsigned int) (cpalette.size - 1) << 8))) - 1;
      if (i < 0)
	i = 0;
    }
  iter = ((unsigned int) i) % ((cpalette.size - 1) << 8);
  if ((cpalette.type & (C256 | SMALLITER)) || !(iter & 255))
    return (cpalette.pixels[1 + (iter >> 8)]);
  {
    unsigned int i1, i2;

    i1 = cpalette.pixels[1 + (iter >> 8)];

    if ((int) (iter >> 8) == cpalette.size - 2)
      i2 = cpalette.pixels[1];
    else
      i2 = cpalette.pixels[2 + (iter >> 8)];

    iter &= 255;
    return (interpoltype (cpalette, i2, i1, iter));
  }

}

static unsigned int
incolor_output (number_t zre, number_t zim, number_t pre, number_t pim,
		unsigned int iter)
     CONSTF REGISTERS (3);
     static unsigned int
       incolor_output (number_t zre, number_t zim, number_t pre, number_t pim,
		       unsigned int iter)
{
  int i = iter;
  switch (cfractalc.incoloringmode)
    {
    case 1:			/* zmag */
      i =
	(int) (((zre * zre + zim * zim) *
		(number_t) (cfractalc.maxiter >> 1) * SMUL + SMUL));
      break;
    case 2:			/* real */
      i =
	(int) (((atan2 ((double) zre, (double) zim) / (M_PI + M_PI) + 0.75) *
		20000));
      break;
    default:
      break;
    case 3:			/* real / imag */
      i = (int) (100 + (zre / zim) * SMUL * 10);
      break;
    case 4:
      zre = myabs (zre);
      zim = myabs (zim);
      pre = myabs (pre);
      pre = myabs (pim);
      i += (int) (myabs (pre - zre) * 256 * 64);
      i += (int) (myabs (pim - zim) * 256 * 64);
      break;
    case 5:
      if (((int) ((zre * zre + zim * zim) * 10)) % 2)
	i = (int) (cos ((double) (zre * zim * pre * pim)) * 256 * 256);
      else
	i = (int) (sin ((double) (zre * zim * pre * pim)) * 256 * 256);
      break;
    case 6:
      i =
	(int) ((zre * zre + zim * zim) * cos ((double) (zre * zre)) * 256 *
	       256);
      break;
    case 7:
      i = (int) (sin ((double) (zre * zre - zim * zim)) * 256 * 256);
      break;
    case 8:
      i = (int) (atan ((double) (zre * zim * pre * pim)) * 256 * 64);
      break;
    case 9:
      if ((abs ((int) (zre * 40)) % 2) ^ (abs ((int) (zim * 40)) % 2))
	i =
	  (int) (((atan2 ((double) zre, (double) zim) / (M_PI + M_PI) + 0.75)
		  * 20000));
      else
	i =
	  (int) (((atan2 ((double) zim, (double) zre) / (M_PI + M_PI) + 0.75)
		  * 20000));
      break;
    };

  if (i < 0)
    {
      i =
	(((unsigned int) (cpalette.size - 1)) << 8) -
	((-i) % (((unsigned int) (cpalette.size - 1) << 8))) - 1;
      if (i < 0)
	i = 0;
    }
  iter = ((unsigned int) i) % ((cpalette.size - 1) << 8);

  if ((cpalette.type & (C256 | SMALLITER)) || !(iter & 255))
    return (cpalette.pixels[1 + ((unsigned int) iter >> 8)]);
  {
    unsigned int i1, i2;
    i1 = cpalette.pixels[1 + ((unsigned int) iter >> 8)];
    if (((unsigned int) iter >> 8) == (unsigned int) (cpalette.size - 2))
      i2 = cpalette.pixels[1];
    else
      i2 = cpalette.pixels[2 + ((unsigned int) iter >> 8)];
    iter &= 255;
    return (interpoltype (cpalette, i2, i1, iter));
  }

}

#define VARIABLES
#define INIT
#define UNCOMPRESS
#define USEHACKS
#define PRETEST 0
#define FORMULA  \
	    zim=(zim*zre)*2+pim; \
	    zre = rp - ip + pre; \
            ip=zim*zim; \
            rp=zre*zre;

#ifdef _NEVER_
#ifdef __GNUC__
#ifdef __i386__
#ifndef NOASSEMBLY
/* The hand optimized internal loops can save extra 9% of CPU speed compared
   to latest GCC snapshot.  */

/* GCC has ugly support for asm statements with fp input/output, so we use
 * memory.  */
#define NSFORMULALOOP(iter) \
{ int tmp; \
asm( \
"movl %%edx, %1\n\t" \
"fldt	%9\n\t" \
"fxch	%%st(2)\n\t" \
"fldt	%8\n\t" \
"fxch	%%st(2)\n\t" \
"fld	%%st(0)\n\t" \
".align 16\n\t" \
"1:\n\t"  \
"fld	%%st(0)\n\t" 		/* zre  zre  zim  pre pim */  \
"fxch	%%st(2)\n\t" 		/* zim  zre  zre ... */  \
"fmul	%%st(0),%%st(2)\n\t" 	/* zim  zre  zim*zre */  \
"movl	%1,%%eax\n\t" \
"fmul	%%st(0),%%st(0)\n\t" 	/* zim*zim  zre  zim*zre */  \
"fxch	%%st(2)\n\t" 		/* zim*zre  zre  zim*zim */  \
"fadd	%%st(0),%%st(0)\n\t" 	/* 2*zre*zim  zre  zim*zim */  \
"fxch	%%st(1)\n\t" 		/* zre  2*zre*zim  zim*zim */  \
"fmul	%%st(0),%%st(0)\n\t" 	/* zre*zre  2*zre*zim  zim*zim */  \
"fxch	%%st(1)\n\t" 		/* 2*zre*zim  zre*zre  zim*zim */  \
"fld	%%st(2)\n\t" 		/* zim*zim  2*zre*zim  zre*zre  zim*zim */  \
"fsub	%%st(4),%%st(0)\n\t" 	/* zim*zim-pre  2*zre*zim  zre*zre  zim*zim */  \
"fxch	%%st(3)\n\t" 		/* zim*zim  2*zre*zim  zre*zre  zim*zim-pre */  \
"fadd	%%st(2),%%st(0)\n\t" 	/* zim*zim+zre*zre  2*zre*zim  zre*zre  zim*zim-pre */ \
"fxch	%%st(1)\n\t" 		/* 2*zre*zim  zim*zim+zre*zre  zre*zre  zim*zim-pre    */  \
"fadd	%%st(5),%%st(0)\n\t" 	/* 2*zre*zim*pim  zim*zim+zre*zre  zre*zre  zim*zim-pre*/ \
"fxch	%%st(3)\n\t" 		/* zim*zim-pre  zim*zim+zre*zre  zre*zre  2*zre*zim+pim */ \
"fsubp	%%st(0),%%st(2)\n\t" 	/* zim*zim+zre*zre  zre*zre-zim*zim+pre  2*zre*zim+pim  */  \
"cmpl	%%edx,%%eax\n\t"  \
"ja	2f\n\t" /* cond branch               */  \
"fstps	%1\n\t" /* aa-bb+r 2ab+i r i         */  \
"decl	%%ecx\n\t" /*                           */  \
"jnz	1b\n\t" /*                           */  \
"fld	%%st(0)\n\t"  \
"2:\n\t"  \
"fstp	%%st(0)\n\t"  \
"fstp	%%st(2)\n\t"  \
"fstp	%%st(2)\n\t"  \
:"=c"(iter),"=m"(tmp),"=&t"(zim),"=&u"(zre) \
:"d"(0x40800000),"0"(iter),"2"(zre),"3"(zim),"m"(pre),"m"(pim) \
:"eax","st(2)","st(3)","st(4)","st(5)"); \
}

pacalc (long double zre, long double zim, long double pre, long double pim)
{
  int iter = 1000000;
  NSFORMULALOOP (iter);
  return iter;
}
#endif
#endif
#endif
#endif

#define BTEST less_than_4(rp+ip)
#define SMOOTH
#define SCALC smand_calc
#define SPERI smand_peri
#define CALC mand_calc
#define PERI mand_peri
#define JULIA mand_julia
#define RANGE 2
#define RPIP
#include "docalc.c"

#ifdef __i386__
#define UNCOMPRESS
#endif
#define USEHACKS
#define PRETEST 0
#define FORMULA \
	rp = zre * (rp - 3 * ip); \
	zim = zim * (3 * zre * zre - ip) + pim; \
	zre = rp + pre; \
	rp = zre * zre; \
	ip = zim * zim;
#define BTEST less_than_4(rp+ip)
#define SMOOTH
#define SCALC smand3_calc
#define SPERI smand3_peri
#define CALC mand3_calc
#define PERI mand3_peri
#define JULIA mand3_julia
#define RANGE 2
#define RPIP
#include "docalc.c"


#define UNCOMPRESS
#define VARIABLES number_t br,tmp;
#define FORMULA \
	br = zre + zre + pre - 2; \
	tmp = zre * zim; \
	zre = rp - ip + pre - 1; \
	ip = zim + zim + pim; \
	zim = tmp + tmp + pim; \
	tmp = 1 / (br * br + ip * ip); \
	rp = (zre * br + zim * ip) * tmp; \
	ip = (zim * br - zre * ip) * tmp; \
	zre = (rp + ip) * (rp - ip); \
	zim = rp * ip; \
	zim += zim; \
	rp = zre - 1; \
	ip = zim * zim; \
	rp = zre * zre;
#define BTEST (rp+ip<(number_t)100*100&&(rp-2*zre+ip)>0.04/cfractalc.bailout-1)
#define POSTCALC \
	if(rp-2*zre+ip>0.04/cfractalc.bailout-1){ \
		zre *= 0.08/cfractalc.bailout, zim *= 0.08/cfractalc.bailout; \
		if(iter) \
			iter = cfractalc.maxiter - iter + 1; \
	}
#define CALC magnet_calc
#define PERI magnet_peri
#define SCALC smagnet_calc
#define SPERI smagnet_peri
#define SMOOTH
#define PRESMOOTH szmag/=100*100/4;zre=(rp+ip)/(100*100*4);
#define JULIA magnet_julia
#define RANGE 4
#define RPIP
#include "docalc.c"

#define UNCOMPRESS
#define VARIABLES number_t inre,inim,tmp1,tmp2,dnre,nmre,dnim;
#define INIT \
	inre = pre*pre - pim*pim - pre - pre - pre; \
	inim = pre*pim; \
	inim = inim + inim - pim - pim - pim;
#define FORMULA \
	tmp1 = rp - ip; \
	tmp2 = zre*pre - zim*pim - zre; \
	dnre = tmp1 + tmp1 + tmp1 + tmp2 + tmp2 + tmp2 - zre - zre - zre + inre + 3; \
	tmp1 = zre*ip;\
	nmre = zre*rp - tmp1 - tmp1 - tmp1 + tmp2 + tmp2 + tmp2 + inre + 2; \
	tmp1 = zre*zim; \
	tmp2 = zre*pim + zim*pre - zim; \
	dnim = tmp1 + tmp1 + tmp1 + tmp1 + tmp1 + tmp1 + tmp2 + tmp2 + tmp2 - zim - zim - zim + inim; \
	tmp1 = zim*rp; \
	zim = tmp1 + tmp1 + tmp1 - zim*ip + tmp2 + tmp2 + tmp2 + inim; \
	zre = nmre; \
	ip = dnim; \
	tmp1 = 1 / (dnre * dnre + ip * ip); \
	rp = (zre * dnre + zim * ip) * tmp1; \
	ip = (zim * dnre - zre * ip) * tmp1; \
	zre = (rp + ip) * (rp - ip); \
	zim = rp * ip; \
	zim += zim; \
	ip = zim * zim; \
	rp = zre * zre;
#define BTEST (rp+ip<(number_t)100*100&&(rp-2*zre+ip)>0.04/cfractalc.bailout-1)
#define POSTCALC \
	if(rp-2*zre+ip>0.04/cfractalc.bailout-1){ \
		zre *= 0.08/cfractalc.bailout, zim *= 0.08/cfractalc.bailout; \
		if(iter) \
			iter = cfractalc.maxiter - iter + 1; \
	}
#define CALC magnet2_calc
#define PERI magnet2_peri
#define SCALC smagnet2_calc
#define SPERI smagnet2_peri
#define SMOOTH
#define PRESMOOTH szmag/=100*100/4;zre=(rp+ip)/(100*100*4);
#define JULIA magnet2_julia
#define RANGE 2
#define RPIP
#include "docalc.c"

#ifdef __i386__
#define UNCOMPRESS
#endif
#define BTEST less_than_4(rp+ip)
#define FORMULA \
	rp = rp * rp - 6 * rp * ip + ip * ip + pre; \
	zim = 4 * zre * zre * zre * zim - 4 * zre * ip * zim + pim; \
	zre = rp; \
	rp = zre * zre; \
	ip = zim * zim;
#define SMOOTH
#define SCALC smand4_calc
#define SPERI smand4_peri
#define CALC mand4_calc
#define PERI mand4_peri
#define JULIA mand4_julia
#define RANGE 2
#define RPIP
#include "docalc.c"

#define VARIABLES register number_t t;
#define BTEST less_than_4(rp+ip)
#define FORMULA \
	c_pow4(zre, zim, rp, ip); \
	c_mul(zre, zim, rp, ip, t, zim); \
	zre = t + pre; \
	zim += pim; \
	rp = zre * zre; \
	ip = zim * zim;
#define SMOOTH
#define SCALC smand5_calc
#define SPERI smand5_peri
#define CALC mand5_calc
#define PERI mand5_peri
#define JULIA mand5_julia
#define RANGE 2
#define RPIP
#include "docalc.c"


#define VARIABLES register number_t t;
#define BTEST less_than_4(rp+ip)
#define FORMULA \
	c_pow3(zre, zim, rp, ip); \
	c_pow3(rp, ip, t, zim); \
	zre = t + pre; \
	zim += pim; \
	rp = zre * zre; \
	ip = zim * zim;
#define SMOOTH
#define SCALC smand6_calc
#define SPERI smand6_peri
#define CALC mand6_calc
#define PERI mand6_peri
#define JULIA mand6_julia
#define RANGE 2
#define RPIP
#include "docalc.c"

/* plan9 compiler has problem with rest of formulas files. Hope that will be fixed later */

#define VARIABLES
#define BTEST less_than_4(rp+ip)
#define FORMULA \
	if (less_than_0 (zre)) { \
	    rp = zre + 1; \
	} else { \
	    rp = zre - 1; \
	} \
	c_mul(rp, zim, pre, pim, zre, zim); \
	rp = zre * zre; \
	ip = zim * zim;
#define SMOOTH
#define SCALC sbarnsley1_calc
#define CALC barnsley1_calc
#define JULIA barnsley1_julia
#define RANGE 2
#define RPIP
#include "docalc.c"


#define VARIABLES
#define BTEST less_than_4(rp+ip)
#define FORMULA \
	if (less_than_0 (zre*pim + zim*pre)) { \
	    rp = zre + 1; \
	} else { \
	    rp = zre - 1; \
	} \
	c_mul(rp, zim, pre, pim, zre, zim); \
	rp = zre * zre; \
	ip = zim * zim;
#define SMOOTH
#define SCALC sbarnsley2_calc
#define CALC barnsley2_calc
#define JULIA barnsley2_julia
#define RANGE 2
#define RPIP
#include "docalc.c"


#define VARIABLES register number_t n,sqrr,sqri,zre1,zim1;
#define INIT sqri=zim*zim,n=zre,zre=pre,pre=n,n=zim,zim=pim,pim=n,n=(number_t)1;
#define BTEST greater_then_1Em6(n)
#define FORMULA \
	zre1 = zre; \
	zim1 = zim; \
	n = zim * zim; \
	sqri = zre * zre; \
	sqrr = sqri - n; \
	sqri = n + sqri; \
	n = 0.3333333333 / ((sqri * sqri)); \
	zim = (0.66666666) * zim - (zre + zre) * zim * n + pim; \
	zre = (0.66666666) * zre + (sqrr) * n + pre; \
	zre1 -= zre; \
	zim1 -= zim; \
	n = zre1 * zre1 + zim1 * zim1;
#define CALC newton_calc
#include "docalc.c"


#define VARIABLES register number_t n,sqrr,sqri,zre1,zim1;
#define INIT sqri=zim*zim,n=zre,zre=pre,pre=n,n=zim,zim=pim,pim=n,n=(number_t)1;
#define BTEST greater_then_1Em6(n)
#define FORMULA \
    zre1 = zre; \
    zim1 = zim; \
    sqrr = zre * zre; \
    sqri = zim * zim; \
    n = sqri + sqrr; \
    n = 1 / ((n * n * n)); \
    zim = (0.25) * zim * (3 + (sqri - 3 * sqrr) * n) + pim; \
    zre = (0.25) * zre * (3 + (sqrr - 3 * sqri) * n) + pre; \
    zre1 -= zre; \
    zim1 -= zim; \
    n = zre1 * zre1 + zim1 * zim1;
#define CALC newton4_calc
#include "docalc.c"


#define VARIABLES register number_t zpr,zip;
#define SAVEVARIABLES register number_t szpr,szip;
#define SAVE szpr=zpr,szip=zip;
#define RESTORE zpr=szpr,zip=szip;
#define INIT zpr=zip=(number_t)0;
#define BTEST less_than_4(rp+ip)
#define FORMULA \
	rp = rp - ip + pre + pim * zpr; \
	ip = 2 * zre * zim + pim * zip; \
	zpr = zre, zip = zim; \
	zre = rp; \
	zim = ip; \
	rp = zre * zre, ip = zim * zim;
#define SMOOTH
#define SCALC sphoenix_calc
#define SPERI sphoenix_peri
#define CALC phoenix_calc
#define PERI phoenix_peri
#define RPIP
#include "docalc.c"


#define VARIABLES register number_t tr,ti,zpr,zpm,rp1,ip1;
#define INIT zpr=zpm=0,tr=zre,zre=pre,pre=tr,tr=zim,zim=pim,pim=tr,tr=1;
#define BTEST less_than_4(zpr*zpr+zpm*zpm)
#define FORMULA \
	rp1 = zre; \
	ip1 = zim; \
	c_pow3(zre, zim, tr, ti); \
	c_add(tr, ti, zpr, zpm, zre, zim); \
	zpr = rp1 + pre; \
	zpm = ip1 + pim;
#define CALC octo_calc
#define SCALC socto_calc
#define SMOOTH
#define CUSTOMSAVEZMAG szmag=zpr*zpr+zpm*zpm
#define PRESMOOTH zre=zpr*zpr+zpm*zpm
#include "docalc.c"

static CONST symetrytype sym6[] = {
  {0, 1.73205080758},
  {0, -1.73205080758}
};

static CONST symetrytype sym8[] = {
  {0, 1},
  {0, -1}
};

static CONST symetrytype sym16[] = {
  {0, 1},
  {0, -1},
  {0, 0.414214},
  {0, -0.414214},
  {0, 2.414214},
  {0, -2.414214}
};

CONST struct formula formulas[] = {
  {
   FORMULAMAGIC,
#ifndef SLOWFUNCPTR
   mand_calc,
   mand_peri,
   smand_calc,
   smand_peri,
#endif
   mand_julia,
   {"Mandelbrot", "Julia"},
   "mandel",
   /*{0.5, -2.0, 1.25, -1.25}, */
   {-0.75, 0.0, 2.5, 2.5},
   1, 1, 0.0, 0.0,
   {
    {INT_MAX, 0, 0, NULL},
    {INT_MAX, 0, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, 0, 0, NULL},
    {INT_MAX, 0, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, 0, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    },
   {
    {INT_MAX, 0, 0, NULL},
    {INT_MAX, 0, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    },
   MANDEL_BTRACE,
   },
  {
   FORMULAMAGIC,
#ifndef SLOWFUNCPTR
   mand3_calc,
   mand3_peri,
   smand3_calc,
   smand3_peri,
#endif
   mand3_julia,
   {"Mandelbrot^3", "Julia^3"},
   "mandel3",
   /*{1.25, -1.25, 1.25, -1.25}, */
   {0.0, 0.0, 2.5, 2.5},
   1, 1, 0.0, 0.0,
   {
    {0, 0, 0, NULL},
    {INT_MAX, 0, 0, NULL},
    {0, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {0, INT_MAX, 0, NULL},
    {0, 0, 0, NULL},
    {0, 0, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {0, 0, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    },
   {
    {0, 0, 0, NULL},
    {0, 0, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {0, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    },
   MANDEL_BTRACE,
   },
  {
   FORMULAMAGIC,
#ifndef SLOWFUNCPTR
   mand4_calc,
   mand4_peri,
   smand4_calc,
   smand4_peri,
#endif
   mand4_julia,
   {"Mandelbrot^4", "Julia^4"},
   "mandel4",
   /*{1.25, -1.25, 1.25, -1.25}, */
   {0.0, 0.0, 2.5, 2.5},
   1, 1, 0.0, 0.0,
   {
    {INT_MAX, 0, 2, sym6},
    {INT_MAX, 0, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, 0, 0, NULL},
    {INT_MAX, 0, 2, sym6},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, 0, 2, sym6},
    {INT_MAX, INT_MAX, 0, NULL},
    },
   {
    {INT_MAX, 0, 2, sym6},
    {INT_MAX, 0, 2, sym6},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    },
   MANDEL_BTRACE,
   },
  {
   FORMULAMAGIC,
#ifndef SLOWFUNCPTR
   mand5_calc,
   mand5_peri,
   smand5_calc,
   smand5_peri,
#endif
   mand5_julia,
   {"Mandelbrot^5", "Julia^5"},
   "mandel5",
   /*{1.25, -1.25, 1.25, -1.25}, */
   {0.0, 0.0, 2.5, 2.5},
   1, 1, 0.0, 0.0,
   {
    {0, 0, 2, sym8},
    {INT_MAX, 0, 0, NULL},
    {0, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {0, INT_MAX, 0, NULL},
    {0, 0, 2, sym8},
    {0, 0, 2, sym8},
    {INT_MAX, INT_MAX, 0, NULL},
    {0, 0, 2, sym8},
    {INT_MAX, INT_MAX, 0, NULL},
    },
   {
    {0, 0, 2, sym8},
    {0, 0, 2, sym8},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    },
   MANDEL_BTRACE,
   },
  {
   FORMULAMAGIC,
#ifndef SLOWFUNCPTR
   mand6_calc,
   mand6_peri,
   smand6_calc,
   smand6_peri,
#endif
   mand6_julia,
   {"Mandelbrot^6", "Julia^6"},
   "mandel6",
   /*{1.25, -1.25, 1.25, -1.25}, */
   {0.0, 0.0, 2.5, 2.5},
   1, 1, 0.0, 0.0,
   {
    {0, 0, 6, sym16},
    {INT_MAX, 0, 0, NULL},
    {0, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {0, INT_MAX, 0, NULL},
    {0, 0, 0, NULL},
    {0, 0, 6, sym16},
    {INT_MAX, INT_MAX, 0, NULL},
    {0, 0, 6, sym16},
    {INT_MAX, INT_MAX, 0, NULL},
    },
   {
    {0, 0, 6, sym16},
    {0, 0, 6, sym16},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    },
   MANDEL_BTRACE,
   },
  {
   FORMULAMAGIC,
#ifndef SLOWFUNCPTR
   octo_calc,
   /*octo_peri, */ NULL,
   socto_calc,
   /*socto_peri, */ NULL,
#endif
   NULL,
   {"Octal", "Octal"},
   "octal",
   /*{1.25, -1.25, 1.25, -1.25}, */
   {0.0, 0.0, 2.5, 2.5},
   0, 1, 0.0, 0.0,
   {
    {0, 0, 6, sym16},
    {INT_MAX, 0, 0, NULL},
    {0, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {0, INT_MAX, 0, NULL},
    {0, 0, 0, NULL},
    {0, 0, 6, sym16},
    {INT_MAX, INT_MAX, 0, NULL},
    {0, 0, 6, sym16},
    {INT_MAX, INT_MAX, 0, NULL},
    },
   {
    {0, 0, 6, sym16},
    {0, 0, 6, sym16},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    },
   MANDEL_BTRACE | STARTZERO,
   },
  {
   FORMULAMAGIC,
#ifndef SLOWFUNCPTR
   newton_calc,
   NULL,
   NULL,
   NULL,
#endif
   NULL,
   {"Newton", "Newton julia?"},
   "newton",
   /*{1.25, -1.25, 1.25, -1.25}, */
   {0.0, 0.0, 2.5, 2.5},
   0, 1, 1.0199502202048319698, 0.0,
   {
    {INT_MAX, 0, 2, sym6},
    {INT_MAX, 0, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, 0, 2, sym6},
    {INT_MAX, 0, 2, sym6},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, 0, 2, sym6},
    {INT_MAX, INT_MAX, 0, NULL},
    },
   {
    {INT_MAX, 0, 2, sym6},
    {INT_MAX, 0, 2, sym6},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    },
   STARTZERO,
   },
  {
   FORMULAMAGIC,
#ifndef SLOWFUNCPTR
   barnsley1_calc,
   NULL,
   sbarnsley1_calc,
   NULL,
#endif
   barnsley1_julia,
   {"Barnsley1 Mandelbrot", "Barnsley1"},
   "barnsley",
   /*{1.25, -1.25, 1.25, -1.25}, */
   {0.0, 0.0, 2.5, 2.5},
   0, 0, -0.6, 1.1,
   {
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    },
   {
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    },
   STARTZERO | MANDEL_BTRACE,
   },
  {
   FORMULAMAGIC,
#ifndef SLOWFUNCPTR
   phoenix_calc,
   phoenix_peri,
   sphoenix_calc,
   sphoenix_peri,
#endif
   NULL,
   {"MandPhoenix", "Phoenix"},
   "phoenix",
   /*{1.25, -1.25, 1.25, -1.25}, */
   {0.0, 0.0, 2.5, 2.5},
   1, 0, 0.56667000000000001, -0.5,
   {
    {INT_MAX, 0, 0, NULL},
    {INT_MAX, 0, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, 0, 0, NULL},
    {INT_MAX, 0, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, 0, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    },
   {
    {INT_MAX, 0, 0, NULL},
    {INT_MAX, 0, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    },
   MANDEL_BTRACE,
   },
  {
   FORMULAMAGIC,
#ifndef SLOWFUNCPTR
   magnet_calc,
   magnet_peri,
   smagnet_calc,
   smagnet_peri,
#endif
   magnet_julia,
   {"Magnet", "Magnet"},
   "magnet",
   /*{3, 0, 2.2, -2.2}, */
   {1.5, 0.0, 3.0, 4.4},
   1, 1, 0.0, 0.0,
   {
    {INT_MAX, 0, 0, NULL},
    {INT_MAX, 0, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, 0, 0, NULL},
    {INT_MAX, 0, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, 0, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    },
   {
    {INT_MAX, 0, 0, NULL},
    {INT_MAX, 0, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    },
   STARTZERO,
   },
/* formulas added by Andreas Madritsch */
  {
   FORMULAMAGIC,
#ifndef SLOWFUNCPTR
   newton4_calc,
   NULL,
   NULL,
   NULL,
#endif
   NULL,
   {"Newton^4", "Newton^4 julia?"},
   "newton4",
   /*{1.25, -1.25, 1.25, -1.25}, */
   {0.0, 0.0, 2.5, 2.5},
   0, 1, 1.0199502202048319698, 0.0,
   {
    {INT_MAX, 0, 2, sym6},
    {INT_MAX, 0, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, 0, 2, sym6},
    {INT_MAX, 0, 2, sym6},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, 0, 2, sym6},
    {INT_MAX, INT_MAX, 0, NULL},
    },
   {
    {INT_MAX, 0, 2, sym6},
    {INT_MAX, 0, 2, sym6},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    },
   STARTZERO,
   },
  {
   FORMULAMAGIC,
#ifndef SLOWFUNCPTR
   barnsley2_calc,
   NULL,
   sbarnsley2_calc,
   NULL,
#endif
   barnsley2_julia,
   {"Barnsley2 Mandelbrot", "Barnsley2"},
   "barnsley2",
   /*{1.25, -1.25, 1.25, -1.25}, */
   {0.0, 0.0, 2.5, 5.5},
   0, 0, -0.6, 1.1,
   {
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    },
   {
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    },
   STARTZERO | MANDEL_BTRACE,
   },
  {
   FORMULAMAGIC,
#ifndef SLOWFUNCPTR
   magnet2_calc,
   magnet2_peri,
   smagnet2_calc,
   smagnet2_peri,
#endif
   magnet2_julia,
   {"Magnet2", "Magnet2"},
   "magnet2",
   /*{3, 0, 2.2, -2.2}, */
   {1.0, 0.0, 3.0, 3.2},
   1, 1, 0.0, 0.0,
   {
    {INT_MAX, 0, 0, NULL},
    {INT_MAX, 0, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, 0, 0, NULL},
    {INT_MAX, 0, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, 0, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    },
   {
    {INT_MAX, 0, 0, NULL},
    {INT_MAX, 0, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    {INT_MAX, INT_MAX, 0, NULL},
    },
   STARTZERO,
   },
};

#ifdef SLOWFUNCPTR
unsigned int
calculateswitch (register number_t x1, register number_t y1,
		 register number_t x2, register number_t y2, int periodicity)
{
  if (periodicity && cfractalc.periodicity)
    if (cfractalc.coloringmode == 9)
      switch (cfractalc.currentformula - formulas)
	{
	case 0:
	  return (smand_peri (x1, y1, x2, y2));
	  break;
	case 1:
	  return (smand3_peri (x1, y1, x2, y2));
	  break;
	case 2:
	  return (smand4_peri (x1, y1, x2, y2));
	  break;
	case 3:
	  return (smand5_peri (x1, y1, x2, y2));
	  break;
	case 4:
	  return (smand6_peri (x1, y1, x2, y2));
	  break;
	case 5:
	  return (octo_calc (x1, y1, x2, y2));
	  break;
	case 6:
	  return (newton_calc (x1, y1, x2, y2));
	  break;
	case 7:
	  return (sbarnsley1_calc (x1, y1, x2, y2));
	  break;
	case 8:
	  return (sphoenix_peri (x1, y1, x2, y2));
	  break;
	case 9:
	  return (smagnet_peri (x1, y1, x2, y2));
	  break;
	case 10:
	  return (newton4_calc (x1, y1, x2, y2));
	  break;
	case 11:
	  return (sbarnsley2_calc (x1, y1, x2, y2));
	  break;
	case 12:
	  return (smagnet2_peri (x1, y1, x2, y2));
	  break;
	}
    else
      switch (cfractalc.currentformula - formulas)
	{
	case 0:
	  return (mand_peri (x1, y1, x2, y2));
	  break;
	case 1:
	  return (mand3_peri (x1, y1, x2, y2));
	  break;
	case 2:
	  return (mand4_peri (x1, y1, x2, y2));
	  break;
	case 3:
	  return (mand5_peri (x1, y1, x2, y2));
	  break;
	case 4:
	  return (mand6_peri (x1, y1, x2, y2));
	  break;
	case 5:
	  return (octo_calc (x1, y1, x2, y2));
	  break;
	case 6:
	  return (newton_calc (x1, y1, x2, y2));
	  break;
	case 7:
	  return (barnsley1_calc (x1, y1, x2, y2));
	  break;
	case 8:
	  return (phoenix_peri (x1, y1, x2, y2));
	  break;
	case 9:
	  return (magnet_peri (x1, y1, x2, y2));
	  break;
	case 10:
	  return (newton4_calc (x1, y1, x2, y2));
	  break;
	case 11:
	  return (sbarnsley2_calc (x1, y1, x2, y2));
	  break;
	case 12:
	  return (smagnet2_peri (x1, y1, x2, y2));
	  break;
	}
  else if (cfractalc.coloringmode == 9)
    switch (cfractalc.currentformula - formulas)
      {
      case 0:
	return (smand_calc (x1, y1, x2, y2));
	break;
      case 1:
	return (smand3_calc (x1, y1, x2, y2));
	break;
      case 2:
	return (smand4_calc (x1, y1, x2, y2));
	break;
      case 3:
	return (smand5_calc (x1, y1, x2, y2));
	break;
      case 4:
	return (smand6_calc (x1, y1, x2, y2));
	break;
      case 5:
	return (octo_calc (x1, y1, x2, y2));
	break;
      case 6:
	return (newton_calc (x1, y1, x2, y2));
	break;
      case 7:
	return (sbarnsley1_calc (x1, y1, x2, y2));
	break;
      case 8:
	return (sphoenix_calc (x1, y1, x2, y2));
	break;
      case 9:
	return (smagnet_calc (x1, y1, x2, y2));
	break;
      case 10:
	return (newton4_calc (x1, y1, x2, y2));
	break;
      case 11:
	return (sbarnsley2_calc (x1, y1, x2, y2));
	break;
      case 12:
	return (smagnet2_peri (x1, y1, x2, y2));
	break;
      }
  else
    switch (cfractalc.currentformula - formulas)
      {
      case 0:
	return (mand_calc (x1, y1, x2, y2));
	break;
      case 1:
	return (mand3_calc (x1, y1, x2, y2));
	break;
      case 2:
	return (mand4_calc (x1, y1, x2, y2));
	break;
      case 3:
	return (mand5_calc (x1, y1, x2, y2));
	break;
      case 4:
	return (mand6_calc (x1, y1, x2, y2));
	break;
      case 5:
	return (octo_calc (x1, y1, x2, y2));
	break;
      case 6:
	return (newton_calc (x1, y1, x2, y2));
	break;
      case 7:
	return (barnsley1_calc (x1, y1, x2, y2));
	break;
      case 8:
	return (phoenix_calc (x1, y1, x2, y2));
	break;
      case 9:
	return (magnet_calc (x1, y1, x2, y2));
	break;
      case 10:
	return (newton4_calc (x1, y1, x2, y2));
	break;
      case 11:
	return (sbarnsley2_calc (x1, y1, x2, y2));
	break;
      case 12:
	return (smagnet2_peri (x1, y1, x2, y2));
	break;
      }
  return 0;
}
#endif

CONST struct formula *currentformula;
CONST int nformulas = sizeof (formulas) / sizeof (struct formula);
