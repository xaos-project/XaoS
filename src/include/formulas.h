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
#ifndef FORMULAS_H
#define FORMULAS_H

#include <config.h>
#include <fractal.h>

#ifdef SFFE_USING
#include "sffe.h"
#ifdef SFFE_CMPLX_ASM
#include "sffe_cmplx_asm.h"
#elif SFFE_CMPLX_GSL
#include "sffe_cmplx_gsl.h"
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef SFFE_USING
/*extern sffe* parser;
   extern sffe* pinit; */
extern cmplx Z;
extern cmplx pZ;
extern cmplx C;
#endif

#define MAX_LAMBDA 2


#define FORMULAMAGIC 1121

extern const char *const incolorname[];
extern const struct formula formulas[];
extern const char *const outcolorname[];
extern const char *const tcolorname[];
extern const int nformulas;
extern const int nmformulas;

#ifdef __cplusplus
}
#endif
#endif				/* FORMULAS_H */
