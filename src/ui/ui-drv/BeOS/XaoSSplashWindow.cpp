// Window class for "About XaoS" window.
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
#include <Button.h>
#include <Font.h>
#include <OS.h>
#include <Message.h>
#include <Point.h>
#include <Rect.h>
#include <View.h>
#include <Window.h>

#include "XaoSSplashWindow.h"

#include "version.h"

const BPoint kDefaultLocation(196, 192);
const char kStartButtonLabel[] = "Go!";
const uint32 kStartButtonPressed = 'Go  ';

static BRect
calcFrame(BRect viewBounds)
{
	viewBounds.OffsetBy(kDefaultLocation);
	return viewBounds;
}

XaoSSplashWindow::XaoSSplashWindow(BView *pView)
:	inherited(calcFrame(pView->Bounds()),
				 "XaoS " XaoS_VERSION,
				 B_FLOATING_WINDOW,
				 B_NOT_RESIZABLE|B_NOT_ZOOMABLE),
	mButtonSem(B_BAD_SEM_ID),
	mStartButtonPressed(false)
{
	AddChild(pView);
	
	// Create the start button.
	BRect emptyFrame(B_ORIGIN, B_ORIGIN);
	BButton *pButton =
		new BButton(emptyFrame, "button", kStartButtonLabel,
						new BMessage(kStartButtonPressed));
	pButton->SetTarget(this);
	pButton->ResizeToPreferred();
	float width, height;
	pButton->GetPreferredSize(&width, &height);
	BRect bounds(pView->Bounds());
	pButton->MoveTo(bounds.RightBottom() - BPoint(width+height/2, 1.5*height));
	pView->AddChild(pButton);
	SetDefaultButton(pButton);
}

XaoSSplashWindow::~XaoSSplashWindow(void)
{
	// empty
}

void
XaoSSplashWindow::MessageReceived(BMessage *pMessage)
{
	switch (pMessage->what) {
	
	case kStartButtonPressed:
		// Button pressed.
		mStartButtonPressed = true;
		if (mButtonSem >= B_NO_ERROR) {
			release_sem(mButtonSem);
		}
		break;
	
	default:
		inherited::MessageReceived(pMessage);
		break;
	}
}

bool
XaoSSplashWindow::QuitRequested(void)
{
	if (mButtonSem >= B_NO_ERROR) {
		release_sem(mButtonSem);
	}
	return true;
}

bool
XaoSSplashWindow::Go(void)
{
	Show();

	bool result = false;	
	if (Lock()) {
		mButtonSem = create_sem(0, "XaoS splash window");
		Unlock();
		acquire_sem(mButtonSem);
		result = mStartButtonPressed;
	}

	if (Lock()) {
		Quit();
	}
	
	return result;
}
