// The main program for XaoS on BeOS.
//
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

#include "XaoSApplication.h"
extern int translator;
extern int linkaddontoo;

//	The real main() function - renamed for BeOS.
extern "C" int be_main(int argc, char **ppArgv);
#include <stdio.h>
//	The usual highly exciting Be main program.
int main(void)
{
	/*linkaddontoo=1;*/
	XaoSApplication theApplication(be_main);
	theApplication.Run();
	return(theApplication.ExitStatus());
 
}
