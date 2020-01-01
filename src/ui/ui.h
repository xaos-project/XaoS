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
#ifndef UI_H
#define UI_H
/*
#include "xio.h"
#include "zoom.h"
*/
#include "config.h"
#include "param.h"
#include "xmenu.h"
#ifdef __cplusplus
extern "C" {
#endif

void ui_call_resize(void);
void ui_quit(int);
void ui_menuactivate(const menuitem *item, dialogparam *d);
int ui_key(int);

#ifdef __cplusplus
}
#endif
#endif /* UI_H */
