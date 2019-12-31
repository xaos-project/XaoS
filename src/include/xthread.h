/*
 * An XaoS thread API implementation
 * Many functions are implemented as macros that maps simple
 * XaoS thread API into architecture depended API.
 * I tried avoid functions because of speed issues
 * So this header will be quite messy soon :)
 *
 * Supported API for now:
 * pthread (POSIX) enabled using USE_PTHREAD
 * nothread (my own) enabled by default
 */
#ifndef THREAD_H
#define THREAD_H 1
#include <fconfig.h>
#ifdef __cplusplus
extern "C" {
#endif
#define MAXTHREADS 32
/*You might increase this constant if needed
   (xaos on connection machine? :) */

#define NSEMAPHORS 2
#define MAXSEMAPHORS 2
#define NSLEEPS 2
#define MAXCONDS 2

#ifdef USE_PTHREAD
#include <pthread.h>
#endif

struct taskinfo {
    int n;
#ifdef USE_PTHREAD
    pthread_t id;
#endif
};

extern struct taskinfo definfo;
extern int ethreads; /*Is threading enabled? */
extern int nthreads; /*Number of threads */
typedef void (*xfunction)(void *, struct taskinfo *, int, int);

/*No-thread API implementation version */
#define nothread                                                               \
    {                                                                          \
    }
#define nothread_init(nthreads) nothread
#define nothread_uninit() nothread
#define nothread_function(f, data, range) f(data, &definfo, 0, range)
#define nothread_bgjob(f, d) f(d, &definfo, 0, 0)
#define nothread_lock(n) nothread
#define nothread_unlock(n) nothread
#define nothread_sync() nothread
#define nothread_sleep(n, l) nothread
#define nothread_wakeup(n) nothread
#define nothread_wakefirst(n) nothread

#define xth_wrap(f1, f2)                                                       \
    if (nthreads != 1)                                                         \
        f1();                                                                  \
    else                                                                       \
        f2();

#ifdef USE_PTHREAD
/* A posix thread API maps */
void pth_init(int nthreads);
void pth_uninit(void);
void pth_function(xfunction f, void *d, int r);
void pth_synchronize(void);
void pth_bgjob(xfunction f, void *d);
extern pthread_mutex_t semaphors[MAXSEMAPHORS];
extern pthread_cond_t conds[MAXCONDS];

/*Map pthread API to XaoS thread API */

#define xth_init(nthreads) pth_init(nthreads)
#define xth_uninit() pth_uninit()
#define xth_lock(n)                                                            \
    if (ethreads)                                                              \
    pthread_mutex_lock(semaphors + (n))
#define xth_unlock(n)                                                          \
    if (ethreads)                                                              \
    pthread_mutex_unlock(semaphors + (n))
#define xth_function(f, d, r)                                                  \
    if (ethreads)                                                              \
        pth_function(f, d, r);                                                 \
    else                                                                       \
        nothread_function(f, d, r)
#define xth_nthread(ts) (ethreads ? ts->n : 0)
#define xth_sync()                                                             \
    if (ethreads)                                                              \
        pth_synchronize();
#define xth_bgjob(f, d)                                                        \
    if (ethreads)                                                              \
        pth_bgjob(f, d);                                                       \
    else                                                                       \
        f(d, &definfo, 0, 0);
#define xth_sleep(n, l)                                                        \
    if (ethreads)                                                              \
    pthread_cond_wait(conds + (n), semaphors + (l))
#define xth_wakeup(n)                                                          \
    if (ethreads)                                                              \
    pthread_cond_broadcast(conds + (n))
#define xth_wakefirst(n)                                                       \
    if (ethreads)                                                              \
    pthread_cond_signal(conds + (n))
#define API_MAPPED
#endif /*USE_PTHREAD */

#ifndef API_MAPPED
/*
 * No thread support is compiled - do just wrappers
 * to nothread implementation
 */
#define nthreads 1
#define ethreads 0
#define xth_init(n) nothread_init(n)
#define xth_uninit() nothread_uninit()
#define xth_function(f, d, r) nothread_function(f, d, r)
#define xth_lock(n) nothread_lock(n)
#define xth_unlock(n) nothread_unlock(n)
#define xth_sync() nothread_sync()
#define xth_bgjob(f, d) nothread_bgjob(f, d)
#define xth_nthread(ts) 0
#define xth_wakeup(n) nothreads_wakeup(n)
#define xth_wakefirst(n) nothreads_wakefirst(n)
#define xth_sleep(n, l) nothreads_sleep(n, l)
#endif

#ifdef __cplusplus
}
#endif
#endif
