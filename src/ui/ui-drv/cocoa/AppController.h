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
@class VideatorProxy;

@interface AppController : NSObject
{
    IBOutlet FractalView *view;
	IBOutlet NSWindow *window;
	NSMutableDictionary *menuItems;
	NSMutableDictionary *powerKeyDictionary;
    struct uih_context *lastContext;
}

#pragma mark Accessors
- (FractalView *)view;

#pragma mark Driver Initialization
- (int)initLocale;
- (int)initDriver:(struct ui_driver)driver;
- (void)uninitDriver:(struct ui_driver)driver;

#pragma mark Menus
- (void)localizeApplicationMenu;
- (void)performMenuAction:(NSMenuItem *)sender;
- (void)clearMenu:(NSMenu *)menu;
- (NSString *)keyEquivalentForName:(NSString *)name;
- (void)buildMenuWithContext:(struct uih_context *)context name:(CONST char *)name;
- (void)buildMenuWithContext:(struct uih_context *)context name:(CONST char *)menuName parent:(NSMenu *)parentMenu;
- (void)toggleMenuWithContext:(struct uih_context *)context name:(CONST char *)name;
- (void)showPopUpMenuWithContext:(struct uih_context *)context name:(CONST char *)name;

#pragma mark Dialogs
- (void)showDialogWithContext:(struct uih_context *)context name:(CONST char *)name;

#pragma mark Help
- (void)showHelpWithContext:(struct uih_context *)context name:(CONST char *)name;
@end

extern AppController *controller;