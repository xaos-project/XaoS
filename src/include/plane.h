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
#ifndef PLANE_H
#define PLANE_H

#include <config.h>
#include <zoom.h>

#ifdef __cplusplus
extern "C"
{
#endif


/*#define PLANES 7*/
#define P_MU 0
#define P_INVERSE 1
#define P_PARABOL 2
#define P_LAMBDA 3
#define P_INVLAMBDA 4
#define P_TRANLAMBDA 5
#define P_MEREBERG 6
  extern void recalculate (int plane, number_t *, number_t *) REGISTERS (3);

  extern CONST char *CONST planename[];

#ifdef __cplusplus
}
#endif
#endif				/* PLANE_H */
