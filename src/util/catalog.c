#include <config.h>
#ifndef _plan9_
#include <string.h>
#ifndef NO_MALLOC_H
#include <malloc.h>
#endif
#include <string.h>
#else
#include <u.h>
#include <libc.h>
#endif
#include <catalog.h>
#include <misc-f.h>
/* Well, just simple implementation of unbalanced trees in combination
 * of small hash table. I am lazy :) but should be OK for my purposes
 *
 * This function is used for both-lookups and adds into table (since most of
 * code is the same, depends whether newvalue is NULL. If newvalue is nonNULL,
 * new variable is added into table, if name is not present here or value is
 * changed othewise.
 */
static char *
find_variable (catalog_t * context, CONST char *name, CONST char *newvalue)
{
  int r = 0;
  int hash = (int) strlen (name);
  struct varnames *current, *last, *newp;
  hash =
    ((unsigned char) (name[0]) + (unsigned char) (name[hash - 1]) +
     hash) % (unsigned int) CHASHMAX;
  current = last = context->root[hash];
  while (current != NULL)
    {
      last = current;
      r = strcmp (current->name, name);
      if (!r)
	{
	  if (newvalue != NULL)	/*overwrite value */
	    {
	      free (current->value);
	      current->value = mystrdup (newvalue);
	    }
	  return (current->value);
	}
      if (r > 0)
	current = current->left;
      else
	current = current->right;
    }
  /*Entry is new */
  if (newvalue == NULL)
    return (NULL);
  newp = (struct varnames *) calloc (1, sizeof (struct varnames));
  newp->name = mystrdup (name);
  newp->value = mystrdup (newvalue);
  /*FIXME. Should take a care to full memory */
  newp->left = NULL;
  newp->right = NULL;
  if (last == NULL)
    {
      context->root[hash] = newp;
    }
  else
    {
      if (r > 0)
	last->left = newp;
      else
	last->right = newp;
    }
  return (newp->value);
}

/*
 * free memory used by node and its sons
 */
static void
free_node (struct varnames *node)
{
  while (node != NULL)
    {
      struct varnames *nextnode;
      free_node (node->left);
      nextnode = node->right;
      free (node->name);
      free (node->value);
      free (node);
      node = nextnode;
    }
}

/*
 * free catalog
 */
void
free_catalog (catalog_t * context)
{
  int i;
  for (i = 0; i < CHASHMAX; i++)
    {
      free_node (context->root[i]);
      context->root[i] = NULL;
    }
  free (context);
}
static catalog_t *
alloc_catalog (void)
{
  int i;
  catalog_t *c;
  c = (catalog_t *) calloc (1, sizeof (catalog_t));
  if (c == NULL)
    return NULL;
  for (i = 0; i < CHASHMAX; i++)
    c->root[i] = NULL;
  return c;
}

/*
 * Parse an catalog file and save values into memory
 */
#define seterror(text) sprintf(errort,"line %i:%s",line,text),*error=errort
catalog_t *
load_catalog (xio_file f, CONST char **error)
{
  int i;
  int line = 1;
  int size;
  int c;
  catalog_t *catalog = alloc_catalog ();
  static char errort[40];
  char name[1024];
  char value[1024];
  if (catalog == NULL)
    {
      *error = "Out of memory";
    }
  if (f == NULL)
    {
      *error = "File could not be opended";
      free_catalog (catalog);
      return NULL;
    }
  /* Just very simple parsing loop of format 
   * [blanks]name[blanks]"value"[blanks]
   * Blanks should be comments using # or space, newline, \r and tabulator
   * Value shoud contain and \ seqences where \\ means \ and
   * \[something] means something. Should be used for character "
   */
  while (!xio_feof (f))
    {

      do
	{
	  c = xio_getc (f);
	  if (c == '\n')
	    line++;
	  if (c == '#')
	    {
	      while ((c = xio_getc (f)) != '\n' && c != XIO_EOF);
	      line++;
	    }
	}
      while (c == ' ' || c == '\n' || c == '\r' || c == '\t');
      /*Skip blanks */
      if (c == XIO_EOF)
	{
	  if (xio_feof (f))
	    break;
	  free_catalog (catalog);
	  seterror ("read error");
	  xio_close (f);
	  return NULL;
	}
      i = 0;

      /*read name */
      do
	{
	  name[i] = c;
	  i++;
	  c = xio_getc (f);
	  if (c == '\n')
	    line++;
	  if (i == 1024)
	    {
	      seterror ("Name is too long(1024 or more characters)");
	      free_catalog (catalog);
	      xio_close (f);
	      return NULL;
	    }
	}
      while (c != '\n' && c != ' ' && c != '\t' && c != XIO_EOF);

      /*Skip blanks */
      while (c == ' ' || c == '\n' || c == '\r' || c == '\t')
	{
	  c = xio_getc (f);
	  if (c == '\n')
	    line++;
	  if (c == '#')
	    {
	      while ((c = xio_getc (f)) != '\n' && c != XIO_EOF);
	      line++;
	    }
	}

      /*Skip blanks */
      if (c == XIO_EOF)
	{
	  if (xio_feof (f))
	    seterror ("Inexpected end of file after name field");
	  else
	    seterror ("read error");
	  free_catalog (catalog);
	  xio_close (f);
	  return NULL;
	}
      name[i] = 0;
      if (c != '"')
	{
	  seterror ("Begin of value field expected (\")");
	  free_catalog (catalog);
	  xio_close (f);
	  return NULL;
	}
      c = xio_getc (f);
      if (c == '\n')
	line++;
      i = 0;

      size = 0;
      do
	{
	  if (c == '\\')
	    value[i] = xio_getc (f);
	  else
	    value[i] = c;
	  i++;
	  c = xio_getc (f);
	  if (c == '\n')
	    line++, size = 0;
	  if (size == 40 && c != '"')
	    {
	      fprintf (stderr, "Warning - too long text at line %i\n", line);
	    }
	  size++;
	  if (i == 1024)
	    {
	      seterror ("Value is too long(1024 or more characters)");
	      free_catalog (catalog);
	      xio_close (f);
	      return NULL;
	    }
	}
      while (c != '"' && c != XIO_EOF);

      if (c == XIO_EOF)
	{
	  seterror ("Inexpeced end of file in value filed");
	  free_catalog (catalog);
	  xio_close (f);
	  return NULL;
	}
      value[i] = 0;
      find_variable (catalog, name, value);
    }				/*while */
  xio_close (f);
  return (catalog);
}				/*load_catalog */

char *
find_text (catalog_t * catalog, CONST char *name)
{
  return (find_variable (catalog, name, NULL));
}
