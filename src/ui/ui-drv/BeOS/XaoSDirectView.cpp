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

#include "XaoSDirectView.h"
#include "XaoSEvent.h"
#include "XaoSDirectWindow.h"

#include "ui.h"
#include "version.h"

XaoSDirectView::XaoSDirectView(BRect r, port_id p)
:	inherited(XaoSView(r, p))
{
	buffer=NULL;
}

XaoSDirectView::~XaoSDirectView(void)
{
	// empty
}

inline void
XaoSDirectView::DoDraw(void)
{
    XaoSDirectWindow *w=(XaoSDirectWindow *)Window();
	if (buffer) {
             int32 i; 
             int32 adder; 
             int32 xstart;
             int32 ystart;
             int32 xend;
             int32 yend;
             char *img;
             int32 width; 
             int32 height; 
	     int32 y;
             clipping_rect *clip; 
	     char *p;

             int32 img_linewidth = (bufferwidth*w->fBytesPerPixel+15)&~15;
             if(w->fConnectionDisabled) return;
             w->locker->Lock();
	     if(w->fConnected && !w->fConnectionDisabled && buffer) {
             adder = w->fRowBytes;   // Stash locally for this pass 
             for (i=0; i<(int)w->fNumClipRects; i++) { 
                clip = &(w->fClipList[i]); 
                xstart = clip->left;
                ystart = clip->top;
                if (xstart<Rect.left) xstart=(int)Rect.left;
                if (ystart<Rect.top) ystart=(int)Rect.top;
                xend = clip->right+1;
                yend = clip->bottom+1;
                if (xend>Rect.right) xend=(int)Rect.right;
                if (yend>Rect.bottom) yend=(int)Rect.bottom;
                if (xend>Rect.left+bufferwidth) xend=(int)Rect.left+bufferwidth;
                if (yend>Rect.top+bufferheight) yend=(int)Rect.top+bufferheight;
                img = buffer + (xstart-(int)Rect.left)*w->fBytesPerPixel + (ystart-(int)Rect.top)*img_linewidth;
		width=xend-xstart;
		height=yend-ystart;
                width*=w->fBytesPerPixel;
		if(width<=0 || height<=0) continue;
		p=(char *)w->fBits+(ystart*w->fRowBytes)+xstart*w->fBytesPerPixel;
	        for(y=0;y<height;y++)
                  memcpy(p,img, width), p+=adder, img+=img_linewidth;
		}
             }
             w->locker->Unlock();
	}
}
static int32 DrawingThread(void *data) { 
    XaoSDirectView *v=(XaoSDirectView *)data; 
    v->DoDraw();
    return B_OK; 
}
void XaoSDirectView::Draw(BRect /*updateRect*/)
{
	status_t result;
        XaoSDirectWindow *w=(XaoSDirectWindow *)Window();
        BRect r1=w->Frame();
        Rect=Frame();
	//printf("%i %i %i %i\n",bufferwidth, (int)(Rect.right-Rect.left, bufferheight, (int)Rect.);
        if (bufferwidth - 1 != (int)(Rect.right-Rect.left) ||
            bufferheight - 1 != (int)(Rect.bottom-Rect.top)) return;
        Rect.top+=r1.top;
        Rect.bottom+=r1.bottom;
        Rect.left+=r1.left;
        Rect.right+=r1.right;
        SetViewColor(B_TRANSPARENT_32_BIT);
	fDrawThreadID = spawn_thread(DrawingThread, "XaoS drawing_thread", 
               	B_DISPLAY_PRIORITY, (void *) this); 
	//resume_thread(fDrawThreadID); 
	wait_for_thread(fDrawThreadID, &result); 
}


