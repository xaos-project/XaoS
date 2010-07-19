#ifndef _TEXT_H
#define _TEXT_H
#ifdef __cplusplus
extern "C" {
#endif
extern const struct xfont {
    const unsigned char *data;
    int width, height, realwidth, encoding;
} xsmallfont, xbigfont, xbigfont2, xaafont, xsmallfontil1, xbigfontil1,
xmedfontil1, xbigfont3;
#define TEXT_PRESSED 1
int xprint (struct image *image, const struct xfont *current, int x,
               int y, const char *text, int fgcolor,
               int bgcolor, int mode);
int xtextwidth (struct image *image, const struct xfont *font, const char *text);
int xtextheight (struct image *image, const struct xfont *font);
int xtextcharw (struct image *image, const struct xfont *font, const char c);
void xrectangle (struct image *image, int x, int y, int width,
                    int height, int fgcolor);
void xvline (struct image *image, int x, int y, int height,
                int fgcolor);
void xhline (struct image *image, int x, int y, int width, int fgcolor);
char *xsaveline (struct image *img, int x1, int y1, int x2, int y2);
void xrestoreline (struct image *img, char *data, int x1, int y1,
                      int x2, int y2);
void xline (struct image *img, int x1, int y1, int x2, int y2,
               int color);
void xprepareimage (struct image *img);
void xdrawcursor (struct image *img, int x, int y, int color,
                     int height);

#ifdef __cplusplus
}
#endif
#endif
