/*
 *	be_thread.c		BeOS threading, Jens Kilian (jjk@acm.org)
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

#include <stdio.h>
#include "xthread.h"
#ifdef __BEOS__
#include <OS.h>


#define THREAD_DEBUG 0

struct taskinfo definfo = {
  0, B_BAD_THREAD_ID
};

#ifndef nthreads

#define getrange1(range,ncpu) ((range)*(ncpu))/nthreads
#define getrange2(range,ncpu) getrange1(range,(ncpu)+1)

int ethreads = 0;
int nthreads = 1;

static struct taskinfo infos[MAXTHREADS];
benaphore mutexes[MAXSEMAPHORS];
benaphore condvars[MAXCONDS];

static sem_id start_work, started_work, work_finished;
static volatile xfunction gFunction;
static volatile void *gpArg;
static volatile int gRange;

static char *
format_name (const char *fmt, int n)
{
  static char name[B_OS_NAME_LENGTH];
  sprintf (name, fmt, n);
  return name;
}

static int32
control_routine (void *pData)
{
  struct taskinfo *pInfo = (struct taskinfo *) pData;

  /*
   *      All the worker threads start off running this function.
   *              They synchronize with the controlling thread using a few
   *              dedicated semaphores.
   *              Information is passed via global variables (ouch!)
   */

  for (;;)
    {
      xfunction function;
      volatile void *pArg;
      int range;

      /* We're raring to go. */
      release_sem (work_finished);

      /* Wait for the starting signal. */
      while (acquire_sem (start_work) == B_INTERRUPTED)
	;

      /* Find out what to do. */
      function = gFunction;
      pArg = gpArg;
      range = gRange;

      /* "Sure thing, boss." */
#if THREAD_DEBUG
      fprintf (stderr, "Worker %d starting on range %d-%d.\n",
	       pInfo->n,
	       getrange1 (range, pInfo->n), getrange2 (range, pInfo->n));
#endif
      release_sem (started_work);
      function ((void *) pArg, pInfo,
		getrange1 (range, pInfo->n), getrange2 (range, pInfo->n));
    }

  return 0;
}

void
be_thread_init (int num_threads)
{
  int i;

#if THREAD_DEBUG
  fprintf (stderr, "Calling be_thread_init(%d).\n", num_threads);
#endif
  if (num_threads < 1)
    {
      /* Select a reasonable number of threads. */
      system_info info;
      if (get_system_info (&info) == B_NO_ERROR)
	{
	  if (info.cpu_count > 1)
	    num_threads = (int) info.cpu_count + 1;
	  else
	    num_threads = 1;
	}
      else
	{
#if THREAD_DEBUG
	  fprintf (stderr, "Could not get system info!?\n");
#endif
	  num_threads = 1;
	}
    }

  if (num_threads == 1)
    {
#if THREAD_DEBUG
      fprintf (stderr, "Threading disabled.\n");
#endif
      return;
    }
  else if (num_threads > MAXTHREADS)
    {
      num_threads = MAXTHREADS;
    }

  nthreads = num_threads;
#if THREAD_DEBUG
  fprintf (stderr, "Proposed number of threads: %d.\n", nthreads);
#endif

  /* Initialize semaphores. */
  start_work = create_sem (0, "XaoS worker start");
  started_work = create_sem (0, "XaoS worker started");
  work_finished = create_sem (0, "XaoS worker finished");

  for (i = 0; i < MAXSEMAPHORS; ++i)
    {
      mutexes[i].cnt = 0;
      mutexes[i].sem = create_sem (0, format_name ("XaoS mutex %d", i));
    }
  for (i = 0; i < MAXCONDS; ++i)
    {
      condvars[i].cnt = 0;
      condvars[i].sem = create_sem (0, format_name ("XaoS condition %d", i));
    }

  /* Start up threads. */
  infos[0].n = 0;
  infos[0].id = find_thread (0);
  for (i = 1; i < nthreads; ++i)
    {
      infos[i].n = i;
      infos[i].id = spawn_thread (control_routine,
				  format_name ("XaoS worker %d", i),
				  B_NORMAL_PRIORITY, infos + i);
      if (infos[i].id < B_NO_ERROR
	  || resume_thread (infos[i].id) < B_NO_ERROR)
	{
	  nthreads = 1;
	  break;
	}
    }
#if THREAD_DEBUG
  fprintf (stderr, "Actual number of threads: %d.\n", nthreads);
#endif

  if (nthreads > 1)
    {
      ethreads = 1;
    }
}

void
be_thread_uninit (void)
{
  int i;

  /* Clean up. */
  for (i = 1; i < nthreads; ++i)
    {
      kill_thread (infos[i].id);
      infos[i].id = B_BAD_THREAD_ID;
    }

  delete_sem (start_work);
  delete_sem (started_work);
  delete_sem (work_finished);

  for (i = 0; i < MAXSEMAPHORS; ++i)
    {
      delete_sem (mutexes[i].sem);
      mutexes[i].sem = B_BAD_SEM_ID;
    }
  for (i = 0; i < MAXCONDS; ++i)
    {
      delete_sem (condvars[i].sem);
      condvars[i].sem = B_BAD_SEM_ID;
    }

  nthreads = 1;
  ethreads = 0;
}

void
be_thread_function (xfunction function, void *pArg, int range)
{
#if THREAD_DEBUG
  fprintf (stderr, "be_thread_function(%08lx, %08lx, %d)\n",
	   (long) function, (long) pArg, range);
#endif

  /* Wait until all workers are ready. */
  while (acquire_sem_etc (work_finished, nthreads - 1, 0, 0) == B_INTERRUPTED)
    ;

  /* Tell them to start on the new function. */
  gFunction = function;
  gpArg = pArg;
  gRange = range;
  release_sem_etc (start_work, nthreads - 1, 0);
  /* Wait until all have acquired the data. */
  while (acquire_sem_etc (started_work, nthreads - 1, 0, 0) == B_INTERRUPTED)
    ;

  /* Do our own thing. */
  function (pArg, infos, getrange1 (range, 0), getrange2 (range, 0));

#if THREAD_DEBUG
  fprintf (stderr, "be_thread_function() finished.\n");
#endif
}

void
be_thread_synchronize (void)
{
#if THREAD_DEBUG
  fprintf (stderr, "be_thread_synchronize()\n");
#endif

  /* Wait until all worker threads have finished. */
  while (acquire_sem_etc (work_finished, nthreads - 1, 0, 0) == B_INTERRUPTED)
    ;

  /* Restore the semaphore's state. */
  release_sem_etc (work_finished, nthreads - 1, 0);

#if THREAD_DEBUG
  fprintf (stderr, "be_thread_synchronize() finished.\n");
#endif
}

void
be_thread_bgjob (xfunction function, void *pArg)
{
#if THREAD_DEBUG
  fprintf (stderr, "be_thread_bgjob(%08lx, %08lx)\n",
	   (long) function, (long) pArg);
#endif

  if (acquire_sem_etc (work_finished, 1, B_TIMEOUT, 0) >= B_NO_ERROR)
    {
      /* A worker thread is available. */
#if THREAD_DEBUG
      fprintf (stderr, "be_thread_bgjob(): running in background.\n");
#endif
      gFunction = function;
      gpArg = pArg;
      gRange = 0;
      release_sem (start_work);
      /* Wait until the worker has acquired the data. */
      while (acquire_sem (started_work) == B_INTERRUPTED)
	;

    }
  else
    {
      /* No worker is available. */
#if THREAD_DEBUG
      fprintf (stderr, "be_thread_bgjob(): running in foreground.\n");
#endif
      function (pArg, infos, 0, 0);
    }
}

/*
 *		The following emulation of pthread condition variables
 *		was inspired by Eric Berdahl's Betelgeuse library, but
 *		has been somewhat simplified.
 */

void
be_thread_sleep (benaphore * pCondition, benaphore * pMutex)
{
  /*
   *              Update the number of threads waiting on the condition.
   *              The condition is exclusively held at this time.
   */
  ++(pCondition->cnt);

  /* Unlock the mutex, then block on the condition. */
  release_benaphore (pMutex);
  while (acquire_sem (pCondition->sem) == B_INTERRUPTED)
    ;

  /* We've been signaled - relock the mutex. */
  acquire_benaphore (pMutex);
}

void
be_thread_wakeup (benaphore * pCondition)
{
  /*
   *              Wake all waiting threads, if any.
   *              The condition is exclusively held at this time.
   */
  int32 nWaiting = pCondition->cnt;
  if (nWaiting > 0)
    {
      pCondition->cnt = 0;
      release_sem_etc (pCondition->sem, nWaiting, 0);
    }
}

void
be_thread_wakefirst (benaphore * pCondition)
{
  /*
   *              Wake one waiting thread, if any.
   *              The condition is exclusively held at this time.
   */
  if (pCondition->cnt > 0)
    {
      --(pCondition->cnt);
      release_sem (pCondition->sem);
    }
}

#endif /*nthreads */
#endif /*__BEOS__ */
