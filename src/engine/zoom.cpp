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
#include <cstdlib>
#include <cstdio>
#include <climits>
#include <cmath>
#include <cstring>
#include <cassert>

#include "config.h"
#define SLARGEITER
#include "filter.h"
#include "zoom.h"
#include "cmplx.h" /*for myabs */
#include "plane.h"
#include "btrace.h"
#include "xthread.h"
#include "xerror.h"
#include "calculate.h" /*An inlined calculate function */
#include "i18n.h"

#define ASIZE 16
#define ALIGN(x) (((x) + ASIZE - 1) & (~(ASIZE - 1)))
static int nsymmetrized;
unsigned char *tmpdata, *tmpdata1;
struct realloc_s {
    number_t position;
    number_t price;
    unsigned int plus;
    int recalculate;
    int symto;
    int symref;
    int dirty;
}
#ifdef __GNUC__
__attribute__((aligned(32)))
#endif
;
typedef struct realloc_s realloc_t;

typedef struct zoom_context {
    number_t *xpos, *ypos;
    int newcalc;
    int forversion;
    int forpversion;
    realloc_t *reallocx, *reallocy;
    int incomplete;
    int changed;
} zoom_context;

struct filltable {
    int from;
    int to;
    int length;
    int end;
};
#define getzcontext(f) ((zoom_context *)((f)->data))
#define getfcontext(f) ((f)->fractalc)

#define callwait()                                                             \
    if (cfilter.wait_function != NULL)                                         \
        cfilter.wait_function(&cfilter);
#define tcallwait()                                                            \
    if (!xth_nthread(task) && cfilter.wait_function != NULL)                   \
        cfilter.wait_function(&cfilter);
#define setincomplete(i) (getzcontext(&cfilter)->incomplete = i)
#define incincomplete() (getzcontext(&cfilter)->incomplete++)
#define setchanged(i) (getzcontext(&cfilter)->changed = i)

zoom_context czoomc;
struct filter cfilter;
#ifdef STATISTICS
static int tocalculate = 0, avoided = 0;
static int nadded = 0, nsymmetry = 0, nskipped = 0;
int nperi = 0;
#endif

static void moveoldpoints(void *data1, struct taskinfo *task, int r1, int r2);

/* Repeated inclusions of zoomd.h replaced with C++ templates */
#include "zoomd.h"

#define calcline(a)                                                            \
    drivercall(cimage, tpl::calcline<Pixel8Traits>(a),                         \
                       tpl::calcline<Pixel16Traits>(a),                        \
                       tpl::calcline<Pixel24Traits>(a),                        \
                       tpl::calcline<Pixel32Traits>(a));
#define calccolumn(a)                                                          \
    drivercall(cimage, tpl::calccolumn<Pixel8Traits>(a),                       \
                       tpl::calccolumn<Pixel16Traits>(a),                      \
                       tpl::calccolumn<Pixel24Traits>(a),                      \
                       tpl::calccolumn<Pixel32Traits>(a));

struct dyn_data {
    long price;
    struct dyn_data *previous;
};

#define FPMUL 64 /*Let multiplication table fit into Pentium cache */
#define RANGES 2 /*Bit shift count for x*RANGE */
#define RANGE 4

#define DSIZEHMASK (0x7) /*Bit mask for x % DSIZE */
#define DSIZE (2 * RANGE)
#define DSIZES (RANGES + 1) /*Bit shift count for x*DSIZE */

#define adddata(n, i) (dyndata + (((n) << DSIZES) + (((i) & (DSIZEHMASK)))))
#define getbest(i) (dyndata + ((size) << DSIZES) + (i))
#define nosetadd ((size * 2) << DSIZES)
#ifndef DEBUG
#define CHECKPOS(pos)
#else
#define CHECKPOS(pos)                                                          \
    (assert((pos) >= dyndata),                                                 \
     assert((pos) < dyndata + (size) + ((size) << DSIZES)))
#endif

#ifdef __POWERPC__
#undef USE_MULTABLE
#else
#define USE_MULTABLE 1
#endif

#ifdef USE_MULTABLE
#define PRICE(i, i1) mulmid[(i) - (i1)]
#else
#define PRICE(i, i1) (((i) - (i1)) * ((i) - (i1)))
#endif
#define NEWPRICE (FPMUL * FPMUL * (RANGE) * (RANGE))

#define NOSETMASK ((unsigned int)0x80000000)
#define END NULL
#define MAXPRICE LONG_MAX
/*static int dynsize = (int)sizeof (struct dyn_data);*/
#ifndef INT_MIN
#define INT_MIN (-INT_MAX - 1)
#endif
#define IRANGE FPMUL *RANGE

#ifdef USE_MULTABLE
static int multable[RANGE * FPMUL * 2];
static int *mulmid;
#endif

/*Functions looks through rows/columns marked for calculation and tries to use
 *some symmetrical one instead
 */

/*FIXME should be threaded...but thread overhead would take more work than
 *doing it in one thread, since this is quite simple and only executes when
 *the fractal on the screen is symmetrical, which is a rare case...who knows
 */

static void preparesymmetries(realloc_t *realloc, const int size, int symi,
                              number_t sym, number_t step)
{
    int i;
    int istart = 0;
    number_t fy, ftmp;
    realloc_t *r = realloc, *reallocs;

    sym *= 2;
    i = 2 * symi - size;
    if (i < 0)
        i = 0;
    realloc += i;

    for (; i <= symi; i++, realloc++) { /*makes symmetries */
        int j, min = 0;
        number_t dist = NUMBER_BIG, tmp1;

        if (realloc->symto != -1)
            continue;

        fy = realloc->position;
        realloc->symto = 2 * symi - i;

        if (realloc->symto >= size - RANGE)
            realloc->symto = size - RANGE - 1;

        dist = RANGE * step;
        min = RANGE;
#ifdef DEBUG
        if (realloc->symto < 0 || realloc->symto >= size) {
            x_fatalerror("Internal error #22-1 %i", realloc->symto);
            assert(0);
        }
#endif
        reallocs = &r[realloc->symto];
        j = (realloc->symto - istart > RANGE) ? -RANGE
                                              : (-realloc->symto + istart);

        if (realloc->recalculate) {
            for (; j < RANGE && realloc->symto + j < size - 1; j++) {
                ftmp = sym - (reallocs + j)->position;
                if ((tmp1 = myabs(ftmp - fy)) < dist) {
                    if ((realloc == r || ftmp > (realloc - 1)->position) &&
                        (ftmp < (realloc + 1)->position)) {
                        dist = tmp1;
                        min = j;
                    }
                } else if (ftmp < fy)
                    break;
            }

        } else {
            for (; j < RANGE && realloc->symto + j < size - 1; j++) {
                if (!realloc->recalculate)
                    continue;
                ftmp = sym - (reallocs + j)->position;
                if ((tmp1 = myabs(ftmp - fy)) < dist) {
                    if ((realloc == r || ftmp > (realloc - 1)->position) &&
                        (ftmp < (realloc + 1)->position)) {
                        dist = tmp1;
                        min = j;
                    }
                } else if (ftmp < fy)
                    break;
            }
        }
        realloc->symto += min;

        if (min == RANGE || realloc->symto <= symi ||
            (reallocs = reallocs + min)->symto != -1 ||
            reallocs->symref != -1) {
            realloc->symto = -1;
            continue;
        }

        if (!realloc->recalculate) {
            realloc->symto = -1;
            if (reallocs->symto != -1 || !reallocs->recalculate)
                continue;
            reallocs->plus = realloc->plus;
            reallocs->symto = i;
            nsymmetrized++;
            istart = realloc->symto - 1;
            reallocs->dirty = 1;
            realloc->symref = (int)(reallocs - r);
            STAT(nadded -= reallocs->recalculate);
            reallocs->recalculate = 0;
            reallocs->position = sym - realloc->position;
        } else {
            if (reallocs->symto != -1) {
                realloc->symto = -1;
                continue;
            }
            istart = realloc->symto - 1;
            STAT(nadded -= realloc->recalculate);
            nsymmetrized++;
            realloc->dirty = 1;
            realloc->plus = reallocs->plus;
            realloc->recalculate = 0;
            reallocs->symref = i;
            realloc->position = sym - reallocs->position;
        }
        STAT(nsymmetry++);

#ifdef DEBUG
        if (realloc->symto < -1 || realloc->symto >= size) {
            x_fatalerror("Internal error #22 %i", realloc->symto);
            assert(0);
        }
        if (reallocs->symto < -1 || reallocs->symto >= size) {
            x_fatalerror("Internal error #22-2 %i", reallocs->symto);
            assert(0);
        }
#endif
    }
}

static void newpositions(realloc_t *realloc, unsigned int size, number_t begin1,
                         number_t end1, const number_t *fpos, int yend)
{
    realloc_t *rs, *re, *rend;
    number_t step = size / (end1 - begin1);
    number_t start;
    number_t end;
    rend = realloc + size;
    rs = realloc - 1;
    re = realloc;
    while (rs < rend - 1) {
        re = rs + 1;
        if (re->recalculate) {
            while (re < rend && re->recalculate)
                re++;

            if (re == rend)
                end = end1;
            else
                end = re->position;

            if (rs == realloc - 1) {
                start = begin1;
                if (start > end)
                    start = end;
            } else
                start = rs->position;

            if (re == rend && start > end)
                end = start;

            if (re - rs == 2)
                end = (end - start) * 0.5;
            else
                end = ((number_t)(end - start)) / (re - rs);

            switch (yend) {
                case 1:
                    for (rs++; rs < re; rs++) {
                        start += end, rs->position = start;
                        rs->price =
                            1 / (1 + myabs(fpos[rs - realloc] - start) * step);
                    }
                    break;
                case 2:
                    for (rs++; rs < re; rs++) {
                        start += end, rs->position = start;
                        rs->price = (myabs(fpos[rs - realloc] - start) * step);
                        if (rs == realloc || rs == rend - 1)
                            rs->price *= 500;
                    }
                    break;
                default:
                    for (rs++; rs < re; rs++) {
                        start += end, rs->position = start;
                        rs->price = (number_t)1;
                    }
                    break;
            }
        }
        rs = re;
    }
}

/*
 * mkrealloc_table - Dynamic programming algorithm for optimal row/column approximation
 *
 * This is the core of XaoS's zooming optimization. Instead of recalculating every pixel,
 * it determines which old rows/columns can be reused for new positions, and which must
 * be freshly calculated. See algorithms.md "Approximation Algorithm" section.
 *
 * The algorithm uses dynamic programming to find the lowest-cost mapping between old
 * and new positions, where:
 *   - Reusing an old row/column costs: (distance)^2
 *   - Calculating a new row/column costs: (4 * step)^2 = NEWPRICE
 *
 * Parameters:
 *   fpos     - Array of old row/column coordinates (floating-point)
 *   realloc  - Output table describing how to build the new frame
 *   size     - Number of rows or columns
 *   begin    - Starting coordinate of new viewport
 *   end      - Ending coordinate of new viewport
 *   sym      - Symmetry axis (if applicable)
 *   tmpdata  - Temporary buffer for dynamic programming tables
 */
static void mkrealloc_table(const number_t *fpos, realloc_t *realloc,
                            const unsigned int size, const number_t begin,
                            const number_t end, number_t sym,
                            unsigned char *tmpdata)
{
    unsigned int i;
    int counter;
    unsigned int ps, ps1 = 0, pe;  /* Previous start, previous start1, previous end */
    unsigned int p;                 /* Current position index */
    long int bestprice = MAXPRICE;
    realloc_t *r = realloc;        /* Save original realloc pointer */
    struct dyn_data *dyndata;      /* Dynamic programming state table */
    int yend, y;                   /* Range boundaries in fixed-point */
    struct dyn_data **best;        /* Best solution for previous row at each old position */
    struct dyn_data **best1, **tmp; /* Double buffering for best arrays */
    int *pos;                      /* Old positions converted to fixed-point */
    number_t step, tofix;          /* Step size and conversion factor */
    int symi = -1;                 /* Symmetry index */
    unsigned int lastplus = 0;     /* Last used old position */
    struct dyn_data *data;         /* Current state being evaluated */
    struct dyn_data *previous = NULL, *bestdata = NULL;
    int myprice;                   /* Price of current solution being evaluated */
#ifdef STATISTICS
    nadded = 0, nsymmetry = 0, nskipped = 0;
#endif

    /* Partition temporary buffer into arrays needed for dynamic programming.
     * Layout: [pos array] [best array] [best1 array] [dyndata array] */
    pos = (int *)tmpdata;
    best = (struct dyn_data **)(tmpdata + ALIGN((size + 2) * sizeof(int)));
    best1 = (struct dyn_data **)(tmpdata + ALIGN((size + 2) * sizeof(int)) +
                                 ALIGN(size * sizeof(struct dyn_data **)));
    dyndata = (struct dyn_data *)(tmpdata + ALIGN((size + 2) * sizeof(int)) +
                                  2 * ALIGN(size * sizeof(struct dyn_data **)));

    /* === INITIALIZATION PHASE ===
     * Convert floating-point old positions to fixed-point for fast integer arithmetic.
     * Fixed-point representation: 0 = begin, size*FPMUL = end */

    tofix = size * FPMUL / (end - begin);  /* Conversion factor to fixed-point */
    pos[0] = INT_MIN;  /* Guard value before first position */
    pos++;             /* Adjust pointer so pos[0] corresponds to fpos[0] */

    for (counter = (int)size - 1; counter >= 0; counter--) {
        /* Convert old position to fixed-point coordinate relative to new viewport */
        pos[counter] = (int)((fpos[counter] - begin) * tofix);

        /* Ensure positions remain sorted (fix floating-point rounding errors).
         * If a position is greater than the next, it would cause incorrect ordering. */
        if (counter < (int)size - 1 && pos[counter] > pos[counter + 1])
            pos[counter] = pos[counter + 1];
    }
    pos[size] = INT_MAX;  /* Guard value after last position */

    step = (end - begin) / (number_t)size;  /* Distance between adjacent new positions */

    /* Calculate symmetry axis position for optimization (if applicable) */
    if (begin > sym || sym > end)
        symi = -2;  /* Symmetry axis outside viewport */
    else
        symi = (int)((sym - begin) / step);  /* Index of symmetry axis */

    /* Initialize range tracking variables for dynamic programming.
     * ps/pe track which old positions fall within acceptable range of current new position. */
    ps = 0;   /* Previous range start */
    pe = 0;   /* Previous range end */
    y = 0;    /* Current new position in fixed-point */

    /* === DYNAMIC PROGRAMMING FORWARD PASS ===
     * This is the core algorithm. For each new position i, we evaluate all possible
     * ways to build an optimal solution:
     *   1. Calculate this position fresh (cost = NEWPRICE)
     *   2. Reuse an old position p within acceptable range (cost = distance^2)
     *
     * We build upon solutions for previous positions (i-1, i-2, ...) to find the
     * minimum total cost. The dyndata table stores all possibilities; best[] tracks
     * the optimal choice at each old position for the previous new position.
     *
     * Key constraint: We cannot reuse the same old position twice (no doubling).
     * This is enforced by only considering old positions >= the one used previously.
     */

    for (i = 0; i < size; i++, y += FPMUL) {
        bestprice = MAXPRICE;  /* Will track best solution found for position i */
        p = ps;                /* Start scanning old positions from previous range start */

        /* Swap best arrays for double buffering.
         * best1[] becomes the new best[] for position i-1
         * best[] will be filled with best solutions for position i */
        tmp = best1;
        best1 = best;
        best = tmp;

        /* Calculate acceptable range for old positions.
         * Only old positions within ±IRANGE of y can be used (beyond that, NEWPRICE is cheaper).
         * IRANGE is chosen so that PRICE(IRANGE, 0) ≈ NEWPRICE */
        yend = y - IRANGE;
        if (yend < -FPMUL)     /* Clamp to screen boundaries */
            yend = -FPMUL;

        while (pos[p] <= yend) /* Skip old positions below acceptable range */
            p++;
        ps1 = p;               /* Remember first old position in range */
        yend = y + IRANGE;     /* Upper bound of acceptable range */

        /* === OPTION 1: Calculate this position fresh ===
         * This option doesn't reuse any old position, so we just need to connect
         * optimally to the best solution from position i-1. */

        /* Find the best solution from position i-1 to base this decision on.
         * We need to connect to the best solution that doesn't use old positions >= p,
         * since we want to reserve those for position i. */
        if (ps != pe && p > ps) {
            /* Position i-1 had old positions in range. Find the best one before position p. */
            assert(p >= ps);
            if (p < pe) {
                previous = best[p - 1];  /* Best solution using old positions < p */
                CHECKPOS(previous);
            } else {
                previous = best[pe - 1]; /* All positions were < p, use the last one */
            }
            CHECKPOS(previous);
            myprice = previous->price;
        } else {
            /* Position i-1 had no old positions in range (was freshly calculated).
             * Connect to its solution directly. */
            if (i > 0) {
                previous = getbest(i - 1);  /* Solution for i-1 (no old position used) */
                myprice = previous->price;
            } else {
                previous = END;  /* Special marker: no previous solution (first position) */
                myprice = 0;
            }
        }

        /* Store the "calculate fresh" option in the dyndata table.
         * This solution has cost = previous cost + NEWPRICE */
        data = getbest(i);      /* Storage location for "no old position used" option */
        myprice += NEWPRICE;    /* Add cost of calculating this position */
        bestdata = data;        /* Currently this is the best option we've found */
        data->previous = previous;  /* Remember how we got here */
        bestprice = myprice;    /* Track best total cost so far */
        data->price = myprice;  /* Store this option's cost */
        assert(bestprice >= 0);
        data = adddata(p, i);   /* Move to storage for "reuse old position p" options */

        /* === OPTION 2: Reuse old positions ===
         * Now try reusing each old position p within acceptable range.
         * For each old position p, we calculate: previous_cost + PRICE(pos[p], y)
         * We can only use old position p if the previous solution used old position < p
         * (to prevent doubling). */
        if (ps != pe) {
            /* === CASE A: Previous position had old positions in range ===
             * Position i-1 had old positions in range, so we can potentially reuse them.
             * This is more complex because ranges overlap - we must ensure no doubling.
             * We process old positions in three phases:
             *   Phase 1: First position in range (special case - can't connect to i-1's solution)
             *   Phase 2: Overlapping positions (where both i-1 and i can use them)
             *   Phase 3: Remaining positions in range for i only */

            int price1 = INT_MAX;  /* Cost of best solution from i-1 at current p */

            /* --- Phase 1: First old position in range ---
             * At p == ps, we're at the first old position that i-1 could use.
             * If we use it for i, then i-1 couldn't have used it (no doubling).
             * So we can only connect to i-1's "calculate fresh" solution. */
            if (p == ps) {
                if (pos[p] != pos[p + 1]) {  /* Skip duplicate positions */
                    previous = getbest(i - 1);       /* i-1's "calculate fresh" solution */
                    myprice = previous->price;
                    myprice += PRICE(pos[p], y);     /* Add cost of using old position p */
                    if (myprice < bestprice) {       /* Is this better than calculating fresh? */
                        bestprice = myprice;
                        bestdata = data;
                        data->price = myprice;
                        data->previous = previous;
                    }
                }
                assert(bestprice >= 0);
                assert(myprice >= 0);
                best1[p] = bestdata;  /* Record best solution when using old position p */
                data += DSIZE;        /* Advance to next old position's storage */
                p++;
            }

            /* --- Phase 2: Overlapping range ---
             * For ps < p < pe, both i-1 and i have this old position in range.
             * We can connect to i-1's best solution using old position p-1.
             * This ensures no doubling and maintains sorted order. */
            previous = NULL;
            price1 = myprice;
            while (p < pe) {  /* Process all old positions that were in range for i-1 */
                if (pos[p] != pos[p + 1]) {  /* Skip duplicates */
                    /* Check if we need to update our base cost from i-1.
                     * We reuse the previous value when possible for efficiency. */
                    if (previous != best[p - 1]) {
                        /* i-1's best solution changed, update our base cost */
                        previous = best[p - 1];
                        CHECKPOS(previous);
                        price1 = myprice = previous->price;

                        /* Retroactive optimization (backward correction):
                         * We just discovered that best[p-1] has a much better cost than before.
                         * This means position p-1 (which we already processed) might have made
                         * a suboptimal choice. Go back and check: would p-1 be better off
                         * calculating FRESH and connecting to this cheaper best[p-1], rather
                         * than reusing an old position? If so, retroactively change p-1's decision.
                         *
                         * Example: p-1 chose to reuse old position with cost=1000, but if it
                         * calculated fresh and connected to best[p-1], cost would be 100+NEWPRICE=500.
                         * So we go back and change p-1's decision to "calculate fresh". */
                        if (myprice + NEWPRICE < bestprice) {  /* Common case (≈2/3 of time) */
                            bestprice = myprice + NEWPRICE;
                            bestdata = data - DSIZE;  /* Go back to p-1's storage */
                            (bestdata)->price = bestprice;
                            (bestdata)->previous = previous + nosetadd;  /* Special marker: "calculate fresh" */
                            best1[p - 1] = bestdata;  /* Update best solution for p-1 */
                        }
                    } else {
                        /* Same base cost as before, reuse it */
                        myprice = price1;
                    }

                    /* Calculate cost of using old position p for new position i */
                    myprice += PRICE(pos[p], y);

                    /* Is this better than our current best? */
                    if (myprice < bestprice) {  /* Common case (≈2/3 of time) */
                        bestprice = myprice;
                        bestdata = data;
                        data->price = myprice;
                        data->previous = previous;
                    } else if (pos[p] > y) {
                        /* Old position is past new position y, no point continuing.
                         * Future positions will be even farther away. */
                        best1[p] = bestdata;
                        data += DSIZE;
                        p++;
                        break;
                    }
                }

                assert(myprice >= 0);
                assert(bestprice >= 0);

                best1[p] = bestdata;  /* Record best solution at this old position */
                data += DSIZE;
                p++;
            }
            /* Continue filling best1[] for remaining positions in i-1's range.
             * These positions were already optimized in the loop above, just need
             * to record the best solutions. */
            while (p < pe) {
#ifdef DEBUG
                /* Sanity check: previous should be stable in this region */
                if (pos[p] != pos[p + 1]) {
                    if (previous != best[p - 1]) {
                        x_fatalerror("Misoptimization found!");
                    }
                }
#endif
                assert(myprice >= 0);
                assert(bestprice >= 0);

                best1[p] = bestdata;
                data += DSIZE;
                p++;
            }

            /* --- Phase 3: Positions in range for i but not i-1 ---
             * Now p >= pe, so these old positions are only in range for i.
             * The base cost from i-1 is stable (doesn't depend on p anymore),
             * so we can compute it once and reuse it. */

            if (p > ps) {
                /* Use i-1's best solution at the last old position it considered */
                previous = best[p - 1];
                CHECKPOS(previous);
                price1 = previous->price;
            } else {
                /* i-1 didn't use any old positions */
                previous = getbest(i - 1);
                price1 = previous->price;
            }

            /* Final retroactive optimization:
             * Now that we have the final base cost from i-1, do one last check:
             * should position p-1 recalculate fresh instead of reusing an old position?
             * This is the same backward correction as above, but done once we know
             * the base cost won't change anymore. */
            if (price1 + NEWPRICE < bestprice && p > ps1) {
                myprice = price1 + NEWPRICE;
                bestprice = myprice;
                bestdata = data - DSIZE;  /* Go back to p-1 */
                (bestdata)->price = myprice;
                (bestdata)->previous = previous + nosetadd;  /* Mark as "calculate fresh" */
                best1[p - 1] = bestdata;
                myprice -= NEWPRICE;  /* Restore for reuse below */
            }

            /* Process remaining old positions in range for i */
            while (pos[p] < yend) {
                if (pos[p] != pos[p + 1]) {
                    myprice = price1;
                    myprice += PRICE(pos[p], y);
                    if (myprice < bestprice) {
                        bestprice = myprice;
                        bestdata = data;
                        data->price = myprice;
                        data->previous = previous;
                    } else if (pos[p] > y) {
                        /* Past the target, stop */
                        break;
                    }
                }

                assert(bestprice >= 0);
                assert(myprice >= 0);

                best1[p] = bestdata;
                data += DSIZE;
                p++;
            }

            /* Fill in remaining positions with the best solution found */
            while (pos[p] < yend) {
                best1[p] = bestdata;
                p++;
            }

        } else {
            /* === CASE B: Previous position had no old positions in range ===
             * Position i-1 was calculated fresh (no old positions were in acceptable range).
             * This simplifies things: there's no overlap to worry about, and our decisions
             * don't affect i-1's solution. We just connect to i-1's solution and try
             * each old position in range. This is similar to Phase 3 above. */

            int myprice1;  /* Base cost from i-1 (constant for all old positions) */

            if (pos[p] < yend) {
                /* Get base cost from i-1 */
                if (i > 0) {
                    previous = getbest(i - 1);
                    myprice1 = previous->price;
                } else {
                    previous = END;
                    myprice1 = 0;
                }

                /* Try each old position in range */
                while (pos[p] < yend) {
                    if (pos[p] != pos[p + 1]) {
                        myprice = myprice1 + PRICE(pos[p], y);
                        if (myprice < bestprice) {
                            data->price = myprice;
                            data->previous = previous;
                            bestprice = myprice;
                            bestdata = data;
                        } else if (pos[p] > y) {
                            break;  /* Past target, stop */
                        }
                    }
                    assert(bestprice >= 0);
                    assert(myprice >= 0);
                    best1[p] = bestdata;
                    p++;
                    data += DSIZE;
                }

                /* Fill remaining positions with best solution */
                while (pos[p] < yend) {
                    best1[p] = bestdata;
                    p++;
                }
            }
        }

        /* Update range tracking for next iteration.
         * ps, pe track the range of old positions that were in range for the previous
         * new position. This helps us efficiently process the overlapping regions. */
        ps = ps1;   /* Start of range for current position becomes previous start */
        ps1 = pe;   /* End of previous range becomes ps1 for next iteration */
        pe = p;     /* Current end becomes previous end for next iteration */
    }

    assert(bestprice >= 0);

    /* === DYNAMIC PROGRAMMING BACKWARD PASS (Backtracking) ===
     * The forward pass filled the dyndata table with all possible solutions.
     * Now we trace backward from the last position to reconstruct the optimal path.
     * bestdata points to the best solution for position size-1; we follow the
     * "previous" pointers to recover the complete solution. */

    realloc = realloc + size;  /* Point to end of realloc table */

    /* Determine if we need special handling for positions at viewport boundaries.
     * yend encodes boundary conditions for newpositions() function. */
    yend = (int)((begin > fpos[0]) && (end < fpos[size - 1]));
    if (pos[0] > 0 && pos[size - 1] < (int)size * FPMUL)
        yend = 2;

    /* Backtrack through the optimal solution to fill the realloc table.
     * We go backward from position size-1 to 0, following the chain of
     * "previous" pointers that represent the optimal decisions. */
    for (i = size; i > 0;) {
        struct dyn_data *bestdata1;
        realloc--;  /* Move backward in realloc table */
        i--;
        realloc->symto = -1;   /* Initialize symmetry fields */
        realloc->symref = -1;
        bestdata1 = bestdata->previous;  /* Follow the optimal path backward */

        /* Decode what action was taken for this position.
         * The encoding uses pointer arithmetic tricks:
         *   - Pointers >= (dyndata + nosetadd) mean "calculate fresh"
         *   - Pointers >= (dyndata + size<<DSIZES) mean "calculate fresh"
         *   - Other pointers encode which old position was reused */

        if (bestdata1 >= dyndata + nosetadd ||
            bestdata >= dyndata + ((size) << DSIZES)) {
            /* This position should be calculated fresh (new row/column) */
            if (bestdata1 >= dyndata + nosetadd)
                bestdata1 -= nosetadd;  /* Remove the special marker */

            realloc->recalculate = 1;  /* Mark for fresh calculation */
            STAT(nadded++);
            realloc->dirty = 1;        /* Needs calculation */
            lastplus++;                /* Assign a calculation slot */

            if (lastplus >= size)
                lastplus = 0;

            realloc->plus = lastplus;  /* Store which slot to use */

        } else {
            /* This position reuses an old row/column.
             * Decode which old position from the pointer value. */
            p = ((unsigned int)(bestdata - dyndata)) >> DSIZES;
            assert(p < size);
            realloc->position = fpos[p];   /* Use old position p */
            realloc->plus = p;             /* Index of old position */
            realloc->dirty = 0;            /* Already calculated */
            realloc->recalculate = 0;      /* Don't recalculate */
            lastplus = p;
        }
        bestdata = bestdata1;  /* Continue backtracking */
    }

    /* === POST-PROCESSING ===
     * Refine the positions and apply optimizations */

    newpositions(realloc, size, begin, end, fpos, yend);  /* Adjust positions for smoothness */
    realloc = r;  /* Restore original pointer */

    /* Apply symmetry optimizations if symmetry axis is within viewport */
    if (symi <= (int)size && symi >= 0) {
        preparesymmetries(r, (int)size, symi, sym, step);
    }

    STAT(printf("%i added %i skipped %i mirrored\n", nadded, nskipped,
                nsymmetry));
    STAT(nadded2 += nadded; nskipped2 += nskipped; nsymmetry2 += nsymmetry);
}

struct movedata {
    unsigned int size;
    unsigned int start;
    unsigned int plus;
};
int avgsize;
/*
 * this function prepares fast moving table for moveoldpoints
 * see xaos.info for details. It is not threaded since it is quite
 * fast.
 */
static void preparemoveoldpoints(void)
{
    struct movedata *data, *sizend;
    realloc_t *rx, *rx1, *rend1;
    int sum = 0, num = 0;
    int plus1 = 0;

    data = (struct movedata *)tmpdata;
    for (rx = czoomc.reallocx, rend1 = rx + cimage.width; rx < rend1; rx++)
        if ((rx->dirty) && plus1 < cimage.width + 1)
            plus1++;
        else
            break;
    data->start = czoomc.reallocx->plus;
    data->size = 0;
    data->plus = plus1;
    rend1--;
    while (rend1->dirty) {
        if (rend1 == czoomc.reallocx)
            return;
        rend1--;
    }
    rend1++;
    for (; rx < rend1; rx++) {
        if ((rx->dirty || rx->plus == data->start + data->size))
            data->size++;
        else {
            if (data->size) {
                plus1 = 0;
                rx1 = rx - 1;
                while (rx1 > czoomc.reallocx && rx1->dirty)
                    plus1++, data->size--, rx1--;
                if (!(data->start + data->size < (unsigned int)cimage.width) &&
                    !rx->dirty) {
                    int i;
                    if (rx == rend1)
                        break;
                    for (i = 0; rx->dirty && rx < rend1; rx++)
                        i++;
                    data++;
                    data->plus = plus1;
                    data->size = (unsigned int)i;
                    data->start = rx->plus - i;
                } else {
                    sum += data->size;
                    num++;
                    data++;
                    data->plus = plus1;
                    data->start = rx->plus;
                }
            } else
                data->start = rx->plus;
            assert(rx->plus < (unsigned int)cimage.width);
            data->size = 1;
        }
    }
    if (data->size) {
        sizend = data + 1;
        sum += data->size;
        rx1 = rx - 1;
        while (rx1 > czoomc.reallocx && rx1->dirty)
            data->size--, rx1--;
        num++;
    } else
        sizend = data;
    sizend->size = 0;
    if (cimage.bytesperpixel != 1) {
        sum *= cimage.bytesperpixel;
        for (data = (struct movedata *)tmpdata; data < sizend; data++) {
            data->plus *= cimage.bytesperpixel;
            data->size *= cimage.bytesperpixel;
            data->start *= cimage.bytesperpixel;
        }
    }
    if (num)
        avgsize = sum / num;
}

static void moveoldpoints(void * /*data1*/, struct taskinfo * /*task*/, int r1,
                          int r2)
{
    struct movedata *data;
    unsigned char *vline, *vbuff;
    realloc_t *ry, *rend;
    int i = r1;

    for (ry = czoomc.reallocy + r1, rend = czoomc.reallocy + r2; ry < rend;
         ry++, i++) {
        if (!ry->dirty) {
            assert(ry->plus < (unsigned int)cimage.height);
            vbuff = cimage.currlines[i];
            vline = cimage.oldlines[ry->plus];
            for (data = (struct movedata *)tmpdata; data->size; data++) {
                vbuff += data->plus;
                memcpy(vbuff, vline + data->start, (size_t)data->size),
                    vbuff += data->size;
            }
        }
    }
}

/* This function prepares fast filling tables for fillline */
static int mkfilltable(void)
{
    int vsrc;
    int pos;
    realloc_t *rx, *r1, *r2, *rend, *rend2;
    int n = 0;
    int num = 0;
    struct filltable *tbl = (struct filltable *)tmpdata;

    pos = 0;
    vsrc = 0;

    rx = czoomc.reallocx;
    while (rx > czoomc.reallocx && rx->dirty)
        rx--;
    for (rend = czoomc.reallocx + cimage.width,
        rend2 = czoomc.reallocx + cimage.width;
         rx < rend; rx++) {
        if (rx->dirty) {
            r1 = rx - 1;
            for (r2 = rx + 1; r2 < rend2 && r2->dirty; r2++)
                ;
            while (rx < rend2 && rx->dirty) {
                n = (int)(r2 - rx);
                assert(n > 0);
                if (r2 < rend2 &&
                    (r1 < czoomc.reallocx ||
                     rx->position - r1->position > r2->position - rx->position))
                    vsrc = (int)(r2 - czoomc.reallocx), r1 = r2;
                else {
                    vsrc = (int)(r1 - czoomc.reallocx);
                    if (vsrc < 0)
                        goto end;
                }
                pos = (int)(rx - czoomc.reallocx);
                assert(pos >= 0 && pos < cimage.width);
                assert(vsrc >= 0 && vsrc < cimage.width);

                tbl[num].length = n;
                tbl[num].to = pos * cimage.bytesperpixel;
                tbl[num].from = vsrc * cimage.bytesperpixel;
                tbl[num].end =
                    tbl[num].length * cimage.bytesperpixel + tbl[num].to;
                /*printf("%i %i %i %i\n",num,tbl[num].length, tbl[num].to,
                 * tbl[num].from); */
                while (n) {
                    rx->position = czoomc.reallocx[vsrc].position;
                    rx->dirty = 0;
                    rx++;
                    n--;
                }
                num++;
            } /*while rx->dirty */
        }     /*if rx->dirty */
    }         /*for czoomc */
end:
    tbl[num].length = 0;
    tbl[num].to = pos;
    tbl[num].from = vsrc;
    return num;
}

static void filly(void * /*data*/, struct taskinfo * /*task*/, int rr1, int rr2)
{
    unsigned char **vbuff = cimage.currlines;
    realloc_t *ry, *r1, *r2, *rend, *rend2, *rs = NULL;
    int linesize = cimage.width * cimage.bytesperpixel;

    ry = czoomc.reallocy + rr1;

    ry = czoomc.reallocy + rr1;
    while (ry > czoomc.reallocy && ry->dirty > 0)
        ry--;
    for (rend = czoomc.reallocy + rr2, rend2 = czoomc.reallocy + cimage.height;
         ry < rend; ry++) {
        if (ry->dirty > 0) {
            incincomplete();
            r1 = ry - 1;
            for (r2 = ry + 1; r2 < rend2 && r2->dirty > 0; r2++)
                ;
            if (r2 >= rend2 && (rr2 != cimage.height || ry == 0))
                return;
            while (ry < rend2 && ry->dirty > 0) {
                if (r1 < czoomc.reallocy) {
                    rs = r2;
                    if (r2 >= rend2)
                        return;
                } else if (r2 >= rend2)
                    rs = r1;
                else if (ry->position - r1->position <
                         r2->position - ry->position)
                    rs = r1;
                else
                    rs = r2;
                if (!rs->dirty) {
                    drivercall(cimage, tpl::fillline<Pixel8Traits>(rs - czoomc.reallocy),
                               tpl::fillline<Pixel16Traits>(rs - czoomc.reallocy),
                               tpl::fillline<Pixel24Traits>(rs - czoomc.reallocy),
                               tpl::fillline<Pixel32Traits>(rs - czoomc.reallocy));
                    ry->dirty = -1;
                }
                memcpy(vbuff[ry - czoomc.reallocy], vbuff[rs - czoomc.reallocy],
                       (size_t)linesize);
                ry->position = rs->position;
                ry->dirty = -1;
                ry++;
            }
        }
        if (ry < rend && !ry->dirty) {
            drivercall(cimage, tpl::fillline<Pixel8Traits>(ry - czoomc.reallocy),
                       tpl::fillline<Pixel16Traits>(ry - czoomc.reallocy),
                       tpl::fillline<Pixel24Traits>(ry - czoomc.reallocy),
                       tpl::fillline<Pixel32Traits>(ry - czoomc.reallocy));
            ry->dirty = -1;
        }
    }
}

static void fill(void)
{
    if (cfilter.interrupt) {
        cfilter.pass = "reducing resolution";
        mkfilltable();
        xth_function(filly, NULL, cimage.height);
    }
    xth_sync();
}

static void calculatenew(void * /*data*/, struct taskinfo *task, int /*r1*/,
                         int /*r2*/)
{
    int s;
    int i, y;
    realloc_t *rx, *ry, *rend;
    int range = cfractalc.range * 2;
    int positions[16];
    int calcpositions[16];
    /*int s3; */
    if (range < 1)
        range = 1;
    if (range > 16)
        range = 16;
    memset(positions, 0, sizeof(positions));
    calcpositions[0] = 0;
    positions[0] = 1;
    for (s = 1; s < range;) {
        for (i = 0; i < range; i++) {
            if (!positions[i]) {
                for (y = i; y < range && !positions[y]; y++)
                    ;
                positions[(y + i) / 2] = 1;
                calcpositions[s++] = (y + i) / 2;
            }
        }
    }

    if (!xth_nthread(task)) {
        STAT(tocalculate = 0);
        STAT(avoided = 0);
        cfilter.pass = TR("Message", "Solid guessing 1");
        cfilter.max = 0;
        cfilter.pos = 0;
    }

    /* We don't need to wory about race conditions here, since only
     * problem that should happen is incorrectly counted number
     * of lines to do...
     *
     * I will fix that problem later, but I think that this information
     * should be quite useless at multithreaded systems so it should
     * be a bit inaccurate. Just need to take care in percentage
     * displayers that thinks like -100% or 150% should happen
     */
    if (!xth_nthread(task)) {
        for (ry = czoomc.reallocy, rend = ry + cimage.height; ry < rend; ry++) {
            if (ry->recalculate)
                cfilter.max++;
        }
        for (rx = czoomc.reallocx, rend = rx + cimage.width; rx < rend; rx++) {
            if (rx->recalculate) {
                cfilter.max++;
            }
        }
    }
    tcallwait();
    for (s = 0; s < range; s++) {
        for (ry = czoomc.reallocy + calcpositions[s],
            rend = czoomc.reallocy + cimage.height;
             ry < rend; ry += range) {
            xth_lock(0);
            if (ry->recalculate == 1) {
                ry->recalculate = 2;
                xth_unlock(0);
                setchanged(1);
                ry->dirty = 0;
                calcline(ry);
                cfilter.pos++;
#ifndef DRAW
                tcallwait();
#endif
                if (cfilter.interrupt) {
                    break;
                }
            } else {
                xth_unlock(0);
            }
        } /*for ry */
        for (rx = czoomc.reallocx + calcpositions[s],
            rend = czoomc.reallocx + cimage.width;
             rx < rend; rx += range) {
            xth_lock(1);
            if (rx->recalculate == 1) {
                rx->recalculate = 2;
                xth_unlock(1);
                setchanged(1);
                rx->dirty = 0;
                calccolumn(rx);
                cfilter.pos++;
#ifndef DRAW
                tcallwait();
#endif
                if (cfilter.interrupt) {
                    return;
                }
            } else {
                xth_unlock(1);
            }
        }
    }
    STAT(
        printf("Avoided calculating %i points from %i and %2.2f%% %2.2f%%\n",
               avoided, tocalculate, 100.0 * (avoided) / tocalculate,
               100.0 * (tocalculate - avoided) / cimage.width / cimage.height));
    STAT(avoided2 += avoided; tocalculate2 += tocalculate; frames2 += 1);
}

static void addprices(realloc_t *r, realloc_t *r2);

static void addprices(realloc_t *r, realloc_t *r2)
{
    realloc_t *r3;
    while (r < r2) {
        r3 = r + (((unsigned int)(r2 - r)) >> 1);
        r3->price = (r2->position - r3->position) * (r3->price);
        if (r3->symref != -1)
            r3->price = r3->price / 2;
        addprices(r, r3);
        r = r3 + 1;
    }
}

/* We can't do both symmetryies (x and y) in one loop at multithreaded
 * systems,since we need to take care to points at the cross of symmetrized
 * point/column
 */
static void dosymmetry(void * /*data*/, struct taskinfo * /*task*/, int r1,
                       int r2)
{
    unsigned char **vbuff = cimage.currlines + r1;
    realloc_t *ry, *rend;
    int linesize = cimage.width * cimage.bytesperpixel;

    for (ry = czoomc.reallocy + r1, rend = czoomc.reallocy + r2; ry < rend;
         ry++) {
        assert(ry->symto >= 0 || ry->symto == -1);
        if (ry->symto >= 0) {
            assert(ry->symto < cimage.height);
            if (!czoomc.reallocy[ry->symto].dirty) {
                memcpy(*vbuff, cimage.currlines[ry->symto], (size_t)linesize);
                ry->dirty = 0;
            }
        }
        vbuff++;
    }
}

/*Well, classical simple quicksort. Should be faster than the library one
 *because of the reduced number of function calls :)
 */
static inline void myqsort(realloc_t **start, realloc_t **end)
{
    number_t med;
    realloc_t **left = start, **right = end - 1;
    while (1) {

        /*Quite strange calculation of median, but should be
         *as good as Sedgewick's middle-of-three method and is faster*/
        med = ((*start)->price + (*(end - 1))->price) * 0.5;

        /*Avoid one comparison */
        if (med > (*start)->price) {
            realloc_t *tmp;
            tmp = *left;
            *left = *right;
            *right = tmp;
        }
        right--;
        left++;

        while (1) {
            realloc_t *tmp;

            while (left < right && (*left)->price > med)
                left++;
            while (left < right && med > (*right)->price)
                right--;

            if (left < right) {
                tmp = *left;
                *left = *right;
                *right = tmp;
                left++;
                right--;
            } else
                break;
        }
        if (left - start > 1)
            myqsort(start, left);
        if (end - right <= 2)
            return;
        left = start = right;
        right = end - 1;
    }
}

static int tocalcx, tocalcy;
static void processqueue(void *data, struct taskinfo *task, int /*r1*/,
                         int /*r2*/)
{
    realloc_t **tptr = (realloc_t **)data, **tptr1 = (realloc_t **)tmpdata;
    realloc_t *r, *end;
    end = czoomc.reallocx + cimage.width;

    while (tptr1 < tptr && (!cfilter.interrupt || tocalcx == cimage.width ||
                            tocalcy == cimage.height)) {
        xth_lock(0);
        r = *tptr1;
        if (r != NULL) {
            *tptr1 = NULL;
            xth_unlock(0);
            cfilter.pos++;
            if (tocalcx < cimage.width - 2 && tocalcy < cimage.height - 2)
                cfilter.readyforinterrupt = 1;
            tcallwait();
            if (r >= czoomc.reallocx && r < end) {
                r->dirty = 0;
                tocalcx--;
                calccolumn(r);
            } else {
                r->dirty = 0;
                tocalcy--;
                calcline(r);
            }
        } else {
            xth_unlock(0);
        }
        tptr1++;
    }
}

/*
 * Another long unthreaded code. It seems to be really long and
 * ugly, but believe it or not it takes just about 4% of calculation time,
 * so why worry about it? :)
 *
 * This code looks for columns/lines to calculate, adds them into a queue,
 * sorts it in order of significance and then calls parallel processqueue,
 * which does the job.
 */
static void calculatenewinterruptible(void)
{
    realloc_t *r, *r2, *end, *end1;
    realloc_t **table, **tptr;

    /*tptr = table = (realloc_t **) malloc (sizeof (*table) * (cimage.width +
     * cimage.height)); */
    tptr = table = (realloc_t **)tmpdata;
    end = czoomc.reallocx + cimage.width;
    tocalcx = 0, tocalcy = 0;

    STAT(tocalculate = 0);
    STAT(avoided = 0);

    cfilter.pass = TR("Message", "Solid guessing");

    for (r = czoomc.reallocx; r < end; r++)
        if (r->dirty)
            tocalcx++, setchanged(1);

    for (r = czoomc.reallocx; r < end; r++) {
        if (r->recalculate) {
            for (r2 = r; r2 < end && r2->recalculate; r2++)
                *(tptr++) = r2;
            if (r2 == end)
                /*(r2 - 1)->price = 0, */
                r2--;
            addprices(r, r2);
            r = r2;
        }
    }

    end1 = czoomc.reallocy + cimage.height;

    for (r = czoomc.reallocy; r < end1; r++)
        if (r->dirty)
            tocalcy++, setchanged(1);

    for (r = czoomc.reallocy; r < end1; r++) {
        if (r->recalculate) {
            for (r2 = r; r2 < end1 && r2->recalculate; r2++)
                *(tptr++) = r2;
            if (r2 == end1)
                /*(r2 - 1)->price = 0, */
                r2--;
            addprices(r, r2);
            r = r2;
        }
    }
    if (table != tptr) {

        if (tptr - table > 1)
            myqsort(table, tptr);

        cfilter.pos = 0;
        cfilter.max = (int)(tptr - table);
        cfilter.incalculation = 1;
        callwait();

        xth_function(processqueue, tptr, 1);

        callwait();
    }

    cfilter.pos = 0;
    cfilter.max = 0;
    cfilter.pass = "Processing symmetries";
    cfilter.incalculation = 0;
    callwait();

    xth_sync();
    if (nsymmetrized) {
        xth_function(dosymmetry, NULL, cimage.height);
        xth_sync();
        drivercall(cimage, xth_function(tpl::dosymmetry2<Pixel8Traits>, NULL, cimage.width),
                   xth_function(tpl::dosymmetry2<Pixel16Traits>, NULL, cimage.width),
                   xth_function(tpl::dosymmetry2<Pixel24Traits>, NULL, cimage.width),
                   xth_function(tpl::dosymmetry2<Pixel32Traits>, NULL, cimage.width));
        xth_sync();
    }
    if (cfilter.interrupt) {
        cfilter.pass = "reducing resolution";
        mkfilltable();
        xth_function(filly, NULL, cimage.height);
    }
    xth_sync();

    STAT(
        printf("Avoided calculating %i points from %i and %2.2f%% %2.2f%%\n",
               avoided, tocalculate, 100.0 * (avoided) / tocalculate,
               100.0 * (tocalculate - avoided) / cimage.width / cimage.height));
    STAT(avoided2 += avoided; tocalculate2 += tocalculate; frames2 += 1);
}

static void init_tables(struct filter *f)
{
    int i;
    zoom_context *c = getzcontext(f);

    /*c->dirty = 2; */
    for (i = 0; i < f->image->width + 1; i++)
        c->xpos[i] =
            (-f->fractalc->rs.nc + f->fractalc->rs.mc) + f->fractalc->rs.mc;
    for (i = 0; i < f->image->height + 1; i++)
        c->ypos[i] =
            (-f->fractalc->rs.ni + f->fractalc->rs.mi) + f->fractalc->rs.mi;
}

static int alloc_tables(struct filter *f)
{
    zoom_context *c = getzcontext(f);
    c->xpos = (number_t *)malloc((f->image->width + 8) * sizeof(*c->xpos));
    if (c->xpos == NULL)
        return 0;
    c->ypos = (number_t *)malloc((f->image->height + 8) * sizeof(*c->ypos));
    if (c->ypos == NULL) {
        free((void *)c->xpos);
        return 0;
    }
    c->reallocx =
        (realloc_t *)malloc(sizeof(realloc_t) * (f->image->width + 8));
    if (c->reallocx == NULL) {
        free((void *)c->xpos);
        free((void *)c->ypos);
        return 0;
    }
    c->reallocy =
        (realloc_t *)malloc(sizeof(realloc_t) * (f->image->height + 8));
    if (c->reallocy == NULL) {
        free((void *)c->xpos);
        free((void *)c->ypos);
        free((void *)c->reallocx);
        return 0;
    }
    return 1;
}

static void free_tables(struct filter *f)
{
    zoom_context *c = getzcontext(f);
    if (c->xpos != NULL)
        free((void *)c->xpos), c->xpos = NULL;
    if (c->ypos != NULL)
        free((void *)c->ypos), c->ypos = NULL;
    if (c->reallocx != NULL)
        free((void *)c->reallocx), c->reallocx = NULL;
    if (c->reallocy != NULL)
        free((void *)c->reallocy), c->reallocy = NULL;
}

static void free_context(struct filter *f)
{
    zoom_context *c;
    c = getzcontext(f);
    free_tables(f);
    free((void *)c);
    f->data = NULL;
}

static zoom_context *make_context(void)
{
    zoom_context *new_ctxt;

    new_ctxt = (zoom_context *)calloc(1, sizeof(zoom_context));
    if (new_ctxt == NULL)
        return NULL;
    new_ctxt->forversion = -1;
    new_ctxt->newcalc = 1;
    new_ctxt->reallocx = NULL;
    new_ctxt->reallocy = NULL;
    new_ctxt->xpos = NULL;
    new_ctxt->ypos = NULL;
    new_ctxt->incomplete = 0;
    return (new_ctxt);
}

static void startbgmkrealloc(void * /*data*/, struct taskinfo * /*task*/,
                             int /*r1*/, int /*r2*/)
{
    mkrealloc_table(czoomc.ypos, czoomc.reallocy, (unsigned int)cimage.height,
                    cfractalc.rs.ni, cfractalc.rs.mi, cursymmetry.ysym,
                    tmpdata1);
}

static int do_fractal(struct filter *f, int flags, int /*time*/)
{
    number_t *posptr;
    int maxres;
    int size;
    int rflags = 0;
    realloc_t *r, *rend;

    f->image->flip(f->image);
    cfilter = *f;
    set_fractalc(f->fractalc, f->image);

    if (getzcontext(f)->forversion != f->fractalc->version ||
        getzcontext(f)->newcalc ||
        getzcontext(f)->forpversion != f->image->palette->version) {
        clear_image(f->image);
        free_tables(f);
        if (!alloc_tables(f))
            return 0;
        init_tables(f);
        getzcontext(f)->newcalc = 0;
        getzcontext(f)->forversion = getfcontext(f)->version;
        getzcontext(f)->forpversion = f->image->palette->version;
        czoomc = *getzcontext(f);
        if (BTRACEOK && !(flags & INTERRUPTIBLE)) {
            boundarytraceall(czoomc.xpos, czoomc.ypos);
            f->flags &= ~ZOOMMASK;
            return CHANGED | (cfilter.interrupt ? INCOMPLETE : 0);
        }
    } else
        rflags |= INEXACT;

    czoomc = *getzcontext(f);

    setincomplete(0);
    setchanged(0);

    maxres = cimage.width;
    if (maxres < cimage.height)
        maxres = cimage.height;
    size = ALIGN((maxres) * (DSIZE + 1) * (int)sizeof(struct dyn_data)) +
           2 * ALIGN(maxres * (int)sizeof(struct dyn_data **)) +
           ALIGN((maxres + 2) * (int)sizeof(int));
    tmpdata = (unsigned char *)malloc(size);
    if (tmpdata == NULL) {
        x_error(
            "XaoS fatal error:Could not allocate memory for temporary data of size %i. "
            "I am unable to handle this problem so please resize to smaller window.",
            size);
        return 0;
    }
    if (nthreads != 1) {
        tmpdata1 = (unsigned char *)malloc(size);
        if (tmpdata1 == NULL) {
            x_error(
                "XaoS fatal error:Could not allocate memory for temporary data of size %i. "
                "I am unable to handle this problem so please resize to smaller window",
                size);
            return 0;
        }
    } else
        tmpdata1 = tmpdata;

    cfilter.incalculation = 0;
    cfilter.readyforinterrupt = 0;
    cfilter.interrupt = 0;

    nsymmetrized = 0;
    cfilter.max = 0;
    cfilter.pos = 0;
    cfilter.pass = "Making y realloc table";
    xth_bgjob(startbgmkrealloc, NULL);

    cfilter.pass = "Making x realloc table";
    mkrealloc_table(czoomc.xpos, czoomc.reallocx, (unsigned int)cimage.width,
                    cfractalc.rs.nc, cfractalc.rs.mc, cursymmetry.xsym,
                    tmpdata);

    callwait();

    cfilter.pass = "Moving old points";
    callwait();
    preparemoveoldpoints();
    xth_sync();
    xth_function(moveoldpoints, NULL, cimage.height);

    cfilter.pass = "Starting calculation";
    callwait();
    xth_sync();
    if (flags & INTERRUPTIBLE)
        calculatenewinterruptible();
    else {
        xth_function(calculatenew, NULL, 1);
        if (cfilter.interrupt) {
            getzcontext(f)->incomplete = 1;
        }
        cfilter.pos = 0;
        cfilter.max = 0;
        cfilter.pass = "Processing symmetries";
        callwait();
        xth_sync();
        if (nsymmetrized) {
            xth_function(dosymmetry, NULL, cimage.height);
            xth_sync();
            drivercall(cimage, xth_function(tpl::dosymmetry2<Pixel8Traits>, NULL, cimage.width),
                       xth_function(tpl::dosymmetry2<Pixel16Traits>, NULL, cimage.width),
                       xth_function(tpl::dosymmetry2<Pixel24Traits>, NULL, cimage.width),
                       xth_function(tpl::dosymmetry2<Pixel32Traits>, NULL, cimage.width));
            xth_sync();
        }
        if (getzcontext(f)->incomplete) {
            fill();
        }
    }
    for (r = czoomc.reallocx, posptr = czoomc.xpos,
        rend = czoomc.reallocx + cimage.width;
         r < rend; r++, posptr++) {
        *posptr = r->position;
    }
    for (r = czoomc.reallocy, posptr = czoomc.ypos,
        rend = czoomc.reallocy + cimage.height;
         r < rend; r++, posptr++) {
        *posptr = r->position;
    }
#ifdef STATISTICS
    STAT(printf("Statistics: frames %i\n"
                "mkrealloctable: added %i, symmetry %i\n"
                "calculate loop: tocalculate %i avoided %i\n"
                "calculate:calculated %i inside %i\n"
                "iters inside:%i iters outside:%i periodicty:%i\n",
                frames2, nadded2, nsymmetry2, tocalculate2, avoided2,
                ncalculated2, ninside2, niter2, niter1, nperi));
#endif
    f->flags &= ~ZOOMMASK;
    if (getzcontext(f)->incomplete)
        rflags |= INCOMPLETE, f->flags |= INCOMPLETE;
    if (getzcontext(f)->incomplete > (cimage.width + cimage.height) / 2)
        f->flags |= LOWQUALITY;
    if (getzcontext(f)->changed)
        rflags |= CHANGED;
    free(tmpdata);
    if (nthreads != 1)
        free(tmpdata1);
    return rflags;
}

static struct filter *getinstance(const struct filteraction *a)
{
    struct filter *f = createfilter(a);
    f->data = make_context();
    f->name = "Zooming engine";
    return (f);
}

static void destroyinstance(struct filter *f)
{
    free_context(f);
    free(f);
}

static int requirement(struct filter *f, struct requirements *r)
{
    r->nimages = 2;
    r->supportedmask = C256 | TRUECOLOR | TRUECOLOR24 | TRUECOLOR16 |
                       LARGEITER | SMALLITER | GRAYSCALE;
    r->flags = IMAGEDATA | TOUCHIMAGE;
    return (f->next->action->requirement(f->next, r));
}

static int initialize(struct filter *f, struct initdata *i)
{
#ifdef USE_MULTABLE
    if (!multable[0]) {
        int i;
        mulmid = multable + RANGE * FPMUL;
        for (i = -RANGE * FPMUL; i < RANGE * FPMUL; i++)
            mulmid[i] = i * i;
    }
#endif
    inhermisc(f, i);
    if (i->image != f->image || datalost(f, i))
        getzcontext(f)->forversion = -1, f->image = i->image;
    f->imageversion = i->image->version;
    return (1);
}

const struct filteraction zoom_filter = {
    "XaoS's zooming engine",
    "zoom",
    0,
    getinstance,
    destroyinstance,
    do_fractal,
    requirement,
    initialize,
    convertupgeneric,
    convertdowngeneric,
    NULL,
};
