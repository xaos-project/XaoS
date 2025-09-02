#ifndef UI_HELPER_H
#define UI_HELPER_H
#include "timers.h"
#include "xio.h"
#include "fractal.h"

#define MAXFILTERS 20
#define AVRGSIZE 50
#define NMESSAGES 5
#define BGCOLOR(uih) uih->palette->index[0]
#define FGCOLOR(uih) uih->palette->index[1]
#define SELCOLOR(uih) uih->palette->index[2]
#define NONTRANSPARENTW 1

struct uih_message {
    char *message[NMESSAGES];
    tl_timer *messagetimer[NMESSAGES];
    int messagetype[NMESSAGES];
    struct uih_window *w[NMESSAGES];
    int pid[NMESSAGES];
    int messagestart;
};
struct uih_line {
    int key;
    int morph;
    int color;
    int posmode;
    number_t x1, y1, x2, y2;
    int mposmode;
    number_t mx1, my1, mx2, my2;
    struct uih_line *next, *prev;
    struct uih_window *w;
};
struct uih_lines {
    struct uih_line *first;
    int morphing;
    int currkey;
};

struct uih_savedcontext {
    xio_file file;
    int mode;
    number_t speedup, maxstep;
    number_t xcenter, ycenter;
    tl_timer *timer;
    tl_timer *synctimer;
    struct fractal_context *fcontext;
    int clearscreen;
    int fastmode, juliamode, fastrotate, autorotate;
    number_t rotationspeed;
    int firsttime;
    int filter[MAXFILTERS];
    int pressed;
    int rotatepressed;
    int cycling;
    int shifted;
    int manualpaletteshift;
    int direction;
    int cyclingspeed;
    int zoomactive;
    int xtextpos, ytextpos;
    int writefailed;
    int nonfractalscreen;
    int color;
};
#define MAXLEVEL 10 /*Maximal include level */
struct uih_playcontext {
    xio_file file;
    xio_file prevfiles[MAXLEVEL + 1];
    int level;
    xio_path directory;
    tl_timer *timer;
    int waittime;
    int playframe;
    int timerin;
    int frametime, starttime;
    int morph;
    int morphtimes[2];
    int morphjulia;
    int morphjuliatimes[2];
    int morphangle;
    int morphangletimes[2];
    vinfo destination;
    vinfo source;
    number_t srcangle, destangle;
    number_t sr, si, dr, di;
    int readfailed;
    int line;
    struct uih_lines lines;
    int morphlinetimes[2];
};
#define SQR(val) (((double)(val)) * (val))
#define MORPHVALUE(time, len, starttime, endtime)                              \
    (time) < 0                                                                 \
        ? 0.0                                                                  \
        : (time) >= (len)                                                      \
              ? 1.0                                                            \
              : (time) < (starttime) && (starttime)                            \
                    ? (SQR((time) / (double)(starttime)) / 2 * (starttime) /   \
                       ((len) - (starttime) / 2 - (endtime) / 2))              \
                    : ((len) - (time) < (endtime)) && (endtime)                \
                          ? 1 - (SQR(((len) - (time)) / (double)(endtime)) /   \
                                 2 * (endtime) /                               \
                                 ((len) - (starttime) / 2 - (endtime) / 2))    \
                          : ((time) - (starttime) / 2) /                       \
                                ((double)(len) - (starttime) / 2 -             \
                                 (endtime) / 2)
#define DOMORPH(time, len, starttime, endtime, startval, endval)               \
    ((startval) +                                                              \
     ((endval) - (startval)) * MORPHVALUE(time, len, starttime, endtime))

#define UNDOLEVEL 256
struct uih_undocontext {
    int last;
    char *undos[256];
};
struct uih_context {
    void (*updatemenus)(struct uih_context *, const char *);
    /*stuff that should be visible from outside */
    number_t speedup, maxstep; /*zooming speed */

    /* Information provided to the user interface: */
    const char *menuroot;
    int display;       /*1 when ui wants to display something */
    int save;          /*1 if save animation is enabled */
    int play;          /*1 if animation replay is active */
    int cycling;       /*1 if cycling is enabled */
    int incalculation; /*1 if calulcation is currently in process */
    int flags;
    int interrupt; /*set to interrupt current calculation */

    /*server's callbacks */
    int (*passfunc)(struct uih_context *, int, const char *, float);
    void (*longwait)(struct uih_context *);

    struct uih_undocontext undo;
    /*Filter system state */
    struct image *image;
    struct palette *palette;
    struct fractal_context *fcontext; /*fractal information */
    struct queue *queue;
    struct filter *uifilter; /*user interface layer */
    struct filter *rotatef;  /* Special filters handler by ui_helper: */
    struct filter *zengine;
    struct filter *fixedcolor;
    /*Julia/filter mechanizm */
    struct filter *subwindow, *julia, *smalliter;
    struct filter *filter[MAXFILTERS];

    /*General status variables */
    double mul;             /*speed of last iteration */
    int rotatemode;         /*ROTATE_NONE, ROTATE_CONTINUOUS or ROTATE_NONE */
    number_t rotationspeed; /*speed of continuous rotation */
    int fastmode;           /*when draw in fast mode */
    int juliamode;
    int fastrotate;
    int incomplete; /*1 if image is not completely calculated or in animation */
    int dirty;      /*1 if image is inexact */
    int inanimation;              /*1 if uih_update wants to be called soon */
    int fastanimation;            /*1 if animation needs to be fast */
    int palettetype, paletteseed; /*0 for default palette,1,2 for random */
    int clearscreen;              /*1 when ui want to clear screen */
    int indofractal;              /*1 when caluclation is in the process */
    int xtextpos, ytextpos;       /*positioning of text */
    int color;                    /*Color of text */
    int recalculatemode;          /*information for user interface */
    int stoppedtimers;            /*1 when timers are stopped */
    int nletters;                 /*Number of letters displayed at screen */
    int letterspersec; /*Number of letters per second user should read */
    char *text[3];     /*Currently displayed text information: */
    struct uih_window *textwindow[3], *cscreenwindow;
    int textpos[3], textcolor[3];
    const char *errstring; /*String of last unprocessed error */

    void *font; /*Font used by UI */
    struct uih_window *wtop;
    int wflipped;
    int wdisplayed;

    /*Save variables */
    int todisplayletters;
    struct uih_savedcontext *savec;
    int viewchanged;    /*When to generate setview commands */
    int palettechanged; /*When to generate setpalette commands */
    int displaytext;    /*When text was displayed in this frame */
    int palettepickerenabled; /*If palette picker is used */
    int nonfractalscreen;
    /*waiting variables */
    void (*complettehandler)(
        void *); /*Handler to be activated when calculation is complete */
    void *handlerdata;
    /*replay variables */
    struct uih_playcontext *playc;
    int playpos;
    const char *playstring;

    /*For constant framerate */
    struct timeemulator *emulator;
    int emulatedframetime;
    int aliasnum;
    int fixedstep;

    /*zoom/unzoom */
    number_t speed, step;
    number_t xcenter, ycenter;
    int xcenterm, ycenterm;
    int zoomactive;

    /*drag&drop move */
    int pressed;
    number_t oldx, oldy;
    int moved;

    /*drag&drop rotate */
    int rotatepressed;
    number_t oldangle;

    int ddatalost;
    int tbreak;

    int autopilot; /*for uih side of autopilot */
    int autopilotx, autopiloty, autopilotbuttons;

    /*calculation time variables */
    int interruptiblemode;
    int starttime, endtime;
    int maxtime;

    /*dynamical timeout measuring */
    int times[2][AVRGSIZE]; /*for statistics */
    int timespos, count[2];
    double lastspeed, lasttime;

    /*number_t xsize, ysize; */
    tl_timer *maintimer, *cyclingtimer, *autopilottimer, *calculatetimer,
        *doittimer;
    tl_group *autopilotgroup;

    /*color cycling values */
    int direction;
    int cyclingdirection;
    int stopped;
    int cyclingspeed;

    /*autopilot internal values */
    int x1, y1, c1;
    number_t minsize;
    number_t maxsize;
    int autopilotversion;
    int autime;
    int minlong;
    int interlevel;

    /*saved palettes */
    struct palette *palette2;

    int paletteshift;
    int manualpaletteshift;

    struct uih_message messg;

    /*Used by uih_update to figure out when save undo */
    int lastbuttons;

    int encoding;

    /* performers really don't want text echoed to the screen */
    int inhibittextoutput;

    /* user data */
    void *data;
};

typedef void (*uih_getposfunc)(struct uih_context *c, int *x, int *y,
                               int *width, int *height, void *data);
typedef void (*uih_drawfunc)(struct uih_context *c, void *data);
struct uih_window {
    int x, y, width, height;
    uih_getposfunc getpos;
    uih_drawfunc draw;
    struct uih_window *next;
    struct uih_window *previous;
    int savedline, savedpos;
    char *saveddata;
    void *data;
    int flags;
};
typedef struct uih_context uih_context;

#define UIH_SAVEALL 2
#define UIH_SAVEANIMATION 1
#define UIH_SAVEPOS 0

#define UIH_PALETTEDRAW -2
#define UIH_FILTERANIMATION -1
#define UIH_INTERRUPTIBLE 0
#define UIH_ANIMATION 1
#define UIH_NEW_IMAGE 2
#define UIH_UNINTERRUPTIBLE 3
#define FRAMETIME (1000000 / FRAMERATE)

#define UIH_TEXTTOP 0
#define UIH_TEXTMIDDLE 1
#define UIH_TEXTBOTTOM 2

#define UIH_TEXTLEFT 0
#define UIH_TEXTCENTER 1
#define UIH_TEXTRIGHT 2

// use the same order here as for language1 and language2
#define UIH_LANG_SYS_DEFAULT 0
#define UIH_LANG_HY 1
#define UIH_LANG_CS 2
#define UIH_LANG_EN 3
#define UIH_LANG_FR 4
#define UIH_LANG_DE 5
#define UIH_LANG_HE 6
#define UIH_LANG_HI 7
#define UIH_LANG_HU 8
#define UIH_LANG_IS 9
#define UIH_LANG_IT 10
#define UIH_LANG_PT 11
#define UIH_LANG_RO 12
#define UIH_LANG_RU 13
#define UIH_LANG_RS 14
#define UIH_LANG_ES 15
#define UIH_LANG_SV 16
#define UIH_LANG_TR 17
#define UIH_LANG_VI 18

#define RANDOM_PALETTE_SIZE 1
#define FULLSCREEN 2
#define UPDATE_AFTER_PALETTE 4
#define UPDATE_AFTER_RESIZE 8
#define PALETTE_ROTATION 16
#define ASYNC_PALETTE 32
#define ROTATE_INSIDE_CALCULATION 64
#define PALETTE_REDISPLAYS 128
#define SCREENSIZE 256
#define PIXELSIZE 512
#define RESOLUTION 1024

#define BUTTON1 256
#define BUTTON2 512
#define BUTTON3 1024

#define ROTATE_NONE 0
#define ROTATE_MOUSE 1
#define ROTATE_CONTINUOUS 2

#define uih_needrecalculate(context) ((context)->recalculatemode)
#define uih_needdisplay(context) ((context)->display)
#define GETMAX(a, b) ((a) > (b) ? (a) : (b))
#define uih_newimage(c)                                                        \
    ((c)->display = 1,                                                         \
     ((c)->recalculatemode = GETMAX((c)->recalculatemode, UIH_NEW_IMAGE)))
#define uih_animate_image(c)                                                   \
    ((c)->display = 1,                                                         \
     (c)->recalculatemode = GETMAX((c)->recalculatemode, UIH_ANIMATION))

#define uih_updatemenus(uih, c)                                                \
    if (uih->updatemenus != NULL)                                              \
        uih->updatemenus(uih, c);

extern const struct filteraction *const uih_filters[MAXFILTERS];
extern const int uih_nfilters;

extern const char *const xtextposnames[];
extern const char *const ytextposnames[];

extern int defthreads;

struct uih_context *
uih_mkcontext(int flags, struct image *image,
              int (*passfunc)(struct uih_context *, int, const char *, float),
              void (*longwait)(struct uih_context *),
              void (*updatemenus)(struct uih_context *c, const char *));
int uih_updateimage(uih_context *c, struct image *img);
void uih_freecontext(uih_context *c);

void uih_callcomplette(uih_context *c);
/*palette functions */
void uih_mkdefaultpalette(uih_context *c);
void uih_mkpalette(uih_context *c);
void uih_savepalette(uih_context *c);
void uih_restorepalette(uih_context *c);
void uih_loadpalette(uih_context *c, struct palette *palette);

/*autopilot handling */
void uih_autopilot_on(uih_context *c);
void uih_autopilot_off(uih_context *c);

/*misc functions */
int uih_update(uih_context *c, int mousex, int mousey, int mousebuttons);
const char *uih_save(struct uih_context *c, xio_constpath filename, xio_file xpf_data);
void uih_tbreak(uih_context *c);
double uih_displayed(uih_context *c);
void uih_do_fractal(uih_context *c);
void uih_prepare_image(uih_context *c);
void uih_interrupt(uih_context *c);
void uih_stopzooming(uih_context *c);
void uih_setspeedup(uih_context *c, number_t speed);
void uih_setmaxstep(uih_context *c, number_t speed);
void uih_setcomplettehandler(uih_context *c, void(h)(void *), void *d);
void uih_recalculate(struct uih_context *c);
void uih_initstate(struct uih_context *uih);
void uih_screentofractalcoord(uih_context *c, int mousex, int mousey,
                              number_t *re, number_t *im);

/*cycling functions */
void uih_cycling_off(struct uih_context *c);
void uih_cycling_stop(struct uih_context *c);
void uih_cycling_continue(struct uih_context *c);
void uih_setcycling(struct uih_context *c, int speed);
int uih_cycling_on(struct uih_context *c);
int uih_cycling(struct uih_context *c, int mode);

/*fractal context manipulation routines */
void uih_setformula(uih_context *c, int formula);
void uih_setperbutation(uih_context *c, number_t re, number_t im);
void uih_perbutation(uih_context *c, int mousex, int mousey);
void uih_setmaxiter(uih_context *c, int maxiter);
void uih_setbailout(uih_context *c, number_t bailout);
void uih_setincoloringmode(uih_context *c, int mode);
void uih_setoutcoloringmode(uih_context *c, int mode);
void uih_setintcolor(uih_context *c, int mode);
void uih_setouttcolor(uih_context *c, int mode);
void uih_setplane(uih_context *c, int mode);
void uih_setmandelbrot(uih_context *c, int mode, int mousex, int mousey);
void uih_setfastmode(uih_context *c, int mode);
void uih_setguessing(uih_context *c, int range);
void uih_setperiodicity(uih_context *c, int periodicity);
void uih_display(uih_context *c);
void uih_disablejulia(uih_context *c);
int uih_enablejulia(uih_context *c);
int uih_setjuliamode(uih_context *c, int mode);
void uih_setjuliaseed(uih_context *c, number_t zre, number_t zim);

/*filter manipulation */
int uih_enablefilter(uih_context *c, int n);
void uih_disablefilter(uih_context *c, int n);

/*Animation save routines */
int uih_save_enable(struct uih_context *uih, xio_file f, int mode);
void uih_save_disable(struct uih_context *uih);
void uih_saveframe(struct uih_context *uih);
void uih_save_position(struct uih_context *uih, xio_file f, int mode);

void uih_load(struct uih_context *uih, xio_file f, xio_constpath name);
void uih_loadstr(struct uih_context *uih, const char *data);
void uih_playstr(struct uih_context *uih, const char *data);
void uih_playupdate(struct uih_context *uih);
void uih_replaydisable(struct uih_context *uih);
void uih_skipframe(struct uih_context *uih);
int uih_replayenable(struct uih_context *uih, xio_file f,
                     xio_constpath filename, int animroot);
void uih_command(struct uih_context *uih, const char *command);
void uih_playtutorial(struct uih_context *c, const char *name);

/* Easy to use functions for handling save/load*/
void uih_loadfile(struct uih_context *uih, xio_constpath d);
void uih_playfile(struct uih_context *c, xio_constpath d);
void uih_loadexample(struct uih_context *c);
void uih_loadpngfile(struct uih_context *c, xio_constpath d);
void uih_savepngfile(struct uih_context *c, xio_constpath d);
void uih_saveposfile(struct uih_context *c, xio_constpath d);
void uih_copyposurl(struct uih_context *c);
char *uih_savepostostr(struct uih_context *c);
void uih_savecfg(struct uih_context *c);
void uih_saveanimfile(struct uih_context *c, xio_constpath d);
void uih_update_lines(uih_context *c);

/*timer functions */
void uih_stoptimers(uih_context *c);
void uih_resumetimers(uih_context *c);
void uih_slowdowntimers(uih_context *c, int time);

/*text output functions */
void uih_clearscreen(uih_context *c);
void uih_settextpos(uih_context *c, int x, int y);
void uih_text(uih_context *c, const char *text);
void uih_letterspersec(uih_context *c, int n);

/*image rotation functions */
int uih_fastrotate(uih_context *c, int mode);
int uih_fastrotateenable(uih_context *c);
void uih_fastrotatedisable(uih_context *c);
void uih_angle(uih_context *c, number_t angle);
void uih_rotatemode(uih_context *c, int mode);
void uih_rotate(uih_context *c, int mode);
void uih_rotationspeed(uih_context *c, number_t speed);

/*Catalog functions */
int uih_loadcatalog(uih_context *c, const char *name);
void uih_freecatalog(uih_context *c);

void uih_registermenus(void);
void uih_registermenus_i18n(void);
void uih_registermenudialogs_i18n(void);
void uih_unregistermenus(void);

/*Windows :)*/
struct uih_window *uih_registerw(struct uih_context *uih, uih_getposfunc getpos,
                                 uih_drawfunc draw, void *data, int flags);
void uih_removew(struct uih_context *uih, struct uih_window *w);
void uih_clearwindows(struct uih_context *uih);
void uih_drawwindows(struct uih_context *uih);
void uih_setline(struct uih_context *uih, struct uih_window *w, int color,
                 int x1, int y1, int x2, int y2);
struct uih_window *uih_registerline(struct uih_context *uih, int color, int x1,
                                    int y1, int x2, int y2);

/*Messages*/
void uih_scrollup(uih_context *c);
void uih_clearmessages(uih_context *c);
int uih_message(uih_context *c, const char *message);
int uih_error(uih_context *c, const char *error);
void uih_rmmessage(uih_context *c, int pid);
void uih_printmessages(uih_context *c);

/*Constant framerate functions*/
void uih_emulatetimers(uih_context *c);
void uih_constantframetime(uih_context *c, int time);
void uih_noconstantframetime(uih_context *c);

/*undo and redo*/
void uih_saveundo(uih_context *c);
void uih_undo(uih_context *c);
void uih_redo(uih_context *c);

void uih_setfont(uih_context *c);

/*animation rendering*/
int uih_renderanimation(struct uih_context *gc, const char *basename,
                        const xio_constpath animation, int width, int height,
                        float pixelwidth, float pixelheight, int frametime,
                        int type, int antialiasing, int slowmode,
                        int letterspersec, const char *catalog);
int uih_renderimage(struct uih_context *gc1, xio_file af, xio_constpath path,
                    struct image *img, int antialias, const char *catalog,
                    int noise);

void uih_initmessages(uih_context *c);
void uih_destroymessages(uih_context *c);
void uih_inittext(uih_context *c);
void uih_destroytext(uih_context *c);

void uih_inhibittextsw(uih_context *c);
int uih_inhibittextselected(uih_context *c);

void uih_updatestatus(uih_context *uih);
void uih_status(uih_context *uih);
int uih_statusenabled(uih_context *uih);
int uih_ministatusenabled(uih_context *uih);
void uih_ministatus(uih_context *uih);
int uih_cartesiangridenabled(uih_context *uih);
void uih_cartesiangrid(uih_context *uih);
void uih_sffeset(uih_context *c, sffe *parser, const char *formula);

#endif
