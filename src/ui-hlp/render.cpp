
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstdarg>
#ifndef _WIN32
#include <unistd.h>
#endif

#include "config.h"
#include "filter.h"
#include "fractal.h"
#include "ui_helper.h"
#include "misc-f.h"
#include "xmenu.h"
#include "xerror.h"
#include "i18n.h"

#define SILENT 0
#define ERRORS 1
#define MESSAGES 2
#define ALL 3
static int noiselevel;
/*static struct uih_context *uih, *gc;*/
static struct uih_context *gc;
static struct uih_context *uih;
static int newline = 1;
static int interrupt = 0;
static void error(const char *str)
{
    if (noiselevel < ERRORS)
        return;
    if (!gc)
        x_error(TR("Error", "Error: %s"), str);
    uih_error(gc, str);
}

static void uiherror(struct uih_context *c)
{
    if (noiselevel < ERRORS)
        return;
    if (!gc) {
        uih_printmessages(c);
    } else
        uih_error(gc, uih->errstring);
}

static void printmsg(const char *text, ...)
{
    va_list ap;
    if (noiselevel < MESSAGES)
        return;
    va_start(ap, text);
    if (!gc) {
        vprintf(text, ap);
        printf("\n");
    } else {
        char s[256];
        vsnprintf(s, 256, text, ap);
        uih_message(gc, s);
        interrupt |= gc->interrupt |= gc->passfunc(gc, 1, s, 100);
        uih_clearwindows(gc);
    }
}

static int passfunc(struct uih_context */*c*/, int display, const char *text,
                    float percent)
{
    if (noiselevel < ALL)
        return 0;
    if (gc) {
        if (gc->passfunc != NULL)
            interrupt |= gc->interrupt |=
                gc->passfunc(gc, display, text, percent);
        uih_clearwindows(gc);
        return interrupt;
    } else if (display) {
        {
            if (newline)
                printf("\n"), newline = 0;
            printf("\r %s %3.2f%% ", text, (double)percent);
            fflush(stdout);
        }
    }
    return 0;
}

struct frame_info {
    vrect rect;
    number_t angle;
    char *name;
    int newimage;
};

int uih_renderanimation(struct uih_context *gc1, const char *basename,
                        xio_constpath animation, int width, int height,
                        float pixelwidth, float pixelheight, int frametime,
                        int type, int antialias, int slowmode,
                        int letterspersec, const char *catalog)
{
    struct palette *pal =
        createpalette(0, 0, TRUECOLOR, 0, 0, NULL, NULL, NULL, NULL, NULL);
    struct image *img;
    xio_file af;
    char s[200];
    int lastframenum = -1;
    int aliasnum = 0;
    static char *saveddata;
    int newimage;
    int y;

    struct frame_info curframe;
    int framenum = 0;

    noiselevel = ALL;
    gc = gc1;
    if (gc)
        gc->incalculation = 1;

    printmsg(TR("Message", "Initializing"));
    if (!(type & (TRUECOLOR24 | TRUECOLOR | TRUECOLOR16 | GRAYSCALE)))
        antialias = 0;

    while (uih_filters[aliasnum] != &antialias_filter)
        aliasnum++;

    if (!pal) {
        error(TR("Error", "Cannot create palette"));
        if (gc)
            gc->incalculation = 0;
        return 0;
    }
    if (!pixelwidth)
        pixelwidth = 0.025; // pixel pitch of modern non-retina monitors is
                            // roughly in this range
    if (!pixelheight)
        pixelheight = 0.025; // most importantly pixels should be square to
                             // avoid distorted image
    img = create_image_qt(width, height, pal, pixelwidth, pixelheight);

    if (!img) {
        error(TR("Error", "Cannot create image\n"));
        if (gc)
            gc->incalculation = 0;
        destroypalette(pal);
        return 0;
    }
    saveddata = (char *)malloc(img->width * img->height * img->bytesperpixel);
    if (saveddata == NULL) {
        error(TR("Error", "Cannot create checking buffer!"));
        if (gc)
            gc->incalculation = 0;
        destroy_image(img);
        destroypalette(pal);
        return 0;
    }
    uih = uih_mkcontext(0, img, passfunc, NULL, NULL);
    if (!uih) {
        error(TR("Error", "Cannot create context\n"));
        if (gc)
            gc->incalculation = 0;
        destroy_image(img);
        destroypalette(pal);
        free(saveddata);
        return 0;
    }
    if(gc1)
        uih->font = gc1->font;
    uih->fcontext->slowmode = 1;
    uih_constantframetime(uih, frametime);
    af = xio_ropen(animation);
    if (af == NULL) {
        error(TR("Error", "Cannot open animation file\n"));
        if (gc)
            gc->incalculation = 0;
        uih_freecontext(uih);
        destroy_image(img);
        destroypalette(pal);
        free(saveddata);
        return 0;
    }

    if (!gc) {
        printmsg(TR("Message", "Loading catalogs"));
        if (!gc) {
            uih_loadcatalog(uih, "english");
            if (uih->errstring) {
                uiherror(uih);
                if (gc)
                    gc->incalculation = 0;
                uih_freecontext(uih);
                destroy_image(img);
                destroypalette(pal);
                free(saveddata);
                xio_close(af);
                return 0;
            }
        }
        if (catalog != NULL)
            uih_loadcatalog(uih, catalog);
        if (uih->errstring) {
            uiherror(uih);
            if (gc)
                gc->incalculation = 0;
            uih_freecontext(uih);
            destroy_image(img);
            destroypalette(pal);
            free(saveddata);
            if (!gc)
                uih_freecatalog(uih);
            xio_close(af);
            return 0;
        }
        printmsg(TR("Message", "Processing command line options"));
        {
            const menuitem *item;
            dialogparam *d;
            while ((item = menu_delqueue(&d)) != NULL) {
                menu_activate(item, uih, d);
            }
        }
        if (uih->errstring) {
            uiherror(uih);
            if (gc)
                gc->incalculation = 0;
            uih_freecontext(uih);
            destroy_image(img);
            destroypalette(pal);
            free(saveddata);
            if (!gc)
                uih_freecatalog(uih);
            xio_close(af);
            return 0;
        }
    }

    printmsg(TR("Message", "Enabling animation replay\n"));

    uih_replayenable(uih, af, animation, 1);

    uih_letterspersec(uih, letterspersec);

    if (!gc)
        x_message(TR("Message", "Entering calculation loop!"));
    else
        printmsg(TR("Message", "Entering calculation loop!"));

    while ((uih->play || uih->display) && !interrupt) {
        if (uih->errstring) {
            uiherror(uih);
            if (gc)
                gc->incalculation = 0;
            uih_freecontext(uih);
            destroy_image(img);
            destroypalette(pal);
            free(saveddata);
            if (!gc)
                uih_freecatalog(uih);
            return 0;
        }
        fflush(stdout);
        tl_process_group(syncgroup, NULL);
        uih_update(uih, 0, 0, 0);

        if (uih->display) {
            printmsg(TR("Message", "Rendering frame %i..."), framenum);

            newline = 1;
            newimage = 0;
            if (uih->recalculatemode > 0) {
                if (slowmode)
                    uih_newimage(uih), uih->fcontext->version++;
            }
            if (antialias && !uih->filter[aliasnum]) {
                uih->aliasnum = aliasnum;
                uih_enablefilter(uih, aliasnum);
            }
            uih_prepare_image(uih);

            fflush(stdout);
            uih_drawwindows(uih);

            y = 0;
            if (lastframenum >= 0) {
                for (; y < img->height; y++)
                    if (memcmp(saveddata + img->width * img->bytesperpixel * y,
                               uih->image->currlines[y],
                               img->width * img->bytesperpixel))
                        break;
            }

            if (y != img->height) {
                for (; y < img->height; y++)
                    memcpy(saveddata + img->width * img->bytesperpixel * y,
                           uih->image->currlines[y],
                           img->width * img->bytesperpixel);
                fflush(stdout);
                sprintf(s, "%s%06i.png", basename, framenum);
                curframe.rect = uih->fcontext->rs;
                curframe.angle = uih->fcontext->angle;
                curframe.name = s;
                curframe.newimage = newimage;
                writepng(s, uih->image, NULL);
                uih_displayed(uih);
                lastframenum = framenum;
            } else {
                uih_displayed(uih);
            }
        }

        if (lastframenum < framenum) {
            // The image is a duplicate of the previous frame
            char t[256];
            sprintf(t, "%s%06i.png", basename, framenum);
#ifdef _WIN32
            // No symlinks on windows, so just save another copy
            writepng(t, uih->image, NULL);
#else
            // On Unix, save a symlink.
            printmsg(TR("Message", "Linking frame %i to %i..."), framenum,
                     lastframenum);
            if (symlink(s, t) != 0) {
                if (gc)
                    uih_error(gc, "Error creating symlink.");
                else
                    printf("Error creating symlink.");
            }
#endif
        }
        framenum++;
    }
    curframe.newimage = 1;
    if (uih->errstring) {
        uiherror(uih);
        if (gc)
            gc->incalculation = 0;
        uih_freecontext(uih);
        destroy_image(img);
        destroypalette(pal);
        free(saveddata);
        if (!gc)
            uih_freecatalog(uih);
        return 0;
    }
    free(saveddata);
    uih_freecontext(uih);
    destroy_image(img);
    destroypalette(pal);
    if (interrupt)
        error(TR("Error", "Calculation interrupted"));
    else {
        if (!gc)
            x_message(TR("Error", "Calculation finished"));
        else
            printmsg(TR("Error", "Calculation finished"));
    }
    if (gc)
        gc->incalculation = 0;
    if (!gc)
        uih_freecatalog(uih);
    return 1;
}

int uih_renderimage(struct uih_context *gc1, xio_file af, xio_constpath path,
                    struct image *img, int antialias, const char *catalog,
                    int noise)
{
    int aliasnum = 0;
    noiselevel = noise;
    gc = gc1;
    if (gc)
        gc->incalculation = 1;

    while (uih_filters[aliasnum] != &antialias_filter)
        aliasnum++;

    uih = uih_mkcontext(0, img, passfunc, NULL, NULL);
    if (!uih) {
        error(TR("Error", "Cannot create context\n"));
        if (gc)
            gc->incalculation = 0;
        return 0;
    }
    uih->fcontext->slowmode = 1;
    uih_constantframetime(uih, 1000000 / 10);

    if (!gc) {
        printmsg(TR("Message", "Loading catalogs"));
        uih_loadcatalog(uih, "english");
        if (uih->errstring) {
            fprintf(stderr, "%s", uih->errstring);
            uih_clearmessages(uih);
            uih->errstring = NULL;
        }
        if (catalog != NULL)
            uih_loadcatalog(uih, catalog);
        if (uih->errstring) {
            fprintf(stderr, "%s", uih->errstring);
            uih_clearmessages(uih);
            uih->errstring = NULL;
        }
        if (uih->errstring) {
            uih_freecatalog(uih);
            uih_freecontext(uih);
            uiherror(uih);
            if (gc)
                gc->incalculation = 0;
            return 0;
        }
    }

    uih_load(uih, af, path);
    if (uih->errstring) {
        uiherror(uih);
        uih_freecatalog(uih);
        uih_freecontext(uih);
        if (gc)
            gc->incalculation = 0;
        return 0;
    }
    printmsg(TR("Message", "Entering calculation loop!"));

    tl_process_group(syncgroup, NULL);
    uih_update(uih, 0, 0, 0);

    uih_newimage(uih), uih->fcontext->version++;
    if (antialias && !uih->filter[aliasnum]) {
        uih->aliasnum = aliasnum;
        uih_enablefilter(uih, aliasnum);
    }
    uih_prepare_image(uih);
    uih_drawwindows(uih);
    uih_freecontext(uih);
    uih_freecatalog(uih);
    if (interrupt)
        error(TR("Error", "Calculation interrupted"));
    else {
        printmsg(TR("Message", "Calculation finished"));
    }
    if (gc)
        gc->incalculation = 0;
    return 1;
}
