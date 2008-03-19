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

#import "FractalView.h"
#include "ui.h"
@class PrefsController;
@class VideatorProxy;

@interface AppController : NSObject
{
    IBOutlet FractalView *view;
	IBOutlet NSWindow *window;
	NSMutableDictionary *menuItems;
	NSMutableDictionary *powerKeyDictionary;
	NSCursor *performanceCursor;
    VideatorProxy *videatorProxy;
    int keysDown;
    PrefsController *prefsController;
}

- (void)refreshDisplay;
- (void)printText:(CONST char *)text atX:(int)x y:(int)y;
- (void)flipBuffers;
- (int)allocBuffer1:(char **)b1 buffer2:(char **)b2;
- (void)freeBuffers;
- (void)getWidth:(int *)w height:(int *)h;
- (void)getMouseX:(int *)mx mouseY:(int *)my mouseButton:(int *)mb;
- (void)getMouseX:(int *)mx mouseY:(int *)my mouseButton:(int *)mb keys:(int *)k;
- (void)setMouseType:(int)type;
- (void)performMenuAction:(NSMenuItem *)sender;
- (void)buildMenuWithContext:(struct uih_context *)context name:(CONST char *)name;
- (void)buildMenuWithContext:(struct uih_context *)context name:(CONST char *)menuName parent:(NSMenu *)parentMenu;
- (void)toggleMenuWithContext:(struct uih_context *)context name:(CONST char *)name;
- (void)showDialogWithContext:(struct uih_context *)context name:(CONST char *)name;
- (void)showHelpWithContext:(struct uih_context *)context name:(CONST char *)name;
- (void)keyDown:(NSEvent *)e;
- (void)keyUp:(NSEvent *)e;
- (IBAction)showPreferencesPanel:(id)sender;
@end

extern AppController *controller;