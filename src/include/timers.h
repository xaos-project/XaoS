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

#ifndef TIMERS_H
#define TIMERS_H
#include "config.h"
#ifdef __cplusplus
extern "C" {
#endif

struct timer;
struct timeemulator;
typedef struct timer tl_timer;
typedef struct timer tl_group;

void tl_update_time (void);
tl_timer *tl_create_timer (void);
tl_group *tl_create_group (void);
void tl_set_interval (tl_timer * timer, int interval);
void tl_set_handler (tl_timer * timer, void (*handler) (void *), void *userdata);
void tl_set_multihandler (tl_timer * timer, void (*handler) (void *, int), void *userdata);
void tl_add_timer (tl_group * group, tl_timer * timer);
void tl_remove_timer (tl_timer * timer);
void tl_free_timer (tl_timer * timer);
void tl_free_group (tl_group * timer);
void tl_stop_timer (tl_timer * t);
void tl_resume_timer (tl_timer * t);
void tl_slowdown_timer (tl_timer * t, int time);
int tl_process_group (tl_group * group, int *activated);
extern tl_group *syncgroup, *asyncgroup;
void tl_sleep (int);
void tl_allegromode (int mode);	/*Just for djgpp */
int tl_lookup_timer (tl_timer * t) REGISTERS (3);
void tl_reset_timer (tl_timer * t) REGISTERS (3);

struct timeemulator *tl_create_emulator(void);
void tl_free_emulator(struct timeemulator *t);
void tl_elpased(struct timeemulator *t, int elpased);
void tl_emulate_timer(struct timer *t, struct timeemulator *e);
void tl_unemulate_timer(struct timer *t);

#ifdef __cplusplus
}
#endif
#endif /* TIMER_H */
