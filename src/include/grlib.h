#ifndef _TEXT_H
#define _TEXT_H
#ifdef __cplusplus
extern "C"
{
#endif
  extern CONST struct xfont
  {
    CONST unsigned char *data;
    int width, height, realwidth;
  }
  xsmallfont, xbigfont, xaafont, xsmallfontil1, xbigfontil1, xmedfontil1;
  int xprint (struct image *image, CONST struct xfont *font, int x, int y,
	      CONST char *text, int fgcolor, int bgcolor, int mode);
#define TEXT_PRESSED 1

#define xtextheight(font) ((font)->height+1)
#define xtextcharw(font,c) ((font)->width)
  int xtextwidth (CONST struct xfont *font, CONST char *text);
  void xrectangle (struct image *image, int x, int y, int width, int height,
		   int fgcolor);
  void xvline (struct image *image, int x, int y, int height, int fgcolor);
  void xhline (struct image *image, int x, int y, int width, int fgcolor);
  char *xsaveline (struct image *img, int x1, int y1, int x2, int y2);
  void xrestoreline (struct image *img, char *data, int x1, int y1, int x2,
		     int y2);
  void xline (struct image *img, int x1, int y1, int x2, int y2, int color);
  void xprepareimage (struct image *img);
  void xdrawcursor (struct image *img, int x, int y, int color, int height);
#ifdef __cplusplus
}
#endif

#endif
