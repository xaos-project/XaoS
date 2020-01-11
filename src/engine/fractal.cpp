/*
 *     XaoS, a fast portable realtime fractal zoomer
 *                  Copyright (C) 1996,1997 by
 *
 *      Jan Hubicka          (hubicka@paru.cas.cz)
 *      Thomas Marsh         (tmarsh@austin.ibm.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*#define STATISTICS */
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <climits>
#include <cassert>
#include <cmath>
#include "config.h"
#include "filter.h"
#include "cmplx.h"
#include "plane.h"
#include "timers.h"
#ifndef M_PI
#define M_PI 3.1415
#endif
#include "xerror.h"

struct symmetryinfo2 cursymmetry;
struct palette cpalette;
struct image cimage;
struct fractal_context cfractalc;
struct formula cformula;

static symmetry2 sym_lines[100];

static void precalculate_rotation(fractal_context *c)
{
    c->sin = sin((c->angle) * M_PI / 180);
    c->cos = cos((c->angle) * M_PI / 180);
}

static void recalc_view(fractal_context *c)
{
    number_t xs = c->s.rr, ys = c->s.ri * c->windowwidth / c->windowheight,
             xc = c->s.cr, yc = c->s.ci, size;
    precalculate_rotation(c);
    rotate(*c, xc, yc);
    /*assert(c->s.rr >= 0);
       assert(c->s.ri >= 0); */

    xs = myabs(xs); /*do not crash in owerflowing cases */
    ys = myabs(ys);
    if (xs > ys)
        size = xs;
    else
        size = ys;
    c->rs.nc = xc - size / 2;
    c->rs.mc = xc + size / 2;
    c->rs.ni = yc - size * c->windowheight / c->windowwidth / 2;
    c->rs.mi = yc + size * c->windowheight / c->windowwidth / 2;
    if (c->rs.nc > c->rs.mc)
        xc = c->rs.nc, c->rs.nc = c->rs.mc, c->rs.mc = xc;
    if (c->rs.ni > c->rs.mi)
        xc = c->rs.ni, c->rs.ni = c->rs.mi, c->rs.mi = xc;
}

static void set_view(fractal_context *c, const vinfo *s)
{
    c->s = *s;
    recalc_view(c);
}

/*FIXME most of this code is obsolette */
static void /*inline */
combine_methods(void)
{
    int angle = (int)cfractalc.angle;
    const struct symmetryinfo *s1 = cfractalc.currentformula->out +
                                    cfractalc.coloringmode,
                              *s2 = cfractalc.currentformula->in +
                                    cfractalc.incoloringmode;
    if (angle < 0) {
        angle = 360 - ((-angle) % 360);
    } else
        angle %= 360;
    if (cfractalc.mandelbrot != cfractalc.currentformula->mandelbrot ||
        cfractalc.bre || cfractalc.bim) {
        cursymmetry.xsym = (number_t)INT_MAX;
        cursymmetry.ysym = (number_t)INT_MAX;
        cursymmetry.nsymmetries = 0;
        return;
    }
    if (s1->xsym == s2->xsym)
        cursymmetry.xsym = s1->xsym;
    else
        cursymmetry.xsym = (number_t)INT_MAX;
    if (s1->ysym == s2->ysym)
        cursymmetry.ysym = s1->ysym;
    else
        cursymmetry.ysym = (number_t)INT_MAX;
    switch (cfractalc.plane) {
        case P_PARABOL:
            cursymmetry.xsym = (number_t)INT_MAX;
            break;
        case P_LAMBDA:
            if (cursymmetry.xsym == 0 && cursymmetry.ysym == 0)
                cursymmetry.xsym = (number_t)1;
            else
                cursymmetry.xsym = (number_t)INT_MAX;
            break;
        case P_INVLAMBDA:
            cursymmetry.xsym = (number_t)INT_MAX;
            break;
        case P_TRANLAMBDA:
            if (cursymmetry.xsym != 0 || cursymmetry.ysym != 0)
                cursymmetry.xsym = (number_t)INT_MAX;
            break;
        case P_MEREBERG:
            cursymmetry.xsym = (number_t)INT_MAX;
            break;
    }
    cursymmetry.symmetry = sym_lines;
    cursymmetry.nsymmetries = 0;
    if ((number_t)angle == cfractalc.angle) {
        switch (angle) {
            case 0:
                break;
            case 180:
                cursymmetry.xsym = -cursymmetry.xsym;
                cursymmetry.ysym = -cursymmetry.ysym;
                break;
            case 90: {
                number_t tmp = cursymmetry.xsym;
                cursymmetry.xsym = -cursymmetry.ysym;
                cursymmetry.ysym = tmp;
            } break;
            case 210: {
                number_t tmp = cursymmetry.xsym;
                cursymmetry.xsym = cursymmetry.ysym;
                cursymmetry.ysym = -tmp;
            } break;
            default:
                cursymmetry.xsym = (number_t)INT_MAX;
                cursymmetry.ysym = (number_t)INT_MAX;
        }
    } else {
        cursymmetry.xsym = (number_t)INT_MAX;
        cursymmetry.ysym = (number_t)INT_MAX;
    }
    if (cursymmetry.xsym == -(number_t)INT_MAX)
        cursymmetry.xsym = (number_t)INT_MAX;
    if (cursymmetry.ysym == -(number_t)INT_MAX)
        cursymmetry.ysym = (number_t)INT_MAX;
}

void update_view(fractal_context *context) { set_view(context, &context->s); }

void set_fractalc(fractal_context *context, struct image *img)
{
    update_view(context);
    precalculate_rotation(context);
    cfractalc = *context; /*its better to copy often accesed data into fixed
                             memory locations */
    cpalette = *img->palette;
    cimage = *img;
    cformula = *context->currentformula;

    if (cfractalc.maxiter < 1)
        cfractalc.maxiter = 1;

    if (cfractalc.bailout < 0)
        cfractalc.bailout = 0;

    if (cfractalc.periodicity) {
        if (!cformula.hasperiodicity || cfractalc.incoloringmode ||
            !cfractalc.mandelbrot)
            cfractalc.periodicity = 0;
        else if (!cfractalc.plane)
            cfractalc.periodicity_limit =
                (context->rs.mc - context->rs.nc) / (double)img->width;
        else {
            int x, y;
            number_t xstep =
                ((context->rs.mc - context->rs.nc) / (double)img->width);
            number_t ystep =
                ((context->rs.mc - context->rs.nc) / (double)img->height);
            number_t xstep2 = ((context->rs.mc - context->rs.nc) / 5);
            number_t ystep2 = ((context->rs.mc - context->rs.nc) / 5);

            for (x = 0; x < 5; x++)
                for (y = 0; y < 5; y++) {
                    number_t x1 = context->rs.mc + xstep2 * x;
                    number_t y1 = context->rs.mi + ystep2 * y;
                    number_t x2 = context->rs.mc + xstep2 * x + xstep;
                    number_t y2 = context->rs.mi + ystep2 * y + ystep;

                    recalculate(cfractalc.plane, &x1, &y1);
                    recalculate(cfractalc.plane, &x2, &y2);

                    x1 = myabs(x2 - x1);
                    y1 = myabs(y2 - y1);

                    if (x == y && x == 0)
                        cfractalc.periodicity_limit = x1;
                    if (cfractalc.periodicity > x1)
                        cfractalc.periodicity_limit = x1;
                    if (cfractalc.periodicity > y1)
                        cfractalc.periodicity_limit = y1;
                }
        }
    }

    combine_methods();

    if (cursymmetry.xsym == (number_t)INT_MAX)
        cursymmetry.xsym = cfractalc.rs.mc + INT_MAX;

    if (cursymmetry.ysym == (number_t)INT_MAX)
        cursymmetry.ysym = cfractalc.rs.mi + INT_MAX;

    if (cfractalc.coloringmode == 9 && cformula.smooth_calculate != NULL &&
        (cpalette.type &
         (TRUECOLOR | TRUECOLOR16 | TRUECOLOR24 | GRAYSCALE | LARGEITER))) {
        cfractalc.calculate[0] = cformula.smooth_calculate;
        if (cformula.smooth_calculate_periodicity && cfractalc.periodicity)
            cfractalc.calculate[1] = cformula.smooth_calculate_periodicity;
        else
            cfractalc.calculate[1] = cformula.smooth_calculate;
    } else {
        cfractalc.calculate[0] = cformula.calculate;
        if (cformula.calculate_periodicity && cfractalc.periodicity)
            cfractalc.calculate[1] = cformula.calculate_periodicity;
        else
            cfractalc.calculate[1] = cformula.calculate;
    }
}

void set_formula(fractal_context *c, int num)
{
    assert(num < nformulas);
    assert(num >= 0);
    if (num >= nformulas)
        num = 0;
    if (c->currentformula != formulas + num) {
        c->currentformula = formulas + num;
        c->version++;
    }
    if (c->mandelbrot != c->currentformula->mandelbrot) {
        c->mandelbrot = c->currentformula->mandelbrot;
        c->version++;
    }
    if (c->currentformula->pre != c->pre) {
        c->pre = c->currentformula->pre;
        if (!c->mandelbrot)
            c->version++;
    }
    if (c->currentformula->pim != c->pim) {
        c->pim = c->currentformula->pim;
        if (!c->mandelbrot)
            c->version++;
    }
    if (c->angle) {
        c->angle = 0;
        c->version++;
    }
    if (c->s.cr != c->currentformula->v.cr ||
        c->s.ci != c->currentformula->v.ci ||
        c->s.rr != c->currentformula->v.rr ||
        c->s.ri != c->currentformula->v.ri) {
        c->s = c->currentformula->v;
        c->version++;
    }
    if (c->bre && c->bim) {
        c->bre = c->bim = 0;
        if (c->mandelbrot)
            c->version++;
    }
}

void fractalc_resize_to(fractal_context *c, float wi, float he)
{
    c->windowwidth = wi;
    c->windowheight = he;
    recalc_view(c);
    return;
}

fractal_context *make_fractalc(const int formula, float wi, float he)
{
    fractal_context *new_ctxt;
    new_ctxt = (fractal_context *)calloc(1, sizeof(fractal_context));
    if (new_ctxt == NULL)
        return 0;
    new_ctxt->windowwidth = wi;
    new_ctxt->periodicity = 1;
    new_ctxt->windowheight = he;
    new_ctxt->maxiter = DEFAULT_MAX_ITER;
    new_ctxt->bailout = DEFAULT_BAILOUT;
    new_ctxt->coloringmode = 0;
    new_ctxt->intcolor = 0;
    new_ctxt->outtcolor = 0;
    new_ctxt->slowmode = 0;
    new_ctxt->range = 3;
    new_ctxt->angle = 0;
#ifdef USE_SFFE
    // These parsers don't actually calculate anything; they're just
    // here to validate the formula before it's sent to the thread
    // local parsers, so the variables are just set to a dummy location
    // to make them legal for the parser
    static cmplx sffe_dummy;
    new_ctxt->userformula = sffe_alloc();
    sffe_regvar(&new_ctxt->userformula, &sffe_dummy, "p");
    sffe_regvar(&new_ctxt->userformula, &sffe_dummy, "z");
    sffe_regvar(&new_ctxt->userformula, &sffe_dummy, "c");
    sffe_regvar(&new_ctxt->userformula, &sffe_dummy, "n");

    new_ctxt->userinitial = sffe_alloc();
    sffe_regvar(&new_ctxt->userinitial, &sffe_dummy, "p");
    sffe_regvar(&new_ctxt->userinitial, &sffe_dummy, "z");
    sffe_regvar(&new_ctxt->userinitial, &sffe_dummy, "c");
    sffe_regvar(&new_ctxt->userinitial, &sffe_dummy, "n");
#endif
    set_formula(new_ctxt, formula);
    return (new_ctxt);
}

void free_fractalc(fractal_context *c)
{
#ifdef USE_SFFE
    sffe_free(&c->userformula);
    sffe_free(&c->userinitial);
#endif
    free(c);
}

void speed_test(fractal_context *c, struct image *img)
{
    // unsigned int sum;
    tl_timer *t;
    int time;
    unsigned int i;
    set_fractalc(c, img);
    t = tl_create_timer();
    cfractalc.maxiter = 100;
    (void)cfractalc.currentformula->calculate(0.0, 0.0, 0.0, 0.0);
    if (cfractalc.currentformula->calculate_periodicity != NULL)
        (void)cfractalc.currentformula->calculate_periodicity(0.0, 0.0, 0.0,
                                                              0.0);
    if (cfractalc.currentformula->smooth_calculate != NULL)
        (void)cfractalc.currentformula->smooth_calculate(0.0, 0.0, 0.0, 0.0);
    if (cfractalc.currentformula->smooth_calculate_periodicity != NULL)
        (void)cfractalc.currentformula->smooth_calculate_periodicity(0.0, 0.0,
                                                                     0.0, 0.0);
    cfractalc.maxiter = 20000000;

    tl_update_time();
    tl_reset_timer(t);
    /*sum = rdtsc (); */
    i = cfractalc.currentformula->calculate(0.0, 0.0, 0.0, 0.0);
    /*sum -= rdtsc ();
       printf ("%f\n", (double) (-sum) / cfractalc.maxiter); */
    tl_update_time();
    time = tl_lookup_timer(t);
    x_message("Result:%i Formulaname:%s Time:%i Mloops per sec:%.2f", (int)i,
              cfractalc.currentformula->name[0], time,
              cfractalc.maxiter / (double)time);

    if (cfractalc.currentformula->smooth_calculate != NULL) {
        tl_update_time();
        tl_reset_timer(t);
        i = cfractalc.currentformula->smooth_calculate(0.0, 0.0, 0.0, 0.0);
        tl_update_time();
        time = tl_lookup_timer(t);
        x_message("Result:%i Formulaname:%s Time:%i Mloops per sec:%.2f",
                  (int)i, cfractalc.currentformula->name[0], time,
                  cfractalc.maxiter / (double)time);
    }
    tl_free_timer(t);
}
