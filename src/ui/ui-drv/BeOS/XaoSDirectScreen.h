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

#ifndef XAOSDIRECTSCREEN_H
#define XAOSDIRECTSCREEN_H

#include <View.h>
#include <WindowScreen.h>
#include "XaoSEvent.h"
#include "XaoSAbstractWindow.h"

typedef long (*blit_hook)(long,long,long,long,long,long);
typedef long (*sync_hook)();
class XaoSDirectScreen : public BWindowScreen, XaoSAbstractWindow
{
public:
	typedef BDirectWindow inherited;
	
	// Constructor, destructor.
	XaoSDirectScreen(port_id port, BView *v, status_t *error);
	virtual ~XaoSDirectScreen(void);

	// Hook functions.
	virtual  bool QuitRequested(void);

	// Allow quit requests to succeed.
	virtual  void AllowQuit(void);

	virtual void KeyDown(const char *pBytes, int32 numBytes);
	virtual void KeyUp(const char *pBytes, int32 numBytes);
	virtual void MessageReceived(BMessage *pMessage);
        virtual void   ScreenConnected(bool connected); 
	void Blit(char *t, int width, int height, int bpp);
	void DoBlit(void);
        void DoMouse(void);
	void SetColor(rgb_color *c, int start=0, int last=255);

        char         *fBits; 
        int32         fRowBytes; 
	int32	      fBytesPerPixel;
	int32	      fxstart,fystart;
	int32	      fWidth, fHeight;
	char	      rgba_order[4];
        color_space      fFormat; 
        bool         fDirty;      // needs refresh? 
        bool         fConnected; 
        bool         fConnectionDisabled; 
        BLocker         *locker; 
        void   UpdateParams(void); 
	void SetMode(int x, int y, int depth);
	void SetPointer (const char *pointer);
	void SetMouse (int x, int y);
private:
        XaoSDirectScreen (const XaoSDirectScreen &orig);
	XaoSDirectScreen &operator =(const XaoSDirectScreen &orig);
	const char *mousepointer;
	char *storeddata;
	int mouseX, mouseY;
	int oldMouseX, oldMouseY;
	BView *child;
	blit_hook blit;
	sync_hook sync;
	int mode;
	rgb_color colormap[256];

        int width, height, bpp;
        char *data;
};

#endif // XAOSWINDOW_H

