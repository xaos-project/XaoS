/*
** xstdio_mac.c
**
** Written by Dominic Mazzoni
** based on xstdio.c
**
** Uses Apple Computer's MoreFiles sample code
**
** Directory listings are functional, but not mac-like.
** Going up directories currently works, but not well.
*/

#include <config.h>
#include <fconfig.h>
#include <string.h>
#include <unix.h>
#include <stdlib.h>
#include <unistd.h>
#include <filter.h>
#include <fractal.h>
#include <ui_helper.h>
#include <misc-f.h>
#include <xio.h>

#include <Files.h>

// Apple Computer MoreFiles sample code
#include "FullPath.h"
#include "IterateD.h"

// Case insensitive filenames
#define strcmp stricmp

/* We reserve character 01 to application directory so we can easily refer to data files */
char *xio_appdir;
char *xio_homedir;

char *
xio_fixpath (CONST char *c)
{
  char *c2 = (char *)malloc(strlen(c) + strlen(xio_homedir) + strlen(xio_appdir) + 5);
  
  // replace ~ with home directory
  if (c[0] == '~')
  {
    if (c[1] == ':' && xio_appdir[strlen(xio_homedir)-1]==':')
      c++;
    sprintf (c2, "%s%s", xio_homedir, c + 1);
  }
  // replace \01 with app directory
  else if (c[0] == '\01')
  {
    if (c[1] == ':' && xio_appdir[strlen(xio_appdir)-1]==':')
      c++;
    sprintf (c2, "%s%s", xio_appdir, c + 1);
    return c2;
  }
  else
    strcpy(c2, c);

  // make :: function as "back a directory"
  while(strstr(c2,"::")) {
    char *d = strstr(c2,"::");
    char *e = d+2;

    while (d > c && d[-1]!=':')
      d--;
    
    while(e[-1])
      *d++ = *e++;
  }

  return c2;
}

typedef struct dirlist {
  char **names;
  char **dirs;
  int nnames;
  int ndirs;
} dirlist;

const int xstdio_mac_maxnames = 200;
const int xstdio_mac_maxdirs = 200;

pascal void myIterateFn (const CInfoPBRec * const cpbPtr,
											   Boolean *quitFlag,
											   void *myDataPtr)
{
  dirlist *list = (dirlist *)myDataPtr;
  int x;
  int len;
  unsigned char *name;
  
  if (cpbPtr->dirInfo.ioFlAttrib & 16) {
    // it's a directory
    
    if (list->ndirs == xstdio_mac_maxdirs)
      return;
    
    name = (unsigned char *)(cpbPtr->dirInfo.ioNamePtr);
    len = name[0];

    list->dirs[list->ndirs] = (char *)malloc(len+1);
    for(x=0; x<len; x++)
      list->dirs[list->ndirs][x] = name[x+1];
    list->dirs[list->ndirs][len] = 0;
    list->ndirs++;
  }
  else {
    // it's a file

    if (list->nnames == xstdio_mac_maxnames)
      return;
    
    name = (unsigned char *)(cpbPtr->hFileInfo.ioNamePtr);
    len = name[0];

    list->names[list->nnames] = (char *)malloc(len+1);
    for(x=0; x<len; x++)
      list->names[list->nnames][x] = name[x+1];
    list->names[list->nnames][len] = 0;
    list->nnames++;
  }
}

int
xio_getfiles (xio_constpath path1, char ***names, char ***dirs, int *nnames2, int *ndirs2)
{
  char *p2 = xio_fixpath (path1);
  char *path;
  int pathlen;
  dirlist list;
  OSErr err;
  short vRefNum;
  long  parID;
  Str255 name;

  *names = 0;
  *dirs = 0;
  *nnames2 = 0;
  *ndirs2 = 0;

  // Make sure path ends in a colon
  if (strlen(p2) && p2[strlen(p2)-1]!=':')
  {
    path = malloc(strlen(p2)+2);
    strcpy(path,p2);
    strcat(path,":");
  }
  else
    path = p2;
  
  list.nnames = 0;
  list.ndirs = 0;
  list.names = (char **) malloc (xstdio_mac_maxnames * sizeof (char *));
  list.dirs = (char **) malloc (xstdio_mac_maxdirs * sizeof (char *));

  pathlen = strlen(path);
  err = LocationFromFullPath(pathlen, path, &vRefNum, &parID, name);
  if (err) goto bad;
  
  err = IterateDirectory(vRefNum, parID, name, 1,
                         myIterateFn,
                         (void *)&list);
  if (err) goto bad;

  // Add the empty directory
  list.dirs[list.ndirs] = malloc(2);
  strcpy(list.dirs[list.ndirs],":");
  list.ndirs++;

  if (list.nnames)
    list.names = (char **) realloc (list.names, list.nnames * sizeof (char *));
  else {
    free (list.names);
    list.names = NULL;
  }
  
  if (list.ndirs)
    list.dirs = (char **) realloc (list.dirs, list.ndirs * sizeof (char *));
  else {
    free (list.dirs);
    list.dirs = NULL;
  }

  *nnames2 = list.nnames;
  *ndirs2 = list.ndirs;
  *names = list.names;
  *dirs = list.dirs;

  free(p2);
  if (p2 != path)
    free(path);

  return 1;
  
bad:

  free(p2);
  if (p2 != path)
    free(path);

  return 0;
}

xio_path
xio_getdirectory (xio_constpath filename)
{
  int i;
  xio_pathdata directory;
  for (i = (int) strlen (filename); i && filename[i] != '/' &&
       filename[i] != '\\' && filename[i] != XIO_PATHSEP; i--) {}
       
//  if (filename[i] == '/' || filename[i] == '\\' || filename[i] == XIO_PATHSEP)
//    i++;

  directory[i] = 0;
  i--;
  for (; i >= 0; i--)
    directory[i] =
      filename[i];
  return (mystrdup (directory));
}

xio_path
xio_getfilename (CONST char *basename, CONST char *extension)
{
  static char name[40];
  int nimage = 0;
#ifdef _plan9_
  char edir[DIRLEN];
#else
  struct stat sb;
#endif
  char *base = xio_fixpath (basename);
  do
    {
      sprintf (name, "%s%i%s", base, nimage++, extension);
#ifndef _plan9_
    }
  while (stat (name, &sb) != -1);
#else
    }
  while (stat (name, edir) != -1);
#endif
  free (base);
  return (name);
}
xio_file
xio_getrandomexample (xio_path name)
{
#if 0
  static CONST char * CONST paths[] =
  {				/*Where examples should be located? */
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
  static CONST xio_constpath paths[] =
  {				/*Where catalogs should be located? */
    CATALOGSPATH,		/*Data path when XaoS is propertly installed */
    "\01" XIO_PATHSEPSTR "catalogs" XIO_PATHSEPSTR,
    "\01" XIO_PATHSEPSTR ".." XIO_PATHSEPSTR "catalogs" XIO_PATHSEPSTR,
    "." XIO_PATHSEPSTR "catalogs" XIO_PATHSEPSTR,
    /*XaoS was started from root of source tree */
    ".." XIO_PATHSEPSTR "catalogs" XIO_PATHSEPSTR,
    /*XaoS was started from bin directory in source tree */
    XIO_EMPTYPATH,		/*Oops...it's not. Try curent directory */
  };
  int i;
  xio_file f = XIO_FAILED;
  xio_pathdata tmp;
  for (i = 0; i < (int) (sizeof (paths) / sizeof (char *)) && f == XIO_FAILED; i++)
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
  static CONST xio_constpath paths[] =
  {	/*Where help should be located? */
    HELPPATH,			/*Data path when XaoS is propertly installed */
  "\01" XIO_PATHSEPSTR "help" XIO_PATHSEPSTR "xaos.hlp",
  "\01" XIO_PATHSEPSTR ".."   XIO_PATHSEPSTR "help" XIO_PATHSEPSTR "xaos.hlp",
    "." XIO_PATHSEPSTR "help" XIO_PATHSEPSTR "xaos.hlp",
  /*XaoS was started from root of source tree */
    ".." XIO_PATHSEPSTR "help" XIO_PATHSEPSTR "xaos.hlp",
  /*XaoS was started from bin directory in source tree */
    "." XIO_PATHSEPSTR "xaos.hlp",
  /*Oops...it's not. Try curent directory */
  };
  int i;
  xio_file f = XIO_FAILED;
  for (i = 0; i < (int) (sizeof (paths) / sizeof (char *)) && f == XIO_FAILED; i++)
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
  static CONST xio_constpath paths[] =
  {				/*Where tutorials should be located? */
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

  for (i = 0; i < (int) (sizeof (paths) / sizeof (char *)) && f == XIO_FAILED; i++)
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
  f->data = (void *) fopen (name, "rb");
  /*free (name);*/
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
  /*free (name);*/
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
       (((name[0] >= 'a' && name[0] <= 'z') || (name[0] >= 'A' && name[0] <= 'Z')) && name[1] == ':' && (name[2] == '\\' || name[2] == '/')) ||
#endif
       name[0] == '/' || name[0] == '\\' || name[0] == XIO_PATHSEP || name[0] == '~')
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
      
      if (xio_appdir[strlen(xio_appdir)-1] != XIO_PATHSEP)
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
