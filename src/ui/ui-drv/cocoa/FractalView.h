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

@interface FractalView : NSView {
	int mouseX, mouseY;
    int mouseButton, rightMouseButton, otherMouseButton, mouseScrollWheel;
	int currentBuffer;
	NSBitmapImageRep *imageRep[2];
    NSString *messageText;
    NSPoint messageLocation;
}
- (int)allocBuffer1:(char **)b1 buffer2:(char **)b2;
- (void)freeBuffers;
- (void)getMouseX:(int *)mx mouseY:(int *)my mouseButton:(int *)mb;
- (void)printText:(CONST char *)text atX:(int)x y:(int)y;
- (void)flipBuffers;
- (NSBitmapImageRep *)imageRep;
@end
