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

#ifndef XAOSAPPLICATION_H
#define XAOSAPPLICATION_H

#include <Application.h>
#include <Bitmap.h>
#include <OS.h>

class XaoSApplication : public BApplication
{
public:
	typedef BApplication inherited;
	typedef int (*main_function_ptr)(int argc, char **ppArgv);
	
	// Constructor, destructor.
	XaoSApplication(main_function_ptr pMainFunction);
	virtual ~XaoSApplication(void);
	
	// Hook functions.
	virtual void AboutRequested(void);
	virtual void ArgvReceived(int32 argc, char **ppArgv);
	virtual void RefsReceived(BMessage *m);
	virtual bool QuitRequested(void);
	virtual void ReadyToRun(void);
	
	// Exit status of real main program.
	int ExitStatus(void) const;

	int mExitStatus;
private:
	XaoSApplication(const XaoSApplication &orig);
	XaoSApplication &operator =(const XaoSApplication &orig);
	void InstallMimeTypes(void);

	// Auxiliary methods.
	static long MainThread(void *data);
	BBitmap *MakeSplashBitmap(void);
	
	// Data members.
	thread_id mMainThreadID;
	main_function_ptr mpMainFunction;
	int mArgc;
	char **mppArgv;
};

inline int
XaoSApplication::ExitStatus(void) const
{
	return mExitStatus;
}

#endif // XAOSAPPLICATION_H
