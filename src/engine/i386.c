
/*
 * ctrl87.c
 */


#define __CONTROL87_C__


#include "../include/i386/ctrl87.h"
#include "../include/config.h"

#ifdef __GNUC__
#ifdef __i386__
#ifndef NOASSEMBLY

/***** _control87 *****/
unsigned short
_control87 (unsigned short newcw, unsigned short mask)
{
  unsigned short cw;

  asm volatile ("                                                    \n\
      wait                                                          \n\
      fstcw  %0                                                       ": /* outputs */ "=m" (cw)
		:		/* inputs */
    );

  if (mask)
    {				/* mask is not 0 */
      asm volatile ("                                                  \n\
        mov    %2, %%ax                                             \n\
        mov    %3, %%bx                                             \n\
        and    %%bx, %%ax                                           \n\
        not    %%bx                                                 \n\
        nop                                                         \n\
        wait                                                        \n\
        mov    %1, %%dx                                             \n\
        and    %%bx, %%dx                                           \n\
        or     %%ax, %%dx                                           \n\
        mov    %%dx, %0                                             \n\
        wait                                                        \n\
        fldcw  %1                                                     ": /* outputs */ "=m" (cw)
		    : /* inputs */ "m" (cw), "m" (newcw), "m" (mask)
		    : /* registers */ "ax", "bx", "dx"
	);
    }
  return cw;

}				/* _control87 */
#endif
#endif
#endif

#ifdef __GNUC__
#ifdef __i386__
#if 0

/*
 * copy.c -- fast memcpy routines for Pentium using FPU
 * Copyright (c) 1995, 1996 Robert Krawitz <rlk@tiac.net>
 * and Gerhard Koerting (G.Koerting@techem.ruhr-uni-bochum.de)
 * Exception handling in kernel/userspace routines by Gerhard
 * Koerting.
 * May be used and redistributed under the terms of the GNU Public License
 */
#include <sys/types.h>

#define CACHELINE 32
#define CACHEMASK (CACHELINE - 1)
#define BIGMASK (~255)
#define SMALLMASK (~31)

void *
penium___zero_chunk (void *_to, size_t _bytes)
{
  unsigned long temp0, temp1;
  register unsigned long to asm ("di") = (unsigned long) _to;
  register unsigned long bytes asm ("dx") = (unsigned long) _bytes;
  char save[42];
  unsigned long zbuf[2] = { 0, 0 };
  temp0 = to & 7;
  if (temp0)
    {
      bytes -= temp0;
      asm __volatile__ ("cld\n\t"
			"rep; stosb\n\t":"=D" (to):"D" (to), "a" (0),
			"c" (temp0):"cx");
    }
  asm __volatile__ ("shrl $3, %0\n\t"
		    "fstenv %4\n\t"
		    "fstpt 32+%4\n\t"
		    "movl (%1), %2\n\t"
		    "fildq %3\n"
		    "2:\n\t"
		    "fstl (%1)\n\t"
		    "addl $8, %1\n\t"
		    "decl %0\n\t"
		    "jne 2b\n\t"
		    "fstpl %%st\n\t"
		    "fldt 32+%4\n\t"
		    "fldenv %4\n\t":"=&r" (temp0), "=&r" (to),
		    "=&r" (temp1):"m" (*(char *) zbuf), "m" (*(char *) save),
		    "0" (bytes), "1" (to));
  bytes &= 7;
  if (bytes)
    {
      asm __volatile__ ("shrl $2, %%ecx\n\t"
			"cld\n\t"
			"rep ; stosl\n\t"
			"testb $2,%%dl\n\t"
			"je 111f\n\t"
			"stosw\n"
			"111:\ttestb $1,%%dl\n\t"
			"je 112f\n\t"
			"stosb\n"
			"112:":"=D" (to):"D" (to), "a" (0), "c" (bytes),
			"d" (bytes):"cx", "memory");
    }
  return _to;
}

void *
pentium__memcpy_g (void *_to, const void *_from, size_t _bytes)
{
  register unsigned long from asm ("si") = (unsigned long) _from;
  register unsigned long to asm ("di") = (unsigned long) _to;
  register unsigned long bytes asm ("dx") = (unsigned long) _bytes;
  if (bytes >= 1024)
    {
      unsigned long temp0, temp1;
      char save[108];

      temp0 = to & 7;
      if (temp0)
	{
	  bytes -= temp0;
	  asm __volatile__ ("cld\n\t"
			    "rep; movsb\n\t":"=D" (to), "=S" (from):"D" (to),
			    "S" (from), "c" (temp0):"cx");
	}
      asm __volatile__ ("shrl $8, %0\n\t"
			"movl (%2), %3\n\t" "movl (%1), %3\n\t"
			/*"fsave %4\n" */
			"1:\n\t"
			"movl $4, %3\n"
			"2:\n\t"
			"fildq 0x0(%2)\n\t"
			"fildq 0x20(%2)\n\t"
			"fildq 0x40(%2)\n\t"
			"fildq 0x60(%2)\n\t"
			"fildq 0x80(%2)\n\t"
			"fildq 0xa0(%2)\n\t"
			"fildq 0xc0(%2)\n\t"
			"fildq 0xe0(%2)\n\t"
			"fxch\n\t"
			"fistpq 0xc0(%1)\n\t"
			"fistpq 0xe0(%1)\n\t"
			"fistpq 0xa0(%1)\n\t"
			"fistpq 0x80(%1)\n\t"
			"fistpq 0x60(%1)\n\t"
			"fistpq 0x40(%1)\n\t"
			"fistpq 0x20(%1)\n\t"
			"fistpq 0x0(%1)\n\t"
			"addl $8, %2\n\t"
			"addl $8, %1\n\t"
			"decl %3\n\t"
			"jne 2b\n\t"
			"addl $224, %2\n\t"
			"addl $224, %1\n\t" "decl %0\n\t" "jne 1b\n\t"
			/*"frstor %4\n\t" */
			:"=&r" (temp0), "=&r" (to), "=&r" (from),
			"=&r" (temp1):"m" (save[0]), "0" (bytes), "1" (to),
			"2" (from):"memory");
      bytes &= 255;
    }
  if (bytes)
    {
      asm __volatile__ ("shrl $2, %%ecx\n\t"
			"cld\n\t"
			"rep ; movsl\n\t"
			"testb $2,%%dl\n\t"
			"je 111f\n\t"
			"movsw\n"
			"111:\ttestb $1,%%dl\n\t"
			"je 112f\n\t"
			"movsb\n"
			"112:":"=D" (to), "=S" (from):"D" (to), "S" (from),
			"c" (bytes), "d" (bytes):"cx", "memory");
    }
  return _to;
}
#endif
#endif
#endif
