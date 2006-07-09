#import "AppController.h"

AppController *controller;

@implementation AppController

- (void)awakeFromNib
{
	controller = self;
	menuItems = [[NSMutableDictionary alloc] init];
}

- (void)refreshDisplay
{
	[view setNeedsDisplay:YES];
}

- (void)printText:(CONST char *)text atX:(int)x y:(int)y
{
}

- (void)flipBuffers
{
	[view flipBuffers];
}

- (int)allocBuffer1:(char **)b1 buffer2:(char **)b2 
{
	return [view allocBuffer1:b1 buffer2:b2];
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
	*k = 0;
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

}

- (void)buildMenuWithContext:(struct uih_context *)context name:(CONST char *)name
{
	[self clearMenu:[NSApp mainMenu]];
	[self buildMenuWithContext:context name:name parent:[NSApp mainMenu]];
}

- (void)clearMenu:(NSMenu *)menu
{
	while ([menu numberOfItems] > 1) {
		[menu removeItemAtIndex:1];
	}
}

- (void)buildMenuWithContext:(struct uih_context *)context name:(CONST char *)menuName parent:(NSMenu *)parentMenu
{
	int i;
	CONST menuitem *item;

    NSMenu *newMenu;
    NSMenuItem *newItem;
	NSString *menuTitle, *menuShortName;
	
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
	for (i=0; (item = menu_item(menuName, i)) != NULL; i++)
	{
		if (item->type == MENU_SEPARATOR) {
			[parentMenu addItem:[NSMenuItem separatorItem]];
		} else {
			menuTitle = [NSString stringWithCString:item->name];
			menuShortName = [NSString stringWithCString:item->shortname];
			newItem = [[NSMenuItem allocWithZone:[NSMenu menuZone]] initWithTitle:menuTitle action:nil keyEquivalent:@""];
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

	[pool release];
}

- (void)toggleMenuWithContext:(struct uih_context *)context name:(CONST char *)name
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

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
	[pool release];
}

@end