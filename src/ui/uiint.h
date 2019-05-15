#ifndef UI_INT_H
#define UI_INT_H

#define MOUSE_PRESS 1
#define MOUSE_RELEASE 2
#define MOUSE_DRAG 4
#define MOUSE_MOVE 8

#define BORDERWIDTH 2
#define BORDERHEIGHT 2

#define BUTTONHEIGHT (xtextheight(uih->image, uih->font)+2*BORDERWIDTH)

struct ui_textdata
{
    int x, y, width;
    char *text;
    int size;
    int cursor;
    int cursorpos;
    int start;
    int ndisplayed;
    int clear;
};
extern uih_context *uih;
extern int ui_nmenus;
extern char *ui_helptext[];
extern int ui_helpsize;
extern const struct params ui_fractal_params[];
extern int ui_nogui;
extern float pixelwidth, pixelheight;

#ifdef __cplusplus
extern "C" {
#endif

float ui_get_windowwidth (int width);
float ui_get_windowheight (int height);

void ui_updatetext (struct ui_textdata *d);
struct ui_textdata *ui_opentext (int x, int y, int width, const char *def);
void ui_drawtext (struct ui_textdata *d, int active);
void ui_textmouse (struct ui_textdata *d, int x, int y);
void ui_closetext (struct ui_textdata *d);
int ui_textkey (struct ui_textdata *d, int key);

int ui_dorender_params (void);

void ui_pipe_init (const char *name);

void ui_init (int argc, char **argv);
void ui_mainloop (int loop);

#ifdef __cplusplus
}
#endif

#endif
