#ifndef _plan9_
#include <string.h>
#include <stdlib.h>
#else
#include <u.h>
#include <libc.h>
#endif

#include <xio.h>
#include <misc-f.h>
struct fr
{
  char *string;
  int pos;
  int allocedsize;
  int stringsize;
};

char *
mystrdup (const char *c)
{
  int l = strlen (c);
  char *d = malloc (l + 1);
  if (!d)
    return NULL;
  memcpy (d, c, l + 1);
  return d;
}

static int
sputc (int c, xio_file s)
{
  struct fr *f = (struct fr *) s->data;
  if (f->pos >= f->allocedsize - 1)
    {
      char *c = (char *) realloc (f->string, f->allocedsize * 2);
      if (!c)
	return XIO_EOF;
      f->string = c;
      f->allocedsize *= 2;
    }
  f->string[f->pos++] = c;
  if (f->pos >= f->stringsize)
    f->string[f->pos] = 0, f->stringsize = f->pos;
  return 0;
}
static int
sputs (CONST char *c, xio_file s)
{
  int l = strlen (c);
  struct fr *f = (struct fr *) s->data;
  while (f->pos + l >= f->allocedsize - 1)
    {
      char *c = (char *) realloc (f->string, f->allocedsize * 2);
      if (!c)
	return XIO_EOF;
      f->string = c;
      f->allocedsize *= 2;
    }
  memcpy (f->string + f->pos, c, l);
  f->pos += l;
  if (f->pos >= f->stringsize)
    f->string[f->pos] = 0, f->stringsize = f->pos;
  return 0;
}
static int
sungetc (int c, xio_file s)
{
  struct fr *f = (struct fr *) s->data;
  f->pos--;
  /*f->string[f->pos]=c; */
  return 0;
}
static int
sgetc (xio_file s)
{
  struct fr *f = (struct fr *) s->data;
  if (f->pos == f->stringsize)
    return XIO_EOF;
  return f->string[f->pos++];
}
static int
sfeof (xio_file s)
{
  struct fr *f = (struct fr *) s->data;
  return (f->pos == f->stringsize);
}
static int
srclose (xio_file s)
{
  struct fr *f = (struct fr *) s->data;
  free (f->string);
  free (f);
  free (s);
  return 0;
}
static int
swclose (xio_file s)
{
  struct fr *f = (struct fr *) s->data;
  f->string = (char *) realloc (f->string, f->stringsize + 1);
  /*free(s);
     free(f); */
  return 0;
}

char *
xio_getstring (xio_file s)
{
  struct fr *f = (struct fr *) s->data;
  char *c = f->string;
  free (f);
  free (s);
  return c;
}

xio_file
xio_strropen (CONST char *string)
{
  xio_file s = (xio_file) calloc (1, sizeof (*s));
  struct fr *f = (struct fr *) calloc (1, sizeof (*f));
  s->data = f;
  f->pos = 0;
  f->string = (char *) string;
  f->stringsize = strlen (string);
  s->fclose = srclose;
  s->xeof = sfeof;
  s->fgetc = sgetc;
  s->fungetc = sungetc;
  return s;
}

#define PAGE 4096
xio_file
xio_strwopen (void)
{
  xio_file s = (xio_file) calloc (1, sizeof (*s));
  struct fr *f = (struct fr *) calloc (1, sizeof (*f));
  s->data = f;
  f->pos = 0;
  f->string = (char *) malloc (PAGE);
  f->allocedsize = PAGE;
  f->stringsize = 0;
  s->fputc = sputc;
  s->fputs = sputs;
  s->fclose = swclose;
  s->flush = NULL;
  return s;
}
