/*
        This file is part of mac driver for XaoS
	Common Dialogs.c
	Error, OK/Cancel, etc.
*/

#include "Common Dialogs.h"

#include <Dialogs.h>
#include <QDOffScreen.h>

void doErrorAlert(Str255 errmsg)
{
	short			itemHit;
	DialogPtr	myDialog;
	Handle		itemHandle;
	short			itemType;
	Rect			displayRect;
	CGrafPtr	savePort;
	GDHandle	saveDev;
	
	SysBeep(5);
	ParamText(errmsg,"\p ","\p ","\p ");
	myDialog = GetNewDialog(rErrorAlert,nil,(WindowPtr) -1);

	GetGWorld(&savePort, &saveDev);
		
	if(myDialog != nil)
	{
		SetPort(myDialog);
		PenSize(3,3);
		GetDItem(myDialog,ok,&itemType,&itemHandle,&displayRect);
		InsetRect(&displayRect,-4,-4);
		FrameRoundRect(&displayRect,16,16);
		do
		{
			ModalDialog(nil, &itemHit);
		} while(itemHit != ok);
		DisposDialog(myDialog);
	}
	SetGWorld(savePort, saveDev);
}

Boolean doOKcancel(Str255 msg)
{
	short			itemHit;
	DialogPtr		myDialog;
	Handle			itemHandle;
	short			itemType;
	Rect			displayRect;
	CGrafPtr	savePort;
	GDHandle	saveDev;
	Boolean		result = false;
	
	ParamText(msg,"\p ","\p ","\p ");
	myDialog = GetNewDialog(rOKcancelAlert,nil,(WindowPtr) -1);
	
	if(myDialog != nil)
	{
		GetGWorld(&savePort, &saveDev);		/* remember and restore current grafPort */
		SetPort(myDialog);
		PenSize(3,3);
		GetDItem(myDialog,ok,&itemType,&itemHandle,&displayRect);
		InsetRect(&displayRect,-4,-4);
		FrameRoundRect(&displayRect,16,16);
		do
		{
			ModalDialog(nil, &itemHit);
			if(itemHit == ok)
				result = true;
		} while(itemHit != ok && itemHit != cancel);
		DisposDialog(myDialog);
		SetGWorld(savePort, saveDev);
	}
	return result;
}
