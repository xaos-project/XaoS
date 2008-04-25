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
#import "CustomDialog.h"

#ifdef HAVE_GETTEXT
#include <libintl.h>
#include <locale.h>
#define _(string) gettext(string)
#else
#define _(string) (string)
#endif

#define MARGIN 20
#define SPACING 8

@implementation CustomDialog

#pragma mark Initialization

- (id)initWithContext:(struct uih_context *)myContext 
             menuItem:(CONST menuitem *)myItem 
               dialog:(CONST menudialog *)myDialog {
    
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
        context = myContext;
        item = myItem;
        dialog = myDialog;	
        
        [self setTitle:[NSString stringWithUTF8String:item->name]];
        
        controls = [[NSMutableDictionary alloc] initWithCapacity:10];
        
        NSMutableArray *labels = [[NSMutableArray alloc] initWithCapacity:10];
        
        int maxLabelWidth = 0, maxControlWidth = 0, nitems = 0, i = 0;
        
        for (nitems = 0; dialog[nitems].question; nitems++);
        
        for (i = 0; i < nitems; i++) {
            NSTextField *label = [[NSTextField alloc] initWithFrame:labelRect];
            NSString *question = [NSString stringWithUTF8String:dialog[i].question];
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
                  [[textField cell] setScrollable:YES];
                  switch (dialog[i].type) {
                    case DIALOG_INT:
                        [textField setIntValue:dialog[i].defint];
                        break;
                    case DIALOG_FLOAT:
                        [textField setDoubleValue:dialog[i].deffloat];
                        break;
                    case DIALOG_STRING:
                        [textField setStringValue:[NSString stringWithUTF8String:dialog[i].defstr]];
                        break;
                    case DIALOG_KEYSTRING:
                        [textField setStringValue:[NSString stringWithUTF8String:dialog[i].defstr]];
                        break;
                  }
                  
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
                  [[textField cell] setScrollable:YES];
                  [textField setStringValue:[NSString stringWithUTF8String:dialog[i].defstr]];
                  [[textField cell] setRepresentedObject:[NSString stringWithUTF8String:dialog[i].defstr]];
                  
                  if ([textField frame].size.width > maxControlWidth)
                      maxControlWidth = [textField frame].size.width;
                  
                  [[self contentView] addSubview:textField];
                  [controls setValue:textField forKey:question];					
                  
                  NSButton *chooseButton = [[NSButton alloc] initWithFrame:okButtonRect];
                  [chooseButton setTitle:[NSString stringWithUTF8String:_("Choose")]];
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
                        NSMenuItem *menuItem = [[NSMenuItem alloc] initWithTitle:[NSString stringWithUTF8String:str[y]] action:nil keyEquivalent:@""];
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
                  [[textField cell] setScrollable:YES];
                  [textField setDoubleValue:dialog[i].deffloat];
                  
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
                  [[textField cell] setScrollable:YES];
                  [textField setDoubleValue:dialog[i].deffloat2];
                  
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
        [okButton setTitle:[NSString stringWithUTF8String:_("OK")]];
        [okButton setButtonType:NSMomentaryPushInButton];
        [okButton setBezelStyle:NSRoundedBezelStyle];
        [okButton setKeyEquivalent:@"\r"];
        [okButton setTarget:self];
        [okButton setAction:@selector(execute:)];
        [okButton sizeToFit];
        [[self contentView] addSubview:okButton];
        
        NSButton *cancelButton = [[NSButton alloc] initWithFrame:cancelButtonRect];
        [cancelButton setTitle:[NSString stringWithUTF8String:_("Cancel")]];
        [cancelButton setButtonType:NSMomentaryPushInButton];
        [cancelButton setBezelStyle:NSRoundedBezelStyle];
        [cancelButton setTarget:self];
        [cancelButton setAction:@selector(cancel:)];
        [cancelButton sizeToFit];
        [[self contentView] addSubview:cancelButton];
        
        NSButton *helpButton = [[NSButton alloc] initWithFrame:helpButtonRect];
        [helpButton setTitle:@""];
        [helpButton setButtonType:NSMomentaryPushInButton];
        [helpButton setBezelStyle:NSHelpButtonBezelStyle];
        [helpButton setTarget:self];
        [helpButton setAction:@selector(help:)];
        [[self contentView] addSubview:helpButton];
        [helpButton release];
        
        okButtonRect.size.width = MAX([okButton frame].size.width + 2, [cancelButton frame].size.width) + 2 * SPACING;
        cancelButtonRect.size.width = okButtonRect.size.width;
        
        okButtonRect.origin.x = windowRect.size.width - MARGIN - okButtonRect.size.width;
        cancelButtonRect.origin.x = okButtonRect.origin.x - SPACING - cancelButtonRect.size.width;
        
        [okButton setFrame:okButtonRect];
        [cancelButton setFrame:cancelButtonRect];
        
        [cancelButton release];
        [okButton release];
        
        [self setContentSize:windowRect.size];
    }
    return self;
}

# pragma mark Targets

- (IBAction)execute:(id)sender {
    int nitems;
    for (nitems = 0; dialog[nitems].question; nitems++);
    params = malloc (sizeof (*params) * nitems);
    
    int i;
    for (i = 0; i < nitems; i++) {
        NSString *question = [NSString stringWithUTF8String:dialog[i].question];
        NSControl *control;
        switch (dialog[i].type) {
          case DIALOG_IFILE:
          case DIALOG_OFILE:
          case DIALOG_STRING:
          case DIALOG_KEYSTRING:
              control = [controls objectForKey:question];
              params[i].dstring = strdup ([[control stringValue] UTF8String]);
              break;
          case DIALOG_INT:
              control = [controls objectForKey:question];
              params[i].dint = [control intValue];
              break;
          case DIALOG_FLOAT:
              control = [controls objectForKey:question];
              params[i].number = [control floatValue];
              break;
          case DIALOG_COORD:
              control = [controls objectForKey:question];
              params[i].dcoord[0] = [control floatValue];
              control = [controls objectForKey:[question stringByAppendingString:@"2"]];
              params[i].dcoord[1] = [control floatValue];
              break;
          case DIALOG_CHOICE:
              control = [controls objectForKey:question];
              params[i].dint = [(NSPopUpButtonCell *)control indexOfSelectedItem];
              break;
        }
    }
    [NSApp stopModal];
}

- (IBAction)cancel:(id)sender {
    params = NULL;
    [NSApp stopModal];
}

- (IBAction)help:(id)sender {
    ui_help(item->shortname);
}

- (IBAction)chooseInput:(id)sender {
    NSTextField *target = [[sender cell] representedObject];
    NSString *extension = [[[target cell] representedObject] pathExtension];
    NSOpenPanel *oPanel = [NSOpenPanel openPanel];
    
    int result = [oPanel runModalForDirectory:nil 
                                         file:nil 
                                        types:[NSArray arrayWithObject:extension]];
    
    if (result == NSOKButton)
        [target setStringValue:[oPanel filename]];
}

- (IBAction)chooseOutput:(id)sender {
    NSTextField *target = [[sender cell] representedObject];
    NSString *extension = [[[target cell] representedObject] pathExtension];
    NSSavePanel *sPanel = [NSSavePanel savePanel];
    [sPanel setRequiredFileType:extension];
    
    int result = [sPanel runModalForDirectory:nil file:[target stringValue]];
    if (result == NSOKButton)
        [target setStringValue:[sPanel filename]];
}

#pragma mark Accessors

- (dialogparam *)params {
    return params;
}

#pragma mark Deallocation

- (void)dealloc {
    [controls release];
    [super dealloc];
}
@end
