/*
 *     XaoS, a fast portable realtime fractal zoomer 
 *                  Copyright ¬© 1996,1997 by
 *
 *      Jan Hubicka          (hubicka@paru.cas.cz)
 *      Thomas Marsh         (tmarsh@austin.ibm.com)
 *	
 *	Mac OS X Driver by J.B. Langston (jb-langston at austin dot rr dot com)
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

#include "osxcommon.h"
#include "ui.h"

static int primaryButtonFunction = BUTTON1;
static int primaryButtonPressed = FALSE;

static EventHandlerRef keyEventHandlerRef;
static EventHandlerRef mouseEventHandlerRef;
static EventHandlerRef commandEventHandlerRef;
static EventHandlerRef windowEventHandlerRef;


static const EventTypeSpec kKeyEvents[] = {
{ kEventClassKeyboard, kEventRawKeyDown },
{ kEventClassKeyboard, kEventRawKeyRepeat },
{ kEventClassKeyboard, kEventRawKeyUp },
{ kEventClassKeyboard, kEventRawKeyModifiersChanged }
};

static OSStatus KeyEventHandler( EventHandlerCallRef handlerCallRef,
                                 EventRef event,
                                 void *userData )
{
    switch ( GetEventKind( event ) ) {
        case kEventRawKeyDown:
		case kEventRawKeyRepeat:
		{
			char keyChar;
			
			if( GetEventParameter( event,
								   kEventParamKeyMacCharCodes,
								   typeChar,
								   NULL,
								   sizeof(keyChar),
								   NULL,
								   &keyChar) != noErr )
				return eventNotHandledErr;
			
			switch(keyChar) {
				case kLeftArrowCharCode:
					osx_keys |= 1;
					ui_key(UIKEY_LEFT);
					break;
				case kRightArrowCharCode:
					osx_keys |= 2;
					ui_key(UIKEY_RIGHT);
					break;
				case kUpArrowCharCode:
					osx_keys |= 4;
					ui_key(UIKEY_UP);
					break;
				case kDownArrowCharCode:
					osx_keys |= 8;
					ui_key(UIKEY_DOWN);
					break;
				case kBackspaceCharCode:
					ui_key(UIKEY_BACKSPACE);
					break;
				case kEndCharCode:
					ui_key(UIKEY_END);
					break;
				case kEscapeCharCode:
					ui_key(UIKEY_ESC);
					break;
				case kHomeCharCode:
					ui_key(UIKEY_HOME);
					break;
				case kPageDownCharCode:
					ui_key(UIKEY_PGDOWN);
					break;
				case kPageUpCharCode:
					ui_key(UIKEY_PGUP);
					break;
				case kTabCharCode:
					ui_key(UIKEY_TAB);
					break;
				default:
					ui_key(keyChar);
			}
			
			return noErr;
            break;
		}
        case kEventRawKeyUp:
		{
			char keyChar;
			
			if( GetEventParameter( event,
								   kEventParamKeyMacCharCodes,
								   typeChar,
								   NULL,
								   sizeof(keyChar),
								   NULL,
								   &keyChar) != noErr )
				return eventNotHandledErr;
			
			switch(keyChar)	{
				case kLeftArrowCharCode:
					osx_keys &= ~1;
					break;
				case kRightArrowCharCode:
					osx_keys &= ~2;
					break;
				case kUpArrowCharCode:
					osx_keys &= ~4;
					break;
				case kDownArrowCharCode:
					osx_keys &= ~8;
					break;
			}
			
			return noErr;
            break;
		}
        case kEventRawKeyModifiersChanged:
		{
			UInt32 modifiers;
			
			if ( GetEventParameter( event,
									kEventParamKeyModifiers,
									typeUInt32,
									NULL,
									sizeof( UInt32 ),
									NULL,
									&modifiers ) != noErr )
				return eventNotHandledErr;
			
			if (primaryButtonPressed)			
				osx_mouse_buttons &= ~primaryButtonFunction;
			
			switch (modifiers) {
				case controlKey:
					primaryButtonFunction = BUTTON3;
					break;
				case shiftKey:
					primaryButtonFunction = BUTTON2;
					break;
				default:
					primaryButtonFunction = BUTTON1;
			}
			
			if (primaryButtonPressed)			
				osx_mouse_buttons |= primaryButtonFunction;
			
			return noErr;
            break;
		}
    }
	
    return eventNotHandledErr;
}

static const EventTypeSpec kMouseEvents[] = {
    { kEventClassMouse, kEventMouseDown },
    { kEventClassMouse, kEventMouseUp },
    { kEventClassMouse, kEventMouseMoved },
    { kEventClassMouse, kEventMouseDragged },
    { kEventClassMouse, kEventMouseWheelMoved },
};

static OSStatus MouseEventHandler( EventHandlerCallRef handlerCallRef,
                                   EventRef event,
                                   void *userData )
{
	
	UInt32 eventKind = GetEventKind (event);
	switch (eventKind) {
		case kEventMouseDown:
		{
			WindowRef window;
			EventRecord oldStyleMacEvent;
			
			if (!ConvertEventRefToEventRecord( event, &oldStyleMacEvent ))
				return eventNotHandledErr;
			
			switch (FindWindow ( oldStyleMacEvent.where, &window )) {
				case inMenuBar:
				{
					if (eventKind == kEventMouseDown) {
						MenuSelect( oldStyleMacEvent.where );
						HiliteMenu(0);
						return noErr;
					}
					break;
				}
				case inContent:
				{
					EventMouseButton button;
					
					if ( GetEventParameter( event,
											kEventParamMouseButton,
											typeMouseButton,
											NULL,
											sizeof( EventMouseButton ),
											NULL,
											&button ) != noErr )
						return eventNotHandledErr;
					
					switch (button) {
						case kEventMouseButtonPrimary:
							primaryButtonPressed = TRUE;
							osx_mouse_buttons |= primaryButtonFunction;
							break;
						case kEventMouseButtonSecondary:
							osx_mouse_buttons |= BUTTON3;
							break;
						case kEventMouseButtonTertiary:
							osx_mouse_buttons |= BUTTON2;
							break;
					}
					return noErr;
					break;
				}
			}
			case kEventMouseUp:
			{
				EventMouseButton button;
				
				if ( GetEventParameter( event,
										kEventParamMouseButton,
										typeMouseButton,
										NULL,
										sizeof( EventMouseButton ),
										NULL,
										&button ) != noErr )
					return eventNotHandledErr;
				
				switch (button) {
					case kEventMouseButtonPrimary:
						primaryButtonPressed = FALSE;
						osx_mouse_buttons &= ~primaryButtonFunction;
						break;
					case kEventMouseButtonSecondary:
						osx_mouse_buttons &= ~BUTTON3;
						break;
					case kEventMouseButtonTertiary:
						osx_mouse_buttons &= ~BUTTON2;
						break;
				}
				
				return noErr;
				break;
			}
		}
		case kEventMouseMoved:
		case kEventMouseDragged:
		{
			HIPoint mouseLocation;
			Rect contentRect;
			
			if ( GetEventParameter( event,
									kEventParamMouseLocation,
									typeHIPoint,
									NULL,
									sizeof( HIPoint ),
									NULL,
									&mouseLocation ) != noErr )
				return eventNotHandledErr;
			
			if (GetWindowBounds(osx_window, kWindowContentRgn, &contentRect) != noErr)
				return eventNotHandledErr;
			
			osx_mouse_x = mouseLocation.x - contentRect.left;
			osx_mouse_y = mouseLocation.y - contentRect.top;
			return noErr;
		}
	}
	
	return eventNotHandledErr;
}

static const EventTypeSpec kCommandEvents[] = {
	{ kEventClassCommand, kEventCommandProcess }
};

static OSStatus CommandHandler( EventHandlerCallRef handlerCallRef,
								EventRef event,
								void *userData )
{
	HICommand command;
	if ( GetEventParameter( event,
							kEventParamDirectObject,
							typeHICommand,
							NULL,
							sizeof( HICommand ),
							NULL,
							&command ) != noErr )
		return eventNotHandledErr;
	
	switch ( command.commandID ) {
		case kHICommandClose:
		case kHICommandQuit:
			//driver->uninit();
			ExitToShell();
			return noErr;
			break;
		default:
			osx_menu_selected(GetMenuID(command.menu.menuRef), command.menu.menuItemIndex);
			return noErr;
			break;
	}
	
	return eventNotHandledErr;
}

static const EventTypeSpec kWindowEvents[] = {
	{ kEventClassWindow, kEventWindowBoundsChanged },
	{ kEventClassWindow, kEventWindowClose },
	{ kEventClassWindow, kEventWindowDrawContent },
};

static OSStatus WindowEventHandler( EventHandlerCallRef handlerCallRef,
									EventRef event,
									void *userData )
{
	switch (GetEventKind(event)) {
		case kEventWindowBoundsChanged:
		{
			WindowRef window;
			GetEventParameter(event, kEventParamDirectObject, typeWindowRef, NULL,
							  sizeof(WindowRef), NULL, &window);
			
			Rect rect;
			GetWindowPortBounds(window, &rect);
			
			if ( osx_window_width != rect.right || osx_window_height != rect.bottom) {
				osx_window_width  = rect.right;
				osx_window_height = rect.bottom;
				ui_call_resize();
			}
			break;
		}
			
		case kEventWindowClose:
		{
			//driver->uninit();
			ExitToShell();
			return noErr;
		}
		case kEventWindowDrawContent:
		{
			//driver->display();
		}
	}
	
	return eventNotHandledErr;
}

OSStatus InstallEventHandlers( void )
{
	OSStatus error;
	
	EventHandlerUPP MouseUPP = NewEventHandlerUPP( MouseEventHandler );
	
	if ((error = InstallEventHandler( GetApplicationEventTarget(),
									  MouseUPP,
									  GetEventTypeCount( kMouseEvents ),
									  kMouseEvents,
									  NULL,
									  &mouseEventHandlerRef )) != noErr)
		return error;
	
	EventHandlerUPP CommandUPP = NewEventHandlerUPP( CommandHandler );
	
	if ((error = InstallEventHandler( GetApplicationEventTarget(),
									  CommandUPP,
									  GetEventTypeCount( kCommandEvents ),
									  kCommandEvents,
									  NULL,
									  &commandEventHandlerRef )) != noErr)
		return error;
	
	EventHandlerUPP KeyboardUPP = NewEventHandlerUPP( KeyEventHandler );
	
	if ((error = InstallEventHandler( GetApplicationEventTarget(),
									  KeyboardUPP,
									  GetEventTypeCount( kKeyEvents ),
									  kKeyEvents,
									  NULL,
									  &keyEventHandlerRef )) != noErr)
		return error;
	
	EventHandlerUPP WindowUPP = NewEventHandlerUPP( WindowEventHandler );
	
	if ((error = InstallWindowEventHandler( osx_window,
											WindowUPP,
											GetEventTypeCount( kWindowEvents ),
											kWindowEvents,
											NULL,
											&windowEventHandlerRef )) != noErr)
		return error;
	
	return noErr;
}

OSStatus UninstallEventHandlers( void )
{
	OSStatus error;
	
	if ((error = RemoveEventHandler(mouseEventHandlerRef)) != noErr)
		return error;
		
	if ((error = RemoveEventHandler(commandEventHandlerRef)) != noErr)
		return error;

	if ((error = RemoveEventHandler(keyEventHandlerRef)) != noErr)
		return error;

	if ((error = RemoveEventHandler(windowEventHandlerRef)) != noErr)
		return error;
		
	return noErr;
}