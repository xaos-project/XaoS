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
#import "VideatorProxy.h"
#import "ui.h"

AppController *controller;


@implementation AppController

+ (void)setupDefaults
{
    NSString *userDefaultsValuesPath;
    NSDictionary *userDefaultsValuesDict;
    NSDictionary *initialValuesDict;
    NSArray *resettableUserDefaultsKeys;
    
    // load the default values for the user defaults
    userDefaultsValuesPath=[[NSBundle mainBundle] pathForResource:@"UserDefaults" 
														   ofType:@"plist"];
    userDefaultsValuesDict=[NSDictionary dictionaryWithContentsOfFile:userDefaultsValuesPath];
    
    // set them in the standard user defaults
    [[NSUserDefaults standardUserDefaults] registerDefaults:userDefaultsValuesDict];
    
    // if your application supports resetting a subset of the defaults to 
    // factory values, you should set those values 
    // in the shared user defaults controller
    resettableUserDefaultsKeys=[NSArray arrayWithObjects:@"EnableVideator",nil];
    initialValuesDict=[userDefaultsValuesDict dictionaryWithValuesForKeys:resettableUserDefaultsKeys];
    
    // Set the initial values in the shared user defaults controller 
    [[NSUserDefaultsController sharedUserDefaultsController] setInitialValues:initialValuesDict];
}


+ (void)initialize {
    [self setupDefaults];
}


- (void)refreshDisplay
{
	[view display]; //setNeedsDisplay:YES
    [videatorProxy sendImageRep:[view imageRep]];
}

- (void)awakeFromNib
{
	controller = self;
	menuItems = [[NSMutableDictionary alloc] init];
    videatorProxy = [[VideatorProxy alloc] init];
	
	// this is how we implement hotkeys in about 8 lines of code ;-)
	[[view window] makeFirstResponder:view];
	[[view window] setDelegate:self]; 
	//[self updateWindowTitle];
}

- (void)printText:(CONST char *)text atX:(int)x y:(int)y
{
    [view printText:text atX:x y:y];
}

- (void)flipBuffers
{
	[view flipBuffers];
}

- (int)allocBuffer1:(char **)b1 buffer2:(char **)b2 
{
	return [view allocBuffer1:b1 buffer2:b2];
}

- (void)freeBuffers
{
    [view freeBuffers];
}

- (void)getWidth:(int *)w height:(int *)h
{
	NSRect bounds = [view bounds];
	*w = bounds.size.width;
	*h = bounds.size.height;
}

- (void)getMouseX:(int *)mx mouseY:(int *)my mouseButton:(int *)mb
{
	[view getMouseX:mx mouseY:my mouseButton:mb];
}

- (void)getMouseX:(int *)mx mouseY:(int *)my mouseButton:(int *)mb keys:(int *)k
{
	[view getMouseX:mx mouseY:my mouseButton:mb];
	*k = keysDown;
}

- (void)setMouseType:(int)type
{
}

- (void)performMenuAction:(NSMenuItem *)sender
{
	CONST menuitem *item;	
	NSString *name;
	
	name = [sender representedObject];
	item = menu_findcommand([name cString]);
	
	ui_menuactivate(item, NULL);
	
	if ([name isEqualToString:@"inhibittextoutput"]) {
		if (!performanceCursor) performanceCursor = [[NSCursor alloc] initWithImage:[NSImage imageNamed:@"performanceCursor"] hotSpot:NSMakePoint(1.0,1.0)];
		[[view window]invalidateCursorRectsForView:view];
		NSCursor *cursor = [sender state] ? performanceCursor : [NSCursor arrowCursor];
		[view addCursorRect:[view bounds] cursor:cursor];
	}
}

- (void)clearMenu:(NSMenu *)menu {
	while ([menu numberOfItems] > 1) {
		[menu removeItemAtIndex:1];
	}
}


- (void)buildMenuWithContext:(struct uih_context *)context name:(CONST char *)name {
	[self clearMenu:[NSApp mainMenu]];
	[self buildMenuWithContext:context name:name parent:[NSApp mainMenu]];
}

- (NSString *)keyEquivalentForName:(NSString *)name {
    // If you want more command-keys, just add them here based on their name:
	if ([name isEqualToString:@"Undo"]) return @"z";
	else if ([name isEqualToString:@"Redo"]) return @"Z";
	else if ([name isEqualToString:@"Load..."]) return @"o";
	else if ([name isEqualToString:@"Save..."]) return @"s";
	return @"";
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

            /*
            // Disappearing Help menu in Leopard - probably due to the new search field dude
            if (NSAppKitVersionNumber > 830.0 && [menuTitle isEqualToString:@"Help"]) {
				menuTitle = @"Learn";
			}
            */
			
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
		CustomDialog *customDialog = [[CustomDialog alloc] initWithContext:context menuItem:item dialog:dialog];
		[NSApp beginSheet:customDialog modalForWindow:window modalDelegate:nil didEndSelector:nil contextInfo:nil];
		[NSApp runModalForWindow:customDialog];
		[NSApp endSheet:customDialog];
		[customDialog orderOut:self];
		[customDialog release];
	}
	
    [[view window] makeKeyAndOrderFront:self];
}

- (void)showHelpWithContext:(struct uih_context *)context name:(CONST char *)name {
	NSString *anchor = [NSString stringWithCString:name];
	
	// Display help frontpage instead of main XaoS page
	if ([anchor isEqualToString:@"main"])
		anchor = @"access";
	
	// Display requested help page
	[[NSHelpManager sharedHelpManager] openHelpAnchor:anchor inBook:@"XaoS Help"];
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
        case '\033': /* Escape */
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

- (void)windowWillClose:(NSNotification *)notification {
    [NSApp terminate:self];
}

- (void)windowDidResize:(NSNotification *)notification {
    /* Handle maximize restore, but ignore live resizing */
    if (![view inLiveResize])
        ui_resize();
}

@end