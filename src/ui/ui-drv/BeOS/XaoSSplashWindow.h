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

#ifndef XAOSSPLASHWINDOW_H
#define XAOSSPLASHWINDOW_H

#include <Message.h>
#include <OS.h>
#include <View.h>
#include <Window.h>

class XaoSSplashWindow : public BWindow
{
public:
	typedef BWindow inherited;
	
	// Constructor, destructor.
	XaoSSplashWindow(BView *const pView);
	virtual ~XaoSSplashWindow(void);

	// Hook functions.
	virtual void MessageReceived(BMessage *pMessage);
	virtual bool QuitRequested(void);
	
	// Wait for button press.
	bool Go(void);
	
private:
	XaoSSplashWindow(const XaoSSplashWindow &orig);
	XaoSSplashWindow &operator =(const XaoSSplashWindow &orig);
	
	// Data members.
	sem_id mButtonSem;
	bool mStartButtonPressed;
};

#endif // XAOSSPLASHWINDOW_H
