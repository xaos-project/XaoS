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

/* Simplified version. See XaoS2.2 for full version of this library.
 * It caused problems (overflows) at certain preprocesors
 */
#ifndef COMPLEX_H
#define COMPLEX_H
#include <config.h>

#define c_add(ar,ai,br,bi,or,oi) ((or)=(ar)+(br),(oi)=(ai)+(bi))
#define c_mul(ar,ai,br,bi,or,oi) ((or)=(ar)*(br)-(ai)*(bi),(oi)=((ar)*(bi))+((ai)*(br)))

#define c_sub(ar,ai,br,bi,or,oi) ((or)=(ar)-(br),(oi)=(ai)-(bi))

#define c_div_rp(ar,ai,br,bi) (((ar) * (br) + (ai) * (bi))/ ((bi) * (bi) + (br) * (br)))
#define c_div_ip(ar,ai,br,bi) ((-(ar) * (bi) + (ai) * (br)) / ((br) * (br) + (bi) * (bi)))
#define c_div(ar,ai,br,bi,or,oi) ((or)=c_div_rp(ar,ai,br,bi),(oi)=c_div_ip(ar,ai,br,bi))

#define c_pow2_rp(ar,ai) ((ar)*(ar)-(ai)*(ai))
#define c_pow2_ip(ar,ai) (2*(ar)*(ai))
#define c_pow2(ar,ai,or,oi) ((or)=c_pow2_rp(ar,ai),(oi)=c_pow2_ip(ar,ai))


#define c_pow3_rp(ar,ai) ((ar)*(ar)*(ar)-3*(ar)*(ai)*(ai))
#define c_pow3_ip(ar,ai) (3*(ar)*(ar)*(ai)-(ai)*(ai)*(ai))
#define c_pow3(ar,ai,or,oi) ((or)=c_pow3_rp(ar,ai),(oi)=c_pow3_ip(ar,ai))

#define c_pow4_rp(ar,ai) ((ar)*(ar)*(ar)*(ar)-6*(ar)*(ar)*(ai)*(ai)+(ai)*(ai)*(ai)*(ai))
#define c_pow4_ip(ar,ai) (4*(ar)*(ar)*(ar)*(ai)-4*(ar)*(ai)*(ai)*(ai))
#define c_pow4(ar,ai,or,oi) ((or)=c_pow4_rp(ar,ai),(oi)=c_pow4_ip(ar,ai))


#ifndef __GNUC__
#ifdef INLINEFABS
#define myabs(x) INLINEFABS(x)
#else
#define myabs(x) ((x)>0?(x):-(x))
#endif
#else
#include <gccbuild.h>
#endif

#endif /* COMPLEX_H */
