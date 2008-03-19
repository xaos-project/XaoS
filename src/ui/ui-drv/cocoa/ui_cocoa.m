/*
 *     XaoS, a fast portable realtime fractal zoomer 
 *                  Copyright (C) 1996 by
 *
 *      Jan Hubicka          (hubicka@paru.cas.cz)
 *      Thomas Marsh         (tmarsh@austin.ibm.com)
 *
 *    Cocoa Driver by J.B. Langston III (jb-langston@austin.rr.com)
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
#import "AppController.h"

#include "ui.h"

struct ui_driver osx_driver;

static void
osx_printText(int x, int y, CONST char *text)
{
	[controller printText:text atX:x y:y];
}

static void
osx_refreshDisplay()
{
    [controller refreshDisplay];
}

static void
osx_flipBuffers ()
{
	[controller flipBuffers];
}

void
osx_freeBuffers (char *b1, char *b2)
{
	[controller freeBuffers];
}

int
osx_allocBuffers (char **b1, char **b2)
{
	return [controller allocBuffer1:b1 buffer2:b2];
}

static void
osx_getImageSize (int *w, int *h)
{
	[controller getWidth:w height:h];
}

static void
osx_processEvents (int wait, int *mx, int *my, int *mb, int *k)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
	//NSDate *eventDate = wait ? [NSDate distantFuture] : [NSDate distantPast];
	
	NSEvent *event = [NSApp nextEventMatchingMask: NSAnyEventMask
										untilDate: nil
										   inMode: NSDefaultRunLoopMode
										  dequeue: YES];
	
	if (event != nil)
	{
	    [NSApp sendEvent: event];
	}
	
	[pool release];
	
	[controller getMouseX:mx mouseY:my mouseButton:mb keys:k];
}


static int
osx_initDriver ()
{	
	osx_init(0);
    return ( /*1 for sucess 0 for fail */ 1);
}

static int
osx_initDriverFull ()
{	
	osx_init(1);
    return ( /*1 for sucess 0 for fail */ 1);
}

static void
osx_uninitDriver ()
{
	//[controller unInitApp];
}

static void
osx_getMouse (int *x, int *y, int *b)
{
	[controller getMouseX:x mouseY:y mouseButton:b];
}


static void
osx_setMouseType (int type)
{
	[controller setMouseType:type];
}

void osx_buildMenu (struct uih_context *uih, CONST char *name)
{
	[controller buildMenuWithContext:uih name:name];
}

void osx_toggleMenu (struct uih_context *uih, CONST char *name)
{
	[controller toggleMenuWithContext:uih name:name];
}

void osx_menu (struct uih_context *c, CONST char *name)
{
}


void osx_showDialog (struct uih_context *c, CONST char *name)
{
	[controller showDialogWithContext:c name:name];
}

void osx_showHelp (struct uih_context *c, CONST char *name)
{
	[controller showHelpWithContext:c name:name];
}

int main(int argc, char* argv[])
{
	[NSApplication sharedApplication];
	[NSBundle loadNibNamed:@"MainMenu" owner:NSApp];
	[NSApp finishLaunching];
	
	return XaoS_main(argc, argv);
}

struct gui_driver osx_gui_driver = {
    osx_buildMenu,	// setrootmenu
    osx_toggleMenu,	// enabledisable
    NULL,		// menu
    osx_showDialog,	// dialog
    NULL	// help
};


static struct params osx_params[] = {
	{NULL, 0, NULL, NULL}
};

struct ui_driver osx_driver = {
    /* name */          "Mac OS X Driver",
    /* init */          osx_initDriver,
    /* getsize */       osx_getImageSize,
    /* processevents */ osx_processEvents,
    /* getmouse */      osx_getMouse,
    /* uninit */        osx_uninitDriver,
    /* set_color */     NULL,
    /* set_range */     NULL,
    /* print */         osx_printText,
    /* display */       osx_refreshDisplay,
    /* alloc_buffers */ osx_allocBuffers,
    /* free_buffers */  osx_freeBuffers,
    /* filp_buffers */  osx_flipBuffers,
    /* mousetype */     osx_setMouseType,
    /* flush */         NULL,
    /* textwidth */     12,
    /* textheight */    12,
    /* params */        osx_params,
    /* flags */         RESOLUTION | PIXELSIZE,
    /* width */         0.01, 
    /* height */        0.01,
    /* maxwidth */      0, 
    /* maxheight */     0,
    /* imagetype */     UI_TRUECOLOR,
    /* palettestart */  0, 
    /* paletteend */    256, 
    /* maxentries */    255,
#if __BIG_ENDIAN__
    /* rmask */         0xff000000,
    /* gmask */         0x00ff0000,
    /* bmask */         0x0000ff00,
#else
    /* rmask */         0x000000ff,
    /* gmask */         0x0000ff00,
    /* bmask */         0x00ff0000,
#endif
    /* gui_driver */    &osx_gui_driver
};

struct ui_driver osx_fullscreen_driver = {
    /* name */          "Mac OS X Fullscreen Driver",
    /* init */          osx_initDriverFull,
    /* getsize */       osx_getImageSize,
    /* processevents */ osx_processEvents,
    /* getmouse */      osx_getMouse,
    /* uninit */        osx_uninitDriver,
    /* set_color */     NULL,
    /* set_range */     NULL,
    /* print */         osx_printText,
    /* display */       osx_refreshDisplay,
    /* alloc_buffers */ osx_allocBuffers,
    /* free_buffers */  osx_freeBuffers,
    /* filp_buffers */  osx_flipBuffers,
    /* mousetype */     osx_setMouseType,
    /* flush */         NULL,
    /* textwidth */     12,
    /* textheight */    12,
    /* params */        osx_params,
    /* flags */         RESOLUTION | PIXELSIZE,
    /* width */         0.01, 
    /* height */        0.01,
    /* maxwidth */      0, 
    /* maxheight */     0,
    /* imagetype */     UI_TRUECOLOR,
    /* palettestart */  0, 
    /* paletteend */    256, 
    /* maxentries */    255,
#if __BIG_ENDIAN__
    /* rmask */         0xff000000,
    /* gmask */         0x00ff0000,
    /* bmask */         0x0000ff00,
#else
    /* rmask */         0x000000ff,
    /* gmask */         0x0000ff00,
    /* bmask */         0x00ff0000,
#endif
    /* gui_driver */    &osx_gui_driver
};

int osx_init (int fullscreen)
{
    struct ui_driver    *driver;
    
    driver = fullscreen ? &osx_fullscreen_driver : &osx_driver;
}

/* DONT FORGET TO ADD DOCUMENTATION ABOUT YOUR DRIVER INTO xaos.hlp FILE!*/
