// The event types for XaoS on BeOS.
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

#ifndef XAOSEVENT_H
#define XAOSEVENT_H

#include <SupportDefs.h>

struct XaoSKeyEvent
{
  char bytes[6];
  int32 numBytes;
};

struct XaoSMouseEvent
{
  int x, y;
  ulong buttons;
  ulong modifiers;
};

union XaoSEvent
{
  enum
  {
    KeyDown = 'XaKd',
    KeyUp = 'XaKu',
    Mouse = 'XaMs',
    Resize = 'XaRs',
    Redraw = 'XaRe',
    Quit = 'XaQi'
  };

    XaoSEvent (void);
    XaoSEvent (const char *pBytes, int32 numBytes);
    XaoSEvent (int x, int y, ulong buttons, ulong modifiers);

  XaoSKeyEvent keyEvent;
  XaoSMouseEvent mouseEvent;
};

// Inline functions.

inline XaoSEvent::XaoSEvent (void)
{
  // empty
}

inline XaoSEvent::XaoSEvent (const char *pBytes, int32 numBytes)
{
  keyEvent.numBytes = numBytes;
  for (int32 i = 0; i < numBytes; ++i)
    {
      keyEvent.bytes[i] = pBytes[i];
    }
}

inline XaoSEvent::XaoSEvent (int x, int y, ulong buttons, ulong modifiers)
{
  mouseEvent.x = x;
  mouseEvent.y = y;
  mouseEvent.buttons = buttons;
  mouseEvent.modifiers = modifiers;
}

#endif // XAOSEVENT_H
