#ifndef XSHL_H
#define XSHL_H
#ifdef __cplusplus
extern "C"
{
#endif

#define XSHL_BIG 1
#define XSHL_EMPH 2
#define XSHL_WHITE 4
#define XSHL_RED 8
#define XSHL_BLACK 16
#define XSHL_COLORMASK (XSHL_BLACK|XSHL_RED|XSHL_WHITE)
#define XSHL_RIGHTALIGN 32
#define XSHL_CENTERALIGN 64
#define XSHL_MONOSPACE 128
#define XSHL_LINK 256
  struct xshl_context
  {
    int flags;
    char *linktext;
  };
  struct xshl_item
  {
    struct xshl_context c;
    char *text;
    int x;
    int width;
    struct xshl_item *next;
  };
  struct xshl_line
  {
    int y;
    struct xshl_item *first;
  };
  extern char *xshl_error;
  struct xshl_line *xshl_interpret (void *userdata, int (*get) (void *),
				    int width, int getwidth (void *,
							     int flags,
							     CONST char
							     *text),
				    int startflags, int smallheight,
				    int bigheight);
  void xshl_free (struct xshl_line *);
  int xshl_textlen (void *data, int flags, CONST char *text);
  void xshl_print (int startskip, struct xshl_line *lines);
  struct xshl_line *help_make (CONST char *command,
			       int getwidth (void *, int flags,
					     CONST char *text), int width,
			       int smallheight, int bigheight);
#ifdef __cplusplus
}
#endif
#endif
