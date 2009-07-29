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
#ifdef VIDEATOR_SUPPORT
#import "VideatorProxy.h"
#endif

@interface NSObject(AppDelegateStuff)

- (void)keyPressed:(NSString *)key;

@end

@implementation FractalView

#pragma mark Initialization

- (id)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
#ifdef VIDEATOR_SUPPORT
        videatorProxy = [[VideatorProxy alloc] init];
#endif
    }
    return self;
}

- (void)dealloc {
#ifdef VIDEATOR_SUPPORT
    [videatorProxy release];
#endif
    [super dealloc];
}

#pragma mark Drawing

- (BOOL)isOpaque {
    return YES;
}

- (void)drawRect:(NSRect)rect {
    if (imageRep[currentBuffer]) {
        [imageRep[currentBuffer] drawInRect:[self bounds]];
    }
    
    if (messageText) {
        [messageText drawAtPoint:messageLocation withAttributes:[self textAttributes]];
        [messageText release];
        messageText = nil;
    }
    
#ifdef VIDEATOR_SUPPORT
    [videatorProxy sendImageRep:imageRep[currentBuffer]];
#endif    
}

#pragma mark Resize Handling

- (void)viewDidEndLiveResize {
    ui_resize();
}

#pragma mark Mouse Event Handling

- (void)calculateMouseLocationFromEvent:(NSEvent *)theEvent {
    NSPoint mouseLoc = [self convertPoint:[theEvent locationInWindow] fromView:nil];
    mouseX = mouseLoc.x;
    mouseY = [self bounds].size.height - mouseLoc.y;
}

- (void)mouseDown:(NSEvent *)theEvent {
    [self calculateMouseLocationFromEvent:theEvent];
    
    // Emulate 3 buttons based on modifier keys
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
    // Only scroll if no mouse buttons are held
    if ((mouseButton | rightMouseButton | otherMouseButton) == 0) {
        mouseScrollWheel = BUTTON2;
        mouseX += [theEvent deltaX];
        mouseY += [theEvent deltaY];
    }
}

#pragma mark Keyboard Event Handling

- (void)flagsChanged:(NSEvent *)theEvent {
    // Emulate 3 buttons based on modifier keys 
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

- (void)keyDown:(NSEvent *)e {
    NSString *characters = [e characters];
    if ([characters length] == 0) return;
    
    unichar keyChar = [characters characterAtIndex:0];
    switch(keyChar) {
      case NSLeftArrowFunctionKey:
          keysDown |= 1;
          ui_key(UIKEY_LEFT);
          break;
      case NSRightArrowFunctionKey:
          keysDown |= 2;
          ui_key(UIKEY_RIGHT);
          break;
      case NSUpArrowFunctionKey:
          keysDown |= 4;
          ui_key(UIKEY_UP);
          break;
      case NSDownArrowFunctionKey:
          keysDown |= 8;
          ui_key(UIKEY_DOWN);
          break;
      case NSBackspaceCharacter:
          ui_key(UIKEY_BACKSPACE);
          break;
      case NSEndFunctionKey:
          ui_key(UIKEY_END);
          break;
      case '\033': // Escape
          ui_key(UIKEY_ESC);
          break;
      case NSHomeFunctionKey:
          ui_key(UIKEY_HOME);
          break;
      case NSPageDownFunctionKey:
          ui_key(UIKEY_PGDOWN);
          break;
      case NSPageUpFunctionKey:
          ui_key(UIKEY_PGUP);
          break;
      case NSTabCharacter:
          ui_key(UIKEY_TAB);
          break;
      default:
          ui_key(keyChar);
    }
}

- (void)keyUp:(NSEvent *)e {
    NSString *characters = [e characters];
    if ([characters length] == 0) return;
    
    unichar keyChar = [characters characterAtIndex:0];
    switch(keyChar)	{
      case NSLeftArrowFunctionKey:
          keysDown &= ~1;
          break;
      case NSRightArrowFunctionKey:
          keysDown &= ~2;
          break;
      case NSUpArrowFunctionKey:
          keysDown &= ~4;
          break;
      case NSDownArrowFunctionKey:
          keysDown &= ~8;
          break;
    }
}

#pragma mark Accessors

#ifdef VIDEATOR_SUPPORT
- (VideatorProxy *)videatorProxy {
    return videatorProxy;
}
#endif

- (void)getWidth:(int *)w height:(int *)h {
    NSRect bounds = [self bounds];
    *w = bounds.size.width;
    *h = bounds.size.height;
}

- (void)getMouseX:(int *)mx mouseY:(int *)my mouseButton:(int *)mb {
    *mx = mouseX;
    *my = mouseY;
    *mb = mouseButton | rightMouseButton | otherMouseButton | mouseScrollWheel;
}

- (void)getMouseX:(int *)mx mouseY:(int *)my mouseButton:(int *)mb keys:(int *)k
{
    [self getMouseX:mx mouseY:my mouseButton:mb];
    *k = keysDown;
}

#pragma mark Cursor

- (void)setCursorType:(int)type {
    cursorType = type;
    [[self window] invalidateCursorRectsForView:self];
}

- (void)resetCursorRects {
    /* BUG - cursor changes back to arrow when mouse is clicked 
     if (cursorType == VJMOUSE) {
     NSCursor *cursor = [[NSCursor alloc] initWithImage:[NSImage imageNamed:@"performanceCursor"] hotSpot:NSMakePoint(1.0,1.0)];
     [cursor setOnMouseEntered:YES];
     [self addCursorRect:[self bounds] cursor:cursor];
     [cursor release];
     } */
}


#pragma mark Text

- (NSDictionary *)textAttributes {
	
	NSMutableDictionary *attrsDictionary = [NSMutableDictionary dictionaryWithObject:[NSColor whiteColor] 
																			  forKey:NSForegroundColorAttributeName];
	
	NSShadow *dockStyleTextShadow = [[NSShadow alloc] init];
	[dockStyleTextShadow setShadowOffset:NSMakeSize(2, -2)];
	[dockStyleTextShadow setShadowBlurRadius:1];
	[dockStyleTextShadow setShadowColor:[NSColor blackColor]];
	[attrsDictionary setValue:[NSFont boldSystemFontOfSize:12.0] forKey:NSFontAttributeName];
	[attrsDictionary setValue:dockStyleTextShadow forKey:NSShadowAttributeName];
	[dockStyleTextShadow autorelease];
	
	return attrsDictionary;
}


- (void)printText:(CONST char *)text atX:(int)x y:(int)y {
    messageText = [[NSString stringWithUTF8String:text] retain];
    messageLocation = NSMakePoint(x + 15, [self bounds].size.height - y);
    [self setNeedsDisplay:YES];
}

#pragma mark Buffers

- (int)allocBuffer1:(char **)b1 buffer2:(char **)b2 {
    currentBuffer = 0;
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
    NSLog(@"bytesPerRow = %d", [imageRep[0] bytesPerRow]);
    return [imageRep[0] bytesPerRow];
}

- (void)freeBuffers {
    [imageRep[0] release];
    [imageRep[1] release];
}

- (void)flipBuffers {
    currentBuffer ^= 1;
}

@end
