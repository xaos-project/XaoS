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

#include "XaoSView.h"
#include "XaoSEvent.h"

#include "ui.h"
#include "version.h"

XaoSView::XaoSView(BRect r, port_id p)
:	inherited(r, "XaoS " XaoS_VERSION,
				 B_FOLLOW_ALL_SIDES,
				 B_WILL_DRAW|B_FRAME_EVENTS),
	mpBuffer(0)
{
        mEventPort=p;
	SetEventMask(0, B_NO_POINTER_HISTORY);
}

XaoSView::~XaoSView(void)
{
	// empty
}

void XaoSView::AttachedToWindow(void)
{
	inherited::AttachedToWindow();

	// We will always Draw() the whole window.
	//SetViewColor(B_TRANSPARENT_32_BIT);
	SetViewColor(0,0,0);
	
	// Select the user's fixed-width font, but use ISO8859-1 encoding.
	BFont font(be_fixed_font);
	font.SetEncoding(B_ISO_8859_1);
	SetFont(&font);
	font.GetHeight(&mFontHeight);
	
	SetDrawingMode(B_OP_COPY);
	SetLowColor(255, 255, 255);
	SetHighColor(0, 0, 0);
}

void XaoSView::Draw(BRect /*updateRect*/)
{
	if (mpBuffer) {
	        SetViewColor(B_TRANSPARENT_32_BIT);
		if(async) DrawBitmapAsync(mpBuffer, B_ORIGIN);
		else DrawBitmap(mpBuffer, B_ORIGIN);
		Flush();
		if(!async) Sync();
	}
}

void XaoSView::FrameResized(float /*width*/, float /*height*/)
{
	SetViewColor(0,0,0);
	SendEvent(XaoSEvent::Resize, XaoSEvent());
}
void XaoSView::ScreenChanged(BRect frame, color_space mode)
{
	SetViewColor(0,0,0);
	SendEvent(XaoSEvent::Resize, XaoSEvent());
}

void XaoSView::KeyDown(const char *pBytes, int32 numBytes)
{
	SendEvent(XaoSEvent::KeyDown, XaoSEvent(pBytes, numBytes));
}

void XaoSView::KeyUp(const char *pBytes, int32 numBytes)
{
	SendEvent(XaoSEvent::KeyUp, XaoSEvent(pBytes, numBytes));
}

void XaoSView::MessageReceived(BMessage *pMessage)
{
	switch (pMessage->what) {

	case B_SELECT_ALL:
		// This message has the keyboard shortcut Command-A, which
		// we appropriate to show an "About" box.  Baad hack.
		be_app->PostMessage(B_ABOUT_REQUESTED);
		break;

	default:
		inherited::MessageReceived(pMessage);
		break;
	}
}

void XaoSView::MouseDown(BPoint point)
{
        int32 val=0;
	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS | B_NO_POINTER_HISTORY);
        Window()->CurrentMessage()->FindInt32("buttons", &val);
 
	SendEvent(XaoSEvent::Mouse,
				 XaoSEvent((int)point.x, (int)point.y, val, modifiers()));
}
void XaoSView::MouseUp(BPoint point)
{
        int32 val=0;
        Window()->CurrentMessage()->FindInt32("buttons", &val);
	SendEvent(XaoSEvent::Mouse,
				 XaoSEvent((int)point.x, (int)point.y, val, modifiers()));
}

void XaoSView::MouseMoved(
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

void XaoSView::WindowActivated(bool active)
{
	inherited::WindowActivated(active);
	if (active) {
		MakeFocus();
	}
}

void XaoSView::SetBuffer(BBitmap *pBuffer, int async)
{
	mpBuffer = pBuffer;
	async = async;
}

void XaoSView::GetTextSize(float &width, float &height)
{
	// Assumes a fixed-width font, of course.
	width = StringWidth("W");
	height = TextHeight();
}

void XaoSView::DrawText(
	BPoint leftTop, const char *const pText, int32 length)
{
	if (length > 0) {
		float width = StringWidth(pText, length);
		float height = TextHeight();
		FillRect(BRect(leftTop, leftTop + BPoint(width, height)),
					B_SOLID_LOW);
		DrawString(pText, length, leftTop + BPoint(0, mFontHeight.ascent));
		Sync();
	}
}
