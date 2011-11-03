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

// Some help can be read below about line 700. :-)
   

#ifndef _MAC
#include <aconfig.h>
#endif

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#ifdef __EMX__
#include <sys/cdefs.h>
#endif
#include <stdio.h>
#endif				/*plan9 */
#include <archaccel.h>
#include <config.h>
#include <complex.h>
#include <filter.h>
#include <fractal.h>
#include "julia.h"
#include <ui_helper.h>
#ifndef M_PI
#define M_PI 3.1415
#endif

#ifdef SLOWFUNCPTR
#define FUNCTYPE INLINE
#else
#define FUNCTYPE
#endif

#ifdef SFFE_USING
#include "sffe.h"

extern struct uih_context *globaluih;	// to be able to use sffe parser
#endif

const char *const incolorname[] = {
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

const char *const outcolorname[] = {
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

const char *const tcolorname[] = {
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
    "simple red (for education purposes)",
    "simple blue (for education purposes)",
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
typedef union {
    unsigned int *i;
    float *f;
} fpint;
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
/* 2009-07-30 JB Langston:
 * Fixing bug #3: HSV modes are completely black when compiled with GCC 4...
 * Removed CONSTF qualifier from hsv_to_rgb declaration. CONSTF macro is 
 * defined to __attribute__((__const__)), on which I found some more details 
 * here: http://unixwiz.net/techtips/gnu-c-attributes.html#const.  Apparently
 * this should never be used with a function that takes a pointer or relies on
 * side-effects, and hsv_to_rgb does both.  Therefore, it should never have 
 * been declared this way in the first place.
 */

static INLINE void
hsv_to_rgb(int h, int s, int v, int *red, int *green, int *blue) /*CONSTF*/;
static INLINE void
hsv_to_rgb(int h, int s, int v, int *red, int *green, int *blue)
{
    int hue;
    int f, p, q, t;

    if (s == 0) {
	*red = v;
	*green = v;
	*blue = v;
    } else {
	h %= 256;
	if (h < 0)
	    h += 256;
	hue = h * 6;

	f = hue & 255;
	p = v * (256 - s) / 256;
	q = v * (256 - (s * f) / 256) >> 8;
	t = v * (256 * 256 - (s * (256 - f))) >> 16;

	switch ((int) (hue / 256)) {
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
truecolor_output(number_t zre, number_t zim, number_t pre, number_t pim,
		 int mode, int inset)
CONSTF REGISTERS(3);
REGISTERS(3)
static unsigned int
truecolor_output(number_t zre, number_t zim, number_t pre,
		 number_t pim, int mode, int inset)
{
    /* WARNING: r and b fields are swapped for HISTORICAL REASONS (BUG :),
     * in other words: use r for blue and b for red. */
    int r = 0, g = 0, b = 0, w = 0;

    switch (mode) {
    case 0:
	break;
    case 1:
	b = (int) ((sin((double) atan2((double) zre, (double) zim) * 20) +
		    1) * 127);
	w = (int) ((sin((double) zim / zre)) * 127);
	r = (int) ((int) (zre * zim));
	g = (int) ((sin((double) (zre * zre) / 2) + 1) * 127);
	break;
    case 2:
	if (!inset) {
	    r = (int) ((sin((double) zre * 2) + 1) * 127);
	    g = (int) ((sin((double) zim * 2) + 1) * 127);
	    b = (int) ((sin((double) (zim * zim + zre * zre) / 2) +
			1) * 127);
	} else {
	    r = (int) ((sin((double) zre * 50) + 1) * 127);
	    g = (int) ((sin((double) zim * 50) + 1) * 127);
	    b = (int) ((sin((double) (zim * zim + zre * zre) * 50) +
			1) * 127);
	}
	w = (int) ((sin((double) zim / zre)) * 127);
	break;
    case 3:
	if (inset)
	    hsv_to_rgb((int)
		       (atan2((double) zre, (double) zim) * 256 / M_PI),
		       (int) ((sin((double) (zre * 50)) + 1) * 128),
		       (int) ((sin((double) (zim * 50)) + 1) * 128), &r,
		       &g, &b);
	else
	    hsv_to_rgb((int)
		       (atan2((double) zre, (double) zim) * 256 / M_PI),
		       (int) ((sin((double) zre) + 1) * 128),
		       (int) ((sin((double) zim) + 1) * 128), &r, &g, &b);
	break;
    case 4:
	if (inset)
	    hsv_to_rgb((int)
		       (sin((double) (zre * zre + zim * zim) * 0.1) * 256),
		       (int) (sin(atan2((double) zre, (double) zim) * 10) *
			      128 + 128),
		       (int) ((sin((double) (zre + zim) * 10)) * 65 + 128),
		       &r, &g, &b);
	else
	    hsv_to_rgb((int)
		       (sin((double) (zre * zre + zim * zim) * 0.01) *
			256),
		       (int) (sin(atan2((double) zre, (double) zim) * 10) *
			      128 + 128),
		       (int) ((sin((double) (zre + zim) * 0.3)) * 65 +
			      128), &r, &g, &b);
	break;
    case 5:
	{
	    if (!inset) {
		r = (int) (cos((double) myabs(zre * zre)) * 128) + 128;
		g = (int) (cos((double) myabs(zre * zim)) * 128) + 128;
		b = (int) (cos((double) myabs(zim * zim + zre * zre)) *
			   128) + 128;
	    } else {
		r = (int) (cos((double) myabs(zre * zre) * 10) * 128) +
		    128;
		g = (int) (cos((double) myabs(zre * zim) * 10) * 128) +
		    128;
		b = (int) (cos((double) myabs(zim * zim + zre * zre) * 10)
			   * 128) + 128;
	    }
	}
	break;
    case 6:
	{
	    if (!inset) {
		r = (int) (zre * zim * 64);
		g = (int) (zre * zre * 64);
		b = (int) (zim * zim * 64);
	    } else
		r = (int) (zre * zim * 256);
	    g = (int) (zre * zre * 256);
	    b = (int) (zim * zim * 256);
	}
	break;
    case 7:
	{
	    if (!inset) {
		r = (int) ((zre * zre + zim * zim - pre * pre -
			    pim * pim) * 16);
		g = (int) ((zre * zre * 2 - pre * pre - pim * pim) * 16);
		b = (int) ((zim * zim * 2 - pre * pre - pim * pim) * 16);
	    } else {
		r = (int) ((zre * zre + zim * zim - pre * pre -
			    pim * pim) * 256);
		g = (int) ((zre * zre * 2 - pre * pre - pim * pim) * 256);
		b = (int) ((zim * zim * 2 - pre * pre - pim * pim) * 256);
	    }
	}
	break;
    case 8:
	{
	    if (!inset) {
		r = (int) ((myabs(zim * pim)) * 64);
		g = (int) ((myabs(zre * pre)) * 64);
		b = (int) ((myabs(zre * pim)) * 64);
	    } else {
		r = (int) ((myabs(zim * pim)) * 256);
		g = (int) ((myabs(zre * pre)) * 256);
		b = (int) ((myabs(zre * pim)) * 256);
	    }
	}
	break;
    case 9:
	{
	    if (!inset) {
		r = (int) ((myabs(zre * zim - pre * pre - pim * pim)) *
			   64);
		g = (int) ((myabs(zre * zre - pre * pre - pim * pim)) *
			   64);
		b = (int) ((myabs(zim * zim - pre * pre - pim * pim)) *
			   64);
	    } else {
		r = (int) ((myabs(zre * zim - pre * pre - pim * pim)) *
			   256);
		g = (int) ((myabs(zre * zre - pre * pre - pim * pim)) *
			   256);
		b = (int) ((myabs(zim * zim - pre * pre - pim * pim)) *
			   256);
	    }
	}
	break;
    case 10:
	{
	    r = (int) (atan2((double) zre, (double) zim) * 128 / M_PI) +
		128;
	    g = (int) (atan2((double) zre, (double) zim) * 128 / M_PI) +
		128;
	    b = (int) (atan2((double) zim, (double) zre) * 128 / M_PI) +
		128;
	}
	break;
	// case 11 is for disabling truecolor mode
    case 12:
	{
	    b = 255;
	    g = 0;
	    r = 0;
	    w = 50;
	}
	break;
    case 13:
	{
	    r = 255;
	    g = 0;
	    b = 0;
	    w = 0;
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

    switch (cpalette.type) {
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
color_output(number_t zre, number_t zim, unsigned int iter)
CONSTF REGISTERS(3);
static unsigned int
REGISTERS(3) color_output(number_t zre, number_t zim, unsigned int iter)
{
    int i;
    iter <<= SHIFT;
    i = iter;

    switch (cfractalc.coloringmode) {
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
	if (myabs(zim) < 2.0 || myabs(zre) < 2.0)
	    i = ((cfractalc.maxiter << SHIFT) - iter);
	break;
    case 7:
	zre = zre * zre + zim * zim;
#ifdef __TEST__
	if (zre < 1 || !i)
	    i = 0;
	else
#endif
	    i = (int) (sqrt(log((double) zre) / i) * 256 * 256);
	break;
    default:
    case 8:
	i = (int) ((atan2((double) zre, (double) zim) / (M_PI + M_PI) +
		    0.75) * 20000);
	break;
    }

    if (i < 0) {
	i = (((unsigned int) (cpalette.size - 1)) << 8) -
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
	return (interpoltype(cpalette, i2, i1, iter));
    }

}

static unsigned int
incolor_output(number_t zre, number_t zim, number_t pre, number_t pim,
	       unsigned int iter)
CONSTF REGISTERS(3);
REGISTERS(3)
static unsigned int
incolor_output(number_t zre, number_t zim, number_t pre, number_t pim,
	       unsigned int iter)
{
    int i = iter;
    switch (cfractalc.incoloringmode) {
    case 1:			/* zmag */
	i = (int) (((zre * zre + zim * zim) *
		    (number_t) (cfractalc.maxiter >> 1) * SMUL + SMUL));
	break;
    case 2:			/* real */
	i = (int) (((atan2((double) zre, (double) zim) / (M_PI + M_PI) +
		     0.75) * 20000));
	break;
    default:
	break;
    case 3:			/* real / imag */
	i = (int) (100 + (zre / zim) * SMUL * 10);
	break;
    case 4:
	zre = myabs(zre);
	zim = myabs(zim);
	pre = myabs(pre);
	pre = myabs(pim);
	i += (int) (myabs(pre - zre) * 256 * 64);
	i += (int) (myabs(pim - zim) * 256 * 64);
	break;
    case 5:
	if (((int) ((zre * zre + zim * zim) * 10)) % 2)
	    i = (int) (cos((double) (zre * zim * pre * pim)) * 256 * 256);
	else
	    i = (int) (sin((double) (zre * zim * pre * pim)) * 256 * 256);
	break;
    case 6:
	i = (int) ((zre * zre +
		    zim * zim) * cos((double) (zre * zre)) * 256 * 256);
	break;
    case 7:
	i = (int) (sin((double) (zre * zre - zim * zim)) * 256 * 256);
	break;
    case 8:
	i = (int) (atan((double) (zre * zim * pre * pim)) * 256 * 64);
	break;
    case 9:
	if ((abs((int) (zre * 40)) % 2) ^ (abs((int) (zim * 40)) % 2))
	    i = (int) (((atan2((double) zre, (double) zim) /
			 (M_PI + M_PI) + 0.75)
			* 20000));
	else
	    i = (int) (((atan2((double) zim, (double) zre) /
			 (M_PI + M_PI) + 0.75)
			* 20000));
	break;
    };

    if (i < 0) {
	i = (((unsigned int) (cpalette.size - 1)) << 8) -
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
	if (((unsigned int) iter >> 8) ==
	    (unsigned int) (cpalette.size - 2))
	    i2 = cpalette.pixels[1];
	else
	    i2 = cpalette.pixels[2 + ((unsigned int) iter >> 8)];
	iter &= 255;
	return (interpoltype(cpalette, i2, i1, iter));
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

pacalc(long double zre, long double zim, long double pre, long double pim)
{
    int iter = 1000000;
    NSFORMULALOOP(iter);
    return iter;
}
#endif
#endif
#endif
#endif

/* Some help for the brave ones. :-)
 *
 * Mandelbrot's original formula is z=z^2+c which means
 * z[next]=z[previous]^2+c.
 * Here c is the pixel coordinates from the screen and z[0] is usually 0
 * (if not perturbation was added.)
 * In the following code z[previous] is described by (zre;zim)
 * and z[next] will also be zre and zim.
 * c is described by (pre;pim).
 * Finally rp and ip are helper variables, mostly for checking the bailout
 * (which usually means abs(z)>=4, see BTEST).
 *
 * Both basic operations and some other functions (c_mul, c_pow3, ...) can
 * be used. For a "detailed" description refer to ../include/complex.h.
 * 
 * If you add/modify fractals, please note that struct formula_formulas
 * (at line cca. 1300) should be also edited for proper initialization
 * and for menu entries. However it is not self-explanatory, just copy-paste
 * existing tables and give it a try.
 *
 * Finally, please also edit the calculateswitch function and
 * the nmformulas constant (at the end of this file).
 *
 * -- Zoltan, 2009-07-30
 */


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
	c_mul(rp, ip, rp, ip, t, zim); \
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
#define SCALC smand9_calc
#define SPERI smand9_peri
#define CALC mand9_calc
#define PERI mand9_peri
#define JULIA mand9_julia
#define RANGE 2
#define RPIP
#include "docalc.c"

/* formulas from here to the next comment are not tested under plan9 compiler */

#define VARIABLES
#define BTEST less_than_4(rp+ip)
#define FORMULA \
	zim=zre*zim+zim/2+pim; \
	zre=(rp-ip+zre)/2+pre; \
	rp=zre*zre; \
	ip=zim*zim;
#define SMOOTH
#define SCALC strice_calc
#define SPERI strice_peri
#define CALC trice_calc
#define PERI trice_peri
#define RANGE 2
#define RPIP
#include "docalc.c"

#define VARIABLES register number_t zor,zoi;
/* 2009-08-01 JB Langston
 * On Mac OS X, for some reason Cat's Eye renders as an empty circle unless
 * the bailout is slightly more than 4.  This doesn't appear to happen on any
 * other operating systems, and it's not processor specific.  It's probably
 * a compiler bug, but I haven't been able to figure out exactly what's
 * happening.  I can work around it by subtracting LDBL_MIN from the amount 
 * before performing the bailout test.
 */
// #define LDBL_MIN 0.00000001
#define BTEST less_than_4(rp+ip-LDBL_MIN)
#define FORMULA \
	c_div(pre,pim,zre,zim,rp,ip); \
	c_div(zre,zim,pre,pim,zor,zoi); \
	zre=zor+rp; \
	zim=zoi+ip; \
	rp=zre*zre; \
	ip=zim*zim;
#define SMOOTH
#define SCALC scatseye_calc
#define SPERI scatseye_peri
#define CALC catseye_calc
#define PERI catseye_peri
#define RANGE 2
#define RPIP
#include "docalc.c"

#define VARIABLES
#define BTEST less_than_4(rp+ip)
#define FORMULA \
	    zim=(zim*zre)*(-2.0)+pim; \
	    zre=rp-ip+pre; \
            ip=zim*zim; \
            rp=zre*zre;
#define SMOOTH
#define SCALC smbar_calc
#define SPERI smbar_peri
#define CALC mbar_calc
#define PERI mbar_peri
#define JULIA mbar_julia
#define RANGE 2
#define RPIP
#include "docalc.c"

#define VARIABLES
#define INIT \
	rp=zre;zre=pre;pre=rp; \
        ip=zim;zim=pim;pim=ip;
#define BTEST less_than_4(rp+ip)
#define FORMULA \
	rp=ip-rp+zre; \
	ip=zim-2*zre*zim; \
	c_mul(rp,ip,pre,pim,zre,zim); \
	rp=zre*zre; \
	ip=zim*zim;
#define SMOOTH
#define SCALC smlambda_calc
#define SPERI smlambda_peri
#define CALC mlambda_calc
#define PERI mlambda_peri
#define RANGE 2
#define RPIP
#include "docalc.c"

#define VARIABLES register number_t zre1,zim1,zre2,zim2;
#define INIT zre1=zre;zim1=zim;
#define BTEST less_than_4(rp+ip)
#define FORMULA \
	zre2=zre;zim2=zim; \
	zim=(zim*zre)*2+pim+zim1; \
	zre=rp-ip+pre+zre1; \
	zre1=zre2; \
	zim1=zim2; \
	ip=zim*zim; \
	rp=zre*zre;
#define SMOOTH
#define SCALC smanowar_calc
#define CALC manowar_calc
#define RANGE 2
#define RPIP
#include "docalc.c"

#define VARIABLES register number_t zre1,zim1;
#define INIT zre1=pre;zim1=pim;
#define BTEST less_than_4(rp+ip)
#define FORMULA \
	zim=(zim*zre)*2+zim1; \
	zre=rp-ip+zre1; \
	zre1=zre1/2+zre; \
	zim1=zim1/2+zim; \
	ip=zim*zim; \
	rp=zre*zre;
#define SMOOTH
#define SCALC sspider_calc
#define CALC spider_calc
#define RANGE 2
#define RPIP
#include "docalc.c"

#define VARIABLES
#define INIT \
	if((zre==pre)&&(zim==pim)){pre=0.5;pim=0.8660254;} \
	if(pim<0)pim=(-pim); \
	if(((pim*zre-pre*zim)<0)||(zim<0)){zre=2*pre+2;zim=2*pim;}
#define BTEST ((pim*zre+(1-pre)*zim)<pim)
#define FORMULA \
	zre=2*zre;zim=2*zim; \
	if((pim*zre-pre*zim)>pim)zre=zre-1; \
	if(zim>pim){zim=zim-pim;zre=zre-pre;}
#define CALC sier_calc
#define RANGE 2
#define RPIP
#include "docalc.c"

#define VARIABLES
#define INIT \
	if((zre==pre)&&(zim==pim)){pre=0.5;pim=0.8660254;} \
	if(pim<0)pim=(-pim); \
	if(((pim*zre-pre*zim)<0)||(zim<0)){zre=2*pre+2;zim=2*pim;}
#define BTEST ((pim*zre+(1-pre)*zim)<pim)
#define FORMULA \
	zre=1.6180339*zre;zim=1.6180339*zim; \
	if((pim*zre-pre*zim)>pim*0.6180339)zre=zre-0.6180339; \
	if(zim>pim*0.6180339){zim=zim-pim*0.6180339;zre=zre-pre*0.6180339;}
#define CALC goldsier_calc
#define RANGE 2
#define RPIP
#include "docalc.c"

#define VARIABLES
#define INIT
#define BTEST (zre*zre+zim*zim<1)
#define FORMULA \
	zre=3*zre;zim=3*zim; \
	if((zim-2)*(zim-2)+zre*zre<1)zim=zim-2; \
	if((zim+2)*(zim+2)+zre*zre<1)zim=zim+2; \
	if((zim-1)*(zim-1)+(zre-1.7320508)*(zre-1.7320508)<1){zim=zim-1;zre=zre-1.7320508;} \
	if((zim+1)*(zim+1)+(zre-1.7320508)*(zre-1.7320508)<1){zim=zim+1;zre=zre-1.7320508;} \
	if((zim-1)*(zim-1)+(zre+1.7320508)*(zre+1.7320508)<1){zim=zim-1;zre=zre+1.7320508;} \
	if((zim+1)*(zim+1)+(zre+1.7320508)*(zre+1.7320508)<1){zim=zim+1;zre=zre+1.7320508;}
#define CALC circle7_calc
#define RANGE 2
#define RPIP
#include "docalc.c"

#define VARIABLES
#define INIT
#define BTEST less_than_4((rp+ip)/4.0)
#define FORMULA \
	if (less_than_0 (zre)) { \
	    rp = zre + 1; \
	} else { \
	    rp = zre - 1; \
	} \
	if (less_than_0 (zim)) { \
	    ip = zim + 1; \
	} else { \
	    ip = zim - 1; \
	} \
	c_mul(rp, ip, pre, pim, zre, zim); \
	rp = zre * zre; \
	ip = zim * zim;
#define CALC symbarn_calc
#define JULIA symbarn_julia
#define RANGE 2
#define RPIP
#include "docalc.c"

#define VARIABLES
#define INIT \
	if((zre==pre)&&(zim==pim)){pre=1;pim=1;} \
	if(pre<0)pre=(-pre);if(pim<0)pim=(-pim); \
	if((zre<0)||(zre>pre)){zre=pre/2;zim=pim/2;} \
	if((zim<0)||(zim>pim)){zre=pre/2;zim=pim/2;}
#define BTEST \
	((zre<pre/3)||(zre>2*pre/3)|| \
	(zim<pim/3)||(zim>2*pim/3))
#define FORMULA \
	zre=3*zre;zim=3*zim; \
	if(zre>2*pre)zre=zre-2*pre;else if(zre>pre)zre=zre-pre; \
	if(zim>2*pim)zim=zim-2*pim;else if(zim>pim)zim=zim-pim;
#define CALC carpet_calc
#define RANGE 2
#define RPIP
#include "docalc.c"


#define VARIABLES
#define BTEST \
	((((1.5*zre+0.8660254038*zim)>0.8660254038)|| \
	((0.8660254038*zim-1.5*zre)>0.8660254038)|| \
	(zim<(-0.5)))&& \
	(((1.5*zre+0.8660254038*zim)<-0.8660254038)|| \
	((0.8660254038*zim-1.5*zre)<-0.8660254038)|| \
	(zim>0.5)))
#define FORMULA \
	zre=3*zre;zim=3*zim; \
	if((0.2886751346*zim-0.5*zre)>0.0){ \
		if((0.2886751346*zim+0.5*zre)>0.0){ \
			zim=zim-2.0;\
		}else{ \
			if(zim>0){zre=zre+1.732050808;zim=zim-1.0;} \
	    		else{zre=zre+1.732050808;zim=zim+1.0;} \
	    } \
	}else{ \
		if((0.2886751346*zim+0.5*zre)<0.0){ \
			zim=zim+2.0;\
		}else{ \
			if(zim>0){zre=zre-1.732050808;zim=zim-1.0;} \
			else{zre=zre-1.732050808;zim=zim+1.0;} \
		} \
	}
#define CALC koch_calc
#define RANGE 2
#define RPIP
#include "docalc.c"

#define VARIABLES register number_t zre1, zim1;
#define INIT pim=fabs(pim); zre=pre; zim=pim;
#define BTEST \
	(!((zre<0)&&(zim>0)&&(-1.0*zre+1.732050808*zim<1.732050808)))
#define FORMULA \
	zre1=1.5*zre-0.866+0.866*zim; \
	zim1=-1.5+1.5*zim-0.866*zre; \
	zre=zre1; zim=zim1;
#define CALC hornflake_calc
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

#define VARIABLES
#define BTEST less_than_4(rp+ip)
#define FORMULA \
	if (!less_than_0 (-zre)) { \
		zim = 2*zre*zim + pim*zre; \
		zre = rp - ip - 1 + pre*zre; \
	} else { \
		zim = 2*zre*zim; \
		zre = rp - ip - 1; \
	} \
	rp = zre * zre; \
	ip = zim * zim;
#define SMOOTH
#define SCALC sbarnsley3_calc
#define CALC barnsley3_calc
#define JULIA barnsley3_julia
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


#define VARIABLES register number_t yre, yim, re1tmp, re2tmp, im1tmp;
#define BTEST (rp+ip<9||(yre*yre+yim*yim)<4*(rp+ip))
#define INIT yre=pre; yim=pim;
#define FORMULA \
        re1tmp=zre; \
      	re2tmp=yre; \
	im1tmp=zim; \
        zre=re1tmp+yre; \
	zim=im1tmp+yim; \
	yre=(re1tmp*re2tmp-im1tmp*yim   ); \
	yim=(re1tmp*yim   +re2tmp*im1tmp); \
	rp=zre*zre; \
	ip=zim*zim;
#define CALC beryl_calc
#define PERI beryl_peri
#define RANGE 2
#define RPIP
#include "docalc.c"



#ifdef SFFE_USING
 /* SFFE - malczak */
 //#define VARIABLES sffe *p = globaluih->parser; 
#define INIT cmplxset(pZ,0,0); cmplxset(C,pre,pim); \
		if (globaluih->pinit) Z=sffe_eval(globaluih->pinit); else cmplxset(Z,zre,zim);
 //#define SAVE cmplxset(pZ,real(Z),imag(Z));
 //#define PRETEST 0
#define FORMULA \
	 Z = sffe_eval(globaluih->parser);\
	 cmplxset(pZ,zre,zim); \
	 zre = real( Z ); \
	 zim = imag( Z );
#define BTEST less_than_4(zre*zre+zim*zim)
 //less_than_4(rp+ip)
#define CALC sffe_calc
#define JULIA sffe_julia
//#define SCALC ssffe_calc
//#define SMOOTH
#include "docalc.c"
#endif


static const symetrytype sym6[] = {
    {0, 1.73205080758},
    {0, -1.73205080758}
};

static const symetrytype sym8[] = {
    {0, 1},
    {0, -1}
};

static const symetrytype sym16[] = {
    {0, 1},
    {0, -1},
    {0, 0.414214},
    {0, -0.414214},
    {0, 2.414214},
    {0, -2.414214}
};

const struct formula formulas[] = {
    {				/* 0 */
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
    {				/* 1 */
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
    {				/* 2 */
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
    {				/* 3 */
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
    {				/* 4 */
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
    {				/* 5 */
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
    {				/* formula added by Andreas Madritsch *//* 6 */
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
    {				/* 7 */
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
    {				/* formula added by Andreas Madritsch *//* 8 */
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
    {				/* formula added by Arpad Fekete *//* 9 */
     FORMULAMAGIC,
#ifndef SLOWFUNCPTR
     barnsley3_calc,
     NULL,
     sbarnsley3_calc,
     NULL,
#endif
     barnsley3_julia,
     {"Barnsley3 Mandelbrot", "Barnsley3"},
     "barnsley3",
     /*{1.25, -1.25, 1.25, -1.25}, */
     {0.0, 0.0, 2.5, 3.5},
     0, 0, 0.0, 0.4,
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
    {				/* 10 */
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
    {				/* 11 */
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
    {				/* 12 */
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
    {				/* formula added by Andreas Madritsch *//* 13 */
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
    {				/* formula added by Arpad Fekete *//* 14 */
     FORMULAMAGIC,
#ifndef SLOWFUNCPTR
     trice_calc,
     trice_peri,
     strice_calc,
     strice_peri,
#endif
     NULL,
     {"Triceratops", "Triceratops Julia"},
     "trice",
     {0.0, 0.0, 2.5, 4.5},
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
      {INT_MAX, 0, 0, NULL},
      {INT_MAX, 0, 0, NULL},
      {INT_MAX, 0, 0, NULL},
      {INT_MAX, 0, 0, NULL},
      {INT_MAX, INT_MAX, 0, NULL},
      {INT_MAX, INT_MAX, 0, NULL},
      },
     MANDEL_BTRACE,
     },
    {				/* formula added by Arpad Fekete *//* 15 */
     FORMULAMAGIC,
#ifndef SLOWFUNCPTR
     catseye_calc,
     catseye_peri,
     scatseye_calc,
     scatseye_peri,
#endif
     NULL,
     {"Catseye", "Catseye Julia"},
     "catseye",
     {0.0, 0.0, 2.5, 4.5},
     1, 1, 0.0, 0.0,
     {
      {0, 0, 0, NULL},
      {INT_MAX, 0, 0, NULL},
      {INT_MAX, INT_MAX, 0, NULL},
      {INT_MAX, INT_MAX, 0, NULL},
      {INT_MAX, INT_MAX, 0, NULL},
      {INT_MAX, INT_MAX, 0, NULL},
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
      {INT_MAX, INT_MAX, 0, NULL},
      {INT_MAX, INT_MAX, 0, NULL},
      {0, 0, 0, NULL},
      {0, 0, 0, NULL},
      {0, 0, 0, NULL},
      {0, 0, 0, NULL},
      {INT_MAX, INT_MAX, 0, NULL},
      {INT_MAX, INT_MAX, 0, NULL},
      },
     MANDEL_BTRACE,
     },
    {				/*formula added by Arpad Fekete *//* 16 */
     /*in Gnofract4d from mathworld.wolfram.com */
     FORMULAMAGIC,
#ifndef SLOWFUNCPTR
     mbar_calc,
     mbar_peri,
     smbar_calc,
     smbar_peri,
#endif
     mbar_julia,
     {"Mandelbar", "Mandelbar Julia"},
     "mbar",
     {0.0, 0.0, 2.5, 3.5},
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
      {INT_MAX, 0, 0, NULL},
      {INT_MAX, 0, 0, NULL},
      {INT_MAX, 0, 0, NULL},
      {INT_MAX, 0, 0, NULL},
      {INT_MAX, INT_MAX, 0, NULL},
      {INT_MAX, INT_MAX, 0, NULL},
      },
     MANDEL_BTRACE,
     },
    {				/* formula added by Arpad Fekete (from fractint) *//* 17 */
     FORMULAMAGIC,
#ifndef SLOWFUNCPTR
     mlambda_calc,
     mlambda_peri,
     smlambda_calc,
     smlambda_peri,
#endif
     NULL,
     {"Lambda Mandelbrot", "Lambda"},
     "mlambda",
     {0.5, 0.0, 2.5, 5.5},
     0, 0, 0.5, 0.0,
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
     MANDEL_BTRACE,
     },
    {				/* formula added by Arpad Fekete (from fractint) *//* 18 */
     FORMULAMAGIC,
#ifndef SLOWFUNCPTR
     manowar_calc,
     NULL,
     smanowar_calc,
     NULL,
#endif
     NULL,
     {"Manowar", "Manowar Julia"},
     "manowar",
     {0.0, 0.0, 2.5, 2.5},
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
      {INT_MAX, 0, 0, NULL},
      {INT_MAX, 0, 0, NULL},
      {INT_MAX, 0, 0, NULL},
      {INT_MAX, 0, 0, NULL},
      {INT_MAX, INT_MAX, 0, NULL},
      {INT_MAX, INT_MAX, 0, NULL},
      },
     MANDEL_BTRACE,
     },
    {				/* formula added by Arpad Fekete (from fractint) *//* 19 */
     FORMULAMAGIC,
#ifndef SLOWFUNCPTR
     spider_calc,
     NULL,
     sspider_calc,
     NULL,
#endif
     NULL,
     {"Spider", "Spider Julia"},
     "spider",
     {0.0, 0.0, 2.5, 4.5},
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
      {INT_MAX, 0, 0, NULL},
      {INT_MAX, 0, 0, NULL},
      {INT_MAX, 0, 0, NULL},
      {INT_MAX, 0, 0, NULL},
      {INT_MAX, INT_MAX, 0, NULL},
      {INT_MAX, INT_MAX, 0, NULL},
      },
     MANDEL_BTRACE,
     },
    {				/* formula added by Arpad Fekete, method from fractint *//* 20 */
     FORMULAMAGIC,
#ifndef SLOWFUNCPTR
     sier_calc,
     NULL,
     NULL,
     NULL,
#endif
     NULL,
     {"Sierpinski", "Sierpinski"},
     "sier",
     {0.5, 0.43, 1.5, 1.0},
     0, 0, 0.5, 0.8660254,
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
     MANDEL_BTRACE,
     },
    {				/* formula added by Arpad Fekete, method from fractint *//* 21 */
     FORMULAMAGIC,
#ifndef SLOWFUNCPTR
     carpet_calc,
     NULL,
     NULL,
     NULL,
#endif
     NULL,
     {"S.Carpet", "S.Carpet"},
     "carpet",
     {0.5, 0.5, 1.5, 1.5},
     0, 0, 1, 1,
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
     MANDEL_BTRACE,
     },
    {				/* formula added by Arpad Fekete, method from fractint *//* 22 */
     FORMULAMAGIC,
#ifndef SLOWFUNCPTR
     koch_calc,
     NULL,
     NULL,
     NULL,
#endif
     NULL,
     {"Koch Snowflake", "Koch Snowflake"},
     "koch",
     {0.0, 0.0, 2.5, 2.5},
     0, 1, 0, 0,
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
     MANDEL_BTRACE,
     },
    {				/* formula added by Z. Kovacs *//* 23 */
     FORMULAMAGIC,
#ifndef SLOWFUNCPTR
     hornflake_calc,
     NULL,
     NULL,
     NULL,
#endif
     NULL,
     {"Spidron hornflake", "Spidron hornflake"},
     "hornflake",
     {-0.75, 0, 3.8756, 3.8756},
     0, 1, 0, 0,
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
     MANDEL_BTRACE,
     },
    {				/* formula added by Z. Kovacs, originally mand6 but it was mand9 by accident *//* 24 */
     FORMULAMAGIC,
#ifndef SLOWFUNCPTR
     mand9_calc,
     mand9_peri,
     smand9_calc,
     smand9_peri,
#endif
     mand9_julia,
     {"Mandelbrot^9", "Julia^9"},
     "mandel9",
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
     
   { /* formula added by S. Bizien *//* 25 */
    FORMULAMAGIC,
#ifndef SLOWFUNCPTR
   beryl_calc,
   beryl_peri,
   NULL,
   NULL,
#endif
   NULL,
   {"Beryl", "Beryl"},
   "beryl",
   {-0.6, 0, 2, 2},
   0, 0, 1.0, 0.0,
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
    MANDEL_BTRACE,
    },   
    {				/* formula added by Arpad Fekete *//* 26 */
     FORMULAMAGIC,
#ifndef SLOWFUNCPTR
     goldsier_calc,
     NULL,
     NULL,
     NULL,
#endif
     NULL,
     {"Golden Sierpinski", "Golden Sierpinski"},
     "goldsier",
     {0.5, 0.43, 1.5, 1.0},
     0, 0, 0.5, 0.8660254,
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
     MANDEL_BTRACE,
     },
    {				/* formula added by Arpad Fekete *//* 27 */
     FORMULAMAGIC,
#ifndef SLOWFUNCPTR
     circle7_calc,
     NULL,
     NULL,
     NULL,
#endif
     NULL,
     {"Circle 7", "Circle 7"},
     "circle7",
     {0.0, 0.0, 2.5, 2.5},
     0, 0, 0.0, 0.0,
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
     MANDEL_BTRACE,
     },
    {				/* formula added by Arpad Fekete *//* 28 */
     FORMULAMAGIC,
#ifndef SLOWFUNCPTR
     symbarn_calc,
     NULL,
     NULL,
     NULL,
#endif
     symbarn_julia,
     {"Sym. Barnsley M.", "Sym. Barnsley"},
     "symbarn",
     {0.0, 0.0, 8.0, 1.0},
     0, 0, 1.3, 1.3,
     /* Arpad hasn't created the symmetry properties, */
     /* because he doesn't considered it to be important */
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
     MANDEL_BTRACE,
     }

#ifdef SFFE_USING
    , {				/* formula added by M. Malczak - SFFE *//* 29 */
       FORMULAMAGIC,
#ifndef SLOWFUNCPTR
       sffe_calc,
       NULL,
       NULL,
       NULL,
#endif
       sffe_julia,
       {"User defined", "User defined"},
       "user",
       /*{0.5, -2.0, 1.25, -1.25}, */
	/*{-0.75, 0.0, 1, 1},*/
	/* 2009-08-01 JB Langston
	 * Changed default zoom level to match Mandelbrot
	 */
	{-0.75, 0.0, 2.5, 2.5},
	0, 1, 0.0, 0.0,
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
       MANDEL_BTRACE | SFFE_FRACTAL,
       }
#endif

};

#ifdef SLOWFUNCPTR
unsigned int
calculateswitch(register number_t x1, register number_t y1,
		register number_t x2, register number_t y2,
		int periodicity)
{
    if (periodicity && cfractalc.periodicity)
	if (cfractalc.coloringmode == 9)
	    switch (cfractalc.currentformula - formulas) {	/* periodicity checking and smoothmode SPERI */
	    case 0:
		return (smand_peri(x1, y1, x2, y2));
		break;
	    case 1:
		return (smand3_peri(x1, y1, x2, y2));
		break;
	    case 2:
		return (smand4_peri(x1, y1, x2, y2));
		break;
	    case 3:
		return (smand5_peri(x1, y1, x2, y2));
		break;
	    case 4:
		return (smand6_peri(x1, y1, x2, y2));
		break;
	    case 5:
		return (newton_calc(x1, y1, x2, y2));
		break;
	    case 6:
		return (newton4_calc(x1, y1, x2, y2));
		break;
	    case 7:
		return (sbarnsley1_calc(x1, y1, x2, y2));
		break;
	    case 8:
		return (sbarnsley2_calc(x1, y1, x2, y2));
		break;
	    case 9:
		return (sbarnsley3_calc(x1, y1, x2, y2));
		break;
	    case 10:
		return (octo_calc(x1, y1, x2, y2));
		break;
	    case 11:
		return (sphoenix_peri(x1, y1, x2, y2));
		break;
	    case 12:
		return (smagnet_peri(x1, y1, x2, y2));
		break;
	    case 13:
		return (smagnet2_peri(x1, y1, x2, y2));
		break;
	    case 14:
		return (strice_peri(x1, y1, x2, y2));
		break;
	    case 15:
		return (scatseye_peri(x1, y1, x2, y2));
		break;
	    case 16:
		return (smbar_peri(x1, y1, x2, y2));
		break;
	    case 17:
		return (smlambda_peri(x1, y1, x2, y2));
		break;
	    case 18:
		return (smanowar_calc(x1, y1, x2, y2));
		break;
	    case 19:
		return (sspider_calc(x1, y1, x2, y2));
		break;
	    case 20:
		return (sier_calc(x1, y1, x2, y2));
		break;
	    case 21:
		return (carpet_calc(x1, y1, x2, y2));
		break;
	    case 22:
		return (koch_calc(x1, y1, x2, y2));
		break;
	    case 23:
		return (hornflake_calc(x1, y1, x2, y2));
		break;
	    case 24:
		return (smand9_peri(x1, y1, x2, y2));
		break;
	    case 25:
		return (beryl_calc(x1, y1, x2, y2));
		break;
            case 26:
		return (goldsier_calc(x1, y1, x2, y2));
		break;
            case 27:
		return (circle7_calc(x1, y1, x2, y2));
		break;
            case 28:
		return (symbarn_calc(x1, y1, x2, y2));
		break;
		
#ifdef SFFE_USING
	    case 29:
		return (sffe_calc(x1, y1, x2, y2));
		break;
#endif
	} else
	    switch (cfractalc.currentformula - formulas) {	/* periodicity checking and no smoothmode PERI */
	    case 0:
		return (mand_peri(x1, y1, x2, y2));
		break;
	    case 1:
		return (mand3_peri(x1, y1, x2, y2));
		break;
	    case 2:
		return (mand4_peri(x1, y1, x2, y2));
		break;
	    case 3:
		return (mand5_peri(x1, y1, x2, y2));
		break;
	    case 4:
		return (mand6_peri(x1, y1, x2, y2));
		break;
	    case 5:
		return (newton_calc(x1, y1, x2, y2));
		break;
	    case 6:
		return (newton4_calc(x1, y1, x2, y2));
		break;
	    case 7:
		return (barnsley1_calc(x1, y1, x2, y2));
		break;
	    case 8:
		return (barnsley2_calc(x1, y1, x2, y2));
		break;
	    case 9:
		return (barnsley3_calc(x1, y1, x2, y2));
		break;
	    case 10:
		return (octo_calc(x1, y1, x2, y2));
		break;
	    case 11:
		return (phoenix_peri(x1, y1, x2, y2));
		break;
	    case 12:
		return (magnet_peri(x1, y1, x2, y2));
		break;
	    case 13:
		return (magnet2_peri(x1, y1, x2, y2));
		break;
	    case 14:
		return (trice_peri(x1, y1, x2, y2));
		break;
	    case 15:
		return (catseye_peri(x1, y1, x2, y2));
		break;
	    case 16:
		return (mbar_peri(x1, y1, x2, y2));
		break;
	    case 17:
		return (mlambda_peri(x1, y1, x2, y2));
		break;
	    case 18:
		return (manowar_calc(x1, y1, x2, y2));
		break;
	    case 19:
		return (spider_calc(x1, y1, x2, y2));
		break;
	    case 20:
		return (sier_calc(x1, y1, x2, y2));
		break;
	    case 21:
		return (carpet_calc(x1, y1, x2, y2));
		break;
	    case 22:
		return (koch_calc(x1, y1, x2, y2));
		break;
	    case 23:
		return (hornflake_calc(x1, y1, x2, y2));
		break;
	    case 24:
		return (mand9_peri(x1, y1, x2, y2));
		break;
	    case 25:
		return (beryl_peri(x1, y1, x2, y2));
		break;
            case 26:
		return (goldsier_calc(x1, y1, x2, y2));
		break;
            case 27:
		return (circle7_calc(x1, y1, x2, y2));
		break;
            case 28:
		return (symbarn_calc(x1, y1, x2, y2));
		break;
		
#ifdef SFFE_USING
	    case 29:
		return (sffe_calc(x1, y1, x2, y2));
		break;
#endif
    } else if (cfractalc.coloringmode == 9)
	switch (cfractalc.currentformula - formulas) {	/* no periodicity checking and smoothmode SCALC */
	case 0:
	    return (smand_calc(x1, y1, x2, y2));
	    break;
	case 1:
	    return (smand3_calc(x1, y1, x2, y2));
	    break;
	case 2:
	    return (smand4_calc(x1, y1, x2, y2));
	    break;
	case 3:
	    return (smand5_calc(x1, y1, x2, y2));
	    break;
	case 4:
	    return (smand6_calc(x1, y1, x2, y2));
	    break;
	case 5:
	    return (newton_calc(x1, y1, x2, y2));
	    break;
	case 6:
	    return (newton4_calc(x1, y1, x2, y2));
	    break;
	case 7:
	    return (sbarnsley1_calc(x1, y1, x2, y2));
	    break;
	case 8:
	    return (sbarnsley2_calc(x1, y1, x2, y2));
	    break;
	case 9:
	    return (sbarnsley3_calc(x1, y1, x2, y2));
	    break;
	case 10:
	    return (socto_calc(x1, y1, x2, y2));
	    break;
	case 11:
	    return (sphoenix_calc(x1, y1, x2, y2));
	    break;
	case 12:
	    return (smagnet_calc(x1, y1, x2, y2));
	    break;
	case 13:
	    return (smagnet2_calc(x1, y1, x2, y2));
	    break;
	case 14:
	    return (strice_calc(x1, y1, x2, y2));
	    break;
	case 15:
	    return (scatseye_calc(x1, y1, x2, y2));
	    break;
	case 16:
	    return (smbar_calc(x1, y1, x2, y2));
	    break;
	case 17:
	    return (smlambda_calc(x1, y1, x2, y2));
	    break;
	case 18:
	    return (smanowar_calc(x1, y1, x2, y2));
	    break;
	case 19:
	    return (sspider_calc(x1, y1, x2, y2));
	    break;
	case 20:
	    return (sier_calc(x1, y1, x2, y2));
	    break;
	case 21:
	    return (carpet_calc(x1, y1, x2, y2));
	    break;
	case 22:
	    return (koch_calc(x1, y1, x2, y2));
	    break;
	case 23:
	    return (hornflake_calc(x1, y1, x2, y2));
	    break;
	case 24:
	    return (smand6_calc(x1, y1, x2, y2));
	    break;
	case 25:
	    return (beryl_calc(x1, y1, x2, y2));
	    break;
        case 26:
            return (goldsier_calc(x1, y1, x2, y2));
            break;
        case 27:
            return (circle7_calc(x1, y1, x2, y2));
            break;
        case 28:
            return (symbarn_calc(x1, y1, x2, y2));
            break;
	    
#ifdef SFFE_USING
	case 29:
	    return (sffe_calc(x1, y1, x2, y2));
	    break;
#endif
    } else
	switch (cfractalc.currentformula - formulas) {	/* no periodicity checking and no smoothmode CALC */
	case 0:
	    return (mand_calc(x1, y1, x2, y2));
	    break;
	case 1:
	    return (mand3_calc(x1, y1, x2, y2));
	    break;
	case 2:
	    return (mand4_calc(x1, y1, x2, y2));
	    break;
	case 3:
	    return (mand5_calc(x1, y1, x2, y2));
	    break;
	case 4:
	    return (mand6_calc(x1, y1, x2, y2));
	    break;
	case 5:
	    return (newton_calc(x1, y1, x2, y2));
	    break;
	case 6:
	    return (newton4_calc(x1, y1, x2, y2));
	    break;
	case 7:
	    return (barnsley1_calc(x1, y1, x2, y2));
	    break;
	case 8:
	    return (barnsley2_calc(x1, y1, x2, y2));
	    break;
	case 9:
	    return (barnsley3_calc(x1, y1, x2, y2));
	    break;
	case 10:
	    return (octo_calc(x1, y1, x2, y2));
	    break;
	case 11:
	    return (phoenix_calc(x1, y1, x2, y2));
	    break;
	case 12:
	    return (magnet_calc(x1, y1, x2, y2));
	    break;
	case 13:
	    return (magnet2_calc(x1, y1, x2, y2));
	    break;
	case 14:
	    return (trice_calc(x1, y1, x2, y2));
	    break;
	case 15:
	    return (catseye_calc(x1, y1, x2, y2));
	    break;
	case 16:
	    return (mbar_calc(x1, y1, x2, y2));
	    break;
	case 17:
	    return (mlambda_calc(x1, y1, x2, y2));
	    break;
	case 18:
	    return (manowar_calc(x1, y1, x2, y2));
	    break;
	case 19:
	    return (spider_calc(x1, y1, x2, y2));
	    break;
	case 20:
	    return (sier_calc(x1, y1, x2, y2));
	    break;
	case 21:
	    return (carpet_calc(x1, y1, x2, y2));
	    break;
	case 22:
	    return (koch_calc(x1, y1, x2, y2));
	    break;
	case 23:
	    return (hornflake_calc(x1, y1, x2, y2));
	    break;
	case 24:
	    return (mand9_calc(x1, y1, x2, y2));
	    break;
	case 25:
	    return (beryl_peri(x1, y1, x2, y2));
	    break;
        case 26:
            return (goldsier_calc(x1, y1, x2, y2));
	    break;
        case 27:
            return (circle7_calc(x1, y1, x2, y2));
	    break;
        case 28:
            return (symbarn_calc(x1, y1, x2, y2));
	    break;
	    
#ifdef SFFE_USING
	case 29:
	    return (sffe_calc(x1, y1, x2, y2));
	    break;
#endif
	}
    return 0;
}
#endif

const struct formula *currentformula;
const int nformulas = sizeof(formulas) / sizeof(struct formula);
const int nmformulas = 16; // Is this correct here? -- Zoltan, 2009-07-30
