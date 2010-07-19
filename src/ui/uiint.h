#ifndef UI_INT_H
#define UI_INT_H

#define MOUSE_PRESS 1
#define MOUSE_RELEASE 2
#define MOUSE_DRAG 4
#define MOUSE_MOVE 8

#define BORDERWIDTH 2
#define BORDERHEIGHT 2

#define BUTTONHEIGHT (xtextheight(uih->image, uih->font)+2*BORDERWIDTH)

struct ui_textdata {
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
extern const struct ui_driver *driver;
extern const int ndrivers;
extern const struct ui_driver *const drivers[];
extern int ui_nmenus;
extern char *ui_helptext[];
extern int ui_helpsize;
extern const struct params ui_fractal_params[];
extern int filevisible, helpvisible, dialogvisible, yesnodialogvisible;
extern int ui_nogui;

float ui_get_windowwidth(int width);
float ui_get_windowheight(int height);

void ui_updatetext(struct ui_textdata *d);
struct ui_textdata *ui_opentext(int x, int y, int width, const char *def);
void ui_drawtext(struct ui_textdata *d, int active);
void ui_textmouse(struct ui_textdata *d, int x, int y);
void ui_closetext(struct ui_textdata *d);
int ui_textkey(struct ui_textdata *d, int key);

int ui_menumouse(int x, int y, int mousebuttons, int flags);
int ui_menukey(int key);
int ui_menuwidth(void);
void ui_closemenus(void);

int ui_dorender_params(void);

void ui_updatestarts(void);

void ui_builddialog(const menuitem * d);
void ui_close_help(void);
void ui_closedialog(int call);
int ui_dialogmouse(int x, int y, int mousebuttons, int flags);
int ui_dialogkeys(int key);
void ui_buildyesno(const char *question, void (*handler) (int yes));
void ui_drawbutton(const char *text, int pressed, int selected, int x1,
		   int x2, int y);


void ui_buildfilesel(const char *f, const char *m,
                     void (*c) (const char *, int));
int ui_keyfilesel(int k);
int ui_mousefilesel(int x, int y, int buttons, int flags);
void ui_closefilesel(int succ);


void ui_pipe_init(const char *name);
int ui_helpkeys(int key);
int ui_helpmouse(int x, int y, int buttons, int flags);

#endif
