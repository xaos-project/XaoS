/*
 *     XaoS, a fast portable realtime fractal zoomer 
 *                  Copyright (C) 1996 by
 *
 *      Jan Hubicka          (hubicka@paru.cas.cz)
 *      Thomas Marsh         (tmarsh@austin.ibm.com)
 *
 *    Cocoa Driver by J.B. Langston III (jb-langston@austin.rr.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#import <Cocoa/Cocoa.h>

#include "ui.h"

#ifdef VIDEATOR_SUPPORT
@ class VideatorProxy;
#endif

@interface FractalView: NSOpenGLView {
    int mouseX, mouseY;
    int mouseButton, rightMouseButton, otherMouseButton, mouseScrollWheel;
    int keysDown;
    int cursorType;
    int width, height;

    int currentBuffer;
    //NSBitmapImageRep *imageRep[2];
    unsigned char *buffer[2];

    NSString *messageText;
    NSPoint messageLocation;

#ifdef VIDEATOR_SUPPORT
    VideatorProxy *videatorProxy;
#endif
}

#pragma mark Buffers
-(int) allocBuffer1:(char **)b1 buffer2:(char **) b2;
-(void) freeBuffers;
-(void) flipBuffers;

#pragma mark Accessors

#ifdef VIDEATOR_SUPPORT
-(VideatorProxy *) videatorProxy;
#endif
-(void) getWidth:(int *)w height:(int *) h;
-(void) getMouseX:(int *)mx mouseY:(int *)my mouseButton:(int *) mb;
-(void) getMouseX:(int *)mx mouseY:(int *)my mouseButton:(int *)mb keys:(int *) k;

#pragma mark Cursor
-(void) setCursorType:(int) type;

#pragma mark Text
-(void) printText:(CONST char *)text atX:(int)x y:(int) y;
- (NSDictionary *) textAttributes;

@end
