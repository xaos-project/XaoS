/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/* The code is taken from GNU C library. Goal is to provide same
 * random numbers at all platforms to make possible save random
 * palette just by saving random seed
 *
 * Very simplified because I don't need to have numbers "very random"
 */

/*
 * This is derived from the Berkeley source:
 *      @(#)random.c    5.5 (Berkeley) 7/6/88
 * It was reworked for the GNU C Library by Roland McGrath.
 */

/* Ugly hack because of unknown problems w/ wa_list in v*print* in plan9 */
#ifdef _plan9_
#define va_list char *
#endif

#include <stdio.h>
#include <config.h>
#include <misc-f.h>
static unsigned int state;
void
XaoS_srandom (unsigned int x)
{
  state = x;
}

#define MYLONG_MAX 0xffffff	/*this is enought for me. */
long int
XaoS_random (void)
{
  state = ((state * 1103515245) + 12345) & MYLONG_MAX;
  return state;
}
