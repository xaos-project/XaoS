/*
 *     XaoS, a fast portable realtime fractal zoomer 
 *                  Copyright ¬© 1996,1997 by
 *
 *      Jan Hubicka          (hubicka@paru.cas.cz)
 *      Thomas Marsh         (tmarsh@austin.ibm.com)
 *	
 *	Mac OS X Driver by J.B. Langston (jb-langston at austin dot rr dot com)
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

#ifndef OSXCOMMON_H
#define OSXCOMMON_H

#include <Carbon/Carbon.h>

extern int osx_window_width;
extern int osx_window_height;

extern int osx_mouse_x;
extern int osx_mouse_y;
extern int osx_mouse_buttons;
extern int osx_keys;

extern WindowRef osx_window;

OSStatus InstallEventHandlers( void );
OSStatus UninstallEventHandlers( void );

#endif