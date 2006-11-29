//
//  PrefsController.h
//  XaoS
//
//  Created by J.B. Langston III on 11/11/06.
//  Copyright 2006 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface PrefsController : NSWindowController {
    NSToolbar               * fToolbar;
    IBOutlet NSView         * fGeneralView, * fVideatorView;
}
- (IBAction)goToStoneWebSite:(id)sender;
- (IBAction)downloadVideator:(id)sender;
- (IBAction)goToVideatorWebSite:(id)sender;
- (IBAction)goToVideatorVJ:(id)sender;
- (IBAction)updateWindowTitleAction:(id)sender;
@end
