//
//  FractalView.m
//  XaoS
//
//  Created by J.B. Langston III on 6/7/06.
//  Copyright 2006 __MyCompanyName__. All rights reserved.
//

#import "FractalView.h"
@interface NSObject(AppDelegateStuff)
- (void)keyPressed:(NSString *)key;
@end

@implementation FractalView

- (id)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        // Initialization code here.
		mouseButton = mouseX = mouseY = currentBuffer = 0;
    }
    return self;
}

- (BOOL)isOpaque
{
	return YES;
}

- (void)mouseDown:(NSEvent *)theEvent
{
	if (0) printf("mouseDown");

	// Get location and translate coordinates to origin at upper left corner
	NSPoint mouseLoc = [self convertPoint:[theEvent locationInWindow] fromView:nil];
	NSRect bounds = [self bounds];
	mouseX = mouseLoc.x;
	mouseY = bounds.size.height - mouseLoc.y;
	
	// Select button based on modifier keys
	if ([theEvent modifierFlags] & NSControlKeyMask) {
		if (0) printf("+NSControlKeyMask");
		mouseButton |= BUTTON3;
	} else if ([theEvent modifierFlags] & NSShiftKeyMask) {
		if (0) printf("+NSShiftKeyMask");
		mouseButton |= BUTTON2;
	} else {
		mouseButton |= BUTTON1;
	}
	if (0) printf("\n");
}

- (void)mouseUp:(NSEvent *)theEvent
{
	if (0) printf("mouseUp\n");
    mouseButton = 0;
}

- (void)mouseDragged:(NSEvent *)theEvent
{
	if (0) printf("mouseDragged\n");

	// Get location and translate coordinates to origin at upper left corner
	NSPoint mouseLoc = [self convertPoint:[theEvent locationInWindow] fromView:nil];
	NSRect bounds = [self bounds];
	mouseX = mouseLoc.x;
	mouseY = bounds.size.height - mouseLoc.y;
	
}

- (void)drawRect:(NSRect)rect {
	if (imageRep[currentBuffer]) {
        // Drawing code here.
        [imageRep[currentBuffer] drawInRect:[self bounds]];
	}
}

- (NSBitmapImageRep *)imageRep {
	return imageRep[currentBuffer];
}

- (int)allocBuffer1:(char **)b1 buffer2:(char **)b2 
{
    // Initialize image rep to current size of image view
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

- (void)getMouseX:(int *)mx mouseY:(int *)my mouseButton:(int *)mb 
{
	*mx = mouseX;
	*my = mouseY;
	*mb = mouseButton;
}

- (void)flipBuffers
{
	currentBuffer ^= 1;
}

// ACS: need to implement hot keys!

- (void)keyDown:(NSEvent *)e {
	NSString *characters = [e characters];
	if ([characters length] > 0) [[[self window] delegate] keyPressed:[NSString stringWithFormat:@"%C",[characters characterAtIndex:0]]];
}

@end
