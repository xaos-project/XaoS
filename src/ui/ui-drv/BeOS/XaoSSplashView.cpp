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

#include <Bitmap.h>
#include <Point.h>
#include <Rect.h>
#include <View.h>

#include "XaoSSplashView.h"

#include "version.h"

XaoSSplashView::XaoSSplashView(BBitmap *pSplashBitmap)
:	inherited(pSplashBitmap->Bounds(),
				 "About XaoS " XaoS_VERSION,
				 B_FOLLOW_ALL_SIDES,
				 B_WILL_DRAW),
	mpSplashBitmap(pSplashBitmap)
{
	// empty
}

XaoSSplashView::~XaoSSplashView(void)
{
	delete mpSplashBitmap;
}

void
XaoSSplashView::AttachedToWindow(void)
{
	inherited::AttachedToWindow();

	// We will always Draw() the whole window.
	SetViewColor(B_TRANSPARENT_32_BIT);
}

void
XaoSSplashView::Draw(BRect updateRect)
{
	DrawBitmap(mpSplashBitmap, B_ORIGIN);
}
