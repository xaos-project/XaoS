#ifndef _plan9_
#include <string.h>
#if defined(__EMX__) || defined(__APPLE__)
#include <sys/types.h>
#endif
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#else
#include <u.h>
#include <libc.h>
#endif
#include <config.h>
#include <filter.h>
#include <fractal.h>
#include <ui_helper.h>
#include <misc-f.h>
#ifdef _WIN32
#define strcmp stricmp
#endif
#ifdef DJGPP
#define strcmp stricmp
#endif

/* We reserve character 01 to application directory so we can easily refer to data files */
char *xio_appdir;
char *xio_homedir;

char *
xio_fixpath (CONST char *c)
{
  if (c[0] == '~')
    {
      char *c1 = (char *) malloc (strlen (c) + strlen (xio_homedir) + 5);
      sprintf (c1, "%s%s", xio_homedir, c + 1);
      return c1;
    }
  if (c[0] == '\01')
    {
      char *c1 = (char *) malloc (strlen (c) + strlen (xio_appdir) + 5);
      sprintf (c1, "%s%s", xio_appdir, c + 1);
      return c1;
    }
  return mystrdup (c);
}

int
xio_getfiles (xio_constpath path1, char ***names, char ***dirs, int *nnames2,
	      int *ndirs2)
{
#ifdef _plan9_
  *nnames2 = *ndirs2 = 0;
#else
  char *path = xio_fixpath (path1);
  int maxnames = 200, maxdirs = 200;
  int nnames = 0, ndirs = 0;
  DIR *dir = opendir (path);
  struct stat buf;
  char buff[4096];
  int pathlen;
  struct dirent *e;
  if (dir == NULL)
    return 0;
  *nnames2 = 0;
  *ndirs2 = 0;
  e = readdir (dir);
  strcpy (buff, path);
  pathlen = (int) strlen (path);
  if (buff[pathlen - 1] != XIO_PATHSEP)
    buff[pathlen] = XIO_PATHSEP;
  else
    pathlen--;
  *names = (char **) malloc (maxnames * sizeof (**names));
  *dirs = (char **) malloc (maxdirs * sizeof (**dirs));
  free (path);
  while (e != NULL)
    {
      char *n = mystrdup (e->d_name);
      strcpy (buff + pathlen + 1, e->d_name);
      stat (buff, &buf);
      if (S_ISDIR (buf.st_mode))
	{
	  if (ndirs == maxdirs)
	    maxdirs *= 2, *dirs =
	      (char **) realloc (*dirs, maxdirs * sizeof (**dirs));
	  (*dirs)[ndirs] = n;
	  ndirs++;
	}
      else
	{
	  if (nnames == maxnames)
	    maxnames *= 2, *names =
	      (char **) realloc (*names, maxnames * sizeof (**names));
	  (*names)[nnames] = n;
	  nnames++;
	}
      e = readdir (dir);
    }
  if (nnames)
    *names = (char **) realloc (*names, nnames * sizeof (**names));
  else
    free (*names), *names = NULL;
  if (ndirs)
    *dirs = (char **) realloc (*dirs, ndirs * sizeof (**dirs));
  else
    free (*dirs), *dirs = NULL;
  *nnames2 = nnames;
  *ndirs2 = ndirs;
  closedir (dir);
  return 1;
#endif
}

xio_path
xio_getdirectory (xio_constpath filename)
{
  int i;
  xio_pathdata directory;
  for (i = (int) strlen (filename); i && filename[i] != '/' &&
       filename[i] != '\\' && filename[i] != XIO_PATHSEP; i--);
  if (filename[i] == '/' || filename[i] == '\\' || filename[i] == XIO_PATHSEP)
    i++;
  directory[i] = 0;
  i--;
  for (; i >= 0; i--)
    directory[i] = filename[i];
  return (mystrdup (directory));
}

xio_path
xio_getfilename (CONST char *basename, CONST char *extension)
{
  static char name[40];
  int nimage = 0;
#ifdef _plan9_
#ifdef _plan9v4_
#define DIRLEN 116
  uchar edir[DIRLEN];
#else
  char edir[DIRLEN];
#endif
#else
  struct stat sb;
#endif
  char *base = xio_fixpath (basename);
  do
    {
      sprintf (name, "%s%i%s", base, nimage++, extension);
    }
#ifndef _plan9_
  while (stat (name, &sb) != -1);
#else
#ifdef _plan9v4_
  while (stat (name, edir, DIRLEN) != -1);
#else
  while (stat (name, edir) != -1);
#endif
#endif
  free (base);
  return (name);
}

xio_file
xio_getrandomexample (xio_path name)
{
#ifndef _plan9_
  static CONST char *CONST paths[] = {	/*Where examples should be located? */
    EXAMPLESPATH,		/*Data path when XaoS is propertly installed */
    "\01" XIO_PATHSEPSTR "examples",
    /*XaoS was started from root of source tree */
    "\01" XIO_PATHSEPSTR ".." XIO_PATHSEPSTR "examples",
    "." XIO_PATHSEPSTR "examples",
    /*XaoS was started from root of source tree */
    ".." XIO_PATHSEPSTR "examples",
    /*XaoS was started from bin directory in source tree */
    XIO_EMPTYPATH,		/*Oops...it's not. Try curent directory */
  };
  int i = -1, p;
  DIR *d = NULL;
  xio_file f;
  struct dirent *dir;
  int n;
  int max = 0;
  char *realpath = NULL;

  for (p = 0; p < (int) (sizeof (paths) / sizeof (char *)) && d == NULL; p++)
    {
      char *pp = xio_fixpath (paths[p]);
      d = opendir (pp);
      free (pp);
      if (d != NULL)
	{
	  realpath = xio_fixpath (paths[p]);
	  max = 800 - (int) strlen (realpath);
	  for (i = 0; (dir = readdir (d)) != NULL; i++)
	    {
	      int s = (int) strlen (dir->d_name);
	      if (s > max || s < 4 || strcmp (".xpf", dir->d_name + s - 4))
		i--;
	      /*free(dir); */
	    }
	  if (!i)
	    {
	      /*uih->errstring = "No *.xpf files found"; */
	      closedir (d);
	      free (realpath);
	      d = NULL;
	      continue;
	    }
	  break;
	}
    }
  if (d == NULL)
    {
      /*uih->errstring = "Can not open examples directory"; */
      return NULL;
    }



  rewinddir (d);
  dir = NULL;
  n = (int) ((number_t) i * rand () / (RAND_MAX + 1.0));

  for (i = 0; i <= n; i++)
    {
      int s;
      do
	{
	  /*if(dir!=NULL) free(dir); */
	  dir = readdir (d);
	  if (dir == NULL)
	    {
	      /*uih->errstring = "Reading of examples directory failed"; */
	      closedir (d);
	      free (realpath);
	      return NULL;
	    }
	  s = (int) strlen (dir->d_name);
	}
      while (s > max || s < 4 || strcmp (".xpf", dir->d_name + s - 4));
    }
  if (dir == NULL)
    {
      /*uih->errstring = "Reading of examples directory failed"; */
      closedir (d);
      free (realpath);
      return NULL;
    }
  strcpy (name, realpath);
  if (name[strlen (name) - 1] != XIO_PATHSEP)
    strcat (name, XIO_PATHSEPSTR);
  strcat (name, dir->d_name);
  closedir (d);
  /*free(dir); */

  f = xio_ropen (name);
  free (realpath);
  return (f);
#endif
}

xio_file
xio_getcatalog (CONST char *name)
{
  static CONST xio_constpath paths[] = {	/*Where catalogs should be located? */
    CATALOGSPATH,		/*Data path when XaoS is propertly installed */
#ifndef _plan9_
    "\01" XIO_PATHSEPSTR "catalogs" XIO_PATHSEPSTR,
    "\01" XIO_PATHSEPSTR ".." XIO_PATHSEPSTR "catalogs" XIO_PATHSEPSTR,
    "." XIO_PATHSEPSTR "catalogs" XIO_PATHSEPSTR,
    /*XaoS was started from root of source tree */
    ".." XIO_PATHSEPSTR "catalogs" XIO_PATHSEPSTR,
    /*XaoS was started from bin directory in source tree */
#else
    "./catalogs/",
    /*XaoS was started from root of source tree */
    "../catalogs/",
    /*XaoS was started from bin directory in source tree */
#endif
    XIO_EMPTYPATH,		/*Oops...it's not. Try curent directory */
  };
  int i;
  xio_file f = XIO_FAILED;
  xio_pathdata tmp;
  for (i = 0; i < (int) (sizeof (paths) / sizeof (char *)) && f == XIO_FAILED;
       i++)
    {
      char *p = xio_fixpath (paths[i]);
      xio_addfname (tmp, p, name);
      free (p);
      f = xio_ropen (tmp);
      if (f == XIO_FAILED)
	{
	  xio_addextension (tmp, ".cat");
	  f = xio_ropen (tmp);
	}
    }
  return (f);
}

xio_file
xio_gethelp (void)
{
  static CONST xio_constpath paths[] = {	/*Where help should be located? */
    HELPPATH,			/*Data path when XaoS is propertly installed */
#ifndef _plan9_
    "\01" XIO_PATHSEPSTR "help" XIO_PATHSEPSTR "xaos.hlp",
    "\01" XIO_PATHSEPSTR ".." XIO_PATHSEPSTR "help" XIO_PATHSEPSTR "xaos.hlp",
    "." XIO_PATHSEPSTR "help" XIO_PATHSEPSTR "xaos.hlp",
    /*XaoS was started from root of source tree */
    ".." XIO_PATHSEPSTR "help" XIO_PATHSEPSTR "xaos.hlp",
    /*XaoS was started from bin directory in source tree */
    "." XIO_PATHSEPSTR "xaos.hlp",
#else
    "./help/xaos.hlp",
    /*XaoS was started from root of source tree */
    "../help/xaos.hlp",
    /*XaoS was started from bin directory in source tree */
    "./xaos.hlp",
#endif
    /*Oops...it's not. Try curent directory */
  };
  int i;
  xio_file f = XIO_FAILED;
  for (i = 0; i < (int) (sizeof (paths) / sizeof (char *)) && f == XIO_FAILED;
       i++)
    {
      char *p = xio_fixpath (paths[i]);
      f = xio_ropen (p);
      free (p);
    }
  return (f);
}

xio_file
xio_gettutorial (CONST char *name, xio_path tmp)
{
  int i;
  xio_file f = XIO_FAILED;
  static CONST xio_constpath paths[] = {	/*Where tutorials should be located? */
    TUTORIALPATH,		/*Data path when XaoS is propertly installed */
#ifndef _plan9_
    "\01" XIO_PATHSEPSTR "tutorial" XIO_PATHSEPSTR,
    "\01" XIO_PATHSEPSTR ".." XIO_PATHSEPSTR "tutorial" XIO_PATHSEPSTR,
    "." XIO_PATHSEPSTR "tutorial" XIO_PATHSEPSTR,	/*XaoS was started from root of source tree */
    ".." XIO_PATHSEPSTR "tutorial" XIO_PATHSEPSTR,	/*XaoS was started from bin directory in source tree */
#else
    "./tutorial/",		/*XaoS was started from root of source tree */
    "../tutorial/",		/*XaoS was started from bin directory in source tree */
#endif
    XIO_EMPTYPATH,		/*Oops...it's not. Try curent directory */
  };

  for (i = 0; i < (int) (sizeof (paths) / sizeof (char *)) && f == XIO_FAILED;
       i++)
    {
      char *p = xio_fixpath (paths[i]);
      xio_addfname (tmp, p, name);
      f = xio_ropen (tmp);
      free (p);
    }
  return (f);
}

int
xio_exist (xio_constpath name)
{
#ifdef _plan9_
  return (0);
#else
  struct stat buf;
  return (!stat (name, &buf));
#endif
}
static int
sputc (int c, xio_file f)
{
  return putc (c, (FILE *) f->data);
}
static int
sputs (CONST char *c, xio_file f)
{
  return fputs (c, (FILE *) f->data);
}
static int
sungetc (int c, xio_file f)
{
  return ungetc (c, (FILE *) f->data);
}
static int
sgetc (xio_file f)
{
  return getc ((FILE *) f->data);
}
static int
sfeof (xio_file f)
{
  return feof ((FILE *) f->data);
}
static int
sflush (xio_file f)
{
  return fflush ((FILE *) f->data);
}
static int
ssclose (xio_file f)
{
  int r = fclose ((FILE *) f->data);
  free (f);
  return r;
}

xio_file
xio_ropen (CONST char *name)
{
  xio_file f = (xio_file) calloc (1, sizeof (*f));
  name = xio_fixpath (name);
  f->data = (void *) fopen (name, "rt");
  /*free (name); */
  if (!f->data)
    {
      free (f);
      return 0;
    }
  f->fclose = ssclose;
  f->xeof = sfeof;
  f->fgetc = sgetc;
  f->fungetc = sungetc;
  return f;
}

xio_file
xio_wopen (CONST char *name)
{
  xio_file f = (xio_file) calloc (1, sizeof (*f));
  name = xio_fixpath (name);
  f->data = (void *) fopen (name, "wt");
  /*free (name); */
  if (!f->data)
    {
      free (f);
      return 0;
    }
  f->fputc = sputc;
  f->fputs = sputs;
  f->fclose = ssclose;
  f->flush = sflush;
  return f;
}

#ifdef DJGPP
#define DRIVES
#endif
#ifdef _WIN32
#define DRIVES
#endif
void
xio_init (CONST char *name)
{
  if (getenv ("HOME"))
    xio_homedir = mystrdup (getenv ("HOME"));
  else
    xio_homedir = mystrdup ("./");
  if (
#ifdef DRIVES
       (((name[0] >= 'a' && name[0] <= 'z')
	 || (name[0] >= 'A' && name[0] <= 'Z')) && name[1] == ':'
	&& (name[2] == '\\' || name[2] == '/')) ||
#endif
       name[0] == '/' || name[0] == '\\' || name[0] == XIO_PATHSEP
       || name[0] == '~')
    {
      char *c = mystrdup (name);
      int i;
      int pos = 0;
      for (i = 0; i < (int) strlen (c); i++)
	if (name[i] == '/' || name[i] == '\\' || name[i] == XIO_PATHSEP)
	  pos = i;
      c[pos] = 0;
      xio_appdir = xio_fixpath (c);
      free (c);
    }
  else
    {
      char buf[4096];
      buf[0] = '.';
      buf[1] = 0;
#ifndef _plan9_
      getcwd (buf, sizeof (buf));
#endif
      xio_appdir = mystrdup (buf);
      {
	char *c = mystrdup (name), *c1;
	int i;
	int pos = 0;
	for (i = 0; i < (int) strlen (c); i++)
	  if (name[i] == '/' || name[i] == '\\' || name[i] == XIO_PATHSEP)
	    pos = i;
	c[pos] = 0;
	c1 = (char *) malloc (strlen (c) + strlen (xio_appdir) + 2);
	sprintf (c1, "%s%s%s", xio_appdir, XIO_PATHSEPSTR, c);
	free (c);
	free (xio_appdir);
	xio_appdir = c1;
      }
    }
}
void
xio_uninit ()
{
  free (xio_appdir);
  free (xio_homedir);
}
