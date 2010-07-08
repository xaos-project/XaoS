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
/*drivers registry */
#include <config.h>
#include <ui.h>
extern const struct ui_driver svga_driver, x11_driver, dog_driver,
    plan9_driver, plan9_driver, mac_driver, mac_full_driver, osx_driver,
    osx_fullscreen_driver, os2vio_driver, cocoa_driver, qt_driver,
    cocoa_fullscreen_driver, be_driver, be_direct_driver, be_screen_driver,
    aalib_driver, gtk_driver, ggi_driver, win32_driver, dxw_driver,
    dxf_driver, DGA_driver;
const struct ui_driver *const drivers[] = {
#ifdef WIN32_DRIVER
    &win32_driver,
#endif
#ifdef DDRAW_DRIVER
    &dxw_driver,
    &dxf_driver,
#endif
#ifdef SVGA_DRIVER
    &svga_driver,
#endif
#ifdef X11_DRIVER
    &x11_driver,
#endif
#ifdef DGA_DRIVER
    &DGA_driver,
#endif
#ifdef GTK_DRIVER
    &gtk_driver,
#endif
#ifdef GGI_DRIVER
    &ggi_driver,
#endif
#ifdef OS2VIO_DRIVER
    &os2vio_driver,
#endif
#ifdef DOG_DRIVER
    &dog_driver,
#endif
#ifdef PLAN9_DRIVER
    &plan9_driver,
#endif
#ifdef AA_DRIVER
    &aalib_driver,
#endif
#ifdef _MAC
    &mac_driver,
    &mac_full_driver,
#endif
#ifdef OSX_DRIVER
    &osx_driver,
    &osx_fullscreen_driver,
#endif
#ifdef COCOA_DRIVER
    &cocoa_driver,
    &cocoa_fullscreen_driver,
#endif
#ifdef QT_DRIVER
    &qt_driver,
#endif
#ifdef BEOS_DRIVER
    &be_driver,
    &be_direct_driver,
    &be_screen_driver,
#endif
    NULL
};

const int ndrivers = (sizeof(drivers) / sizeof(*drivers) - 1);
