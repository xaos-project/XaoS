/*
	Note: view this file with a tabstop of 4.
	
	ui_mac.c
	
	Apple Macintosh user interface support for Xaos Fractal Viewer
	Written by Tapio K. Vocadlo of Ottawa, Canada (taps@rmx.com).
	Some stuff borrowed from 'Tricks of the Mac Game Programming Gurus'.
	
	XaoS version: 	2.1g
	Driver version: 1.31
	
	Revision History:
	
	When		Who		What
	------------------------------------------------------------------
	Nov 5/96	taps	Created
	Nov 7/96	taps	More features supported, nicer panning cursors
	Nov 8/96	taps	Check to optimize buffer-to-GWorld data transfer

	Nov 11/96	taps	Took out 'save not supported' help text, set 
						auto-update for palette change to true.

	Nov 12/96	taps	mac_alloc_buffers returns bytes-per-scanline for XaoS 2.1
						Let fractal engine draw directly onto Mac GWorlds.

	Nov 18/96	taps	Support window resizing
	
	Nov 21/96	taps	Made resize icon visible on window.
						Added introductory splash screen.
						
	Nov 22/96	taps	Default window size fits on smallest Mac monitor.
						Added menu bar and 'Visible Resize Control' option.

	Nov 25/96	taps	Support disk-insertion event, XaoS_VERSION constant,
						overlapping help/status text onto fractal, and
						menu commands for black vs. white help text.

*/


#ifdef _MAC

// Format floppy constants
const short kDITop = 0x0050;
const short kDILeft	= 0x0070;

// Default window size.
static int MAC_WINWIDTH	= 508;
static int MAC_WINHEIGHT = 381;

// Cursor rez IDs
const short CURS_PAN		= 202;
const short CURS_PANACTIVE	= 203;
const short CURS_ZOOM		= 204;	// not used
const short CURS_ZOOMOUT	= 205;	// not used

// Splash pict ID
const short kSplashPICT = 128;

// Menu IDs
const short rMenuBar =	128;
const short	mApple = 	128;
const short mFile = 	129;
const short mEdit = 	130;
const short mOptions = 	131;

// Menu command IDs
const short iToggleGrowIcon = 1;
const short iBlackText = 3;
const short iWhiteText = 4;
const short iAbout = 1;
const short iQuit = 1;
const short iCopy = 1;

#include "version.h"
#include "aconfig.h"
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "zoom.h"
#include "gif.h"
#include "ui.h"
#include "palette.h"

#include <stdlib.h>

#include <QDOffScreen.h>
#include <Palettes.h>

#include "CommonDialogs.h"

struct ui_driver mac_driver;
static int initialised;
static int mode = -1;
static width = 640, height = 480;
static int mac_currentbuff = 0;
static int ncolors = 0;
static WindowPtr screen;
static GWorldPtr	offscreen[2];
static char* buffers[2];
void *font = NULL;
static GDHandle ourScreen = nil;	// which monitor we're on
static GDHandle gScreengdev = nil;

static int gOrgSceenMode;
static GDHandle gGameScreen = nil;
static Boolean gbPaletteChanged = true;
static Boolean gbShowGrowIcon = true;
static Boolean gbPanning = false;
static GWorldPtr gGrowIconPixels = nil;
static RGBColor gTextColor = { 0xFFFF, 0xFFFF ,0xFFFF };	// white

void SetWindPaletteFromClut(WindowPtr theWindow, CTabHandle theClut);
GDHandle GetGameScreen(void);
void RestoreGameScreen(void);
void DrawNiceGrowIcon(WindowPtr w);
void AdjustMenus(void);
void DoMenuCommand(long menuResult);


// SUPPORT ROUTINES ----------------------


#define kIndexedGDeviceType			0
#define kFixedGDeviceType			1
#define kDirectGDeviceType			2

// Make sure menus are up-to-date with application environment
void AdjustMenus(void)
{
	MenuHandle	optMenu = GetMHandle(mOptions);

	SetItemMark(optMenu, iToggleGrowIcon, gbShowGrowIcon ? checkMark : noMark );
	SetItemMark(optMenu, iBlackText, gTextColor.red == 0x0000 ? diamondMark : noMark );
	SetItemMark(optMenu, iWhiteText, gTextColor.red == 0xFFFF ? diamondMark : noMark );
	
}


// Handle menu command choice
void DoMenuCommand(long menuResult)
{
	int		menuID, menuItem;
	int		daRefNum;
	Str255		daName;
	MenuHandle	mh;

	
	menuID = HiWord(menuResult);	/* use macros for efficiency to... */
	menuItem = LoWord(menuResult);	/* get menu item number and menu number */
	mh = GetMenuHandle(menuID);
	
	if(mh != nil) {
	
	
		switch ( menuID ) {
			case mApple:
				switch ( menuItem ) {
					case iAbout:		/* bring up alert for About */
						ui_key('h');
						break;
					default:			/* all non-About items in this menu are DAs et al */
						/* type Str255 is an array in MPW 3 */
						GetItem(mh, menuItem, daName);
						daRefNum = OpenDeskAcc(daName);
						break;
				}
				break;
				
				
			case mFile:
				switch ( menuItem )
				{
					case iQuit:
						ui_key('q');
						break;
				}
				break;
				
				
			case mEdit:					/* call SystemEdit for DA editing & MultiFinder */

				switch( menuItem)
				{
					case iCopy:	// want to support this later on.
						break;

				}
						
				break;
				
			case mOptions:
				switch(menuItem)
				{
					case iToggleGrowIcon:
						gbShowGrowIcon = !gbShowGrowIcon;
						InvalRect(&screen->portRect);
						break;
						
					case iBlackText:
						gTextColor.red = gTextColor.green = gTextColor.blue = 0x0000;
						break;
					
					case iWhiteText:
						gTextColor.red = gTextColor.green = gTextColor.blue = 0xFFFF;
						break;
						
				}
				
				break;
			
		} // switch menu

	}	// if menuhandle good
	HiliteMenu(0);					/* unhighlight what MenuSelect (or MenuKey) hilited */
	AdjustMenus();
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


// Return a GDHandle to a screen that can be drawn on.
// If no such screen, return nil.
GDHandle GetGameScreen()
{
	GDHandle	gdhAux = nil, gdh = GetDeviceList();	// Get first GDevice.
	
	while(gdh != nil)
	{
		// If this device is an 8-bit CLUT device, use it.
		if(	TestDeviceAttribute(gdh, screenDevice) &&
				TestDeviceAttribute(gdh, screenActive) )
		{
		
			if((**gdh).gdType == clutType && (**gdh).gdMode == 128)
			{
				// Found an okay device already set.
				gOrgSceenMode = (**gdh).gdMode;
				gGameScreen = gdh;
				return gdh;
			}
			// As long as we're going through all the devices,
			// flag down one which could be used (if we haven't already).
			if(gdhAux == nil && HasDepth(gdh, 8, gdDevType, 1 ))
			{
				gdhAux = gdh;
			}
			
		}
		gdh = GetNextDevice(gdh);
	}
	// Hmm... no screen was already at the preferred setting. See if
	// we have an auxiliary device.
	if(gdhAux == nil)
	{
		doErrorAlert("\pSorry, but a monitor that supports 256 colors is required.");
		return nil;
	}
	// Ask the user if okay to switch the auxiliary monitor to 8-bit color mode.
	if(doOKcancel("\pOkay to switch monitor to 256 colors?"))
	{
		gOrgSceenMode = (**gdhAux).gdMode;
		if(SetDepth(gdhAux, 8, gdDevType, 1) == noErr)
		{
			gGameScreen = gdhAux;
			return gdhAux;
		}
	}
	
	gGameScreen = nil;
	return nil;
}

// Restore original monitor device pixel depth.
void RestoreGameScreen()
{
	if(gGameScreen == nil)
		return;
		
	switch(gOrgSceenMode)
	{
		case 128:
			SetDepth(gGameScreen, 8, gdDevType, 1);
			break;
		case 129:
			SetDepth(gGameScreen, 16, gdDevType, 1);
			break;
		default:
			SetDepth(gGameScreen, 32, gdDevType, 1);	// uh... try Millions mode.
			break;
	}
}


// Buffer-to-window copy routine.
static void display_refresh()
{
	PixMapHandle pmap, pmapicon;
	GWorldPtr	oldPort;
	GDHandle	oldDev;
	Rect		r;
	
	if(offscreen[mac_currentbuff] == nil)
		return;
	
	if(gbShowGrowIcon)
	{	
		// Draw the grow icon onto the bitmap so it appears all the time.
		GetGWorld(&oldPort, &oldDev);
		

		r = offscreen[mac_currentbuff]->portRect;
		r.left = r.right - 15;
		r.top = r.bottom - 15;

		// Save current pixels.
		SetGWorld(gGrowIconPixels, nil);
		ForeColor(blackColor);
		BackColor(whiteColor);

		pmap = GetGWorldPixMap(offscreen[mac_currentbuff]);
		pmapicon = GetGWorldPixMap(gGrowIconPixels);
		
		// Force color tables to be the same so speed is at maximum.
		(*(( *pmap)->pmTable ))->ctSeed = (*(( *pmapicon)->pmTable ))->ctSeed;

		CopyBits(&((GrafPtr)offscreen[mac_currentbuff])->portBits, &((GrafPtr)gGrowIconPixels)->portBits, &r, &gGrowIconPixels->portRect, srcCopy, nil);

		// Draw grow icon.		
		SetGWorld(offscreen[mac_currentbuff], nil);
		
		
		//DrawNiceGrowIcon((WindowPtr) offscreen[mac_currentbuff]);	// won't work.
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
	// An update event occurred, so just copy from our buffer to the screen.
	pmap = GetGWorldPixMap(offscreen[mac_currentbuff]);
	
	// Force color tables to be the same so speed is at maximum.
	(*(( *pmap)->pmTable ))->ctSeed = 
		(*((*((*(gScreengdev)) -> gdPMap ))->pmTable))->ctSeed;

	CopyBits(&((GrafPtr)offscreen[mac_currentbuff])->portBits, &((GrafPtr)screen)->portBits, &offscreen[mac_currentbuff]->portRect, &offscreen[mac_currentbuff]->portRect, srcCopy, nil);

	// Restore pixels.
	if(gbShowGrowIcon)
	{
		r = offscreen[mac_currentbuff]->portRect;
		r.left = r.right - 15;
		r.top = r.bottom - 15;

		SetGWorld(offscreen[mac_currentbuff], nil);
		pmap = GetGWorldPixMap(offscreen[mac_currentbuff]);
		pmapicon = GetGWorldPixMap(gGrowIconPixels);
		
		// Force color tables to be the same so speed is at maximum.
		(*(( *pmap)->pmTable ))->ctSeed = (*(( *pmapicon)->pmTable ))->ctSeed;

		CopyBits(&((GrafPtr)gGrowIconPixels)->portBits, &((GrafPtr)offscreen[mac_currentbuff])->portBits, &gGrowIconPixels->portRect, &r, srcCopy, nil);
	
		SetGWorld(oldPort, oldDev);
	
	}

}


// ------------------------------------------------------

// XAOS UI API SUPPORT

// Display c-string <text> at x,y
static void mac_print(int x, int y, CONST char *text)
{
	static short lastY = 32000;
	
	// Clear screen if cursor has moved up or on same line.
	if(y <= lastY)
		display_refresh();
	//	EraseRect(&screen->portRect);
		
	lastY = y;
		
	MoveTo(x+3, y+12);
	RGBForeColor(&gTextColor);
	DrawText(text, 0, strlen(text));
	
	
	ForeColor(blackColor);	// so copybits will work
}


// Display pixel buffer from fractal engine to window.
static void mac_display()
{
	// If this is our first time, activate the palette.
	//static Boolean bBeenHere = false;
	PixMapHandle pmap;

	if(gbPaletteChanged)
	{
		gbPaletteChanged = false;
		ActivatePalette(screen);
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
		ncolors = 1;
		gbPaletteChanged = true;	// signal mac_display to activate new palette
	}
    if (ncolors == 255)
		return (-1);
	
	srcRGB.red = r * 256;
	srcRGB.green = g * 256;
	srcRGB.blue = b * 256;
	
	hPalette = GetPalette(screen);
	SetEntryColor(hPalette, ncolors-1, &srcRGB);
    return (ncolors++);
}


// Switch current pixel buffer to copy from on next display update.
static void mac_flip_buffers()
{
    mac_currentbuff ^= 1;
}


// Deallocate any stuff we allocated at startup.
static void mac_uninitialise()
{
	int i;
	for(i=0; i<2; i++)
	{
		if(offscreen[i] != nil)
			DisposeGWorld(offscreen[i]);
	}
	DisposeWindow(screen);
	RestoreGameScreen();
}



// Begin Mac UI support. Init Toolbox, create window, etc.
static int mac_init()
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
	Rect	r2 = {0,0,15,15};
	Handle		menuBar;
	char	winTitle[255];

	// Init the Mac Toolbox.
		//	Test the computer to be sure we can do color.  
	//	If not we would crash, which would be bad.  
	//	If we canÕt run, just beep and exit.
	//

	static Boolean	bBeenHere = false;
	
	if(!bBeenHere)
	{
		error = SysEnvirons(1, &theWorld);
		if (theWorld.hasColorQD == false) {
			SysBeep(50);
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
		ourScreen = GetGameScreen();
		
		if(ourScreen == nil)
			ExitToShell();


		// -- Mac Toolbox initialized.
		bBeenHere = true;
		
		// Install menu bar.
		menuBar = GetNewMBar(rMenuBar);			/* read menus into menu bar */
		if ( menuBar == nil ) {
			doErrorAlert("\pCouldnÕt allocate memory for menus");
			ExitToShell();
		}
		
		SetMenuBar(menuBar);					/* install menus */
		DisposHandle(menuBar);

		AddResMenu(GetMHandle(mApple), 'DRVR');	/* add DA names to Apple menu */

		DrawMenuBar();

	}	
	
	//CTabHandle hColors = GetCTable(8 + 64);	// default 8-bit color table
	SetRect(&r, 0, 0, MAC_WINWIDTH, MAC_WINHEIGHT);
	
	// Set up grow icon buffer.
	NewGWorld(&gGrowIconPixels, 8, &r2, 0,0,0);
	if(gGrowIconPixels == nil)
		return 0;

	LockPixels(	GetGWorldPixMap(gGrowIconPixels));
	OffsetRect(&r, 100, 100);
	sprintf(winTitle, "XaoS Fractal Viewer %s", XaoS_VERSION);
	c2pstr(winTitle);
	screen = NewCWindow(nil, &r, (StringPtr)winTitle, true,  documentProc, (WindowPtr) -1, true, nil);
	
	
	if(screen == nil)
		return 0;
		
		
	
	// Set the window's characteristics.
	clut = GetCTable(128);
	if(clut)
		SetWindPaletteFromClut(screen, clut);
	
	SetPort(screen);
	GetGWorld(&oldWorld, &gScreengdev);
	ValidRect(&screen->portRect);	// kill upcoming update event since offscreen buffers n/a.
	//DrawNiceGrowIcon(screen);
	ForeColor(blackColor);
	BackColor(whiteColor);
	
	TextMode(srcOr);
	
	// Display splash screen
	EraseRect(&screen->portRect);
	ph = GetPicture(kSplashPICT);
	
	SetRect(&rSplash, 0, 0, 348, 170);
	OffsetRect(&rSplash, (MAC_WINWIDTH - rSplash.right)/2, (MAC_WINHEIGHT - rSplash.bottom)/2);	// center
	DrawPicture(ph, &rSplash);
	ReleaseResource((Handle)ph);


	// Wait for user to click mouse (a mousedown and then a mouseup).
	while(!Button()) { }
	while(Button()) { }

	
	// Set font to display ugly help as least ugly as possible.
	TextFont(monaco);
	TextSize(9);

	return 1;	
	
}

// Return display buffer size.
static void mac_get_size(int *x, int *y)
{
	*x = MAC_WINWIDTH;
	*y = MAC_WINHEIGHT;
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
/*		if(!gbPanning)
		{*/
			gbPanning = true;
			SetCursor(*GetCursor(Button() ? CURS_PANACTIVE : CURS_PAN));
		//}
	}
	else
	{
		if(gbPanning)
		{
			gbPanning = false;
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
	
	*k = 0;

	//DrawNiceGrowIcon(screen);

	if(GetNextEvent(everyEvent, &event))
	{
	
		switch(event.what)
		{
			/*	1.01 - It is not a bad idea to at least call DIBadMount in response
			to a diskEvt, so that the user can format a floppy. */
			case diskEvt:
				if ( HiWord(event.message) != noErr )
				{
					SetPt(&ptLocal, kDILeft, kDITop);
					(void) DIBadMount(ptLocal, event.message);
				}
				break;

			case updateEvt:
				if((WindowPtr) event.message == screen)
				{
					BeginUpdate(screen);
					display_refresh();
					//DrawNiceGrowIcon(screen);
					EndUpdate(screen);
				}
				break;
				
			case mouseUp:
				//DrawNiceGrowIcon(screen);
				break;
				
			case mouseDown:
				part = FindWindow(event.where, &hitWindow);
				if(hitWindow != screen)
				{
					switch(part)
					{
						case inMenuBar:             /* process a mouse menu command (if any) */
							AdjustMenus();	/* bring Õem up-to-date */
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
							DragWindow(screen, event.where, &rDesk);
							break;
							
						case inGoAway:
							if(TrackGoAway(screen, event.where))
							{
								mac_uninitialise();
								ExitToShell();
							}
							break;
							
						case inGrow:
							SetRect(&rSizeRect, 200, 150, 30000, 30000); 
							newWinSize = GrowWindow(screen, event.where, &rSizeRect);
							if(newWinSize != 0)
							{
								MAC_WINWIDTH = LoWord(newWinSize);
								MAC_WINHEIGHT = HiWord(newWinSize);
								SizeWindow(screen, MAC_WINWIDTH, MAC_WINHEIGHT, false);
								// Switch to black help text since resized window wipes itself white.
								gTextColor.red = gTextColor.green = gTextColor.blue = 0x0000;
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
					mac_uninitialise();
					ExitToShell();
				}
				
				switch(key)
				{
					case 0x1C: *k += 1; break;	// left
					case 0x1D: *k += 2; break;	// right
					case 0x1E: *k += 4; break;	// up
					case 0x1F: *k += 8; break;	// down
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
	
	int				width, height;
	
	mac_get_size(&width, &height);
	r.right = width;
	r.bottom = height;
	

	err1 = NewGWorld(&offscreen[0], 8, &r, nil, nil, 0);
	err2 = NewGWorld(&offscreen[1], 8, &r, nil, nil, 0);
	
	if(err1 != noErr || err2 != noErr)
	{
		if(offscreen[0] != nil) { DisposeGWorld(offscreen[0]); offscreen[0] = nil; }
		if(offscreen[1] != nil) { DisposeGWorld(offscreen[1]); offscreen[1] = nil; }
		return 0;
	}


	pmap = GetGWorldPixMap(offscreen[0]);
	LockPixels(pmap);
	rowBytes = (*pmap)->rowBytes & 0x3fff; // mask off top two bits
	buffers[0] = GetPixBaseAddr(pmap);

	pmap = GetGWorldPixMap(offscreen[1]);
	LockPixels(pmap);
	buffers[1] = GetPixBaseAddr(pmap);
	
	*buffer1 = buffers[0];
	*buffer2 = buffers[1];

	mac_currentbuff = 0;
	
	return rowBytes;
}

// Deallocate fractal engine pixel buffers.
static void mac_free_buffers(char *buffer1, char *buffer2)
{
	
	int i;
	for(i=0; i<2; i++)
	{
		if(offscreen[i] != nil) { DisposeGWorld(offscreen[i]); offscreen[i] = nil; }
	}
}


static char *helptext[] =
{
    "MACINTOSH DRIVER VERSION 1.31          ",
    "====================================   ",
    "Written by Tapio K. Vocadlo            ",
    "Ottawa, Canada                         ",
    "(taps@rmx.com)                         ",
    "                                       ",
    "Shift-drag    = pan                    ",
    "Control-click = zoom out               ",
    "Cmd-period    = force quit at any time ",
    "                                       ",
    "See menubar for more commands.         ",
    "                                       ",
    "Will force 8-bit mode on monitor       ",
    "if necessary (and restore previous     ",
    "mode on exit). May not run if main     ",
    "monitor not 8-bit capable.             ",
    "                                       ",
    "If the message 'Could not allocate     ",
    "buffers' "appears after resizing the   ",
    "window larger, use the Finder's 'Get   ",
    "Info...' command to increase this      ",
    "application's Preferred Size memory    ",
    "requirement.                           "
     
};

#define UGLYTEXTSIZE (sizeof(helptext)/sizeof(char *))
static struct params params[] =
{
    {NULL, 0, NULL, NULL}
};

struct ui_driver mac_driver =
{
    "",
    mac_init,
    mac_get_size,
    mac_processevents,
    mac_getmouse,
    mac_uninitialise,
    mac_set_color,
    mac_print,
    mac_display,
    mac_alloc_buffers,
    mac_free_buffers,
    mac_flip_buffers,
    NULL,
    NULL,
    NULL,
    256,
    11,
    helptext,
    UGLYTEXTSIZE,
    params,
    UPDATE_AFTER_PALETTE | UPDATE_AFTER_RESIZE,
    0.0,0.0,
    0,0
};
#endif	// _MAC
