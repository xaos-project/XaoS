/*
 *     XaoS, a fast portable realtime fractal zoomer 
 *                  Copyright © 1996,1997 by
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

/*
 *	Portions of this file adapted from:
 *	ui_mac.c
 *	
 *	Xaos Fractal Viewer
 *	Macintosh Driver 2.0
 *	
 *	Written by Dominic Mazzoni (dmazzoni@cs.cmu.edu)
 *	based on the Macintosh Driver (version 1.31) by
 *	Tapio K. Vocadlo of Ottawa, Canada (taps@rmx.com).
 */

#include "aconfig.h"
#ifdef OSX_DRIVER

#include "osxcommon.h"
#include "ui.h"

#define kPixelDepth 32
#define kWindowTitle "XaoS"

int osx_window_width = 0;
int osx_window_height = 0;

int osx_mouse_x = 0;
int osx_mouse_y = 0;
int osx_mouse_buttons = 0;
int osx_keys = 0;

WindowRef osx_window;

static char *osx_window_size = "640x480";
static int osx_autoscreensize = 0;

static int osx_currentbuff;
static GWorldPtr osx_offscreen[2];
static RGBColor mac_text_color = { 0xFFFF, 0xFFFF, 0xFFFF };	// white

struct ui_driver osx_driver;
struct ui_driver osx_fullscreen_driver;

static void osx_display ();

static void
osx_print (int x, int y, CONST char *text)
{
	static short lastY = 32000;
	
	// Clear mac_screen if cursor has moved up or on same line.
	if (y <= lastY)
		osx_display ();
	lastY = y;
	
	MoveTo (x + 3, y + 12);
	RGBForeColor (&mac_text_color);
	DrawText (text, 0, strlen (text));
	
	ForeColor (blackColor);	// so copybits will work
}

static void
osx_display ()
{
    PixMapHandle pmap;
    Rect tempRect;
    
    if (osx_offscreen[osx_currentbuff] == nil)
        return;
    
    // An update event occurred, so just copy from our buffer to osx_window
    pmap = GetGWorldPixMap (osx_offscreen[osx_currentbuff]);
    
    CopyBits (GetPortBitMapForCopyBits(osx_offscreen[osx_currentbuff]),
              GetPortBitMapForCopyBits(GetWindowPort(osx_window)),
              GetPortBounds(osx_offscreen[osx_currentbuff], &tempRect), 
              GetPortBounds(GetWindowPort(osx_window), &tempRect),
              srcCopy, nil);    
}

static void
osx_flip_buffers ()
{
    osx_currentbuff ^= 1;
}

void
osx_free_buffers (char *b1, char *b2)
{
    int i;
    for (i = 0; i < 2; i++) {
        if (osx_offscreen[i] != nil) {
            DisposeGWorld (osx_offscreen[i]);
            osx_offscreen[i] = nil;
        }
    }
}

int
osx_alloc_buffers (char **b1, char **b2)
{
    int rowBytes = 0;
    PixMapHandle pmap;
    OSErr err1, err2;
    Rect r = { 0, 0, 0, 0 };
    
    static char *buffers[2];
    
    r.right = osx_window_width;
    r.bottom = osx_window_height;
    
    err1 = NewGWorld (&osx_offscreen[0], kPixelDepth, &r, nil, nil, 0);
    err2 = NewGWorld (&osx_offscreen[1], kPixelDepth, &r, nil, nil, 0);
    
    if (err1 != noErr || err2 != noErr) {
        if (osx_offscreen[0] != nil) {
            DisposeGWorld (osx_offscreen[0]);
            osx_offscreen[0] = nil;
        }
        if (osx_offscreen[1] != nil) {
            DisposeGWorld (osx_offscreen[1]);
            osx_offscreen[1] = nil;
        }
        return 0;
    }
    
    pmap = GetGWorldPixMap (osx_offscreen[0]);
    LockPixels (pmap);
    rowBytes = (*pmap)->rowBytes & 0x3fff;	// mask off top two bits
    buffers[0] = GetPixBaseAddr (pmap);
    
    pmap = GetGWorldPixMap (osx_offscreen[1]);
    LockPixels (pmap);
    buffers[1] = GetPixBaseAddr (pmap);
    
    *b1 = buffers[0];
    *b2 = buffers[1];
    
    osx_currentbuff = 0;
    
    return rowBytes;
}

static void
osx_getsize (int *w, int *h)
{
    *w = osx_window_width;
    *h = osx_window_height;
}

static void
osx_uninit ()
{
	int i;
	
	UninstallEventHandlers();
	
	for (i = 0; i < 2; i++) {
		if (osx_offscreen[i] != nil)
			DisposeGWorld (osx_offscreen[i]);
    }
	
	DisposeWindow (osx_window);
	
	ShowMenuBar ();
}

static int
osx_init (int fullscreen)
{
    WindowAttributes    windowAttrs;
    Rect                contentRect; 
    CFStringRef         windowTitle; 
    CGDirectDisplayID   mainDisplay;
	CGSize				screenSize;
    struct ui_driver    *driver;
    
    driver = fullscreen ? &osx_fullscreen_driver : &osx_driver;

    // Determine screen dimensions
    mainDisplay = CGMainDisplayID();
    screenSize = CGDisplayScreenSize(mainDisplay);
    driver->maxwidth = CGDisplayPixelsWide(mainDisplay);
    driver->maxheight = CGDisplayPixelsHigh(mainDisplay);
    driver->width = ((double) ((unsigned int) screenSize.width)) / driver->maxwidth / 10.0;
    driver->height = ((double) ((unsigned int) screenSize.height)) / driver->maxheight / 10.0;

    if (fullscreen) {
        HideMenuBar();
        osx_window_width = driver->maxwidth;
        osx_window_height = driver->maxheight;
    } else {
        // Read image dimensions from parameter
        if (sscanf(osx_window_size, "%dx%d", &osx_window_width, &osx_window_height) != 2)
            return 0;
    }

    // Set up the window
	// From: file:///Developer/ADC%20Reference%20Library/documentation/Carbon/Conceptual/HandlingWindowsControls/hitb-wind_cont_tasks/chapter_3_section_4.html#//apple_ref/doc/uid/TP30001004-CH206-TPXREF149
        
    if (fullscreen) {
        windowAttrs = kWindowNoTitleBarAttribute
            | kWindowStandardHandlerAttribute;
    } else {
        windowAttrs = kWindowStandardDocumentAttributes 
            | kWindowStandardHandlerAttribute 
            | kWindowInWindowMenuAttribute; 
    }
    
    SetRect (&contentRect, 0,  0,              
             osx_window_width, osx_window_height);
    
    if (CreateNewWindow (kDocumentWindowClass, windowAttrs,
						 &contentRect, &osx_window) != noErr)
		return 0;
    
    windowTitle = CFSTR(kWindowTitle); 
    
    if (SetWindowTitleWithCFString (osx_window, windowTitle) != noErr)
		return 0;
    
    CFRelease (windowTitle); 
    
    if (RepositionWindow (osx_window, NULL, kWindowCenterOnMainScreen) != noErr)
		return 0; 
	
    ShowWindow (osx_window); 
    
    SetPort (GetWindowPort(osx_window));
	
    ForeColor (blackColor);
    BackColor (whiteColor);    
    TextMode (srcOr);
	
	if (InstallEventHandlers() != noErr)
		return 0;
	
    return 1;
}

static int
osx_window_init()
{
    return osx_init(FALSE);
}

static int
osx_fullscreen_init()
{
    return osx_init(TRUE);
}


static void
osx_getmouse (int *x, int *y, int *b)
{
	*x = osx_mouse_x;
	*y = osx_mouse_y;
	*b = osx_mouse_buttons;
}

static void
osx_processevents (int wait, int *mx, int *my, int *mb, int *k)
{
	EventRef theEvent;
	EventTargetRef theTarget;
	
	theTarget = GetEventDispatcherTarget();
	
	if (ReceiveNextEvent(0, NULL, wait ? kEventDurationForever : kEventDurationNoWait, true, &theEvent) == noErr) {
		SendEventToEventTarget(theEvent , theTarget);
		ReleaseEvent(theEvent);
	}
	
	*mx = osx_mouse_x;
	*my = osx_mouse_y;
	*mb = osx_mouse_buttons;
	*k = osx_keys;
}

static void
osx_mousetype (int type)
{
}

void osx_dorootmenu (struct uih_context *uih, CONST char *name);
void osx_enabledisable (struct uih_context *uih, CONST char *name);
void osx_menu (struct uih_context *c, CONST char *name);
void osx_dialog (struct uih_context *c, CONST char *name);
void osx_help (struct uih_context *c, CONST char *name);

struct gui_driver osx_gui_driver = {
    osx_dorootmenu,		//dorootmenu
    osx_enabledisable,	//enabledisable
    NULL,				//pop-up menu
    NULL,				//osx_dialog
    NULL				//osx_help
};


static struct params osx_params[] = {
	{"", P_HELP, NULL, "Mac OS X driver options:"},
	{"-size", P_STRING, &osx_window_size, "Select size of window (WIDTHxHEIGHT)."},
	{NULL, 0, NULL, NULL}
};

struct ui_driver osx_driver = {
    /* name */          "Mac OS X Windowed Driver",
    /* init */          osx_window_init,
    /* getsize */       osx_getsize,
    /* processevents */ osx_processevents,
    /* getmouse */      osx_getmouse,
    /* uninit */        osx_uninit,
    /* set_color */     NULL,
    /* set_range */     NULL,
    /* print */         osx_print,
    /* display */       osx_display,
    /* alloc_buffers */ osx_alloc_buffers,
    /* free_buffers */  osx_free_buffers,
    /* filp_buffers */  osx_flip_buffers,
    /* mousetype */     osx_mousetype,
    /* flush */         NULL,
    /* textwidth */     12,
    /* textheight */    12,
    /* params */        osx_params,
    /* flags */         RESOLUTION | PIXELSIZE | NOFLUSHDISPLAY | PALETTE_ROTATION | ROTATE_INSIDE_CALCULATION,
    /* width */         0.0, 
    /* height */        0.0,
    /* maxwidth */      0, 
    /* maxheight */     0,
    /* imagetype */     UI_TRUECOLOR,
    /* palettestart */  0, 
    /* paletteend */    0, 
    /* maxentries */    0,
#if __BIG_ENDIAN__
    /* rmask */         0x00ff0000,
    /* gmask */         0x0000ff00,
    /* bmask */         0x000000ff,
#else
    /* rmask */         0x0000ff00,
    /* gmask */         0x00ff0000,
    /* bmask */         0xff000000,
#endif
    /* gui_driver */    &osx_gui_driver
};

struct ui_driver osx_fullscreen_driver = {
    /* name */          "Mac OS X Fullscreen Driver",
    /* init */          osx_fullscreen_init,
    /* getsize */       osx_getsize,
    /* processevents */ osx_processevents,
    /* getmouse */      osx_getmouse,
    /* uninit */        osx_uninit,
    /* set_color */     NULL,
    /* set_range */     NULL,
    /* print */         osx_print,
    /* display */       osx_display,
    /* alloc_buffers */ osx_alloc_buffers,
    /* free_buffers */  osx_free_buffers,
    /* filp_buffers */  osx_flip_buffers,
    /* mousetype */     osx_mousetype,
    /* flush */         NULL,
    /* textwidth */     12,
    /* textheight */    12,
    /* params */        osx_params,
    /* flags */         RESOLUTION | PIXELSIZE | NOFLUSHDISPLAY | FULLSCREEN | PALETTE_ROTATION | ROTATE_INSIDE_CALCULATION,
    /* width */         0.0, 
    /* height */        0.0,
    /* maxwidth */      0, 
    /* maxheight */     0,
    /* imagetype */     UI_TRUECOLOR,
    /* palettestart */  0, 
    /* paletteend */    0, 
    /* maxentries */    0,
#if __BIG_ENDIAN__
    /* rmask */         0x00ff0000,
    /* gmask */         0x0000ff00,
    /* bmask */         0x000000ff,
#else
    /* rmask */         0x0000ff00,
    /* gmask */         0x00ff0000,
    /* bmask */         0xff000000,
#endif
    /* gui_driver */    NULL
};

/* DONT FORGET TO ADD DOCUMENTATION ABOUT YOUR DRIVER INTO xaos.hlp FILE!*/

#endif

