// The Window class for XaoS on BeOS.
// Copyright © 1997  Jens Kilian (jjk@acm.org)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include <Application.h>
#include <GraphicsDefs.h>
#include <Message.h>
#include <MessageQueue.h>
#include <Rect.h>
#include <Screen.h>
#include <View.h>
#include <Window.h>
#include <Point.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "archaccel.h"

#include "XaoSDirectScreen.h"
#include "XaoSView.h"
#include "XaoSEvent.h"
#include "version.h"
#define MOUSEWIDTH 16
#define MOUSEHEIGHT 16

const BPoint kDefaultLocation(100, 100);


#define DEFAULTMODE B_8_BIT_640x480
XaoSDirectScreen::XaoSDirectScreen(port_id p, BView *v, status_t *error)
:	BWindowScreen("XaoS " XaoS_VERSION, DEFAULTMODE, error),
        XaoSAbstractWindow(p)
{
        if (*error == B_OK) {
		fConnected = false; 
		fConnectionDisabled = false; 
		locker = new BLocker(); 
		fDirty = true; 
		mode = DEFAULTMODE;
		Show(); 
		AddChild(v);
		child=v;
		fWidth=10;
		fHeight=10;
		fBytesPerPixel=4;
		storeddata=NULL;
		mousepointer=NULL;
		mouseX=mouseY=0;
	} 
}
/* Find the correct mode for resolution */
void
XaoSDirectScreen::SetMode(int x, int y, int depth)
{
   int i;
   static struct
   {
      int x,y,depth,constant;
   } infos[] =
   {
      {640,480,8,B_8_BIT_640x480},
      {640,480,16,B_16_BIT_640x480},
      {640,480,32,B_32_BIT_640x480},
      {800,600,8,B_8_BIT_800x600},
      {800,600,16,B_16_BIT_800x600},
      {800,600,32,B_32_BIT_800x600},
      {1024,768,8,B_8_BIT_1024x768},
      {1024,768,16,B_16_BIT_1024x768},
      {1024,768,32,B_32_BIT_1024x768},
      {1152,900,8,B_8_BIT_1152x900},
      {1152,900,16,B_16_BIT_1152x900},
      {1152,900,32,B_32_BIT_1152x900},
      {1280,1024,8,B_8_BIT_1280x1024},
      {1280,1024,16,B_16_BIT_1280x1024},
      {1280,1024,32,B_32_BIT_1280x1024},
      {1600,1200,8,B_8_BIT_1600x1200},
      {1600,1200,16,B_16_BIT_1600x1200},
      {1600,1200,32,B_32_BIT_1600x1200},
      {0,0,8,B_8_BIT_640x480},
   };
   for(i=0;infos[i].x&&(infos[i].x!=x || infos[i].y!=y || infos[i].depth!=depth);i++);
   mode=infos[i].constant;
   UpdateParams();
}

XaoSDirectScreen::~XaoSDirectScreen(void)
{
	fConnectionDisabled = true;      // Connection is dying 
	Hide(); 
	Sync(); 
	delete locker; 
}
void
XaoSDirectScreen::UpdateParams(void) {
   int changed=0;

   if (!fConnected)  return;

   if (SetSpace(mode)!=B_OK) SetSpace(B_8_BIT_640x480);
   graphics_card_info *c=CardInfo();

   SetFrameBuffer(c->width,c->height);
   frame_buffer_info *f=FrameBufferInfo();

   fxstart = f->display_x;
   fystart = f->display_y;

   if(fBytesPerPixel != (int)(c->bits_per_pixel/8))
     fBytesPerPixel = c->bits_per_pixel/8, changed=1;
   fRowBytes = c->bytes_per_row;
   fBits = (char *)c->frame_buffer;

   if(fWidth != c->width) changed=1;
   if(fHeight != c->height) changed=1;
   fWidth = c->width;
   fHeight = c->height;

   memcpy(rgba_order, c->rgba_order, sizeof(rgba_order));
   SetColorList(colormap);
   if (changed)
	SendEvent(XaoSEvent::Resize, XaoSEvent());
   else SendEvent(XaoSEvent::Redraw, XaoSEvent());
}

void XaoSDirectScreen::ScreenConnected(bool connected) { 
   //if(!fConnected && connected) SendEvent(XaoSEvent::Redraw, XaoSEvent());
   locker->Lock(); 
   fConnected = connected; 
   if(fConnected) {
	//SetSpace(B_32_BIT_640x480);
	//SetFrameBuffer(640,480);
   	UpdateParams();
        blit=(blit_hook)CardHookAt(7);
        sync=(sync_hook)CardHookAt(10);
	//SetEventMask(B_MOUSE_EVENT, B_NO_POINTER_HISTORY | B_LOCK_WINDOW_FOCUS);
   }
   locker->Unlock(); 
}

/*
void XaoSDirectScreen::FrameResized(float width, float height)
{
	// Remove queued-up resize-messages.
	BMessageQueue *pQueue = MessageQueue();
	BMessage *pMessage;
	while ((pMessage = pQueue->FindMessage(B_WINDOW_RESIZED, 0)) != 0) {
		pQueue->RemoveMessage(pMessage);
	}
		

	inherited::FrameResized(width, height);
}
*/



void
XaoSDirectScreen::AllowQuit(void) 
{
     DoAllowQuit();
}
bool
XaoSDirectScreen::QuitRequested(void) 
{
     return(DoQuitRequested());
}

/* Mouse drawing helper functions*/
static char *
store (const char *data, int depth, int lpitch, int width, int height, int xpos, int ypos)
{
  int d = depth;
  char *store = (char *)malloc (d * MOUSEWIDTH * MOUSEHEIGHT);
  int y;
  if (xpos + MOUSEWIDTH > width)
    xpos = width - MOUSEWIDTH;
  if (ypos + MOUSEHEIGHT > height)
    ypos = height - MOUSEHEIGHT;
  if (xpos < 0)
    xpos = 0;
  if (ypos < 0)
    ypos = 0;
  for (y = 0; y < MOUSEHEIGHT; y++)
    memcpy (store + d * MOUSEWIDTH * y, data + xpos * d + (ypos + y) * lpitch, MOUSEWIDTH * d);
  return store;
}
static void 
restore (char *data, const char *store, int depth, int lpitch, int width, int height, int xpos, int ypos)
{
  int y;
  int d = depth;
  if (xpos + MOUSEWIDTH > width)
    xpos = width - MOUSEWIDTH;
  if (ypos + MOUSEHEIGHT > height)
    ypos = height - MOUSEHEIGHT;
  if (xpos < 0)
    xpos = 0;
  if (ypos < 0)
    ypos = 0;
  for (y = 0; y < MOUSEHEIGHT; y++)
    memcpy (data + xpos * d + (ypos + y) * lpitch, store + d * MOUSEWIDTH * y, MOUSEWIDTH * d);
}
static void 
drawmouse (char *data, const char *mouse, int depth, int lpitch, int width, int height, int xpos, int ypos)
{
  int x, y, z, c;
  int d = depth;
  for (y = 0; y < MOUSEWIDTH; y++)
    for (x = 0; x < MOUSEWIDTH; x++)
      if (mouse[x + MOUSEWIDTH * y] && x + xpos > 0 && (x + xpos) < width && y + ypos > 0 && y + ypos < height)
	{
	  c = mouse[x + MOUSEWIDTH * y] == 2 ? (d == 1 ? 1 : 255) : 0;
	  for (z = 0; z < d; z++)
	    data[z + d * (x + xpos) + (y + ypos) * lpitch] = c;
	}
}
void
XaoSDirectScreen::DoBlit(void)
{
     locker->Lock();
     int linesize=(fWidth*bpp+15)&~15;
     char *buff = fBits+(fxstart)*fBytesPerPixel+fystart*fRowBytes;
     int y;
     if (width!=fWidth || height!=fHeight || !fConnected || bpp != fBytesPerPixel) {locker->Unlock(); return;}
      if(mousepointer) {
      if (storeddata)
        free(storeddata), storeddata = NULL;
	oldMouseX=mouseX;
	oldMouseY=mouseY;
      	storeddata = store (data, fBytesPerPixel, linesize, fWidth, fHeight, mouseX, mouseY);
      	drawmouse (data, mousepointer, fBytesPerPixel, linesize, fWidth, fHeight, mouseX, mouseY);

      }
     for(y=0;y<height;y++)
        memcpy(buff+y*fRowBytes, data+y*linesize, linesize);
      if(mousepointer) {
        restore (data, storeddata, fBytesPerPixel, linesize, fWidth, fHeight, mouseX, mouseY);
      }
 
     locker->Unlock();
     //blit(0,0,0,0,width,height);
     //if (sync) sync();
}
static int32 DrawingThread(void *data) { 
    XaoSDirectScreen *v=(XaoSDirectScreen *)data; 
    v->DoBlit();
    return B_OK; 
}
void
XaoSDirectScreen::Blit(char *d, int w, int h, int b)
{
     status_t result;
     data=d;
     width=w;
     height=h;
     bpp=b;
     int fDrawThreadID = spawn_thread(DrawingThread, "XaoS direct screen drawing_thread", 
               	B_DISPLAY_PRIORITY, (void *) this); 
     wait_for_thread(fDrawThreadID, &result); 
}
void XaoSDirectScreen::DoMouse(void)
{
   locker->Lock(); 
   char *buff = fBits+(fxstart)*fBytesPerPixel+fystart*fRowBytes;
   if(oldMouseX==mouseX && oldMouseY==mouseY) {locker->Unlock();return;}
   if(fConnected && mousepointer && storeddata) restore (buff, storeddata, fBytesPerPixel, fRowBytes, fWidth, fHeight, oldMouseX, oldMouseY);
   oldMouseX=mouseX;
   oldMouseY=mouseY;
   if(fConnected && mousepointer) {
	free(storeddata);
      	storeddata = store (buff, fBytesPerPixel, fRowBytes, fWidth, fHeight, mouseX, mouseY);
      	drawmouse (buff, mousepointer, fBytesPerPixel, fRowBytes, fWidth, fHeight, mouseX, mouseY);
   }
   locker->Unlock(); 
}
static int32 MouseThread(void *data) { 
    XaoSDirectScreen *v=(XaoSDirectScreen *)data; 
    v->DoMouse();
    return B_OK; 
}
void XaoSDirectScreen::SetMouse(int x, int y)
{
   status_t result;
   if(mouseX==x && mouseY==y) return;
   mouseX=x;
   mouseY=y;
   int fDrawThreadID = spawn_thread(MouseThread, "XaoS mouse drawing_thread", 
               	B_DISPLAY_PRIORITY, (void *) this); 
   wait_for_thread(fDrawThreadID, &result); 
}
void XaoSDirectScreen::SetPointer(const char *pointer)
{
   int x=mouseX, y=mouseY;
   mousepointer=pointer;
   SetMouse(0,0); /*update mouse on the screen*/
   SetMouse(x,y);
}
void XaoSDirectScreen::MessageReceived(BMessage *pMessage)
{
	switch (pMessage->what) {

	case B_SELECT_ALL:
		// This message has the keyboard shortcut Command-A, which
		// we appropriate to show an "About" box.  Baad hack.
		be_app->PostMessage(B_ABOUT_REQUESTED);
		break;
	case B_QUIT_REQUESTED: QuitRequested();
	        break;
	case B_KEY_DOWN: 
		{
		const char *bytes;
		pMessage->FindString("bytes",&bytes);
		KeyDown(bytes, strlen(bytes));
		}
		break;
	case B_KEY_UP:break;
#if 0
	case B_MOUSE_DOWN: 
                {BPoint point;
		pMessage->PrintToStream();
		 pMessage->FindPoint("point",&point);
                 MouseDown(point);
                }
	case B_MOUSE_MOVED: 
                {/*MouseMoved();*/
		pMessage->PrintToStream();
                }
#endif
	}
}

void XaoSDirectScreen::KeyDown(const char *pBytes, int32 numBytes)
{
	SendEvent(XaoSEvent::KeyDown, XaoSEvent(pBytes, numBytes));
}

void XaoSDirectScreen::KeyUp(const char *pBytes, int32 numBytes)
{
	SendEvent(XaoSEvent::KeyUp, XaoSEvent(pBytes, numBytes));
}

void
XaoSDirectScreen::SetColor(rgb_color *c, int start, int last)
{
    memcpy(colormap+start, c, (last-start)*sizeof(*c));
    SetColorList(colormap,0,255);
}

