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
 *
 * All ugly architecture depended timing code is separated into this file..
 */
#include "config.h"
#include "timers.h"
#include <cstdlib>
#include <climits>
#include <chrono>
#include <thread>

#define EMULDIV 1024
struct timeemulator {
    unsigned long int time;
    unsigned long exact;
};

struct timer {
    std::chrono::time_point<std::chrono::steady_clock> lastactivated;
    unsigned long lastemulated;
    struct timeemulator *emulator;
    void (*handler)(void *);
    void (*multihandler)(void *, int);
    void *userdata;
    struct timer *next, *previous, *group;
    int interval;
    int stopped;
    int stoppedtime;
    int slowdown;
};

/*Variable for saving current time */
std::chrono::time_point<std::chrono::steady_clock> currenttime;

static tl_group group1;
tl_group *syncgroup = &group1;

/*following functions are architecture dependent */
void tl_update_time(void) { currenttime = std::chrono::steady_clock::now(); }

static inline int __lookup_timer(tl_timer *t)
{
    return std::chrono::duration_cast<std::chrono::microseconds>(
               currenttime - t->lastactivated)
        .count();
}

int tl_lookup_timer(tl_timer *t)
{
    if (t->stopped) {
        return (t->stoppedtime);
    }
    if (t->emulator != NULL) {
        return ((int)(t->emulator->time - t->lastemulated) * EMULDIV);
    }
    return (__lookup_timer(t) - t->slowdown);
}

void tl_stop_timer(tl_timer *t)
{
    if (!t->stopped) {
        t->stoppedtime = tl_lookup_timer(t);
        t->stopped = 1;
    }
}

void tl_slowdown_timer(tl_timer *t, int time)
{
    if (!t->stopped) {
        t->slowdown += time;
    } else
        t->stoppedtime -= time;
}

void tl_resume_timer(tl_timer *t)
{
    if (t->stopped) {
        t->stopped = 0;
        t->slowdown = tl_lookup_timer(t) - t->stoppedtime;
    }
}

void tl_sleep(int time)
{
    std::this_thread::sleep_for(std::chrono::microseconds(time));
}

void tl_reset_timer(tl_timer *t)
{
    if (t->stopped)
        t->stoppedtime = 0;
    else {
        if (t->emulator != NULL) {
            t->lastemulated = t->emulator->time;
        } else
            t->lastactivated = currenttime, t->slowdown = 0;
    }
}

tl_timer *tl_create_timer(void)
{
    tl_timer *timer;
    timer = (tl_timer *)calloc(1, sizeof(tl_timer));
    if (timer == NULL)
        return NULL;
    timer->interval = -1;
    timer->handler = NULL;
    timer->multihandler = NULL;
    timer->userdata = NULL;
    timer->next = NULL;
    timer->previous = NULL;
    timer->group = NULL;
    timer->stopped = 0;
    timer->stoppedtime = 0;
    timer->slowdown = 0;
    timer->emulator = NULL;
    tl_reset_timer(timer);
    return (timer);
}

tl_group *tl_create_group(void)
{
    tl_group *timer;
    timer = (tl_group *)calloc(1, sizeof(tl_group));
    if (timer == NULL)
        return NULL;
    timer->interval = -1;
    timer->handler = NULL;
    timer->multihandler = NULL;
    timer->userdata = NULL;
    timer->next = NULL;
    timer->previous = NULL;
    timer->group = timer;
    tl_reset_timer(timer);
    return (timer);
}

void tl_free_timer(tl_timer *timer)
{
    if (timer->group)
        tl_remove_timer(timer);
    free((void *)timer);
}

void tl_free_group(tl_group *timer)
{
    tl_timer *next;
    do {
        next = timer->next;
        free((void *)timer);
    } while (next != NULL);
}

int tl_process_group(tl_group *group, int *activated)
{
    int again = 1;
    tl_timer *timer, *timer1;
    int minwait = INT_MAX;
    tl_update_time();
    if (activated != NULL)
        *activated = 0;
    while (again) {
        group->slowdown = 0;
        again = 0;
        minwait = INT_MAX;
        timer = group->next;
        while (timer != NULL) {
            timer1 = timer->next;
            if (timer->handler && timer->interval >= 0) {
                int time = timer->interval - tl_lookup_timer(timer);
                if (time < 500) {
                    if (activated != NULL)
                        (*activated)++;
                    again = 1;
                    tl_reset_timer(timer);
                    if (time < -200 * 1000000)
                        time = 0; /*underflow? */
                    tl_slowdown_timer(timer, time);
                    time = timer->interval + time;
                    timer->handler(timer->userdata);
                    tl_update_time();
                }
                if (time < minwait)
                    minwait = time;
            } else if (timer->multihandler && timer->interval > 0) {
                int time = timer->interval - tl_lookup_timer(timer);
                if (time < 500) {
                    int n;
                    if (activated != NULL)
                        (*activated)++;
                    tl_reset_timer(timer);
                    if (time < -200 * 1000000)
                        time = 0; /*underflow? */
                    n = -(time + 500) / timer->interval + 1;
                    time = timer->interval * n + time;
                    tl_slowdown_timer(timer, time - timer->interval +
                                                 n * timer->interval);
                    timer->multihandler(timer->userdata, n);
                    tl_update_time();
                }
                if (time < minwait)
                    minwait = time;
            }
            if (group->slowdown) {
                again = 1;
                break;
            }
            timer = timer1;
        }
    }
    if (minwait != INT_MAX) {
        if (minwait < 0)
            return (0);
        return (minwait);
    }
    return (-1);
}

void tl_add_timer(tl_group *group, tl_timer *timer)
{
    if (timer->group)
        tl_remove_timer(timer);
    timer->previous = group;
    timer->next = group->next;
    timer->group = group;
    group->next = timer;
    if (timer->next != NULL)
        timer->next->previous = timer;
}

void tl_set_interval(tl_timer *timer, int interval)
{
    if (timer->interval <= 0) {
        tl_reset_timer(timer);
    }
    timer->interval = interval;
}

void tl_set_handler(tl_timer *timer, void (*handler)(void *), void *ud)
{
    timer->handler = handler;
    timer->userdata = ud;
}

void tl_set_multihandler(tl_timer *timer, void (*handler)(void *, int),
                         void *ud)
{
    timer->multihandler = handler;
    timer->userdata = ud;
}

void tl_remove_timer(tl_timer *timer)
{
    timer->group->slowdown = 1;
    timer->previous->next = timer->next;
    if (timer->next != NULL)
        timer->next->previous = timer->previous;
    timer->group = NULL;
}

struct timeemulator *tl_create_emulator(void)
{
    return ((struct timeemulator *)calloc(1, sizeof(struct timeemulator)));
}

void tl_free_emulator(struct timeemulator *t) { free(t); }

void tl_elpased(struct timeemulator *t, int elpased)
{
    t->exact += elpased;
    t->time += t->exact / EMULDIV;
    t->exact &= (EMULDIV - 1);
}

void tl_emulate_timer(struct timer *t, struct timeemulator *e)
{
    int time = tl_lookup_timer(t);
    t->emulator = e;
    t->lastemulated = e->time;
    tl_slowdown_timer(t, -time);
}

void tl_unemulate_timer(struct timer *t)
{
    int time = tl_lookup_timer(t);
    t->emulator = NULL;
    tl_slowdown_timer(t, tl_lookup_timer(t) - time);
}
