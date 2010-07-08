/* 
 *     XaoS, a fast portable realtime fractal zoomer 
 *                  Copyright (C) 1996 by
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

#include <config.h>
#ifndef _plan9_
#include <fconfig.h>
#include <string.h>
#include <stdlib.h>
#ifdef DESTICKY
#include <unistd.h>
#endif
#else
#include <u.h>
#include <libc.h>
#endif
#include <filter.h>
#include <ui_helper.h>
#include <ui.h>
#include <param.h>
#include <xmenu.h>
#include <plane.h>
#include <xerror.h>
#include "uiint.h"


static char *defrender = NULL;
static const char *rbasename = "anim";
static int alias = 0;
static int slowmode = 0;
static char *imgtype;
static char *defsize;
static float framerate;
static int letterspersec = 20;
static int defvectors;
static int iframedist;
const struct params ui_fractal_params[] = {

    {"", P_HELP, NULL, "Animation rendering:"},
    {"-render", P_STRING, &defrender,
     "Render animation into seqence of .png files"},
    {"-basename", P_STRING, &rbasename,
     "Name for .png files (XaoS will add 4 digit number and extension"},
    {"-size", P_STRING, &defsize, "widthxheight"},
    {"-renderimage", P_STRING, &imgtype, "256 or truecolor"},
    {"-renderframerate", P_FLOAT, &framerate, "framerate"},
    {"-antialiasing", P_SWITCH, &alias,
     "Perform antialiasing (slow, requires quite lot of memory)"},
    {"-alwaysrecalc", P_SWITCH, &slowmode,
     "Always recalculate whole image (slowes down rendering, increases quality)"},
    {"-rendervectors", P_SWITCH, &defvectors,
     "Render motion vectors (should be used for MPEG encoding)"},
    {"-iframedist", P_NUMBER, &iframedist,
     "Recommended distance between I frames in pat file (should be used for MPEG encoding)"},
    {NULL, 0, NULL, NULL}
};

int ui_dorender_params(void)
{
    if (defrender != NULL) {
	int imagetype = TRUECOLOR24;
	int width = 640, height = 480;
#ifdef DESTICKY
	seteuid(getuid());	/* Don't need supervisor rights anymore. */
	setegid(getgid());
#endif
#ifndef STRUECOLOR24
	if (imagetype == TRUECOLOR24)
	    imagetype = TRUECOLOR;
#endif
	if (imgtype != NULL) {
	    if (!strcmp("256", imgtype))
		imagetype = C256;
	    else if (!strcmp("truecolor", imgtype)) {
		x_fatalerror("Unknown image type:%s", imgtype);
	    }
	}
	if (defsize != NULL &&
	    !sscanf(defsize, "%ix%i", &width, &height) &&
	    (width <= 0 || height <= 0)) {
	    x_fatalerror("Invalid size (use for example 320x200");
	}
	if (framerate <= 0)
	    framerate = 30;
	uih_renderanimation(NULL, rbasename, defrender, width, height,
			    ui_get_windowwidth(width) / width,
			    ui_get_windowheight(height) / height,
			    (int) (1000000 / framerate), imagetype, alias,
			    slowmode, letterspersec, NULL, defvectors,
			    iframedist);
	return 1;
    }
    return 0;
}
