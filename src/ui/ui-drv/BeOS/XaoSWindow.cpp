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

#include "XaoSWindow.h"
#include "XaoSView.h"
#include "XaoSEvent.h"
#include "version.h"

const BPoint kDefaultLocation(100, 100);
#include <malloc.h>
static BRect calcFrame(BRect viewBounds)
{
	viewBounds.OffsetBy(kDefaultLocation);
	return viewBounds;
}

XaoSWindow::XaoSWindow(BRect frame, port_id p)
:	inherited(calcFrame(frame),
				 "XaoS " XaoS_VERSION,
				 B_DOCUMENT_WINDOW,
				 0),
        XaoSAbstractWindow(p)
{
	// empty
}

XaoSWindow::~XaoSWindow(void)
{
	// empty
}

void XaoSWindow::FrameResized(float width, float height)
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
XaoSWindow::AllowQuit(void) 
{
     DoAllowQuit();
}
bool
XaoSWindow::QuitRequested(void) 
{
     return(DoQuitRequested());
}



/* FIXME: this can be hadnled elsewhere */
void XaoSWindow::MessageReceived(BMessage *pMessage)
{
	if(pMessage->what == 'XaCm') PassMenuMessage(pMessage);
        else inherited::MessageReceived(pMessage);
}
