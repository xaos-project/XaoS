
#ifdef _plan9_
#include <u.h>
#include <libc.h>
#ifdef _plan9v2_
#include <stdarg.h>		/* not needed in plan9v3 */
#endif
#else
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#endif

#include <ui.h>
#include <fconfig.h>
#include <filter.h>
#include <fractal.h>
#include <ui_helper.h>
#include <misc-f.h>
#include <xmenu.h>
#include <xerror.h>

#ifdef HAVE_GETTEXT
#include <libintl.h>
#else
#define gettext(STRING) STRING
#endif

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
	x_error(gettext("Error: %s"), str);
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
	vsprintf(s, text, ap);
	uih_message(gc, s);
	interrupt |= gc->interrupt |= gc->passfunc(gc, 1, s, 100);
	uih_clearwindows(gc);
    }
}

static int
passfunc(struct uih_context *c, int display, const char *text,
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
	    printf("\r %s %3.2f%% ", text, (double) percent);
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
static void
save_frame_dist(uih_context * c, int backward, struct frame_info *f1,
		struct frame_info *f2)
{
    xio_file f;
    int x1, y1;
    char str[256];
    if (!(f2->rect.mc - f2->rect.nc) || !(f2->rect.mi - f2->rect.ni))
	return;
    /*printf ("Frame\n"); */
    sprintf(str, "%s.%c", f1->name, backward ? 'b' : 'p');
    f = xio_wopen(str);
    if (f == NULL) {
	x_error(gettext("Cannot open motion vector file!"));
	return;
    }
    for (y1 = 0; y1 < (c->image->height + 7) / 8; y1++) {
	for (x1 = 0; x1 < (c->image->width + 7) / 8; x1++) {
	    number_t x, y;
	    number_t x2, y2;
	    number_t tmp;
	    x = f1->rect.nc + (x1 * 8 + 4) * (f1->rect.mc -
					      f1->rect.nc) /
		c->image->width;
	    y = f1->rect.ni + (y1 * 8 + 4) * (f1->rect.mi -
					      f1->rect.ni) /
		c->image->height;

	    if (f2->angle != f1->angle) {
		tmp =
		    x * cos(f2->angle - f1->angle) - y * sin(f2->angle -
							     f1->angle);
		y = x * sin(f2->angle - f1->angle) + y * cos(f2->angle -
							     f1->angle);
		x = tmp;
	    }
	    x2 = (x - f2->rect.nc) * c->image->width / (f2->rect.mc -
							f2->rect.nc);
	    y2 = (y - f2->rect.ni) * c->image->height / (f2->rect.mi -
							 f2->rect.ni);
	    sprintf(str, "%3.2g %3.2g  ",
		    ((int) ((x2 - (x1 * 8 + 4)) * 10)) / 10.0,
		    ((int) ((y2 - (y1 * 8 + 4)) * 10)) / 10.0);
	    xio_puts(str, f);
	}
	xio_putc('\n', f);
    }
    xio_close(f);
    /*printf ("Frameend\n"); */
}

#define MAXBFRAMES 5
#define IFRAMEDIST (27)
static int iframedist;
static int mvectors;
static xio_file patf;
static void
uih_encodeframe(int startpos, int endpos, struct frame_info *curframe)
{
    static int lastiframe = -200;
    static struct frame_info lastp;
    static struct frame_info bframes[MAXBFRAMES];
    static int nbframes = 0;
    int i;
    char type;
    if (!gc)
	printf(" motion");
    fflush(stdout);
    if (endpos > startpos + 4) {
	if (endpos - lastiframe > iframedist)
	    type = 'I';
	else
	    type = 'P';
    } else {
	if (endpos - lastiframe > iframedist)
	    type = 'I';
	else
	    type = (startpos - lastiframe) % 3 ? 'B' : 'P';
	if (startpos != endpos)
	    type = 'P';
    }
    if (curframe->newimage)
	type = 'I';
    if (mvectors) {
	switch (type) {
	case 'I':
	    if (startpos)
		save_frame_dist(uih, 0, curframe, &lastp);
	    lastp = *curframe;
	    break;
	case 'P':
	    save_frame_dist(uih, 0, curframe, &lastp);
	    lastp = *curframe;
	    break;
	case 'B':
	    save_frame_dist(uih, 0, curframe, &lastp);
	    if (nbframes < MAXBFRAMES) {
		bframes[nbframes] = *curframe;
		bframes[nbframes].name = mystrdup(curframe->name);
		nbframes++;
	    }
	    break;
	}
    }
    xio_putc(type, patf);
    if (type == 'I')
	lastiframe = startpos;
    if (startpos != endpos) {
	while (startpos != endpos)
	    xio_putc('p', patf), startpos++;
    }
    if (!gc)
	printf(" %c", type);
    fflush(stdout);
    if (mvectors) {
	if (type != 'B' && nbframes) {
	    if (!gc)
		printf(" backframes");
	    fflush(stdout);
	    for (i = 0; i < nbframes; i++) {
		save_frame_dist(uih, 1, bframes + i, curframe);
		fflush(stdout);
		free(bframes[i].name);
	    }
	    nbframes = 0;
	}
    }
    xio_flush(patf);
}

extern struct filteraction antialias_filter;
int
uih_renderanimation(struct uih_context *gc1, const char *basename,
		    const xio_constpath animation, int width, int height,
		    float pixelwidth, float pixelheight, int frametime,
		    int type, int antialias, int slowmode,
		    int letterspersec, const char *catalog,
		    int motionvectors, int iframedist2)
{
    struct palette *pal =
	createpalette(0, 0, type, 0, 0, NULL, NULL, NULL, NULL, NULL);
    struct image *img;
    xio_file of;
    /*FILE *f;*/
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

    mvectors = motionvectors;
    printmsg(gettext("Vectors: %i"), motionvectors);
    if (iframedist2)
	iframedist = iframedist2;
    else
	iframedist = 27;

    printmsg(gettext("Initializing"));
    if (!(type & (TRUECOLOR24 | TRUECOLOR | TRUECOLOR16 | GRAYSCALE)))
	antialias = 0;

    while (uih_filters[aliasnum] != &antialias_filter)
	aliasnum++;

    if (!pal) {
	error(gettext("Cannot create palette"));
	if (gc)
	    gc->incalculation = 0;
	return 0;
    }
    if (!pixelwidth)
	pixelwidth = 29.0 / width;
    if (!pixelheight)
	pixelheight = 21.5 / height;
    img = create_image_mem(width, height, 2, pal, pixelwidth, pixelheight);
    if (!img) {
	error(gettext("Cannot create image\n"));
	if (gc)
	    gc->incalculation = 0;
	destroypalette(pal);
	return 0;
    }
    saveddata =
	(char *) malloc(img->width * img->height * img->bytesperpixel);
    if (saveddata == NULL) {
	error(gettext("Cannot create checking buffer!"));
	if (gc)
	    gc->incalculation = 0;
	destroy_image(img);
	destroypalette(pal);
	return 0;
    }
    uih = uih_mkcontext(0, img, passfunc, NULL, NULL);
    if (!uih) {
	error(gettext("Cannot create context\n"));
	if (gc)
	    gc->incalculation = 0;
	destroy_image(img);
	destroypalette(pal);
	free(saveddata);
	return 0;
    }
    uih->fcontext->slowmode = 1;
    uih_constantframetime(uih, frametime);
    af = xio_ropen(animation);
    if (af == NULL) {
	error(gettext("Cannot open animation file\n"));
	if (gc)
	    gc->incalculation = 0;
	uih_freecontext(uih);
	destroy_image(img);
	destroypalette(pal);
	free(saveddata);
	return 0;
    }

    if (!gc) {
	printmsg(gettext("Loading catalogs"));
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
	printmsg(gettext("Processing command line options"));
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

    printmsg(gettext("Enabling animation replay\n"));

    uih_replayenable(uih, af, animation, 1);

    sprintf(s, "%s.par", basename);
    of = xio_wopen(s);
    if (of == NULL) {
	error(gettext("Cannot open image file"));
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
    sprintf(s, "%s.pat", basename);
    patf = xio_wopen(s);
    if (patf == NULL) {
	error(gettext("Cannot open pattern file"));
	if (gc)
	    gc->incalculation = 0;
	uih_freecontext(uih);
	destroy_image(img);
	destroypalette(pal);
	free(saveddata);
	if (!gc)
	    uih_freecatalog(uih);
	xio_close(of);
	return 0;
    }
    uih_letterspersec(uih, letterspersec);



    if (!gc)
	x_message(gettext("Entering calculation loop!"));
    else
	printmsg(gettext("Entering calculation loop!"));

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
	    xio_close(of);
	    xio_close(patf);
	    return 0;
	}
	fflush(stdout);
	tl_process_group(syncgroup, NULL);
	uih_update(uih, 0, 0, 0);


	if (uih->display) {

	    if (lastframenum < framenum - 1) {
		if (lastframenum == framenum - 1)
		    printmsg(gettext("Frame %i skipped."), framenum - 1);
		else
		    printmsg(gettext("Frames %i - %i skipped."),
			     lastframenum, framenum - 1);
	    }

	    printmsg(gettext("Frame %4i: "), framenum);

	    newline = 1;
	    newimage = 0;
	    if (uih->recalculatemode > 0) {
		if (!gc)
		    printf("calculating"), fflush(stdout);
		if (slowmode)
		    uih_newimage(uih), uih->fcontext->version++;
	    }
	    if (antialias && !uih->filter[aliasnum]) {
		if (!gc)
		    printf("antialias ");
		uih->aliasnum = aliasnum;
		uih_enablefilter(uih, aliasnum);
	    }
	    uih_prepare_image(uih);

	    if (!gc)
		printf(" rendering");
	    fflush(stdout);
	    uih_drawwindows(uih);

	    y = 0;
	    if (lastframenum >= 0) {
		for (; y < img->height; y++)
		    if (memcmp
			(saveddata + img->width * img->bytesperpixel * y,
			 uih->image->currlines[y],
			 img->width * img->bytesperpixel))
			break;
	    }


	    if (y != img->height) {
		for (; y < img->height; y++)
		    memcpy(saveddata + img->width * img->bytesperpixel * y,
			   uih->image->currlines[y],
			   img->width * img->bytesperpixel);
		if (framenum)
		    uih_encodeframe(lastframenum, framenum - 1, &curframe);
		if (!gc)
		    printf(" saving");
		fflush(stdout);
		sprintf(s, "%s%04i.png", basename, framenum);
		curframe.rect = uih->fcontext->rs;
		curframe.angle = uih->fcontext->angle;
		curframe.name = s;
		curframe.newimage = newimage;

		/*
		f = fopen(s, "wb");
		if (f == NULL) {
		    error(gettext("Cannot open image file"));
		    if (gc)
			gc->incalculation = 0;
		    uih_freecontext(uih);
		    destroy_image(img);
		    destroypalette(pal);
		    free(saveddata);
		    if (!gc)
			uih_freecatalog(uih);
		    xio_close(of);
		    xio_close(patf);
		    return 0;
		}
		*/
		writepng(s, uih->image);
		printmsg(gettext(" done."));
		uih_displayed(uih);
		lastframenum = framenum;
	    } else {
		printmsg(gettext(" skipping..."));
		uih_displayed(uih);
	    }
	}
	xio_puts(s, of);
	xio_puts("\n", of);
	xio_flush(of);
	framenum++;
    }
    curframe.newimage = 1;
    if (framenum)
	uih_encodeframe(lastframenum, framenum - 1, &curframe);
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
	xio_close(of);
	xio_close(patf);
	return 0;
    }
    xio_close(of);
    free(saveddata);
    xio_close(patf);
    uih_freecontext(uih);
    destroy_image(img);
    destroypalette(pal);
    if (interrupt)
	error(gettext("Calculation interrupted"));
    else {
	if (!gc)
	    x_message(gettext("Calculation finished"));
	else
	    printmsg(gettext("Calculation finished"));
    }
    if (gc)
	gc->incalculation = 0;
    if (!gc)
	uih_freecatalog(uih);
    return 1;
}

int
uih_renderimage(struct uih_context *gc1, xio_file af,
		const xio_constpath path, struct image *img, int antialias,
		const char *catalog, int noise)
{
    int aliasnum = 0;
    int ok = 1;
    noiselevel = noise;
    gc = gc1;
    if (gc)
	gc->incalculation = 1;

    while (uih_filters[aliasnum] != &antialias_filter)
	aliasnum++;

    uih = uih_mkcontext(0, img, passfunc, NULL, NULL);
    if (!uih) {
	error(gettext("Cannot create context\n"));
	if (gc)
	    gc->incalculation = 0;
	return 0;
    }
    uih->fcontext->slowmode = 1;
    uih_constantframetime(uih, 1000000 / 10);

    if (!gc) {
	printmsg(gettext("Loading catalogs"));
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
    printmsg(gettext("Entering calculation loop!"));

    tl_process_group(syncgroup, NULL);
    uih_update(uih, 0, 0, 0);

    uih_newimage(uih), uih->fcontext->version++;
    if (antialias && !uih->filter[aliasnum]) {
	uih->aliasnum = aliasnum;
	uih_enablefilter(uih, aliasnum);
    }
    uih_prepare_image(uih);
    if (uih->errstring)
	ok = 0;
    uih_drawwindows(uih);
    if (uih->errstring)
	ok = 0;
    uih_freecontext(uih);
    uih_freecatalog(uih);
    if (interrupt)
	error(gettext("Calculation interrupted"));
    else {
	printmsg(gettext("Calculation finished"));
    }
    if (gc)
	gc->incalculation = 0;
    return 1;
}
