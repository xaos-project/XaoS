#ifdef _plan9_
#include <u.h>
#include <libc.h>
#else
#include <stdlib.h>
#endif
#include <ctype.h>
#include <config.h>
#include <xio.h>
#include <xshl.h>
struct helpstatus
{
  int eol;
  xio_file file;
};
static int
gethelp (void *userdata)
{
  struct helpstatus *s = (struct helpstatus *) userdata;
  int c = xio_getc (s->file);
  if (c == '\r')
    return (gethelp (userdata));
  if (c < 0 || c > 255)
    return 0;
  if (c == '%' && s->eol)
    return 0;
  s->eol = (c == '\n');
  return c;
}
struct xshl_line *
help_make (CONST char *command, int getwidth (void *, int flags,
					      CONST char *text), int width,
	   int smallheight, int bigheight)
{
  struct helpstatus *s = (struct helpstatus *) malloc (sizeof (*s));
  struct xshl_line *line;
  s->file = xio_gethelp ();
  if (s->file == XIO_FAILED)
    {
      free (s);
      return 0;
    }
  s->eol = 1;
  while (1)
    {
      int c;
      c = xio_getc (s->file);
      if (c == '%' && s->eol)
	{
	  c = xio_getc (s->file);
	  do
	    {
	      int i = 0;
	      i = 0;
	      while (command[i] && c == command[i])
		{
		  c = xio_getc (s->file);
		  i++;
		}
	      if (!command[i])
		{
		  if (isspace (c))
		    {
		      while (c != '\n' && !xio_feof (s->file))
			c = xio_getc (s->file);
		      line =
			xshl_interpret (s, gethelp, width, getwidth, 0,
					smallheight, bigheight);
		      xio_close (s->file);
		      free (s);
		      return (line);
		    }
		}
	      else
		{
		  while (c != '\n' && c != ' ' && !xio_feof (s->file))
		    c = xio_getc (s->file);
		  if (c == ' ')
		    while (c == ' ')
		      c = xio_getc (s->file);
		}
	    }
	  while (c != '\n' && !xio_feof (s->file));
	}			/*c==% */
      s->eol = (c == '\n');
      if (xio_feof (s->file))
	{
	  xio_close (s->file);
	  return NULL;
	}
    }				/*while 1 */
}

#if _NEVER_
void
help_print (char *name, int skip, int width)
{
  struct xshl_line *l;
  l = help_make (name, xshl_textlen, width, 1, 1);
  if (l == NULL)
    {
      printf ("Help not available!\n");
      return;
    }
  xshl_print (skip, l);
  xshl_free (l);
}
#endif
