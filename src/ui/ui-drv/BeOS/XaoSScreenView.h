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

#ifndef XAOSSCREENVIEW_H
#define XAOSSCREENVIEW_H

#include <Bitmap.h>
#include <Font.h>
#include <OS.h>
#include <Point.h>
#include <Rect.h>
#include <SupportDefs.h>
#include <View.h>

#include "XaoSEvent.h"

class XaoSScreenView:public BView
{
public:
  typedef BView inherited;

  // Constructor, destructor.
    XaoSScreenView (BRect r, port_id p);
    virtual ~ XaoSScreenView (void);

  // Hook functions.
  //virtual void AttachedToWindow(void);
  //virtual void Draw(BRect updateRect);
  //virtual void FrameResized(float width, float height);
  //virtual void ScreenChanged(BRect rect, color_space space);
  //virtual void KeyDown(const char *pBytes, int32 numBytes);
  //virtual void KeyUp(const char *pBytes, int32 numBytes);
  //virtual void MessageReceived(BMessage *pMessage);
  virtual void MouseDown (BPoint point);
  virtual void MouseUp (BPoint point);
  virtual void MouseMoved (BPoint point, uint32 transit,
			   const BMessage * pMessage);
  //virtual void WindowActivated(bool active);


protected:
    BBitmap * mpBuffer;
    XaoSScreenView (const XaoSScreenView & orig);
    XaoSScreenView & operator = (const XaoSScreenView & orig);
private:

  // Data members.
    port_id mEventPort;
  void SendEvent (long eventCode, const XaoSEvent & event) const;
};

// Inline functions.

inline void
XaoSScreenView::SendEvent (long eventCode, const XaoSEvent & event) const
{
  (void) write_port (mEventPort, eventCode, &event, sizeof (XaoSEvent));
}
#endif // XAOSVIEW_H
