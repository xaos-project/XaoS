// A view class that just displays a bitmap.
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

#ifndef XAOSSPLASHVIEW_H
#define XAOSSPLASHVIEW_H

#include <Bitmap.h>
#include <View.h>

class XaoSSplashView : public BView {
public:
	typedef BView inherited;
	
	// Constructor, destructor.
	XaoSSplashView(BBitmap *pSplashBitmap);
	virtual ~XaoSSplashView(void);

	// Hook functions.
	virtual void AttachedToWindow(void);
	virtual void Draw(BRect updateRect);

private:
	XaoSSplashView(const XaoSSplashView &orig);
	XaoSSplashView &operator =(const XaoSSplashView &orig);
	
	// Data members.
	BBitmap *mpSplashBitmap;
};
	
#endif // XAOSSPLASHVIEW_H

