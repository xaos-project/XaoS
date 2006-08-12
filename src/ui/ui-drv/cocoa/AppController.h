/* AppController */

#import <Cocoa/Cocoa.h>

#import "FractalView.h"
#include "ui.h"

@interface AppController : NSObject
{
    IBOutlet FractalView *view;
	IBOutlet NSWindow *window;
	NSMutableDictionary *menuItems;
}

- (void)mainThread;
- (void)refreshDisplay;
- (void)printText:(CONST char *)text atX:(int)x y:(int)y;
- (void)flipBuffers;
- (int)allocBuffer1:(char **)b1 buffer2:(char **)b2;
- (void)getWidth:(int *)w height:(int *)h;
- (void)getMouseX:(int *)mx mouseY:(int *)my mouseButton:(int *)mb;
- (void)getMouseX:(int *)mx mouseY:(int *)my mouseButton:(int *)mb keys:(int *)k;
- (void)setMouseType:(int)type;
- (void)performMenuAction:(NSMenuItem *)sender;
- (void)buildMenuWithContext:(struct uih_context *)context name:(CONST char *)name;
- (void)buildMenuWithContext:(struct uih_context *)context name:(CONST char *)menuName parent:(NSMenu *)parentMenu;
- (void)toggleMenuWithContext:(struct uih_context *)context name:(CONST char *)name;

@end

extern AppController *controller;