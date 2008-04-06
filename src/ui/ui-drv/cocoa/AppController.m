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
#import "AppController.h"
#import "CustomDialog.h"
#import "ui.h"

#ifdef HAVE_GETTEXT
#include <libintl.h>
#include <locale.h>
#define _(string) gettext(string)
#else
#define _(string) (string)
#endif

AppController *controller;

@implementation AppController


#pragma mark Defaults

+ (void)setupDefaults
{
    NSString *userDefaultsValuesPath;
    NSDictionary *userDefaultsValuesDict;
    NSDictionary *initialValuesDict;
    NSArray *resettableUserDefaultsKeys;
    
    userDefaultsValuesPath=[[NSBundle mainBundle] pathForResource:@"UserDefaults" 
														   ofType:@"plist"];
    userDefaultsValuesDict=[NSDictionary dictionaryWithContentsOfFile:userDefaultsValuesPath];
    
    [[NSUserDefaults standardUserDefaults] registerDefaults:userDefaultsValuesDict];
    
    resettableUserDefaultsKeys=[NSArray arrayWithObjects:@"EnableVideator",nil];
    initialValuesDict=[userDefaultsValuesDict dictionaryWithValuesForKeys:resettableUserDefaultsKeys];
    
    [[NSUserDefaultsController sharedUserDefaultsController] setInitialValues:initialValuesDict];
}

#pragma mark Initialization

+ (void)initialize {
    [self setupDefaults];
}

- (void)awakeFromNib {
	controller = self;
	menuItems = [[NSMutableDictionary alloc] init];

	[[view window] makeFirstResponder:view];
	[[view window] setDelegate:self]; 
}

#pragma mark Accessors

- (FractalView *)view {
    return view;
}

#pragma mark Driver Initialization

- (int)initDriver:(struct ui_driver)driver {
    // TODO Implement driver initialization
    return 1; // 1 for success; 0 for failure
}

- (void)uninitDriver:(struct ui_driver)driver {
    // TODO Implement driver uninitialization
}

#pragma mark Menus

- (void)performMenuAction:(NSMenuItem *)sender {
	CONST menuitem *item;	
	NSString *name;
	
	name = [sender representedObject];
	item = menu_findcommand([name UTF8String]);
	
	ui_menuactivate(item, NULL);
}

- (void)clearMenu:(NSMenu *)menu {
	while ([menu numberOfItems] > 1) {
		[menu removeItemAtIndex:1];
	}
}

- (NSString *)keyEquivalentForName:(NSString *)name {
    // If you want more command-keys, just add them here based on their name:
	if ([name isEqualToString:@"undo"]) return @"z";
	if ([name isEqualToString:@"redo"]) return @"Z";
	if ([name isEqualToString:@"loadpos"]) return @"o";
	if ([name isEqualToString:@"savepos"]) return @"s";
	return @"";
}

- (void)buildMenuWithContext:(struct uih_context *)context name:(CONST char *)name {
    /* Cache a reference to the last UI context we received */
    lastContext = context;
    
	[self clearMenu:[NSApp mainMenu]];
	[self buildMenuWithContext:context name:name parent:[NSApp mainMenu]];
}

- (void)buildMenuWithContext:(struct uih_context *)context 
                        name:(CONST char *)menuName 
                      parent:(NSMenu *)parentMenu {
    /* Cache a reference to the last UI context we received */
    lastContext = context;

	int i;
	CONST menuitem *item;
	
    NSMenu *newMenu;
    NSMenuItem *newItem;
	NSString *menuTitle, *menuShortName;
	
	NSMenu *mainMenu = [NSApp mainMenu];
	
	for (i=0; (item = menu_item(menuName, i)) != NULL; i++)	{
		if (item->type == MENU_SEPARATOR) {
			[parentMenu addItem:[NSMenuItem separatorItem]];
		} else {
			NSString *keyEquiv = @"";
			menuTitle = [NSString stringWithUTF8String:item->name];
			if (item->type == MENU_CUSTOMDIALOG || item->type == MENU_DIALOG)
				menuTitle = [menuTitle stringByAppendingString:@"..."];
			
			menuShortName = [NSString stringWithUTF8String:item->shortname];

			keyEquiv = [self keyEquivalentForName:menuShortName];
			
			if (item->key && parentMenu != mainMenu)
				menuTitle = [NSString stringWithFormat:@"%@ (%s)", menuTitle, item->key];

			newItem = [[NSMenuItem allocWithZone:[NSMenu menuZone]] initWithTitle:menuTitle action:nil keyEquivalent:keyEquiv];
			
			[menuItems setValue:newItem forKey:menuShortName];
			if (item->type == MENU_SUBMENU) {
				newMenu = [[NSMenu allocWithZone:[NSMenu menuZone]] initWithTitle:menuTitle];
				[newItem setSubmenu:newMenu];
				[self buildMenuWithContext:context name:item->shortname parent:newMenu];
                
                /* These items are necessary to implement cut & paste in custom dialogs */
                if ([menuShortName isEqualToString:@"edit"]) {
                    [newMenu addItem:[NSMenuItem separatorItem]];
                    [newMenu addItemWithTitle:[NSString stringWithUTF8String:_("Cut")]
                                       action:@selector(cut:) keyEquivalent:@"x"];

                    [newMenu addItemWithTitle:[NSString stringWithUTF8String:_("Copy")]
                                       action:@selector(copy:) keyEquivalent:@"c"];
                    
                    [newMenu addItemWithTitle:[NSString stringWithUTF8String:_("Paste")]
                                       action:@selector(paste:) keyEquivalent:@"v"];
                    
                    [newMenu addItemWithTitle:[NSString stringWithUTF8String:_("Delete")]
                                       action:@selector(delete:) keyEquivalent:@""];
                    
                    [newMenu addItemWithTitle:[NSString stringWithUTF8String:_("Select All")]
                                       action:@selector(selectAll:) keyEquivalent:@"a"];
                }
                
                if ([menuShortName isEqualToString:@"window"]) {
                    [newMenu addItemWithTitle:[NSString stringWithUTF8String:_("Minimize")]
                                       action:@selector(performMiniaturize:) keyEquivalent:@"m"];

                    [newMenu addItemWithTitle:[NSString stringWithUTF8String:_("Zoom")]
                                       action:@selector(performZoom:) keyEquivalent:@""];

                    [newMenu addItem:[NSMenuItem separatorItem]];

                    [newMenu addItemWithTitle:[NSString stringWithUTF8String:_("Bring All to Front")]
                                       action:@selector(arrangeInFront:) keyEquivalent:@""];
                }
                
				[newMenu release];
			} else {				
				[newItem setTarget:self];
				[newItem setAction:@selector(performMenuAction:)];
				[newItem setRepresentedObject:menuShortName];
				if (item->flags & (MENUFLAG_RADIO | MENUFLAG_CHECKBOX) && menu_enabled (item, context))
					[newItem setState:NSOnState];
			}
			[parentMenu addItem:newItem];
			[newItem release];
		}
	}
}

- (void)toggleMenuWithContext:(struct uih_context *)context name:(CONST char *)name {
    /* Cache a reference to the last UI context we received */
    lastContext = context;

	CONST struct menuitem *xaosItem = menu_findcommand(name);
	NSMenuItem *menuItem = [menuItems objectForKey:[NSString stringWithUTF8String:name]];
	
	[menuItem setState:(menu_enabled(xaosItem, context) ? NSOnState : NSOffState)];
	if (xaosItem->flags & MENUFLAG_RADIO) {
		NSEnumerator *itemEnumerator = [[[menuItem menu] itemArray] objectEnumerator];
		while (menuItem = [itemEnumerator nextObject]) {
			if ([menuItem representedObject]) {
				xaosItem = menu_findcommand([[menuItem representedObject] UTF8String]);
				[menuItem setState:(menu_enabled(xaosItem, context) ? NSOnState : NSOffState)];
			}
		}
	}
}

- (void)showPopUpMenuWithContext:(struct uih_context *)context name:(CONST char *)name {
    /* Cache a reference to the last UI context we received */
    lastContext = context;

    // TODO Implement popup menus
}

#pragma mark Dialogs

- (void)showDialogWithContext:(struct uih_context *)context name:(CONST char *)name {
    /* Cache a reference to the last UI context we received */
    lastContext = context;

	CONST menuitem *item = menu_findcommand (name);
	if (!item) return;
	
	CONST menudialog *dialog = menu_getdialog (context, item);
	if (!dialog) return;
	
	int nitems;
	for (nitems = 0; dialog[nitems].question; nitems++);
	
	if (nitems == 1 && (dialog[0].type == DIALOG_IFILE || dialog[0].type == DIALOG_OFILE)) {
		NSString *fileName = nil;
		
		switch(dialog[0].type) {
			case DIALOG_IFILE:
			{
				NSArray *fileTypes = nil;
				if (strcmp(name, "loadpos") == 0)			
					fileTypes = [NSArray arrayWithObject:@"xpf"];
				else if (strcmp(name, "play") == 0)
					fileTypes = [NSArray arrayWithObject:@"xaf"];
				
				NSOpenPanel *oPanel = [NSOpenPanel openPanel];
				
				int result = [oPanel runModalForDirectory:nil
													 file:nil types:fileTypes];
				
				if (result == NSOKButton)
					fileName = [oPanel filename];
				break;
			}
			case DIALOG_OFILE:
			{
				NSSavePanel *sPanel = [NSSavePanel savePanel];
				if (strcmp(name, "savepos") == 0)			
					[sPanel setRequiredFileType:@"xpf"];
				else if (strcmp(name, "record") == 0)
					[sPanel setRequiredFileType:@"xaf"];
				else if (strcmp(name, "saveimg") == 0)
					[sPanel setRequiredFileType:@"png"];
				
				int result = [sPanel runModalForDirectory:nil file:@"untitled"];
				
				if (result == NSOKButton)
					fileName = [sPanel filename];
				break;
			}
		}
		
		if (fileName) {
			dialogparam *param = malloc (sizeof (dialogparam));
			param->dstring = strdup([fileName UTF8String]);
			ui_menuactivate (item, param);
		}
		
	} else {
		CustomDialog *customDialog = [[CustomDialog alloc] initWithContext:context 
                                                                  menuItem:item 
                                                                    dialog:dialog];
		[NSApp beginSheet:customDialog 
           modalForWindow:window 
            modalDelegate:nil 
           didEndSelector:nil 
              contextInfo:nil];
		[NSApp runModalForWindow:customDialog];
		[NSApp endSheet:customDialog];
		[customDialog orderOut:self];
		[customDialog release];
	}
	
    [[view window] makeKeyAndOrderFront:self];
}

#pragma mark Help

- (void)showHelpWithContext:(struct uih_context *)context name:(CONST char *)name {
    /* Cache a reference to the last UI context we received */
    lastContext = context;

	NSString *anchor = [NSString stringWithUTF8String:name];
	[[NSHelpManager sharedHelpManager] openHelpAnchor:anchor inBook:@"XaoS Help"];
}

#pragma mark Window Delegates

- (void)windowWillClose:(NSNotification *)notification {
    [NSApp terminate:self];
}

- (void)windowDidResize:(NSNotification *)notification {
    // Handle maximize/zoom, but ignore live resizing
    if (![view inLiveResize])
        ui_resize();
}

#pragma mark Application Delegates

- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename {
    /* Don't try to open the file unless a valid context has been cached */
    if (!lastContext) return NO;

    if ([[filename pathExtension] isEqualToString:@"xpf"]) {
        uih_loadfile(lastContext, [filename UTF8String]);
        return YES;
    } else if ([[filename pathExtension] isEqualToString:@"xaf"]) {
        uih_playfile(lastContext, [filename UTF8String]);
        return YES;
    } else {
        return NO;
    }
}

@end