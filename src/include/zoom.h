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
#ifndef ZOOM_H
#define ZOOM_H
#ifdef __cplusplus
extern "C"
{
#endif


#define LOWQUALITY (1<<30)
#define ZOOMMASK (LOWQUALITY|UNCOMPLETTE)

  extern CONST struct filteraction zoom_filter;
  extern struct filter cfilter;
  extern CONST struct filteraction subwindow_filter;
  void subwindow_setsecond (struct filter *f, struct filter *f1);

#ifdef __cplusplus
}
#endif
#endif				/* ZOOM_H */
