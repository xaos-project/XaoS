//
//  CustomDialog.m
//  XaoS
//
//  Created by J.B. Langston III on 7/12/06.
//  Copyright 2006 __MyCompanyName__. All rights reserved.
//

#import "CustomDialog.h"

#define MARGIN 20
#define SPACING 8


@implementation CustomDialog
- (id)initWithContext:(struct uih_context *)myContext menuItem:(CONST menuitem *)myItem dialog:(CONST menudialog *)myDialog
{
	context = myContext;
	item = myItem;
	dialog = myDialog;	

	NSRect windowRect		= NSMakeRect(200, 200, 300, 107);
	NSRect helpButtonRect	= NSMakeRect(20, 20, 25, 25);
	NSRect cancelButtonRect	= NSMakeRect(128, 20, 75, 25);
	NSRect okButtonRect		= NSMakeRect(210, 20, 75, 25);
	NSRect labelRect		= NSMakeRect(20, 67, 62, 17);
	NSRect controlRect		= NSMakeRect(90, 65, 190, 22);
	NSRect coordRect		= NSMakeRect(90, 65, 85, 22);

	self = [super initWithContentRect:windowRect
							styleMask:NSTitledWindowMask
							  backing:NSBackingStoreBuffered
								defer:YES];
	
	if (self) {
		[self setTitle:[NSString stringWithCString:item->name]];

		controls = [[NSMutableDictionary alloc] initWithCapacity:10];
		
		NSMutableArray *labels = [[NSMutableArray alloc] initWithCapacity:10];
		
		int maxLabelWidth = 0, maxControlWidth = 0, nitems = 0, i = 0;

		for (nitems = 0; dialog[nitems].question; nitems++);
		
		for (i = 0; i < nitems; i++) {
			NSTextField *label = [[NSTextField alloc] initWithFrame:labelRect];
			NSString *question = [NSString stringWithCString:dialog[i].question];
			[label setEditable:NO];
			[label setBezeled:NO];
			[label setDrawsBackground:NO];
			[label setStringValue:question];

			[label sizeToFit];
			if ([label frame].size.width > maxLabelWidth)
				maxLabelWidth = [label frame].size.width;

			[[self contentView] addSubview:label];
			[labels addObject:label];
			[label release];
			
			switch (dialog[i].type) {
				case DIALOG_INT:
				case DIALOG_FLOAT:
				case DIALOG_STRING:
				case DIALOG_KEYSTRING:
				{
					NSTextField *textField = [[NSTextField alloc] initWithFrame:controlRect];
					[textField setEditable:YES];
					[textField setBezeled:YES];
					[textField setBezelStyle:NSTextFieldSquareBezel];
					switch (dialog[i].type) {
						case DIALOG_INT:
							[textField setStringValue:[NSString stringWithFormat:@"%d", dialog[i].defint]];
							break;
						case DIALOG_FLOAT:
							[textField setStringValue:[NSString stringWithFormat:@"%f", dialog[i].deffloat]];
							break;
						case DIALOG_STRING:
							[textField setStringValue:[NSString stringWithCString:dialog[i].defstr]];
							break;
						case DIALOG_KEYSTRING:
							[textField setStringValue:[NSString stringWithCString:dialog[i].defstr]];
							break;
					}
					
					//[textField sizeToFit];
					if ([textField frame].size.width > maxControlWidth)
						maxControlWidth = [textField frame].size.width;

					[[self contentView] addSubview:textField];
					[controls setValue:textField forKey:question];					
					[textField release];

					break;
				}
				case DIALOG_IFILE:
				case DIALOG_OFILE:
				{
					NSTextField *textField = [[NSTextField alloc] initWithFrame:controlRect];
					[textField setEditable:NO];
					[textField setBezeled:YES];
					[textField setBezelStyle:NSTextFieldSquareBezel];
					[textField setStringValue:[NSString stringWithCString:dialog[i].defstr]];
					
					//[textField sizeToFit];
					if ([textField frame].size.width > maxControlWidth)
						maxControlWidth = [textField frame].size.width;

					[[self contentView] addSubview:textField];
					[controls setValue:textField forKey:question];					
					
					NSButton *chooseButton = [[NSButton alloc] initWithFrame:okButtonRect];
					[chooseButton setTitle:@"Choose"];
					[chooseButton setButtonType:NSMomentaryPushInButton];
					[chooseButton setBezelStyle:NSRoundedBezelStyle];
					[chooseButton setTarget:self];
					if (dialog[i].type == DIALOG_IFILE)
						[chooseButton setAction:@selector(chooseInput:)];
					else
						[chooseButton setAction:@selector(chooseOutput:)];
					[[chooseButton cell] setRepresentedObject:textField];
					
					[[self contentView] addSubview:chooseButton];
					[controls setValue:chooseButton forKey:[question stringByAppendingString:@"choose"]];
					
					if ([textField frame].size.width + SPACING + [chooseButton frame].size.width > maxControlWidth)
						maxControlWidth = [textField frame].size.width + SPACING + [chooseButton frame].size.width;

					[chooseButton release];
					[textField release];
					break;
				}
				case DIALOG_CHOICE:
				{

					NSPopUpButton *popupButton = [[NSPopUpButton alloc] initWithFrame:controlRect];

					NSMenu *menu = [[NSMenu alloc] initWithTitle:@""];
					CONST char **str = (CONST char **) dialog[i].defstr;
					int y;
					for (y = 0; str[y] != NULL; y++)
					{
						NSMenuItem *menuItem = [[NSMenuItem alloc] initWithTitle:[NSString stringWithCString:str[y]] action:nil keyEquivalent:@""];
						[menu addItem:menuItem];
						[menuItem release];
					}
					[popupButton setMenu:menu];
					[menu release];
					[popupButton selectItemAtIndex:dialog[i].defint];

					[popupButton sizeToFit];
					if ([popupButton frame].size.width > maxControlWidth)
						maxControlWidth = [popupButton frame].size.width;

					[[self contentView] addSubview:popupButton];
					[controls setValue:popupButton forKey:question];					
					[popupButton release];
					
					break;
				}
				case DIALOG_ONOFF:
				{
					break;
				}
				case DIALOG_COORD:
				{
					int coordWidth = 0;
					NSTextField *textField = [[NSTextField alloc] initWithFrame:coordRect];
					[textField setEditable:YES];
					[textField setBezeled:YES];
					[textField setBezelStyle:NSTextFieldSquareBezel];
					[textField setStringValue:[NSString stringWithFormat:@"%f", dialog[i].deffloat]];

					//[textField sizeToFit];
					coordWidth += [textField frame].size.width;

					[[self contentView] addSubview:textField];
					[controls setValue:textField forKey:question];
					[textField release];

					textField = [[NSTextField alloc] initWithFrame:labelRect];
					[textField setEditable:NO];
					[textField setBezeled:NO];
					[textField setDrawsBackground:NO];
					[textField setStringValue:@"+"];

					[textField sizeToFit];
					coordWidth += [textField frame].size.width;
					
					[[self contentView] addSubview:textField];
					[controls setValue:textField forKey:[question stringByAppendingString:@"+"]];
					[textField release];
					
					textField = [[NSTextField alloc] initWithFrame:coordRect];
					[textField setEditable:YES];
					[textField setBezeled:YES];
					[textField setBezelStyle:NSTextFieldSquareBezel];
					[textField setStringValue:[NSString stringWithFormat:@"%f", dialog[i].deffloat2]];

					//[textField sizeToFit];
					coordWidth += [textField frame].size.width;

					[[self contentView] addSubview:textField];
					[controls setValue:textField forKey:[question stringByAppendingString:@"2"]];
					[textField release];

					textField = [[NSTextField alloc] initWithFrame:labelRect];
					[textField setEditable:NO];
					[textField setBezeled:NO];
					[textField setDrawsBackground:NO];
					[textField setStringValue:@"i"];

					[textField sizeToFit];
					coordWidth += [textField frame].size.width;
					
					[[self contentView] addSubview:textField];
					[controls setValue:textField forKey:[question stringByAppendingString:@"i"]];
					[textField release];
					
					if (coordWidth > maxControlWidth)
						maxControlWidth = coordWidth;
				}
			}
			if (MARGIN + maxLabelWidth + SPACING + maxControlWidth + MARGIN > windowRect.size.width)
				windowRect.size.width = MARGIN + maxLabelWidth + SPACING + maxControlWidth + MARGIN;
		}
		
		controlRect.origin.x = labelRect.origin.x + maxLabelWidth + SPACING;
		
		for (i = nitems-1; i >= 0; i--) {
			NSTextField *label = [labels objectAtIndex:i];
			NSControl *control = [controls objectForKey:[label stringValue]];
			[label setFrameOrigin:labelRect.origin];
			
			[control setFrameOrigin:controlRect.origin];
			
			switch (dialog[i].type) {
				case DIALOG_IFILE:
				case DIALOG_OFILE:
				{
					NSButton *chooseButton = [controls objectForKey:[[label stringValue] stringByAppendingString:@"choose"]];
					[chooseButton setFrameOrigin:NSMakePoint([control frame].origin.x + [control frame].size.width + SPACING, [control frame].origin.y - 2)];
					break;
				}
				case DIALOG_COORD:
				{
					NSRect controlRect2 = controlRect;
					controlRect2.origin.x += [control frame].size.width;
					control = [controls objectForKey:[[label stringValue] stringByAppendingString:@"+"]];
					[control setFrameOrigin:controlRect2.origin];
					controlRect2.origin.x += [control frame].size.width;
					control = [controls objectForKey:[[label stringValue] stringByAppendingString:@"2"]];
					[control setFrameOrigin:controlRect2.origin];
					controlRect2.origin.x += [control frame].size.width;
					control = [controls objectForKey:[[label stringValue] stringByAppendingString:@"i"]];
					[control setFrameOrigin:controlRect2.origin];
					control = [controls objectForKey:[label stringValue]];
				}
			}

			labelRect.origin.y += SPACING + [control frame].size.height;
			controlRect.origin.y += SPACING + [control frame].size.height;
		}
		
		controlRect.origin.y += MARGIN;

		windowRect.size.height = controlRect.origin.y;

		okButtonRect.origin.x += windowRect.size.width - [self frame].size.width;
		cancelButtonRect.origin.x += windowRect.size.width - [self frame].size.width;
		
		NSButton *okButton = [[NSButton alloc] initWithFrame:okButtonRect];
		[okButton setTitle:@"OK"];
		[okButton setButtonType:NSMomentaryPushInButton];
		[okButton setBezelStyle:NSRoundedBezelStyle];
		[okButton setTarget:self];
		[okButton setAction:@selector(execute:)];
		[[self contentView] addSubview:okButton];
		[okButton release];
		
		NSButton *cancelButton = [[NSButton alloc] initWithFrame:cancelButtonRect];
		[cancelButton setTitle:@"Cancel"];
		[cancelButton setButtonType:NSMomentaryPushInButton];
		[cancelButton setBezelStyle:NSRoundedBezelStyle];
		[cancelButton setTarget:self];
		[cancelButton setAction:@selector(cancel:)];
		[[self contentView] addSubview:cancelButton];
		[cancelButton release];
		
		NSButton *helpButton = [[NSButton alloc] initWithFrame:helpButtonRect];
		[helpButton setTitle:@""];
		[helpButton setButtonType:NSMomentaryPushInButton];
		[helpButton setBezelStyle:NSHelpButtonBezelStyle];
		[helpButton setTarget:self];
		[helpButton setAction:@selector(help:)];
		[[self contentView] addSubview:helpButton];
		[helpButton release];
		
		[self setContentSize:windowRect.size];
	}
	return self;
}

- (IBAction)execute:(id)sender
{
	int nitems;
	for (nitems = 0; dialog[nitems].question; nitems++);
	dialogparam *p = malloc (sizeof (*p) * nitems);

	int i;
	for (i = 0; i < nitems; i++)
    {
		NSString *question = [NSString stringWithCString:dialog[i].question];
		NSControl *control;
		switch (dialog[i].type)
		{
			case DIALOG_IFILE:
			case DIALOG_OFILE:
			case DIALOG_STRING:
			case DIALOG_KEYSTRING:
				control = [controls objectForKey:question];
				p[i].dstring = strdup ([[control stringValue] cString]);
				break;
			case DIALOG_INT:
				control = [controls objectForKey:question];
				p[i].dint = [control intValue];
				break;
			case DIALOG_FLOAT:
				control = [controls objectForKey:question];
				p[i].number = [control floatValue];
				break;
			case DIALOG_COORD:
				control = [controls objectForKey:question];
				p[i].dcoord[0] = [control floatValue];
				control = [controls objectForKey:[question stringByAppendingString:@"2"]];
				p[i].dcoord[1] = [control floatValue];
				break;
			case DIALOG_CHOICE:
				control = [controls objectForKey:question];
				p[i].dint = [control indexOfSelectedItem];
				break;
		}
    }
	ui_menuactivate (item, p);
	[NSApp stopModal];
}

- (IBAction)cancel:(id)sender
{
	[NSApp stopModal];
}

- (IBAction)help:(id)sender
{
	osx_showHelp(context, item->shortname);
}

- (IBAction)chooseInput:(id)sender
{
	
}

- (IBAction)chooseOutput:(id)sender
{
	
}

- (void)dealloc
{
	[controls release];
	[super dealloc];
}
@end
