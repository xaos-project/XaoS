// The Application class for XaoS on BeOS.
// Copyright © 1997  Jens Kilian (jjk@acm.org)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include <cstring>
#include <stdio.h>

#include <Application.h>
#include <Archivable.h>
#include <Bitmap.h>
#include <File.h>
#include <Message.h>
#include <OS.h>
#include <Point.h>
#include <Rect.h>
#include <NodeInfo.h>
#include <Resources.h>
#include <Roster.h>
#include <Screen.h>
#include <Entry.h>
#include <setjmp.h>
#include <Path.h>

#include "XaoSApplication.h"
#include "XaoSSplashView.h"
#include "XaoSSplashWindow.h"
#include "XaoSWindow.h"

#include "setjmp.h"
#include "version.h"
#include "xerror.h"
#include "xio.h"
#include "xthread.h"

// Application signature.
static const char kSignature[] = "application/x-jjk-xaos";
static char *loadarg=NULL;
static char *filearg=NULL;

extern jmp_buf translatorjmp;
extern int translator;
extern "C" {
void be_exit_xaos(int i)
{
  /* In translator mode we ought to cleanup everything and return error.
     but we can't. XaoS calls exit in translator mode only in x_fatalerror
     that is called in case of internal error and out of memory sitations
     only. Hope this is not major problem.*/
  if (translator) longjmp(translatorjmp, 1);
  if(be_app) {
		be_app->PostMessage(B_QUIT_REQUESTED);
		((XaoSApplication*)be_app)->mExitStatus=i;
		xth_uninit();
		if(i==10) exit(10);
		exit_thread(i);
  } else exit(i);
}
}

XaoSApplication::XaoSApplication(main_function_ptr pMainFunction)
:	inherited(kSignature),
	mExitStatus(-1),
	mMainThreadID(B_ERROR),
	mpMainFunction(pMainFunction),
	mArgc(0),
	mppArgv(0)
{
	// empty
}
static const char *xpfdesc="XaoS Position File";
static const char *xafdesc="XaoS Animation File";

/* Install information about xaf and xpf to mime database, set icons etc. 
   this is done only in the case mime database don't contain their records*/
void XaoSApplication::InstallMimeTypes()
{
	int installed1=0, installed2=0;
	BMimeType mime;
	size_t length;
	char name[B_MIME_TYPE_LENGTH];
	// Get application info.
	app_info info;
	if (GetAppInfo(&info) != B_NO_ERROR) {
		x_error("Oh my god! I don't exist!\n");
		return;
	}
	
	// Get the executable.
	BFile appFile(&(info.ref), O_RDONLY);
	if (appFile.InitCheck() != B_NO_ERROR) {
		x_error("Can't open myself! Help me!\n");
		return;
	}
	
	// Get resources.
	BResources resources;
	if (resources.SetTo(&appFile) != B_NO_ERROR) {
		x_error("Someone throwed away my resources! Pleasase recompile me!\n");
		return;
	}
	// Find the icon resource.	
	void *pResource = resources.FindResource((type_code) 'MICN', (int32) 0, &length);
	if (!pResource) {
		x_error("Resources are wrong. I want MY resources back!\n");
		return;
	}
	
	BBitmap xpfsmall(BRect(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1), B_COLOR_8_BIT);
	xpfsmall.SetBits(pResource, xpfsmall.BitsLength(),0,B_COLOR_8_BIT);
	delete pResource;


	pResource = resources.FindResource('ICON', (int32) 0, &length);
	if (!pResource) {
		x_error("Resources are wrong. I want MY resources back!\n");
		return;
	}
	BBitmap xpflarge(BRect(0, 0, B_LARGE_ICON - 1, B_LARGE_ICON - 1), B_COLOR_8_BIT);
	xpflarge.SetBits(pResource, xpflarge.BitsLength(),0,B_COLOR_8_BIT);
	delete pResource;
	
	pResource = resources.FindResource((type_code) 'MICN', (int32) 1, &length);
	if (!pResource) {
		x_error("Resources are wrong. I want MY resources back!\n");
		return;
	}
	
	BBitmap xafsmall(BRect(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1), B_COLOR_8_BIT);
	xafsmall.SetBits(pResource, xafsmall.BitsLength(),0,B_COLOR_8_BIT);
	delete pResource;


	pResource = resources.FindResource('ICON', (int32) 1, &length);
	if (!pResource) {
		x_error("Resources are wrong. I want MY resources back!\n");
		return;
	}
	BBitmap xaflarge(BRect(0, 0, B_LARGE_ICON - 1, B_LARGE_ICON - 1), B_COLOR_8_BIT);
	xaflarge.SetBits(pResource, xpflarge.BitsLength(),0,B_COLOR_8_BIT);
	delete pResource;
	mime.SetType("image/x-xaos-position");
	mime.GetShortDescription(name);
	if (!mime.IsInstalled() || strcmp(name,xpfdesc)) {
		BMessage ext;
		if (!mime.IsInstalled()) mime.Install();
		printf("Installing XaoS Position File mime type\n");
		installed1=1;
		ext.AddString("extensions","xpf");
		mime.SetFileExtensions(&ext);
		mime.SetShortDescription(xpfdesc);
		mime.SetLongDescription("Position files used by XaoS realtime fractal zoomer");
		mime.SetPreferredApp(kSignature);
		mime.SetIcon(&xpflarge, B_LARGE_ICON);
		mime.SetIcon(&xpfsmall, B_MINI_ICON);
	}
	mime.SetType("video/x-xaos-animation");
	mime.GetShortDescription(name);
	if (!mime.IsInstalled() || strcmp(name,xafdesc)) {
		BMessage ext;
		if (!mime.IsInstalled()) mime.Install();
		printf("Installing XaoS Animation File mime type\n");
		installed2=1;
		ext.AddString("extensions","xaf");
		mime.SetFileExtensions(&ext);
		mime.SetShortDescription("XaoS Animation File");
		mime.SetLongDescription("Animation files used by XaoS realtime fractal zoomer");
		mime.SetPreferredApp(kSignature);
		mime.SetIcon(&xaflarge, B_LARGE_ICON);
		mime.SetIcon(&xafsmall, B_MINI_ICON);
	}
	if (installed1 && installed2)
		x_message("To archieve a more confortable behaviour I've registered the new mime types:\n\nimage/x-xaos-position (XaoS Position File)\nvideo/x-xaos-animation (XaoS Animation File)\n\nLovely!\n");
	else if (installed1)
		x_message("To archieve a more confortable behaviour I've registered the new mime type:\n\nimage/x-xaos-position (XaoS Position File)\n\n Thank you!");
	else if (installed2)
		x_message("To archieve a more confortable behaviour I've registered the new mime type:\n\nvideo/x-xaos-animation (XaoS Animation File)\n\n Lovely!\n");
}

XaoSApplication::~XaoSApplication(void)
{
	if (mppArgv) {
		// Delete command line arguments.
		for (int i = 0; i < mArgc; ++i) {
			delete [] mppArgv[i];
		}
		delete [] mppArgv;
		mppArgv = 0;
	}
}

void
XaoSApplication::AboutRequested(void)
{
	BBitmap *b = MakeSplashBitmap();
	if (b!=NULL) {
		XaoSSplashView *pSplashView =
			new XaoSSplashView(MakeSplashBitmap());
		(new XaoSSplashWindow(pSplashView))->Go();
	}
}

void
XaoSApplication::ArgvReceived(int32 argc, char **ppArgv)
{
	if (mppArgv) {
		// This shouldn't happen (as long as XaoS is not made
		// a single-launch application, which won't happen soon :-)
	} else {
		// Copy the command line arguments.
		mArgc = (int)argc;
		char **p = mppArgv = new char*[mArgc+3];
		
		for (int i = 0; i < mArgc; ++i) {
			*p++ = strcpy(new char [strlen(*ppArgv)+1], *ppArgv);
			++ppArgv;
		}
		*p = 0;
	}
}

bool
XaoSApplication::QuitRequested(void)
{
	// Cleanly shutting down a BeOS application is hard work.
	// This function may be called multiple times before it finally
	// returns 'true'.  In an extreme case, the following
	// interactions may occur:
	//
	//		User			Application			Window			XaoS thread
	//		 |	B_QUIT_REQ.	  |				  |				    |
	//		 | ##########>	  | QuitReq.	  |				    |
	//		 |					  | =========>	  | Quit cmd.	    |
	//		 |					  | false		  | ############>	 |
	//		 |					  | <---------	  |				    |
	//		 |					  |				  | AllowQuit	    |
	//		 |					  |				  | <===========	 |
	//		 |					  |				  | B_QUIT_REQ.	 |
	//		 |					  | B_QUIT_REQ	  | <############	 |
	//		 |					  | <#########	  |				    |
	//		 |					  | QuitReq.	  |				(thread ends)
	//		 |					  | =========>	  |				    .
	//		 |					  | true			  |				    .
	//		 |					  | <---------	  |				    .
	//		 |					  | Quit			  |				    .
	//		 |					  | =========>	  |				    .
	//		 |					  |				  .				    .
	//		 |			(wait_for_thread)		  .				    .
	//		 |					  | < - - - - - - - - - - - - - - .
	//		 |					  |				  .				    .
	//
	//		#######> BMessage or other asynchronous event
	//		=======> function call
	//		-------> function return value
	
	const bool mayQuit = inherited::QuitRequested();
	if (mayQuit && mMainThreadID >= B_NO_ERROR) {
		// Wait for the main thread to finish.
		int32 dummy;
		(void)wait_for_thread(mMainThreadID, &dummy);
		mMainThreadID = B_ERROR;
	}
	
	return mayQuit;
}
// This is a small hack to add -loadpos or -play option in case files are specified
void
XaoSApplication::RefsReceived(BMessage *message)
{
	const char *filename;
	char type[B_MIME_TYPE_LENGTH];
	int l;
	BPath path;
   	entry_ref refs;
	int animation = 0;
	int one=0;
	int c;
	xio_file sfile=NULL;

   	if (message->FindRef("refs", 0, &refs) != B_OK) return;
   	if (message->FindRef("refs", 1, &refs) != B_OK) one=1;

	/* In case one file is received just load it. For multiple files construct an "slide show" animation */
	for(c=0;message->FindRef("refs",c,&refs)==B_OK;c++) {
   		BEntry e(&refs,true);
   		e.GetPath(&path);
		filename=path.Path();
		l=strlen(filename);
		BNode node(&e);
		BNodeInfo ni(&node);
		if (ni.GetType(type)!=B_OK) type[0]=0;
	
		if (!strcmp(type, "image/x-xaos-position")) animation = 0;
		else if (!strcmp(type, "video/x-xaos-animation")) animation = 1;
		else if (l>4 && !strcmp(type+l-4, ".xpf")) animation = 0;
		else if (l>4 && !strcmp(type+l-4, ".xaf")) animation = 1;
		else x_fatalerror("%s:Unknown file\n",filename), exit(1);
		if (one) {
			filearg=strdup(filename);
			if (!animation) loadarg=strdup("-loadpos"); else loadarg=strdup("-play");
		} else {
			if (!sfile) sfile=xio_strwopen();
			xio_puts("(load \"",sfile);
			xio_puts((const char *)filename,sfile);
			xio_puts("\")\n(usleep 500000)(wait)(usleep 500000)\n",sfile);
		}
	}
	if (sfile) {
		loadarg=strdup("-playstr");
		xio_close(sfile);
		filearg=xio_getstring(sfile);
	}
}

void
XaoSApplication::ReadyToRun(void)
{
        InstallMimeTypes();
	if (!mppArgv) {
		// User didn't pass any command line options.
		// ### TO DO: LET THE USER SELECT MODE, SIZE ETC.

		mppArgv = new char*[mArgc+3];
		mppArgv[1] = 0;
		mArgc = 1;
	} else delete mppArgv[0];
	if (loadarg) {
		mppArgv[mArgc] = loadarg;
		mppArgv[mArgc+1] = filearg;
		mArgc+=2;
	}

	/* Get the path to file executable. XaoS use it to locate
	tutorials and other files */

        app_info info;
        if (GetAppInfo(&info) != B_NO_ERROR) {
	        x_error("Oh my god! I don't exist!\n");
        }
        BEntry e(&info.ref,true);
	BPath path;
        e.GetPath(&path);
        const char *filename=path.Path();
	mppArgv[0] = strdup (filename);

	// Spawn and start the thread running the real main program.
	if ((mMainThreadID = spawn_thread(MainThread,
		 										 "XaoS main program",
		 										 B_NORMAL_PRIORITY,
		 										 this)) < B_NO_ERROR
		 || resume_thread(mMainThreadID) < B_NO_ERROR)
	{
		PostMessage(B_QUIT_REQUESTED);
	}
}

long
XaoSApplication::MainThread(void *data)
{
	XaoSApplication *pApp = (XaoSApplication *)data;
	pApp->mExitStatus =
		(pApp->mpMainFunction)(pApp->mArgc, pApp->mppArgv);
	printf("%i\n",pApp->mExitStatus);

	return 0;
}

BBitmap *
XaoSApplication::MakeSplashBitmap(void)
{
	// Get application info.
	app_info info;
	if (GetAppInfo(&info) != B_NO_ERROR) {
		x_error("No application - no about boxes. Sorry!\n");
		return 0;
	}
	
	// Get the executable.
	BFile appFile(&(info.ref), O_RDONLY);
	if (appFile.InitCheck() != B_NO_ERROR) {
		x_error("No application - no about boxes. Sorry!\n");
		return 0;
	}
	
	// Get resources.
	BResources resources;
	if (resources.SetTo(&appFile) != B_NO_ERROR) {
		x_error("No resources - no about boxes. Sorry!\n");
		return 0;
	}

	// Find the splash screen resource.	
	size_t length;
	void *pResource = resources.FindResource('XaSp', 1, &length);
	if (!pResource) {
		x_error("Resources are wrong and the lovely about dialog is lost. Sorry!\n");
		return 0;
	}
	
	// Unflatten the resource.
	BMessage archived;
	status_t status = archived.Unflatten((const char *)pResource);
	delete pResource;
	if (status != B_NO_ERROR) {
		return 0;
	}
	
	// Instantiate & return a bitmap.
	BArchivable *pResult = instantiate_object(&archived);
	if (!pResult) {
		return 0;
	}
	BBitmap *pSplashBitmap = dynamic_cast<BBitmap *>(pResult);
	if (!pSplashBitmap) {
		delete pResult;
	}
	
	return pSplashBitmap;
}

