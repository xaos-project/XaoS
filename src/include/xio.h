/* This is implementation of input/output routines similar to stdio.
purpose of this library is to hide differences between OSes (Mac OS don't
have stdio!) and allow general streams to strings etc. */
#ifndef XIO1_H
#define XIO1_H 1
#include <config.h>
#ifdef __cplusplus
extern "C" {
#endif


#define XIO_FAILED NULL
struct xio_filestruct {
    void *data;
    int (*fputc) (int c, struct xio_filestruct * f);
    int (*fputs) (const char *s, struct xio_filestruct * f);
    int (*fgetc) (struct xio_filestruct * f);
    int (*fungetc) (int c, struct xio_filestruct * f);
    int (*xeof) (struct xio_filestruct * f);
    int (*fclose) (struct xio_filestruct * f);
    int (*flush) (struct xio_filestruct * f);
};

typedef struct xio_filestruct *xio_file;

#define xio_puts(s,f) (f)->fputs((s),(f))
#define xio_putc(s,f) (f)->fputc((s),(f))
#define xio_getc(f) (f)->fgetc((f))
#define xio_ungetc(s,f) (f)->fungetc((s),(f))
#define xio_feof(f) (f)->xeof((f))
#define xio_close(f) (f)->fclose((f))
#define xio_flush(f) if ((f)->flush!=NULL) (f)->flush((f))

/* Standard stdio maps. These defines says, that filenames are strings and
path is separated by slash or backslash (windoze, dog)
the main I/O routines are in the xstdio file
*/

#ifdef USE_STDIO
/* Ugly hack because of unknown problems w/ va_list in v*print* in plan9 */
#ifdef _plan9_
#define va_list char *
#endif
#include <stdio.h>

typedef char *xio_path;
typedef const char *xio_constpath;
typedef char xio_pathdata[4096];
extern char *xio_appdir;	/*Where application binary is */
extern char *xio_homedir;


#ifdef _WIN32
#define XIO_PATHSEP '\\'
#define XIO_PATHSEPSTR "\\"
#define XIO_EMPTYPATH ".\\"	/*Should be also call to currentdir function */
#else
#ifdef DJGPP
#define XIO_PATHSEP '\\'
#define XIO_PATHSEPSTR "\\"
#define XIO_EMPTYPATH ".\\"	/*Should be also call to currentdir function */
#else
#define XIO_PATHSEP '/'
#define XIO_PATHSEPSTR "/"
#define XIO_EMPTYPATH "./"	/*Should be also call to currentdir function */
#endif
#endif
#define XIO_EOF EOF

#define xio_addfname(destination, dirrectory, filename) \
{strcpy(destination,dirrectory);if (strlen(dirrectory)&&destination[strlen(destination)-1]!=XIO_PATHSEP) strcat(destination,XIO_PATHSEPSTR);strcat(destination,filename);}
#define xio_addextension(destination,extension) strcat(destination,extension)

#ifdef _plan9_
#define xio_errorstring() errstr
#else
#define xio_errorstring() strerror(errno)
#endif				/*plan9 */

char *xio_fixpath(const char *name);
#endif				/*USE_STDIO */

xio_file xio_ropen(xio_constpath name);
xio_file xio_wopen(xio_constpath name);
xio_file xio_strropen(const char *c);
xio_file xio_strwopen(void);
char *xio_getstring(xio_file f);
xio_path xio_getdirectory(xio_constpath name);
xio_path xio_getfilename(const char *base, const char *extension);
xio_file xio_getrandomexample(xio_path name);
xio_file xio_getcatalog(const char *name);
xio_file xio_gethelp(void);
xio_file xio_gettutorial(const char *name, xio_path result);

/*look trought directory with examples, choose one file, open it (and return
 *descriptor+put name into name parameter*/
int xio_exist(xio_constpath name);
int xio_getfiles(xio_constpath path, char ***names, char ***dirs,
                 int *nnames, int *ndirs);
void xio_init(const char *c);
void xio_uninit(void);


#ifdef __cplusplus
}
#endif
#endif
