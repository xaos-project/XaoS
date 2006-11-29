//
//  PrefsController.m
//  XaoS
//
//  Created by J.B. Langston III on 11/11/06.
//  Copyright 2006 __MyCompanyName__. All rights reserved.
//

#import "PrefsController.h"

#define TOOLBAR_GENERAL     @"General"
#ifdef VIDEATOR_SUPPORT
#define TOOLBAR_VIDEATOR    @"Videator"
#endif

@interface PrefsController (Private)

- (void) showGeneralPref: (id) sender;
#ifdef VIDEATOR_SUPPORT
- (void) showVideatorPref: (id) sender;
#endif

- (void) setPrefView: (NSView *) view;

@end

@implementation PrefsController

- (id)init
{
    self = [super initWithWindowNibName:@"PrefsWindow"];

    fToolbar = [[NSToolbar alloc] initWithIdentifier: @"Preferences Toolbar"];
    [fToolbar setDelegate: self];
    [fToolbar setAllowsUserCustomization: NO];
    [[self window] setToolbar: fToolbar];
    [fToolbar setDisplayMode: NSToolbarDisplayModeIconAndLabel];
    [fToolbar setSizeMode: NSToolbarSizeModeRegular];

    [fToolbar setSelectedItemIdentifier:TOOLBAR_GENERAL];
    [self showGeneralPref: nil];

    return self;
}

- (NSToolbarItem *) toolbar: (NSToolbar *) t itemForItemIdentifier:
    (NSString *) ident willBeInsertedIntoToolbar: (BOOL) flag
{
    NSToolbarItem * item;
    item = [[NSToolbarItem alloc] initWithItemIdentifier: ident];

    if ([ident isEqualToString: TOOLBAR_GENERAL])
    {
        [item setLabel: TOOLBAR_GENERAL];
        [item setImage: [NSImage imageNamed: @"GeneralPreferences.tiff"]];
        [item setTarget: self];
        [item setAction: @selector( showGeneralPref: )];
    }
#ifdef VIDEATOR_SUPPORT
    else if ([ident isEqualToString: TOOLBAR_VIDEATOR])
    {
        [item setLabel: TOOLBAR_VIDEATOR];
        [item setImage: [NSImage imageNamed: @"vidiot.icns"]];
        [item setTarget: self];
        [item setAction: @selector( showVideatorPref: )];
    }
#endif
    else
    {
        [item release];
        return nil;
    }

    return item;
}

- (NSArray *) toolbarSelectableItemIdentifiers: (NSToolbar *) toolbar
{
    return [self toolbarDefaultItemIdentifiers: nil];
}

- (NSArray *) toolbarDefaultItemIdentifiers: (NSToolbar *) toolbar
{
    return [self toolbarAllowedItemIdentifiers: nil];
}

- (NSArray *) toolbarAllowedItemIdentifiers: (NSToolbar *) toolbar
{
    return [NSArray arrayWithObjects:
            TOOLBAR_GENERAL, 
#ifdef VIDEATOR_SUPPORT
            TOOLBAR_VIDEATOR, 
#endif
            nil];
}

- (void)launchURL:(NSString *)url {
	[[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:url]];
}

- (IBAction)goToStoneWebSite:(id)sender {
    [self launchURL:@"http://www.stone.com"];
}

#ifdef VIDEATOR_SUPPORT
- (IBAction)goToVideatorWebSite:(id)sender {
    [self launchURL:@"http://www.stone.com/Videator"];
}
- (IBAction)goToVideatorVJ:(id)sender {
    [self launchURL:@"http://www.stone.com/Videator/VJ.html"];
}

- (IBAction)downloadVideator:(id)sender {
    [self launchURL:@"ftp://ftp.swcp.com/pub/tmp/a/Videator.dmg.gz"];
}

- (IBAction)updateWindowTitleAction:(id)sender {
	[[NSApp delegate] updateWindowTitle];
}
#endif

@end

@implementation PrefsController (Private)
- (void) showGeneralPref: (id) sender
{
    [self setPrefView: fGeneralView];
}

#ifdef VIDEATOR_SUPPORT
- (void) showVideatorPref: (id) sender
{
    [self setPrefView: fVideatorView];
}
#endif

- (void) setPrefView: (NSView *) view
{
    NSWindow * window = [self window];
    
    NSRect windowRect = [window frame];
    int difference = [view frame].size.height - [[window contentView] frame].size.height;
    windowRect.origin.y -= difference;
    windowRect.size.height += difference;

    [window setTitle: [fToolbar selectedItemIdentifier]];
    
    [window setContentView: view];
    [view setHidden: YES];
    [window setFrame: windowRect display: YES animate: YES];
    [view setHidden: NO];
}
@end