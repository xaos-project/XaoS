/*
 *     XaoS, a fast portable realtime fractal zoomer 
 *                  Copyright © 1996,1997 by
 *
 *      Jan Hubicka          (hubicka@paru.cas.cz)
 *      Thomas Marsh         (tmarsh@austin.ibm.com)
 *
 *	be_error.cpp	BeOS user interface error output
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

#include <stdarg.h>
#include <stdio.h>
#include <Alert.h>
#include <Application.h>
#include "config.h"
#include "xerror.h"

int be_noalert=0;
/* An message displaying routines used by XaoS */
void
x_message(const char *text,...)
{
  va_list ap;
  char buf[4096];
  va_start (ap, text);
  vsprintf(buf,text,ap);
  fputs(buf,stderr);
  fputs("\n",stderr);
  snooze(1000000/3);
  if (be_app && !be_noalert) {
  BAlert *allert=new BAlert("XaoS",buf, "I see!", NULL, NULL, B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_INFO_ALERT);
  allert->Go();
  }
  va_end (ap);
}
void
x_error(const char *text,...)
{
  va_list ap;
  char buf[4096];
  va_start (ap, text);
  vsprintf(buf,text,ap);
  fputs(buf,stderr);
  fputs("\n",stderr);
  snooze(1000000/3);
  if (be_app && !be_noalert) {
  BAlert *allert=new BAlert("XaoS is in trouble",buf, "Oops!", NULL, NULL, B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
  allert->Go();
  }
  va_end (ap);
}
void
x_fatalerror(const char *text,...)
{
  va_list ap;
  char buf[4096];
  va_start (ap, text);
  vsprintf(buf,text,ap);
  fputs(buf,stderr);
  fputs("\n",stderr);
  snooze(1000000/3);
  if (be_app && !be_noalert) {
  BAlert *allert=new BAlert("XaoS crashed badly",buf, "Kill?", "Kill!", "Kill!!!!!!", B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_STOP_ALERT);
  allert->Go();
  }
  va_end (ap);
  exit_xaos(10);
}
