/*

	ui_mac.c
	
	Xaos Fractal Viewer
	Macintosh Driver 2.0
	
	Written by Dominic Mazzoni (dmazzoni@cs.cmu.edu)
	based on the Macintosh Driver (version 1.31) by
	Tapio K. Vocadlo of Ottawa, Canada (taps@rmx.com).
	
	XaoS version: 	3.1 beta
	Driver version: 2.0
	
	This file contains routines for two mac drivers,
	the main Mac driver and the fullscreen mac driver.

  Features / Notes:
  
  * Uses the main monitor (the one with the menu bar).
  
  * Supports 8-bit, 16-bit, or 32-bit color.  If the monitor is set to
    any other depth, XaoS is told to render 8-bit and Quickdraw does the
    translation to the screen
  
  * Fullscreen mode hides and shows the menu bar using new (Mac OS 8.5)
    routines that may not work on older systems.
  
  * Menus are native in windowed mode, and XaoS-rendered in fullscreen mode.
  
  * Supports resolution-switching on the fly

*/

#include "config.h"
#include "filter.h"

#ifdef _MAC

// Cursor rez IDs
const short CURS_PAN		= 202;
const short CURS_PANACTIVE	= 203;
const short CURS_ZOOM		= 204;	// not used
const short CURS_ZOOMOUT	= 205;	// not used

// Splash pict ID
const short kSplashPICT = 128;

// Menu IDs
const short apple_menu_id = 8000;
const short about_item = 1;

#include "version.h"
#include "aconfig.h"
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "zoom.h"
//#include "gif.h"
#include "ui.h"
#include "ui_helper.h"
#include "uiint.h"
//#include "palette.h"

#include <stdlib.h>

#include <QDOffScreen.h>
#include <Palettes.h>
#include <Menus.h>

#include "CommonDialogs.h"

static void
ui_setdriver (uih_context * c, int d);

struct ui_driver    mac_driver;
struct ui_driver    full_driver;
struct ui_driver    double_driver;

static int          mac_window_width  = 0;
static int          mac_window_height = 0;

static int          mac_depth;

static int          full_screen_mode;
static int          mac_currentbuff = 0;
static int          mac_num_colors = 0;
static WindowPtr    mac_screen;
static GWorldPtr    mac_offscreen[2];
static GDHandle     mac_gdev = nil;
static int          mac_original_gdev_mode;
static Boolean      mac_palette_changed = true;
static Boolean      mac_show_grow_icon = true;
static Boolean      mac_panning = false;
static GWorldPtr    mac_grow_icon_gworld = nil;
static RGBColor     mac_text_color = { 0xFFFF, 0xFFFF ,0xFFFF };	// white

void       SetWindPaletteFromClut(WindowPtr theWindow, CTabHandle theClut);
GDHandle   GetGameScreen(void);
void       RestoreGameScreen(void);
void       DrawNiceGrowIcon(WindowPtr w);
void       DoMenuCommand(long menuResult);

static void mac_free_buffers(char *buffer1, char *buffer2);

MenuHandle mac_createrootmenu ();
void mac_menu_selected (int menu_id, int item);

// SUPPORT ROUTINES ----------------------

#define kIndexedGDeviceType			0
#define kFixedGDeviceType			1
#define kDirectGDeviceType			2

// Handle menu command choice
void DoMenuCommand(long menuResult)
{
	int		menuID, menuItem;
	Str255		daName;
	MenuHandle	mh;

	menuID = HiWord(menuResult);	/* use macros for efficiency to... */
	menuItem = LoWord(menuResult);	/* get menu item number and menu number */
	mh = GetMenuHandle(menuID);

  if (menuID == apple_menu_id) {
    if (menuItem == about_item)
      ui_key('h');
    else {
      GetMenuItemText(mh, menuItem, daName);
			OpenDeskAcc(daName);
    }
  }
  else if (menuItem > 0) {
    mac_menu_selected(menuID, menuItem);
  }
	HiliteMenu(0);					/* unhighlight what MenuSelect (or MenuKey) hilited */
  FlushEvents(everyEvent, 0);
}


// Draw the standard grow icon but without the scrollbar lines.
void DrawNiceGrowIcon(WindowPtr w)
{
	// Since DrawGrowIcon includes the scrollbar lines,
	// the simple way to draw just the icon is to clip it
	// to just the grow box.
	Rect	clipRect;
	RgnHandle	rh, rgnClip;
	
	rh = NewRgn();
	GetClip(rh);
	clipRect = w->portRect;
	clipRect.left = clipRect.right - 15;
	clipRect.top = clipRect.bottom - 15;
	
	rgnClip = NewRgn();
	RectRgn(rgnClip, &clipRect);
	SetClip(rgnClip);	// restore clip region
	
	DrawGrowIcon(w);
	SetClip(rh);	// restore clip region
	DisposeRgn(rh);
	DisposeRgn(rgnClip);
}


// Set a window's palette from a 'clut' resource handle.
void SetWindPaletteFromClut(WindowPtr theWindow, CTabHandle theClut)
{
	// From 'Tricks of the Mac Game Programming Gurus', 1st ed., page 104-105,
	// chapter 2, "Basic Game Graphics"
	
	PaletteHandle thePalette;
	
	// Create a palette from the clut
	thePalette = GetPalette(theWindow);
	if(thePalette == nil)
	{
		// create palette for window
		thePalette = NewPalette( (**theClut).ctSize + 1, theClut, pmTolerant +
															pmExplicit, 0x0000 );
	}
	else
	{
		// modify the window's palette to be the clut.
		CTab2Palette(theClut, thePalette, pmTolerant + pmExplicit, 0x0000 );
	}
	// Install palette into window
	NSetPalette( theWindow, thePalette, pmAllUpdates );
	
	// Update color environment
	ActivatePalette( theWindow );
}


// Return a GDHandle to a mac_screen that can be drawn on.
// If no such mac_screen, return nil.
GDHandle GetGameScreen()
{
	mac_gdev = GetMainDevice(); // Get device with menu bar, which we will use
	
  mac_depth = (**(**mac_gdev).gdPMap).pixelSize;
			
	return mac_gdev;
}

// Restore original monitor device pixel depth.
void RestoreGameScreen()
{
}


// Buffer-to-window copy routine.
static void display_refresh()
{
	PixMapHandle pmap, pmapicon;
	GWorldPtr	oldPort;
	GDHandle	oldDev;
	Rect		r;
	
	if(mac_offscreen[mac_currentbuff] == nil)
		return;
	
	if(mac_show_grow_icon)
	{	
		// Draw the grow icon onto the bitmap so it appears all the time.
		GetGWorld(&oldPort, &oldDev);
		

		r = mac_offscreen[mac_currentbuff]->portRect;
		r.left = r.right - 15;
		r.top = r.bottom - 15;

		// Save current pixels.
		SetGWorld(mac_grow_icon_gworld, nil);
		ForeColor(blackColor);
		BackColor(whiteColor);

		pmap = GetGWorldPixMap(mac_offscreen[mac_currentbuff]);
		pmapicon = GetGWorldPixMap(mac_grow_icon_gworld);
		
		// Force color tables to be the same so speed is at maximum.
		(*(( *pmap)->pmTable ))->ctSeed = (*(( *pmapicon)->pmTable ))->ctSeed;

		CopyBits(&((GrafPtr)mac_offscreen[mac_currentbuff])->portBits, &((GrafPtr)mac_grow_icon_gworld)->portBits, &r, &mac_grow_icon_gworld->portRect, srcCopy, nil);

		// Draw grow icon.		
		SetGWorld(mac_offscreen[mac_currentbuff], nil);
		
		
		//DrawNiceGrowIcon((WindowPtr) mac_offscreen[mac_currentbuff]);	// won't work.
		OffsetRect(&r, 1, 1);
		PenSize(1,1);
		EraseRect(&r);
		FrameRect(&r);
		
		InsetRect(&r, 3, 3);
		OffsetRect(&r, 1, 1);
		FrameRect(&r);
		InsetRect(&r, 1, 1);
		OffsetRect(&r, -3, -3);
		EraseRect(&r);
		FrameRect(&r);
		
		
		
		SetGWorld(oldPort, oldDev);
	}
	// An update event occurred, so just copy from our buffer to the mac_screen.
	pmap = GetGWorldPixMap(mac_offscreen[mac_currentbuff]);
	
	// Force color tables to be the same so speed is at maximum.
	(*(( *pmap)->pmTable ))->ctSeed = 
		(*((*((*(mac_gdev)) -> gdPMap ))->pmTable))->ctSeed;

	CopyBits(&((GrafPtr)mac_offscreen[mac_currentbuff])->portBits, &((GrafPtr)mac_screen)->portBits, &mac_offscreen[mac_currentbuff]->portRect, &mac_screen->portRect, srcCopy, nil);

	// Restore pixels.
	if(mac_show_grow_icon)
	{
		r = mac_offscreen[mac_currentbuff]->portRect;
		r.left = r.right - 15;
		r.top = r.bottom - 15;

		SetGWorld(mac_offscreen[mac_currentbuff], nil);
		pmap = GetGWorldPixMap(mac_offscreen[mac_currentbuff]);
		pmapicon = GetGWorldPixMap(mac_grow_icon_gworld);
		
		// Force color tables to be the same so speed is at maximum.
		(*(( *pmap)->pmTable ))->ctSeed = (*(( *pmapicon)->pmTable ))->ctSeed;

		CopyBits(&((GrafPtr)mac_grow_icon_gworld)->portBits, &((GrafPtr)mac_offscreen[mac_currentbuff])->portBits, &mac_grow_icon_gworld->portRect, &r, srcCopy, nil);
	
		SetGWorld(oldPort, oldDev);
	
	}

}


// ------------------------------------------------------

// XAOS UI API SUPPORT

// Display c-string <text> at x,y
static void mac_print(int x, int y, CONST char *text)
{
	static short lastY = 32000;
	
	// Clear mac_screen if cursor has moved up or on same line.
	if(y <= lastY)
		display_refresh();
	//	EraseRect(&mac_screen->portRect);
		
	lastY = y;
		
	MoveTo(x+3, y+12);
	RGBForeColor(&mac_text_color);
	DrawText(text, 0, strlen(text));
	
	ForeColor(blackColor);	// so copybits will work
}


// Display pixel buffer from fractal engine to window.
static void mac_display()
{
	PixMapHandle pmap;

	if(mac_palette_changed)
	{
		mac_palette_changed = false;
		ActivatePalette(mac_screen);
	}

	display_refresh();
}




// Add color to palette.
// r,g,b are 0-255 range color channel values.
static int mac_set_color(int r, int g, int b, int init)
{
	PaletteHandle hPalette;
	RGBColor	srcRGB;
	
    if (init)
    {
		mac_num_colors = 1;
		mac_palette_changed = true;	// signal mac_display to activate new palette
	}
    if (mac_num_colors == 255)
		return (-1);
	
	srcRGB.red = r * 256;
	srcRGB.green = g * 256;
	srcRGB.blue = b * 256;
	
	hPalette = GetPalette(mac_screen);
	SetEntryColor(hPalette, mac_num_colors-1, &srcRGB);
    return (mac_num_colors++);
}

// Switch current pixel buffer to copy from on next display update.
static void mac_flip_buffers()
{
    mac_currentbuff ^= 1;
}

// Deallocate any stuff we allocated at startup.
static void mac_uninitialize()
{
	int i;
	for(i=0; i<2; i++)
	{
		if(mac_offscreen[i] != nil)
			DisposeGWorld(mac_offscreen[i]);
	}
	DisposeWindow(mac_screen);
	RestoreGameScreen();
	
	ShowMenuBar();
}

static int mac_init(int fullScreen)
{
	Rect	r;
	OSErr err, error;
	SysEnvRec	theWorld;
	PixMapHandle pmap;
	GWorldPtr	oldWorld;
	GDHandle	oldDev;
	CTabHandle	clut;
	int			i;
	PicHandle	ph;
	Rect rSplash;
	Rect	r2;
  MenuHandle appleMenu;
	char	winTitle[255];

	// Init the Mac Toolbox.
	//	Test the computer to be sure we can do color.  
	//	If not we would crash, which would be bad.  
	//	If we canÕt run, just beep and exit.
	//

	static Boolean	bBeenHere = false;
	
	full_screen_mode = fullScreen;
	
	if(!bBeenHere)
	{
		error = SysEnvirons(1, &theWorld);
		if (theWorld.hasColorQD == false) {
			SysBeep(1);
			ExitToShell();					/* If no color QD, we must leave. */
		}
		
		/* Initialize all the needed managers. */
		InitGraf(&qd.thePort);
		InitFonts();
		InitWindows();
		InitMenus();
		TEInit();
		InitDialogs(nil);
		InitCursor();

		//
		//	To make the Random sequences truly random, we need to make the seed start
		//	at a different number.  An easy way to do this is to put the current time
		//	and date into the seed.  Since it is always incrementing the starting seed
		//	will always be different.  DonÕt for each call of Random, or the sequence
		//	will no longer be random.  Only needed once, here in the init.
		//
		GetDateTime((unsigned long*) &qd.randSeed);
		
		
		// Nicely acquire drawing device.
		// Do this before the messier init/terminate stuff occurs.
		if (!GetGameScreen())
			ExitToShell();

		// -- Mac Toolbox initialized.
		bBeenHere = true;
		
		// Install apple menu
		appleMenu = NewMenu(apple_menu_id, "\p\024");
		
		MacAppendMenu(appleMenu, "\pXaoS HelpÉ");
		MacAppendMenu(appleMenu, "\p(-");
    
    MacInsertMenu(appleMenu, 0);
		
		// This voodoo tells the Mac OS to add its junk to the Apple menu
		AppendResMenu(appleMenu, 'DRVR');
		
		mac_createrootmenu();

		DrawMenuBar();
	}	
	
	// Set up grow icon buffer.
	SetRect(&r2, 0, 0, 15, 15);	
	NewGWorld(&mac_grow_icon_gworld, 8, &r2, 0,0,0);
	if(mac_grow_icon_gworld == nil)
		return 0;
	LockPixels(	GetGWorldPixMap(mac_grow_icon_gworld));

	sprintf(winTitle, "XaoS Fractal Viewer %s", XaoS_VERSION);
	c2pstr(winTitle);

  if (full_screen_mode) {
    if (mac_gdev == GetMainDevice())
      HideMenuBar();
  
    r = (**mac_gdev).gdRect;

    mac_window_width = r.right - r.left;
    mac_window_height = r.bottom - r.top;
    
    if (full_screen_mode == 2) {
      mac_window_width /= 2;
      mac_window_height /= 2;
    }

    // Create a window the size of the entire screen

	  mac_screen = NewCWindow(nil, &r, (StringPtr)winTitle, true,  plainDBox,
	                          (WindowPtr) -1, true, nil);

  	if(mac_screen == nil)
	  	return 0;
	  
	  // Make its visRgn the size of the screen so it can overdraw the
	  // menu bar and other stuff.
	  
	  RectRgn(((CGrafPort *)mac_screen) -> visRgn, &r);
	  
	  mac_show_grow_icon = false;
	  
  }
  else {
  
	  r = (**mac_gdev).gdRect;
	  
	  InsetRect(&r, 40, 40);
	  
	  if (mac_gdev == GetMainDevice())
	    r.top += GetMBarHeight();
	  
	  r.top += 20; // for title bar height

    mac_window_width = r.right - r.left;
    mac_window_height = r.bottom - r.top;
	  
	  mac_screen = NewCWindow(nil, &r, (StringPtr)winTitle, true,  documentProc,
	                          (WindowPtr) -1, true, nil);

  	if(mac_screen == nil)
	  	return 0;
	  
	  mac_show_grow_icon = true;
	}
	
	// Set the window's characteristics.
	clut = GetCTable(128);
	if(clut)
		SetWindPaletteFromClut(mac_screen, clut);
	
	SetPort(mac_screen);
	GetGWorld(&oldWorld, &mac_gdev);
	ValidRect(&mac_screen->portRect);	// kill upcoming update event since mac_offscreen buffers n/a.
	//DrawNiceGrowIcon(mac_screen);
	ForeColor(blackColor);
	BackColor(whiteColor);
	
	TextMode(srcOr);
	
	#if 0
		// Display splash mac_screen
		EraseRect(&mac_screen->portRect);
		ph = GetPicture(kSplashPICT);
		
		SetRect(&rSplash, 0, 0, 348, 170);
		OffsetRect(&rSplash, (mac_window_width - rSplash.right)/2, (mac_window_height - rSplash.bottom)/2);	// center
		DrawPicture(ph, &rSplash);
		ReleaseResource((Handle)ph);


		// Wait for user to click mouse (a mousedown and then a mouseup).
		while(!Button()) { }
		while(Button()) { }
  #endif
	
	// Set font to display ugly help as least ugly as possible.
	TextFont(1);
	TextSize(9);

	return 1;	
}

static int mac_window_init()
{
  return mac_init(0);
}

static int mac_fullscreen_init()
{
  return mac_init(1);
}

static int mac_doublescreen_init()
{
  return mac_init(2);
}

// Return display buffer size, and also set drivers based on monitor
// bit depth appropriately.
static void mac_get_size(int *x, int *y)
{
	struct ui_driver *driver;
	int i;
	*x = mac_window_width;
	*y = mac_window_height;	
	
	for(i=0; i<3; i++) {
	
    switch(i) {
    case 0:
	    driver = &mac_driver;
	    break;
	  case 1:
	    driver = &full_driver;
	    break;
	  case 2:
	    driver = &double_driver;
	    break;
	  }
	  	
		switch(mac_depth)
	  {
	    case 16:
	      driver->imagetype = UI_TRUECOLOR16;
	      driver->bmask = 0x001f;    //             11111b
			  driver->gmask = 0x03e0;    //        1111100000b
			  driver->rmask = 0x7c00;    //   111110000000000b
	      break;
	    case 24:
	      driver->imagetype = UI_TRUECOLOR24;
	      driver->bmask = 0x000000ff;
			  driver->gmask = 0x0000ff00;
			  driver->rmask = 0x00ff0000;
	      break;
	    case 32:
	      driver->imagetype = UI_TRUECOLOR;
	      driver->bmask = 0x000000ff;
			  driver->gmask = 0x0000ff00;
			  driver->rmask = 0x00ff0000;
	      break;
	    case 8:
	    default:
	      driver->imagetype = UI_C256;
	      mac_depth = 8; // use 8-bit offscreen and let QuickDraw fix it.
	      break;
	  }
  }
}


// Read the mouse. Since Mac has one button, emulate
// multiple buttons by scanning shift/cmd/control keys.
static void mac_getmouse(int *x, int *y, int *buttons)
{
	static KeyMap charmap;
	
	Point p;
	GetMouse(&p);
	*x = p.h;
	*y = p.v;
	
    *buttons = 0;

	GetKeys(charmap);
	
	// Switch the cursor to a hand if the shift key is down.
	// If not, switch to the default arrow pointer.

	if(charmap[1] & 0x0001)	// shift - pan
	{
/*		if(!mac_panning)
		{*/
			mac_panning = true;
			SetCursor(*GetCursor(Button() ? CURS_PANACTIVE : CURS_PAN));
		//}
	}
	else
	{
		if(mac_panning)
		{
			mac_panning = false;
			SetCursor(&qd.arrow);
		}
	}


    if (Button())
    {

	// The Mac only has one button, so emulate
	// others by seeing if keyboard has certain
	// keys down.

		if(charmap[1] == 0x0000)	// no keys down
			*buttons |= BUTTON1;	// mousedown - zoom in

		if(charmap[1] & 0x0001)	// shift - pan
			*buttons |= BUTTON2;
			

		if(charmap[1] & 0x0008)	// control - zoom out
			*buttons |= BUTTON3;
			

	}
}


// Get next event from keyboard, mouse, etc. For Mac UI, prehandle
// certain events like window dragging, window closing, etc.
static void mac_processevents(int wait, int *x, int *y, int *buttons, int *k)
{
	EventRecord	event;
	char	key;
	char	vkey;
	WindowPtr	hitWindow;
	short	part;
	Rect	rDesk;
	RgnHandle	hRgnDesk;
	Point	ptLocal;
	Rect	rSizeCtl;
	static	Rect rSizeRect;
	long	newWinSize;
	int   old_depth;
	
	*k = 0;

	//DrawNiceGrowIcon(mac_screen);

	if(GetNextEvent(everyEvent, &event))
	{
	
		switch(event.what)
		{
			/*	1.01 - It is not a bad idea to at least call DIBadMount in response
			to a diskEvt, so that the user can format a floppy. */
			case diskEvt:
				if ( HiWord(event.message) != noErr )
				{
					SetPt(&ptLocal, 0x0070, 0x0050);
					(void) DIBadMount(ptLocal, event.message);
				}
				break;

			case updateEvt:
			  // If the monitor has switched depths, we need to reinitialize
			  old_depth = mac_depth;
			  GetGameScreen();
			  if (old_depth != mac_depth) {
          ui_resize();
        }
			  else if((WindowPtr) event.message == mac_screen)
				{
					BeginUpdate(mac_screen);
					display_refresh();
					//DrawNiceGrowIcon(mac_screen);
					EndUpdate(mac_screen);
				}
				break;
				
			case mouseUp:
				//DrawNiceGrowIcon(mac_screen);
				break;
				
			case mouseDown:
			  SetPort(mac_screen);
				part = FindWindow(event.where, &hitWindow);
				if(hitWindow != mac_screen)
				{
					switch(part)
					{
						case inMenuBar:             /* process a mouse menu command (if any) */
							DoMenuCommand(MenuSelect(event.where));
							break;
					}
				}
				else
				{
					switch(part)
					{

						case inDrag:
							hRgnDesk = GetGrayRgn();
							rDesk = (*hRgnDesk)->rgnBBox;
							DragWindow(mac_screen, event.where, &rDesk);
							break;
							
						case inGoAway:
							if(TrackGoAway(mac_screen, event.where))
							{
								mac_uninitialize();
								ExitToShell();
							}
							break;
							
						case inGrow:
							SetRect(&rSizeRect, 200, 150, 30000, 30000); 
							newWinSize = GrowWindow(mac_screen, event.where, &rSizeRect);
							if(newWinSize != 0)
							{
								mac_window_width = LoWord(newWinSize);
								mac_window_height = HiWord(newWinSize);
								SizeWindow(mac_screen, mac_window_width, mac_window_height, false);
								// Switch to black help text since resized window wipes itself white.
								mac_text_color.red = mac_text_color.green = mac_text_color.blue = 0x0000;
								ui_call_resize();
								mac_getmouse(x, y, buttons);
								*buttons = 0;	// mouse is up by now, but make sure
								return;
							}
							break;
					}
				}
				break;
				
				
			case keyDown:
			case autoKey:
				key = event.message & charCodeMask;
				vkey = event.message & keyCodeMask;
				
				if(key == '.' && vkey == cmdKey)
				{
					mac_uninitialize();
					ExitToShell();
				}
				
				switch(key)
				{
					case 0x1C:
						*k += 1; break;	// left
					case 0x1D:
						*k += 2; break;	// right
					case 0x1E:
						*k += 4; break;	// up
					case 0x1F:
						*k += 8; break;	// down
				}
				
				ui_key(key);
				break;
		}
		
	}

	mac_getmouse(x, y, buttons);

}



// Allocate fractal engine pixel buffers.
static int mac_alloc_buffers(char **buffer1, char **buffer2)
{
	int 			rowBytes = 0;
	PixMapHandle 	pmap;
	OSErr			err1, err2;
	Rect			r = { 0,0,0,0 };
	
  static char*        buffers[2];
	
	int				width, height;
	int x,y;
	
	mac_get_size(&width, &height);
	r.right = width;
	r.bottom = height;
	

	err1 = NewGWorld(&mac_offscreen[0], mac_depth, &r, nil, nil, 0);
	err2 = NewGWorld(&mac_offscreen[1], mac_depth, &r, nil, nil, 0);
	
	if(err1 != noErr || err2 != noErr)
	{
		if(mac_offscreen[0] != nil) { DisposeGWorld(mac_offscreen[0]); mac_offscreen[0] = nil; }
		if(mac_offscreen[1] != nil) { DisposeGWorld(mac_offscreen[1]); mac_offscreen[1] = nil; }
		return 0;
	}

	pmap = GetGWorldPixMap(mac_offscreen[0]);
	LockPixels(pmap);
	rowBytes = (*pmap)->rowBytes & 0x3fff; // mask off top two bits
	buffers[0] = GetPixBaseAddr(pmap);

	pmap = GetGWorldPixMap(mac_offscreen[1]);
	LockPixels(pmap);
	buffers[1] = GetPixBaseAddr(pmap);
	
	*buffer1 = buffers[0];
	*buffer2 = buffers[1];
	/*
	for(y=0; y<height; y++)
	  for(x=0; x<rowBytes; x++) {
	    ((char *)buffers[0])[y*rowBytes+x] = 0;
	  }*/

	mac_currentbuff = 0;
	
	return rowBytes;
}

// Deallocate fractal engine pixel buffers.
static void mac_free_buffers(char *buffer1, char *buffer2)
{
	
	int i;
	for(i=0; i<2; i++)
	{
		if(mac_offscreen[i] != nil) { DisposeGWorld(mac_offscreen[i]); mac_offscreen[i] = nil; }
	}
}


void mac_dorootmenu (struct uih_context *uih, CONST char *name);
void mac_enabledisable (struct uih_context *uih, CONST char *name);
void mac_menu (struct uih_context *c, CONST char *name);
void mac_dialog (struct uih_context *c, CONST char *name);
void mac_help (struct uih_context *c, CONST char *name);


static char *helptext[] =
{
    "MACINTOSH DRIVER VERSION 2.0           ",
    "====================================   ",
    "Written by Dominic Mazzoni             ",
    "(dmazzoni@cs.cmu.edu)                  ",
    "based on the 1.31 driver               ",
    "by Tapio K. Vocadlo                    ",
    "(taps@rmx.com)                         ",
    "                                       ",
    "Shift-drag    = pan                    ",
    "Control-click = zoom out               ",
    "Cmd-period    = force quit at any time ",
    "                                       ",
    "See menubar for more commands.         "
};

#define UGLYTEXTSIZE (sizeof(helptext)/sizeof(char *))
static struct params mac_params[] =
{
    {NULL, NULL, NULL, NULL}
};

struct gui_driver mac_gui_driver =
{
   mac_dorootmenu,      //dorootmenu
   mac_enabledisable,   //enabledisable
   NULL,                //pop-up menu
   NULL,                //mac_dialog
   NULL                 //mac_help
};

struct ui_driver mac_driver =
{
    "Mac Windowed Driver",
    mac_window_init,
    mac_get_size,
    mac_processevents,
    mac_getmouse,
    mac_uninitialize,
    mac_set_color,
    NULL, /* set_range */
    mac_print,
    mac_display,
    mac_alloc_buffers,
    mac_free_buffers,
    mac_flip_buffers,
    NULL, /* mouse_type*/
    NULL, /* flush */
    12, /* textwidth */
    12, /* textheight */
    mac_params,
		PIXELSIZE|RESOLUTION /*| UPDATE_AFTER_PALETTE*/, /* flags */
		0.01, 0.01, /* width, height */
		0, 0, /* maxwidth, maxheight */
		UI_C256, /* imagetype */
		0, 256, 256, /* palette start, end, maxentries */
		0, 0, 0, /* rmask, gmask, bmask */
    &mac_gui_driver //NULL /* GUI_Driver */
};

struct ui_driver full_driver =
{
    "Mac Fullscreen Driver",
    mac_fullscreen_init,
    mac_get_size,
    mac_processevents,
    mac_getmouse,
    mac_uninitialize,
    mac_set_color,
    NULL, /* set_range */
    mac_print,
    mac_display,
    mac_alloc_buffers,
    mac_free_buffers,
    mac_flip_buffers,
    NULL, /* mouse_type*/
    NULL, /* flush */
    12, /* textwidth */
    12, /* textheight */
    mac_params,
		FULLSCREEN | PIXELSIZE |RESOLUTION  /*| UPDATE_AFTER_PALETTE*/, /* flags */
		0.01, 0.01, /* width, height */
		0, 0, /* maxwidth, maxheight */
		0, //UI_C256, /* imagetype */
		0, 256, 256, /* palette start, end, maxentries */
	  0, 0, 0, /* rmask, gmask, bmask */
    NULL /* GUI_Driver */
};

struct ui_driver double_driver =
{
    "Mac Doubled Fullscreen Driver",
    mac_doublescreen_init,
    mac_get_size,
    mac_processevents,
    mac_getmouse,
    mac_uninitialize,
    mac_set_color,
    NULL, /* set_range */
    mac_print,
    mac_display,
    mac_alloc_buffers,
    mac_free_buffers,
    mac_flip_buffers,
    NULL, /* mouse_type*/
    NULL, /* flush */
    12, /* textwidth */
    12, /* textheight */
    mac_params,
		FULLSCREEN | PIXELSIZE |RESOLUTION  /*| UPDATE_AFTER_PALETTE*/, /* flags */
		0.01, 0.01, /* width, height */
		0, 0, /* maxwidth, maxheight */
		0, //UI_C256, /* imagetype */
		0, 256, 256, /* palette start, end, maxentries */
	  0, 0, 0, /* rmask, gmask, bmask */
    NULL /* GUI_Driver */
};

#endif	// _MAC
