#if 0
#ifndef _I386_STRING_I486_H_
#define _I386_STRING_I486_H_
#if defined(__OPTIMIZE__) && defined(__GNUC__) && defined(__i386__)
/*
 * This string-include defines all string functions as inline
 * functions. Use gcc. It also assumes ds=es=data space, this should be
 * normal. Most of the string-functions are rather heavily hand-optimized,
 * see especially strtok,strstr,str[c]spn. They should work, but are not
 * very easy to understand. Everything is done entirely within the register
 * set, making the functions fast and clean. 
 *
 *		Copyright (C) 1991, 1992 Linus Torvalds
 *		Revised and optimized for i486/pentium
 *		1994/03/15 by Alberto Vignani/Davide Parodi @crf.it
 *
 *	Split into 2 CPU specific files by Alan Cox to keep #ifdef noise down.
 *
 *	Revised and optimized again by Jan Hubicka  (1997/11/16)
 *	(please report bugs to hubicka@paru.cas.cz)
 *
 *	memset and memcpy routines seems to be always faster at 486 and
 *	pentium but at pentium MMX they are sometimes bit slower (5-10%)..
 *	because of less strict register allocation they produces better code.
 */


#define __HAVE_ARCH_MEMCPY
#define memcpy(d,s,count) \
(__builtin_constant_p(count) ? \
 __memcpy_c((d),(s),(count)) : \
 __memcpy_g((d),(s),(count)))

/*
 *	These ought to get tweaked to do some cache priming.
 */


/* This implementation of the memcpy is designed for moveoldpoints from
 * mkrealloctables. It is expected to work well for both small and large
 * sizes.
 *
 * Small (1-10) and meduim (300) sizes seems to be important for XaoS. 
 * So implementation is not super fast for large sizes, but my experiemnts
 * don't show large improvements in speed anyway.
 *
 * We use rep movsX operations (they works well on PPro and don't seems to be
 * so bad on Pentium) and expect cld operation to be set. Hope that it will
 * not make problems.
 *
 * My attempt was to use c code where possible to let GCC do the
 */
extern inline void *__memcpy_g (void *to, const register void *from,
				register size_t n);
extern inline void *
__memcpy_g (void *to, const register void *from, register size_t n)
{
  register void *tmp = (void *) to;
  if (n >= 7)
    {
      register int c = (-(int) to) & 3;
      n -= c;
      __asm__ __volatile__ (	/*Align the destination */
			     "rep\n\tmovsb":"=c" (c), "=D" (tmp),
			     "=S" (from):"c" (c), "D" ((long) tmp),
			     "S" ((long) from):"memory");
      c = n >> 2;
      __asm__ __volatile__ (	/*Copy the main body */
			     "rep\n\tmovsl":"=c" (c), "=D" (tmp),
			     "=S" (from):"c" (c), "D" ((long) tmp),
			     "S" ((long) from):"memory");
      n &= 3;
    }
  __asm__ __volatile__ ("rep\n\tmovsb":"=c" (n), "=D" (tmp),
			"=S" (from):"c" (n), "D" ((long) tmp),
			"S" ((long) from):"memory");
  return (to);
}

/*
 * This looks horribly ugly, but the compiler can optimize it totally,
 * as the count is constant.
 */

#define COMMON(x) \
__asm__ __volatile__ ( \
	"\n.align 4\n" \
	"1:\tmovl (%2),%0\n\t" \
	"addl $4,%2\n\t" \
	"movl %0,(%1)\n\t" \
	"addl $4,%1\n\t" \
	"decl %3\n\t" \
	"jnz 1b\n" \
	x \
	:"=r" (dummy1), "=r" (tmp), "=r" (from), "=r" (dummy2)  \
	:"1" (tmp), "2" (from), "3" (n/4) \
	:"memory"); \
return (to); \

extern inline void *__memcpy_c (void *to, const void *from, size_t n);
extern inline void *
__memcpy_c (void *to, const void *from, size_t n)
{
  if (n < 24)
    {
      if (n >= 4)
	((unsigned long *) to)[0] = ((const unsigned long *) from)[0];
      if (n >= 8)
	((unsigned long *) to)[1] = ((const unsigned long *) from)[1];
      if (n >= 12)
	((unsigned long *) to)[2] = ((const unsigned long *) from)[2];
      if (n >= 16)
	((unsigned long *) to)[3] = ((const unsigned long *) from)[3];
      if (n >= 20)
	((unsigned long *) to)[4] = ((const unsigned long *) from)[4];
      switch ((unsigned int) (n % 4))
	{
	case 3:
	  ((unsigned short *) to)[n / 2 - 1] =
	    ((const unsigned short *) from)[n / 2 - 1];
	  ((unsigned char *) to)[n - 1] =
	    ((const unsigned char *) from)[n - 1];
	  return to;
	case 2:
	  ((unsigned short *) to)[n / 2 - 1] =
	    ((const unsigned short *) from)[n / 2 - 1];
	  return to;
	case 1:
	  ((unsigned char *) to)[n - 1] =
	    ((const unsigned char *) from)[n - 1];
	case 0:
	  return to;
	}
    }
  {
    register void *tmp = (void *) to;
    register int dummy1, dummy2;
    switch ((unsigned int) (n % 4))
      {
      case 0:
	COMMON ("");
      case 1:
	COMMON ("movb (%2),%b0 ; movb %b0,(%1)");
      case 2:
	COMMON ("movw (%2),%w0 ; movw %w0,(%1)");
      case 3:
	COMMON ("movw (%2),%w0 ; movw %w0,(%1)\n\t"
		"movb 2(%2),%b0 ; movb %b0,2(%1)");
      }
  }
  return to;
}

#undef COMMON


#define __HAVE_ARCH_MEMMOVE
extern inline void *memmove (void *dest, const void *src, size_t n);
extern inline void *
memmove (void *dest, const void *src, size_t n)
{
  register void *tmp = (void *) dest;
  if (dest < src)
    __asm__ __volatile__ ("cld\n\t" "rep\n\t" "movsb":	/* no output */
			  :"c" (n), "S" (src), "D" (tmp):"cx", "si", "di",
			  "memory");
  else
__asm__ __volatile__ ("std\n\t" "rep\n\t" "movsb\n\t" "cld":	/* no output */
: "c" (n), "S" (n - 1 + (const char *) src), "D" (n - 1 + (char *) tmp):"cx", "si", "di", "memory");
  return dest;
}

#define memcmp __builtin_memcmp

#define __HAVE_ARCH_MEMCHR
extern inline void *memchr (const void *cs, int c, size_t count);
extern inline void *
memchr (const void *cs, int c, size_t count)
{
  register void *__res;
  if (!count)
    return NULL;
  __asm__ __volatile__ ("cld\n\t"
			"repne\n\t"
			"scasb\n\t"
			"je 1f\n\t"
			"movl $1,%0\n"
			"1:\tdecl %0":"=D" (__res):"a" (c), "D" (cs),
			"c" (count):"cx");
  return __res;
}



#define __HAVE_ARCH_MEMSET
#define memset(s,c,count) \
(__builtin_constant_p(c) ? \
 (__builtin_constant_p(count) ? \
  __memset_cc((s),(c),(count)) : \
  __memset_cg((s),(c),(count))) : \
 (__builtin_constant_p(count) ? \
  __memset_gc((s),(c),(count)) : \
  __memset_gg((s),(c),(count))))




extern inline void *__memset_cg (void *s, char c, size_t count);
extern inline void *
__memset_cg (void *s, char c, size_t count)
{
  int tmp2;
  register void *tmp = (void *) s;
  __asm__ __volatile__ ("shrl $1,%%ecx\n\t"
			"rep\n\t"
			"stosw\n\t"
			"jnc 1f\n\t"
			"movb %%al,(%%edi)\n"
			"1:":"=c" (tmp2), "=D" (tmp):"c" (count), "D" (tmp),
			"a" (0x0101U * (unsigned char) c):"memory");
  return s;
}

extern inline void *__memset_gg (void *s, char c, size_t count);
extern inline void *
__memset_gg (void *s, char c, size_t count)
{
  register void *tmp = (void *) s;
  int tmp2;
  __asm__ __volatile__ ("movb %%al,%%ah\n\t"
			"shrl $1,%%ecx\n\t"
			"rep\n\t"
			"stosw\n\t"
			"jnc 1f\n\t"
			"movb %%al,(%%edi)\n"
			"1:":"=c" (tmp2), "=D" (tmp):"c" (count), "D" (tmp),
			"a" (c):"memory");
  return s;
}

/*
 * This non-rep routines are not much faster (slower for small strings)
 * but they allows better register allocation
 */
#define COMMON(x) \
__asm__ __volatile__ ( \
	"\n.align 4\n" \
	"1:\tmovl %k2,(%k0)\n\t" \
	"addl $4,%k0\n\t" \
	"decl %k1\n\t" \
	"jnz 1b\n" \
	x \
	:"=r" (tmp), "=r" (dummy) \
	:"q" ((unsigned) pattern),  "0"  (tmp), "1" (count/4) \
	:"memory"); \
return s;

extern inline void *__memset_cc (void *s, unsigned long pattern,
				 size_t count);
extern inline void *
__memset_cc (void *s, unsigned long pattern, size_t count)
{
  pattern = ((unsigned char) pattern) * 0x01010101UL;
  if (count < 24)
    {
      /*Handle small values manualy since they are incredibly slow */

      if (count >= 4)
	*(unsigned long *) s = pattern;
      if (count >= 8)
	((unsigned long *) s)[1] = pattern;
      if (count >= 12)
	((unsigned long *) s)[2] = pattern;
      if (count >= 16)
	((unsigned long *) s)[3] = pattern;
      if (count >= 20)
	((unsigned long *) s)[4] = pattern;
      switch ((unsigned int) (count % 4))
	{
	case 3:
	  ((unsigned short *) s)[count / 2 - 1] = pattern;
	  ((unsigned char *) s)[count - 1] = pattern;
	  return s;
	case 2:
	  ((unsigned short *) s)[count / 2 - 1] = pattern;
	  return s;
	case 1:
	  ((unsigned char *) s)[count - 1] = pattern;
	case 0:
	  return s;
	}
    }
  else
    {
      register void *tmp = (void *) s;
      register int dummy;
      switch ((unsigned int) (count % 4))
	{
	case 0:
	  COMMON ("");
	case 1:
	  COMMON ("movb %b2,(%0)");
	case 2:
	  COMMON ("movw %w2,(%0)");
	case 3:
	  COMMON ("movw %w2,(%0)\n\tmovb %b2,2(%0)");
	}
    }
  return s;
}

extern inline void *__memset_gc (void *s, unsigned long pattern,
				 size_t count);
extern inline void *
__memset_gc (void *s, unsigned long pattern, size_t count)
{
  if (count < 4)
    {
      if (count > 1)
      __asm__ ("movb %b0,%h0\n\t": "=q" (pattern):"0" ((unsigned)
	     pattern));
      switch ((unsigned int) (count))
	{
	case 3:
	  ((unsigned short *) s)[0] = pattern;
	  ((unsigned char *) s)[2] = pattern;
	  return s;
	case 2:
	  *((unsigned short *) s) = pattern;
	  return s;
	case 1:
	  *(unsigned char *) s = pattern;
	case 0:
	  return s;
	}
    }

__asm__ ("movb %b0,%h0\n\t" "pushw %w0\n\t" "shll $16,%k0\n\t" "popw %w0\n": "=q" (pattern):"0" ((unsigned)
       pattern));

  if (count < 24)
    {
      /*Handle small values manualy since they are incredibly slow */

      *(unsigned long *) s = pattern;
      if (count >= 8)
	((unsigned long *) s)[1] = pattern;
      if (count >= 12)
	((unsigned long *) s)[2] = pattern;
      if (count >= 16)
	((unsigned long *) s)[3] = pattern;
      if (count >= 20)
	((unsigned long *) s)[4] = pattern;
      switch ((unsigned int) (count % 4))
	{
	case 3:
	  ((unsigned short *) s)[count / 2 - 1] = pattern;
	  ((unsigned char *) s)[count - 1] = pattern;
	  return s;
	case 2:
	  ((unsigned short *) s)[count / 2 - 1] = pattern;
	  return s;
	case 1:
	  ((unsigned char *) s)[count - 1] = pattern;
	case 0:
	  return s;
	}
    }
  else
    {
      register void *tmp = (void *) s;
      register int dummy;
      switch ((unsigned int) (count % 4))
	{
	case 0:
	  COMMON ("");
	case 1:
	  COMMON ("movb %b2,(%0)");
	case 2:
	  COMMON ("movw %w2,(%0)");
	case 3:
	  COMMON ("movw %w2,(%0)\n\tmovb %b2,2(%0)");
	}
    }
  return s;
}

#undef COMMON


/*
 * find the first occurrence of byte 'c', or 1 past the area if none
 */
#define __HAVE_ARCH_MEMSCAN
extern inline void *memscan (void *addr, int c, size_t size);
extern inline void *
memscan (void *addr, int c, size_t size)
{
  if (!size)
    return addr;
  __asm__ __volatile__ ("cld\n\t"
			"repnz; scasb\n\t"
			"jnz 1f\n\t"
			"dec %%edi\n\t"
			"1:":"=D" (addr), "=c" (size):"0" (addr), "1" (size),
			"a" (c));
  return addr;
}

#define memset_long(x,y,z) __memset_long(x,y,z)

extern inline void *__memset_long (void *s, char c, size_t count);
extern inline void *
__memset_long (void *s, char c, size_t count)
{
  register unsigned int fill = c;
  register void *tmp = (void *) s;
  if (count >= 7)
    {
      register int c = (-(int) s) & 3;
/*__asm__ __volatile__ ("movb %b0,%h0":"=r"(fill):"r"(fill));*/
      fill |= fill << 8;
      count -= c;
      fill |= fill << 16;
      __asm__ __volatile__ ("rep\n\tstosb":"=c" (c), "=D" (tmp):"c" (c),
			    "D" (tmp), "a" (fill):"memory");
      c = count >> 2;
      __asm__ __volatile__ ("rep\n\tstosl":"=c" (c), "=D" (tmp):"c" (c),
			    "D" (tmp), "a" (fill):"memory");
      count &= 3;
    }
  __asm__ __volatile__ ("rep\n\tstosb":"=c" (count), "=D" (tmp):"c" (count),
			"D" (tmp), "a" ((char) fill):"memory");
  return s;
}
#endif
#endif
#endif
