#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <iomanip>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QSettings>

#include "filter.h"
#include "config.h"
#include "formulas.h"
#include "ui_helper.h"
#include "plane.h"
#include "xmenu.h"
#include "play.h"
#include "i18n.h"
#include "xthread.h"
#include "customdialog.h"

#define LANG(name, name2)                                                      \
    MENUSTRING("lang", NULL, name, name2, 0,                                   \
               (void (*)(struct uih_context * c, char *)) uih_loadcatalog,     \
               name2)
#define TUTOR(name1, name2, name3)                                             \
    MENUSTRING(name1, NULL, name2, name3, MENUFLAG_INTERRUPT | UI,             \
               uih_playtutorial, name3)
#define LANG_I(name, name2)                                                    \
    MENUSTRING_I("lang", NULL, name, name2, 0,                                 \
                 (void (*)(struct uih_context * c, char *)) uih_loadcatalog,   \
                 name2)
#define TUTOR_I(name1, name2, name3)                                           \
    MENUSTRING_I(name1, NULL, name2, name3, MENUFLAG_INTERRUPT | UI,           \
                 uih_playtutorial, name3)

const static char *const morphstypes[] = {"view", "julia", "angle", "line"};

static const char *const imgtypes[] = {"Truecolor", "256 colors", NULL};

static const char *const yesno[] = {"No", "Yes", NULL};

const static char *const lineposs[] = {"screen", "scaled", "fractal", NULL};

const char *const uih_colornames[] = {"white", "black", "red", NULL};

/* Registering internationalized dialogs.
 *
 * The method we are internationalizing dialogs is similar to
 * menu i18n. The original version of XaoS (without i18n)
 * contained lots of static variables. Each row of a variable
 * (which is a structure) contains a widget of a dialog.
 * The last row contains NULL and 0 values to show
 * that the dialog does not contain any other widgets.
 *
 * Here we are using a huge static variable which contains
 * all widget from all dialogs. We copy each dialog after
 * each other into this huge array. The original static
 * variables will now be pointers pointing to the first
 * row of the widget data from the appropriate line
 * of the huge array.
 *
 * Note that in the first version there are only 2
 * internationalized text, the rest will be "converted"
 * continuously (as I have enough time :-).
 *
 * Zoltan Kovacs <kovzol@math.u-szeged.hu>, 2003-01-05
 */

#define MAX_MENUDIALOGS_I18N 150
#define Register(variable) variable = &menudialogs_i18n[no_menudialogs_i18n]
static menudialog menudialogs_i18n[MAX_MENUDIALOGS_I18N];
// static int no_menudialogs_i18n;

static menudialog *uih_perturbationdialog, *uih_juliadialog,
    *uih_smoothmorphdialog, *uih_renderdialog, *uih_viewdialog, *uih_linedialog,
    *uih_colordialog, *uih_rotationdialog, *uih_lettersdialog, *uih_iterdialog,
    *dtextparam, *dcommand, *loaddialog, *playdialog, *saveimgdialog,
    *saveposdialog, *uih_formuladialog, *uih_plviewdialog, *uih_coorddialog,
    *uih_angledialog, *uih_autorotatedialog, *uih_fastrotatedialog,
    *uih_filterdialog, *uih_shiftdialog, *uih_speeddialog, *printdialog,
    *uih_bailoutdialog, *uih_threaddialog, *saveanimdialog, *uih_juliamodedialog,
    *uih_textposdialog, *uih_fastmodedialog, *uih_timedialog, *uih_numdialog,
    *uih_fpdialog, *palettedialog, *uih_cyclingdialog, *palettegradientdialog,
    *uih_renderimgdialog, *palettepickerdialog, *loadgpldialog, *savegpldialog,
    *uih_palettecolorsdialog
#ifdef USE_SFFE
    ,
    *uih_sffedialog, *uih_sffeinitdialog
#endif
    ;

void uih_registermenudialogs_i18n(void)
{
    int no_menudialogs_i18n = 0;

    /*
     * The original code was:

    static menudialog uih_perturbationdialog[] = {
      DIALOGCOORD ("Perturbation:", 0, 0),
      {NULL}

    };

     * Now first the static variable have to be registered (1),
     * the widget must be inserted into the huge array (2),
     * and the last row shows that no more widget comes (3).
     */

    Register(uih_perturbationdialog);                   // (1)
    DIALOGCOORD_I(TR("Dialog", "Perturbation:"), 0, 0); // (2)
    NULL_I();                                           // (3)

    Register(uih_juliadialog);
    DIALOGCOORD_I(TR("Dialog", "Julia-seed:"), 0, 0);
    NULL_I();

    Register(uih_smoothmorphdialog);
    DIALOGCHOICE_I(TR("Dialog", "Morphing type:"), morphstypes, 0);
    DIALOGINT_I(TR("Dialog", "Startuptime:"), 0);
    DIALOGINT_I(TR("Dialog", "Stoptime:"), 0);
    NULL_I();

    Register(uih_renderdialog);
    DIALOGIFILES_I(TR("Dialog", "Files to render:"), 0);
    DIALOGOFILE_I(TR("Dialog", "Basename:"), "anim");
    DIALOGINT_I(TR("Dialog", "Width:"), 640);
    DIALOGINT_I(TR("Dialog", "Height:"), 480);
    DIALOGFLOAT_I(TR("Dialog", "Pixel width (cm):"), 0.025);
    DIALOGFLOAT_I(TR("Dialog", "Pixel height (cm):"), 0.025);
    DIALOGFLOAT_I(TR("Dialog", "Framerate:"), 30);
    DIALOGCHOICE_I(TR("Dialog", "Image type:"), imgtypes, 0);
    DIALOGCHOICE_I(TR("Dialog", "Antialiasing:"), yesno, 0);
    DIALOGCHOICE_I(TR("Dialog", "Always recalculate:"), yesno, 0);
    NULL_I();

    Register(uih_renderimgdialog);
    DIALOGOFILE_I(TR("Dialog", "Basename:"), "anim");
    DIALOGINT_I(TR("Dialog", "Width:"), 640);
    DIALOGINT_I(TR("Dialog", "Height:"), 480);
    DIALOGFLOAT_I(TR("Dialog", "Pixel width (cm):"), 0.025);
    DIALOGFLOAT_I(TR("Dialog", "Pixel height (cm):"), 0.025);
    DIALOGCHOICE_I(TR("Dialog", "Image type:"), imgtypes, 0);
    DIALOGCHOICE_I(TR("Dialog", "Antialiasing:"), yesno, 0);
    NULL_I();

    Register(uih_viewdialog);
    DIALOGCOORD_I(TR("Dialog", "Center:"), 0, 0);
    DIALOGFLOAT_I(TR("Dialog", "Radius:"), 1);
    DIALOGFLOAT_I(TR("Dialog", "Angle:"), 0);
    NULL_I();

    Register(uih_linedialog);
    DIALOGCHOICE_I(TR("Dialog", "Mode:"), lineposs, 0);
    DIALOGCOORD_I(TR("Dialog", "Start:"), 0, 0);
    DIALOGCOORD_I(TR("Dialog", "End:"), 0, 0);
    NULL_I();

    Register(uih_colordialog);
    DIALOGCHOICE_I(TR("Dialog", "Color:"), uih_colornames, 0);
    NULL_I();

    Register(uih_rotationdialog);
    DIALOGFLOAT_I(TR("Dialog", "Rotations per second:"), 0);
    NULL_I();

    Register(uih_lettersdialog);
    DIALOGINT_I(TR("Dialog", "Letters per second:"), 0);
    NULL_I();

    Register(uih_iterdialog);
    DIALOGINT_I(TR("Dialog", "Iterations:"), 0);
    NULL_I();

    Register(dtextparam);
    DIALOGSTR_I(TR("Dialog", "Text:"), "");
    NULL_I();

    Register(dcommand);
    DIALOGSTR_I(TR("Dialog", "Your command:"), "");
    NULL_I();

    Register(loaddialog);
    DIALOGIFILE_I(TR("Dialog", "Filename:"), "*.png *.xpf");
    NULL_I();

    Register(playdialog);
    DIALOGIFILE_I(TR("Dialog", "Filename:"), "anim*.xaf");
    NULL_I();

    Register(saveimgdialog);
    DIALOGOFILE_I(TR("Dialog", "Filename:"), "fract*.png");
    NULL_I();

    Register(saveposdialog);
    DIALOGOFILE_I(TR("Dialog", "Filename:"), "fract*.xpf");
    NULL_I();

    Register(uih_formuladialog);
    DIALOGKEYSTR_I(TR("Dialog", "Formula:"), "mandel");
    NULL_I();

    Register(uih_plviewdialog);
    DIALOGFLOAT_I(TR("Dialog", "X center:"), 0);
    DIALOGFLOAT_I(TR("Dialog", "Y center:"), 0);
    DIALOGFLOAT_I(TR("Dialog", "X Radius:"), 1);
    DIALOGFLOAT_I(TR("Dialog", "Y Radius:"), 1);
    NULL_I();

    Register(uih_coorddialog);
    DIALOGCOORD_I(TR("Dialog", "Coordinates:"), 0, 0);
    NULL_I();

    Register(uih_angledialog);
    DIALOGFLOAT_I(TR("Dialog", "Angle:"), 1);
    NULL_I();

    Register(uih_autorotatedialog);
    DIALOGONOFF_I(TR("Dialog", "continuous rotation"), 0);
    NULL_I();

    Register(uih_fastrotatedialog);
    DIALOGONOFF_I(TR("Dialog", "Fast rotation"), 0);
    NULL_I();

    Register(uih_filterdialog);
    DIALOGKEYSTR_I(TR("Dialog", "filter"), "");
    DIALOGONOFF_I(TR("Dialog", "enable"), 0);
    NULL_I();

    Register(uih_shiftdialog);
    DIALOGINT_I(TR("Dialog", "Amount:"), 0);
    NULL_I();

    Register(uih_speeddialog);
    DIALOGFLOAT_I(TR("Dialog", "Zooming speed:"), 0);
    NULL_I();

    Register(printdialog);
    DIALOGSTR_I(TR("Dialog", "Name:"), "");
    NULL_I();

    Register(uih_bailoutdialog);
    DIALOGFLOAT_I(TR("Dialog", "Bailout:"), 0);
    NULL_I();

    Register(uih_threaddialog);
    DIALOGINT_I(TR("Dialog", "Threads:"), 0);
    NULL_I();

    Register(saveanimdialog);
    DIALOGOFILE_I(TR("Dialog", "Filename:"), "anim*.xaf");
    NULL_I();

    Register(uih_juliamodedialog);
    DIALOGONOFF_I(TR("Dialog", "Julia mode:"), 0);
    NULL_I();

    Register(uih_textposdialog);
    DIALOGCHOICE_I(TR("Dialog", "Horizontal position:"), xtextposnames, 0);
    DIALOGCHOICE_I(TR("Dialog", "Vertical position:"), ytextposnames, 0);
    NULL_I();

    Register(uih_fastmodedialog);
    DIALOGCHOICE_I(TR("Dialog", "Dynamic resolution:"), save_fastmode, 0);
    NULL_I();

    Register(uih_timedialog);
    DIALOGINT_I(TR("Dialog", "Time:"), 0);
    NULL_I();

    Register(uih_numdialog);
    DIALOGINT_I(TR("Dialog", "Number:"), 0);
    NULL_I();

    Register(uih_fpdialog);
    DIALOGFLOAT_I(TR("Dialog", "Number:"), 0);
    NULL_I();

    Register(palettedialog);
    DIALOGINT_I(TR("Dialog", "Algorithm number:"), 0);
    DIALOGINT_I(TR("Dialog", "Seed:"), 0);
    DIALOGINT_I(TR("Dialog", "Shift:"), 0);
    NULL_I();

    Register(palettegradientdialog);
    DIALOGPALSLIDER_I("Visualiser:", 0);
    NULL_I();

    Register(uih_palettecolorsdialog);
    for (int colidx = 0; colidx < 31; colidx++) {
        DIALOGSTR_I(TR("Dialog", "Color:"), "000000");
    }
    NULL_I();

    Register(palettepickerdialog);
    DIALOGPALPICKER_I("Palette:", 0);
    NULL_I();

    Register(loadgpldialog);
    DIALOGIFILE_I(TR("Dialog", "Load Palette Config"), "file*.gpl");
    NULL_I();

    Register(savegpldialog);
    DIALOGOFILE_I(TR("Dialog", "Save Palette Config"), "file*.gpl");
    NULL_I();

    Register(uih_cyclingdialog);
    DIALOGINT_I(TR("Dialog", "Frames per second:"), 0);
    NULL_I();

#ifdef USE_SFFE
    Register(uih_sffedialog);
    DIALOGLIST_I(TR("Dialog", "Formula"), USER_FORMULA);
    NULL_I();

    Register(uih_sffeinitdialog);
    DIALOGSTR_I(TR("Dialog", "Initialization:"), "");
    NULL_I();
#endif

    if (no_menudialogs_i18n > MAX_MENUDIALOGS_I18N) {
        fprintf(stderr, "MAX_MENUDIALOGS_I18N is set to an insufficiently low number, please increase it to %d\n", no_menudialogs_i18n);
        fflush(stderr);
        exit(1);
    }
#ifdef DEBUG
    printf("Filled %d widgets out of %d.\n", no_menudialogs_i18n,
           MAX_MENUDIALOGS_I18N);
    fflush(stdout);
#endif

}

#undef Register

/*
 * End of registering internationalized dialogs.
 */

#ifdef USE_SFFE
void uih_sffein(uih_context *c, const char *text);
void uih_sffeinitin(uih_context *c, const char *text);
#endif

static void uih_smoothmorph(struct uih_context *c, dialogparam *p)
{
    if (!c->playc)
        return;
    switch (p[0].dint) {
        case 0:
            c->playc->morphtimes[0] = p[1].dint;
            c->playc->morphtimes[1] = p[2].dint;
            break;
        case 1:
            c->playc->morphjuliatimes[0] = p[1].dint;
            c->playc->morphjuliatimes[1] = p[2].dint;
            break;
        case 2:
            c->playc->morphangletimes[0] = p[1].dint;
            c->playc->morphangletimes[1] = p[2].dint;
            break;
        case 3:
            c->playc->morphlinetimes[0] = p[1].dint;
            c->playc->morphlinetimes[1] = p[2].dint;
            break;
    }
}

static void uih_render(struct uih_context *c, dialogparam *d)
{

    if(fnames.size() == 0) {
        uih_error(c, "No file Selected");
        return;
    }
    if (d[2].dint <= 0 || d[2].dint > 4096) {
        uih_error(
            c,
            TR("Error",
               "renderanim: Width parameter must be positive integer in the range 0..4096"));
        return;
    }
    if (d[3].dint <= 0 || d[3].dint > 4096) {
        uih_error(
            c,
            TR("Error",
               "renderanim: Height parameter must be positive integer in the range 0..4096"));
        return;
    }
    if (d[4].number <= 0 || d[5].number <= 0) {
        uih_error(c,
                  TR("Error",
                     "renderanim: Invalid real width and height dimensions"));
        return;
    }
    if (d[6].number <= 0 || d[6].number >= 1000000) {
        uih_error(c, TR("Error", "renderanim: invalid framerate"));
        return;
    }
    if (d[7].dint && d[8].dint) {
        uih_error(
            c, TR("Error",
                  "renderanim: antialiasing not supported in 256 color mode"));
        return;
    }
    for(int i=0; i < (int)fnames.size(); i++) {

        QString hlpmsg = "Rendering (" + QString::number(i) + "/" +
                QString::number(fnames.size()) + ") " + fnames[i];
        uih_message(c, hlpmsg.toStdString().c_str());

        char* curr_file = strdup(fnames[i].toStdString().c_str());

        QString file_number = "_" + fnames[i].split("/").back().split(".").front() + "_";
        char* file_suffix = strdup(file_number.toStdString().c_str());
        char* base_name = (char *)malloc(strlen(d[1].dstring) + strlen(file_suffix) + 2);
        strcpy(base_name, d[1].dstring);
        strcat(base_name, file_suffix);

        uih_renderanimation(c, base_name, (xio_path)curr_file, d[2].dint,
                                d[3].dint, d[4].number, d[5].number,
                                (int)(1000000 / d[6].number),
        #ifdef STRUECOLOR24
                                d[7].dint ? C256 : TRUECOLOR24,
        #else
                                d[7].dint ? C256 : TRUECOLOR,
        #endif
                                d[8].dint, d[9].dint, c->letterspersec, NULL);
        free(base_name);
    }
}

static void uih_renderimg(struct uih_context *c, dialogparam *d)
{
    xio_file f = xio_wopen(".xaos_temp.xpf");
    if(!f) {
        uih_error(c, "Could not Render Image");
        return;
    }

    uih_save_position(c, f, 0);
    xio_constpath path = ".xaos_temp.xpf";

    if (d[1].dint <= 0 || d[1].dint > 4096) {
        uih_error(
            c,
            TR("Error",
               "renderanim: Width parameter must be positive integer in the range 0..4096"));
        return;
    }
    if (d[2].dint <= 0 || d[2].dint > 4096) {
        uih_error(
            c,
            TR("Error",
               "renderanim: Height parameter must be positive integer in the range 0..4096"));
        return;
    }
    if (d[3].number <= 0 || d[4].number <= 0) {
        uih_error(c,
                  TR("Error",
                     "renderanim: Invalid real width and height dimensions"));
        return;
    }
    if (d[5].dint && d[6].dint) {
        uih_error(
            c, TR("Error",
                  "renderanim: antialiasing not supported in 256 color mode"));
        return;
    }

    uih_renderanimation(c, d[0].dstring, (xio_path)path, d[1].dint,
                            d[2].dint, d[3].number, d[4].number,
                            (int)(1000000 / 30),
    #ifdef STRUECOLOR24
                            d[5].dint ? C256 : TRUECOLOR24,
    #else
                            d[5].dint ? C256 : TRUECOLOR,
    #endif
                            d[6].dint, 0, c->letterspersec, NULL);

    remove(".xaos_temp.xpf");
}

static menudialog *uih_getcolordialog(struct uih_context *c)
{
    if (c != NULL) {
        uih_colordialog[0].defint = c->color;
    }
    return (uih_colordialog);
}

static void uih_setcolor(struct uih_context *c, int color) { c->color = color; }

static menudialog *uih_getperturbationdialog(struct uih_context *c)
{
    if (c != NULL) {
        uih_perturbationdialog[0].deffloat = c->fcontext->bre;
        uih_perturbationdialog[0].deffloat2 = c->fcontext->bim;
    }
    return (uih_perturbationdialog);
}

static menudialog *uih_getjuliadialog(struct uih_context *c)
{
    if (c != NULL) {
        uih_juliadialog[0].deffloat = c->fcontext->pre;
        uih_juliadialog[0].deffloat2 = c->fcontext->pim;
    }
    return (uih_juliadialog);
}

static void uih_plview(struct uih_context *c, dialogparam *d)
{
    if (d[2].number <= 0 || d[3].number <= 0) {
        uih_error(c, TR("Error", "animateview: Invalid viewpoint"));
        return;
    }
    c->fcontext->s.cr = d[0].number;
    c->fcontext->s.ci = d[1].number;
    c->fcontext->s.rr = d[2].number;
    c->fcontext->s.ri = d[3].number;
    uih_newimage(c);
}

static void uih_plview2(struct uih_context *c, dialogparam *d)
{
    if (d[2].number <= 0 || d[3].number <= 0) {
        uih_error(c, TR("Error", "animateview: Invalid viewpoint"));
        return;
    }
    c->fcontext->s.cr = d[0].number;
    c->fcontext->s.ci = d[1].number;
    c->fcontext->s.rr = d[2].number;
    c->fcontext->s.ri = d[3].number;
    uih_animate_image(c);
}

static void uih_dview(struct uih_context *c, dialogparam *d)
{
    if (d[1].number <= 0) {
        uih_error(c, TR("Error", "Invalid viewpoint"));
        return;
    }
    c->fcontext->s.cr = d[0].dcoord[0];
    c->fcontext->s.ci = d[0].dcoord[1];
    c->fcontext->s.rr = d[1].number;
    c->fcontext->s.ri = d[1].number;
    uih_angle(c, d[2].number);
    uih_newimage(c);
}

static menudialog *uih_getviewdialog(struct uih_context *c)
{
    number_t xs, ys;
    if (c != NULL) {
        xs = c->fcontext->s.rr;
        ys = c->fcontext->s.ri * c->fcontext->windowwidth /
             c->fcontext->windowheight;
        uih_viewdialog[0].deffloat = c->fcontext->s.cr;
        uih_viewdialog[0].deffloat2 = c->fcontext->s.ci;
        uih_viewdialog[2].deffloat = c->fcontext->angle;
        if (xs > ys)
            uih_viewdialog[1].deffloat = c->fcontext->s.rr;
        else
            uih_viewdialog[1].deffloat = c->fcontext->s.ri;
    }
    return (uih_viewdialog);
}

static menudialog *uih_getlettersdialog(struct uih_context *c)
{
    if (c != NULL)
        uih_lettersdialog[0].defint = c->letterspersec;
    return (uih_lettersdialog);
}

static menudialog *uih_getiterdialog(struct uih_context *c)
{
    if (c != NULL)
        uih_iterdialog[0].defint = c->fcontext->maxiter;
    return (uih_iterdialog);
}

static menudialog *uih_getbailoutdialog(struct uih_context *c)
{
    if (c != NULL)
        uih_bailoutdialog[0].deffloat = c->fcontext->bailout;
    return (uih_bailoutdialog);
}

int defthreads = 0;

static menudialog *uih_getthreaddialog(struct uih_context *c)
{
    if (c != NULL)
        uih_threaddialog[0].defint = defthreads;
    return (uih_threaddialog);
}

void uih_setthreads(uih_context */*c*/, int threads)
{
    if (threads < 1)
        threads = 1;
    if (threads > MAXTHREADS)
        threads = MAXTHREADS;
    if (threads != defthreads) {
        QSettings settings;
        settings.setValue("MainWindow/threadCount", threads);
        QMessageBox msgBox;
        msgBox.setText(
            TR("Message", "XaoS must restart to change the thread count."));
        msgBox.setInformativeText(TR("Message", "Do you want to quit now?"));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        int ret = msgBox.exec();
        if (ret == QMessageBox::Yes) {
            exit(0);
        }
    }
}

static int uih_saveanimenabled(struct uih_context *c)
{
    if (c == NULL)
        return 0;
    return (c->save);
}

static menudialog *uih_getrotationdialog(struct uih_context *c)
{
    if (c != NULL)
        uih_rotationdialog[0].deffloat = c->rotationspeed;
    return (uih_rotationdialog);
}

#ifdef USE_SFFE
static menudialog *uih_getsffedialog(struct uih_context *c)
{
    if (c != NULL) {
        if (c->fcontext->userformula->expression)
            uih_sffedialog[0].defstr = c->fcontext->userformula->expression;
        else
            uih_sffedialog[0].defstr = USER_FORMULA;
    }
    return (uih_sffedialog);
}

static menudialog *uih_getsffeinitdialog(struct uih_context *c)
{
    if (c != NULL) {
        if (c->fcontext->userinitial->expression)
            uih_sffeinitdialog[0].defstr = c->fcontext->userinitial->expression;
        else
            uih_sffeinitdialog[0].defstr = "";
    }
    return (uih_sffeinitdialog);
}
#endif

static menudialog *uih_getpalettedialog(struct uih_context *uih)
{
    if (uih != NULL) {
        palettedialog[0].defint = uih->palettetype;
        palettedialog[1].defint = uih->paletteseed;
        palettedialog[2].defint = uih->paletteshift + uih->manualpaletteshift;
    }
    return (palettedialog);
}

static menudialog *uih_palettepickerdialog(struct uih_context *uih)
{
    return (palettepickerdialog);
}

static menudialog *uih_getpalettegradientdialog(struct uih_context *uih)
{
    if (uih != NULL) {
        palettegradientdialog[0].defint = 0;
    }
    return (palettegradientdialog);
}

static menudialog *uih_getcyclingdialog(struct uih_context *uih)
{
    if (uih != NULL)
        uih_cyclingdialog[0].defint = uih->cyclingspeed * uih->direction;
    return (uih_cyclingdialog);
}

static menudialog *uih_getspeeddialog(struct uih_context *uih)
{
    if (uih != NULL)
        uih_speeddialog[0].deffloat = uih->speedup / STEP;
    return (uih_speeddialog);
}

static void uih_setspeed(uih_context *c, number_t p)
{
    if (p >= 100)
        p = 1.0;
    if (p < 0)
        p = 0;
    c->speedup = STEP * p;
    c->maxstep = MAXSTEP * p;
}

static void uih_palette(struct uih_context *uih, dialogparam *p)
{
    int n1 = p[0].dint;
    int n2 = p[1].dint;
    int shift = p[2].dint;

    if (!n1) {
        uih_playdefpalette(uih, shift);
        return;
    }
    if (n1 < 1 || n1 > PALGORITHMS) {
        uih_error(uih, TR("Error", "Unknown palette type"));
    }
    if (uih->zengine->fractalc->palette == NULL)
        return;
    if (mkpalette(uih->zengine->fractalc->palette, n2, n1 - 1) != 0) {
        uih_newimage(uih);
    }
    uih->manualpaletteshift = 0;
    uih->palettetype = n1;
    uih->palettechanged = 1;
    uih->paletteseed = n2;
    if (shiftpalette(uih->zengine->fractalc->palette, shift)) {
        uih_newimage(uih);
    }
    uih->paletteshift = shift;
    uih->palettepickerenabled = 0;
}

static void uih_palettegradient(struct uih_context *uih, dialogparam *p)
{
    int n1 = uih->palettetype;
    int n2 = uih->paletteseed;
    int shift = uih->paletteshift;

    if (!n1) {
        uih_playdefpalette(uih, shift);
        return;
    }
    if (n1 < 1 || n1 > PALGORITHMS) {
        uih_error(uih, TR("Error", "Unknown palette type"));
    }
    if (uih->zengine->fractalc->palette == NULL)
        return;
    if (mkpalette(uih->zengine->fractalc->palette, n2, n1 - 1) != 0) {
        uih_newimage(uih);
    }
    uih->manualpaletteshift = 0;
    uih->palettetype = n1;
    uih->palettechanged = 1;
    uih->paletteseed = n2;
    if (shiftpalette(uih->zengine->fractalc->palette, shift)) {
        uih_newimage(uih);
    }
    uih->paletteshift = shift;
    uih->palettepickerenabled = 0;
}

static void uih_palettecolors(struct uih_context *uih, dialogparam *p){
    unsigned char colors[31][3];
    memset(colors, 0, sizeof (colors));
    for(int i=0; i < 31; i++) {
        rgb_t color;
        hextorgb(p[i].dstring, color);
        colors[i][0] = color[0];
        colors[i][1] = color[1];
        colors[i][2] = color[2];
    }
    mkcustompalette(uih->palette, colors);
    uih_newimage(uih);
    uih->palettepickerenabled = 1;
}

static void uih_palettepicker(struct uih_context *uih, dialogparam *p)
{
    uih_newimage(uih);
    uih->palettepickerenabled = 1;
}

static void uih_loadgpl(struct uih_context *uih, xio_constpath d)
{
    QFile *loadfile = new QFile(d);
    unsigned char colors[31][3];
    memset(colors, 0, sizeof (colors));

    if (loadfile->open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(loadfile);
        QStringList colorvals= in.readAll().split("\n");
        if((int)colorvals.size() != 36) {
            uih_error(uih, "Corrupted palette File");
            loadfile->close();
            return;
        }

        for(int i = 4; i < 35; i++) {
            QStringList currcolors = colorvals[i].split(QRegExp("\\s+"));
            int r = currcolors[0].toInt();
            int g = currcolors[1].toInt();
            int b = currcolors[2].toInt();

            if (r < 0 || r > 255 ||
                g < 0 || g > 255 ||
                b < 0 || b > 255) {
                uih_error(uih, "RGB out of range. Failed to load palette.");
                loadfile->close();
                return;
            }

            colors[i-4][0] = r;
            colors[i-4][1] = g;
            colors[i-4][2] = b;
        }
        mkcustompalette(uih->palette, colors);
        loadfile->close();
        char s[256];
        sprintf(s, TR("Message", "File %s opened."), d);
        uih_message(uih, s);

    } else {
        uih_error(uih, "Failed to open palette configuration");
        return;
    }

    uih_newimage(uih);
    uih->palettepickerenabled = 1;
}

static void uih_savegpl(struct uih_context *uih, xio_constpath d) {
    QFile *savefile = new QFile(d);
    unsigned char colors[31][3];

    if(savefile->open(QIODevice::WriteOnly | QIODevice::Text)) {
            getDEFSEGMENTColor(colors);
            QTextStream stream(savefile);
            stream << "GIMP Palette" << "\n";
            stream << "Name: XaoS_Palette" << "\n";
            stream << "Columns: 16" << "\n" << "#" << "\n";
            for(int i=0; i < 31; i++){
                char s[256];
                sprintf(s, "%3d %3d %3d", colors[i][0], colors[i][1], colors[i][2]);
                stream << s << "\t color_" << QString::number(i) << "\n";
            }
            savefile->close();
            char s[256];
            sprintf(s, TR("Message", "File %s saved."), d);
            uih_message(uih, s);

        } else {
        uih_error(uih, "Failed to save palette Configuration");
    }
    uih->palettepickerenabled = 1;
}

static int uih_rotateselected(struct uih_context *c, int n)
{
    if (c == NULL)
        return 0;
    if (!c->fastrotate)
        return !n;
    return (c->rotatemode == n);
}

static int uih_guessingselected(struct uih_context *c, int n)
{
    if (c == NULL)
        return 0;
    return (c->fcontext->range == n);
}

static int uih_fastmode(struct uih_context *c, int n)
{
    if (c == NULL)
        return 0;
    return (c->fastmode == n);
}

static int uih_periodicityselected(struct uih_context *c)
{
    if (c == NULL)
        return 0;
    return (c->fcontext->periodicity);
}

static void uih_periodicitysw(struct uih_context *c)
{
    uih_setperiodicity(c, c->fcontext->periodicity ^ 1);
}

static int uih_cyclingselected(struct uih_context *c)
{
    if (c == NULL)
        return 0;
    return (c->cycling && c->cyclingdirection == 1);
}

static int uih_rcyclingselected(struct uih_context *c)
{
    if (c == NULL)
        return 0;
    return (c->cycling && c->cyclingdirection == -1);
}

static void uih_cyclingsw(struct uih_context *c)
{
    // Andrew Stone: this fixes what I consider a bug - switching from Y to y
    // should keep cycling:
    if (c->cycling && c->cyclingdirection == -1)
        uih_cycling_off(c);
    c->cyclingdirection = 1;
    if (c->cycling)
        uih_cycling_off(c);
    else if (!uih_cycling_on(c))
        uih_error(c, TR("Error", "Initialization of color cycling failed.")),
            uih_message(c,
                        TR("Error", "Try to enable palette emulation filter"));
}

static void uih_rcyclingsw(struct uih_context *c)
{
    // Andrew Stone: this fixes what I consider a bug - switching from y to Y
    // should keep cycling:
    if (c->cycling && c->cyclingdirection == 1)
        uih_cycling_off(c);
    c->cyclingdirection = -1;
    if (c->cycling)
        uih_cycling_off(c);
    else if (!uih_cycling_on(c))
        uih_error(c, TR("Error", "Initialization of color cycling failed.")),
            uih_message(c,
                        TR("Error", "Try to enable palette emulation filter"));
}

static void uih_juliasw(struct uih_context *c)
{
    if (!c->juliamode)
        uih_enablejulia(c);
    else
        uih_disablejulia(c);
}

static int uih_juliaselected(struct uih_context *c)
{
    if (c == NULL)
        return 0;
    return (c->juliamode);
}

static int uih_mandelbrotselected(struct uih_context *c)
{
    if (c == NULL)
        return 0;
    return (c->fcontext->mandelbrot);
}

static void uih_mandelbrotsw(struct uih_context *c, number_t x, number_t y)
{
    c->fcontext->mandelbrot ^= 1;
    if (c->fcontext->mandelbrot == 0 && !c->juliamode) {
        c->fcontext->pre = x;
        c->fcontext->pim = y;
    } else
        uih_disablejulia(c);
    c->fcontext->version++;
    uih_newimage(c);
    uih_updatemenus(c, "uimandelbrot");
}

static int uih_autopilotselected(struct uih_context *c)
{
    if (c == NULL)
        return 0;
    return (c->autopilot);
}

static int uih_fixedstepselected(struct uih_context *c)
{
    if (c == NULL)
        return 0;
    return (c->fixedstep);
}

static void uih_persw(struct uih_context *c, number_t x, number_t y)
{
    if (c->fcontext->bre || c->fcontext->bim)
        uih_setperbutation(c, 0.0, 0.0);
    else
        uih_setperbutation(c, x, y);
    // printf(""); // fixme: some newer versions of gcc crashes without this
}

static int uih_perselected(struct uih_context *c)
{
    if (c == NULL)
        return 0;
    return (c->fcontext->bre || c->fcontext->bim);
}

static void uih_autopilotsw(struct uih_context *c)
{
    if (c->autopilot)
        uih_autopilot_off(c);
    else
        uih_autopilot_on(c);
}

static void uih_fixedstepsw(struct uih_context *c) { c->fixedstep ^= 1; }

static void uih_setxtextpos(uih_context *c, int p)
{
    uih_settextpos(c, p, c->ytextpos);
}

static int uih_xtextselected(uih_context *c, int p)
{
    if (c == NULL)
        return 0;
    return (c->xtextpos == p);
}

static void uih_setytextpos(uih_context *c, int p)
{
    uih_settextpos(c, c->xtextpos, p);
}

static int uih_ytextselected(uih_context *c, int p)
{
    if (c == NULL)
        return 0;
    return (c->ytextpos == p);
}

static void uih_menumkpalette(uih_context *c)
{
    char s[256];
    uih_mkpalette(c);
    sprintf(s, TR("Error", "Algorithm:%i seed:%i size:%i"), c->palettetype,
            c->paletteseed, c->zengine->fractalc->palette->size);
    uih_message(c, s);
}

static void uih_shiftpalette(uih_context *c, int shift)
{
    if (shiftpalette(c->zengine->fractalc->palette, shift)) {
        uih_newimage(c);
    }
    c->manualpaletteshift += shift;
}

static void uih_fshift(uih_context *c) { uih_shiftpalette(c, 1); }

static void uih_bshift(uih_context *c) { uih_shiftpalette(c, -1); }

static const menuitem *menuitems; /*XaoS menu specifications */
/* This structure is now empty. All static definitions have been moved
   to uih_registermenus_i18n() which fills up its own static array. */

/* Registering internationalized menus. See also include/xmenu.h
   for details. Note that MAX_MENUITEMS_I18N should be increased
   if more items will be added in future.

   On 2006-07-12 J.B. Langston wrote:

   The menu.c.diff file changes the MAX_MENUITEMS_I18N macro from
   200 to 250.  As of 3.2.1, the number of allocated menu items had
   been exceeded (there are 201 menu items now). This was causing
   the memory within the application to be clobbered, resulting
   in subtle bugs on the PowerPC platform (both X11 and OSX drivers).
   For example, rendering animations generated a segmentation fault.
   It's quite possible it could have been causing other subtle bugs
   elsewhere in the program. */

#define MAX_MENUITEMS_I18N 250
static menuitem menuitems_i18n[MAX_MENUITEMS_I18N];
int uih_no_menuitems_i18n;

void uih_registermenus_i18n(void)
{
    // Special version (currently it's OK):
    int no_menuitems_i18n = 0;
    SUBMENU_I("", NULL, TR("Menu", "Root menu"), "root");
    SUBMENU_I("", NULL, TR("Menu", "Animation root menu"), "animroot");
    SUBMENU_I("", NULL, TR("Menu", "Replay only commands"), "plc");
#define MP (MENUFLAG_NOMENU | MENUFLAG_NOOPTION)
    /* Commands suitable only for animation replay */
    SUBMENU_I("plc", NULL, TR("Menu", "Line drawing functions"), "linemenu");
    MENUDIALOG_I("linemenu", NULL, TR("Menu", "Line"), "line", MP, uih_line,
                 uih_linedialog);
    MENUDIALOG_I("linemenu", NULL, TR("Menu", "Morph line"), "morphline", MP,
                 uih_morphline, uih_linedialog);
    MENUDIALOG_I("linemenu", NULL, TR("Menu", "Morph last line"),
                 "morphlastline", MP, uih_morphlastline, uih_linedialog);
    MENUDIALOG_I("linemenu", NULL, TR("Menu", "Set line key"), "linekey", MP,
                 uih_setkey, uih_numdialog);
    MENUNOP_I("linemenu", NULL, TR("Menu", "Clear line"), "clearline", MP,
              uih_clear_line);
    MENUNOP_I("linemenu", NULL, TR("Menu", "Clear all lines"), "clearlines", MP,
              uih_clear_lines);
    SUBMENU_I("plc", NULL, TR("Menu", "Animation functions"), "animf");
    MENUDIALOG_I("animf", NULL, TR("Menu", "View"), "animateview", MP,
                 uih_plview2, uih_plviewdialog);
    MENUDIALOG_I("animf", NULL, TR("Menu", "Morph view"), "morphview", MP,
                 uih_playmorph, uih_plviewdialog);
    MENUDIALOG_I("animf", NULL, TR("Menu", "Morph julia"), "morphjulia", MP,
                 uih_playmorphjulia, uih_coorddialog);
    MENUDIALOG_I("animf", NULL, TR("Menu", "Move view"), "moveview", MP,
                 uih_playmove, uih_coorddialog);
    MENUDIALOG_I("animf", NULL, TR("Menu", "Morph angle"), "morphangle", MP,
                 uih_playmorphangle, uih_angledialog);
    MENUDIALOG_I("animf", NULL, TR("Menu", "Zoom center"), "zoomcenter", MP,
                 uih_zoomcenter, uih_coorddialog);
    MENUNOP_I("animf", NULL, TR("Menu", "Zoom"), "zoom", MP, uih_playzoom);
    MENUNOP_I("animf", NULL, TR("Menu", "Un-zoom"), "unzoom", MP,
              uih_playunzoom);
    MENUNOP_I("animf", NULL, TR("Menu", "Stop zooming"), "stop", MP,
              uih_playstop);
    MENUDIALOG_I("animf", NULL, TR("Menu", "Smooth morphing parameters"),
                 "smoothmorph", MP, uih_smoothmorph, uih_smoothmorphdialog);
    SUBMENU_I("plc", NULL, TR("Menu", "Timing functions"), "time");
    MENUDIALOG_I("time", NULL, TR("Menu", "Usleep"), "usleep", MP,
                 uih_playusleep, uih_timedialog);
    MENUNOP_I("time", NULL, TR("Menu", "Wait for text"), "textsleep", MP,
              uih_playtextsleep);
    MENUNOP_I("time", NULL, TR("Menu", "Wait for complete image"), "wait", MP,
              uih_playwait);
    MENUDIALOG_I("plc", NULL, TR("Menu", "Include file"), "load", MP,
                 uih_playload, loaddialog);
    MENUDIALOG_I("palette", NULL, TR("Menu", "Default palette"),
                 "defaultpalette", MP, uih_playdefpalette, uih_numdialog);
    MENUDIALOG_I("fractal", NULL, TR("Menu", "Formula"), "formula", MP,
                 uih_play_formula, uih_formuladialog);
    MENUDIALOG_I("ui", NULL, TR("Menu", "Maximal zooming step"), "maxstep", MP,
                 uih_setmaxstep, uih_fpdialog);
    MENUDIALOG_I("ui", NULL, TR("Menu", "Zooming speedup"), "speedup", MP,
                 uih_setspeedup, uih_fpdialog);
    MENUDIALOG_I("mfilter", NULL, TR("Menu", "Filter"), "filter", MP,
                 uih_playfilter, uih_filterdialog);
#undef MP
#define UI (MENUFLAG_NOPLAY | MENUFLAG_NOOPTION)
    MENUCDIALOG_I("ui", NULL, TR("Menu", "Letters per second"), "letterspersec",
                  MENUFLAG_NOMENU, uih_letterspersec, uih_getlettersdialog);
    MENUCDIALOG_I("uia", NULL, TR("Menu", "Letters per second"), "letters", UI,
                  uih_letterspersec, uih_getlettersdialog);
    MENUNOP_I("uia", "z", TR("Menu", "Interrupt"), "animinterrupt",
              MENUFLAG_INTERRUPT | MENUFLAG_INCALC, uih_interrupt);
    MENUSEPARATOR_I("ui");
    MENUNOPCB_I("ui", "/", TR("Menu", "Status"), "status", MENUFLAG_INCALC,
                uih_status, uih_statusenabled); /*FIXME: add also ? as key */

    MENUNOPCB_I("ui", "l", TR("Menu", "Ministatus"), "ministatus",
                MENUFLAG_INCALC, uih_ministatus, uih_ministatusenabled);
    MENUNOPCB_I("ui", "g", TR("Menu", "Cartesian Grid"), "cartesiangrid",
                MENUFLAG_INCALC, uih_cartesiangrid, uih_cartesiangridenabled);
    MENUSEPARATOR_I("ui");
    MENUSEPARATOR_I("uia");
    MENUNOPCB_I("uia", "/", TR("Menu", "Status"), "animstatus",
                UI | MENUFLAG_INCALC, uih_status,
                uih_statusenabled); /*FIXME: add also ? as key */

    MENUNOPCB_I("uia", "l", TR("Menu", "Ministatus"), "animministatus",
                UI | MENUFLAG_INCALC, uih_ministatus, uih_ministatusenabled);
    MENUNOPCB_I("uia", "g", TR("Menu", "Cartesian Grid"), "animcartesiangrid",
                MENUFLAG_INCALC, uih_cartesiangrid, uih_cartesiangridenabled);
    MENUSEPARATOR_I("uia");
    SUBMENU_I("root", "s", TR("Menu", "File"), "file");
    SUBMENU_I("root", NULL, TR("Menu", "Edit"), "edit");
    SUBMENU_I("root", NULL, TR("Menu", "Fractal"), "fractal");
    SUBMENU_I("root", NULL, TR("Menu", "Calculation"), "calc");
    SUBMENU_I("root", "e", TR("Menu", "Filters"), "mfilter");
    SUBMENU_I("root", NULL, TR("Menu", "Action"), "action");
    SUBMENU_I("root", NULL, TR("Menu", "View"), "ui");
    SUBMENU_I("root", NULL, TR("Menu", "Help"), "helpmenu");
    SUBMENU_I("helpmenu", NULL, TR("Menu", "Tutorials"), "tutor");
    SUBMENUNOOPT_I("animroot", "f", TR("Menu", "File"), "file");
    // You cannot have menu items directly on the root menu in some OS
    // So we put the "Stop Replay" item in the UI menu instead
    MENUSEPARATOR_I("uia");
    MENUNOP_I("uia", "s", TR("Menu", "Stop replay"), "stopreplay",
              UI | MENUFLAG_INTERRUPT, uih_replaydisable);
    SUBMENUNOOPT_I("animroot", NULL, TR("Menu", "View"), "uia");
    SUBMENUNOOPT_I("animroot", NULL, TR("Menu", "Help"), "helpmenu");
    MENUDIALOG_I("action", "!", TR("Menu", "Command"), "command", UI,
                 uih_command, dcommand);
    MENUDIALOG_I("action", NULL, TR("Menu", "Play string"), "playstr",
                 MENUFLAG_NOMENU, uih_playstr, dcommand);
    MENUSEPARATOR_I("action");
    MENUNOP_I("action", NULL, TR("Menu", "Clear screen"), "clearscreen",
              MENUFLAG_NOOPTION, uih_clearscreen);
    MENUNOP_I("action", NULL, TR("Menu", "Display fractal"), "display",
              MENUFLAG_NOOPTION, uih_display);
    MENUSEPARATOR_I("action");
    MENUDIALOG_I("action", NULL, TR("Menu", "Display text"), "text", 0,
                 uih_text, dtextparam); /*FIXME: Should allow multiline */

    MENUCDIALOG_I("action", NULL, TR("Menu", "Text color"), "color", 0,
                  uih_setcolor, uih_getcolordialog);
    SUBMENU_I("action", NULL, TR("Menu", "Horizontal text position"),
              "xtextpos");
    SUBMENU_I("action", NULL, TR("Menu", "Vertical text position"), "ytextpos");
    MENUDIALOG_I("action", NULL, TR("Menu", "Text position"), "textposition",
                 MENUFLAG_NOMENU | MENUFLAG_INCALC, uih_playtextpos,
                 uih_textposdialog);
    MENUDIALOG_I("action", NULL, TR("Menu", "Message"), "message",
                 MENUFLAG_NOMENU, uih_playmessage, dtextparam);
    /* The following 6 menu options should not be translated. The example
       files heavily use these constants and lots of examples will not work
       anymore... :-(  Anyway, this should be fixed somehow. */

    MENUINTRB_I("ytextpos", NULL, "Up", "ytextup", UI, uih_setytextpos,
                UIH_TEXTTOP, uih_ytextselected);
    MENUINTRB_I("ytextpos", NULL, "Middle", "ytextmiddle", UI, uih_setytextpos,
                UIH_TEXTMIDDLE, uih_ytextselected);
    MENUINTRB_I("ytextpos", NULL, "Bottom", "ytextbottom", UI, uih_setytextpos,
                UIH_TEXTBOTTOM, uih_ytextselected);
    MENUINTRB_I("xtextpos", NULL, "Left", "xtextleft", UI, uih_setxtextpos,
                UIH_TEXTLEFT, uih_xtextselected);
    MENUINTRB_I("xtextpos", NULL, "Center", "xtextcenter", UI, uih_setxtextpos,
                UIH_TEXTCENTER, uih_xtextselected);
    MENUINTRB_I("xtextpos", NULL, "Right", "xtexteight", UI, uih_setxtextpos,
                UIH_TEXTRIGHT, uih_xtextselected);
    MENUNOP_I("file", NULL, TR("Menu", "New"), "initstate", 0, uih_initstate);
    MENUDIALOG_I("file", NULL, TR("Menu", "Open"), "loadpos",
                 MENUFLAG_INTERRUPT | MENUFLAG_NOPLAY, uih_loadfile,
                 loaddialog);
    MENUDIALOG_I("file", NULL, TR("Menu", "Save"), "savepos", 0,
                 uih_saveposfile, saveposdialog);
    SUBMENU_I("file", NULL, TR("Menu", "Save as"), "saveas");
    MENUDIALOG_I("saveas", NULL, TR("Menu", "PNG"), "saveimg", 0,
                 uih_savepngfile, saveimgdialog);
    MENUSEPARATOR_I("file")
    MENUDIALOGCB_I("file", NULL, TR("Menu", "Record"), "record", 0,
                   uih_saveanimfile, saveanimdialog, uih_saveanimenabled);
    MENUDIALOG_I("file", NULL, TR("Menu", "Replay"), "play",
                 MENUFLAG_INTERRUPT | MENUFLAG_NOPLAY, uih_playfile,
                 playdialog);
    MENUSEPARATOR_I("file");
    MENUDIALOG_I("file", NULL, TR("Menu", "Render"), "renderanim", UI,
                 uih_render, uih_renderdialog);
    MENUDIALOG_I("file", NULL, TR("Menu", "Render Image"), "renderimg", UI,
                 uih_renderimg, uih_renderimgdialog);
    MENUSEPARATOR_I("file");
    MENUNOP_I("file", NULL, TR("Menu", "Load random example"), "loadexample",
              MENUFLAG_INTERRUPT, uih_loadexample);
    MENUNOP_I("file", NULL, TR("Menu", "Save configuration"), "savecfg", 0,
              uih_savecfg);
    MENUSEPARATOR_I("file");
    MENUNOP_I("edit", "u", TR("Menu", "Undo"), "undo",
              MENUFLAG_INTERRUPT | MENUFLAG_NOPLAY | MENUFLAG_NOOPTION,
              uih_undo);
    MENUNOP_I("edit", NULL, TR("Menu", "Redo"), "redo",
              MENUFLAG_INTERRUPT | MENUFLAG_NOPLAY | MENUFLAG_NOOPTION,
              uih_redo);
    SUBMENU_I("fractal", NULL, TR("Menu", "Formulae"), "mformula");
    SUBMENU_I("fractal", NULL, TR("Menu", "More formulae"), "oformula");

#ifdef USE_SFFE
    /*FIXME: Should allow multiline */
    MENUSEPARATOR_I("fractal");
    MENUCDIALOG_I("fractal", NULL, TR("Menu", "User formula"), "usrform", 0,
                  uih_sffein, uih_getsffedialog);
    MENUCDIALOG_I("fractal", NULL, TR("Menu", "User initialization"),
                  "usrformInit", 0, uih_sffeinitin, uih_getsffeinitdialog);
#endif

    MENUSEPARATOR_I("fractal");
    SUBMENU_I("fractal", "f", TR("Menu", "Incoloring mode"), "mincoloring");
    SUBMENU_I("fractal", "c", TR("Menu", "Outcoloring mode"), "moutcoloring");
    SUBMENU_I("fractal", "i", TR("Menu", "Plane"), "mplane");
    SUBMENU_I("fractal", NULL, TR("Menu", "Palette"), "palettemenu");
    MENUSEPARATOR_I("fractal");
    MENUCDIALOGCB_I(
        "fractal", "m", TR("Menu", "Mandelbrot mode"), "uimandelbrot",
        MENUFLAG_DIALOGATDISABLE | MENUFLAG_INTERRUPT | UI, uih_mandelbrotsw,
        uih_getjuliadialog, uih_mandelbrotselected);
    MENUDIALOG_I("fractal", NULL, TR("Menu", "Julia mode"), "julia",
                 MENUFLAG_NOMENU | MENUFLAG_INTERRUPT, uih_playjulia,
                 uih_juliamodedialog);
    MENUNOPCB_I("fractal", "j", TR("Menu", "Fast julia mode"), "fastjulia", 0,
                uih_juliasw, uih_juliaselected);
    MENUSEPARATOR_I("fractal");
    MENUCDIALOG_I("fractal", NULL, TR("Menu", "View"), "uiview",
                  MENUFLAG_INTERRUPT | UI, uih_dview, uih_getviewdialog);
    MENUDIALOG_I("fractal", NULL, TR("Menu", "View"), "view",
                 MENUFLAG_NOMENU | MENUFLAG_INTERRUPT, uih_plview,
                 uih_plviewdialog);
    SUBMENU_I("fractal", "o", TR("Menu", "Rotation"), "rotate");
    MENUDIALOG_I("fractal", NULL, TR("Menu", "Set angle"), "angle",
                 MENUFLAG_NOMENU | MENUFLAG_INTERRUPT, uih_angle,
                 uih_angledialog);
    MENUDIALOG_I("fractal", NULL, TR("Menu", "Set plane"), "plane",
                 MENUFLAG_NOMENU | MENUFLAG_INTERRUPT, uih_setplane,
                 uih_numdialog);
    MENUDIALOG_I("fractal", NULL, TR("Menu", "Inside coloring mode"),
                 "incoloring", MENUFLAG_NOMENU | MENUFLAG_INTERRUPT,
                 uih_setincoloringmode, uih_numdialog);
    MENUDIALOG_I("fractal", NULL, TR("Menu", "Outside coloring mode"),
                 "outcoloring", MENUFLAG_NOMENU | MENUFLAG_INTERRUPT,
                 uih_setoutcoloringmode, uih_numdialog);
    MENUDIALOG_I("fractal", NULL, TR("Menu", "Inside truecolor coloring mode"),
                 "intcoloring", MENUFLAG_NOMENU | MENUFLAG_INTERRUPT,
                 uih_setintcolor, uih_numdialog);
    MENUDIALOG_I("fractal", NULL, TR("Menu", "Outside truecolor coloring mode"),
                 "outtcoloring", MENUFLAG_NOMENU | MENUFLAG_INTERRUPT,
                 uih_setouttcolor, uih_numdialog);
    MENUDIALOG_I("fractal", NULL, TR("Menu", "Julia seed"), "juliaseed",
                 MENUFLAG_NOMENU | MENUFLAG_INTERRUPT, uih_setjuliaseed,
                 uih_coorddialog);
    MENUNOP_I("palettemenu", "d", TR("Menu", "Default palette"), "defpalette",
              0, uih_mkdefaultpalette);
    MENUNOP_I("palettemenu", "p", TR("Menu", "Random palette"), "randompalette",
              0, uih_menumkpalette);
    MENUCDIALOG_I("", NULL, TR("Menu", "Custom palette"), "palette",
                  0, uih_palette, uih_getpalettedialog); //This is a placeholder menu
    MENUCDIALOG_I("palettemenu", NULL, TR("Menu", "Custom palette"), "palettegradient",
                  0, uih_palettegradient, uih_getpalettegradientdialog);
    MENUSEPARATOR_I("palettemenu");
    MENUDIALOG_I("fractal", NULL, TR("Menu", "Palette Colors"), "palettecolors",
                 MENUFLAG_NOMENU | MENUFLAG_INTERRUPT, uih_palettecolors,
                 uih_palettecolorsdialog);
    MENUCDIALOG_I("palettemenu", "x", TR("Menu", "Palette Editor"), "palettepicker",
                  0, uih_palettepicker, uih_palettepickerdialog);
    MENUDIALOG_I("palettemenu", NULL, TR("Menu", "Load Palette Config"), "loadgpl",
                 0, uih_loadgpl, loadgpldialog);
    MENUDIALOG_I("palettemenu", NULL, TR("Menu", "Save Palette Config"), "savegpl",
                 0, uih_savegpl, savegpldialog);
    MENUSEPARATOR_I("palettemenu");
    MENUNOPCB_I("palettemenu", "y", TR("Menu", "Color cycling"), "cycling", 0,
                uih_cyclingsw, uih_cyclingselected);
    MENUNOPCB_I("palettemenu", "Y", TR("Menu", "Reversed color cycling"),
                "rcycling", MENUFLAG_NOOPTION | MENUFLAG_NOPLAY, uih_rcyclingsw,
                uih_rcyclingselected);
    MENUCDIALOG_I("palettemenu", NULL, TR("Menu", "Color cycling speed"),
                  "cyclingspeed", 0, uih_setcycling, uih_getcyclingdialog);
    MENUSEPARATOR_I("palettemenu");
    MENUDIALOG_I("palettemenu", NULL, TR("Menu", "Shift palette"),
                 "shiftpalette", 0, uih_shiftpalette, uih_shiftdialog);
    MENUNOP_I("palettemenu", "+", TR("Menu", "Shift one forward"), "fshift",
              MENUFLAG_NOOPTION | MENUFLAG_NOPLAY, uih_fshift);
    MENUNOP_I("palettemenu", "-", TR("Menu", "Shift one backward"), "bshift",
              MENUFLAG_NOOPTION | MENUFLAG_NOPLAY, uih_bshift);
    SUBMENU_I("calc", NULL, TR("Menu", "Solid guessing"), "mguess");
    MENUINTRB_I("mguess", NULL, TR("Menu", "Disable solid guessing"), "noguess",
                UI, uih_setguessing, 1, uih_guessingselected);
    MENUSEPARATOR_I("mguess");
    MENUINTRB_I("mguess", NULL, TR("Menu", "Guess 2x2 rectangles"), "guess2",
                UI, uih_setguessing, 2, uih_guessingselected);
    MENUINTRB_I("mguess", NULL, TR("Menu", "Guess 3x3 rectangles"), "guess3",
                UI, uih_setguessing, 3, uih_guessingselected);
    MENUINTRB_I("mguess", NULL, TR("Menu", "Guess 4x4 rectangles"), "guess4",
                UI, uih_setguessing, 4, uih_guessingselected);
    MENUINTRB_I("mguess", NULL, TR("Menu", "Guess 5x5 rectangles"), "guess5",
                UI, uih_setguessing, 5, uih_guessingselected);
    MENUINTRB_I("mguess", NULL, TR("Menu", "Guess 6x6 rectangles"), "guess6",
                UI, uih_setguessing, 6, uih_guessingselected);
    MENUINTRB_I("mguess", NULL, TR("Menu", "Guess 7x7 rectangles"), "guess7",
                UI, uih_setguessing, 7, uih_guessingselected);
    MENUINTRB_I("mguess", NULL, TR("Menu", "Guess 8x8 rectangles"), "guess8",
                UI, uih_setguessing, 8, uih_guessingselected);
    MENUINTRB_I("mguess", NULL, TR("Menu", "Guess unlimited rectangles"),
                "guessall", UI, uih_setguessing, 2048, uih_guessingselected);
    SUBMENU_I("calc", NULL, TR("Menu", "Dynamic resolution"), "dynamic");
    MENUNOPCB_I("calc", "k", TR("Menu", "Periodicity checking"), "periodicity",
                0, uih_periodicitysw, uih_periodicityselected);
    MENUSEPARATOR_I("calc");
    MENUCDIALOG_I("calc", NULL, TR("Menu", "Threads"), "threads",
                  MENUFLAG_INTERRUPT, uih_setthreads, uih_getthreaddialog);
    MENUCDIALOG_I("calc", NULL, TR("Menu", "Iterations"), "maxiter",
                  MENUFLAG_INTERRUPT, uih_setmaxiter, uih_getiterdialog);
    MENUCDIALOG_I("calc", NULL, TR("Menu", "Bailout"), "bailout",
                  MENUFLAG_INTERRUPT, uih_setbailout, uih_getbailoutdialog);
    MENUCDIALOGCB_I("calc", "b", TR("Menu", "Perturbation"), "uiperturbation",
                    MENUFLAG_INTERRUPT | UI, uih_persw,
                    uih_getperturbationdialog, uih_perselected);
    MENUCDIALOG_I("calc", NULL, TR("Menu", "Perturbation"), "perturbation",
                  MENUFLAG_NOMENU | MENUFLAG_INTERRUPT, uih_setperbutation,
                  uih_getperturbationdialog);
    MENUSEPARATOR_I("calc");
    MENUCDIALOG_I("calc", NULL, TR("Menu", "Zooming speed"), "speed", 0,
                  uih_setspeed, uih_getspeeddialog);
    MENUNOPCB_I("calc", NULL, TR("Menu", "Fixed step"), "fixedstep", 0,
                uih_fixedstepsw, uih_fixedstepselected);
    MENUSEPARATOR_I("calc");
    MENUDIALOG_I("calc", NULL, TR("Menu", "Solid guessing range"), "range",
                 MENUFLAG_NOMENU, uih_setguessing, uih_numdialog);
    MENUINTRB_I("rotate", NULL, TR("Menu", "Disable rotation"), "norotate", UI,
                uih_rotate, 0, uih_rotateselected);
    MENUSEPARATOR_I("rotate");
    MENUINTRB_I("rotate", NULL, TR("Menu", "Continuous rotation"), "controtate",
                UI, uih_rotate, ROTATE_CONTINUOUS, uih_rotateselected);
    MENUINTRB_I("rotate", NULL, TR("Menu", "Rotate by mouse"), "mouserotate",
                UI, uih_rotate, ROTATE_MOUSE, uih_rotateselected);
    MENUCDIALOG_I("rotate", NULL, TR("Menu", "Rotation speed"), "rotationspeed",
                  0, uih_rotationspeed, uih_getrotationdialog);
    MENUDIALOG_I("rotate", NULL, TR("Menu", "Automatic rotation"), "autorotate",
                 MENUFLAG_NOMENU, uih_playautorotate, uih_autorotatedialog);
    MENUDIALOG_I("rotate", NULL, TR("Menu", "Fast rotation mode"), "fastrotate",
                 MENUFLAG_NOMENU, (funcptr)uih_fastrotate,
                 uih_fastrotatedialog);
    MENUSEPARATOR_I("calc");
    MENUNOP_I("calc", "r", TR("Menu", "Recalculate"), "recalculate", 0,
              uih_recalculate);
    MENUNOP_I("calc", "z", TR("Menu", "Interrupt"), "interrupt",
              MENUFLAG_INTERRUPT | MENUFLAG_INCALC, uih_interrupt);
    MENUINTRB_I("dynamic", NULL, TR("Menu", "Disable dynamic resolution"),
                "nodynamic", UI, uih_setfastmode, 1, uih_fastmode);
    MENUSEPARATOR_I("dynamic");
    MENUINTRB_I("dynamic", NULL, TR("Menu", "Use only during animation"),
                "dynamicanimation", UI, uih_setfastmode, 2, uih_fastmode);
    MENUINTRB_I("dynamic", NULL, TR("Menu", "Use also for new images"),
                "dynamicnew", UI, uih_setfastmode, 3, uih_fastmode);
    MENUDIALOG_I("dynamic", NULL, TR("Menu", "Dynamic resolution mode"),
                 "fastmode", MENUFLAG_NOMENU, uih_setfastmode,
                 uih_fastmodedialog);
    MENUNOPCB_I("ui", "a", TR("Menu", "Autopilot"), "autopilot", 0,
                uih_autopilotsw, uih_autopilotselected);
    MENUSEPARATOR_I("ui");
    MENUNOPCB_I("ui", "v", TR("Menu", "Hide Messages"), "inhibittextoutput", 0,
                uih_inhibittextsw, uih_inhibittextselected);
    /* Language selection is not sensible anymore if i18n is used: */
    SUBMENU_I("tutor", NULL, TR("Menu", "An introduction to fractals"),
              "intro");
    SUBMENU_I("tutor", NULL, TR("Menu", "XaoS features overview"), "features");
    SUBMENU_I("tutor", NULL, TR("Menu", "Math behind fractals"), "fmath");
    SUBMENU_I("tutor", NULL, TR("Menu", "Other fractal types in XaoS"),
              "otherf");
    SUBMENU_I("tutor", NULL, TR("Menu", "What's new?"), "new");
    /* Language selection is not sensible anymore if i18n is used: */
    TUTOR_I("intro", TR("Menu", "Whole story"), "fractal.xaf");
    MENUSEPARATOR_I("intro");
    TUTOR_I("intro", TR("Menu", "Introduction"), "intro.xaf");
    TUTOR_I("intro", TR("Menu", "Mandelbrot set"), "mset.xaf");
    TUTOR_I("intro", TR("Menu", "Julia set"), "julia.xaf");
    TUTOR_I("intro", TR("Menu", "Higher power Mandelbrots"), "power.xaf");
    TUTOR_I("intro", TR("Menu", "Newton's method"), "newton.xaf");
    TUTOR_I("intro", TR("Menu", "Barnsley's formula"), "barnsley.xaf");
    TUTOR_I("intro", TR("Menu", "Phoenix"), "phoenix.xaf");
    TUTOR_I("intro", TR("Menu", "Octo"), "octo.xaf");
    TUTOR_I("intro", TR("Menu", "Magnet"), "magnet.xaf");
    TUTOR_I("features", TR("Menu", "All features"), "features.xaf");
    MENUSEPARATOR_I("features");
    TUTOR_I("features", TR("Menu", "Outcoloring modes"), "outcolor.xaf");
    TUTOR_I("features", TR("Menu", "Incoloring modes"), "incolor.xaf");
    TUTOR_I("features", TR("Menu", "True-color coloring modes"), "truecol.xaf");
    TUTOR_I("features", TR("Menu", "Filters"), "filter.xaf");
    TUTOR_I("features", TR("Menu", "Planes"), "plane.xaf");
    TUTOR_I("features", TR("Menu", "Animations and position files"),
            "anim.xaf");
    TUTOR_I("features", TR("Menu", "Perturbation"), "pert.xaf");
    TUTOR_I("features", TR("Menu", "Random palettes"), "palette.xaf");
    TUTOR_I("features", TR("Menu", "Other noteworthy features"), "other.xaf");
    TUTOR_I("fmath", TR("Menu", "Whole story"), "fmath.xaf");
    MENUSEPARATOR_I("fmath");
    TUTOR_I("fmath", TR("Menu", "The definition and fractal dimension"),
            "dimension.xaf");
    TUTOR_I("fmath", TR("Menu", "Escape time fractals"), "escape.xaf");
    TUTOR_I("otherf", TR("Menu", "Other fractal types in XaoS"), "otherfr.xaf");
    MENUSEPARATOR_I("otherf");
    TUTOR_I("otherf", TR("Menu", "Triceratops and Catseye fractals"),
            "trice.xaf");
    TUTOR_I("otherf", TR("Menu", "Mandelbar, Lambda, Manowar and Spider"),
            "fourfr.xaf");
    TUTOR_I("otherf", TR("Menu", "Sierpinski Gasket, S.Carpet, Koch Snowflake"),
            "classic.xaf");
    TUTOR_I("new", TR("Menu", "What's new in 3.0?"), "new30.xaf");
    TUTOR_I("new", TR("Menu", "What's new in 4.0?"), "new40.xaf");
    if (no_menuitems_i18n > MAX_MENUITEMS_I18N) {
        fprintf(stderr, "MAX_MENUITEMS_I18N is set to an insufficiently low number, please increase it to %d\n", no_menuitems_i18n);
        fflush(stderr);
        exit(1);
    }
#ifdef DEBUG
    printf("Filled %d menu items out of %d.\n", no_menuitems_i18n,
           MAX_MENUITEMS_I18N);
#endif
    menu_add(menuitems_i18n, no_menuitems_i18n);
    uih_no_menuitems_i18n = no_menuitems_i18n;
}

static const menuitem menuitems2[] = {
    SUBMENU("mincoloring", NULL, "True-color incoloring mode", "tincoloring"),
    SUBMENU("moutcoloring", NULL, "True-color outcoloring mode",
            "toutcoloring")};

static int uih_selectedformula(struct uih_context *c, int n)
{
    if (c == NULL)
        return 0;
    return (c->fcontext->currentformula == formulas + n);
}

static int uih_selectedincoloring(struct uih_context *c, int n)
{
    if (c == NULL)
        return 0;
    return (c->fcontext->incoloringmode == n);
}

static void uih_setintruecolor(struct uih_context *c, int n)
{
    uih_setincoloringmode(c, 10);
    uih_setintcolor(c, n);
}

static int uih_selectedintcoloring(struct uih_context *c, int n)
{
    if (c == NULL)
        return 0;
    return (c->fcontext->intcolor == n);
}

static int uih_selectedoutcoloring(struct uih_context *c, int n)
{
    if (c == NULL)
        return 0;
    return (c->fcontext->coloringmode == n);
}

static int uih_selectedplane(struct uih_context *c, int n)
{
    if (c == NULL)
        return 0;
    return (c->fcontext->plane == n);
}

static void uih_setouttruecolor(struct uih_context *c, int n)
{
    uih_setoutcoloringmode(c, 10);
    uih_setouttcolor(c, n);
}

static int uih_selectedouttcoloring(struct uih_context *c, int n)
{
    if (c == NULL)
        return 0;
    return (c->fcontext->outtcolor == n);
}

static int uih_filterenabled(struct uih_context *c, int n)
{
    if (c == NULL)
        return 0;
    return (c->filter[n] != NULL);
}

static void uih_filtersw(struct uih_context *c, int n)
{
    if (c->filter[n] != NULL)
        uih_disablefilter(c, n);
    else
        uih_enablefilter(c, n);
}

static menuitem *formulaitems;
static menuitem *filteritems;
void uih_registermenus(void)
{
    char keys[2];
    menuitem *item;
    int i;
    menu_add(menuitems, NITEMS(menuitems));
    formulaitems = item = (menuitem *)malloc(sizeof(menuitem) * nformulas);

    /* This code automatically generates code for fractal, incoloring and other
     * menus*/
    for (i = 0; i < nformulas; i++) {
        if (i < nmformulas) {
            item[i].menuname = "mformula";
        } else {
            item[i].menuname = "oformula";
        }
        if (i < 9)
            keys[0] = '1' + i;
        else if (i == 9)
            keys[0] = '0';
        else
            keys[0] = '7' + i;
        keys[1] = 0;
        item[i].key = strdup(keys);
        item[i].type = MENU_INT;
        item[i].flags = MENUFLAG_RADIO | MENUFLAG_INTERRUPT | MENUFLAG_NOPLAY;
        item[i].iparam = i;
        item[i].name = formulas[i].name[!formulas[i].mandelbrot];
        item[i].shortname = formulas[i].shortname;
        item[i].function = (void (*)(void))uih_setformula;
        item[i].control = (int (*)(void))uih_selectedformula;
    }
    menu_add(item, nformulas);

    menu_genernumbered(INCOLORING - 1, "mincoloring", incolorname, NULL,
                       MENU_INT, UI | MENUFLAG_RADIO | MENUFLAG_INTERRUPT,
                       uih_setincoloringmode, uih_selectedincoloring, "in");

    menu_genernumbered(TCOLOR - 1, "tincoloring", tcolorname, NULL, MENU_INT,
                       UI | MENUFLAG_RADIO | MENUFLAG_INTERRUPT,
                       uih_setintruecolor, uih_selectedintcoloring, "int");

    menu_genernumbered(OUTCOLORING - 1, "moutcoloring", outcolorname, NULL,
                       MENU_INT, UI | MENUFLAG_RADIO | MENUFLAG_INTERRUPT,
                       uih_setoutcoloringmode, uih_selectedoutcoloring, "out");

    menu_genernumbered(TCOLOR - 1, "toutcoloring", tcolorname, NULL, MENU_INT,
                       UI | MENUFLAG_RADIO | MENUFLAG_INTERRUPT,
                       uih_setouttruecolor, uih_selectedouttcoloring, "outt");

    {
        int i;
        for (i = 0; planename[i] != NULL; i++)
            ;
        menu_genernumbered(i, "mplane", planename, NULL, MENU_INT,
                           UI | MENUFLAG_RADIO | MENUFLAG_INTERRUPT,
                           uih_setplane, uih_selectedplane, "plane");
    }
    filteritems = item = (menuitem *)malloc(sizeof(menuitem) * uih_nfilters);

    /* This code automatically generates code for fractal, incoloring and other
     * menus*/
    for (i = 0; i < uih_nfilters; i++) {
        item[i].menuname = "mfilter";
        item[i].key = NULL;
        item[i].type = MENU_INT;
        item[i].flags =
            MENUFLAG_CHECKBOX | MENUFLAG_INTERRUPT | MENUFLAG_NOPLAY;
        item[i].iparam = i;
        item[i].name = uih_filters[i]->name;
        item[i].shortname = uih_filters[i]->shortname;
        if (!strcmp(item[i].shortname, "palette"))
            item[i].shortname = "palettef";
        /*this is one name collision because of ugly historical reasons */
        item[i].function = (void (*)(void))uih_filtersw;
        item[i].control = (int (*)(void))uih_filterenabled;
    }
    menu_add(item, uih_nfilters);

    menu_add(menuitems2, NITEMS(menuitems2));
}

void uih_unregistermenus(void)
{
    menu_delete(menuitems, NITEMS(menuitems));
    menu_delete(menuitems_i18n, uih_no_menuitems_i18n);

    menu_delete(formulaitems, nformulas);
    free(formulaitems);

    menu_delnumbered(INCOLORING - 1, "in");

    menu_delnumbered(TCOLOR - 1, "int");

    menu_delnumbered(OUTCOLORING - 1, "out");

    menu_delnumbered(TCOLOR - 1, "outt");
    {
        int i;
        for (i = 0; planename[i] != NULL; i++)
            ;
        menu_delnumbered(i, "plane");
    }

    menu_delete(filteritems, uih_nfilters);
    free(filteritems);

    menu_delete(menuitems2, NITEMS(menuitems2));
}

#ifdef USE_SFFE
void uih_sffein(uih_context *c, const char *text)
{
    // Keep only top 10 entries
    QSettings settings;
    QStringList values = settings.value("Formulas/UserFormulas").toStringList();
    values.push_front(text);
    while (values.size() > 10) values.pop_back();
    settings.setValue("Formulas/UserFormulas", values);

    uih_sffeset(c, c->fcontext->userformula, text);
}

void uih_sffeinitin(uih_context *c, const char *text)
{
    uih_sffeset(c, c->fcontext->userinitial, text);
}
#endif
