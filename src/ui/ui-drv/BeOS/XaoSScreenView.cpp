// The View class for XaoS on BeOS.
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

#include <cstring>

#include <AppDefs.h>
#include <Application.h>
#include <Autolock.h>
#include <Bitmap.h>
#include <Directory.h>
#include <File.h>
#include <Font.h>
#include <InterfaceDefs.h>
#include <Message.h>
#include <OS.h>
#include <Point.h>
#include <Rect.h>
#include <StorageDefs.h>
#include <SupportDefs.h>
#include <View.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include "XaoSScreenView.h"
#include "XaoSEvent.h"

#include "ui.h"
#include "version.h"

XaoSScreenView::XaoSScreenView(BRect r, port_id p)
:	inherited(r, "XaoS " XaoS_VERSION,
				 B_FOLLOW_ALL_SIDES,
				 B_WILL_DRAW|B_FRAME_EVENTS),
	mpBuffer(0)
{
        mEventPort=p;
	SetEventMask(0, B_NO_POINTER_HISTORY);
}

XaoSScreenView::~XaoSScreenView(void)
{
	// empty
}
void XaoSScreenView::MouseDown(BPoint point)
{
        int32 val=0;
	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS | B_NO_POINTER_HISTORY);
        Window()->CurrentMessage()->FindInt32("buttons", &val);
 
	SendEvent(XaoSEvent::Mouse,
				 XaoSEvent((int)point.x, (int)point.y, val, modifiers()));
}
void XaoSScreenView::MouseUp(BPoint point)
{
        int32 val=0;
        Window()->CurrentMessage()->FindInt32("buttons", &val);
	SendEvent(XaoSEvent::Mouse,
				 XaoSEvent((int)point.x, (int)point.y, val, modifiers()));
}

void XaoSScreenView::MouseMoved(
	BPoint point, uint32 transit, const BMessage * /*pMessage*/)
{
	// This method is needed to handle mouse movement when no button is pressed.
	// XaoS needs this for menu handling.
	if (transit != B_EXITED_VIEW) {
		int32 buttons = 0;
		Window()->CurrentMessage()->FindInt32("buttons", &buttons);
		SendEvent(XaoSEvent::Mouse,
					 XaoSEvent((int)point.x, (int)point.y, buttons, modifiers()));
	}
}
