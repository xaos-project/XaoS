/*
        This file is part of mac driver for XaoS
	Common Dialogs.h
	
	File 'Common Dialogs.rsrc' must be included in project.
*/

#define	ok				1
#define cancel			2
#define rErrorAlert		129
#define rOKcancelAlert	130


void doErrorAlert (Str255 errmsg);
Boolean doOKcancel (Str255 msg);
