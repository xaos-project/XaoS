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

#ifndef XAOSDIRECTVIEW_H
#define XAOSDIRECTVIEW_H

#include <Bitmap.h>
#include <Font.h>
#include <OS.h>
#include <Point.h>
#include <Rect.h>
#include <SupportDefs.h>
#include <View.h>
#include "XaoSView.h"

#include "XaoSEvent.h"

class XaoSDirectView : public XaoSView
{
public:
	typedef XaoSView inherited;
	
	// Constructor, destructor.
	XaoSDirectView(BRect r, port_id p);
        void SetBuffer(char *buffer, int w, int h);
	virtual ~XaoSDirectView(void);

	// Create thread and call next function
	virtual void Draw(BRect updateRect);
	// Draw that can be called only from non-main thread to avoid deadlocks
	void DoDraw(void);
private:
        char *buffer;
	int bufferwidth, bufferheight;
        thread_id      fDrawThreadID; 
	BRect  Rect;
};

// Inline functions.
inline void
XaoSDirectView::SetBuffer(char *b, int w, int h)
{
   buffer=b;
   bufferwidth=w;
   bufferheight=h;
}


#endif // XAOSVIEW_H
