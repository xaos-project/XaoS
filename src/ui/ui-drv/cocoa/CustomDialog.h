//
//  CustomDialog.h
//  XaoS
//
//  Created by J.B. Langston III on 7/12/06.
//  Copyright 2006 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include "ui.h"

@interface CustomDialog : NSWindow {
	struct uih_context *context; 
	CONST menuitem *item;
	CONST menudialog *dialog;
	NSMutableDictionary *controls;
}

- (id)initWithContext:(struct uih_context *)context menuItem:(CONST menuitem *)item dialog:(CONST menudialog *)dialog;

@end
