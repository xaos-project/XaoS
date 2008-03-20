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
#import "FractalView.h"

@interface NSObject(AppDelegateStuff)

- (void)keyPressed:(NSString *)key;

@end

@implementation FractalView

- (id)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
		mouseButton = mouseX = mouseY = currentBuffer = 0;
    }
    return self;
}

- (BOOL)isOpaque {
	return YES;
}

- (void)printText:(CONST char *)text atX:(int)x y:(int)y {
    messageText = [[NSString stringWithCString:text] retain];
    messageLocation = NSMakePoint(x, [self bounds].size.height - y);
    [self setNeedsDisplay:YES];
}

- (void)calculateMouseLocationFromEvent:(NSEvent *)theEvent {
	// Get location and translate coordinates to origin at upper left corner
	NSPoint mouseLoc = [self convertPoint:[theEvent locationInWindow] fromView:nil];
	NSRect bounds = [self bounds];
	mouseX = mouseLoc.x;
	mouseY = bounds.size.height - mouseLoc.y;
}

- (void)mouseDown:(NSEvent *)theEvent {
    [self calculateMouseLocationFromEvent:theEvent];

	/* Emulate 3 buttons based on modifier keys */
    mouseScrollWheel = 0;
	if ([theEvent modifierFlags] & NSControlKeyMask) {
		mouseButton = BUTTON3;
	} else if ([theEvent modifierFlags] & NSShiftKeyMask) {
		mouseButton = BUTTON2;
	} else {
		mouseButton = BUTTON1;
	}    
}

- (void)mouseUp:(NSEvent *)theEvent {
    mouseButton = 0;
}

- (void)mouseDragged:(NSEvent *)theEvent {
    [self calculateMouseLocationFromEvent:theEvent];
}

- (void)rightMouseDown:(NSEvent *)theEvent {
    [self calculateMouseLocationFromEvent:theEvent];
    mouseScrollWheel = 0;
    rightMouseButton = BUTTON3;
}

- (void)rightMouseUp:(NSEvent *)theEvent {
    rightMouseButton = 0;
}

- (void)rightMouseDragged:(NSEvent *)theEvent {
    [self calculateMouseLocationFromEvent:theEvent];
}

- (void)otherMouseDown:(NSEvent *)theEvent {
    [self calculateMouseLocationFromEvent:theEvent];
    mouseScrollWheel = 0;
    otherMouseButton = BUTTON2;
}

- (void)otherMouseUp:(NSEvent *)theEvent {
    otherMouseButton = 0;
}

- (void)otherMouseDragged:(NSEvent *)theEvent {
    [self calculateMouseLocationFromEvent:theEvent];
}

- (void)scrollWheel:(NSEvent *)theEvent {
    /* Only scroll if no mouse buttons are held */
    if ((mouseButton | rightMouseButton | otherMouseButton) == 0) {
        mouseScrollWheel = BUTTON2;
        mouseX += [theEvent deltaX];
        mouseY += [theEvent deltaY];
    }
}

- (void)flagsChanged:(NSEvent *)theEvent {
	/* Emulate 3 buttons based on modifier keys */
    if (mouseButton) {
        if ([theEvent modifierFlags] & NSControlKeyMask) {
            mouseButton = BUTTON3;
        } else if ([theEvent modifierFlags] & NSShiftKeyMask) {
            mouseButton = BUTTON2;
        } else {
            mouseButton = BUTTON1;
        }
    }
}

- (void)drawRect:(NSRect)rect {
	if (imageRep[currentBuffer]) {
        [imageRep[currentBuffer] drawInRect:[self bounds]];
	}
    
    if (messageText) {
        NSDictionary *attrsDictionary = 
                [NSDictionary dictionaryWithObject:[NSColor whiteColor] 
                                            forKey:NSForegroundColorAttributeName];
        [messageText drawAtPoint:messageLocation withAttributes:attrsDictionary];
        [messageText release];
    }
}

- (NSBitmapImageRep *)imageRep {
	return imageRep[currentBuffer];
}

- (int)allocBuffer1:(char **)b1 buffer2:(char **)b2 {
    currentBuffer = 0;
    /* Initialize image rep to current size of image view */
    NSRect bounds = [self bounds];
    imageRep[0] = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
                                                       pixelsWide:bounds.size.width
                                                       pixelsHigh:bounds.size.height
                                                    bitsPerSample:8
                                                  samplesPerPixel:3
                                                         hasAlpha:NO
                                                         isPlanar:NO
                                                   colorSpaceName:NSDeviceRGBColorSpace
                                                      bytesPerRow:0
                                                     bitsPerPixel:32];
	
	*b1 = (char *)[imageRep[0] bitmapData];

    imageRep[1] = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
                                                       pixelsWide:bounds.size.width
                                                       pixelsHigh:bounds.size.height
                                                    bitsPerSample:8
                                                  samplesPerPixel:3
                                                         hasAlpha:NO
                                                         isPlanar:NO
                                                   colorSpaceName:NSDeviceRGBColorSpace
                                                      bytesPerRow:0
                                                     bitsPerPixel:32];
	
	*b2 = (char *)[imageRep[1] bitmapData];

	return [imageRep[0] bytesPerRow];
}

- (void)freeBuffers {
    [imageRep[0] release];
    [imageRep[1] release];
}

- (void)getMouseX:(int *)mx mouseY:(int *)my mouseButton:(int *)mb {
	*mx = mouseX;
	*my = mouseY;
	*mb = mouseButton | rightMouseButton | otherMouseButton | mouseScrollWheel;
}

- (void)flipBuffers {
	currentBuffer ^= 1;
}

- (void)viewDidEndLiveResize {
    /* Reallocate image only after live resize is complete */
    ui_resize();
}

- (void)keyDown:(NSEvent *)e {
	[[[self window] delegate] keyDown:e];
}

- (void)keyUp:(NSEvent *)e {
	[[[self window] delegate] keyUp:e];
}

@end
