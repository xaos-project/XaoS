#ifndef _plan9_
#include <signal.h>
#include <stdio.h>
#endif
#include <xthread.h>
#ifndef __BEOS__
struct taskinfo definfo = {
  0,
};
#ifndef nthreads

#define getrange1(range,ncpu) ((range)*(ncpu))/nthreads
#define getrange2(range,ncpu) getrange1(range,(ncpu)+1)

int ethreads = 0;
int nthreads = 1;

#ifdef USE_PTHREAD

/* Well conde follows is probably very ugly, since this is
 * my absolutely first application for threads. Please let
 * me know how to improvie it
 */

static pthread_cond_t synccond, startcond;
static struct taskinfo infos[MAXTHREADS];
static pthread_mutex_t synccondmutex, startcondmutex;
pthread_mutex_t semaphors[MAXSEMAPHORS];
pthread_cond_t conds[MAXCONDS];

/*This loop is executed whole time in slave threads.
   Its function is following:
   1) wait for message
   2) call function from message
   3) syncronize
   4) again

   To invoke this mechanizm main thread(#1) should call
   xth_function
   xth_synchronize forces forces thread to wait for others
 */

static int nfinished;
static int npending;
static int counter;
static int bcounter;
static int bcounter1;
static int range;
static void *data;
static xfunction function;
static void *
control_routine (void *i)
{
  struct taskinfo *info = i;
  int mycounter = 0;
  int r;
  void *d;
  xfunction f;
  while (1)
    {
      /* quite a lot pthread calls. Please if you are
       * reading this code and crying "OH NO!" so ugly
       * handling! Why so much calls? Stop crying, I am
       * newbie in threads. Please rewrite my code and
       * send me better and faster version. 
       * 
       * This function comunicates with pth_function from main loop
       * as follows: it uses startcond to wait for order start function!
       * Counter is used to ensure that main function did not give
       * order whie control_routine was busy
       *
       * after order is received, function adress is readed from global
       * variables and started. Pth_function then executes its
       * own part of calculation. After that it waits counter
       * nfinished to reach number of thasks-1. This is done
       * using cond synccond and synccondmutex. Quite complex
       * but it seems to work. Looking forward for someone, who
       * should reduce number of _lock/_unlock
       */
      pthread_mutex_lock (&synccondmutex);
      nfinished++;
      pthread_cond_signal (&synccond);
      pthread_mutex_unlock (&synccondmutex);

      pthread_mutex_lock (&startcondmutex);
      while (mycounter >= counter)
	{
	  if (bcounter > bcounter1)
	    {
	      /*Well we are already locked using start lock..should be OK */
	      mycounter--;
	      bcounter1++;
	      break;
	    }
	  pthread_cond_wait (&startcond, &startcondmutex);
	}
      r = range;
      d = data;
      f = function;
      npending--;
      pthread_mutex_unlock (&startcondmutex);
      mycounter++;
      f (d, info, getrange1 (r, info->n), getrange2 (r, info->n));
    }
  return NULL;
}

void
pth_init (int nthreads1)
{
  int i;
  pthread_attr_t attr;
  if (ethreads)
    return;

  if (nthreads1 == 1 || nthreads1 == 0)
    return;			/*use nothreads_* calls */
  if (nthreads1 > MAXTHREADS)
    nthreads1 = MAXTHREADS;
  nthreads = nthreads1;

  pthread_attr_init (&attr);
  pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);

  if (pthread_cond_init (&synccond, NULL))
    exit (1);
  if (pthread_cond_init (&startcond, NULL))
    exit (1);
  if (pthread_mutex_init (&synccondmutex, NULL))
    exit (1);
  if (pthread_mutex_init (&startcondmutex, NULL))
    exit (1);

  infos[0].n = 0;
  /*infos[0].id = pthread_self(); */

  for (i = 0; i < MAXSEMAPHORS; i++)
    {
      pthread_mutex_init (semaphors + i, NULL);
    }
  for (i = 0; i < MAXCONDS; i++)
    {
      pthread_cond_init (conds + i, NULL);
    }

  nfinished = 0;
  for (i = 0; i < nthreads - 1; i++)
    {
      if (pthread_create
	  (&infos[i + 1].id, &attr, control_routine, infos + i + 1))
	{
	  nthreads = i + 1;
	  break;
	}
      infos[i + 1].n = i + 1;
    }
  if (nthreads != 1)
    ethreads = 1;
}

void
pth_synchronize ()
{
  /*Our job is done, synchronize now */
  if (nfinished < nthreads - 1)
    {
      pthread_mutex_lock (&synccondmutex);
      while (nfinished < nthreads - 1)
	{
	  pthread_cond_wait (&synccond, &synccondmutex);
	}
      pthread_mutex_unlock (&synccondmutex);
    }
  /*Ok job is done, lets continue :) */
}
void
pth_bgjob (xfunction f, void *d)
{
  pthread_mutex_lock (&startcondmutex);
  if (npending)
    {
      printf ("Collision!\n");	/*FIXME:remove this..I just want to know how often this happends */
      pthread_mutex_unlock (&startcondmutex);
      f (d, infos, 0, 0);
    }
  if (bcounter < bcounter1)
    {
      printf ("Internal error\a\n");
    }
  if (!nfinished)
    {
      pthread_mutex_unlock (&startcondmutex);
      /*no more CPU available :( */
      f (d, infos, 0, 0);
    }
  data = d;
  range = 0;
  function = f;
  bcounter++;
  nfinished--;
  npending++;
  pthread_cond_signal (&startcond);
  pthread_mutex_unlock (&startcondmutex);
}

void
pth_function (xfunction f, void *d, int r)
{
  pth_synchronize ();
  pthread_mutex_lock (&startcondmutex);
  data = d;
  range = r;
  function = f;
  /*And lets start it:) */
  nfinished = 0;
  npending = nthreads - 1;
  counter++;
  if (nthreads == 2)
    pthread_cond_signal (&startcond);
  else
    pthread_cond_broadcast (&startcond);
  pthread_mutex_unlock (&startcondmutex);

  function (data, infos, getrange1 (range, 0), getrange2 (range, 0));
}

void
pth_uninit ()
{
  /*Should be empty for now since all threads will be killed after exit call */
  /*FIXME should be added something if necessary :) */
  nthreads = 1;
  ethreads = 0;
}
#endif /*POSIX threads */
#endif /*nthreads */
#endif /*__BEOS__*/
