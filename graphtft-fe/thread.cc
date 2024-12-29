/*
 * thread.c: A simple thread base class
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 */

#include <errno.h>
#include <linux/unistd.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <string.h>

#include "thread.h"
#include "../common.h"

static bool GetAbsTime(struct timespec *Abstime, int MillisecondsFromNow)
{
  struct timeval now;
  if (gettimeofday(&now, NULL) == 0) {           // get current time
     now.tv_usec += MillisecondsFromNow * 1000;  // add the timeout
     while (now.tv_usec >= 1000000) {            // take care of an overflow
           now.tv_sec++;
           now.tv_usec -= 1000000;
           }
     Abstime->tv_sec = now.tv_sec;          // seconds
     Abstime->tv_nsec = now.tv_usec * 1000; // nano seconds
     return true;
     }
  return false;
}

// --- cCondWait -------------------------------------------------------------

cCondWait::cCondWait(void)
{
  signaled = false;
  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&cond, NULL);
}

cCondWait::~cCondWait()
{
  pthread_cond_broadcast(&cond); // wake up any sleepers
  pthread_cond_destroy(&cond);
  pthread_mutex_destroy(&mutex);
}

void cCondWait::SleepMs(int TimeoutMs)
{
  cCondWait w;
  w.Wait(std::max(TimeoutMs, 3)); // making sure the time is >2ms to avoid a possible busy wait
}

bool cCondWait::Wait(int TimeoutMs)
{
  pthread_mutex_lock(&mutex);
  if (!signaled) {
     if (TimeoutMs) {
        struct timespec abstime;
        if (GetAbsTime(&abstime, TimeoutMs)) {
           while (!signaled) {
                 if (pthread_cond_timedwait(&cond, &mutex, &abstime) == ETIMEDOUT)
                    break;
                 }
           }
        }
     else
        pthread_cond_wait(&cond, &mutex);
     }
  bool r = signaled;
  signaled = false;
  pthread_mutex_unlock(&mutex);
  return r;
}

void cCondWait::Signal(void)
{
  pthread_mutex_lock(&mutex);
  signaled = true;
  pthread_cond_broadcast(&cond);
  pthread_mutex_unlock(&mutex);
}

// --- cCondVar --------------------------------------------------------------

cCondVar::cCondVar(void)
{
  pthread_cond_init(&cond, 0);
}

cCondVar::~cCondVar()
{
  pthread_cond_broadcast(&cond); // wake up any sleepers
  pthread_cond_destroy(&cond);
}

void cCondVar::Wait(cMutex &Mutex)
{
  if (Mutex.locked) {
     int locked = Mutex.locked;
     Mutex.locked = 0; // have to clear the locked count here, as pthread_cond_wait
                       // does an implicit unlock of the mutex
     pthread_cond_wait(&cond, &Mutex.mutex);
     Mutex.locked = locked;
     }
}

bool cCondVar::TimedWait(cMutex &Mutex, int TimeoutMs)
{
  bool r = true; // true = condition signaled, false = timeout

  if (Mutex.locked) {
     struct timespec abstime;
     if (GetAbsTime(&abstime, TimeoutMs)) {
        int locked = Mutex.locked;
        Mutex.locked = 0; // have to clear the locked count here, as pthread_cond_timedwait
                          // does an implicit unlock of the mutex.
        if (pthread_cond_timedwait(&cond, &Mutex.mutex, &abstime) == ETIMEDOUT)
           r = false;
        Mutex.locked = locked;
        }
     }
  return r;
}

void cCondVar::Broadcast(void)
{
  pthread_cond_broadcast(&cond);
}

// --- cRwLock ---------------------------------------------------------------

cRwLock::cRwLock(bool PreferWriter)
{
  pthread_rwlockattr_t attr;
  pthread_rwlockattr_init(&attr);
  pthread_rwlockattr_setkind_np(&attr, PreferWriter ? PTHREAD_RWLOCK_PREFER_WRITER_NP : PTHREAD_RWLOCK_PREFER_READER_NP);
  pthread_rwlock_init(&rwlock, &attr);
}

cRwLock::~cRwLock()
{
  pthread_rwlock_destroy(&rwlock);
}

bool cRwLock::Lock(bool Write, int TimeoutMs)
{
  int Result = 0;
  struct timespec abstime;
  if (TimeoutMs) {
     if (!GetAbsTime(&abstime, TimeoutMs))
        TimeoutMs = 0;
     }
  if (Write)
     Result = TimeoutMs ? pthread_rwlock_timedwrlock(&rwlock, &abstime) : pthread_rwlock_wrlock(&rwlock);
  else
     Result = TimeoutMs ? pthread_rwlock_timedrdlock(&rwlock, &abstime) : pthread_rwlock_rdlock(&rwlock);
  return Result == 0;
}

void cRwLock::Unlock(void)
{
  pthread_rwlock_unlock(&rwlock);
}

// --- cMutex ----------------------------------------------------------------

cMutex::cMutex(void)
{
  locked = 0;
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK_NP);
  pthread_mutex_init(&mutex, &attr);
}

cMutex::~cMutex()
{
  pthread_mutex_destroy(&mutex);
}

void cMutex::Lock(void)
{
  pthread_mutex_lock(&mutex);
  locked++;
}

void cMutex::Unlock(void)
{
 if (!--locked)
    pthread_mutex_unlock(&mutex);
}

// --- cMyThread ---------------------------------------------------------------

tThreadId cMyThread::mainThreadId = 0;

cMyThread::cMyThread(const char *Description)
{
  active = running = false;
  childTid = 0;
  childThreadId = 0;
  description = NULL;
  if (Description)
     SetDescription(Description);
}

cMyThread::~cMyThread()
{
  Cancel(); // just in case the derived class didn't call it
  free(description);
}

void cMyThread::SetDescription(const char *Description)
{
  free(description);
  description = NULL;
  description = strdup(Description);
}

void *cMyThread::StartThread(cMyThread *Thread)
{
  Thread->childThreadId = ThreadId();
  if (Thread->description) {
     printf("%s thread started (pid=%d, tid=%d)\n", Thread->description, getpid(), Thread->childThreadId);
#ifdef PR_SET_NAME
     if (prctl(PR_SET_NAME, Thread->description, 0, 0, 0) < 0)
        printf("%s thread naming failed (pid=%d, tid=%d)\n", Thread->description, getpid(), Thread->childThreadId);
#endif
     }
  Thread->Action();
  if (Thread->description)
     printf("%s thread ended (pid=%d, tid=%d)\n", Thread->description, getpid(), Thread->childThreadId);
  Thread->running = false;
  Thread->active = false;
  return NULL;
}

#define THREAD_STOP_TIMEOUT  3000 // ms to wait for a thread to stop before newly starting it
#define THREAD_STOP_SLEEP      30 // ms to sleep while waiting for a thread to stop

bool cMyThread::Start(void)
{
  if (!running) 
  {
     if (active) 
     {
        return true;
     }
     if (!active) 
     {
        active = running = true;
        if (pthread_create(&childTid, NULL, (void *(*) (void *))&StartThread, (void *)this) == 0) {
           pthread_detach(childTid); // auto-reap
           }
        else {
           printf("Error: ...\n");
           active = running = false;
           return false;
        }
     }
  }
  return true;
}

bool cMyThread::Active(void)
{
  if (active) {
     //
     // Single UNIX Spec v2 says:
     //
     // The pthread_kill() function is used to request
     // that a signal be delivered to the specified thread.
     //
     // As in kill(), if sig is zero, error checking is
     // performed but no signal is actually sent.
     //
     int err;
     if ((err = pthread_kill(childTid, 0)) != 0) {
        if (err != ESRCH)
           printf("Error: ...\n");
        childTid = 0;
        active = running = false;
        }
     else
        return true;
     }
  return false;
}

void cMyThread::Cancel(int WaitSeconds)
{
  running = false;
  if (active && WaitSeconds > -1) {
     if (WaitSeconds > 0) {
        for (time_t t0 = time(NULL) + WaitSeconds; time(NULL) < t0; ) {
            if (!Active())
               return;
            cCondWait::SleepMs(10);
            }
        printf("ERROR: %s thread %d won't end (waited %d seconds) - canceling it...\n", 
               description ? description : "", childThreadId, WaitSeconds);
        }
     pthread_cancel(childTid);
     childTid = 0;
     active = false;
     }
}

tThreadId cMyThread::ThreadId(void)
{
  return syscall(__NR_gettid);
}

void cMyThread::SetMainThreadId(void)
{
  if (mainThreadId == 0)
     mainThreadId = ThreadId();
  else
     printf("ERROR: attempt to set main thread id to %d while it already is %d\n", 
            ThreadId(), mainThreadId);
}

// --- cMutexLock ------------------------------------------------------------

cMutexLock::cMutexLock(cMutex *Mutex)
{
  mutex = NULL;
  locked = false;
  Lock(Mutex);
}

cMutexLock::~cMutexLock()
{
  if (mutex && locked)
     mutex->Unlock();
}

bool cMutexLock::Lock(cMutex *Mutex)
{
  if (Mutex && !mutex) {
     mutex = Mutex;
     Mutex->Lock();
     locked = true;
     return true;
     }
  return false;
}

// --- cMyThreadLock -----------------------------------------------------------

cMyThreadLock::cMyThreadLock(cMyThread *Thread)
{
  thread = NULL;
  locked = false;
  Lock(Thread);
}

cMyThreadLock::~cMyThreadLock()
{
  if (thread && locked)
     thread->Unlock();
}

bool cMyThreadLock::Lock(cMyThread *Thread)
{
  if (Thread && !thread) {
     thread = Thread;
     Thread->Lock();
     locked = true;
     return true;
     }
  return false;
}
