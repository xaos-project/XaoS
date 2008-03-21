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
	item = menu_findcommand([name cString]);
	
	ui_menuactivate(item, NULL);
}

- (void)clearMenu:(NSMenu *)menu {
	while ([menu numberOfItems] > 1) {
		[menu removeItemAtIndex:1];
	}
}

- (NSString *)keyEquivalentForName:(NSString *)name {
    // If you want more command-keys, just add them here based on their name:
	if ([name isEqualToString:@"Undo"]) return @"z";
	else if ([name isEqualToString:@"Redo"]) return @"Z";
	else if ([name isEqualToString:@"Load..."]) return @"o";
	else if ([name isEqualToString:@"Save..."]) return @"s";
	return @"";
}

- (void)buildMenuWithContext:(struct uih_context *)context name:(CONST char *)name {
	[self clearMenu:[NSApp mainMenu]];
	[self buildMenuWithContext:context name:name parent:[NSApp mainMenu]];
}

- (void)buildMenuWithContext:(struct uih_context *)context 
                        name:(CONST char *)menuName 
                      parent:(NSMenu *)parentMenu {

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
			menuTitle = [NSString stringWithCString:item->name];
			if (item->type == MENU_CUSTOMDIALOG || item->type == MENU_DIALOG)
				menuTitle = [menuTitle stringByAppendingString:@"..."];
			
			keyEquiv = [self keyEquivalentForName:menuTitle];
			
			if (item->key && parentMenu != mainMenu)
				menuTitle = [NSString stringWithFormat:@"%@ (%s)", menuTitle, item->key];

			menuShortName = [NSString stringWithCString:item->shortname];
			newItem = [[NSMenuItem allocWithZone:[NSMenu menuZone]] initWithTitle:menuTitle action:nil keyEquivalent:keyEquiv];
			
			[menuItems setValue:newItem forKey:menuShortName];
			if (item->type == MENU_SUBMENU) {
				newMenu = [[NSMenu allocWithZone:[NSMenu menuZone]] initWithTitle:menuTitle];
				[newItem setSubmenu:newMenu];
				[self buildMenuWithContext:context name:item->shortname parent:newMenu];
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
	CONST struct menuitem *xaosItem = menu_findcommand(name);
	NSMenuItem *menuItem = [menuItems objectForKey:[NSString stringWithCString:name]];
	
	[menuItem setState:(menu_enabled(xaosItem, context) ? NSOnState : NSOffState)];
	if (xaosItem->flags & MENUFLAG_RADIO) {
		NSEnumerator *itemEnumerator = [[[menuItem menu] itemArray] objectEnumerator];
		while (menuItem = [itemEnumerator nextObject]) {
			if ([menuItem representedObject]) {
				xaosItem = menu_findcommand([[menuItem representedObject] cString]);
				[menuItem setState:(menu_enabled(xaosItem, context) ? NSOnState : NSOffState)];
			}
		}
	}
}

- (void)showPopUpMenuWithContext:(struct uih_context *)context name:(CONST char *)name {
    // TODO Implement popup menus
}

#pragma mark Dialogs

- (void)showDialogWithContext:(struct uih_context *)context name:(CONST char *)name {
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
			param->dstring = strdup([fileName cString]);
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
	NSString *anchor = [NSString stringWithCString:name];
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

@end