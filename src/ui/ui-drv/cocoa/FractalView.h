//
//  FractalView.h
//  XaoS
//
//  Created by J.B. Langston III on 6/7/06.
//  Copyright 2006 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#include "ui.h"

@interface FractalView : NSView {
	int mouseX, mouseY, mouseButton;
	int currentBuffer;
	NSBitmapImageRep *imageRep[2];
}
- (int)allocBuffer1:(char **)b1 buffer2:(char **)b2;
- (void)freeBuffers;
- (void)getMouseX:(int *)mx mouseY:(int *)my mouseButton:(int *)mb;
- (void)flipBuffers;
- (NSBitmapImageRep *)imageRep;
@end
