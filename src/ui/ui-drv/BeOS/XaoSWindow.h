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

#ifndef XAOSWINDOW_H
#define XAOSWINDOW_H

#include <View.h>
#include <Window.h>
#include "XaoSEvent.h"
#include "XaoSAbstractWindow.h"

class XaoSWindow:public BWindow, XaoSAbstractWindow
{
public:
  typedef BWindow inherited;

  // Constructor, destructor.
    XaoSWindow (BRect frame, port_id port);
    virtual ~ XaoSWindow (void);

  // Hook functions.
  virtual void FrameResized (float width, float height);
  virtual bool QuitRequested (void);
  virtual void MessageReceived (BMessage * pMessage);

  // Allow quit requests to succeed.
  virtual void AllowQuit (void);

private:
    XaoSWindow (const XaoSWindow & orig);
    XaoSWindow & operator = (const XaoSWindow & orig);
};

#endif // XAOSWINDOW_H
