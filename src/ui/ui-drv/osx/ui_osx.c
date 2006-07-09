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

#define kDefaultWindowWidth 640
#define kDefaultWindowHeight 480
#define kPixelDepth 32
#define kWindowTitle "XaoS"

int osx_window_width = 0;
int osx_window_height = 0;

int osx_mouse_x = 0;
int osx_mouse_y = 0;
int osx_mouse_buttons = 0;
int osx_keys = 0;

WindowRef osx_window;

static int osx_currentbuff;
static GWorldPtr osx_offscreen[2];
static RGBColor mac_text_color = { 0xFFFF, 0xFFFF, 0xFFFF };	// white

struct ui_driver osx_driver;

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
    
    // An update event occurred, so just copy from our buffer to the osx_window.
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
    for (i = 0; i < 2; i++)
    {
        if (osx_offscreen[i] != nil)
        {
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
    
    if (err1 != noErr || err2 != noErr)
    {
        if (osx_offscreen[0] != nil)
        {
            DisposeGWorld (osx_offscreen[0]);
            osx_offscreen[0] = nil;
        }
        if (osx_offscreen[1] != nil)
        {
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
	
	for (i = 0; i < 2; i++)
    {
		if (osx_offscreen[i] != nil)
			DisposeGWorld (osx_offscreen[i]);
    }
	
	DisposeWindow (osx_window);
	
	ShowMenuBar ();
}

static int
osx_init ()
{
    WindowAttributes    windowAttrs;
    CGRect              displayRect;
    Rect                contentRect; 
    CFStringRef         windowTitle; 
    CGDirectDisplayID   mainDisplay;
	
    // Determine pixel depth and configure driver
    mainDisplay = CGMainDisplayID();
    
    displayRect = CGDisplayBounds(mainDisplay);
    osx_window_height = kDefaultWindowHeight;
    osx_window_width = kDefaultWindowWidth;
    
	// From: file:///Developer/ADC%20Reference%20Library/documentation/Carbon/Conceptual/HandlingWindowsControls/hitb-wind_cont_tasks/chapter_3_section_4.html#//apple_ref/doc/uid/TP30001004-CH206-TPXREF149
	
    windowAttrs = kWindowStandardDocumentAttributes 
        | kWindowStandardHandlerAttribute 
        | kWindowInWindowMenuAttribute; 
    
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
	
	if (ReceiveNextEvent(0, NULL, wait ? kEventDurationForever : kEventDurationNoWait, true, &theEvent) == noErr)
	{
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
	{NULL, 0, NULL, NULL}
};

struct ui_driver osx_driver = {
    /* name */          "Mac OS X Driver",
    /* init */          osx_init,
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
    /* flags */         RESOLUTION | PIXELSIZE,
    /* width */         0.01, 
    /* height */        0.01,
    /* maxwidth */      0, 
    /* maxheight */     0,
    /* imagetype */     UI_TRUECOLOR,
    /* palettestart */  0, 
    /* paletteend */    255, 
    /* maxentries */    256,
    /* rmask */         0x00ff0000,
    /* gmask */         0x0000ff00,
    /* bmask */         0x000000ff,
    /* gui_driver */    &osx_gui_driver
};

/* DONT FORGET TO ADD DOCUMENTATION ABOUT YOUR DRIVER INTO xaos.hlp FILE!*/

#endif
