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

#ifndef XAOSDIRECTWINDOW_H
#define XAOSDIRECTWINDOW_H

#include <View.h>
#include <DirectWindow.h>
#include "XaoSEvent.h"
#include "XaoSAbstractWindow.h"

class XaoSDirectWindow : public BDirectWindow, XaoSAbstractWindow
{
public:
	typedef BDirectWindow inherited;
	
	// Constructor, destructor.
	XaoSDirectWindow(BRect frame, port_id port, status_t *error);
	virtual ~XaoSDirectWindow(void);

	// Hook functions.
	virtual void FrameResized(float width, float height);
	virtual  bool QuitRequested(void);

	// Allow quit requests to succeed.
	virtual  void AllowQuit(void);


        virtual void   DirectConnected(direct_buffer_info *info); 
	virtual void MessageReceived(BMessage *pMessage);

        uint8         *fBits; 
        int32         fRowBytes; 
	int32	      fBytesPerPixel;
        color_space      fFormat; 
        clipping_rect   fBounds; 
        
        uint32         fNumClipRects; 
        clipping_rect   *fClipList; 
         
        bool         fDirty;      // needs refresh? 
        bool         fConnected; 
        bool         fConnectionDisabled; 
        BLocker         *locker; 
private:
        XaoSDirectWindow(const XaoSDirectWindow &orig);
	XaoSDirectWindow &operator =(const XaoSDirectWindow &orig);
};

#endif // XAOSWINDOW_H

