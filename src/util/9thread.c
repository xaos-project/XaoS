#include <u.h>
#include <libc.h>
#include <stdio.h>
#include <xthread.h>
struct taskinfo definfo =
{
  0,
};
#ifndef nthreads

#define getrange1(range,ncpu) ((range)*(ncpu))/nthreads
#define getrange2(range,ncpu) getrange1(range,(ncpu)+1)

int ethreads = 0;
int nthreads = 1;

#ifdef _plan9_

/* Well code follows is probably very ugly, since this is
 * my absolutely first application for threads. Please let
 * me know how to improvie it
 */
void
p9wait (struct Stack *stack, Lock * l)
{
  static volatile int ctag = 0;
  int tag;
  ctag++;
  tag = ctag;
  stack->tags[stack->nwaiting++] = ctag;
  unlock (l);
  rendezvous (ctag, 0);
  lock (l);
}
void
p9wakeup (struct Stack *stack)
{
  int tag;
  /*Queue is locked so we don't need to wory about colisions */
  if (stack->nwaiting)
    {
      tag = stack->tags[--stack->nwaiting];
      rendezvous (tag, 0);
    }
}
void
p9wakeall (struct Stack *stack)
{
  int tag;
  while (stack->nwaiting)
    {
      tag = stack->tags[--stack->nwaiting];
      rendezvous (tag, 0);
    }
}

static struct Stack synccond, startcond;
static struct taskinfo infos[MAXTHREADS];
static Lock synccondmutex, startcondmutex;
static int exitnow = 0;
Lock semaphors[MAXSEMAPHORS];
struct Stack conds[MAXCONDS];

static int nfinished;
static int npending;
static int counter;
static int bcounter;
static int bcounter1;
static int range;
static void *data;
static xfunction function;
static int
control_routine (void *i)
{
  struct taskinfo *info = i;
  int mycounter = 0;
  int r;
  void *d;
  xfunction f;
  unsigned long fcr = 0;	/*getfcr(); */
  fcr |= FPRNR | FPPEXT;
  fcr &= ~(FPINEX | FPOVFL | FPUNFL | FPZDIV);
  setfcr (fcr);
  while (1)
    {
      lock (&synccondmutex);
      nfinished++;
      p9wakeup (&synccond);
      unlock (&synccondmutex);

      lock (&startcondmutex);
      while (mycounter >= counter)
	{
	  if (bcounter > bcounter1)
	    {
	      /*Well we are already locked using start lock..should be OK */
	      mycounter--;
	      bcounter1++;
	      break;
	    }
	  if (exitnow)
	    {
	      unlock (&startcondmutex);
	      exit (1);
	    }
	  p9wait (&startcond, &startcondmutex);
	}
      r = range;
      d = data;
      f = function;
      npending--;
      unlock (&startcondmutex);
      mycounter++;
      f (d, info, getrange1 (r, info->n), getrange2 (r, info->n));
    }
  return 0;
}
void
p9init (int nthreads1)
{
  int i;
  if (ethreads)
    return;

  if (nthreads1 == 1 || nthreads1 == 0)
    return;			/*use nothreads_* calls */
  if (nthreads1 > MAXTHREADS)
    nthreads1 = MAXTHREADS;
  nthreads = nthreads1;
  lockinit ();


  infos[0].n = 0;

  rfork (RFNAMEG);
  nfinished = 0;
  for (i = 0; i < nthreads - 1; i++)
    {
      infos[i + 1].n = i + 1;
      if (!(infos[i + 1].id = rfork (RFPROC | RFNOWAIT | RFMEM)))
	{
	  control_routine (&infos[i + 1]);
	}
    }
  if (nthreads != 1)
    ethreads = 1;
}
void
p9synchronize (void)
{
  /*Our job is done, synchronize now */
  if (nfinished < nthreads - 1)
    {
      lock (&synccondmutex);
      while (nfinished < nthreads - 1)
	{
	  p9wait (&synccond, &synccondmutex);
	}
      unlock (&synccondmutex);
    }
  /*Ok job is done, lets continue :) */
}
void
p9bgjob (xfunction f, void *d)
{
  lock (&startcondmutex);
  if (npending)
    {
      unlock (&startcondmutex);
      f (d, infos, 0, 0);
    }
  if (bcounter < bcounter1)
    {
      printf ("Internal error\a\n");
    }
  if (!nfinished)
    {
      unlock (&startcondmutex);
      /*no more CPU available :( */
      f (d, infos, 0, 0);
    }
  data = d;
  range = 0;
  function = f;
  bcounter++;
  nfinished--;
  npending++;
  p9wakeup (&startcond);
  unlock (&startcondmutex);
}
void
p9function (xfunction f, void *d, int r)
{
  p9synchronize ();
  lock (&startcondmutex);
  data = d;
  range = r;
  function = f;
  /*And lets start it:) */
  nfinished = 0;
  npending = nthreads - 1;
  counter++;
  p9wakeall (&startcond);
  unlock (&startcondmutex);

  function (data, infos, getrange1 (range, 0), getrange2 (range, 0));
}
void
p9uninit (void)
{
  p9synchronize ();
  lock (&startcondmutex);
  exitnow = 1;
  p9wakeall (&startcond);
  unlock (&startcondmutex);
  nthreads = 1;
  ethreads = 0;
}
#endif /*POSIX threads */
#endif /*nthreads */
