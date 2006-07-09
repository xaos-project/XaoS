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
}