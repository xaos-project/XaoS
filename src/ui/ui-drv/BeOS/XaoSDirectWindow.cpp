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
#include <Point.h>
#include <Rect.h>
#include <Screen.h>
#include <View.h>
#include <Window.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "XaoSDirectWindow.h"
#include "XaoSView.h"
#include "XaoSEvent.h"
#include "version.h"
#include "archaccel.h"

const BPoint kDefaultLocation(100, 100);

static BRect calcFrame(BRect viewBounds)
{
	viewBounds.OffsetBy(kDefaultLocation);
	return viewBounds;
}

XaoSDirectWindow::XaoSDirectWindow(BRect frame, port_id p, status_t *error)
:	inherited(calcFrame(frame),
				 "XaoS " XaoS_VERSION,
				 B_DOCUMENT_WINDOW,
				 0),
        XaoSAbstractWindow(p)
{
	fConnected = false; 
	fConnectionDisabled = false; 
	fFormat = (color_space)0;
	locker = new BLocker(); 
	fClipList = NULL; 
	fNumClipRects = 0; 

	//AddChild(pView);

	if (!SupportsWindowMode()) { 
		return;
		*error=B_OK+1;
   		SetFullScreen(true); 
	} 

	fDirty = true; 
	Show(); 
}

XaoSDirectWindow::~XaoSDirectWindow(void)
{
	fConnectionDisabled = true;      // Connection is dying 
	Hide(); 
	Sync(); 
	free(fClipList); 
	delete locker; 
}
void XaoSDirectWindow::DirectConnected(direct_buffer_info *info) { 
   if (!fConnected && fConnectionDisabled) { 
      return; 
   } 
   locker->Lock(); 
   
   switch(info->buffer_state & B_DIRECT_MODE_MASK) { 
      case B_DIRECT_START: 
         fConnected = true; 
      case B_DIRECT_MODIFY: 
         fBits = (uint8 *) info->bits; 
         fRowBytes = info->bytes_per_row; 
         fBytesPerPixel = info->bits_per_pixel/8; 
         fFormat = info->pixel_format; 
         fBounds = info->window_bounds; 
         fDirty = true; 
         
         // Get clipping information 
         
         if (fClipList) { 
            free(fClipList); 
            fClipList = NULL; 
         } 
         fNumClipRects = info->clip_list_count; 
         fClipList = (clipping_rect *) 
               malloc(fNumClipRects*sizeof(clipping_rect)); 
         memcpy(fClipList, info->clip_list, 
               fNumClipRects*sizeof(clipping_rect));         
         break; 
      case B_DIRECT_STOP: 
         fConnected = false; 
         break; 
   } 
   locker->Unlock(); 
}

void XaoSDirectWindow::FrameResized(float width, float height)
{
	// Remove queued-up resize-messages.
	BMessageQueue *pQueue = MessageQueue();
	BMessage *pMessage;
	while ((pMessage = pQueue->FindMessage(B_WINDOW_RESIZED, 0)) != 0) {
		pQueue->RemoveMessage(pMessage);
	}
		

	inherited::FrameResized(width, height);
}



void
XaoSDirectWindow::AllowQuit(void) 
{
     DoAllowQuit();
}
bool
XaoSDirectWindow::QuitRequested(void) 
{
     return(DoQuitRequested());
}

/*FIXME: this is kludge and can be handled elsewhere */
void XaoSDirectWindow::MessageReceived(BMessage *pMessage)
{
	if(pMessage->what == 'XaCm') PassMenuMessage(pMessage);
        else inherited::MessageReceived(pMessage);
}
