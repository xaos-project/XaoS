/*
 *     XaoS, a fast portable realtime fractal zoomer 
 *                  Copyright © 1996,1997 by
 *
 *      Jan Hubicka          (hubicka@paru.cas.cz)
 *      Thomas Marsh         (tmarsh@austin.ibm.com)
 *
 *	ui_be.cpp	BeOS user interface code, Jens Kilian (jjk@acm.org)
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

#include <cstdio>
#include <cstring>
#include <malloc.h>

#include <Application.h>
#include <Autolock.h>
#include <Bitmap.h>
#include <ScrollBar.h>
#include <ByteOrder.h>
#include <GraphicsDefs.h>
#include <InterfaceDefs.h>
#include <StringView.h>
#include <View.h>
#include <Point.h>
#include <Rect.h>
#include <Screen.h>
#include <Box.h>
#include <Alert.h>

#include "XaoSWindow.h"
#include "XaoSDirectWindow.h"
#include "XaoSDirectScreen.h"
#include "XaoSMenu.h"
#include "XaoSView.h"
#include "MenuItem.h"
#include "XaoSDirectView.h"
#include "XaoSScreenView.h"
#include "XaoSEvent.h"
#include "XaoSDialog.h"

#include "version.h"
#include "cursor.h"
#include "fconfig.h"
#include "ui.h"
#include "xmenu.h"
#include "xerror.h"
#include "archaccel.h"

static void be_enabledisable(struct uih_context *c, CONST char *name);

static int visible=0;
static XaoSMenu *gpMenu;
static BView *mainView;
static BWindow *gpWindow;			// our BWindow
static XaoSWindow *gpNormalWindow;			// our BWindow for normal mode
static XaoSDirectWindow *gpDirectWindow;		// our BWindow for direct mode
static XaoSDirectScreen *gpDirectScreen;		// our BWindow for direct screen mode
static XaoSView *gpView;				// our BView
static XaoSDirectView *gpDirectView;			// our BView for direct mode
static BStringView *sView;
static BBox *boxView;
static int fullscreen;
static BRect *Previous=NULL;
static int isfullscreen;

const int NCOLORS = 256;				// # of colors in 8-bit mode
static int ignorequit=0;
static color_space gColorSpace;		// color space to use
static BBitmap *gpDisplayBuffer[2];		// display buffer
static size_t gBufferSize;	// size of buffer memory
static char *gpFractalBuffers[2] =	// fractal buffers
{
	0, 0
};
static int gCurrentBuffer;

extern int be_noalert;
extern struct ui_driver be_driver, be_direct_driver, be_screen_driver;
static int color8bit = 0;				// parameters settable from command line
static int bitmap = 0;
static int truecolor = 0;
static int hicolor = 0;
static int realcolor = 0;
static int grayscale = 0;
static char *window_size = 0;
static int view_width = XSIZE;
static int view_height = YSIZE;
static int async;
static CONST char *screen_mode="640x480x8";


static port_id mEventPort=0;
#define DIRECTWINDOW 1
#define DIRECTSCREEN 2
static int direct=0;

static void be_screen_display(void);

void be_get_imagespecs(int cs, int *imagetype, int *rmask, int *gmask, int *bmask)
{
        switch(cs) {
		case B_GRAY1:  
		  *imagetype = UI_MIBITMAP;
		  break;
		case B_GRAY8:  /* BeOS conversion routines don't seems to support this*/
		  *imagetype = UI_GRAYSCALE;
		  break;
		case B_CMAP8:
		  *imagetype = UI_FIXEDCOLOR;
		  break;
		case B_RGB24:  /*Untested*/
		case B_RGB24_BIG:
		  *imagetype = UI_TRUECOLOR24;
		  *rmask = 0x000000ff;
		  *gmask = 0x0000ff00;
		  *bmask = 0x00ff0000;
		  x_fatalerror("24bpp truecolor support is not compiled in. Contact authors\n");
		  break;
		case B_RGB32: 
		  *imagetype = UI_TRUECOLOR;
		  *rmask = B_HOST_TO_LENDIAN_INT32(0x00ff0000);
		  *gmask = B_HOST_TO_LENDIAN_INT32(0x0000ff00);
		  *bmask = B_HOST_TO_LENDIAN_INT32(0x000000ff);
		  break;
		case B_RGB32_BIG:   /*Untested*/
		  *imagetype = UI_TRUECOLOR;
		  *rmask = B_HOST_TO_BENDIAN_INT32(0x00ff0000);
		  *gmask = B_HOST_TO_BENDIAN_INT32(0x0000ff00);
		  *bmask = B_HOST_TO_BENDIAN_INT32(0x000000ff);
		  break;
                case B_RGB15:
                  *imagetype = UI_TRUECOLOR16;
                  *rmask = B_HOST_TO_LENDIAN_INT16(31 * 32 * 32);
                  *gmask = B_HOST_TO_LENDIAN_INT16(31 * 32);
                  *bmask = B_HOST_TO_LENDIAN_INT16(31);
                  break;
                case B_RGB16:
                  *imagetype = UI_TRUECOLOR16;
                  *rmask = B_HOST_TO_LENDIAN_INT16(31 * 64 * 32);
                  *gmask = B_HOST_TO_LENDIAN_INT16(63 * 32);
                  *bmask = B_HOST_TO_LENDIAN_INT16(31);
		  break;
                case B_RGB15_BIG:  /*Untested*/
                  *imagetype = UI_TRUECOLOR16;
                  *rmask = B_HOST_TO_BENDIAN_INT16(31 * 32 * 32);
                  *gmask = B_HOST_TO_BENDIAN_INT16(31 * 32);
                  *bmask = B_HOST_TO_BENDIAN_INT16(31);
                  break;
                case B_RGB16_BIG: /*Untested*/
                  *imagetype = UI_TRUECOLOR16;
                  *rmask = B_HOST_TO_BENDIAN_INT16(31 * 64 * 32);
                  *gmask = B_HOST_TO_BENDIAN_INT16(63 * 32);
                  *bmask = B_HOST_TO_BENDIAN_INT16(31);
		  break;
		default: x_fatalerror("Unsupported image type %i. Plase contact authors!", cs);

           }
}
// Return display buffer size.
static void be_get_size(int *x, int *y)
{
	// Select the display mode, trying to optimize performance.
	BScreen screen;
	color_space cs = screen.ColorSpace();

	const union {
		char c[4];
		int32 i;
	} test = { { 'B', 'e', 'O', 'S' } };
	const bool bigEndian = test.i == 'BeOS';
	if (!direct) {
		if (color8bit) cs=B_CMAP8;
		if (truecolor && cs!=B_RGB32 && cs!=B_RGB24 && cs!=B_RGB32_BIG && cs!=B_RGB24_BIG) cs=B_RGB32;
		if (hicolor && cs!=B_RGB16 && cs!=B_RGB16 && cs!=B_RGB16_BIG && cs!=B_RGB16_BIG) cs=B_RGB16;
		if (realcolor && cs!=B_RGB15 && cs!=B_RGB15 && cs!=B_RGB15_BIG && cs!=B_RGB15_BIG) cs=B_RGB15;
		if (grayscale) cs=B_GRAY8;
		if (bitmap) cs=B_GRAY1;
	} else {
	  /* Wait until cs is set. FIXME this is hack expecting that mov is atomic...grr */
          do {cs=gpDirectWindow->fFormat;} while(!cs);
        }
	if (cs==B_RGB16 || cs==B_RGB16_BIG) cs=bigEndian?B_RGB16_BIG: B_RGB16;
	if (cs==B_RGB15 || cs==B_RGB15_BIG) cs=bigEndian?B_RGB15_BIG: B_RGB15;
        be_get_imagespecs(cs, &be_driver.imagetype, &be_driver.rmask, &be_driver.gmask, &be_driver.bmask);

        gColorSpace=cs;
        be_direct_driver.imagetype = be_driver.imagetype;
        be_direct_driver.rmask = be_driver.rmask;
        be_direct_driver.gmask = be_driver.gmask;
        be_direct_driver.bmask = be_driver.bmask;
			
	*x = view_width;
	*y = view_height;
}

// XaoS has a somewhat different notion of buttons than BeOS ...
static int map_be_buttons(uint32 beButtons, uint32 /*modifiers*/)
{
	const uint32 bothButtons = B_PRIMARY_MOUSE_BUTTON|B_SECONDARY_MOUSE_BUTTON;
	if ((beButtons & bothButtons) == bothButtons) {
		// Left + right button -> middle, for serial mice (still buggy in DR9).
		return BUTTON2;
	}
	
	return ((beButtons & B_PRIMARY_MOUSE_BUTTON) ? BUTTON1 : 0)
		  | ((beButtons & B_SECONDARY_MOUSE_BUTTON) ? BUTTON3 : 0)
		  | ((beButtons & B_TERTIARY_MOUSE_BUTTON) ? BUTTON2 : 0);
}
void
be_setfullscreen()
{
	if(isfullscreen!=fullscreen)
	{
	  isfullscreen=fullscreen;
	  if(fullscreen) {
		     if (Previous) Previous=NULL;
                     Previous = new BRect(gpWindow->Frame());
                     BScreen a_screen(gpWindow);
                     gpWindow->MoveTo(a_screen.Frame().left, a_screen.Frame().top);
                     gpWindow->ResizeTo(a_screen.Frame().Width(), a_screen.Frame().Height());
	  }
	  else
	  {
                        gpWindow->ResizeTo(Previous->Width(), Previous->Height());
                        gpWindow->MoveTo(Previous->left, Previous->top);
			Previous = NULL;
			delete Previous;
	  }
	}
}
// Read the mouse.
static void be_getmouse(int *x, int *y, int *buttons)
{
	BPoint mouseLocation;
	uint32 mouseButtons;
	{	BAutolock locker(gpWindow);
		if (direct == DIRECTSCREEN)
		mainView->GetMouse(&mouseLocation, &mouseButtons);
		else
		gpView->GetMouse(&mouseLocation, &mouseButtons);
	}
	*x = (int)mouseLocation.x;
	*y = (int)mouseLocation.y;
	*buttons = map_be_buttons(mouseButtons, modifiers());
}

// Get next event from keyboard, mouse, etc.
static void be_processevents(int wait, int *x, int *y, int *buttons, int *k)
{
	static int currX = 0, currY = 0;
	static int currButtons = 0;
	static int cursorMask = 0;

        if (!visible) gpWindow->Show(), visible=0;
	if (wait && direct == DIRECTSCREEN) {
	     snooze(16000);
	     wait=0;
        }
	if (wait || port_count(/*gpView->EventPort()*/mEventPort) > 0) {
		do {
			// Read events from our message port.
			int32 eventCode;
			XaoSEvent event;
			if (read_port(/*gpView->EventPort()*/ mEventPort, &eventCode, &event, sizeof(XaoSEvent))
				 < B_NO_ERROR) {
				 break;
			}
			
			// Handle event.
			switch (eventCode) {

			case XaoSEvent::KeyDown:
				switch (event.keyEvent.bytes[0]) {

				case B_LEFT_ARROW:
					cursorMask |= 1;
					ui_key(UIKEY_LEFT);
					break;
				
				case B_RIGHT_ARROW:
					cursorMask |= 2;
					ui_key(UIKEY_RIGHT);
					break;
				
				case B_UP_ARROW:
					cursorMask |= 4;
					ui_key(UIKEY_UP);
					break;
				
				case B_DOWN_ARROW:
					cursorMask |= 8;
					ui_key(UIKEY_DOWN);
					break;

				case B_PAGE_UP:
					cursorMask |= 4;
					ui_key(UIKEY_PGUP);
					break;
				
				case B_PAGE_DOWN:
					cursorMask |= 8;
					ui_key(UIKEY_PGDOWN);
					break;

				case B_HOME:
					ui_key(UIKEY_HOME);
					break;
				
				case B_END:
					ui_key(UIKEY_END);
					break;

				case B_BACKSPACE:
					ui_key(UIKEY_BACKSPACE);
					break;
				
				case B_TAB:
					ui_key(UIKEY_TAB);
					break;
				
				case B_ESCAPE:
					ui_key(UIKEY_ESC);
					break;
				
				default:
					if (event.keyEvent.numBytes == 1) {
						if (event.keyEvent.bytes[0] == B_F1_KEY) {
							event.keyEvent.bytes[0] = 'h';
						}
						if (ui_key(event.keyEvent.bytes[0]) == 2) {
							return;
						}
					}
					break;
				}
				break;

			case XaoSEvent::KeyUp:
				switch (event.keyEvent.bytes[0]) {

				case B_LEFT_ARROW:
					cursorMask &= ~1;
					break;
				
				case B_RIGHT_ARROW:
					cursorMask &= ~2;
					break;
				
				case B_UP_ARROW:
					cursorMask &= ~4;
					break;
				
				case B_DOWN_ARROW:
					cursorMask &= ~8;
					break;

				default:
					break;
				}
				break;

			case XaoSEvent::Mouse:
				currX = event.mouseEvent.x;
				currY = event.mouseEvent.y;
				currButtons = map_be_buttons(event.mouseEvent.buttons,
													   event.mouseEvent.modifiers);
				break;

			case XaoSEvent::Quit: 
                            if (!ignorequit) ui_quit(); 
                            else ignorequit=0;
                            break;
			case XaoSEvent::Menu:
			    ui_menuactivate(event.command, NULL);
			    break;
			case XaoSEvent::Dialog:
			    ui_menuactivate(event.dialogEvent.command, event.dialogEvent.param);
			    break;
			case XaoSEvent::Redraw:
			    be_screen_display();
			    break;
			case XaoSEvent::Resize:
				{	
					if (direct == DIRECTSCREEN) ui_call_resize();
					else {
						BAutolock locker(gpWindow);
						BRect bounds = gpView->Bounds();
                                        	if(view_width != (int)bounds.Width()+1
                                           	|| view_height != (int)bounds.Height() + 1)
 						{
							view_width = (int)bounds.Width() + 1,
							view_height = (int)bounds.Height() + 1;
				        		ui_call_resize();
						}
					}
				}
				break;
			}
			
		} while (port_count(/*gpView->EventPort()*/mEventPort) > 0);
	}
	
	*x = currX;
	*y = currY;
	*buttons = currButtons;
        if(direct==DIRECTSCREEN)
          be_getmouse(x,y,buttons),
          gpDirectScreen->SetMouse(*x,*y);
	*k = cursorMask;
}


// Set the cursor shape.
static void be_mousetype(int type)
{
      switch (type) {
      
      case NORMALMOUSE:
              be_app->SetCursor(B_HAND_CURSOR);
              break;
      
      case WAITMOUSE:
              {
                      static const char cursor[] = {  // Clock face.
                              16, 1, 8, 8,
                              0x07, 0xe0, 0x1f, 0xf8, 0x38, 0x1c, 0x71, 0x8e,
                              0x61, 0x86, 0xc1, 0x83, 0xc1, 0x83, 0xc1, 0x83,
                              0xc1, 0xc3, 0xc0, 0xe3, 0xc0, 0x63, 0x60, 0x06,
                              0x70, 0x0e, 0x38, 0x1c, 0x1f, 0xf8, 0x07, 0xe0,
                              0x07, 0xe0, 0x1f, 0xf8, 0x3f, 0xfc, 0x7f, 0xfe,
                              0x7f, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, 0xfe,
                              0x7f, 0xfe, 0x3f, 0xfc, 0x1f, 0xf8, 0x07, 0xe0
                      };
                      be_app->SetCursor(cursor);
              }
              break;
      
      case REPLAYMOUSE:
              {
                      static const char cursor[] = {  // Stylized tape deck.
                              16, 1, 8, 8,
                              0x38, 0x38, 0x44, 0x44, 0x82, 0x82, 0x82, 0x82,
                              0x82, 0x82, 0x44, 0x44, 0x3f, 0xf8, 0x00, 0x00,
                              0x01, 0x00, 0x03, 0x80, 0x07, 0xc0, 0x07, 0xc0,
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                              0x38, 0x38, 0x7c, 0x7c, 0xfe, 0xfe, 0xfe, 0xfe,
                              0xfe, 0xfe, 0x7c, 0x7c, 0x3f, 0xf8, 0x00, 0x00,
                              0x01, 0x00, 0x03, 0x80, 0x07, 0xc0, 0x07, 0xc0,
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                      };
                      be_app->SetCursor(cursor);
              }
              break;
      }
}

// Display c-string <text> at x,y
static void be_print(int x, int y, CONST char *text)
{
	BAutolock locker(gpWindow);
	/*gpView->DrawText(BPoint(x, y), text, strlen(text));*/
        sView->SetText(text);
}

// Display pixel buffer from fractal engine to window.
static void be_display(void)
{
	BAutolock locker(gpWindow);
        if(direct) {
        gpDirectView->SetBuffer(gpFractalBuffers[gCurrentBuffer], view_width, view_height);
        } else {
        if(!async) {
            gpView->SetBuffer(gpDisplayBuffer[gCurrentBuffer], 1);
        } else {
	    memcpy(gpDisplayBuffer[0]->Bits(),
			     gpFractalBuffers[gCurrentBuffer],
			     gBufferSize);
        }
        }
 	gpView->Draw(gpView->Bounds());
}

// Allocate fractal engine pixel buffers.
static int be_alloc_buffers(char **buffer1, char **buffer2)
{
	// Allocate the display bitmap
	gCurrentBuffer = 0;
        if(!direct) {
                int i;
                for(i=0;i<(async?1:2);i++)
			gpDisplayBuffer[i] =
				new BBitmap(BRect(B_ORIGIN, BPoint(view_width-1, view_height-1)),
							gColorSpace);
		gBufferSize = gpDisplayBuffer[0]->BytesPerRow() * view_height;
		
                if(async) {
			// Allocate the fractal buffers.
			BAutolock locker(gpWindow);
			gpView->SetBuffer(gpDisplayBuffer[0],0);
			for (int i = 0; i < 2; ++i) {
				gpFractalBuffers[i] = new char[gBufferSize];
			}
                } else {
			gpFractalBuffers[0] = (char *)gpDisplayBuffer[0]->Bits();
			gpFractalBuffers[1] = (char *)gpDisplayBuffer[1]->Bits();
               	}
	       *buffer1 = gpFractalBuffers[0];
	       *buffer2 = gpFractalBuffers[1];
	       return gpDisplayBuffer[0]->BytesPerRow();
        } else {
		int linesize=view_width*gpDirectWindow->fBytesPerPixel;
		linesize=(linesize+15)&~15;
		for (int i = 0; i < 2; ++i) {
			gpFractalBuffers[i] = new char[linesize*view_height];
		}
		*buffer1 = gpFractalBuffers[0];
		*buffer2 = gpFractalBuffers[1];
		return linesize;
        }
	
}

// Deallocate fractal engine pixel buffers.
static void be_free_buffers(char * /*buffer1*/, char * /*buffer2*/)
{
	BAutolock locker(gpWindow);
        if (!direct) {
	  gpView->SetBuffer(0,0);
	  delete gpDisplayBuffer[0];
	  gpDisplayBuffer[0] = 0;
          if(!async)
	    delete gpDisplayBuffer[1];
	  gpDisplayBuffer[1] = 0;
        } else
	  gpDirectView->SetBuffer(0,0,0);
	
	if(async || direct)
	for (int i = 0; i < 2; ++i) {
		delete [] gpFractalBuffers[i];
		gpFractalBuffers[i] = 0;
	}
}

// Switch current pixel buffer.
static void be_flip_buffers(void)
{
	gCurrentBuffer ^= 1;
}

static void be_set_range(ui_palette palette, int start, int end)
{
	// Tell XaoS about our fixed color map.
	BScreen screen;
	const color_map *pColorMap = screen.ColorMap();
	
	for (int i = start; i < end; ++i) {
		palette[i-start][0] = pColorMap->color_list[i].red;
		palette[i-start][1] = pColorMap->color_list[i].green;
		palette[i-start][2] = pColorMap->color_list[i].blue;
	}
}

/*sizes of resizing button. FIXME: How to get this?*/
#define WIDTH B_V_SCROLL_BAR_WIDTH
#define HEIGHT B_H_SCROLL_BAR_HEIGHT
#define BORDERWIDTH 2
// Set up the driver.
static int do_be_init(void)
{
        int window_width=XSIZE;
        int window_height=YSIZE;
        int menuheight;
	status_t error = B_OK;
	// Evaluate parameters.
	isfullscreen=0;
        if (!mEventPort) mEventPort = create_port(100, "XaoS "XaoS_VERSION);
	if (window_size) {
		int width, height;
		sscanf(window_size, "%dx%d", &width, &height);
		if (width > 0) {
			window_width = width;
		}
		if (height > 1) {
			window_height = height;
		}
	}
      	BRect windowRect(0,0,window_width,window_height);
	if (direct) {
	  gpDirectWindow = new XaoSDirectWindow(windowRect, mEventPort, &error);
	  gpWindow = gpDirectWindow;
          if (error!=B_OK) {
             gpDirectWindow->AllowQuit();
             gpDirectWindow->QuitRequested();
	     gpWindow = NULL;
          }
        } else {
          gpNormalWindow = new XaoSWindow(windowRect,  mEventPort);
	  gpWindow = gpNormalWindow;
        }
        if(gpWindow != 0)
        {
	 	BAutolock locker(gpWindow);
        	gpMenu = new XaoSMenu(mEventPort, "Menu Bar", window_width, window_height-HEIGHT);
               	gpWindow->AddChild(gpMenu->menu);
		menuheight = (int)(gpMenu->menu->Bounds().bottom + 1.0);

		BRect viewRect(0,menuheight,window_width+1,window_height-HEIGHT);
        	if(direct)
        	{
	  		gpDirectView = new XaoSDirectView(viewRect, mEventPort);
	  		gpView = gpDirectView;
        	} else
			gpView = new XaoSView(viewRect, mEventPort);

		if (gpView != 0) {
			BRect bounds=gpView->Bounds();
                	view_width = (int)bounds.Width()+1;
                	view_height = (int)bounds.Height()+1;
	
                	BRect boxRect(0,window_height-HEIGHT+2,window_width-WIDTH,window_height);
	        	boxView = new BBox(boxRect, "status-box",
                                   B_FOLLOW_BOTTOM | B_FOLLOW_LEFT_RIGHT, 
                                   B_WILL_DRAW | B_FRAME_EVENTS, B_PLAIN_BORDER);
                	boxView->SetViewColor(216, 216, 216, 0);
			BRect r=boxView->Bounds();
			r.InsetBy(1,1);
			r.bottom+=2;
	        	sView = new BStringView(r, "status-bar", "ahoj",
                                   B_FOLLOW_BOTTOM | B_FOLLOW_LEFT_RIGHT);
                	sView->SetAlignment(B_ALIGN_LEFT);

                	boxView->AddChild(sView);
                	gpWindow->AddChild(boxView, gpMenu->menu);
                	gpWindow->AddChild(gpView, boxView);
			
			/*gpWindow->Show();*/
			visible=0;
                        be_setfullscreen();
			return 1;
		} /*gpView !=0*/
		delete gpMenu;
                gpDirectWindow->AllowQuit();
                gpDirectWindow->QuitRequested();
	        gpWindow = NULL;
	} /* gpWindow*/

	return 0;
}
void be_about(void)
{
	be_app->PostMessage(B_ABOUT_REQUESTED);
}
void be_fullscreen(void)
{
	fullscreen^=1;
	be_setfullscreen();
        be_enabledisable(NULL, "fullscreen");
}
int be_is_fullscreen(void)
{
	return fullscreen;
}

void be_mode(struct uih_context *c, int mode)
{
	bitmap=truecolor=hicolor=realcolor=grayscale=color8bit=0;
        switch(mode)
	{
	   case 1: bitmap=1;
            be_enabledisable(NULL, "grayscale1");
           break;
	   case 2: grayscale=1;
            be_enabledisable(NULL, "grayscale8");
           break;
	   case 3: color8bit=1;
            be_enabledisable(NULL, "color8bit");
           break;
	   case 4: realcolor=1;
            be_enabledisable(NULL, "truecolor15");
           break;
	   case 5: hicolor=1;
            be_enabledisable(NULL, "truecolor16");
           break;
	   case 6: truecolor=1;
            be_enabledisable(NULL, "truecolor32");
           break;
	}
	ui_call_resize();
}
int be_modeselected(struct uih_context *c, int mode)
{
	int m=0;
	if(bitmap) m=1;
	if(grayscale) m=2;
	if(color8bit) m=3;
	if(realcolor) m=4;
	if(hicolor) m=5;
	if(truecolor) m=6;
	return m==mode;
}
void be_copy(struct uih_context *c)
{
	char *str;
	if (be_clipboard->Lock()) {
		str=ui_getpos();
		be_clipboard->Clear();
		BMessage *c=be_clipboard->Data();
		c->AddData("image/x-xaos-position", B_MIME_TYPE, str, strlen(str));
		be_clipboard->Commit();
		be_clipboard->Unlock();
		free(str);
	}
}
void be_paste(struct uih_context *c)
{
	const void *str;
	ssize_t length;
	if (be_clipboard->Lock()) {
		BMessage *c=be_clipboard->Data();
		if (c->FindData("image/x-xaos-position", B_MIME_TYPE, &str, &length) == B_OK) {
			char *str1=(char *)calloc(length+1,1);
			memcpy(str1,str,length);
			ui_loadstr(str1);
		}
		be_clipboard->Unlock();
	}
}
static CONST menuitem winmenuitems[]=
{
   MENUSEPARATOR("file"),
   MENUNOP ("file",NULL,"About XaoS","about",MENUFLAG_INCALC,be_about),


   MENUNOPCB ("ui",NULL,"Fullscreen","fullscreen",MENUFLAG_INCALC,be_fullscreen, be_is_fullscreen),
   SUBMENU ("ui", NULL, "Bitmap mode","bitmapm"),
   MENUINTRB ("bitmapm",NULL,"Default","default",MENUFLAG_INTERRUPT,(void (*)())be_mode, 0, (int (*)())be_modeselected),
   MENUSEPARATOR("bitmapm"),
   MENUINTRB ("bitmapm",NULL,"1BPP grayscale","grayscale1",MENUFLAG_INTERRUPT,(void (*)())be_mode, 1, (int (*)())be_modeselected),
   MENUINTRB ("bitmapm",NULL,"8BPP grayscale","grayscale8",MENUFLAG_INTERRUPT,(void (*)())be_mode, 2, (int (*)())be_modeselected),
   MENUINTRB ("bitmapm",NULL,"256 colors","color8bit",MENUFLAG_INTERRUPT,(void (*)())be_mode, 3, (int (*)())be_modeselected),
   MENUINTRB ("bitmapm",NULL,"32768 colors","truecolor15",MENUFLAG_INTERRUPT,(void (*)())be_mode, 4, (int (*)())be_modeselected),
   MENUINTRB ("bitmapm",NULL,"65536 colors","truecolor16",MENUFLAG_INTERRUPT,(void (*)())be_mode, 5, (int (*)())be_modeselected),
   MENUINTRB ("bitmapm",NULL,"16777216 colors","truecolor32",MENUFLAG_INTERRUPT,(void (*)())be_mode, 6, (int (*)())be_modeselected),
   MENUSEPARATOR("edit"),
   MENUNOP ("edit",NULL,"Copy","copy",MENUFLAG_INCALC,(void (*)())be_copy),
   MENUNOP ("edit",NULL,"Paste","paste",MENUFLAG_INTERRUPT,(void (*)())be_paste),
};
static CONST menuitem directmenuitems[]=
{
   MENUSEPARATOR("file"),
   MENUNOP ("file",NULL,"About XaoS","about",MENUFLAG_INCALC,be_about),
   MENUNOPCB ("ui",NULL,"fullscreen","fullscreen",MENUFLAG_INCALC,be_fullscreen, be_is_fullscreen),
   MENUSEPARATOR("edit"),
   MENUNOP ("edit",NULL,"Copy","copy",MENUFLAG_INCALC,(void (*)())be_copy),
   MENUNOP ("edit",NULL,"Paste","paste",MENUFLAG_INTERRUPT,(void (*)())be_paste),
};
static CONST char * CONST depth[] =
{"8bpp (256 colors)",
 "16bpp (65536 colors)",
 "32bpp (16777216 colors)",
 NULL};
static CONST char * CONST resolution[] =
{"640x480",
 "800x600",
 "1024x768",
 "1152x800",
 "1280x1024",
 "1600x1200",
 NULL};
static menudialog be_screen_resdialog[] =
{
  DIALOGCHOICE ("Resolution", resolution, 0),
  DIALOGCHOICE ("Depth", depth, 0),
  {NULL}
};
static menudialog *
be_screen_resizedialog(uih_context *c)
{
   if(gpDirectScreen!=NULL) {
     switch(gpDirectScreen->fBytesPerPixel)
     {
       case 1:be_screen_resdialog[1].defint=0; break;
       case 2:be_screen_resdialog[1].defint=1; break;
       case 4:be_screen_resdialog[1].defint=2; break;
     }
     switch(gpDirectScreen->fWidth)
     {
       case 640:be_screen_resdialog[0].defint=0; break;
       case 800:be_screen_resdialog[0].defint=1; break;
       case 1024:be_screen_resdialog[0].defint=2; break;
       case 1151:be_screen_resdialog[0].defint=3; break;
       case 1280:be_screen_resdialog[0].defint=4; break;
       case 1600:be_screen_resdialog[0].defint=5; break;
     }
   }
   return be_screen_resdialog;
}
static void
be_screen_resize(struct uih_context *c, dialogparam *p)
{
  int w,h,d;
  CONST char * CONST depths[]={
    "8",
    "16",
    "32",
  };
  static char mode[10];
  sprintf(mode,"%sx%s",resolution[p[0].dint],depths[p[1].dint]);
  screen_mode=mode;
  sscanf(screen_mode, "%ix%ix%i",&w,&h,&d);
  gpDirectScreen->SetMode(w,h,d);
}
static menuitem screenmenuitems[]=
{
   MENUCDIALOG ("ui", "=", "Resize", "resize", MENUFLAG_INTERRUPT, (void(*)())be_screen_resize, be_screen_resizedialog),
   MENUSEPARATOR("edit"),
   MENUNOP ("edit",NULL,"Copy","copy",MENUFLAG_INCALC,(void (*)())be_copy),
   MENUNOP ("edit",NULL,"Paste","paste",MENUFLAG_INTERRUPT,(void (*)())be_paste),
   //MENUSEPARATOR("file"),
   //MENUNOP ("file",NULL,"About XaoS","about",MENUFLAG_INCALC,be_about),
};
static int be_init()
{
        direct=0;
	menu_add (winmenuitems, NITEMS (winmenuitems));
        return do_be_init();
}
static int be_direct_init()
{
	if(!BDirectWindow::SupportsWindowMode()) return 0;
        direct=DIRECTWINDOW;
	menu_add (directmenuitems, NITEMS (directmenuitems));
        return do_be_init();
}

// Deallocate any stuff we allocated at startup.
static void be_do_uninit()
{
	// Tell the window that it may quit, and to do so.
	if (direct!=DIRECTSCREEN)
	  gpWindow->Hide(), gpWindow->Sync(), gpMenu->cleanMenu(), delete gpMenu;
        if (!direct)
	  gpNormalWindow->AllowQuit();
        else
	  if (direct==DIRECTWINDOW)
	    gpDirectWindow->AllowQuit();
	  else
	    gpDirectScreen->AllowQuit();

	gpWindow->PostMessage(B_QUIT_REQUESTED);
        ignorequit=1;
	XaoSDialog::cleanup();
        /*destroy_port(mEventPort);*/
}
static void be_uninit()
{
	be_do_uninit();
	menu_delete (winmenuitems, NITEMS (winmenuitems));
}
static void be_direct_uninit()
{
	be_do_uninit();
	menu_delete (directmenuitems, NITEMS (directmenuitems));
}
static int be_screen_init(void)
{
	status_t error = B_OK;
        BRect mainRect(0,0,65536,65536);
	direct=DIRECTSCREEN;

	// Evaluate parameters.
	visible=1;
	isfullscreen=0;
        mEventPort = create_port(100, "XaoS "XaoS_VERSION);
        mainView = new XaoSScreenView(mainRect, mEventPort); 
        mainView->SetViewColor(215,215,215);
	gpDirectScreen = new XaoSDirectScreen(mEventPort, mainView, &error);
	gpWindow = gpDirectScreen;
        if (error!=B_OK) {
	     //delete (gpDirectScreen);
             gpDirectScreen->AllowQuit();
             gpDirectScreen->QuitRequested();
	     gpWindow = NULL;
	     delete(mainView);
	     return 0;
	}
	int w=640,h=480,d=8;	
	sscanf(screen_mode, "%ix%ix%i",&w,&h,&d);
	gpDirectScreen->SetMode(w,h,d);
        gpDirectScreen->SetPointer((const char *)mouse_pointer_data);
	menu_add (screenmenuitems, NITEMS (screenmenuitems));
	be_noalert=1;
	return 1;
}
static void be_screen_uninit()
{
	be_do_uninit();
	be_noalert=0;
	menu_delete (screenmenuitems, NITEMS (screenmenuitems));
}
static void be_screen_get_size(int *x, int *y)
{
	gpDirectScreen->locker->Lock();
	gpDirectScreen->UpdateParams();
	gpDirectScreen->locker->Unlock();
	view_width=*x=gpDirectScreen->fWidth;
	view_height=*y=gpDirectScreen->fHeight;
	switch(gpDirectScreen->fBytesPerPixel)
	{
		case 1: be_screen_driver.imagetype = UI_C256; break;
		case 2: be_screen_driver.imagetype = UI_TRUECOLOR16; 
                        be_screen_driver.rmask = 31 * 64 * 32;
                        be_screen_driver.gmask = 63 * 32;
                        be_screen_driver.bmask = 31;
			break;
		case 3: be_screen_driver.imagetype = UI_TRUECOLOR24; 
		        be_screen_driver.bmask = 0x000000ff;
		        be_screen_driver.gmask = 0x0000ff00;
		        be_screen_driver.rmask = 0x00ff0000;
                        break;
		case 4: be_screen_driver.bmask = 0x000000ff;
		        be_screen_driver.gmask = 0x0000ff00;
		        be_screen_driver.rmask = 0x00ff0000;
                        be_screen_driver.imagetype = UI_TRUECOLOR; break;
	}
	
}
static void be_screen_set_range(ui_palette palette, int start, int end)
{
        rgb_color c[256];
	int i;
	for(i=start;i<end;i++) {
		c[i].red=palette[i-start][0];
		c[i].green=palette[i-start][1];
		c[i].blue=palette[i-start][2];
        }
	gpDirectScreen->SetColor(c+start,start,end-1);
/*
	// Tell XaoS about our fixed color map.
	BScreen screen;
	const color_map *pColorMap = screen.ColorMap();
	
	for (int i = start; i < end; ++i) {
		palette[i-start][0] = pColorMap->color_list[i].red;
		palette[i-start][1] = pColorMap->color_list[i].green;
		palette[i-start][2] = pColorMap->color_list[i].blue;
	}*/
}
static void be_screen_print(int x, int y,CONST char *text)
{
}
static void be_screen_mousetype(int t)
{
  switch (t)
    {
    default:
    case 0:
      gpDirectScreen->SetPointer((const char *)mouse_pointer_data);
      break;
    case 1:
      gpDirectScreen->SetPointer((const char *)wait_pointer_data);
      break;
    case 2:
      gpDirectScreen->SetPointer((const char *)replay_pointer_data);
      break;
    }
}
static int bytes_per_pixel;
static void be_screen_display(void)
{
	gpDirectScreen->Blit(gpFractalBuffers[gCurrentBuffer], view_width, view_height, bytes_per_pixel);
}
static int be_screen_alloc_buffers(char **b1, char **b2)
{
	int linesize=gpDirectScreen->fWidth*gpDirectScreen->fBytesPerPixel;
	linesize=(linesize+15)&~15;
	gCurrentBuffer = 0;
	*b1=gpFractalBuffers[0] = new char[linesize*view_height];
	*b2=gpFractalBuffers[1] = new char[linesize*view_height];
	bytes_per_pixel=gpDirectScreen->fBytesPerPixel;
	return linesize;
}
static void be_screen_free_buffers(char *b1, char *b2)
{
	delete gpFractalBuffers[0];
	delete gpFractalBuffers[1];
}

CONST static struct params params[] =
{
    {"", P_HELP, NULL,"BeOS driver options:"},
    { "-async", P_SWITCH, &async, "Asynchronous bitmap drawing" },
    { "-bitmap", P_SWITCH, &bitmap, "1-bit grayscale mode" },
    { "-grayscale", P_SWITCH, &grayscale, "8-bit grayscale mode" },
    { "-8bit", P_SWITCH, &color8bit, "8-bit palette mode" },
    { "-15bit", P_SWITCH, &realcolor, "15-bit true color mode" },
    { "-16bit", P_SWITCH, &hicolor, "16-bit true color mode" },
    { "-32bit", P_SWITCH, &truecolor, "32-bit true color mode" },
    { "-size", P_STRING, &window_size, "set window size (WIDTHxHEIGHT)" },
    { "-fullscreen", P_SWITCH, &fullscreen, "set fullscreen mode" },
    { NULL, 0, NULL, NULL }
};
CONST static struct params directparams[] =
{
    {"", P_HELP, NULL,"BeOS DirectWindow driver options:"},
    { "-size", P_STRING, &window_size, "set window size (WIDTHxHEIGHT)" },
    { "-fullscreen", P_SWITCH, &fullscreen, "set fullscreen mode" },
    { NULL, 0, NULL, NULL }
};
CONST static struct params screenparams[] =
{
    {"", P_HELP, NULL,"BeOS DirectWindow driver options:"},
    { "-mode", P_STRING, &screen_mode, "set window size (WIDTHxHEIGHTxMODE)" },
    { NULL, 0, NULL, NULL }
};
static void
be_dorootmenu(struct uih_context *c, CONST char *name)
{
   BAutolock locker(gpWindow);
   gpMenu->setMenu(c,name);
   if (!visible) gpWindow->Show(), visible=0;
}
static void
be_enabledisable(struct uih_context *c, CONST char *name)
{
   BAutolock locker(gpWindow);
   CONST menuitem *i=menu_findcommand(name);
   gpMenu->EnableDisable(i);
}
static void
be_menu(struct uih_context *c, CONST char *name)
{
   BPoint p;
   int x,y,b;
   BAutolock locker(gpWindow);
   be_getmouse(&x, &y, &b);
   p.x=x-5;
   p.y=y-5;
   p=gpView->ConvertToScreen(p);
   XaoSMenu *m=new XaoSMenu(mEventPort, name, p, gpWindow);
   gpMenu->AddToList(m);
   m->setMenu(c, name);
}
static void
be_dialog(struct uih_context *c, CONST char *name)
{
   BAutolock locker(gpWindow);
   new XaoSDialog(gpWindow->Frame(), name, mEventPort, c);
}
void
be_help(struct uih_context *c, CONST char *name)
{
   char s[256];
   char *s1;
   xio_file f;
   sprintf(s,"\01/help/%s.html",name);
   if((f=xio_ropen(s))) {
	xio_close(f);
   } else {
     sprintf(s,"\01/../help/%s.html",name);
     if((f=xio_ropen(s))) {
	xio_close(f);
     } else {
	x_error("Help file %s not found",s);
	return;
     }
   }
   s1=xio_fixpath(s);
   sprintf(s,"NetPositive %s &",s1);
   system(s);
   free(s1);
}

extern "C" {
struct gui_driver be_gui_driver =
{
   be_dorootmenu, /*dorootmenu*/
   be_enabledisable, /*enabledisable*/
   be_menu, /*menu*/
   be_dialog, /*dialog*/
   be_help, /*help*/
};
struct ui_driver be_driver =
{
    "BeOS",
    be_init,
    be_get_size,
    be_processevents,
    be_getmouse,
    be_uninit,
    NULL,						// set_color()
    be_set_range,
    be_print,
    be_display,
    be_alloc_buffers,
    be_free_buffers,
    be_flip_buffers,
    be_mousetype,
    NULL,						// flush()
    0,							// text width (see be_init)
    0,							// text height (see be_init)
    params,						// command-line parameters
    UPDATE_AFTER_PALETTE|PIXELSIZE|RESOLUTION,
    0.02979, 0.02930,		// this is for my monitor in 1024x768
    1024, 768,
    0,							// image type (see be_init)
    0, NCOLORS-1, NCOLORS,
    0, 0, 0,
    &be_gui_driver
};
struct ui_driver be_direct_driver =
{
    "DirectWindow",
    be_direct_init,
    be_get_size,
    be_processevents,
    be_getmouse,
    be_direct_uninit,
    NULL,						// set_color()
    be_set_range,
    be_print,
    be_display,
    be_alloc_buffers,
    be_free_buffers,
    be_flip_buffers,
    be_mousetype,
    NULL,						// flush()
    0,							// text width (see be_init)
    0,							// text height (see be_init)
    directparams,						// command-line parameters
    UPDATE_AFTER_PALETTE|PIXELSIZE|RESOLUTION,
    0.02979, 0.02930,		// this is for my monitor in 1024x768
    1024, 768,
    0,							// image type (see be_init)
    0, NCOLORS-1, NCOLORS,
    0, 0, 0,
    &be_gui_driver
};
struct ui_driver be_screen_driver =
{
    "WindowScreen",
    be_screen_init,
    be_screen_get_size,
    be_processevents,
    be_getmouse,
    be_screen_uninit,
    NULL,						// set_color()
    be_screen_set_range,
    be_screen_print,
    be_screen_display,
    be_screen_alloc_buffers,
    be_screen_free_buffers,
    be_flip_buffers,
    be_screen_mousetype,
    NULL,						// flush()
    0,							// text width (see be_init)
    0,							// text height (see be_init)
    screenparams,						// command-line parameters
    UPDATE_AFTER_PALETTE|PIXELSIZE|RESOLUTION,
    0.02979, 0.02930,		// this is for my monitor in 1024x768
    1024, 768,
    0,							// image type (see be_init)
    0, 256, 255,
    //10, 128+10,128,
    0, 0, 0
};
}
