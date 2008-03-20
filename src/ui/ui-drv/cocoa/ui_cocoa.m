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
cocoa_printText(int x, int y, CONST char *text)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	[controller printText:text atX:x y:y];
	[pool release];
}

static void
cocoa_refreshDisplay()
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    [controller refreshDisplay];
	[pool release];
}

static void
cocoa_flipBuffers ()
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	[controller flipBuffers];
	[pool release];
}

void
cocoa_freeBuffers (char *b1, char *b2)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	[controller freeBuffers];
	[pool release];
}

int
cocoa_allocBuffers (char **b1, char **b2)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    int rowLength = [controller allocBuffer1:b1 buffer2:b2];
	[pool release];
	return rowLength;
}

static void
cocoa_getImageSize (int *w, int *h)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	[controller getWidth:w height:h];
	[pool release];
}

static void
cocoa_processEvents (int wait, int *mx, int *my, int *mb, int *k)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
	//NSDate *eventDate = wait ? [NSDate distantFuture] : [NSDate distantPast];
	
	NSEvent *event = [NSApp nextEventMatchingMask: NSAnyEventMask
										untilDate: nil
										   inMode: NSDefaultRunLoopMode
										  dequeue: YES];
	
	if (event != nil) {
	    [NSApp sendEvent: event];
	}
	
	[controller getMouseX:mx mouseY:my mouseButton:mb keys:k];
    
	[pool release];
}


static int
cocoa_initDriver ()
{	
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	[pool release];
    return ( /*1 for sucess 0 for fail */ 1);
}

static int
cocoa_initDriverFull ()
{	
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	[pool release];
    return ( /*1 for sucess 0 for fail */ 1);
}

static void
cocoa_uninitDriver ()
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	//[controller unInitApp];
	[pool release];
}

static void
cocoa_getMouse (int *x, int *y, int *b)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	[controller getMouseX:x mouseY:y mouseButton:b];
	[pool release];
}


static void
cocoa_setMouseType (int type)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	[controller setMouseType:type];
	[pool release];
}

void 
cocoa_buildMenu (struct uih_context *uih, CONST char *name)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	[controller buildMenuWithContext:uih name:name];
	[pool release];
}

void 
cocoa_toggleMenu (struct uih_context *uih, CONST char *name)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	[controller toggleMenuWithContext:uih name:name];
    [pool release];
}

void 
cocoa_menu (struct uih_context *c, CONST char *name)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    //[controller showPopUpMenuWithContext:c name:name];
	[pool release];
}


void 
cocoa_showDialog (struct uih_context *c, CONST char *name)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	[controller showDialogWithContext:c name:name];
	[pool release];
}

void 
cocoa_showHelp (struct uih_context *c, CONST char *name)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	[controller showHelpWithContext:c name:name];
	[pool release];
}

int 
main(int argc, char* argv[])
{
	[NSApplication sharedApplication];
	[NSBundle loadNibNamed:@"MainMenu" owner:NSApp];
	[NSApp finishLaunching];
	
	return MAIN_FUNCTION(argc, argv);
}

struct gui_driver cocoa_gui_driver = {
    cocoa_buildMenu,	// setrootmenu
    cocoa_toggleMenu,	// enabledisable
    NULL,		// menu
    cocoa_showDialog,	// dialog
    cocoa_showHelp	// help
};


static struct params cocoa_params[] = {
	{NULL, 0, NULL, NULL}
};

struct ui_driver cocoa_driver = {
    /* name */          "Mac OS X (Cocoa) Driver",
    /* init */          cocoa_initDriver,
    /* getsize */       cocoa_getImageSize,
    /* processevents */ cocoa_processEvents,
    /* getmouse */      cocoa_getMouse,
    /* uninit */        cocoa_uninitDriver,
    /* set_color */     NULL,
    /* set_range */     NULL,
    /* print */         cocoa_printText,
    /* display */       cocoa_refreshDisplay,
    /* alloc_buffers */ cocoa_allocBuffers,
    /* free_buffers */  cocoa_freeBuffers,
    /* filp_buffers */  cocoa_flipBuffers,
    /* mousetype */     cocoa_setMouseType,
    /* flush */         NULL,
    /* textwidth */     12,
    /* textheight */    12,
    /* params */        cocoa_params,
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
    /* gui_driver */    &cocoa_gui_driver
};

/* DONT FORGET TO ADD DOCUMENTATION ABOUT YOUR DRIVER INTO xaos.hlp FILE!*/
