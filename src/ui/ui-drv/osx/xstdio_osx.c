/*
 *     XaoS, a fast portable realtime fractal zoomer 
 *                  Copyright ¬© 1996,1997 by
 *
 *      Jan Hubicka          (hubicka@paru.cas.cz)
 *      Thomas Marsh         (tmarsh@austin.ibm.com)
 *	
 *	Mac OS X Driver by J.B. Langston (jb-langston at austin dot rr dot com)
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
	CFBundleRef mainBundle;
	CFArrayRef exampleURLs;
	CFURLRef exampleURL;
	CFIndex exampleCount;
	CFIndex exampleIndex;
	char examplePath[255];
    xio_file f;
	
	// Get the main bundle for the app
	if ((mainBundle = CFBundleGetMainBundle()) == NULL)
		return XIO_FAILED;
	
    // Look for a resource in the main bundle by name and type.
    if ((exampleURLs = CFBundleCopyResourceURLsOfType( mainBundle, 
								   CFSTR( "xpf" ), 
								   NULL )) == NULL)
		return XIO_FAILED;

	if ((exampleCount = CFArrayGetCount(exampleURLs)) == 0)
	{
		CFRelease(exampleURLs);
		return XIO_FAILED;
	}
	
	exampleIndex = exampleCount * (float)rand() / (float)(RAND_MAX + 1.0);
	
	if ((exampleURL = CFArrayGetValueAtIndex(exampleURLs, exampleIndex)) == NULL)
	{
		CFRelease(exampleURLs);
		return XIO_FAILED;
	}
	
	// Convert URL to c-string containing absolute path
	if (!CFURLGetFileSystemRepresentation(exampleURL, true, (UInt8 *)examplePath, sizeof(examplePath)))
	{
		CFRelease(exampleURLs);
		return XIO_FAILED;	
	}
	
	CFRelease(exampleURLs);

	strcpy(name, examplePath);
	f = xio_ropen (examplePath);
	
	return (f);
}

xio_file
xio_getcatalog (CONST char *name)
{
	CFBundleRef mainBundle;
	CFURLRef catalogURL;
	CFStringRef catalogName;
	char catalogPath[255];
    xio_file f;
	
	// Get the main bundle for the app
	if ((mainBundle = CFBundleGetMainBundle()) == NULL)
		return XIO_FAILED;
	
	if ((catalogName = CFStringCreateWithCString(kCFAllocatorDefault, name, kCFStringEncodingUTF8)) == NULL)
		return XIO_FAILED;
	
    // Look for a resource in the main bundle by name and type.
    if ((catalogURL = CFBundleCopyResourceURL( mainBundle, 
								   catalogName, 
								   CFSTR("cat"), 
								   NULL )) == NULL)
	{
		CFRelease(catalogName);
		return XIO_FAILED;
	}
	
	CFRelease(catalogName);

	// Convert URL to c-string containing absolute path
	if (!CFURLGetFileSystemRepresentation(catalogURL, true, (UInt8 *)catalogPath, sizeof(catalogPath)))
	{
		CFRelease(catalogURL);
		return XIO_FAILED;	
	}
	
	CFRelease(catalogURL);

	f = xio_ropen (catalogPath);
	
	return (f);
}

xio_file
xio_gethelp (void)
{
	CFBundleRef mainBundle;
	CFURLRef helpURL;
	char helpPath[255];
    xio_file f;
	
	// Get the main bundle for the app
	if ((mainBundle = CFBundleGetMainBundle()) == NULL)
		return XIO_FAILED;
	
    // Look for a resource in the main bundle by name and type.
    if ((helpURL = CFBundleCopyResourceURL( mainBundle, 
								   CFSTR("xaos"), 
								   CFSTR("hlp"), 
								   NULL )) == NULL)
	{
		return XIO_FAILED;
	}
	
	// Convert URL to c-string containing absolute path
	if (!CFURLGetFileSystemRepresentation(helpURL, true, (UInt8 *)helpPath, sizeof(helpPath)))
	{
		CFRelease(helpURL);
		return XIO_FAILED;	
	}
	
	CFRelease(helpURL);

	f = xio_ropen (helpPath);
	
	return (f);
}

xio_file
xio_gettutorial (CONST char *name, xio_path tmp)
{
	CFBundleRef mainBundle;
	CFURLRef tutorialURL;
	CFStringRef tutorialName;
	char tutorialPath[255];
    xio_file f;
	
	// Get the main bundle for the app
	if ((mainBundle = CFBundleGetMainBundle()) == NULL)
		return XIO_FAILED;
	
	if ((tutorialName = CFStringCreateWithCString(kCFAllocatorDefault, name, kCFStringEncodingUTF8)) == NULL)
		return XIO_FAILED;
	
    // Look for a resource in the main bundle by name and type.
    if ((tutorialURL = CFBundleCopyResourceURL( mainBundle, 
								   tutorialName, 
								   NULL, 
								   NULL )) == NULL)
	{
		CFRelease(tutorialName);
		return XIO_FAILED;
	}
	
	CFRelease(tutorialName);

	// Convert URL to c-string containing absolute path
	if (!CFURLGetFileSystemRepresentation(tutorialURL, true, (UInt8 *)tutorialPath, sizeof(tutorialPath)))
	{
		CFRelease(tutorialURL);
		return XIO_FAILED;	
	}
	
	CFRelease(tutorialURL);

	strcpy(tmp, tutorialPath);
	f = xio_ropen (tutorialPath);
	
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
