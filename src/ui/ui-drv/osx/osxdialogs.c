/*
 *  osxdialogs.c
 *  XaoS
 *
 *  Created by J.B. Langston III on 6/22/06.
 *  Copyright 2006 __MyCompanyName__. All rights reserved.
 *
 */

#include <Carbon/Carbon.h>
#include <ui.h>

#include "osxcommon.h"

OSStatus
GetFSRefOutOfNavReply( NavReplyRecord reply, FSRef* gotFSRef )
{
	AEDesc fsRefAEDesc;
    OSStatus status = paramErr;
	
	require( gotFSRef != NULL, FAIL );
	
	status = AECoerceDesc( &reply.selection, typeFSRef, &fsRefAEDesc );    
    require( status == noErr, COERCE_FAIL );
	status = AEGetDescData( &fsRefAEDesc, (void*)(gotFSRef), sizeof(FSRef));
    require( status == noErr, OPEN_FAIL_DISPOSEAEDESC );
    
OPEN_FAIL_DISPOSEAEDESC:
		AEDisposeDesc( &fsRefAEDesc );
COERCE_FAIL:
FAIL:
		
		switch( status )
		{
			case nsvErr:
				//WarnStatusString("\"no such volume\" error: %li",status);
				break;
			case noErr:
				break;
			default:
				//WarnStatusString("GetFSRefOutOfNavReply failed: %li",status);
				break;
		}
	return status;
}

OSStatus
GetCFURLOutOfNavReply( NavReplyRecord reply, CFURLRef* gotURLRef )
{
    OSStatus status = paramErr;
	FSRef fsRef;
	
	status = GetFSRefOutOfNavReply( reply, &fsRef );
	require( status == noErr, FAIL );
	*gotURLRef = CFURLCreateFromFSRef(NULL, &fsRef);
    
FAIL:
		if( status != noErr )
			//WarnStatusString("GetCFURLOutOfNavReply failed: %li",status);
			
			return status;
}

void MyNavEventProc (    
						 NavEventCallbackMessage callBackSelector,  
						 NavCBRecPtr callBackParms,   
						 void * callBackUD)
{
    OSStatus status = noErr;
    switch( callBackSelector )
    {
        case kNavCBUserAction:
		{
			NavReplyRecord reply;
			NavUserAction userAction = 0;
			status = NavDialogGetReply( callBackParms->context, &reply );
			if( status == noErr )
			{
				userAction = NavDialogGetUserAction( callBackParms->context );
				switch( userAction )
				{
					case kNavUserActionOpen:
					{
						CFURLRef fileURL;
						char filePath[255];
						
						GetCFURLOutOfNavReply( reply, &fileURL );
						
						CFURLGetFileSystemRepresentation(fileURL, true, (UInt8 *)filePath, sizeof(filePath));
						
						dialogparam *param = malloc (sizeof (dialogparam));
						param->dstring = strdup(filePath);
						ui_menuactivate ((CONST menuitem *)callBackUD, param);
						
						CFRelease( fileURL );
					}
						break;
						
				}
				status = NavDisposeReply( &reply );
			}
			break;
		}
    }
}

static void
osx_filedialog (struct uih_context *uih, CONST menuitem * item,
				CONST menudialog * dialog)
{
	char *name = NULL;
	OSStatus status;
	
	NavDialogCreationOptions myDialogOptions;
	NavDialogRef myDialogRef;
	NavEventUPP myNavEventUPP;
	
	status = NavGetDefaultDialogCreationOptions (&myDialogOptions);
	
	myNavEventUPP = NewNavEventUPP(MyNavEventProc);
	
	switch (dialog[0].type) {
		case DIALOG_IFILE:
			status = NavCreateGetFileDialog (&myDialogOptions,
											 NULL,
											 myNavEventUPP,
											 NULL,
											 NULL,
											 item,
											 &myDialogRef);
			break;
		case DIALOG_OFILE:
			status = NavCreatePutFileDialog (&myDialogOptions,
											 NULL,
											 NULL,
											 myNavEventUPP,
											 item,
											 &myDialogRef);
			break;
	}
	
	status = NavDialogRun (myDialogRef);
	
	NavDialogDispose (myDialogRef);
	
}

static void
osx_customdialog (struct uih_context *uih, CONST menuitem * item,
				  CONST menudialog * dialog)
{
	ControlRef	      control;
	WindowRef         theWindow; 
	WindowAttributes  windowAttrs;
	Rect              contentRect; 
	Point		      textSize;
	CFStringRef       titleKey;
	CFStringRef       windowTitle; 
	OSStatus          result; 
	SInt16				baseLine;
	CFStringRef		question, text;
	int i;
	int top, left;
	int maxwidth;
	char defstr[255];
	
	windowAttrs = kWindowStandardDocumentAttributes // 1
		| kWindowStandardHandlerAttribute 
		| kWindowInWindowMenuAttribute; 
	SetRect (&contentRect, 100,  100, // 2
			 500, 500);
	
	CreateNewWindow (kDocumentWindowClass, windowAttrs,// 3
					 &contentRect, &theWindow); 
	
	titleKey    = CFSTR("Dialog"); // 4
	windowTitle = CFCopyLocalizedString(titleKey, NULL); // 5
	result = SetWindowTitleWithCFString (theWindow, windowTitle); // 6
	CFRelease (titleKey); // 7
	CFRelease (windowTitle); 
	
	
	RepositionWindow (theWindow, osx_window, // 9
					  kWindowCenterOnParentWindow); 
	
	
	top = 14;
	left = 20;
	maxwidth = 0;
	for (i = 0; dialog[i].question; i++)
	{
		question = CFStringCreateWithCString(kCFAllocatorDefault, dialog[i].question, kCFStringEncodingUTF8);
		GetThemeTextDimensions(question, kThemeSystemFont, kThemeStateActive, false, &textSize, &baseLine);
		SetRect(&contentRect, left, top, left+textSize.h, top+textSize.v);
		CreateStaticTextControl(theWindow, &contentRect, question, NULL, &control);
		CFRelease(question);
		GetBestControlRect(control, &contentRect, &baseLine);
		SetControlBounds(control, &contentRect);
		
		top += contentRect.bottom - contentRect.top;
		top += 8;
		
		if (contentRect.right - contentRect.left > maxwidth) 
			maxwidth = contentRect.right - contentRect.left;
		
	}

	top = 14;
	left += maxwidth + 8;
	printf("maxwidth=%d\n", maxwidth);
	for (i = 0; dialog[i].question; i++)
	{
		printf("%d,", dialog[i].type);
		switch (dialog[i].type)
		{
			case DIALOG_INT:
			case DIALOG_FLOAT:
			case DIALOG_STRING:
				switch (dialog[i].type)
				{
					case DIALOG_INT:
						sprintf (defstr, "%d", dialog[i].defint);
						break;
					case DIALOG_FLOAT:
						sprintf (defstr, "%f", dialog[i].deffloat);
						break;
					case DIALOG_STRING:
						sprintf (defstr, "%s", dialog[i].defstr);
						break;
				}
				SetRect(&contentRect, left, top, left+1000, top+1000);
				text = CFStringCreateWithCString(kCFAllocatorDefault, defstr, kCFStringEncodingUTF8);
				CreateEditUnicodeTextControl(theWindow, &contentRect, text, false, NULL, &control);
				CFRelease(text);
				GetBestControlRect(control, &contentRect, &baseLine );
				SetControlBounds(control, &contentRect);
				
				top += contentRect.bottom - contentRect.top;
				top += 8;
			case DIALOG_CHOICE:
				SetRect(&contentRect, left, top, left+200, top+20);
				CreatePopupButtonControl(theWindow, &contentRect, NULL, -12345, false, -1, 0, 0, &control);
				GetBestControlRect(control, &contentRect, &baseLine );
				SetControlBounds(control, &contentRect);

				top += contentRect.bottom - contentRect.top;
				top += 8;
		}
		
	}

	
	ShowWindow (theWindow); // 10}
}

void
osx_dialog (struct uih_context *uih, CONST char *name)
{
	CONST menuitem *item = menu_findcommand (name);
	CONST menudialog *dialog;
	int nitems;
	
	if (!item)
		return;
	dialog = menu_getdialog (uih, item);
	if (!dialog)
		return;
	for (nitems = 0; dialog[nitems].question; nitems++);
	if (nitems == 1	&& (dialog[0].type == DIALOG_IFILE || dialog[0].type == DIALOG_OFILE))
		osx_filedialog (uih, item, dialog);
	else
		osx_customdialog(uih, item, dialog);
}
