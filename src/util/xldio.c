/* This file contain long double I/O routines for Windows (because Windows API
   don't support long double at all.

   They don't work on other architectures. So be curefull. */


/* This source comes from the DJGPP runtime library. It has been hacked
   to work with XaoS */
/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1997 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <config.h>
#include <stdlib.h>
#include <ctype.h>
#include <xio.h>
/*#include <libc/unconst.h>*/
#include <sys/types.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <xldio.h>
#ifdef USE_XLDIO

static long double powten[] = {
  1e1L, 1e2L, 1e4L, 1e8L, 1e16L, 1e32L, 1e64L, 1e128L, 1e256L,
  1e512L, 1e1024L, 1e2048L, 1e4096L
};

long double
x_strtold (CONST char *s, CONST char **sret)
{
  long double r;		/* result */
  int e, ne;			/* exponent */
  int sign;			/* +- 1.0 */
  int esign;
  int flags = 0;
  int l2powm1;

  r = 0.0L;
  sign = 1;
  e = ne = 0;
  esign = 1;

  while (*s && isspace (*s))
    s++;

  if (*s == '+')
    s++;
  else if (*s == '-')
    {
      sign = -1;
      s++;
    }

  while ((*s >= '0') && (*s <= '9'))
    {
      flags |= 1;
      r *= 10.0L;
      r += *s - '0';
      s++;
    }

  if (*s == '.')
    {
      s++;
      while ((*s >= '0') && (*s <= '9'))
	{
	  flags |= 2;
	  r *= 10.0L;
	  r += *s - '0';
	  s++;
	  ne++;
	}
    }
  if (flags == 0)
    {
      if (sret)
	*sret = /*unconst(s, char *) */ s;
      return 0.0L;
    }

  if ((*s == 'e') || (*s == 'E'))
    {
      s++;
      if (*s == '+')
	s++;
      else if (*s == '-')
	{
	  s++;
	  esign = -1;
	}
      while ((*s >= '0') && (*s <= '9'))
	{
	  e *= 10;
	  e += *s - '0';
	  s++;
	}
    }
  if (esign < 0)
    {
      esign = -esign;
      e = -e;
    }
  e = e - ne;
  if (e < -4096)
    {
      /* possibly subnormal number, 10^e would overflow */
      r *= 1.0e-2048L;
      e += 2048;
    }
  if (e < 0)
    {
      e = -e;
      esign = -esign;
    }
  if (e >= 8192)
    e = 8191;
  if (e)
    {
      long double d = 1.0L;
      l2powm1 = 0;
      while (e)
	{
	  if (e & 1)
	    d *= powten[l2powm1];
	  e >>= 1;
	  l2powm1++;
	}
      if (esign > 0)
	r *= d;
      else
	r /= d;
    }
  if (sret)
    *sret = /*unconst(s, char *) */ s;
  return r * sign;
}

#if 0
main ()
{
  printf ("%E", (float) x_strtold ("1.4E15", NULL));
}
#endif


#define MAXEXPLD        4952	/* this includes subnormal numbers */
static int is_nan = 0;
static char decimal = '.';
static long double pten[] = {
  1e1L, 1e2L, 1e4L, 1e8L, 1e16L, 1e32L, 1e64L, 1e128L, 1e256L,
  1e512L, 1e1024L, 1e2048L, 1e4096L
};

static long double ptenneg[] = {
  1e-1L, 1e-2L, 1e-4L, 1e-8L, 1e-16L, 1e-32L, 1e-64L, 1e-128L, 1e-256L,
  1e-512L, 1e-1024L, 1e-2048L, 1e-4096L
};
static inline char
tochar (int n)
{
  if (n >= 9)
    return '9';
  if (n <= 0)
    return '0';
  return n + '0';
}
static inline int
todigit (char c)
{
  if (c <= '0')
    return 0;
  if (c >= '9')
    return 9;
  return c - '0';
}

#define	LONGINT		0x01	/* long integer */
#define	LONGDBL		0x02	/* long double */
#define	SHORTINT	0x04	/* short integer */
#define	ALT		0x08	/* alternate form */
#define	LADJUST		0x10	/* left adjustment */
#define	ZEROPAD		0x20	/* zero (as opposed to blank) pad */
#define	HEXPREFIX	0x40	/* add 0x or 0X prefix */

#define MAXP 4096
#define NP   12
#define P    (4294967296.0L * 4294967296.0L * 2.0L)	/* 2^65 */
static long double INVPREC = P;
static long double PREC = 1.0L / P;
#undef P
/*
 * Defining FAST_LDOUBLE_CONVERSION results in a little bit faster
 * version, which might be less accurate (about 1 bit) for long
 * double. For 'normal' double it doesn't matter.
 */
/* #define FAST_LDOUBLE_CONVERSION */
#if 1
#define modfl mymodfl
inline long double
m_floor (long double x)
{
  register long double __value;
  volatile unsigned short int __cw, __cwtmp;

  asm volatile ("fnstcw %0":"=m" (__cw));
  __cwtmp = (__cw & 0xf3ff) | 0x0400;	/* rounding down */
  asm volatile ("fldcw %0"::"m" (__cwtmp));
  asm volatile ("frndint":"=t" (__value):"0" (x));
  asm volatile ("fldcw %0"::"m" (__cw));

  return __value;

}
static inline long double
mymodfl (long double x, long double *pint)
{
  /*int p=(int) x; */
  long double p = m_floor (x);
  long double frac = x - p;
  if (x < 0)
    p = p + 1, frac = frac - 1;
  *pint = p;
  return frac;
}
#endif
static char *
exponentl (char *p, int expv, unsigned char fmtch)
{
  char *t;
  char expbuf[MAXEXPLD];

  *p++ = fmtch;
  if (expv < 0)
    {
      expv = -expv;
      *p++ = '-';
    }
  else
    *p++ = '+';
  t = expbuf + MAXEXPLD;
  if (expv > 9)
    {
      do
	{
	  *--t = tochar (expv % 10);
	}
      while ((expv /= 10) > 9);
      *--t = tochar (expv);
      for (; t < expbuf + MAXEXPLD; *p++ = *t++);
    }
  else
    {
      *p++ = '0';
      *p++ = tochar (expv);
    }
  return p;
}

static int
isspeciall (long double d, char *bufp)
{
  struct IEEExp
  {
    unsigned manl:32;
    unsigned manh:32;
    unsigned exp:15;
    unsigned sign:1;
  }
   *ip = (struct IEEExp *) &d;

  is_nan = 0;			/* don't assume the static is 0 (emacs) */
  if (ip->exp != 0x7fff)
    return (0);
  if ((ip->manh & 0x7fffffff) || ip->manl)
    {
      strcpy (bufp, "NaN");
      is_nan = ip->sign ? -1 : 1;	/* kludge: we don't need the sign,  it's not nice
					   but it should work */
    }
  else
    (void) strcpy (bufp, "Inf");
  return (3);
}
static char *
my_roundl (long double fract, int *expv, char *start, char *end, char ch,
	   char *signp)
{
  long double tmp;

  if (fract)
    {
      if (fract == 0.5L)
	{
	  char *e = end;
	  if (*e == '.')
	    e--;
	  if (*e == '0' || *e == '2' || *e == '4' || *e == '6' || *e == '8')
	    {
	      tmp = 3.0;
	      goto start;
	    }
	}
      (void) modfl (fract * 10.0L, &tmp);
    }
  else
    tmp = todigit (ch);
start:
  if (tmp > 4)
    for (;; --end)
      {
	if (*end == decimal)
	  --end;
	if (++*end <= '9')
	  break;
	*end = '0';
	if (end == start)
	  {
	    if (expv)
	      {			/* e/E; increment exponent */
		*end = '1';
		++*expv;
	      }
	    else
	      {			/* f; add extra digit */
		*--end = '1';
		--start;
	      }
	    break;
	  }
      }
  /* ``"%.3f", (double)-0.0004'' gives you a negative 0. */
  else if (*signp == '-')
    for (;; --end)
      {
	if (*end == decimal)
	  --end;
	if (*end != '0')
	  break;
	if (end == start)
	  *signp = 0;
      }
  return start;
}


static int
cvtl (long double number, int prec, int flags, char *signp,
      unsigned char fmtch, char *startp, char *endp)
{
  char *p, *t;
  long double fract = 0;
  int dotrim, expcnt, gformat;
  int doextradps = 0;		/* Do extra decimal places if the precision needs it */
  int doingzero = 0;		/* We're displaying 0.0 */
  long double integer, tmp;

  if ((expcnt = isspeciall (number, startp)))
    return (expcnt);

  dotrim = expcnt = gformat = 0;
  /* fract = modfl(number, &integer); */
  integer = number;

  /* get an extra slot for rounding. */
  t = ++startp;

  p = endp - 1;
  if (integer)
    {
      int i, lp = NP, pt = MAXP;
#ifndef FAST_LDOUBLE_CONVERSION
      long double oint = integer, dd = 1.0L;
#endif
      if (integer > INVPREC)
	{
	  integer *= PREC;
	  while (lp >= 0)
	    {
	      if (integer >= pten[lp])
		{
		  expcnt += pt;
		  integer *= ptenneg[lp];
#ifndef FAST_LDOUBLE_CONVERSION
		  dd *= pten[lp];
#endif
		}
	      pt >>= 1;
	      lp--;
	    }
#ifndef FAST_LDOUBLE_CONVERSION
	  integer = oint / dd;
#else
	  integer *= INVPREC;
#endif
	}
      /*
       * Do we really need this ?
       */
      for (i = 0; i < expcnt; i++)
	*p-- = '0';
    }
  number = integer;
  fract = modfl (number, &integer);
  /* If integer is zero then we need to look at where the sig figs are */
  if (integer < 1)
    {
      /* If fract is zero the zero before the decimal point is a sig fig */
      if (fract == 0.0)
	doingzero = 1;
      /* If fract is non-zero all sig figs are in fractional part */
      else
	doextradps = 1;
    }
  /*
   * get integer portion of number; put into the end of the buffer; the
   * .01 is added for modf(356.0 / 10, &integer) returning .59999999...
   */
  for (; integer; ++expcnt)
    {
      tmp = modfl (integer * 0.1L, &integer);
      *p-- = tochar ((int) ((tmp + .01L) * 10));
    }
  switch (fmtch)
    {
    case 'f':
      /* reverse integer into beginning of buffer */
      if (expcnt)
	for (; ++p < endp; *t++ = *p);
      else
	*t++ = '0';
      /*
       * if precision required or alternate flag set, add in a
       * decimal point.
       */
      if (prec || flags & ALT)
	*t++ = decimal;
      /* if requires more precision and some fraction left */
      if (fract)
	{
	  if (prec)
	    do
	      {
		fract = modfl (fract * 10.0L, &tmp);
		*t++ = tochar ((int) tmp);
	      }
	    while (--prec && fract);
	  if (fract)
	    startp = my_roundl (fract, (int *) NULL, startp,
				t - 1, (char) 0, signp);
	}
      for (; prec--; *t++ = '0');
      break;
    case 'e':
    case 'E':
    eformat:
      if (expcnt)
	{
	  *t++ = *++p;
	  if (prec || flags & ALT)
	    *t++ = decimal;
	  /* if requires more precision and some integer left */
	  for (; prec && ++p < endp; --prec)
	    *t++ = *p;
	  /*
	   * if done precision and more of the integer component,
	   * round using it; adjust fract so we don't re-round
	   * later.
	   */
	  if (!prec && ++p < endp)
	    {
	      fract = 0;
	      startp = my_roundl ((long double) 0.0L, &expcnt,
				  startp, t - 1, *p, signp);
	    }
	  /* adjust expcnt for digit in front of decimal */
	  --expcnt;
	}
      /* until first fractional digit, decrement exponent */
      else if (fract)
	{
	  int lp = NP, pt = MAXP;
#ifndef FAST_LDOUBLE_CONVERSION
	  long double ofract = fract, dd = 1.0L;
#endif
	  expcnt = -1;
	  if (fract < PREC)
	    {
	      fract *= INVPREC;
	      while (lp >= 0)
		{
		  if (fract <= ptenneg[lp])
		    {
		      expcnt -= pt;
		      fract *= pten[lp];
#ifndef FAST_LDOUBLE_CONVERSION
		      dd *= pten[lp];
#endif
		    }
		  pt >>= 1;
		  lp--;
		}
#ifndef FAST_LDOUBLE_CONVERSION
	      fract = ofract * dd;
#else
	      fract *= PREC;
#endif
	    }
	  /* adjust expcnt for digit in front of decimal */
	  for ( /* expcnt = -1 */ ;; --expcnt)
	    {
	      fract = modfl (fract * 10.0L, &tmp);
	      if (tmp)
		break;
	    }
	  *t++ = tochar ((int) tmp);
	  if (prec || flags & ALT)
	    *t++ = decimal;
	}
      else
	{
	  *t++ = '0';
	  if (prec || flags & ALT)
	    *t++ = decimal;
	}
      /* if requires more precision and some fraction left */
      if (fract)
	{
	  if (prec)
	    do
	      {
		fract = modfl (fract * 10.0L, &tmp);
		*t++ = tochar ((int) tmp);
	      }
	    while (--prec && fract);
	  if (fract)
	    startp =
	      my_roundl (fract, &expcnt, startp, t - 1, (char) 0, signp);
	}
      /* if requires more precision */
      for (; prec--; *t++ = '0');

      /* unless alternate flag, trim any g/G format trailing 0's */
      if (gformat && !(flags & ALT))
	{
	  while (t > startp && *--t == '0');
	  if (*t == decimal)
	    --t;
	  ++t;
	}
      t = exponentl (t, expcnt, fmtch);
      break;
    case 'g':
    case 'G':
      if (prec)
	{
	  /* If doing zero and precision is greater than 0 count the
	   * 0 before the decimal place */
	  if (doingzero)
	    --prec;
	}
      else
	{
	  /* a precision of 0 is treated as precision of 1 unless doing zero */
	  if (!doingzero)
	    ++prec;
	}
      /*
       * ``The style used depends on the value converted; style e
       * will be used only if the exponent resulting from the
       * conversion is less than -4 or greater than the precision.''
       *  -- ANSI X3J11
       */
      if (expcnt > prec || (!expcnt && fract && fract < .0001))
	{
	  /*
	   * g/G format counts "significant digits, not digits of
	   * precision; for the e/E format, this just causes an
	   * off-by-one problem, i.e. g/G considers the digit
	   * before the decimal point significant and e/E doesn't
	   * count it as precision.
	   */
	  --prec;
	  fmtch -= 2;		/* G->E, g->e */
	  gformat = 1;
	  goto eformat;
	}
      /*
       * reverse integer into beginning of buffer,
       * note, decrement precision
       */
      if (expcnt)
	for (; ++p < endp; *t++ = *p, --prec);
      else
	*t++ = '0';
      /*
       * if precision required or alternate flag set, add in a
       * decimal point.  If no digits yet, add in leading 0.
       */
      if (prec || flags & ALT)
	{
	  dotrim = 1;
	  *t++ = decimal;
	}
      else
	dotrim = 0;
      /* if requires more precision and some fraction left */
      while (prec && fract)
	{
	  fract = modfl (fract * 10.0L, &tmp);
	  *t++ = tochar ((int) tmp);
	  /* If we're not adding 0s
	   * or we are but they're sig figs:
	   * decrement the precision */
	  if ((doextradps != 1) || ((int) tmp != 0))
	    {
	      doextradps = 0;
	      prec--;
	    }
	}
      if (fract)
	startp =
	  my_roundl (fract, (int *) NULL, startp, t - 1, (char) 0, signp);
      /* alternate format, adds 0's for precision, else trim 0's */
      if (flags & ALT)
	for (; prec--; *t++ = '0');
      else if (dotrim)
	{
	  while (t > startp && *--t == '0');
	  if (*t != decimal)
	    ++t;
	}
    }
  return t - startp;
}

#if 0
main ()
{
  static char buf[4096];
  int i;
  cvtl (0.00000000000000000005, 4, 0, &softsign, 'G', buf,
	buf + sizeof (buf) - 1);
  printf ("%s\n", buf + 1);
  printf ("%.30LG\n", (long double) 234236723234234231235324.47239L);
}

#endif
void
x_ldout (long double param, int prec, xio_file stream)
{
  static char buf[4095];
  char softsign = 0;
  int l;
  if (param < 0)
    xio_putc ('-', stream), param = -param;
  l = cvtl (param, prec, 0, &softsign, 'G', buf, buf + sizeof (buf));
  /*printf("a:%s %i\n",buf+1, prec); */
  buf[l + 2] = 0;
  l = strlen (buf + 1);
  if (buf[l] == '.')
    buf[l] = 0;
  /*printf("b:%s %i\n",buf+1, prec); */
  xio_puts (buf + 1, stream);
}
#endif
