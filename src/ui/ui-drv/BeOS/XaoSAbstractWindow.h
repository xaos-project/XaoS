// The Window class for XaoS on BeOS.
// Copyright © 1999 Jan Hubicka (hubicka@freesoft.cz)
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

#ifndef XAOSABSTRACTWINDOW_H
#define XAOSABSTRACTWINDOW_H

// The abstract window class covers common stuff for directwindow
// and normal window
#include <View.h>
#include "XaoSEvent.h"

class XaoSAbstractWindow
{
public:
  // Constructor, destructor.
  XaoSAbstractWindow (port_id port);
  ~XaoSAbstractWindow (void);

  // Hook functions.
  bool DoQuitRequested (void);
  // void DrawBuffer(char *buffer);

  // Allow quit requests to succeed.
  void DoAllowQuit (void);
  void PassMenuMessage (const BMessage * m);

  void SendEvent (long eventCode, const XaoSEvent & event) const;
private:
    port_id mEventPort;

  bool mQuitAllowed;
};
inline
XaoSAbstractWindow::XaoSAbstractWindow (port_id port)
{
  mEventPort = port;
  mQuitAllowed = FALSE;
}

inline
XaoSAbstractWindow::~
XaoSAbstractWindow ()
{
}

inline void
XaoSAbstractWindow::SendEvent (long eventCode, const XaoSEvent & event) const
{
  (void) write_port (mEventPort, eventCode, &event, sizeof (XaoSEvent));
}
inline void
XaoSAbstractWindow::PassMenuMessage (const BMessage * m)
{
  void *ptr;
  m->FindPointer ("Cmd", &ptr);
  SendEvent (XaoSEvent::Menu, ptr);
}

inline bool
XaoSAbstractWindow::DoQuitRequested (void)
{
  // Are we allowed to quit?
  if (!mQuitAllowed)
    {
      SendEvent (XaoSEvent::Quit, XaoSEvent ());
    }
  else
    {
      // Yep!
      be_app->PostMessage (B_QUIT_REQUESTED);
    }

  return mQuitAllowed;
}

inline void
XaoSAbstractWindow::DoAllowQuit ()
{
  // Allow quitting; called from the main thread just before
  // it posts B_QUIT_REQUESTED.
  mQuitAllowed = TRUE;
}
#endif // XAOSWINDOW_H
