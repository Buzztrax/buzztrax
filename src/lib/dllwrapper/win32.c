/***********************************************************

Win32 emulation code. Functions that emulate
responses from corresponding Win32 API calls.
Since we are not going to be able to load
virtually any DLL, we can only implement this
much, adding needed functions with each new codec.

Basic principle of implementation: it's not good
for DLL to know too much about its environment.

************************************************************/

#include "config.h"

#define QTX

#ifdef QTX
#define PSEUDO_SCREEN_WIDTH	/*640*/800
#define PSEUDO_SCREEN_HEIGHT	/*480*/600
#endif

#define LOADLIB_TRY_NATIVE

#include "winbase.h"
#include "winreg.h"
#include "winnt.h"
#include "winerror.h"
#include "debugtools.h"
#include "module.h"
#include "winuser.h"

#include <stdio.h>
#include "win32.h"

#include "registry.h"
#include "loader.h"
#include "com.h"

#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <ctype.h>
#include <pthread.h>
#include <errno.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/timeb.h>
#ifdef	HAVE_KSTAT
#include <kstat.h>
#endif

#if HAVE_VSSCANF
# ifndef __sun
/*
 * On solaris, remove the prototype for now; it's incompatible with the one
 * from solaris9 stdio.h
 */
int vsscanf (const char *str, const char *format, va_list ap);
# endif
#else
/* system has no vsscanf.  try to provide one */
static int
vsscanf (const char *str, const char *format, va_list ap)
{
  long p1 = va_arg (ap, long);
  long p2 = va_arg (ap, long);
  long p3 = va_arg (ap, long);
  long p4 = va_arg (ap, long);
  long p5 = va_arg (ap, long);
  return sscanf (str, format, p1, p2, p3, p4, p5);
}
#endif

char *win32_def_path = WIN32_PATH;

static void
do_cpuid (unsigned int ax, unsigned int *regs)
{
  __asm__ __volatile__
      ("pushl %%ebx; pushl %%ecx; pushl %%edx;"
      ".byte  0x0f, 0xa2;"
      "movl   %%eax, (%2);"
      "movl   %%ebx, 4(%2);"
      "movl   %%ecx, 8(%2);"
      "movl   %%edx, 12(%2);" "popl %%edx; popl %%ecx; popl %%ebx;":"=a" (ax)
      :"0" (ax), "S" (regs)
      );
}

static unsigned int
c_localcount_tsc ()
{
  int a;
  __asm__ __volatile__ ("rdtsc\n\t":"=a" (a)
      ::"edx");
  return a;
}

static void
c_longcount_tsc (long long *z)
{
  __asm__ __volatile__
      ("pushl %%ebx\n\t"
      "movl %%eax, %%ebx\n\t"
      "rdtsc\n\t"
      "movl %%eax, 0(%%ebx)\n\t"
      "movl %%edx, 4(%%ebx)\n\t" "popl %%ebx\n\t"::"a" (z)
      );
}

static unsigned int
c_localcount_notsc ()
{
  struct timeval tv;
  unsigned limit = ~0;
  limit /= 1000000;
  gettimeofday (&tv, 0);
  return limit * tv.tv_usec;
}

static void
c_longcount_notsc (long long *z)
{
  struct timeval tv;
  unsigned long long result;
  unsigned limit = ~0;
  if (!z)
    return;
  limit /= 1000000;
  gettimeofday (&tv, 0);
  result = tv.tv_sec;
  result <<= 32;
  result += limit * tv.tv_usec;
  *z = result;
}

static unsigned int localcount_stub (void);
static void longcount_stub (long long *);
static unsigned int (*localcount) () = localcount_stub;
static void (*longcount) (long long *) = longcount_stub;

static unsigned int
localcount_stub (void)
{
  unsigned int regs[4] = { 0, };
  do_cpuid (1, regs);
  if ((regs[3] & 0x00000010) != 0) {
    localcount = c_localcount_tsc;
    longcount = c_longcount_tsc;
  } else {
    localcount = c_localcount_notsc;
    longcount = c_longcount_notsc;
  }
  return localcount ();
}

static void
longcount_stub (long long *z)
{
  unsigned int regs[4] = { 0, };
  do_cpuid (1, regs);
  if ((regs[3] & 0x00000010) != 0) {
    localcount = c_localcount_tsc;
    longcount = c_longcount_tsc;
  } else {
    localcount = c_localcount_notsc;
    longcount = c_longcount_notsc;
  }
  longcount (z);
}

#define NUM_STUB_ENTRIES 300
char export_names[NUM_STUB_ENTRIES][32] = {
  "name1",
};

struct th_list_t;
typedef struct th_list_t
{
  int id;
  void *thread;
  struct th_list_t *next;
  struct th_list_t *prev;
} th_list;


// have to be cleared by GARBAGE COLLECTOR
static tls_t *g_tls = NULL;
static th_list *list = NULL;

#undef MEMORY_DEBUG
//#define MEMORY_DEBUG

#ifdef MEMORY_DEBUG

static unsigned char *heap = NULL;
static int heap_counter = 0;

static void
test_heap (void)
{
  int offset = 0;
  if (heap == 0)
    return;
  while (offset < heap_counter) {
    if (*(int *) (heap + offset) != 0x433476) {
      TRACE ("Heap corruption at address %d\n", offset);
      return;
    }
    offset += 8 + *(int *) (heap + offset + 4);
  }
  for (; offset < __min (offset + 1000, 20000000); offset++)
    if (heap[offset] != 0xCC) {
      TRACE ("Free heap corruption at address %d\n", offset);
    }
}

static void *
my_mreq (int size, int to_zero)
{
  static int test = 0;
  test++;
  if (test % 10 == 0)
    TRACE ("Memory: %d bytes allocated\n", heap_counter);
  //    test_heap();
  if (heap == NULL) {
    heap = malloc (20000000);
    memset (heap, 0xCC, 20000000);
  }
  if (heap == 0) {
    TRACE ("No enough memory\n");
    return 0;
  }
  if (heap_counter + size > 20000000) {
    TRACE ("No enough memory\n");
    return 0;
  }
  *(int *) (heap + heap_counter) = 0x433476;
  heap_counter += 4;
  *(int *) (heap + heap_counter) = size;
  heap_counter += 4;
  TRACE ("Allocated %d bytes of memory: sys %d, user %d-%d\n", size,
      heap_counter - 8, heap_counter, heap_counter + size);
  if (to_zero)
    memset (heap + heap_counter, 0, size);
  else
    memset (heap + heap_counter, 0xcc, size);   // make crash reproducable
  heap_counter += size;
  return heap + heap_counter - size;
}

static int
my_release (char *memory)
{
  //    test_heap();
  if (memory == NULL) {
    TRACE ("ERROR: free(0)\n");
    return 0;
  }
  if (*(int *) (memory - 8) != 0x433476) {
    TRACE ("MEMORY CORRUPTION !!!!!!!!!!!!!!!!!!!\n");
    return 0;
  }
  TRACE ("Freed %d bytes of memory\n", *(int *) (memory - 4));
  //    memset(memory-8, *(int*)(memory-4), 0xCC);
  return 0;
}

#else
#define GARBAGE
typedef struct alloc_header_t alloc_header;
struct alloc_header_t
{
  // let's keep allocated data 16 byte aligned
  alloc_header *prev;
  alloc_header *next;
  long deadbeef;
  long size;
  long type;
  long reserved1;
  long reserved2;
  long reserved3;
};

#ifdef GARBAGE
static void destroy_event (void *event);

static pthread_mutex_t memmut;
static alloc_header *last_alloc = NULL;
static int alccnt = 0;
#endif

#define AREATYPE_CLIENT 0
#define AREATYPE_EVENT 1
#define AREATYPE_MUTEX 2
#define AREATYPE_COND 3
#define AREATYPE_CRITSECT 4

/* -- critical sections -- */
struct CRITSECT
{
  pthread_t id;
  pthread_mutex_t mutex;
  int locked;
  long deadbeef;
};

static void *
mreq_private (int size, int to_zero, int type)
{
  int nsize = size + sizeof (alloc_header);
  alloc_header *header = (alloc_header *) malloc (nsize);
  if (!header)
    return 0;
  if (to_zero)
    memset (header, 0, nsize);
#ifdef GARBAGE
  if (!last_alloc) {
    pthread_mutex_init (&memmut, NULL);
    pthread_mutex_lock (&memmut);
  } else {
    pthread_mutex_lock (&memmut);
    last_alloc->next = header;  /* set next */
  }

  header->prev = last_alloc;
  header->next = 0;
  last_alloc = header;
  alccnt++;
  pthread_mutex_unlock (&memmut);
#endif
  header->deadbeef = 0xdeadbeef;
  header->size = size;
  header->type = type;

  //if (alccnt < 40000) TRACE("MY_REQ: %p\t%d   t:%d  (cnt:%d)\n",  header, size, type, alccnt);
  return header + 1;
}

static int
my_release (void *memory)
{
  alloc_header *header = (alloc_header *) memory - 1;
#ifdef GARBAGE
  alloc_header *prevmem;
  alloc_header *nextmem;

  if (memory == 0)
    return 0;

  if (header->deadbeef != (long) 0xdeadbeef) {
    TRACE ("FATAL releasing corrupted memory! %p  0x%lx  (%d)\n", header,
        header->deadbeef, alccnt);
    return 0;
  }

  pthread_mutex_lock (&memmut);

  switch (header->type) {
    case AREATYPE_EVENT:
      destroy_event (memory);
      break;
    case AREATYPE_COND:
      pthread_cond_destroy ((pthread_cond_t *) memory);
      break;
    case AREATYPE_MUTEX:
      pthread_mutex_destroy ((pthread_mutex_t *) memory);
      break;
    case AREATYPE_CRITSECT:
      pthread_mutex_destroy (&((struct CRITSECT *) memory)->mutex);
      break;
    default:
      //memset(memory, 0xcc, header->size);
      ;
  }

  header->deadbeef = 0;
  prevmem = header->prev;
  nextmem = header->next;

  if (prevmem)
    prevmem->next = nextmem;
  if (nextmem)
    nextmem->prev = prevmem;

  if (header == last_alloc)
    last_alloc = prevmem;

  alccnt--;

  /* xine: mutex must be unlocked on entrance of pthread_mutex_destroy */
  pthread_mutex_unlock (&memmut);
  if (!last_alloc)
    pthread_mutex_destroy (&memmut);

  //if (alccnt < 40000) TRACE("MY_RELEASE: %p\t%ld    (%d)\n", header, header->size, alccnt);
#else
  if (memory == 0)
    return 0;
#endif
  //memset(header + 1, 0xcc, header->size);
  free (header);
  return 0;
}
#endif

static inline void *
my_mreq (int size, int to_zero)
{
  return mreq_private (size, to_zero, AREATYPE_CLIENT);
}

static int
my_size (void *memory)
{
  if (!memory)
    return 0;
  return ((alloc_header *) memory)[-1].size;
}

static void *
my_realloc (void *memory, int size)
{
  void *ans = memory;
  int osize;
  if (memory == NULL)
    return my_mreq (size, 0);
  osize = my_size (memory);
  if (osize < size) {
    ans = my_mreq (size, 0);
    memcpy (ans, memory, osize);
    my_release (memory);
  }
  return ans;
}

/*
 *
 *  WINE  API  - native implementation for several win32 libraries
 *
 */

static int WINAPI
ext_unknown ()
{
  TRACE ("Unknown func called\n");
  return 0;
}

static int WINAPI
expIsBadWritePtr (void *ptr, unsigned int count)
{
  int result = (count == 0 || ptr != 0) ? 0 : 1;
  TRACE ("IsBadWritePtr(%p, 0x%x) => %d\n", ptr, count, result);
  return result;
}

static int WINAPI
expIsBadReadPtr (void *ptr, unsigned int count)
{
  int result = (count == 0 || ptr != 0) ? 0 : 1;
  TRACE ("IsBadReadPtr(%p, 0x%x) => %d\n", ptr, count, result);
  return result;
}

static int WINAPI
expDisableThreadLibraryCalls (int module)
{
  TRACE ("DisableThreadLibraryCalls(0x%x) => 0\n", module);
  return 0;
}

static HMODULE WINAPI
expGetDriverModuleHandle (DRVR * pdrv)
{
  HMODULE result;
  if (pdrv == NULL)
    result = 0;
  else
    result = pdrv->hDriverModule;
  TRACE ("GetDriverModuleHandle(%p) => %d\n", pdrv, result);
  return result;
}

#define	MODULE_HANDLE_kernel32	((HMODULE)0x120)
#define	MODULE_HANDLE_user32	((HMODULE)0x121)
#ifdef QTX
#define	MODULE_HANDLE_wininet	((HMODULE)0x122)
#define	MODULE_HANDLE_ddraw	((HMODULE)0x123)
#define	MODULE_HANDLE_advapi32	((HMODULE)0x124)
#endif

static HMODULE WINAPI
expGetModuleHandleA (const char *name)
{
  WINE_MODREF *wm;
  HMODULE result;
  if (!name)
#ifdef QTX
    result = 1;
#else
    result = 0;
#endif
  else {
    TRACE ("calling FindModule(%s)\n", name);
    wm = MODULE_FindModule (name);
    if (wm == 0)
      result = 0;
    else
      result = (HMODULE) (wm->module);
  }
  if (!result) {
    if (name && (strcasecmp (name, "kernel32") == 0
            || !strcasecmp (name, "kernel32.dll")))
      result = MODULE_HANDLE_kernel32;
#ifdef QTX
    if (name && strcasecmp (name, "user32") == 0)
      result = MODULE_HANDLE_user32;
#endif
  }
  TRACE ("GetModuleHandleA('%s') => 0x%x\n", name, result);
  return result;
}

static void *WINAPI
expCreateThread (void *pSecAttr, long dwStackSize,
    void *lpStartAddress, void *lpParameter, long dwFlags, long *dwThreadId)
{
  pthread_t *pth;
  //TRACE("CreateThread:");
  pth = (pthread_t *) my_mreq (sizeof (pthread_t), 0);
  pthread_create (pth, NULL, (void *(*)(void *)) lpStartAddress, lpParameter);
  if (dwFlags)
    TRACE ("WARNING: CreateThread flags not supported\n");
  if (dwThreadId)
    *dwThreadId = (long) pth;
  if (list == NULL) {
    list = my_mreq (sizeof (th_list), 1);
    list->next = list->prev = NULL;
  } else {
    list->next = my_mreq (sizeof (th_list), 0);
    list->next->prev = list;
    list->next->next = NULL;
    list = list->next;
  }
  list->thread = pth;
  TRACE ("CreateThread(%p, %ld, %p, %p, %ld, %p) => %p\n",
      pSecAttr, dwStackSize, lpStartAddress, lpParameter, dwFlags, dwThreadId,
      pth);
  return pth;
}

struct mutex_list_t;

struct mutex_list_t
{
  char type;
  pthread_mutex_t *pm;
  pthread_cond_t *pc;
  char state;
  char reset;
  char name[128];
  int semaphore;
  struct mutex_list_t *next;
  struct mutex_list_t *prev;
};
typedef struct mutex_list_t mutex_list;
static mutex_list *mlist = NULL;

#ifdef GARBAGE

static void
destroy_event (void *event)
{
  mutex_list *pp = mlist;
  //TRACE("garbage collector: destroy_event(%x)\n", event);
  while (pp) {
    if (pp == (mutex_list *) event) {
      if (pp->next)
        pp->next->prev = pp->prev;
      if (pp->prev)
        pp->prev->next = pp->next;
      if (mlist == (mutex_list *) event)
        mlist = mlist->prev;
      /*
         pp=mlist;
         while(pp)
         {
         TRACE("%x => ", pp);
         pp=pp->prev;
         }
         TRACE("0\n");
       */
      return;
    }
    pp = pp->prev;
  }
}
#endif

static void *WINAPI
expCreateEventA (void *pSecAttr, char bManualReset,
    char bInitialState, const char *name)
{
  pthread_mutex_t *pm;
  pthread_cond_t *pc;
  /*
     mutex_list* pp;
     pp=mlist;
     while(pp)
     {
     TRACE("%x => ", pp);
     pp=pp->prev;
     }
     TRACE("0\n");
   */
  if (mlist != NULL) {
    mutex_list *pp = mlist;
    if (name != NULL)
      do {
        if ((strcmp (pp->name, name) == 0) && (pp->type == 0)) {
          TRACE ("CreateEventA(%p, 0x%x, 0x%x, %p='%s') => %p\n",
              pSecAttr, bManualReset, bInitialState, name, name, pp->pm);
          return pp->pm;
        }
      } while ((pp = pp->prev) != NULL);
  }
  pm = mreq_private (sizeof (pthread_mutex_t), 0, AREATYPE_MUTEX);
  pthread_mutex_init (pm, NULL);
  pc = mreq_private (sizeof (pthread_cond_t), 0, AREATYPE_COND);
  pthread_cond_init (pc, NULL);
  if (mlist == NULL) {
    mlist = mreq_private (sizeof (mutex_list), 00, AREATYPE_EVENT);
    mlist->next = mlist->prev = NULL;
  } else {
    mlist->next = mreq_private (sizeof (mutex_list), 00, AREATYPE_EVENT);
    mlist->next->prev = mlist;
    mlist->next->next = NULL;
    mlist = mlist->next;
  }
  mlist->type = 0;              /* Type Event */
  mlist->pm = pm;
  mlist->pc = pc;
  mlist->state = bInitialState;
  mlist->reset = bManualReset;
  if (name)
    strncpy (mlist->name, name, 127);
  else
    mlist->name[0] = 0;
  /*
     if(bInitialState)
     pthread_mutex_lock(pm);
   */
  if (name)
    TRACE ("CreateEventA(%p, 0x%x, 0x%x, %p='%s') => %p\n",
        pSecAttr, bManualReset, bInitialState, name, name, mlist);
  else
    TRACE ("CreateEventA(%p, 0x%x, 0x%x, NULL) => %p\n",
        pSecAttr, bManualReset, bInitialState, mlist);
  return mlist;
}

static void *WINAPI
expSetEvent (void *event)
{
  mutex_list *ml = (mutex_list *) event;
  TRACE ("SetEvent(%p) => 0x1\n", event);
  pthread_mutex_lock (ml->pm);
  if (ml->state == 0) {
    ml->state = 1;
    pthread_cond_signal (ml->pc);
  }
  pthread_mutex_unlock (ml->pm);

  return (void *) 1;
}

static void *WINAPI
expResetEvent (void *event)
{
  mutex_list *ml = (mutex_list *) event;
  TRACE ("ResetEvent(%p) => 0x1\n", event);
  pthread_mutex_lock (ml->pm);
  ml->state = 0;
  pthread_mutex_unlock (ml->pm);

  return (void *) 1;
}

static void *WINAPI
expWaitForSingleObject (void *object, int duration)
{
  mutex_list *ml = (mutex_list *) object;
  // FIXME FIXME FIXME - this value is sometime unititialize !!!
  int ret = WAIT_FAILED;
  mutex_list *pp = mlist;
  if (object == (void *) 0xcfcf9898) {
        /**
	 From GetCurrentThread() documentation:
	 A pseudo handle is a special constant that is interpreted as the current thread handle. The calling thread can use this handle to specify itself whenever a thread handle is required. Pseudo handles are not inherited by child processes.

	 This handle has the maximum possible access to the thread object. For systems that support security descriptors, this is the maximum access allowed by the security descriptor for the calling process. For systems that do not support security descriptors, this is THREAD_ALL_ACCESS.

	 The function cannot be used by one thread to create a handle that can be used by other threads to refer to the first thread. The handle is always interpreted as referring to the thread that is using it. A thread can create a "real" handle to itself that can be used by other threads, or inherited by other processes, by specifying the pseudo handle as the source handle in a call to the DuplicateHandle function.
	 **/
    TRACE ("WaitForSingleObject(thread_handle) called\n");
    return (void *) WAIT_FAILED;
  }
  TRACE ("WaitForSingleObject(%p, duration %d) =>\n", object, duration);

  // loop below was slightly fixed - its used just for checking if
  // this object really exists in our list
  if (!ml)
    return (void *) ret;
  while (pp && (pp->pm != ml->pm))
    pp = pp->prev;
  if (!pp) {
    TRACE ("WaitForSingleObject: NotFound\n");
    return (void *) ret;
  }

  pthread_mutex_lock (ml->pm);

  switch (ml->type) {
    case 0:                    /* Event */
      if (duration == 0) {      /* Check Only */
        if (ml->state == 1)
          ret = WAIT_FAILED;
        else
          ret = WAIT_OBJECT_0;
      }
      if (duration == -1) {     /* INFINITE */
        if (ml->state == 0)
          pthread_cond_wait (ml->pc, ml->pm);
        if (ml->reset)
          ml->state = 0;
        ret = WAIT_OBJECT_0;
      }
      if (duration > 0) {       /* Timed Wait */
        struct timespec abstime;
        struct timeval now;
        gettimeofday (&now, 0);
        abstime.tv_sec = now.tv_sec + (now.tv_usec + duration) / 1000000;
        abstime.tv_nsec = ((now.tv_usec + duration) % 1000000) * 1000;
        if (ml->state == 0)
          ret = pthread_cond_timedwait (ml->pc, ml->pm, &abstime);
        if (ret == ETIMEDOUT)
          ret = WAIT_TIMEOUT;
        else
          ret = WAIT_OBJECT_0;
        if (ml->reset)
          ml->state = 0;
      }
      break;
    case 1:                    /* Semaphore */
      if (duration == 0) {
        if (ml->semaphore == 0)
          ret = WAIT_FAILED;
        else {
          ml->semaphore++;
          ret = WAIT_OBJECT_0;
        }
      }
      if (duration == -1) {
        if (ml->semaphore == 0)
          pthread_cond_wait (ml->pc, ml->pm);
        ml->semaphore--;
      }
      break;
  }
  pthread_mutex_unlock (ml->pm);

  TRACE ("WaitForSingleObject(%p, %d): %p => 0x%x \n", object, duration, ml,
      ret);
  return (void *) ret;
}

#ifdef QTX
static void *WINAPI
expWaitForMultipleObjects (int count, const void **objects,
    int WaitAll, int duration)
{
  int i;
  void *object;
  void *ret;

  TRACE ("WaitForMultipleObjects(%d, %p, %d, duration %d) =>\n",
      count, objects, WaitAll, duration);

  for (i = 0; i < count; i++) {
    object = (void *) objects[i];
    ret = expWaitForSingleObject (object, duration);
    if (WaitAll)
      TRACE ("WaitAll flag not yet supported...\n");
    else
      return ret;
  }
  return NULL;
}

static void WINAPI
expExitThread (int retcode)
{
  TRACE ("ExitThread(%d)\n", retcode);
  pthread_exit (&retcode);
}

static HANDLE WINAPI
expCreateMutexA (void *pSecAttr, char bInitialOwner, const char *name)
{
#ifndef QTX
  HANDLE mlist = (HANDLE) expCreateEventA (pSecAttr, 0, 0, name);

  TRACE ("CreateMutexA(%p, %d, '%s') => 0x%x\n",
      pSecAttr, bInitialOwner, name ? name : "NULL", mlist);
  /* 10l to QTX, if CreateMutex returns a real mutex, WaitForSingleObject
     waits for ever, else it works ;) */
  return mlist;
#else
  TRACE ("CreateMutexA(%p, %d, '%s') => 0x0\n",
      pSecAttr, bInitialOwner, name ? name : "NULL");
  return 0;
#endif
}

static int WINAPI
expReleaseMutex (HANDLE hMutex)
{
  TRACE ("ReleaseMutex(%x) => 1\n", hMutex);
  /* FIXME:XXX !! not yet implemented */
  return 1;
}
#endif

static int pf_set = 0;
static BYTE PF[64] = { 0, };

static void
DumpSystemInfo (const SYSTEM_INFO * si)
{
  TRACE ("  Processor architecture %d\n", si->u.s.wProcessorArchitecture);
  TRACE ("  Page size: %ld\n", si->dwPageSize);
  TRACE ("  Minimum app address: %p\n", si->lpMinimumApplicationAddress);
  TRACE ("  Maximum app address: %p\n", si->lpMaximumApplicationAddress);
  TRACE ("  Active processor mask: 0x%lx\n", si->dwActiveProcessorMask);
  TRACE ("  Number of processors: %ld\n", si->dwNumberOfProcessors);
  TRACE ("  Processor type: 0x%lx\n", si->dwProcessorType);
  TRACE ("  Allocation granularity: 0x%lx\n", si->dwAllocationGranularity);
  TRACE ("  Processor level: 0x%x\n", si->wProcessorLevel);
  TRACE ("  Processor revision: 0x%x\n", si->wProcessorRevision);
}

static void WINAPI
expGetSystemInfo (SYSTEM_INFO * si)
{
  /* FIXME: better values for the two entries below... */
  static int cache = 0;
  static SYSTEM_INFO cachedsi;
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__svr4__)
  unsigned int regs[4];
#endif
  TRACE ("GetSystemInfo(%p) =>\n", si);

  if (cache) {
    memcpy (si, &cachedsi, sizeof (*si));
    DumpSystemInfo (si);
    return;
  }
  memset (PF, 0, sizeof (PF));
  pf_set = 1;

  cachedsi.u.s.wProcessorArchitecture = PROCESSOR_ARCHITECTURE_INTEL;
  cachedsi.dwPageSize = getpagesize ();

  /* FIXME: better values for the two entries below... */
  cachedsi.lpMinimumApplicationAddress = (void *) 0x00000000;
  cachedsi.lpMaximumApplicationAddress = (void *) 0x7FFFFFFF;
  cachedsi.dwActiveProcessorMask = 1;
  cachedsi.dwNumberOfProcessors = 1;
  cachedsi.dwProcessorType = PROCESSOR_INTEL_386;
  cachedsi.dwAllocationGranularity = 0x10000;
  cachedsi.wProcessorLevel = 5; /* pentium */
  cachedsi.wProcessorRevision = 0x0101;

#ifdef MPLAYER
  /* mplayer's way to detect PF's */
  {
#include "../cpudetect.h"
    extern CpuCaps gCpuCaps;

    if (gCpuCaps.hasMMX)
      PF[PF_MMX_INSTRUCTIONS_AVAILABLE] = TRUE;
    if (gCpuCaps.hasSSE)
      PF[PF_XMMI_INSTRUCTIONS_AVAILABLE] = TRUE;
    if (gCpuCaps.has3DNow)
      PF[PF_AMD3D_INSTRUCTIONS_AVAILABLE] = TRUE;

    switch (gCpuCaps.cpuType) {
      case CPUTYPE_I686:
      case CPUTYPE_I586:
        cachedsi.dwProcessorType = PROCESSOR_INTEL_PENTIUM;
        cachedsi.wProcessorLevel = 5;
        break;
      case CPUTYPE_I486:
        cachedsi.dwProcessorType = PROCESSOR_INTEL_486;
        cachedsi.wProcessorLevel = 4;
        break;
      case CPUTYPE_I386:
      default:
        cachedsi.dwProcessorType = PROCESSOR_INTEL_386;
        cachedsi.wProcessorLevel = 3;
        break;
    }
    cachedsi.wProcessorRevision = gCpuCaps.cpuStepping;
    cachedsi.dwNumberOfProcessors = 1;  /* hardcoded */

  }
#endif

/* disable cpuid based detection (mplayer's cpudetect.c does this - see above) */
#ifndef MPLAYER
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__svr4__)
  do_cpuid (1, regs);
  switch ((regs[0] >> 8) & 0xf) {       // cpu family
    case 3:
      cachedsi.dwProcessorType = PROCESSOR_INTEL_386;
      cachedsi.wProcessorLevel = 3;
      break;
    case 4:
      cachedsi.dwProcessorType = PROCESSOR_INTEL_486;
      cachedsi.wProcessorLevel = 4;
      break;
    case 5:
      cachedsi.dwProcessorType = PROCESSOR_INTEL_PENTIUM;
      cachedsi.wProcessorLevel = 5;
      break;
    case 6:
      cachedsi.dwProcessorType = PROCESSOR_INTEL_PENTIUM;
      cachedsi.wProcessorLevel = 5;
      break;
    default:
      cachedsi.dwProcessorType = PROCESSOR_INTEL_PENTIUM;
      cachedsi.wProcessorLevel = 5;
      break;
  }
  cachedsi.wProcessorRevision = regs[0] & 0xf;  // stepping
  if (regs[3] & (1 << 8))
    PF[PF_COMPARE_EXCHANGE_DOUBLE] = TRUE;
  if (regs[3] & (1 << 23))
    PF[PF_MMX_INSTRUCTIONS_AVAILABLE] = TRUE;
  if (regs[3] & (1 << 25))
    PF[PF_XMMI_INSTRUCTIONS_AVAILABLE] = TRUE;
  if (regs[3] & (1 << 31))
    PF[PF_AMD3D_INSTRUCTIONS_AVAILABLE] = TRUE;
  cachedsi.dwNumberOfProcessors = 1;
#endif
#endif /* MPLAYER */

/* MPlayer: linux detection enabled (based on proc/cpuinfo) for checking
   fdiv_bug and fpu emulation flags -- alex/MPlayer */
#ifdef __linux__
  {
    char buf[20];
    char line[200];
    FILE *f = fopen ("/proc/cpuinfo", "r");

    if (!f)
      return;
    while (fgets (line, 200, f) != NULL) {
      char *s, *value;

      /* NOTE: the ':' is the only character we can rely on */
      if (!(value = strchr (line, ':')))
        continue;
      /* terminate the valuename */
      *value++ = '\0';
      /* skip any leading spaces */
      while (*value == ' ')
        value++;
      if ((s = strchr (value, '\n')))
        *s = '\0';

      /* 2.1 method */
      if (!lstrncmpiA (line, "cpu family", strlen ("cpu family"))) {
        if (isdigit (value[0])) {
          switch (value[0] - '0') {
            case 3:
              cachedsi.dwProcessorType = PROCESSOR_INTEL_386;
              cachedsi.wProcessorLevel = 3;
              break;
            case 4:
              cachedsi.dwProcessorType = PROCESSOR_INTEL_486;
              cachedsi.wProcessorLevel = 4;
              break;
            case 5:
              cachedsi.dwProcessorType = PROCESSOR_INTEL_PENTIUM;
              cachedsi.wProcessorLevel = 5;
              break;
            case 6:
              cachedsi.dwProcessorType = PROCESSOR_INTEL_PENTIUM;
              cachedsi.wProcessorLevel = 5;
              break;
            default:
              cachedsi.dwProcessorType = PROCESSOR_INTEL_PENTIUM;
              cachedsi.wProcessorLevel = 5;
              break;
          }
        }
        /* set the CPU type of the current processor */
        snprintf (buf, sizeof(buf), "CPU %ld", cachedsi.dwProcessorType);
        continue;
      }
      /* old 2.0 method */
      if (!lstrncmpiA (line, "cpu", strlen ("cpu"))) {
        if (isdigit (value[0]) && value[1] == '8' &&
            value[2] == '6' && value[3] == 0) {
          switch (value[0] - '0') {
            case 3:
              cachedsi.dwProcessorType = PROCESSOR_INTEL_386;
              cachedsi.wProcessorLevel = 3;
              break;
            case 4:
              cachedsi.dwProcessorType = PROCESSOR_INTEL_486;
              cachedsi.wProcessorLevel = 4;
              break;
            case 5:
              cachedsi.dwProcessorType = PROCESSOR_INTEL_PENTIUM;
              cachedsi.wProcessorLevel = 5;
              break;
            case 6:
              cachedsi.dwProcessorType = PROCESSOR_INTEL_PENTIUM;
              cachedsi.wProcessorLevel = 5;
              break;
            default:
              cachedsi.dwProcessorType = PROCESSOR_INTEL_PENTIUM;
              cachedsi.wProcessorLevel = 5;
              break;
          }
        }
        /* set the CPU type of the current processor */
        sprintf (buf, sizeof(buf), "CPU %ld", cachedsi.dwProcessorType);
        continue;
      }
      if (!lstrncmpiA (line, "fdiv_bug", strlen ("fdiv_bug"))) {
        if (!lstrncmpiA (value, "yes", 3))
          PF[PF_FLOATING_POINT_PRECISION_ERRATA] = TRUE;

        continue;
      }
      if (!lstrncmpiA (line, "fpu", strlen ("fpu"))) {
        if (!lstrncmpiA (value, "no", 2))
          PF[PF_FLOATING_POINT_EMULATED] = TRUE;

        continue;
      }
      if (!lstrncmpiA (line, "processor", strlen ("processor"))) {
        /* processor number counts up... */
        unsigned int x;

        if (sscanf (value, "%d", &x))
          if (x + 1 > cachedsi.dwNumberOfProcessors)
            cachedsi.dwNumberOfProcessors = x + 1;

        /* Create a new processor subkey on a multiprocessor
         * system
         */
        sprintf (buf, sizeof(buf), "%d", x);
      }
      if (!lstrncmpiA (line, "stepping", strlen ("stepping"))) {
        int x;

        if (sscanf (value, "%d", &x))
          cachedsi.wProcessorRevision = x;
      }
      if ((!lstrncmpiA (line, "flags", strlen ("flags")))
          || (!lstrncmpiA (line, "features", strlen ("features")))) {
        if (strstr (value, "cx8"))
          PF[PF_COMPARE_EXCHANGE_DOUBLE] = TRUE;
        if (strstr (value, "mmx"))
          PF[PF_MMX_INSTRUCTIONS_AVAILABLE] = TRUE;
        if (strstr (value, "tsc"))
          PF[PF_RDTSC_INSTRUCTION_AVAILABLE] = TRUE;
        if (strstr (value, "xmm"))
          PF[PF_XMMI_INSTRUCTIONS_AVAILABLE] = TRUE;
        if (strstr (value, "3dnow"))
          PF[PF_AMD3D_INSTRUCTIONS_AVAILABLE] = TRUE;
      }
    }
    fclose (f);
    /*
     *      ad hoc fix for smp machines.
     *      some problems on WaitForSingleObject,CreateEvent,SetEvent
     *                      CreateThread ...etc..
     *
     */
    cachedsi.dwNumberOfProcessors = 1;
  }
#endif /* __linux__ */
  cache = 1;
  memcpy (si, &cachedsi, sizeof (*si));
  DumpSystemInfo (si);
}

// avoid undefined expGetSystemInfo
static WIN_BOOL WINAPI
expIsProcessorFeaturePresent (DWORD v)
{
  WIN_BOOL result = 0;
  if (!pf_set) {
    SYSTEM_INFO si;
    expGetSystemInfo (&si);
  }
  if (v < 64)
    result = PF[v];
  TRACE ("IsProcessorFeaturePresent(0x%lx) => 0x%x\n", v, result);
  return result;
}


static long WINAPI
expGetVersion ()
{
  TRACE ("GetVersion() => 0xC0000004\n");
  return 0xC0000004;            //Windows 95
}

static HANDLE WINAPI
expHeapCreate (long flags, long init_size, long max_size)
{
  //    TRACE("HeapCreate:");
  HANDLE result;
  if (init_size == 0)
    result = (HANDLE) my_mreq (0x110000, 0);
  else
    result = (HANDLE) my_mreq ((init_size + 0xfff) & 0x7ffff000, 0);
  TRACE
      ("HeapCreate(flags 0x%lx, initial size %ld, maximum size %ld) => 0x%x\n",
      flags, init_size, max_size, result);
  return result;
}

// this is another dirty hack
// VP31 is releasing one allocated Heap chunk twice
// we will silently ignore this second call...
static void *heapfreehack = 0;
static int heapfreehackshown = 0;
//extern void trapbug(void);
static void *WINAPI
expHeapAlloc (HANDLE heap, int flags, int size)
{
  void *z;
    /**
     Morgan's m3jpeg32.dll v. 2.0 encoder expects that request for
     HeapAlloc returns area larger than size argument :-/

     actually according to M$ Doc  HeapCreate size should be rounded
     to page boundaries thus we should simulate this
     **/
  //if (size == 22276) trapbug();
  z = my_mreq ((size + 0xfff) & 0x7ffff000, (flags & HEAP_ZERO_MEMORY));
  if (z == 0)
    TRACE ("HeapAlloc failure\n");
  TRACE ("HeapAlloc(heap 0x%x, flags 0x%x, size %d) => %p\n", heap, flags,
      size, z);
  heapfreehack = 0;             // reset
  return z;
}

static long WINAPI
expHeapDestroy (void *heap)
{
  TRACE ("HeapDestroy(heap %p) => 1\n", heap);
  my_release (heap);
  return 1;
}

static long WINAPI
expHeapFree (HANDLE heap, DWORD dwFlags, LPVOID lpMem)
{
  TRACE ("HeapFree(0x%x, 0x%lx, pointer %p) => 1\n", heap, dwFlags, lpMem);
  if (heapfreehack != lpMem && lpMem != (void *) 0xffffffff
      && lpMem != (void *) 0xbdbdbdbd)
    // 0xbdbdbdbd is for i263_drv.drv && libefence
    // it seems to be reading from relased memory
    // EF_PROTECT_FREE doens't show any probleme
    my_release (lpMem);
  else {
    if (!heapfreehackshown++)
      TRACE ("Info: HeapFree deallocating same memory twice! (%p)\n", lpMem);
  }
  heapfreehack = lpMem;
  return 1;
}

static long WINAPI
expHeapSize (int heap, int flags, void *pointer)
{
  long result = my_size (pointer);
  TRACE ("HeapSize(heap 0x%x, flags 0x%x, pointer %p) => %ld\n", heap,
      flags, pointer, result);
  return result;
}

static void *WINAPI
expHeapReAlloc (HANDLE heap, int flags, void *lpMem, int size)
{
  TRACE ("HeapReAlloc() Size %d org %d\n", my_size (lpMem), size);
  return my_realloc (lpMem, size);
}

static long WINAPI
expGetProcessHeap (void)
{
  TRACE ("GetProcessHeap() => 1\n");
  return 1;
}

static void *WINAPI
expVirtualAlloc (void *v1, long v2, long v3, long v4)
{
  void *z = VirtualAlloc (v1, v2, v3, v4);
  if (z == 0)
    TRACE ("VirtualAlloc failure\n");
  TRACE ("VirtualAlloc(%p, %ld, %ld, %ld) => %p\n", v1, v2, v3, v4, z);
  return z;
}

static int WINAPI
expVirtualFree (void *v1, int v2, int v3)
{
  int result = VirtualFree (v1, v2, v3);
  TRACE ("VirtualFree(%p, %d, %d) => %d\n", v1, v2, v3, result);
  return result;
}

static SIZE_T WINAPI
expVirtualQuery (void *lpAddress, LPMEMORY_BASIC_INFORMATION lpBuffer,
    SIZE_T dwLength)
{
  if (!lpBuffer || !dwLength)
    return 0;

  // create fake info to keep FSM::PhatMan happy
  lpBuffer->BaseAddress = lpBuffer->AllocationBase = lpAddress;
  lpBuffer->RegionSize = 4;
  return sizeof (MEMORY_BASIC_INFORMATION);
}

/* we're building a table of critical sections. cs_win pointer uses the DLL
 cs_unix is the real structure, we're using cs_win only to identifying cs_unix */
struct critsecs_list_t
{
  CRITICAL_SECTION *cs_win;
  struct CRITSECT *cs_unix;
};

/* 'NEWTYPE' is working with VIVO, 3ivX and QTX dll (no more segfaults) -- alex */
#undef CRITSECS_NEWTYPE
//#define CRITSECS_NEWTYPE 1

#ifdef CRITSECS_NEWTYPE
/* increased due to ucod needs more than 32 entries */
/* and 64 should be enough for everything */
#define CRITSECS_LIST_MAX 64
static struct critsecs_list_t critsecs_list[CRITSECS_LIST_MAX];

static int
critsecs_get_pos (CRITICAL_SECTION * cs_win)
{
  int i;

  for (i = 0; i < CRITSECS_LIST_MAX; i++)
    if (critsecs_list[i].cs_win == cs_win)
      return i;
  return -1;
}

static int
critsecs_get_unused (void)
{
  int i;

  for (i = 0; i < CRITSECS_LIST_MAX; i++)
    if (critsecs_list[i].cs_win == NULL)
      return i;
  return -1;
}

struct CRITSECT *
critsecs_get_unix (CRITICAL_SECTION * cs_win)
{
  int i;

  for (i = 0; i < CRITSECS_LIST_MAX; i++)
    if (critsecs_list[i].cs_win == cs_win && critsecs_list[i].cs_unix)
      return (critsecs_list[i].cs_unix);
  return NULL;
}
#endif

static void WINAPI
expInitializeCriticalSection (CRITICAL_SECTION * c)
{
  TRACE ("InitializeCriticalSection(%p)\n", c);
  /*    if(sizeof(pthread_mutex_t)>sizeof(CRITICAL_SECTION))
     {
     TRACE(" ERROR:::: sizeof(pthread_mutex_t) is %d, expected <=%d!\n",
     sizeof(pthread_mutex_t), sizeof(CRITICAL_SECTION));
     return;
     } */
  /*    pthread_mutex_init((pthread_mutex_t*)c, NULL);   */
#ifdef CRITSECS_NEWTYPE
  {
    struct CRITSECT *cs;
    int i = critsecs_get_unused ();

    if (i < 0) {
      TRACE ("InitializeCriticalSection(%p) - no more space in list\n", c);
      return;
    }
    TRACE ("got unused space at %d\n", i);
    cs = malloc (sizeof (struct CRITSECT));
    if (!cs) {
      TRACE ("InitializeCriticalSection(%p) - out of memory\n", c);
      return;
    }
    pthread_mutex_init (&cs->mutex, NULL);
    cs->locked = 0;
    critsecs_list[i].cs_win = c;
    critsecs_list[i].cs_unix = cs;
    TRACE
        ("InitializeCriticalSection -> itemno=%d, cs_win=%p, cs_unix=%p\n", i,
        c, cs);
  }
#else
  {
    struct CRITSECT *cs =
        mreq_private (sizeof (struct CRITSECT) + sizeof (CRITICAL_SECTION),
        0, AREATYPE_CRITSECT);
    pthread_mutex_init (&cs->mutex, NULL);
    cs->locked = 0;
    cs->deadbeef = 0xdeadbeef;
    *(void **) c = cs;
  }
#endif
  return;
}

static void WINAPI
expEnterCriticalSection (CRITICAL_SECTION * c)
{
#ifdef CRITSECS_NEWTYPE
  struct CRITSECT *cs = critsecs_get_unix (c);
#else
  struct CRITSECT *cs = (*(struct CRITSECT **) c);
#endif
  TRACE ("EnterCriticalSection(%p) %p\n", c, cs);
  if (!cs) {
    TRACE ("entered uninitialized critisec!\n");
    expInitializeCriticalSection (c);
#ifdef CRITSECS_NEWTYPE
    cs = critsecs_get_unix (c);
#else
    cs = (*(struct CRITSECT **) c);
#endif
    TRACE
        ("wine/win32: Win32 Warning: Accessed uninitialized Critical Section (%p)!\n",
        c);
  }
  if (cs->locked)
    /* xine: recursive locking */
    if (cs->id == pthread_self ()) {
      cs->locked++;
      return;
    }
  pthread_mutex_lock (&(cs->mutex));
  cs->locked = 1;
  cs->id = pthread_self ();
  return;
}

static void WINAPI
expLeaveCriticalSection (CRITICAL_SECTION * c)
{
#ifdef CRITSECS_NEWTYPE
  struct CRITSECT *cs = critsecs_get_unix (c);
#else
  struct CRITSECT *cs = (*(struct CRITSECT **) c);
#endif
  //    struct CRITSECT* cs=(struct CRITSECT*)c;
  TRACE ("LeaveCriticalSection(%p) %p\n", c, cs);
  if (!cs) {
    TRACE ("Win32 Warning: Leaving uninitialized Critical Section %p!!\n", c);
    return;
  }

  /* xine: recursive unlocking */
  if (cs->locked) {
    cs->locked--;
    if (!cs->locked)
      pthread_mutex_unlock (&(cs->mutex));
  }
  return;
}

static void WINAPI
expDeleteCriticalSection (CRITICAL_SECTION * c)
{
#ifdef CRITSECS_NEWTYPE
  struct CRITSECT *cs = critsecs_get_unix (c);
#else
  struct CRITSECT *cs = (*(struct CRITSECT **) c);
#endif
  //    struct CRITSECT* cs=(struct CRITSECT*)c;
  TRACE ("DeleteCriticalSection(%p)\n", c);

#ifndef GARBAGE

  /* xine: mutex must be unlocked on entrance of pthread_mutex_destroy */
  if (cs->locked)
    pthread_mutex_unlock (&(cs->mutex));
  pthread_mutex_destroy (&(cs->mutex));
  // released by GarbageCollector in my_relase otherwise
#endif
  my_release (cs);
#ifdef CRITSECS_NEWTYPE
  {
    int i = critsecs_get_pos (c);

    if (i < 0) {
      TRACE ("DeleteCriticalSection(%p) error (critsec not found)\n", c);
      return;
    }

    critsecs_list[i].cs_win = NULL;
    expfree (critsecs_list[i].cs_unix);
    critsecs_list[i].cs_unix = NULL;
    TRACE ("DeleteCriticalSection -> itemno=%d\n", i);
  }
#endif
  return;
}

static int WINAPI
expGetCurrentThreadId ()
{
  TRACE ("GetCurrentThreadId() => %ld\n", pthread_self ());
  return (int) pthread_self ();
}

static int WINAPI
expGetCurrentProcess ()
{
  TRACE ("GetCurrentProcess() => %d\n", getpid ());
  return getpid ();
}

#ifdef QTX
// this version is required for Quicktime codecs (.qtx/.qts) to work.
// (they assume some pointers at FS: segment)

#define TLS_COUNT 8192
static int tls_use_map[TLS_COUNT];
static void *tls_minus_one;
static int WINAPI
expTlsAlloc ()
{
  int i;
  for (i = 0; i < TLS_COUNT; i++)
    if (tls_use_map[i] == 0) {
      tls_use_map[i] = 1;
      TRACE ("TlsAlloc() => %d\n", i);
      return i;
    }
  TRACE ("TlsAlloc() => -1 (ERROR)\n");
  return -1;
}

//static int WINAPI expTlsSetValue(DWORD index, void* value)
static int WINAPI
expTlsSetValue (int index, void *value)
{
  TRACE ("TlsSetValue(%d,%p) => 1\n", index, value);
//    if((index<0) || (index>64))
  if ((index >= TLS_COUNT))
    return 0;

  /* qt passes -1 here. probably a side effect of some bad patching */
  if (index < 0) {
    tls_minus_one = value;
    return 1;
  }
#if 0
  *(void **) ((char *) fs_seg + 0x88 + 4 * index) = value;
#else
  /* does not require fs_seg memory, if everything is right
   * we can access FS:xxxx like any win32 code would do.
   */
  index = 0x88 + 4 * index;
  __asm__ __volatile__ ("movl %0,%%fs:(%1)"::"r" (value), "r" (index)
      );
#endif
  return 1;
}

static void *WINAPI
expTlsGetValue (DWORD index)
{
  void *ret;

  TRACE ("TlsGetValue(%ld)\n", index);
//    if((index<0) || (index>64))
  if ((index >= TLS_COUNT))
    return NULL;

  /* qt passes -1 here. probably a side effect of some bad patching */
  if ((int) index < 0) {
    return tls_minus_one;
  }
#if 0
  return *(void **) ((char *) fs_seg + 0x88 + 4 * index);
#else
  /* does not require fs_seg memory, if everything is right
   * we can access FS:xxxx like any win32 code would do.
   */
  index = 0x88 + 4 * index;
  __asm__ __volatile__ ("movl %%fs:(%1),%0":"=r" (ret):"r" (index)
      );
  return ret;
#endif
}

static int WINAPI
expTlsFree (int idx)
{
  int index = (int) idx;
  TRACE ("TlsFree(%d)\n", index);
  if ((index < 0) || (index >= TLS_COUNT))
    return 0;
  tls_use_map[index] = 0;
  return 1;
}

#else
struct tls_s
{
  void *value;
  int used;
  struct tls_s *prev;
  struct tls_s *next;
};

static void *WINAPI
expTlsAlloc ()
{
  if (g_tls == NULL) {
    g_tls = my_mreq (sizeof (tls_t), 0);
    g_tls->next = g_tls->prev = NULL;
  } else {
    g_tls->next = my_mreq (sizeof (tls_t), 0);
    g_tls->next->prev = g_tls;
    g_tls->next->next = NULL;
    g_tls = g_tls->next;
  }
  TRACE ("TlsAlloc() => 0x%x\n", g_tls);
  if (g_tls)
    g_tls->value = 0;           /* XXX For Divx.dll */
  return g_tls;
}

static int WINAPI
expTlsSetValue (void *idx, void *value)
{
  tls_t *index = (tls_t *) idx;
  int result;
  if (index == 0)
    result = 0;
  else {
    index->value = value;
    result = 1;
  }
  TRACE ("TlsSetValue(index 0x%x, value 0x%x) => %d \n", index, value, result);
  return result;
}

static void *WINAPI
expTlsGetValue (void *idx)
{
  tls_t *index = (tls_t *) idx;
  void *result;
  if (index == 0)
    result = 0;
  else
    result = index->value;
  TRACE ("TlsGetValue(index 0x%x) => 0x%x\n", index, result);
  return result;
}

static int WINAPI
expTlsFree (void *idx)
{
  tls_t *index = (tls_t *) idx;
  int result;
  if (index == 0)
    result = 0;
  else {
    if (index->next)
      index->next->prev = index->prev;
    if (index->prev)
      index->prev->next = index->next;
    if (g_tls == index)
      g_tls = index->prev;
    my_release ((void *) index);
    result = 1;
  }
  TRACE ("TlsFree(index 0x%x) => %d\n", index, result);
  return result;
}
#endif

static void *WINAPI
expLocalAlloc (int flags, int size)
{
  void *z = my_mreq (size, (flags & GMEM_ZEROINIT));
  if (z == 0)
    TRACE ("LocalAlloc() failed\n");
  TRACE ("LocalAlloc(%d, flags 0x%x) => %p\n", size, flags, z);
  return z;
}

static void *WINAPI
expLocalReAlloc (int handle, int size, int flags)
{
  void *newpointer;
#ifdef USE_DEBUG
  int oldsize;
#endif

  newpointer = NULL;
  if (flags & LMEM_MODIFY) {
    TRACE ("LocalReAlloc MODIFY\n");
    return (void *) handle;
  }
#ifdef USE_DEBUG
  oldsize = my_size ((void *) handle);
#endif
  newpointer = my_realloc ((void *) handle, size);
  TRACE ("LocalReAlloc(%x %d(old %d), flags 0x%x) => %p\n", handle, size,
      oldsize, flags, newpointer);

  return newpointer;
}

static void *WINAPI
expLocalLock (void *z)
{
  TRACE ("LocalLock(%p) => %p\n", z, z);
  return z;
}

static void *WINAPI
expGlobalAlloc (int flags, int size)
{
  void *z;
  TRACE ("GlobalAlloc(%d, flags 0x%X)\n", size, flags);

  z = my_mreq (size, (flags & GMEM_ZEROINIT));
  //z=calloc(size, 1);
  //z=malloc(size);
  if (z == 0)
    TRACE ("GlobalAlloc() failed\n");
  TRACE ("GlobalAlloc(%d, flags 0x%x) => %p\n", size, flags, z);
  return z;
}

static void *WINAPI
expGlobalLock (void *z)
{
  TRACE ("GlobalLock(%p) => %p\n", z, z);
  return z;
}

// pvmjpg20 - but doesn't work anyway
static int WINAPI
expGlobalSize (void *amem)
{
  int size = 100000;
#ifdef GARBAGE
  alloc_header *header = last_alloc;
  alloc_header *mem = (alloc_header *) amem - 1;
  if (amem == 0)
    return 0;
  pthread_mutex_lock (&memmut);
  while (header) {
    if (header->deadbeef != 0xdeadbeef) {
      TRACE ("FATAL found corrupted memory! %p  0x%lx  (%d)\n", header,
          header->deadbeef, alccnt);
      break;
    }

    if (header == mem) {
      size = header->size;
      break;
    }

    header = header->prev;
  }
  pthread_mutex_unlock (&memmut);
#endif

  TRACE ("GlobalSize(%p)\n", amem);
  return size;
}

static int WINAPI
expLoadStringA (long instance, long id, void *buf, long size)
{
  int result = LoadStringA (instance, id, buf, size);
  //    if(buf)
  TRACE
      ("LoadStringA(instance 0x%lx, id 0x%lx, buffer %p, size %ld) => %d ( %s )\n",
      instance, id, buf, size, result, (char *) buf);
  //    else
  //    TRACE("LoadStringA(instance 0x%x, id 0x%x, buffer 0x%x, size %d) => %d\n",
  //  instance, id, buf, size, result);
  return result;
}

static long WINAPI
expMultiByteToWideChar (long v1, long v2, char *s1, long siz1, short *s2,
    int siz2)
{
// #warning FIXME
  int i;
  int result;
  if (s2 == 0)
    result = 1;
  else {
    if (siz1 > siz2 / 2)
      siz1 = siz2 / 2;
    for (i = 1; i <= siz1; i++) {
      *s2 = *s1;
      if (!*s1)
        break;
      s2++;
      s1++;
    }
    result = i;
  }
  if (s1)
    TRACE ("MultiByteToWideChar(codepage %ld, flags 0x%lx, string %p='%s',"
        "size %ld, dest buffer %p, dest size %d) => %d\n",
        v1, v2, s1, s1, siz1, s2, siz2, result);
  else
    TRACE ("MultiByteToWideChar(codepage %ld, flags 0x%lx, string NULL,"
        "size %ld, dest buffer %p, dest size %d) => %d\n",
        v1, v2, siz1, s2, siz2, result);
  return result;
}

static void
wch_print (const short *str)
{
  TRACE ("  src: ");
  while (*str)
    TRACE ("%c", *str++);
  TRACE ("\n");
}

static long WINAPI
expWideCharToMultiByte (long v1, long v2, short *s1, long siz1,
    char *s2, int siz2, char *c3, int *siz3)
{
  long result;
  TRACE
      ("WideCharToMultiByte(codepage %ld, flags 0x%lx, src %p, src size %ld, "
      "dest %p, dest size %d, defch %p, used_defch %p)", v1, v2, s1, siz1, s2,
      siz2, c3, siz3);
  result = WideCharToMultiByte (v1, v2, (LPCWSTR) s1, siz1, s2, siz2, c3, siz3);
  TRACE ("=> %ld\n", result);
  //if(s1)wch_print(s1);
  if (s2)
    TRACE ("  dest: %s\n", s2);
  return result;
}

static long WINAPI
expGetVersionExA (OSVERSIONINFOA * c)
{
  TRACE ("GetVersionExA(%p) => 1\n", c);
  c->dwOSVersionInfoSize = sizeof (*c);
  c->dwMajorVersion = 4;
  c->dwMinorVersion = 0;
  c->dwBuildNumber = 0x4000457;
#if 1
  // leave it here for testing win9x-only codecs
  c->dwPlatformId = VER_PLATFORM_WIN32_WINDOWS;
  strcpy (c->szCSDVersion, " B");
#else
  c->dwPlatformId = VER_PLATFORM_WIN32_NT;      // let's not make DLL assume that it can read CR* registers
  strcpy (c->szCSDVersion, "Service Pack 3");
#endif
  TRACE
      ("  Major version: 4\n  Minor version: 0\n  Build number: 0x4000457\n"
      "  Platform Id: VER_PLATFORM_WIN32_NT\n Version string: 'Service Pack 3'\n");
  return 1;
}

static HANDLE WINAPI
expCreateSemaphoreA (char *v1, long init_count, long max_count, char *name)
{
  pthread_mutex_t *pm;
  pthread_cond_t *pc;
  /* mutex_list* pp; -- unused */
  /*
     TRACE("CreateSemaphoreA(%p = %s)\n", name, (name ? name : "<null>"));
     pp=mlist;
     while(pp)
     {
     TRACE("%p => ", pp);
     pp=pp->prev;
     }
     TRACE("0\n");
   */
  if (mlist != NULL) {
    mutex_list *pp = mlist;
    if (name != NULL)
      do {
        if ((strcmp (pp->name, name) == 0) && (pp->type == 1)) {
          TRACE
              ("CreateSemaphoreA(%p, init_count %ld, max_count %ld, name %p='%s') => %p\n",
              v1, init_count, max_count, name, name, mlist);
          return (HANDLE) mlist;
        }
      } while ((pp = pp->prev) != NULL);
  }
  pm = mreq_private (sizeof (pthread_mutex_t), 0, AREATYPE_MUTEX);
  pthread_mutex_init (pm, NULL);
  pc = mreq_private (sizeof (pthread_cond_t), 0, AREATYPE_COND);
  pthread_cond_init (pc, NULL);
  if (mlist == NULL) {
    mlist = mreq_private (sizeof (mutex_list), 00, AREATYPE_EVENT);
    mlist->next = mlist->prev = NULL;
  } else {
    mlist->next = mreq_private (sizeof (mutex_list), 00, AREATYPE_EVENT);
    mlist->next->prev = mlist;
    mlist->next->next = NULL;
    mlist = mlist->next;
    //      TRACE("new semaphore %p\n", mlist);
  }
  mlist->type = 1;              /* Type Semaphore */
  mlist->pm = pm;
  mlist->pc = pc;
  mlist->state = 0;
  mlist->reset = 0;
  mlist->semaphore = init_count;
  if (name != NULL)
    strncpy (mlist->name, name, 64);
  else
    mlist->name[0] = 0;
  if (name)
    TRACE
        ("CreateSemaphoreA(%p, init_count %ld, max_count %ld, name %p='%s') => %p\n",
        v1, init_count, max_count, name, name, mlist);
  else
    TRACE
        ("CreateSemaphoreA(%p, init_count %ld, max_count %ld, name NULL) => %p\n",
        v1, init_count, max_count, mlist);
  return (HANDLE) mlist;
}

static long WINAPI
expReleaseSemaphore (long hsem, long increment, long *prev_count)
{
  // The state of a semaphore object is signaled when its count
  // is greater than zero and nonsignaled when its count is equal to zero
  // Each time a waiting thread is released because of the semaphore's signaled
  // state, the count of the semaphore is decreased by one.
  mutex_list *ml = (mutex_list *) hsem;

  pthread_mutex_lock (ml->pm);
  if (prev_count != 0)
    *prev_count = ml->semaphore;
  if (ml->semaphore == 0)
    pthread_cond_signal (ml->pc);
  ml->semaphore += increment;
  pthread_mutex_unlock (ml->pm);
  TRACE
      ("ReleaseSemaphore(semaphore 0x%lx, increment %ld, prev_count %p) => 1\n",
      hsem, increment, prev_count);
  return 1;
}


static long WINAPI
expRegOpenKeyExA (long key, const char *subkey, long reserved, long access,
    int *newkey)
{
  long result = RegOpenKeyExA (key, subkey, reserved, access, newkey);
  TRACE
      ("RegOpenKeyExA(key 0x%lx, subkey %s, reserved %ld, access 0x%lx, pnewkey %p) => %ld\n",
      key, subkey, reserved, access, newkey, result);
  if (newkey)
    TRACE ("  New key: 0x%x\n", *newkey);
  return result;
}

static long WINAPI
expRegCloseKey (long key)
{
  long result = RegCloseKey (key);
  TRACE ("RegCloseKey(0x%lx) => %ld\n", key, result);
  return result;
}

static long WINAPI
expRegQueryValueExA (long key, const char *value, int *reserved, int *type,
    int *data, int *count)
{
  long result = RegQueryValueExA (key, value, reserved, type, data, count);
  TRACE
      ("RegQueryValueExA(key 0x%lx, value %s, reserved %p, data %p, count %p)"
      " => 0x%lx\n", key, value, reserved, data, count, result);
  if (data && count)
    TRACE ("  read %d bytes: '%s'\n", *count, (char *) data);   /* FIXME? */
  return result;
}

static long WINAPI
expRegCreateKeyExA (long key, const char *name, long reserved,
    void *classs, long options, long security,
    void *sec_attr, int *newkey, int *status)
{
  long result =
      RegCreateKeyExA (key, name, reserved, classs, options, security, sec_attr,
      newkey, status);
  TRACE ("RegCreateKeyExA(key 0x%lx, name %p='%s', reserved=0x%lx,"
      " %p, 0x%lx, 0x%lx, %p, newkey=%p, status=%p) => %ld\n", key, name, name,
      reserved, classs, options, security, sec_attr, newkey, status, result);
  if (!result && newkey)
    TRACE ("  New key: 0x%x\n", *newkey);
  if (!result && status)
    TRACE ("  New key status: 0x%x\n", *status);
  return result;
}

static long WINAPI
expRegSetValueExA (long key, const char *name, long v1, long v2, void *data,
    long size)
{
  long result = RegSetValueExA (key, name, v1, v2, data, size);
  TRACE
      ("RegSetValueExA(key 0x%lx, name '%s', 0x%lx, 0x%lx, data %p -> 0x%x '%s', size=%ld) => %ld",
      key, name, v1, v2, data, *(int *) data, (char *) data, size, result);
  return result;
}

static long WINAPI
expRegOpenKeyA (long hKey, LPCSTR lpSubKey, int *phkResult)
{
  long result = RegOpenKeyExA (hKey, lpSubKey, 0, 0, phkResult);
  TRACE ("RegOpenKeyExA(key 0x%lx, subkey '%s', %p) => %ld\n",
      hKey, lpSubKey, phkResult, result);
  if (!result && phkResult)
    TRACE ("  New key: 0x%x\n", *phkResult);
  return result;
}

static DWORD WINAPI
expRegEnumValueA (HKEY hkey, DWORD index, LPSTR value, LPDWORD val_count,
    LPDWORD reserved, LPDWORD type, LPBYTE data, LPDWORD count)
{
  return RegEnumValueA (hkey, index, value, val_count,
      reserved, type, data, count);
}

static DWORD WINAPI
expRegEnumKeyExA (HKEY hKey, DWORD dwIndex, LPSTR lpName, LPDWORD lpcbName,
    LPDWORD lpReserved, LPSTR lpClass, LPDWORD lpcbClass,
    LPFILETIME lpftLastWriteTime)
{
  return RegEnumKeyExA (hKey, dwIndex, lpName, lpcbName, lpReserved, lpClass,
      lpcbClass, lpftLastWriteTime);
}

static long WINAPI
expQueryPerformanceCounter (long long *z)
{
  longcount (z);
  TRACE ("QueryPerformanceCounter(%p) => 1 ( %Ld )\n", z, *z);
  return 1;
}

/*
 * return CPU clock (in kHz), using linux's /proc filesystem (/proc/cpuinfo)
 */
static double
linux_cpuinfo_freq ()
{
  double freq = -1;
  FILE *f;
  char line[200];
  char *s, *value;

  f = fopen ("/proc/cpuinfo", "r");
  if (f != NULL) {
    while (fgets (line, sizeof (line), f) != NULL) {
      /* NOTE: the ':' is the only character we can rely on */
      if (!(value = strchr (line, ':')))
        continue;
      /* terminate the valuename */
      *value++ = '\0';
      /* skip any leading spaces */
      while (*value == ' ')
        value++;
      if ((s = strchr (value, '\n')))
        *s = '\0';

      if (!strncasecmp (line, "cpu MHz", strlen ("cpu MHz"))
          && sscanf (value, "%lf", &freq) == 1) {
        freq *= 1000;
        break;
      }
    }
    fclose (f);
  }
  return freq;
}


static double
solaris_kstat_freq ()
{
#if	defined(HAVE_LIBKSTAT) && defined(KSTAT_DATA_INT32)
  /*
   * try to extract the CPU speed from the solaris kernel's kstat data
   */
  kstat_ctl_t *kc;
  kstat_t *ksp;
  kstat_named_t *kdata;
  int mhz = 0;

  kc = kstat_open ();
  if (kc != NULL) {
    ksp = kstat_lookup (kc, "cpu_info", 0, "cpu_info0");

    /* kstat found and name/value pairs? */
    if (ksp != NULL && ksp->ks_type == KSTAT_TYPE_NAMED) {
      /* read the kstat data from the kernel */
      if (kstat_read (kc, ksp, NULL) != -1) {
        /*
         * lookup desired "clock_MHz" entry, check the expected
         * data type
         */
        kdata = (kstat_named_t *) kstat_data_lookup (ksp, "clock_MHz");
        if (kdata != NULL && kdata->data_type == KSTAT_DATA_INT32)
          mhz = kdata->value.i32;
      }
    }
    kstat_close (kc);
  }

  if (mhz > 0)
    return mhz * 1000.;
#endif /* HAVE_LIBKSTAT */
  return -1;                    // kstat stuff is not available, CPU freq is unknown
}

/*
 * Measure CPU freq using the pentium's time stamp counter register (TSC)
 */
static double
tsc_freq ()
{
  static double ofreq = 0.0;
  int i;
  int x, y;
  i = time (NULL);
  if (ofreq != 0.0)
    return ofreq;
  while (i == time (NULL));
  x = localcount ();
  i++;
  while (i == time (NULL));
  y = localcount ();
  ofreq = (double) (y - x) / 1000.;
  return ofreq;
}

static double
CPU_Freq ()
{
  double freq;

  if ((freq = linux_cpuinfo_freq ()) > 0)
    return freq;

  if ((freq = solaris_kstat_freq ()) > 0)
    return freq;

  return tsc_freq ();
}

static long WINAPI
expQueryPerformanceFrequency (long long *z)
{
  *z = (long long) CPU_Freq ();
  TRACE ("QueryPerformanceFrequency(%p) => 1 ( %Ld )\n", z, *z);
  return 1;
}

static long WINAPI
exptimeGetTime ()
{
  struct timeval t;
  long result;
  gettimeofday (&t, 0);
  result = 1000 * t.tv_sec + t.tv_usec / 1000;
  TRACE ("timeGetTime() => %ld\n", result);
  return result;
}

static void *WINAPI
expLocalHandle (void *v)
{
  TRACE ("LocalHandle(%p) => %p\n", v, v);
  return v;
}

static void *WINAPI
expGlobalHandle (void *v)
{
  TRACE ("GlobalHandle(%p) => %p\n", v, v);
  return v;
}

static int WINAPI
expGlobalUnlock (void *v)
{
  TRACE ("GlobalUnlock(%p) => 1\n", v);
  return 1;
}

static void *WINAPI
expGlobalFree (void *v)
{
  TRACE ("GlobalFree(%p) => 0\n", v);
  my_release (v);
  //free(v);
  return 0;
}

static void *WINAPI
expGlobalReAlloc (void *v, int size, int flags)
{
  void *result = my_realloc (v, size);
  //void* result=realloc(v, size);
  TRACE ("GlobalReAlloc(%p, size %d, flags 0x%x) => %p\n", v, size, flags,
      result);
  return result;
}

static int WINAPI
expLocalUnlock (void *v)
{
  TRACE ("LocalUnlock(%p) => 1\n", v);
  return 1;
}

//
static void *WINAPI
expLocalFree (void *v)
{
  TRACE ("LocalFree(%p) => 0\n", v);
  my_release (v);
  return 0;
}

static HRSRC WINAPI
expFindResourceA (HMODULE module, char *name, char *type)
{
  HRSRC result;

  result = FindResourceA (module, name, type);
  TRACE ("FindResourceA(module 0x%x, name %p(%s), type %p(%s)) => 0x%x\n",
      module, name, HIWORD (name) ? name : "UNICODE", type,
      HIWORD (type) ? type : "UNICODE", result);
  return result;
}

static HRSRC WINAPI
expFindResourceExA (HMODULE module, char *type, char *name, WORD lang)
{
  HRSRC result;

  result = FindResourceExA (module, type, name, lang);
  TRACE
      ("FindResourceExA(module 0x%x, type %p(%s), name %p(%s), lang %d) => 0x%x\n",
      module, name, HIWORD (name) ? name : "UNICODE", type,
      HIWORD (type) ? type : "UNICODE", lang, result);
  return result;
}

extern HRSRC WINAPI LoadResource (HMODULE, HRSRC);
static HGLOBAL WINAPI
expLoadResource (HMODULE module, HRSRC res)
{
  HGLOBAL result = LoadResource (module, res);
  TRACE ("LoadResource(module 0x%x, resource 0x%x) => 0x%x\n", module, res,
      result);
  return result;
}

static void *WINAPI
expLockResource (long res)
{
  void *result = LockResource (res);
  TRACE ("LockResource(0x%lx) => %p\n", res, result);
  return result;
}

static int WINAPI
expFreeResource (long res)
{
  int result = FreeResource (res);
  TRACE ("FreeResource(0x%lx) => %d\n", res, result);
  return result;
}

//bool fun(HANDLE)
//!0 on success
static int WINAPI
expCloseHandle (long v1)
{
  TRACE ("CloseHandle(0x%lx) => 1\n", v1);
  /* do not close stdin,stdout and stderr */
  if (v1 > 2)
    if (!close (v1))
      return 0;
  return 1;
}

static const char *WINAPI
expGetCommandLineA ()
{
  TRACE ("GetCommandLineA() => \"C:\\Programme\\Jeskola Buzz\\Buzz.exe\"\n");
  return "C:\\Programme\\Jeskola Buzz\\Buzz.exe";
}

static const char *WINAPI
expGetCommandLineW ()
{
  TRACE ("GetCommandLineW() => \"C:\\Programme\\Jeskola Buzz\\Buzz.exe\"\n");
  return "C:\\Programme\\Jeskola Buzz\\Buzz.exe";
}

//static short envs[]={'p', 'a', 't', 'h', ' ', 'c', ':', '\\', 0, 0};
static LPWSTR WINAPI
expGetEnvironmentStringsW ()
{
  TRACE ("GetEnvironmentStringsW() => 0\n");
  return 0;
}

static void *WINAPI
expRtlZeroMemory (void *p, size_t len)
{
  void *result = memset (p, 0, len);
  TRACE ("RtlZeroMemory(%p, len %d) => %p\n", p, len, result);
  return result;
}

static void *WINAPI
expRtlMoveMemory (void *dst, void *src, size_t len)
{
  void *result = memmove (dst, src, len);
  TRACE ("RtlMoveMemory (dest %p, src %p, len %d) => %p\n", dst, src, len,
      result);
  return result;
}

static void *WINAPI
expRtlFillMemory (void *p, int ch, size_t len)
{
  void *result = memset (p, ch, len);
  TRACE ("RtlFillMemory(%p, char 0x%x, len %d) => %p\n", p, ch, len, result);
  return result;
}

static int WINAPI
expFreeEnvironmentStringsW (short *strings)
{
  TRACE ("FreeEnvironmentStringsW(%p) => 1\n", strings);
  return 1;
}

static int WINAPI
expFreeEnvironmentStringsA (char *strings)
{
  TRACE ("FreeEnvironmentStringsA(%p) => 1\n", strings);
  return 1;
}

static const char ch_envs[] =
    "__MSVCRT_HEAP_SELECT=__GLOBAL_HEAP_SELECTED,1\r\n"
    "PATH=C:\\;C:\\windows\\;C:\\windows\\system\r\n";
static LPCSTR WINAPI
expGetEnvironmentStrings ()
{
  TRACE ("GetEnvironmentStrings() => %p\n", ch_envs);
  return (LPCSTR) ch_envs;
  // TRACE("GetEnvironmentStrings() => 0\n");
  // return 0;
}

static int WINAPI
expGetStartupInfoA (STARTUPINFOA * s)
{
  /* int i; -- unused */
  TRACE ("GetStartupInfoA(%p) => 1\n", s);
  memset (s, 0, sizeof (*s));
  s->cb = sizeof (*s);
  // s->lpReserved="Reserved";
  s->lpDesktop = "WinSta0\\Default";
  s->lpTitle = "C:\\Programme\\Jeskola Buzz\\Buzz.exe";
  // s->dwX=s->dwY=0;
  s->dwXSize = s->dwYSize = 200;
  s->dwFlags = 0x401;
  s->wShowWindow = 1;
  // s->hStdInput=s->hStdOutput=s->hStdError=0x1234;
  TRACE ("  cb=%ld\n", s->cb);
  TRACE ("  lpReserved='%s'\n", s->lpReserved);
  TRACE ("  lpDesktop='%s'\n", s->lpDesktop);
  TRACE ("  lpTitle='%s'\n", s->lpTitle);
  TRACE ("  dwX=%ld dwY=%ld dwXSize=%ld dwYSize=%ld\n",
      s->dwX, s->dwY, s->dwXSize, s->dwYSize);
  TRACE ("  dwXCountChars=%ld dwYCountChars=%ld dwFillAttribute=%ld\n",
      s->dwXCountChars, s->dwYCountChars, s->dwFillAttribute);
  TRACE ("  dwFlags=0x%lx wShowWindow=0x%x cbReserved2=0x%x\n",
      s->dwFlags, s->wShowWindow, s->cbReserved2);
  TRACE ("  lpReserved2=%p hStdInput=0x%x hStdOutput=0x%x hStdError=0x%x\n",
      s->lpReserved2, s->hStdInput, s->hStdOutput, s->hStdError);
  return 1;
}

static int WINAPI
expGetStdHandle (int z)
{
  z += 0x1234;
  TRACE ("GetStdHandle(0x%x) => 0x%x\n", z, z);
  return z;
}

#ifdef QTX
#define FILE_HANDLE_quicktimeqts	((HANDLE)0x444)
#define FILE_HANDLE_quicktimeqtx	((HANDLE)0x445)
#endif

static int WINAPI
expGetFileType (int handle)
{
  TRACE ("GetFileType(0x%x) => 0x3 = pipe\n", handle);
  return 0x3;
}

#ifdef QTX
static int WINAPI
expGetFileAttributesA (char *filename)
{
  TRACE ("GetFileAttributesA(%s) => FILE_ATTR_NORMAL\n", filename);
  if (strstr (filename, "QuickTime.qts"))
    return FILE_ATTRIBUTE_SYSTEM;
  return FILE_ATTRIBUTE_NORMAL;
}
#endif
static int WINAPI
expSetHandleCount (int count)
{
  TRACE ("SetHandleCount(0x%x) => 1\n", count);
  return 1;
}

static int WINAPI
expGetACP (void)
{
  TRACE ("GetACP() => 0\n");
  return 0;
}

extern WINE_MODREF *MODULE32_LookupHMODULE (HMODULE m);
static int WINAPI
expGetModuleFileNameA (int module, char *s, int len)
{
  WINE_MODREF *mr;
  int result;
  //TRACE("File name of module %X (%s) requested\n", module, s);

  if (module == 0 && len >= 12) {
    /* return caller program name */
    strcpy (s, "Buzz.exe");
    result = 1;
  } else if (s == 0)
    result = 0;
  else if (len < 35)
    result = 0;
  else {
    result = 1;
    strcpy (s, "c:\\windows\\system\\");
    mr = MODULE32_LookupHMODULE (module);
    if (mr == 0)                //oops
      strcat (s, "Buzz.exe");
    else if (strrchr (mr->filename, '/') == NULL)
      strcat (s, mr->filename);
    else
      strcat (s, strrchr (mr->filename, '/') + 1);
  }
  if (!s)
    TRACE ("GetModuleFileNameA(0x%x, NULL, %d) => %d\n", module, len, result);
  else
    TRACE ("GetModuleFileNameA(0x%x, %p, %d) => %d ( '%s' )\n",
        module, s, len, result, s);
  return result;
}

static int WINAPI
expSetUnhandledExceptionFilter (void *filter)
{
  TRACE ("SetUnhandledExceptionFilter(%p) => 1\n", filter);
  return 1;                     //unsupported and probably won't ever be supported
}

static int WINAPI
expLoadLibraryA (char *name)
{
  int result = 0;
  char *lastbc;
  /* int i; */
  if (!name)
    return -1;
  // we skip to the last backslash
  // this is effectively eliminating weird characters in
  // the text output windows

  lastbc = strrchr (name, '\\');
  if (lastbc) {
    int i;
    lastbc++;
    for (i = 0; 1; i++) {
      name[i] = *lastbc++;
      if (!name[i])
        break;
    }
  }
  if (strncmp (name, "c:\\windows\\", 11) == 0)
    name += 11;
  if (strncmp (name, ".\\", 2) == 0)
    name += 2;

  TRACE ("Entering LoadLibraryA(%s)\n", name);

  // PIMJ and VIVO audio are loading  kernel32.dll
  if (strcasecmp (name, "kernel32.dll") == 0
      || strcasecmp (name, "kernel32") == 0)
    return MODULE_HANDLE_kernel32;
//      return ERROR_SUCCESS; /* yeah, we have also the kernel32 calls */
  /* exported -> do not return failed! */

  if (strcasecmp (name, "user32.dll") == 0 || strcasecmp (name, "user32") == 0)
//      return MODULE_HANDLE_kernel32;
    return MODULE_HANDLE_user32;

#ifdef QTX
  if (strcasecmp (name, "wininet.dll") == 0
      || strcasecmp (name, "wininet") == 0)
    return MODULE_HANDLE_wininet;
  if (strcasecmp (name, "ddraw.dll") == 0 || strcasecmp (name, "ddraw") == 0)
    return MODULE_HANDLE_ddraw;
  if (strcasecmp (name, "advapi32.dll") == 0
      || strcasecmp (name, "advapi32") == 0)
    return MODULE_HANDLE_advapi32;
#endif

  result = LoadLibraryA (name);
  TRACE ("Returned LoadLibraryA(%p='%s'), def_path=%s => 0x%x\n", name,
      name, win32_def_path, result);

  return result;
}

static int WINAPI
expFreeLibrary (int module)
{
#ifdef QTX
  int result = 0;               /* FIXME:XXX: qtx svq3 frees up qt.qts */
#else
  int result = FreeLibrary (module);
#endif
  TRACE ("FreeLibrary(0x%x) => %d\n", module, result);
  return result;
}

static void *WINAPI
expGetProcAddress (HMODULE mod, char *name)
{
  void *result;
  switch (mod) {
    case MODULE_HANDLE_kernel32:
      result = LookupExternalByName ("kernel32.dll", name);
      break;
    case MODULE_HANDLE_user32:
      result = LookupExternalByName ("user32.dll", name);
      break;
#ifdef QTX
    case MODULE_HANDLE_wininet:
      result = LookupExternalByName ("wininet.dll", name);
      break;
    case MODULE_HANDLE_ddraw:
      result = LookupExternalByName ("ddraw.dll", name);
      break;
    case MODULE_HANDLE_advapi32:
      result = LookupExternalByName ("advapi32.dll", name);
      break;
#endif
    default:
      result = GetProcAddress (mod, name);
  }
  TRACE ("GetProcAddress(0x%x, '%s') => %p\n", mod, name, result);
  return result;
}

static long WINAPI
expCreateFileMappingA (int hFile, void *lpAttr,
    long flProtect, long dwMaxHigh, long dwMaxLow, const char *name)
{
  long result =
      CreateFileMappingA (hFile, lpAttr, flProtect, dwMaxHigh, dwMaxLow, name);
  if (!name)
    TRACE ("CreateFileMappingA(file 0x%x, lpAttr %p,"
        "flProtect 0x%lx, dwMaxHigh 0x%lx, dwMaxLow 0x%lx, name 0) => %ld\n",
        hFile, lpAttr, flProtect, dwMaxHigh, dwMaxLow, result);
  else
    TRACE ("CreateFileMappingA(file 0x%x, lpAttr %p,"
        "flProtect 0x%lx, dwMaxHigh 0x%lx, dwMaxLow 0x%lx, name %p='%s') => %ld\n",
        hFile, lpAttr, flProtect, dwMaxHigh, dwMaxLow, name, name, result);
  return result;
}

static long WINAPI
expOpenFileMappingA (long hFile, long hz, const char *name)
{
  long result = OpenFileMappingA (hFile, hz, name);
  if (!name)
    TRACE ("OpenFileMappingA(0x%lx, 0x%lx, 0) => %ld\n", hFile, hz, result);
  else
    TRACE ("OpenFileMappingA(0x%lx, 0x%lx, %p='%s') => %ld\n",
        hFile, hz, name, name, result);
  return result;
}

static void *WINAPI
expMapViewOfFile (HANDLE file, DWORD mode, DWORD offHigh,
    DWORD offLow, DWORD size)
{
  TRACE ("MapViewOfFile(0x%x, 0x%lx, 0x%lx, 0x%lx, size %ld) => %p\n",
      file, mode, offHigh, offLow, size, (char *) file + offLow);
  return (char *) file + offLow;
}

static void *WINAPI
expUnmapViewOfFile (void *view)
{
  TRACE ("UnmapViewOfFile(%p) => 0\n", view);
  return 0;
}

static void *WINAPI
expSleep (int time)
{
#if HAVE_NANOSLEEP
  /* solaris doesn't have thread safe usleep */
  struct timespec tsp;
  tsp.tv_sec = time / 1000000;
  tsp.tv_nsec = (time % 1000000) * 1000;
  nanosleep (&tsp, NULL);
#else
  usleep (time);
#endif
  TRACE ("Sleep(%d) => 0\n", time);
  return 0;
}

// why does IV32 codec want to call this? I don't know ...
static int WINAPI
expCreateCompatibleDC (int hdc)
{
  int dc = 0;                   //0x81;
  //TRACE("CreateCompatibleDC(%d) => 0x81\n", hdc);
  TRACE ("CreateCompatibleDC(%d) => %d\n", hdc, dc);
  return dc;
}

static int WINAPI
expGetDeviceCaps (int hdc, int unk)
{
  TRACE ("GetDeviceCaps(0x%x, %d) => 0\n", hdc, unk);
#ifdef QTX
#define BITSPIXEL 12
#define PLANES    14
  if (unk == BITSPIXEL)
    return 24;
  if (unk == PLANES)
    return 1;
#endif
  return 1;
}

static WIN_BOOL WINAPI
expDeleteDC (int hdc)
{
  TRACE ("DeleteDC(0x%x) => 0\n", hdc);
  if (hdc == 0x81)
    return 1;
  return 0;
}

static WIN_BOOL WINAPI
expDeleteObject (int hdc)
{
  TRACE ("DeleteObject(0x%x) => 1\n", hdc);
  /* FIXME - implement code here */
  return 1;
}

/* btvvc32.drv wants this one */
static void *WINAPI
expGetWindowDC (int hdc)
{
  TRACE ("GetWindowDC(%d) => 0x0\n", hdc);
  return 0;
}

#ifdef QTX
static int WINAPI
expGetWindowRect (HWND win, RECT * r)
{
  TRACE ("GetWindowRect(0x%x, %p) => 1\n", win, r);
  /* (win == 0) => desktop */
  r->right = PSEUDO_SCREEN_WIDTH;
  r->left = 0;
  r->bottom = PSEUDO_SCREEN_HEIGHT;
  r->top = 0;
  return 1;
}

static int WINAPI
expMonitorFromWindow (HWND win, int flags)
{
  TRACE ("MonitorFromWindow(0x%x, 0x%x) => 0\n", win, flags);
  return 0;
}

static int WINAPI
expMonitorFromRect (RECT * r, int flags)
{
  TRACE ("MonitorFromRect(%p, 0x%x) => 0\n", r, flags);
  return 0;
}

static int WINAPI
expMonitorFromPoint (void *p, int flags)
{
  TRACE ("MonitorFromPoint(%p, 0x%x) => 0\n", p, flags);
  return 0;
}

static int WINAPI
expEnumDisplayMonitors (void *dc, RECT * r,
    int WINAPI (*callback_proc) (), void *callback_param)
{
  TRACE ("EnumDisplayMonitors(%p, %p, %p, %p) => ?\n",
      dc, r, callback_proc, callback_param);
  return callback_proc (0, dc, r, callback_param);
}

#if 0
typedef struct tagMONITORINFO
{
  DWORD cbSize;
  RECT rcMonitor;
  RECT rcWork;
  DWORD dwFlags;
} MONITORINFO, *LPMONITORINFO;
#endif

#define CCHDEVICENAME 8
typedef struct tagMONITORINFOEX
{
  DWORD cbSize;
  RECT rcMonitor;
  RECT rcWork;
  DWORD dwFlags;
  TCHAR szDevice[CCHDEVICENAME];
} MONITORINFOEX, *LPMONITORINFOEX;

static int WINAPI
expGetMonitorInfoA (void *mon, LPMONITORINFO lpmi)
{
  TRACE ("GetMonitorInfoA(%p, %p) => 1\n", mon, lpmi);

  lpmi->rcMonitor.right = lpmi->rcWork.right = PSEUDO_SCREEN_WIDTH;
  lpmi->rcMonitor.left = lpmi->rcWork.left = 0;
  lpmi->rcMonitor.bottom = lpmi->rcWork.bottom = PSEUDO_SCREEN_HEIGHT;
  lpmi->rcMonitor.top = lpmi->rcWork.top = 0;

  lpmi->dwFlags = 1;            /* primary monitor */

  if (lpmi->cbSize == sizeof (MONITORINFOEX)) {
    LPMONITORINFOEX lpmiex = (LPMONITORINFOEX) lpmi;
    TRACE ("MONITORINFOEX!\n");
    strncpy (lpmiex->szDevice, "Monitor1", CCHDEVICENAME);
  }

  return 1;
}

static int WINAPI
expEnumDisplayDevicesA (const char *device, int devnum,
    void *dispdev, int flags)
{
  TRACE ("EnumDisplayDevicesA(%p = %s, %d, %p, %x) => 1\n",
      device, device, devnum, dispdev, flags);
  return 1;
}

static int WINAPI
expIsWindowVisible (HWND win)
{
  TRACE ("IsWindowVisible(0x%x) => 1\n", win);
  return 1;
}

static HWND WINAPI
expGetActiveWindow (void)
{
  TRACE ("GetActiveWindow() => 0\n");
  return (HWND) 0;
}

static int WINAPI
expGetClassNameA (HWND win, LPTSTR classname, int maxcount)
{
  strncat (classname, "QuickTime", maxcount);
  TRACE ("GetClassNameA(0x%x, %p, %d) => %d\n",
      win, classname, maxcount, strlen (classname));
  return strlen (classname);
}

#define LPWNDCLASS void *
static int WINAPI
expGetClassInfoA (HINSTANCE inst, LPCSTR classname, LPWNDCLASS wndclass)
{
  TRACE ("GetClassInfoA(0x%x, %p = %s, %p) => 1\n", inst,
      classname, classname, wndclass);
  return 1;
}

static int WINAPI
expGetWindowLongA (HWND win, int index)
{
  TRACE ("GetWindowLongA(0x%x, %d) => 0\n", win, index);
  return 1;
}

static int WINAPI
expGetObjectA (HGDIOBJ hobj, int objsize, LPVOID obj)
{
  TRACE ("GetObjectA(0x%x, %d, %p) => %d\n", hobj, objsize, obj, objsize);
  return objsize;
}

static int WINAPI
expCreateRectRgn (int x, int y, int width, int height)
{
  TRACE ("CreateRectRgn(%d, %d, %d, %d) => 0\n", x, y, width, height);
  return 0;
}

static int WINAPI
expEnumWindows (int (*callback_func) (), void *callback_param)
{
  int i, i2;
  TRACE ("EnumWindows(%p, %p) => 1\n", callback_func, callback_param);
  i = callback_func (0, callback_param);
  i2 = callback_func (1, callback_param);
  return i && i2;
}

static int WINAPI
expGetWindowThreadProcessId (HWND win, int *pid_data)
{
  int tid = (int) pthread_self ();
  TRACE ("GetWindowThreadProcessId(0x%x, %p) => %d\n", win, pid_data, tid);
  if (pid_data)
    *pid_data = tid;
  return tid;
}

//HWND      WINAPI CreateWindowExA(DWORD,LPCSTR,LPCSTR,DWORD,INT,INT,
//                                INT,INT,HWND,HMENU,HINSTANCE,LPVOID);

static HWND WINAPI
expCreateWindowExA (int exstyle, const char *classname,
    const char *winname, int style, int x, int y, int w, int h,
    HWND parent, HMENU menu, HINSTANCE inst, LPVOID param)
{
  TRACE
      ("CreateWindowEx(%d, %p = %s, %p = %s, %d, %d, %d, %d, %d, 0x%x, 0x%x, 0x%x, %p) => 1\n",
      exstyle, classname, classname, winname, winname, style, x, y, w, h,
      parent, menu, inst, param);
  return 1;
}

static int WINAPI
expwaveOutGetNumDevs (void)
{
  TRACE ("waveOutGetNumDevs() => 0\n");
  return 0;
}
#endif

/*
 * Returns the number of milliseconds, modulo 2^32, since the start
 * of the wineserver.
 */
static int WINAPI
expGetTickCount (void)
{
  static int tcstart = 0;
  struct timeval t;
  int tc;
  gettimeofday (&t, NULL);
  tc = ((t.tv_sec * 1000) + (t.tv_usec / 1000)) - tcstart;
  if (tcstart == 0) {
    tcstart = 0;
    tc = 0;
  }
  TRACE ("GetTickCount() => %d\n", tc);
  return tc;
}

static int WINAPI
expCreateFontA (void)
{
  TRACE ("CreateFontA() => 0x0\n");
  return 1;
}

/* tried to get pvmjpg work in a different way - no success */
static int WINAPI
expDrawTextA (int hDC, char *lpString, int nCount,
    LPRECT lpRect, unsigned int uFormat)
{
  TRACE ("expDrawTextA(%d,...) => 8\n", hDC);
  return 8;
}

static int WINAPI
expGetPrivateProfileIntA (const char *appname,
    const char *keyname, int default_value, const char *filename)
{
  int size = 255;
  char buffer[256];
  char *fullname;
  int result;

  buffer[255] = 0;
  if (!(appname && keyname && filename)) {
    TRACE ("GetPrivateProfileIntA('%s', '%s', %d, '%s') => %d\n", appname,
        keyname, default_value, filename, default_value);
    return default_value;
  }
  fullname =
      (char *) malloc (50 + strlen (appname) + strlen (keyname) +
      strlen (filename));
  strcpy (fullname, "Software\\IniFileMapping\\");
  strcat (fullname, appname);
  strcat (fullname, "\\");
  strcat (fullname, keyname);
  strcat (fullname, "\\");
  strcat (fullname, filename);
  result =
      RegQueryValueExA (HKEY_LOCAL_MACHINE, fullname, NULL, NULL,
      (int *) buffer, &size);
  if ((size >= 0) && (size < 256))
    buffer[size] = 0;
  //    TRACE("GetPrivateProfileIntA(%s, %s, %s) -> %s\n", appname, keyname, filename, buffer);
  free (fullname);
  if (result)
    result = default_value;
  else
    result = atoi (buffer);
  TRACE ("GetPrivateProfileIntA('%s', '%s', %d, '%s') => %d\n", appname,
      keyname, default_value, filename, result);
  return result;
}

static int WINAPI
expGetProfileIntA (const char *appname, const char *keyname, int default_value)
{
  TRACE ("GetProfileIntA -> ");
  return expGetPrivateProfileIntA (appname, keyname, default_value, "default");
}

static int WINAPI
expGetPrivateProfileStringA (const char *appname,
    const char *keyname,
    const char *def_val, char *dest, unsigned int len, const char *filename)
{
  int result;
  int size;
  char *fullname;
  TRACE
      ("GetPrivateProfileStringA('%s', '%s', def_val '%s', %p, 0x%x, '%s')",
      appname, keyname, def_val, dest, len, filename);
  if (!(appname && keyname && filename))
    return 0;
  fullname =
      (char *) malloc (50 + strlen (appname) + strlen (keyname) +
      strlen (filename));
  strcpy (fullname, "Software\\IniFileMapping\\");
  strcat (fullname, appname);
  strcat (fullname, "\\");
  strcat (fullname, keyname);
  strcat (fullname, "\\");
  strcat (fullname, filename);
  size = len;
  result =
      RegQueryValueExA (HKEY_LOCAL_MACHINE, fullname, NULL, NULL, (int *) dest,
      &size);
  free (fullname);
  if (result) {
    strncpy (dest, def_val, size);
    if (strlen (def_val) < size)
      size = strlen (def_val);
  }
  TRACE (" => %d ( '%s' )\n", size, dest);
  return size;
}

static int WINAPI
expWritePrivateProfileStringA (const char *appname,
    const char *keyname, const char *string, const char *filename)
{
  /* int size=256; */
  char *fullname;
  TRACE ("WritePrivateProfileStringA('%s', '%s', '%s', '%s')", appname,
      keyname, string, filename);
  if (!(appname && keyname && filename)) {
    TRACE (" => -1\n");
    return -1;
  }
  fullname =
      (char *) malloc (50 + strlen (appname) + strlen (keyname) +
      strlen (filename));
  strcpy (fullname, "Software\\IniFileMapping\\");
  strcat (fullname, appname);
  strcat (fullname, "\\");
  strcat (fullname, keyname);
  strcat (fullname, "\\");
  strcat (fullname, filename);
  RegSetValueExA (HKEY_LOCAL_MACHINE, fullname, 0, REG_SZ, (int *) string,
      strlen (string));
  //    TRACE("RegSetValueExA(%s,%d)\n", string, strlen(string));
  //    TRACE("WritePrivateProfileStringA(%s, %s, %s, %s)\n", appname, keyname, string, filename );
  free (fullname);
  TRACE (" => 0\n");
  return 0;
}

// FIXME: not called anywhere, can we remove or at leastmake static
#if 0
unsigned int
_GetPrivateProfileIntA (const char *appname, const char *keyname,
    INT default_value, const char *filename)
{
  return expGetPrivateProfileIntA (appname, keyname, default_value, filename);
}

int
_GetPrivateProfileStringA (const char *appname, const char *keyname,
    const char *def_val, char *dest, unsigned int len, const char *filename)
{
  return expGetPrivateProfileStringA (appname, keyname, def_val, dest, len,
      filename);
}

int
_WritePrivateProfileStringA (const char *appname, const char *keyname,
    const char *string, const char *filename)
{
  return expWritePrivateProfileStringA (appname, keyname, string, filename);
}
#endif


static int WINAPI
expDefDriverProc (int _private, int id, int msg, int arg1, int arg2)
{
  TRACE ("DefDriverProc(0x%x, 0x%x, 0x%x, 0x%x, 0x%x) => 0\n", _private, id,
      msg, arg1, arg2);
  return 0;
}

static int WINAPI
expSizeofResource (int v1, int v2)
{
  int result = SizeofResource (v1, v2);
  TRACE ("SizeofResource(0x%x, 0x%x) => %d\n", v1, v2, result);
  return result;
}

static int WINAPI
expGetLastError ()
{
  int result = GetLastError ();
  TRACE ("GetLastError() => 0x%x\n", result);
  return result;
}

static void WINAPI
expSetLastError (int error)
{
  TRACE ("SetLastError(0x%x)\n", error);
  SetLastError (error);
}

static int WINAPI
expStringFromGUID2 (GUID * guid, char *str, int cbMax)
{
  int result =
      snprintf (str, cbMax, "%.8x-%.4x-%.4x-%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x",
      guid->f1, guid->f2, guid->f3,
      (unsigned char) guid->f4[0], (unsigned char) guid->f4[1],
      (unsigned char) guid->f4[2], (unsigned char) guid->f4[3],
      (unsigned char) guid->f4[4], (unsigned char) guid->f4[5],
      (unsigned char) guid->f4[6], (unsigned char) guid->f4[7]);
  TRACE ("StringFromGUID2(%p, %p='%s', %d) => %d\n", guid, str, str, cbMax,
      result);
  return result;
}


static int WINAPI
expGetFileVersionInfoSizeA (const char *name, int *lpHandle)
{
  TRACE ("GetFileVersionInfoSizeA(%p='%s', %p) => 0\n", name, name, lpHandle);
  return 0;
}

static int WINAPI
expIsBadStringPtrW (const short *string, int nchars)
{
  int result;
  if (string == 0)
    result = 1;
  else
    result = 0;
  TRACE ("IsBadStringPtrW(%p, %d) => %d", string, nchars, result);
  if (string)
    wch_print (string);
  return result;
}

static int WINAPI
expIsBadStringPtrA (const char *string, int nchars)
{
  return expIsBadStringPtrW ((const short *) string, nchars);
}

static long WINAPI
expInterlockedExchangeAdd (long *dest, long incr)
{
  long ret;
  __asm__ __volatile__ ("lock; xaddl %0,(%1)":"=r" (ret)
      :"r" (dest), "0" (incr)
      :"memory");
  return ret;
}

static long WINAPI
expInterlockedCompareExchange (unsigned long *dest, unsigned long exchange,
    unsigned long comperand)
{
  unsigned long retval = *dest;
  if (*dest == comperand)
    *dest = exchange;
  return retval;
}

static long WINAPI
expInterlockedIncrement (long *dest)
{
  long result = expInterlockedExchangeAdd (dest, 1) + 1;
  TRACE ("InterlockedIncrement(%p => %ld) => %ld\n", dest, *dest, result);
  return result;
}

static long WINAPI
expInterlockedDecrement (long *dest)
{
  long result = expInterlockedExchangeAdd (dest, -1) - 1;
  TRACE ("InterlockedDecrement(%p => %ld) => %ld\n", dest, *dest, result);
  return result;
}

static void WINAPI
expOutputDebugStringA (const char *string)
{
  TRACE ("OutputDebugStringA(%p='%s')\n", string, string);
  fprintf (stderr, "DEBUG: %s\n", string);
}

static int WINAPI
expGetDC (int hwnd)
{
  TRACE ("GetDC(0x%x) => 1\n", hwnd);
  return 1;
}

static int WINAPI
expReleaseDC (int hwnd, int hdc)
{
  TRACE ("ReleaseDC(0x%x, 0x%x) => 1\n", hwnd, hdc);
  return 1;
}

static int WINAPI
expGetDesktopWindow ()
{
  TRACE ("GetDesktopWindow() => 0\n");
  return 0;
}

static int cursor[100] = { 0, };

static int WINAPI
expLoadCursorA (int handle, LPCSTR name)
{
  //TRACE("LoadCursorA(%d, %p='%s') => 0x%x\n", handle, name, name, (int)&cursor[0]);
  TRACE ("LoadCursorA(%d, %p) => 0x%x\n", handle, name, (int) &cursor[0]);
  return (int) &cursor[0];
}

static int WINAPI
expSetCursor (void *cursor)
{
  TRACE ("SetCursor(%p) => %p\n", cursor, cursor);
  return (int) cursor;
}

static int WINAPI
expGetCursorPos (void *cursor)
{
  TRACE ("GetCursorPos(%p) => 1\n", cursor);
  return 1;
}

#ifdef QTX
static int show_cursor = 0;
static int WINAPI
expShowCursor (int show)
{
  TRACE ("ShowCursor(%d) => %d\n", show, show);
  if (show)
    show_cursor++;
  else
    show_cursor--;
  return show_cursor;
}
#endif
static int WINAPI
expRegisterWindowMessageA (char *message)
{
  TRACE ("RegisterWindowMessageA(%s)\n", message);
  return 1;
}

static int WINAPI
expGetProcessVersion (int pid)
{
  TRACE ("GetProcessVersion(%d)\n", pid);
  return 1;
}

static int WINAPI
expGetCurrentThread (void)
{
/* #warning FIXME! -- Worry about it later :) */
  TRACE ("GetCurrentThread() => %x\n", 0xcfcf9898);
  return 0xcfcf9898;
}

static int WINAPI
expGetOEMCP (void)
{
  TRACE ("GetOEMCP()\n");
  return 1;
}

static int WINAPI
expGetCPInfo (int cp, void *info)
{
  TRACE ("GetCPInfo()\n");
  return 0;
}

#ifdef QTX
#define SM_CXSCREEN		0
#define SM_CYSCREEN		1
#define SM_XVIRTUALSCREEN	76
#define SM_YVIRTUALSCREEN	77
#define SM_CXVIRTUALSCREEN 	78
#define SM_CYVIRTUALSCREEN	79
#define SM_CMONITORS		80
#endif
static int WINAPI
expGetSystemMetrics (int index)
{
  TRACE ("GetSystemMetrics(%d)\n", index);
#ifdef QTX
  switch (index) {
    case SM_XVIRTUALSCREEN:
    case SM_YVIRTUALSCREEN:
      return 0;
    case SM_CXSCREEN:
    case SM_CXVIRTUALSCREEN:
      return PSEUDO_SCREEN_WIDTH;
    case SM_CYSCREEN:
    case SM_CYVIRTUALSCREEN:
      return PSEUDO_SCREEN_HEIGHT;
    case SM_CMONITORS:
      return 1;
  }
#endif
  return 1;
}

static int WINAPI
expGetSysColor (int index)
{
  TRACE ("GetSysColor(%d) => 1\n", index);
  return 1;
}

static int WINAPI
expGetSysColorBrush (int index)
{
  TRACE ("GetSysColorBrush(%d)\n", index);
  return 1;
}



static int WINAPI
expGetSystemPaletteEntries (int hdc, int iStartIndex, int nEntries, void *lppe)
{
  TRACE ("GetSystemPaletteEntries(0x%x, 0x%x, 0x%x, %p) => 0\n",
      hdc, iStartIndex, nEntries, lppe);
  return 0;
}

/*
 typedef struct _TIME_ZONE_INFORMATION {
 long Bias;
 char StandardName[32];
 SYSTEMTIME StandardDate;
 long StandardBias;
 char DaylightName[32];
 SYSTEMTIME DaylightDate;
 long DaylightBias;
 } TIME_ZONE_INFORMATION, *LPTIME_ZONE_INFORMATION;
 */

static int WINAPI
expGetTimeZoneInformation (LPTIME_ZONE_INFORMATION lpTimeZoneInformation)
{
  const short name[] = { 'C', 'e', 'n', 't', 'r', 'a', 'l', ' ', 'S', 't', 'a',
    'n', 'd', 'a', 'r', 'd', ' ', 'T', 'i', 'm', 'e', 0
  };
  const short pname[] = { 'C', 'e', 'n', 't', 'r', 'a', 'l', ' ', 'D', 'a', 'y',
    'l', 'i', 'g', 'h', 't', ' ', 'T', 'i', 'm', 'e', 0
  };
  TRACE ("GetTimeZoneInformation(%p) => TIME_ZONE_ID_STANDARD\n",
      lpTimeZoneInformation);
  memset (lpTimeZoneInformation, 0, sizeof (TIME_ZONE_INFORMATION));
  lpTimeZoneInformation->Bias = 360;    //GMT-6
  memcpy (lpTimeZoneInformation->StandardName, name, sizeof (name));
  lpTimeZoneInformation->StandardDate.wMonth = 10;
  lpTimeZoneInformation->StandardDate.wDay = 5;
  lpTimeZoneInformation->StandardDate.wHour = 2;
  lpTimeZoneInformation->StandardBias = 0;
  memcpy (lpTimeZoneInformation->DaylightName, pname, sizeof (pname));
  lpTimeZoneInformation->DaylightDate.wMonth = 4;
  lpTimeZoneInformation->DaylightDate.wDay = 1;
  lpTimeZoneInformation->DaylightDate.wHour = 2;
  lpTimeZoneInformation->DaylightBias = -60;
  return TIME_ZONE_ID_STANDARD;
}

static int WINAPI
expSetLocalTime (SYSTEMTIME * systime)
{
  TRACE ("SetLocalTime(%p)\n", systime);
  TRACE ("  Year: %d\n  Month: %d\n  Day of week: %d\n"
      "  Day: %d\n  Hour: %d\n  Minute: %d\n  Second:  %d\n"
      "  Milliseconds: %d\n",
      systime->wYear, systime->wMonth, systime->wDayOfWeek, systime->wDay,
      systime->wHour, systime->wMinute, systime->wSecond,
      systime->wMilliseconds);
  return 1;
}

static void WINAPI
expGetLocalTime (SYSTEMTIME * systime)
{
  time_t local_time;
  struct tm *local_tm;
  struct timeval tv;

  TRACE ("GetLocalTime(%p)\n", systime);
  gettimeofday (&tv, NULL);
  local_time = tv.tv_sec;
  local_tm = localtime (&local_time);

  systime->wYear = local_tm->tm_year + 1900;
  systime->wMonth = local_tm->tm_mon + 1;
  systime->wDayOfWeek = local_tm->tm_wday;
  systime->wDay = local_tm->tm_mday;
  systime->wHour = local_tm->tm_hour;
  systime->wMinute = local_tm->tm_min;
  systime->wSecond = local_tm->tm_sec;
  systime->wMilliseconds = (tv.tv_usec / 1000) % 1000;
  TRACE ("  Year: %d\n  Month: %d\n  Day of week: %d\n"
      "  Day: %d\n  Hour: %d\n  Minute: %d\n  Second:  %d\n"
      "  Milliseconds: %d\n",
      systime->wYear, systime->wMonth, systime->wDayOfWeek, systime->wDay,
      systime->wHour, systime->wMinute, systime->wSecond,
      systime->wMilliseconds);
}

static void WINAPI
expGetSystemTime (SYSTEMTIME * systime)
{
  time_t local_time;
  struct tm *local_tm;
  struct timeval tv;

  TRACE ("GetSystemTime(%p)\n", systime);
  gettimeofday (&tv, NULL);
  local_time = tv.tv_sec;
  local_tm = gmtime (&local_time);

  systime->wYear = local_tm->tm_year + 1900;
  systime->wMonth = local_tm->tm_mon + 1;
  systime->wDayOfWeek = local_tm->tm_wday;
  systime->wDay = local_tm->tm_mday;
  systime->wHour = local_tm->tm_hour;
  systime->wMinute = local_tm->tm_min;
  systime->wSecond = local_tm->tm_sec;
  systime->wMilliseconds = (tv.tv_usec / 1000) % 1000;
  TRACE ("  Year: %d\n  Month: %d\n  Day of week: %d\n"
      "  Day: %d\n  Hour: %d\n  Minute: %d\n  Second:  %d\n"
      "  Milliseconds: %d\n",
      systime->wYear, systime->wMonth, systime->wDayOfWeek, systime->wDay,
      systime->wHour, systime->wMinute, systime->wSecond,
      systime->wMilliseconds);
}

#define SECS_1601_TO_1970  ((369 * 365 + 89) * 86400ULL)
static void WINAPI
expGetSystemTimeAsFileTime (FILETIME * systime)
{
  /* struct tm *local_tm; -- unused */
  struct timeval tv;
  unsigned long long secs;

  TRACE ("GetSystemTime(%p)\n", systime);
  gettimeofday (&tv, NULL);
  secs = (tv.tv_sec + SECS_1601_TO_1970) * 10000000;
  secs += tv.tv_usec * 10;
  systime->dwLowDateTime = secs & 0xffffffff;
  systime->dwHighDateTime = (secs >> 32);
}

static int WINAPI
expSystemTimeToFileTime (const SYSTEMTIME * systime, LPFILETIME filetime)
{
  unsigned long long secs;

  /* FIXME: we need real seonds here */
  secs = systime->wSecond +
      systime->wMinute * 60 +
      systime->wHour * 60 * 60 + systime->wDay * 24 * 60 * 60;

  filetime->dwLowDateTime = secs & 0xffffffff;
  filetime->dwHighDateTime = (secs >> 32);
  return 1;
}

static int WINAPI
expLocalFileTimeToFileTime (const FILETIME * localfiletime, LPFILETIME filetime)
{
  /* FIXME: implement me  */
  filetime->dwLowDateTime = localfiletime->dwLowDateTime;
  filetime->dwHighDateTime = localfiletime->dwHighDateTime;
  return 1;
}

static int WINAPI
expSetFileTime (HANDLE hFile, const FILETIME * lpCreationTime,
    const FILETIME * lpLastAccessTime, const FILETIME * lpLastWriteTime)
{
  /* FIXME: implement me  */
  return 1;
}

static int WINAPI
expGetEnvironmentVariableA (const char *name, char *field, int size)
{
  /* char *p; */
  //    TRACE("%s %x %x\n", name, field, size);
  if (field)
    field[0] = 0;
  /*
     p = getenv(name);
     if (p) strncpy(field,p,size);
   */
  if (!field || size < sizeof ("__GLOBAL_HEAP_SELECTED,1"))
    return 0;
  if (strcmp (name, "__MSVCRT_HEAP_SELECT") == 0)
    strcpy (field, "__GLOBAL_HEAP_SELECTED,1");
  TRACE ("GetEnvironmentVariableA(%p='%s', %p, %d) => %d\n", name, name,
      field, size, strlen (field));
  return strlen (field);
}

static int WINAPI
expSetEnvironmentVariableA (const char *name, const char *value)
{
  TRACE ("SetEnvironmentVariableA(%s, %s)\n", name, value);
  return 0;
}

static void *WINAPI
expCoTaskMemAlloc (ULONG cb)
{
  return my_mreq (cb, 0);
}

static void WINAPI
expCoTaskMemFree (void *cb)
{
  my_release (cb);
}




void *
CoTaskMemAlloc (unsigned long cb)
{
  return expCoTaskMemAlloc (cb);
}

void
CoTaskMemFree (void *cb)
{
  expCoTaskMemFree (cb);
}

struct COM_OBJECT_INFO
{
  GUID clsid;
  long (*GetClassObject) (GUID * clsid, const GUID * iid, void **ppv);
};

static struct COM_OBJECT_INFO *com_object_table = 0;
static int com_object_size = 0;
int
RegisterComClass (const GUID * clsid, GETCLASSOBJECT gcs)
{
  if (!clsid || !gcs)
    return -1;
  com_object_table =
      realloc (com_object_table,
      sizeof (struct COM_OBJECT_INFO) * (++com_object_size));
  com_object_table[com_object_size - 1].clsid = *clsid;
  com_object_table[com_object_size - 1].GetClassObject = gcs;
  return 0;
}

int
UnregisterComClass (const GUID * clsid, GETCLASSOBJECT gcs)
{
  int found = 0;
  int i = 0;
  if (!clsid || !gcs)
    return -1;

  if (com_object_table == 0)
    TRACE
        ("Warning: UnregisterComClass() called without any registered class\n");
  while (i < com_object_size) {
    if (found && i > 0) {
      memcpy (&com_object_table[i - 1].clsid,
          &com_object_table[i].clsid, sizeof (GUID));
      com_object_table[i - 1].GetClassObject =
          com_object_table[i].GetClassObject;
    } else if (memcmp (&com_object_table[i].clsid, clsid, sizeof (GUID)) == 0
        && com_object_table[i].GetClassObject == gcs) {
      found++;
    }
    i++;
  }
  if (found) {
    if (--com_object_size == 0) {
      free (com_object_table);
      com_object_table = 0;
    }
  }
  return 0;
}


const GUID IID_IUnknown = {
  0x00000000, 0x0000, 0x0000,
  {0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}
};

const GUID IID_IClassFactory = {
  0x00000001, 0x0000, 0x0000,
  {0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}
};

static long WINAPI
expCoCreateInstance (GUID * rclsid, struct IUnknown *pUnkOuter,
    long dwClsContext, const GUID * riid, void **ppv)
{
  int i;
  struct COM_OBJECT_INFO *ci = 0;
  for (i = 0; i < com_object_size; i++)
    if (!memcmp (rclsid, &com_object_table[i].clsid, sizeof (GUID)))
      ci = &com_object_table[i];
  if (!ci)
    return REGDB_E_CLASSNOTREG;
  // in 'real' world we should mess with IClassFactory here
  i = ci->GetClassObject (rclsid, riid, ppv);
  return i;
}

long
CoCreateInstance (GUID * rclsid, struct IUnknown *pUnkOuter,
    long dwClsContext, const GUID * riid, void **ppv)
{
  return expCoCreateInstance (rclsid, pUnkOuter, dwClsContext, riid, ppv);
}

static int WINAPI
expIsRectEmpty (CONST RECT * lprc)
{
  int r = 0;
  int w, h;
//trapbug();
  if (lprc) {
    w = lprc->right - lprc->left;
    h = lprc->bottom - lprc->top;
    if (w <= 0 || h <= 0)
      r = 1;
  } else
    r = 1;

  TRACE ("IsRectEmpty(%p) => %s\n", lprc, (r) ? "TRUE" : "FALSE");
  //TRACE("Rect: left: %d, top: %d, right: %d, bottom: %d\n", lprc->left, lprc->top, lprc->right, lprc->bottom);
//    return 0; // wmv9?
  return r;                     // TM20
}

static int _adjust_fdiv = 0;    //what's this? - used to adjust division




static unsigned int WINAPI
expGetTempPathA (unsigned int len, char *path)
{
  TRACE ("GetTempPathA(%d, %p)", len, path);
  if (len < 5) {
    TRACE (" => 0\n");
    return 0;
  }
  strcpy (path, "/tmp");
  TRACE (" => 5 ( '/tmp' )\n");
  return 5;
}

/*
 FYI:
 typedef struct
 {
 DWORD     dwFileAttributes;
 FILETIME  ftCreationTime;
 FILETIME  ftLastAccessTime;
 FILETIME  ftLastWriteTime;
 DWORD     nFileSizeHigh;
 DWORD     nFileSizeLow;
 DWORD     dwReserved0;
 DWORD     dwReserved1;
 CHAR      cFileName[260];
 CHAR      cAlternateFileName[14];
 } WIN32_FIND_DATAA, *LPWIN32_FIND_DATAA;
 */

static DIR *qtx_dir = NULL;

static WIN_BOOL WINAPI
expFindNextFileA (HANDLE h, LPWIN32_FIND_DATAA lpfd)
{
#ifdef QTX
  TRACE ("FindNextFileA(0x%x, %p) => 0\n", h, lpfd);
  if (h == FILE_HANDLE_quicktimeqtx) {
    struct dirent *d;
    if (!qtx_dir)
      return 0;
    while ((d = readdir (qtx_dir))) {
      char *x = strrchr (d->d_name, '.');
      if (!x)
        continue;
      if (strcmp (x, ".qtx"))
        continue;
      strcpy (lpfd->cFileName, d->d_name);
      strcpy (lpfd->cAlternateFileName, "foobar.qtx");
      TRACE ("### FindNext: %s\n", lpfd->cFileName);
      return 1;
    }
    closedir (qtx_dir);
    qtx_dir = NULL;
    return 0;
  }
#endif
  return 0;
}

static HANDLE WINAPI
expFindFirstFileA (LPCSTR s, LPWIN32_FIND_DATAA lpfd)
{
#ifdef QTX
  if (strstr (s, "*.QTX")) {
    TRACE ("FindFirstFileA(%p='%s', %p) => QTX\n", s, s, lpfd);
    qtx_dir = opendir (win32_def_path);
    if (!qtx_dir)
      return (HANDLE) - 1;
    memset (lpfd, 0, sizeof (*lpfd));
    if (expFindNextFileA (FILE_HANDLE_quicktimeqtx, lpfd))
      return FILE_HANDLE_quicktimeqtx;
    TRACE
        ("loader: Couldn't find the QuickTime plugins (.qtx files) at %s\n",
        win32_def_path);
    return (HANDLE) - 1;
  }
  if (strstr (s, "QuickTime.qts")) {
    TRACE ("FindFirstFileA(%p='%s', %p) => QTS\n", s, s, lpfd);
//      if(!strcmp(s,"C:\\windows\\QuickTime.qts\\QuickTime.qts\\*.QTX"))
//          return (HANDLE)-1;
    strcpy (lpfd->cFileName, "QuickTime.qts");
    strcpy (lpfd->cAlternateFileName, "QuickT~1.qts");
    return FILE_HANDLE_quicktimeqts;
  }
#endif
  if (strstr (s, "*.vwp")) {
    // hack for VoxWare codec plugins:
    TRACE ("FindFirstFileA(%p='%s', %p) => 0\n", s, s, lpfd);
    strcpy (lpfd->cFileName, "msms001.vwp");
    strcpy (lpfd->cAlternateFileName, "msms001.vwp");
    return (HANDLE) 0;
  }
  return (HANDLE) - 1;
}

static WIN_BOOL WINAPI
expFindClose (HANDLE h)
{
  TRACE ("FindClose(0x%x) => 0\n", h);
#ifdef QTX
//    if(h==FILE_HANDLE_quicktimeqtx && qtx_dir){
//      closedir(qtx_dir);
//      qtx_dir=NULL;
//    }
#endif
  return 0;
}

static UINT WINAPI
expSetErrorMode (UINT i)
{
  TRACE ("SetErrorMode(%d) => 0\n", i);
  return 0;
}

static UINT WINAPI
expGetWindowsDirectoryA (LPSTR s, UINT c)
{
  char windir[] = "c:\\windows";
  int result;
  strncpy (s, windir, c);
  result = 1 + ((c < strlen (windir)) ? c : strlen (windir));
  TRACE ("GetWindowsDirectoryA(%p, %d) => %d\n", s, c, result);
  return result;
}

#ifdef QTX
static UINT WINAPI
expGetCurrentDirectoryA (UINT c, LPSTR s)
{
  char curdir[] = "c:\\";
  int result;
  strncpy (s, curdir, c);
  result = 1 + ((c < strlen (curdir)) ? c : strlen (curdir));
  TRACE ("GetCurrentDirectoryA(%p, %d) => %d\n", s, c, result);
  return result;
}

static int WINAPI
expSetCurrentDirectoryA (const char *pathname)
{
  TRACE ("SetCurrentDirectoryA(%p = %s) => 1\n", pathname, pathname);
#if 0
  if (strrchr (pathname, '\\'))
    chdir (strcat (strrchr (pathname, '\\') + 1, '/'));
  else
    chdir (pathname);
#endif
  return 1;
}

static int WINAPI
expCreateDirectoryA (const char *pathname, void *sa)
{
  TRACE ("CreateDirectory(%p = %s, %p) => 1\n", pathname, pathname, sa);
#if 0
  p = strrchr (pathname, '\\') + 1;
  strcpy (&buf[0], p);          /* should be strncpy */
  if (!strlen (p)) {
    buf[0] = '.';
    buf[1] = 0;
  }
#if 0
  if (strrchr (pathname, '\\'))
    mkdir (strcat (strrchr (pathname, '\\') + 1, '/'), 666);
  else
    mkdir (pathname, 666);
#endif
  mkdir (&buf);
#endif
  return 1;
}
#endif
static WIN_BOOL WINAPI
expDeleteFileA (LPCSTR s)
{
  TRACE ("DeleteFileA(%p='%s') => 0\n", s, s);
  return 0;
}

static WIN_BOOL WINAPI
expFileTimeToLocalFileTime (const FILETIME * cpf, LPFILETIME pf)
{
  TRACE ("FileTimeToLocalFileTime(%p, %p) => 0\n", cpf, pf);
  return 0;
}

static UINT WINAPI
expGetTempFileNameA (LPCSTR cs1, LPCSTR cs2, UINT i, LPSTR ps)
{
  char mask[16] = "/tmp/AP_XXXXXX";
  int result;
  TRACE ("GetTempFileNameA(%p='%s', %p='%s', %d, %p)", cs1, cs1, cs2, cs2,
      i, ps);
  if (i && i < 10) {
    TRACE (" => -1\n");
    return -1;
  }
  result = mkstemp (mask);
  sprintf (ps, "AP%d", result);
  TRACE (" => %d\n", strlen (ps));
  return strlen (ps);
}

//
// This func might need proper implementation if we want AngelPotion codec.
// They try to open APmpeg4v1.apl with it.
// DLL will close opened file with CloseHandle().
//
static HANDLE WINAPI
expCreateFileA (LPCSTR cs1, DWORD i1, DWORD i2,
    LPSECURITY_ATTRIBUTES p1, DWORD i3, DWORD i4, HANDLE i5)
{
  TRACE ("CreateFileA(%p='%s', %ld, %ld, %p, %ld, %ld, 0x%x)\n", cs1, cs1,
      i1, i2, p1, i3, i4, i5);
  if ((!cs1) || (strlen (cs1) < 2))
    return -1;

#ifdef QTX
  if (strstr (cs1, "QuickTime.qts")) {
    int result;
	int len = strlen (win32_def_path) + 50;
    char *tmp = (char *) malloc (len);
    strcpy (tmp, win32_def_path);
    strcat (tmp, "/");
    strcat (tmp, "QuickTime.qts");
    result = open (tmp, O_RDONLY);
    free (tmp);
    return result;
  }
  if (strstr (cs1, ".qtx")) {
    int result;
    char *tmp = (char *) malloc (strlen (win32_def_path) + 250);
    char *x = strrchr (cs1, '\\');
    snprintf (tmp, len, "%s/%s", win32_def_path, x ? (x + 1) : cs1);
//      TRACE("### Open: %s -> %s\n",cs1,tmp);
    result = open (tmp, O_RDONLY);
    free (tmp);
    return result;
  }
#endif

  if (strncmp (cs1, "AP", 2) == 0) {
    int result;
    char *tmp = (char *) malloc (strlen (win32_def_path) + 50);
    strcpy (tmp, win32_def_path);
    strcat (tmp, "/");
    strcat (tmp, "APmpg4v1.apl");
    result = open (tmp, O_RDONLY);
    free (tmp);
    return result;
  }
  if (strstr (cs1, "vp3")) {
    int r;
    int flg = 0;
    char *tmp = (char *) malloc (20 + strlen (cs1));
    strcpy (tmp, "/tmp/");
    strcat (tmp, cs1);
    r = 4;
    while (tmp[r]) {
      if (tmp[r] == ':' || tmp[r] == '\\')
        tmp[r] = '_';
      r++;
    }
    if (GENERIC_READ & i1)
      flg |= O_RDONLY;
    else if (GENERIC_WRITE & i1) {
      flg |= O_WRONLY;
      TRACE ("Warning: openning filename %s  %d (flags; 0x%x) for write\n",
          tmp, r, flg);
    }
    r = open (tmp, flg);
    free (tmp);
    return r;
  }
//#if 0
  /* we need this for some virtualdub filters */
  {
    int r;
    int flg = 0;
    if (GENERIC_READ & i1)
      flg |= O_RDONLY;
    else if (GENERIC_WRITE & i1) {
      flg |= O_WRONLY;
      TRACE ("Warning: openning filename %s  (flags; 0x%x) for write\n",
          cs1, flg);
    }
    r = open (cs1, flg);
    return r;
  }
//#else

//  return atoi(cs1+2);
//#endif
}

static HANDLE WINAPI
expCreateFileW (LPCWSTR filename, DWORD access, DWORD sharing,
    LPSECURITY_ATTRIBUTES sa, DWORD creation, DWORD attributes, HANDLE template)
{
  // mbstowcs, MultiByteToWideChar
  return expCreateFileA ((LPCSTR) filename, access, sharing, sa, creation,
      attributes, template);
}

static int WINAPI
expCreatePipe (PHANDLE hReadPipe, PHANDLE hWritePipe,
    LPSECURITY_ATTRIBUTES lpPipeAttributes, DWORD nSize)
{
  /* FIXME: implement me */
  return 1;
}

static int WINAPI
expLockFile (HANDLE hFile, DWORD dwFileOffsetLow, DWORD dwFileOffsetHigh,
    DWORD nNumberOfBytesToUnlockLow, DWORD nNumberOfBytesToUnlockHigh)
{
  /* FIXME: implement me */
  return 1;
}

static int WINAPI
expUnlockFile (HANDLE hFile, DWORD dwFileOffsetLow, DWORD dwFileOffsetHigh,
    DWORD nNumberOfBytesToUnlockLow, DWORD nNumberOfBytesToUnlockHigh)
{
  /* FIXME: implement me */
  return 1;
}

static int WINAPI
expGetNumberOfConsoleInputEvents (HANDLE hConsoleInput,
    LPDWORD lpcNumberOfEvents)
{
  /* FIXME: implement me */
  *lpcNumberOfEvents = 0;
  return 1;
}

static int WINAPI
expPeekConsoleInputA (HANDLE hConsoleInput, /*PINPUT_RECORD */ void *lpBuffer,
    DWORD nLength, LPDWORD lpNumberOfEventsRead)
{
  /* FIXME: implement me */
  *lpNumberOfEventsRead = 0;
  return 1;
}

static int WINAPI
expReadConsoleInputA (HANDLE handle, /*PINPUT_RECORD */ void *buffer,
    DWORD count, LPDWORD pRead)
{
  /* FIXME: implement me */
  return 0;
}

static int WINAPI
expPeekNamedPipe (HANDLE hNamedPipe, LPVOID lpBuffer, DWORD nBufferSize,
    LPDWORD lpBytesRead, LPDWORD lpTotalBytesAvail,
    LPDWORD lpBytesLeftThisMessage)
{
  /* FIXME: implement me */
  return 0;
}

static int WINAPI
expGetFileInformationByHandle (HANDLE hFile,    /*LPBY_HANDLE_FILE_INFORMATION */
    void *lpFileInformation)
{
  /* FIXME: implement me */
  return 0;
}

static int WINAPI
expWriteConsoleA (HANDLE hConsoleOutput, const VOID * lpBuffer,
    DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten,
    LPVOID lpReserved)
{
  *lpNumberOfCharsWritten = nNumberOfCharsToWrite;
  return 1;
}

static int WINAPI
expSetEndOfFile (HANDLE hFile)
{
  /* FIXME: implement me */
  return 1;
}

/*
GetConsoleMode

*/

static UINT WINAPI
expGetSystemDirectoryA (char *lpBuffer, // address of buffer for system directory
    UINT uSize                  // size of directory buffer
    )
{
  TRACE ("GetSystemDirectoryA(%p,%d)\n", lpBuffer, uSize);
  if (!lpBuffer || uSize < 2)
    return 0;
  strcpy (lpBuffer, ".");
  return 1;
}

/*
static char sysdir[]=".";
static LPCSTR WINAPI expGetSystemDirectoryA()
{
    TRACE("GetSystemDirectoryA() => 0x%x='%s'\n", sysdir, sysdir);
    return sysdir;
}
*/
static DWORD WINAPI expGetFullPathNameA
    (LPCTSTR lpFileName,
    DWORD nBufferLength, LPTSTR lpBuffer, LPTSTR lpFilePart)
{
  if (!lpFileName)
    return 0;
  TRACE ("GetFullPathNameA('%s',%ld,%p,%p)\n", lpFileName, nBufferLength,
      lpBuffer, lpFilePart);
#if 0
#ifdef QTX
  strcpy (lpFilePart, "Quick123.qts");
#else
  strcpy (lpFilePart, lpFileName);
#endif
#else
  if (strrchr (lpFileName, '\\'))
    *lpFilePart = (int) strrchr (lpFileName, '\\');
  else
    *lpFilePart = (int) lpFileName;
#endif
  strcpy (lpBuffer, lpFileName);
//    strncpy(lpBuffer, lpFileName, rindex(lpFileName, '\\')-lpFileName);
  return strlen (lpBuffer);
}

static DWORD WINAPI expGetShortPathNameA
    (LPCSTR longpath, LPSTR shortpath, DWORD shortlen)
{
  if (!longpath)
    return 0;
  TRACE ("GetShortPathNameA('%s',%p,%ld)\n", longpath, shortpath, shortlen);
  strcpy (shortpath, longpath);
  return strlen (shortpath);
}

static WIN_BOOL WINAPI
expReadFile (HANDLE h, LPVOID pv, DWORD size, LPDWORD rd, LPOVERLAPPED unused)
{
  int result;
  TRACE ("ReadFile(%d, %p, %ld -> %p)\n", h, pv, size, rd);
  result = read (h, pv, size);
  if (rd)
    *rd = result;
  if (!result)
    return 0;
  return 1;
}

static WIN_BOOL WINAPI
expWriteFile (HANDLE h, LPCVOID pv, DWORD size, LPDWORD wr, LPOVERLAPPED unused)
{
  int result;
  TRACE ("WriteFile(%d, %p, %ld -> %p)\n", h, pv, size, wr);
  if (h == 1234)
    h = 1;
  result = write (h, pv, size);
  if (wr)
    *wr = result;
  if (!result)
    return 0;
  return 1;
}

static DWORD WINAPI
expSetFilePointer (HANDLE h, LONG val, LPLONG ext, DWORD whence)
{
  int wh;
  TRACE ("SetFilePointer(%d, 0x%lx, %p = %ld, %ld)\n", h, val, ext,
      ext ? *ext : -1, whence);
  //why would DLL want temporary file with >2Gb size?
  switch (whence) {
    case FILE_BEGIN:
      wh = SEEK_SET;
      break;
    case FILE_END:
      wh = SEEK_END;
      break;
    case FILE_CURRENT:
      wh = SEEK_CUR;
      break;
    default:
      return -1;
  }
#ifdef QTX
  if (val == 0 && ext != 0)
    val = val & (*ext);
#endif
  return lseek (h, val, wh);
}

static HDRVR WINAPI
expOpenDriverA (LPCSTR szDriverName, LPCSTR szSectionName, LPARAM lParam2)
{
  TRACE ("OpenDriverA(%p='%s', %p='%s', 0x%lx) => -1\n", szDriverName,
      szDriverName, szSectionName, szSectionName, lParam2);
  return -1;
}

static HDRVR WINAPI
expOpenDriver (LPCSTR szDriverName, LPCSTR szSectionName, LPARAM lParam2)
{
  TRACE ("OpenDriver(%p='%s', %p='%s', 0x%lx) => -1\n", szDriverName,
      szDriverName, szSectionName, szSectionName, lParam2);
  return -1;
}


static WIN_BOOL WINAPI
expGetProcessAffinityMask (HANDLE hProcess,
    LPDWORD lpProcessAffinityMask, LPDWORD lpSystemAffinityMask)
{
  TRACE ("GetProcessAffinityMask(0x%x, %p, %p) => 1\n",
      hProcess, lpProcessAffinityMask, lpSystemAffinityMask);
  if (lpProcessAffinityMask)
    *lpProcessAffinityMask = 1;
  if (lpSystemAffinityMask)
    *lpSystemAffinityMask = 1;
  return 1;
}

static int WINAPI
expMulDiv (int nNumber, int nNumerator, int nDenominator)
{
  static const long long max_int = 0x7FFFFFFFLL;
  static const long long min_int = -0x80000000LL;
  long long tmp = (long long) nNumber * (long long) nNumerator;
  TRACE ("expMulDiv %d * %d / %d\n", nNumber, nNumerator, nDenominator);
  if (!nDenominator)
    return 1;
  tmp /= nDenominator;
  if (tmp < min_int)
    return 1;
  if (tmp > max_int)
    return 1;
  return (int) tmp;
}

static LONG WINAPI
explstrcmpiA (const char *str1, const char *str2)
{
  LONG result = strcasecmp (str1, str2);
  TRACE ("strcmpi(%p='%s', %p='%s') => %ld\n", str1, str1, str2, str2, result);
  return result;
}

static LONG WINAPI
explstrlenA (const char *str1)
{
  LONG result = strlen (str1);
  TRACE ("strlen(%p='%.50s') => %ld\n", str1, str1, result);
  return result;
}

static LONG WINAPI
explstrcpyA (char *str1, const char *str2)
{
  int result = (int) strcpy (str1, str2);
  TRACE ("strcpy(%p, %p='%.50s') => %d\n", str1, str2, str2, result);
  return result;
}

static LONG WINAPI
explstrcpynA (char *str1, const char *str2, int len)
{
  int result;
  if (strlen (str2) > len)
    result = (int) strncpy (str1, str2, len);
  else
    result = (int) strcpy (str1, str2);
  TRACE ("strncpy(%p, %p='%s' len %d strlen %d) => %x\n", str1, str2, str2,
      len, strlen (str2), result);
  return result;
}

static LONG WINAPI
explstrcatA (char *str1, const char *str2)
{
  int result = (int) strcat (str1, str2);
  TRACE ("strcat(%p, %p='%s') => %d\n", str1, str2, str2, result);
  return result;
}


static LONG WINAPI
expInterlockedExchange (long *dest, long l)
{
  long retval = *dest;
  *dest = l;
  return retval;
}

static void WINAPI
expInitCommonControls (void)
{
  TRACE ("InitCommonControls called!\n");
  return;
}

#ifdef QTX
/* needed by QuickTime.qts */
static HWND WINAPI
expCreateUpDownControl (DWORD style, INT x, INT y, INT cx, INT cy,
    HWND parent, INT id, HINSTANCE inst,
    HWND buddy, INT maxVal, INT minVal, INT curVal)
{
  TRACE ("CreateUpDownControl(...)\n");
  return 0;
}
#endif

/* alex: implement this call! needed for 3ivx */
static HRESULT WINAPI
expCoCreateFreeThreadedMarshaler (void *pUnkOuter, void **ppUnkInner)
{
  TRACE ("CoCreateFreeThreadedMarshaler(%p, %p) called!\n",
      pUnkOuter, ppUnkInner);
//    return 0;
  return ERROR_CALL_NOT_IMPLEMENTED;
}


static int WINAPI
expDuplicateHandle (HANDLE hSourceProcessHandle,        // handle to source process
    HANDLE hSourceHandle,       // handle to duplicate
    HANDLE hTargetProcessHandle,        // handle to target process
    HANDLE * lpTargetHandle,    // duplicate handle
    DWORD dwDesiredAccess,      // requested access
    int bInheritHandle,         // handle inheritance option
    DWORD dwOptions             // optional actions
    )
{
  TRACE ("DuplicateHandle(0x%x, 0x%x, 0x%x, %p, 0x%lx, %d, %ld) called\n",
      hSourceProcessHandle, hSourceHandle, hTargetProcessHandle,
      lpTargetHandle, dwDesiredAccess, bInheritHandle, dwOptions);
  *lpTargetHandle = hSourceHandle;
  return 1;
}

// required by PIM1 codec (used by win98 PCTV Studio capture sw)
static HRESULT WINAPI
expCoInitialize (LPVOID lpReserved      /* [in] pointer to win32 malloc interface
                                           (obsolete, should be NULL) */
    )
{
  /*
   * Just delegate to the newer method.
   */
  return 0;                     //CoInitializeEx(lpReserved, COINIT_APARTMENTTHREADED);
}

static DWORD WINAPI expSetThreadAffinityMask
    (HANDLE hThread, DWORD dwThreadAffinityMask)
{
  return 0;
};

/*
 * no WINAPI functions - CDECL
 */
static void *
expmalloc (int size)
{
  //TRACE("malloc");
  //    return malloc(size);
  void *result = my_mreq (size, 0);
  TRACE ("malloc(0x%x) => %p\n", size, result);
  if (result == 0)
    TRACE ("WARNING: malloc() failed\n");
  return result;
}

static void
expfree (void *mem)
{
  //    return free(mem);
  TRACE ("free(%p)\n", mem);
  my_release (mem);
}

/* needed by atrac3.acm */
static void *
expcalloc (int num, int size)
{
  void *result = my_mreq (num * size, 1);
  TRACE ("calloc(%d,%d) => %p\n", num, size, result);
  if (result == 0)
    TRACE ("WARNING: calloc() failed\n");
  return result;
}

static void *
expnew (int size)
{
  //    TRACE("NEW:: Call from address %08x\n STACK DUMP:\n", *(-1+(int*)&size));
  //    TRACE("%08x %08x %08x %08x\n",
  //    size, *(1+(int*)&size),
  //    *(2+(int*)&size),*(3+(int*)&size));
  void *result;
  assert (size >= 0);

  //result=my_mreq(size,0);
  result = my_mreq (size, 1);
  TRACE ("new(%d) => %p\n", size, result);
  if (result == 0)
    TRACE ("WARNING: new() failed\n");
  return result;

}

static int
expdelete (void *memory)
{
  TRACE ("delete(%p)\n", memory);
  my_release (memory);
  return 0;
}

/* FIXME: its not accepting this  */
static void *
exp_malloc_dbg (int size, int type, char *file, int line)
{
  return my_mreq (size, 0);
}


/*
 * local definition - we need only the last two members at this point
 * otherwice we would have to introduce here GUIDs and some more types..
 */
typedef struct __attribute__ ((__packed__)) {
  char hay[0x40];
  unsigned long cbFormat;       //0x40
  char *pbFormat;               //0x44
} MY_MEDIA_TYPE;

static HRESULT WINAPI
expMoCopyMediaType (MY_MEDIA_TYPE * dest, const MY_MEDIA_TYPE * src)
{
  if (!dest || !src)
    return E_POINTER;
  memcpy (dest, src, sizeof (MY_MEDIA_TYPE));
  if (dest->cbFormat) {
    dest->pbFormat = (char *) my_mreq (dest->cbFormat, 0);
    if (!dest->pbFormat)
      return E_OUTOFMEMORY;
    memcpy (dest->pbFormat, src->pbFormat, dest->cbFormat);
  }
  return S_OK;
}

static HRESULT WINAPI
expMoInitMediaType (MY_MEDIA_TYPE * dest, DWORD cbFormat)
{
  if (!dest)
    return E_POINTER;
  memset (dest, 0, sizeof (MY_MEDIA_TYPE));
  if (cbFormat) {
    dest->pbFormat = (char *) my_mreq (cbFormat, 0);
    if (!dest->pbFormat)
      return E_OUTOFMEMORY;
  }
  return S_OK;
}

static HRESULT WINAPI
expMoCreateMediaType (MY_MEDIA_TYPE ** dest, DWORD cbFormat)
{
  if (!dest)
    return E_POINTER;
  *dest = my_mreq (sizeof (MY_MEDIA_TYPE), 0);
  return expMoInitMediaType (*dest, cbFormat);
}

static HRESULT WINAPI
expMoDuplicateMediaType (MY_MEDIA_TYPE ** dest, const void *src)
{
  if (!dest)
    return E_POINTER;
  *dest = my_mreq (sizeof (MY_MEDIA_TYPE), 0);
  return expMoCopyMediaType (*dest, src);
}

static HRESULT WINAPI
expMoFreeMediaType (MY_MEDIA_TYPE * dest)
{
  if (!dest)
    return E_POINTER;
  if (dest->pbFormat) {
    my_release (dest->pbFormat);
    dest->pbFormat = 0;
    dest->cbFormat = 0;
  }
  return S_OK;
}

static HRESULT WINAPI
expMoDeleteMediaType (MY_MEDIA_TYPE * dest)
{
  if (!dest)
    return E_POINTER;
  expMoFreeMediaType (dest);
  my_release (dest);
  return S_OK;
}


#if 0
static int
exp_initterm (int v1, int v2)
{
  TRACE ("_initterm(0x%x, 0x%x) => 0\n", v1, v2);
  return 0;
}
#else
/* merged from wine - 2002.04.21 */
typedef void (*_INITTERMFUNC) ();
static int
exp_initterm (_INITTERMFUNC * start, _INITTERMFUNC * end)
{
  TRACE ("_initterm(%p, %p) %p\n", start, end, *start);
  while (start < end) {
    if (*start) {
      //TRACE("call _initfunc: from: %p %d\n", *start);
      // ok this trick with push/pop is necessary as otherwice
      // edi/esi registers are being trashed
      void *p = *start;
      __asm__ __volatile__
          ("pushl %%ebx		\n\t"
          "pushl %%ecx		\n\t"
          "pushl %%edx		\n\t"
          "pushl %%edi		\n\t"
          "pushl %%esi		\n\t"
          "call  *%%eax		\n\t"
          "popl  %%esi		\n\t"
          "popl  %%edi		\n\t"
          "popl  %%edx		\n\t"
          "popl  %%ecx		\n\t" "popl  %%ebx		\n\t"::"a" (p)
          :"memory");
      //TRACE("done  %p  %d:%d\n", end);
    }
    start++;
  }
  return 0;
}
#endif

static void *
exp__dllonexit ()
{
  // FIXME extract from WINE
  return NULL;
}

static int
    __attribute__ ((__format__ (__printf__, 2, 3))) expwsprintfA (char *string,
    char *format, ...)
{
  va_list va;
  int result;
  va_start (va, format);
  result = vsprintf (string, format, va);
  TRACE ("wsprintfA(%p, '%s', ...) => %d\n", string, format, result);
  va_end (va);
  return result;
}

static int
    __attribute__ ((__format__ (__printf__, 2, 3))) expsprintf (char *str,
    const char *format, ...)
{
  va_list args;
  int r;
  TRACE ("sprintf(%s, %s)\n", str, format);
  va_start (args, format);
  r = vsprintf (str, format, args);
  va_end (args);
  return r;
}

static int
    __attribute__ ((__format__ (__printf__, 2, 3))) expsscanf (const char *str,
    const char *format, ...)
{
  va_list args;
  int r;
  TRACE ("sscanf(%s, %s)\n", str, format);
  va_start (args, format);
  r = vsscanf (str, format, args);
  va_end (args);
  return r;
}

static void *
expfopen (const char *path, const char *mode)
{
  TRACE ("fopen: \"%s\"  mode:%s\n", path, mode);
  //return fopen(path, mode);
  return fdopen (0, mode);      // everything on screen
}

static int
    __attribute__ ((__format__ (__printf__, 2, 3))) expfprintf (void *stream,
    const char *format, ...)
{
  va_list args;
  int r = 0;
  TRACE ("fprintf(%p, %s, ...)\n", stream, format);
#if 1
  va_start (args, format);
  r = vfprintf ((FILE *) stream, format, args);
  va_end (args);
#endif
  return r;
}

static int
    __attribute__ ((__format__ (__printf__, 1,
            2))) expprintf (const char *format, ...)
{
  va_list args;
  int r;
  TRACE ("printf(%s, ...)\n", format);
  va_start (args, format);
  r = vprintf (format, args);
  va_end (args);
  return r;
}

static char *
expgetenv (const char *varname)
{
  char *v = getenv (varname);
  TRACE ("getenv(%s) => %s\n", varname, v);
  return v;
}

static void *
expwcscpy (WCHAR * dst, const WCHAR * src)
{
  WCHAR *p = dst;
  while ((*p++ = *src++));
  return dst;
}

static char *
expstrrchr (char *string, int value)
{
  char *result = strrchr (string, value);
  if (result)
    TRACE ("strrchr(%p='%s', %d) => %p='%s'", string, string, value, result,
        result);
  else
    TRACE ("strrchr(%p='%s', %d) => 0", string, string, value);
  return result;
}

static char *
expstrchr (char *string, int value)
{
  char *result = strchr (string, value);
  if (result)
    TRACE ("strchr(%p='%s', %d) => %p='%s'", string, string, value, result,
        result);
  else
    TRACE ("strchr(%p='%s', %d) => 0", string, string, value);
  return result;
}

static int
expstrlen (char *str)
{
  int result = strlen (str);
  TRACE ("strlen(%p='%s') => %d\n", str, str, result);
  return result;
}

static char *
expstrcpy (char *str1, const char *str2)
{
  char *result = strcpy (str1, str2);
  TRACE ("strcpy(%p, %p='%s') => %p\n", str1, str2, str2, result);
  return result;
}

static char *
expstrncpy (char *str1, const char *str2, int x)
{
  char *result = strncpy (str1, str2, x);
  TRACE ("strncpy(%p, %p='%s', %d) => %p\n", str1, str2, str2, x, result);
  return result;
}

static int
expstrcmp (const char *str1, const char *str2)
{
  int result = strcmp (str1, str2);
  TRACE ("strcmp(%p='%s', %p='%s') => %d\n", str1, str1, str2, str2, result);
  return result;
}

static int
expstrncmp (const char *str1, const char *str2, int x)
{
  int result = strncmp (str1, str2, x);
  TRACE ("strcmp(%p='%s', %p='%s') => %d\n", str1, str1, str2, str2, result);
  return result;
}

static char *
expstrcat (char *str1, const char *str2)
{
  char *result = strcat (str1, str2);
  TRACE ("strcat(%p='%s', %p='%s') => %p\n", str1, str1, str2, str2, result);
  return result;
}

static char *
exp_strdup (const char *str1)
{
  int l = strlen (str1);
  char *result = (char *) my_mreq (l + 1, 0);
  if (result)
    strcpy (result, str1);
  TRACE ("_strdup(%p='%s') => %p\n", str1, str1, result);
  return result;
}

static int
expisalnum (int c)
{
  int result = (int) isalnum (c);
  TRACE ("isalnum(0x%x='%c' => %d\n", c, c, result);
  return result;
}

static int
expisspace (int c)
{
  int result = (int) isspace (c);
  TRACE ("isspace(0x%x='%c' => %d\n", c, c, result);
  return result;
}

static int
expisalpha (int c)
{
  int result = (int) isalpha (c);
  TRACE ("isalpha(0x%x='%c' => %d\n", c, c, result);
  return result;
}

static int
expisdigit (int c)
{
  int result = (int) isdigit (c);
  TRACE ("isdigit(0x%x='%c' => %d\n", c, c, result);
  return result;
}

static void *
expmemmove (void *dest, void *src, int n)
{
  void *result = memmove (dest, src, n);
  TRACE ("memmove(%p, %p, %d) => %p\n", dest, src, n, result);
  return result;
}

static int
expmemcmp (void *dest, void *src, int n)
{
  int result = memcmp (dest, src, n);
  TRACE ("memcmp(%p, %p, %d) => %d\n", dest, src, n, result);
  return result;
}

static void *
expmemcpy (void *dest, void *src, int n)
{
  void *result = memcpy (dest, src, n);
  TRACE ("memcpy(%p, %p, %d) => %p\n", dest, src, n, result);
  return result;
}

static void *
expmemset (void *dest, int c, size_t n)
{
  void *result = memset (dest, c, n);
  TRACE ("memset(%p, %d, %d) => %p\n", dest, c, n, result);
  return result;
}

static time_t
exptime (time_t * t)
{
  time_t result = time (t);
  TRACE ("time(%p) => %ld\n", t, result);
  return result;
}


unsigned int
exp_control87 (unsigned int newval, unsigned int mask)
{
#if 0
  //#include <fpu_control.h>
  fpu_control_t cw;

  TRACE ("_control87(%u,%u)\n", newval, mask);

  _FPU_GETCW (cw);
  //cw&=~_FPU_EXTENDED;
  //cw|=_FPU_DOUBLE;
  _FPU_SETCW (cw);
#endif

#if defined(__GNUC__) && defined(__i386__)
  unsigned int fpword = 0;
  unsigned int flags = 0;


  TRACE ("(%08x, %08x): Called\n", newval, mask);

  /* Get fp control word */
  __asm__ __volatile__ ("fstcw %0":"=m" (fpword):);

  TRACE ("Control word before : %08x\n", fpword);

  /* Convert into mask constants */

#define _MCW_EM         0x0008001f      /* interrupt Exception Masks */
#define _EM_INEXACT     0x00000001      /*   inexact (precision) */
#define _EM_UNDERFLOW   0x00000002      /*   underflow */
#define _EM_OVERFLOW    0x00000004      /*   overflow */
#define _EM_ZERODIVIDE  0x00000008      /*   zero divide */
#define _EM_INVALID     0x00000010      /*   invalid */
#define _EM_DENORMAL    0x00080000      /* denormal exception mask (_control87 only) */
  if (fpword & 0x1)
    flags |= _EM_INVALID;
  if (fpword & 0x2)
    flags |= _EM_DENORMAL;
  if (fpword & 0x4)
    flags |= _EM_ZERODIVIDE;
  if (fpword & 0x8)
    flags |= _EM_OVERFLOW;
  if (fpword & 0x10)
    flags |= _EM_UNDERFLOW;
  if (fpword & 0x20)
    flags |= _EM_INEXACT;

#define _MCW_RC         0x00000300      /* Rounding Control */
#define _RC_NEAR        0x00000000      /*   near */
#define _RC_DOWN        0x00000100      /*   down */
#define _RC_UP          0x00000200      /*   up */
#define _RC_CHOP        0x00000300      /*   chop */
  switch (fpword & 0xC00) {
    case 0xC00:
      flags |= _RC_UP | _RC_DOWN;
      break;
    case 0x800:
      flags |= _RC_UP;
      break;
    case 0x400:
      flags |= _RC_DOWN;
      break;
  }

#define _MCW_PC         0x00030000      /* Precision Control */
#define _PC_64          0x00000000      /*    64 bits */
#define _PC_53          0x00010000      /*    53 bits */
#define _PC_24          0x00020000      /*    24 bits */
  switch (fpword & 0x300) {
    case 0x0:
      flags |= _PC_24;
      break;
    case 0x200:
      flags |= _PC_53;
      break;
    case 0x300:
      flags |= _PC_64;
      break;
  }

#define _MCW_IC         0x00040000      /* Infinity Control */
#define _IC_AFFINE      0x00040000      /*   affine */
#define _IC_PROJECTIVE  0x00000000      /*   projective */
  if (fpword & 0x1000)
    flags |= _IC_AFFINE;

  /* Mask with parameters */
  flags = (flags & ~mask) | (newval & mask);

  /* Convert (masked) value back to fp word */
  fpword = 0;
  if (flags & _EM_INVALID)
    fpword |= 0x1;
  if (flags & _EM_DENORMAL)
    fpword |= 0x2;
  if (flags & _EM_ZERODIVIDE)
    fpword |= 0x4;
  if (flags & _EM_OVERFLOW)
    fpword |= 0x8;
  if (flags & _EM_UNDERFLOW)
    fpword |= 0x10;
  if (flags & _EM_INEXACT)
    fpword |= 0x20;
  switch (flags & (_RC_UP | _RC_DOWN)) {
    case _RC_UP | _RC_DOWN:
      fpword |= 0xC00;
      break;
    case _RC_UP:
      fpword |= 0x800;
      break;
    case _RC_DOWN:
      fpword |= 0x400;
      break;
  }
  switch (flags & (_PC_24 | _PC_53)) {
    case _PC_64:
      fpword |= 0x300;
      break;
    case _PC_53:
      fpword |= 0x200;
      break;
    case _PC_24:
      fpword |= 0x0;
      break;
  }
  if (flags & _IC_AFFINE)
    fpword |= 0x1000;

  TRACE ("Control word after  : %08x\n", fpword);

  /* Put fp control word */
  __asm__ __volatile__ ("fldcw %0"::"m" (fpword));

  return flags;
#else
  FIXME (":Not Implemented!\n");
  return 0;
#endif
}

#if 1
// prefered compilation with  -O2 -ffast-math !

static double
explog10 (double x)
{
  /*TRACE("Log10 %f => %f    0x%Lx\n", x, log10(x), *((int64_t*)&x)); */
  return log10 (x);
}

static double
expsin (double x)
{
  /*TRACE("Sin %f => %f  0x%Lx\n", x, sin(x), *((int64_t*)&x)); */
  return sin (x);
}

static double
expcos (double x)
{
  /*TRACE("Cos %f => %f  0x%Lx\n", x, cos(x), *((int64_t*)&x)); */
  return cos (x);
}

#else

static void
explog10 (void)
{
  __asm__ __volatile__
      ("fldl 8(%esp)	\n\t"
      "fldln2	\n\t" "fxch %st(1)	\n\t" "fyl2x		\n\t");
}

static void
expsin (void)
{
  __asm__ __volatile__ ("fldl 8(%esp)	\n\t" "fsin		\n\t");
}

static void
expcos (void)
{
  __asm__ __volatile__ ("fldl 8(%esp)	\n\t" "fcos		\n\t");
}

#endif

#if 0
// See other implementation below

// this seem to be the only how to make this function working properly
// ok - I've spent tremendous amount of time (many many many hours
// of debuging fixing & testing - it's almost unimaginable - kabi

// _ftol - operated on the float value which is already on the FPU stack
// This function has a nonstandard calling convention: the input value is
// expected on the x87 stack instead of the callstack. The input value is
// popped by the callee.

static void
exp_ftol (void)
{
  // crashes :/
  __asm__ __volatile__
      ("sub $12, %esp		\n\t"
      "fstcw   -2(%ebp)	\n\t"
      "wait			\n\t"
      "movw	  -2(%ebp), %ax	\n\t"
      "orb	 $0x0C, %ah	\n\t"
      "movw    %ax, -4(%ebp)	\n\t"
      "fldcw   -4(%ebp)	\n\t"
      "fistpl -12(%ebp)	\n\t"
      "fldcw   -2(%ebp)	\n\t" "movl	 -12(%ebp), %eax \n\t"
      //Note: gcc 3.03 does not do the following op if it
      //      knows that ebp=esp
      "movl %ebp, %esp       \n\t");
}
#endif

static double
explog (double x)
{
  /*TRACE("Log %f => %f    0x%Lx\n", x, log(x), *((int64_t*)&x)); */
  return log (x);
}

static double
expexp (double x)
{
  /*TRACE("Exp %f => %f    0x%Lx\n", x, exp(x), *((int64_t*)&x)); */
  return exp (x);
}

#define FPU_DOUBLES(var1,var2) double var1,var2; \
  __asm__ __volatile__( "fstpl %0;fwait" : "=m" (var2) : ); \
  __asm__ __volatile__( "fstpl %0;fwait" : "=m" (var1) : )


static double
exp_CIpow (void)
{
  FPU_DOUBLES (x, y);

  /*TRACE("_CIpow(%lf, %lf)\n", x, y); */
  return pow (x, y);
}

static double
exppow (double x, double y)
{
  /*TRACE("Pow %f  %f    0x%Lx  0x%Lx  => %f\n", x, y, *((int64_t*)&x), *((int64_t*)&y), pow(x, y)); */
  return pow (x, y);
}

static double
expldexp (double x, int expo)
{
  /*TRACE("Cos %f => %f  0x%Lx\n", x, cos(x), *((int64_t*)&x)); */
  return ldexp (x, expo);
}

static double
expfrexp (double x, int *expo)
{
  /*TRACE("Cos %f => %f  0x%Lx\n", x, cos(x), *((int64_t*)&x)); */
  return frexp (x, expo);
}

static int
exp_stricmp (const char *s1, const char *s2)
{
  return strcasecmp (s1, s2);
}

/* from declaration taken from Wine sources - this function seems to be
 * undocumented in any M$ doc */
static int
exp_setjmp3 (void *jmpbuf, int x)
{
  //TRACE("!!!!UNIMPLEMENTED: setjmp3(%p, %d) => 0\n", jmpbuf, x);
  //return 0;
  __asm__ __volatile__ (
      //"mov 4(%%esp), %%edx \n\t"
      "mov (%%esp), %%eax   \n\t" "mov %%eax, (%%edx)	\n\t"   // store ebp
      //"mov %%ebp, (%%edx)  \n\t"
      "mov %%ebx, 4(%%edx)	\n\t" "mov %%edi, 8(%%edx)	\n\t" "mov %%esi, 12(%%edx)	\n\t" "mov %%esp, 16(%%edx)	\n\t" "mov 4(%%esp), %%eax	\n\t" "mov %%eax, 20(%%edx)	\n\t" "movl $0x56433230, 32(%%edx)	\n\t"   // VC20 ??
      "movl $0, 36(%%edx)	\n\t":  // output
      :"d" (jmpbuf)             // input
      :"eax");
#if 1
  __asm__ __volatile__ ("mov %%fs:0, %%eax	\n\t"   // unsure
      "mov %%eax, 24(%%edx)	\n\t"
      "cmp $0xffffffff, %%eax \n\t"
      "jnz l1                \n\t"
      "mov %%eax, 28(%%edx)	\n\t" "l1:                   \n\t":::"eax");
#endif

  return 0;
}

static DWORD WINAPI
expGetCurrentProcessId (void)
{
  TRACE ("GetCurrentProcessId(void) => %d\n", getpid ());
  return getpid ();             //(DWORD)NtCurrentTeb()->pid;
}


typedef struct
{
  UINT wPeriodMin;
  UINT wPeriodMax;
} TIMECAPS, *LPTIMECAPS;

static MMRESULT WINAPI
exptimeGetDevCaps (LPTIMECAPS lpCaps, UINT wSize)
{
  TRACE ("timeGetDevCaps(%p, %u) !\n", lpCaps, wSize);

  lpCaps->wPeriodMin = 1;
  lpCaps->wPeriodMax = 65535;
  return 0;
}

static MMRESULT WINAPI
exptimeBeginPeriod (UINT wPeriod)
{
  TRACE ("timeBeginPeriod(%u) !\n", wPeriod);

  if (wPeriod < 1 || wPeriod > 65535)
    return 96 + 1;              //TIMERR_NOCANDO;
  return 0;
}

#ifdef QTX
static MMRESULT WINAPI
exptimeEndPeriod (UINT wPeriod)
{
  TRACE ("timeEndPeriod(%u) !\n", wPeriod);

  if (wPeriod < 1 || wPeriod > 65535)
    return 96 + 1;              //TIMERR_NOCANDO;
  return 0;
}
#endif

static void WINAPI
expGlobalMemoryStatus (LPMEMORYSTATUS lpmem)
{
  static MEMORYSTATUS cached_memstatus;
  static int cache_lastchecked = 0;
  SYSTEM_INFO si;
  FILE *f;

  if (time (NULL) == cache_lastchecked) {
    memcpy (lpmem, &cached_memstatus, sizeof (MEMORYSTATUS));
    return;
  }
#if 1
  f = fopen ("/proc/meminfo", "r");
  if (f) {
    char buffer[256];
    int total, used, free, shared, buffers, cached;

    lpmem->dwLength = sizeof (MEMORYSTATUS);
    lpmem->dwTotalPhys = lpmem->dwAvailPhys = 0;
    lpmem->dwTotalPageFile = lpmem->dwAvailPageFile = 0;
    while (fgets (buffer, sizeof (buffer), f)) {
      /* old style /proc/meminfo ... */
      if (sscanf (buffer, "Mem: %d %d %d %d %d %d", &total, &used, &free,
              &shared, &buffers, &cached)) {
        lpmem->dwTotalPhys += total;
        lpmem->dwAvailPhys += free + buffers + cached;
      }
      if (sscanf (buffer, "Swap: %d %d %d", &total, &used, &free)) {
        lpmem->dwTotalPageFile += total;
        lpmem->dwAvailPageFile += free;
      }

      /* new style /proc/meminfo ... */
      if (sscanf (buffer, "MemTotal: %d", &total))
        lpmem->dwTotalPhys = total * 1024;
      if (sscanf (buffer, "MemFree: %d", &free))
        lpmem->dwAvailPhys = free * 1024;
      if (sscanf (buffer, "SwapTotal: %d", &total))
        lpmem->dwTotalPageFile = total * 1024;
      if (sscanf (buffer, "SwapFree: %d", &free))
        lpmem->dwAvailPageFile = free * 1024;
      if (sscanf (buffer, "Buffers: %d", &buffers))
        lpmem->dwAvailPhys += buffers * 1024;
      if (sscanf (buffer, "Cached: %d", &cached))
        lpmem->dwAvailPhys += cached * 1024;
    }
    fclose (f);

    if (lpmem->dwTotalPhys) {
      DWORD TotalPhysical = lpmem->dwTotalPhys + lpmem->dwTotalPageFile;
      DWORD AvailPhysical = lpmem->dwAvailPhys + lpmem->dwAvailPageFile;
      lpmem->dwMemoryLoad = (TotalPhysical - AvailPhysical)
          / (TotalPhysical / 100);
    }
  } else
#endif
  {
    /* FIXME: should do something for other systems */
    lpmem->dwMemoryLoad = 0;
    lpmem->dwTotalPhys = 16 * 1024 * 1024;
    lpmem->dwAvailPhys = 16 * 1024 * 1024;
    lpmem->dwTotalPageFile = 16 * 1024 * 1024;
    lpmem->dwAvailPageFile = 16 * 1024 * 1024;
  }
  expGetSystemInfo (&si);
  lpmem->dwTotalVirtual =
      (char *) si.lpMaximumApplicationAddress -
      (char *) si.lpMinimumApplicationAddress;
  /* FIXME: we should track down all the already allocated VM pages and substract them, for now arbitrarily remove 64KB so that it matches NT */
  lpmem->dwAvailVirtual = lpmem->dwTotalVirtual - 64 * 1024;
  memcpy (&cached_memstatus, lpmem, sizeof (MEMORYSTATUS));
  cache_lastchecked = time (NULL);

  /* it appears some memory display programs want to divide by these values */
  if (lpmem->dwTotalPageFile == 0)
    lpmem->dwTotalPageFile++;

  if (lpmem->dwAvailPageFile == 0)
    lpmem->dwAvailPageFile++;
}

/**********************************************************************
 * SetThreadPriority [KERNEL32.@]  Sets priority for thread.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
static WIN_BOOL WINAPI
expSetThreadPriority (HANDLE hthread,   /* [in] Handle to thread */
    INT priority)
{                               /* [in] Thread priority level */
  TRACE ("SetThreadPriority(0x%x,%d)\n", hthread, priority);
  return TRUE;
}

static void WINAPI
expExitProcess (DWORD status)
{
  TRACE ("EXIT - code %d\n", (int) status);
  exit (status);
}

static INT WINAPI
expMessageBoxA (HWND hWnd, LPCSTR text, LPCSTR title, UINT type)
{
  TRACE ("MSGBOX '%s' '%s' (%d)\n", text, title, type);
#ifdef QTX
  if (type == MB_ICONHAND && !strlen (text) && !strlen (title))
    return IDIGNORE;
#endif
  return IDOK;
}

static WIN_BOOL
expTerminateProcess (HANDLE hProcess, DWORD uExitCode)
{
  TRACE ("TERMINATE - pid %u, code %lu\n", hProcess, uExitCode);
  return TRUE;
}

static DWORD WINAPI
expSetCriticalSectionSpinCount (LPVOID lpCriticalSection, DWORD dwSpinCount)
{
  return 0;
}


/* these are needed for mss1 */

/* defined in stubs.s */
void exp_EH_prolog (void);

#include <netinet/in.h>
static WINAPI inline unsigned long int
exphtonl (unsigned long int hostlong)
{
//    TRACE("htonl(%x) => %x\n", hostlong, htonl(hostlong));
  return htonl (hostlong);
}

static WINAPI inline unsigned long int
expntohl (unsigned long int netlong)
{
//    TRACE("ntohl(%x) => %x\n", netlong, ntohl(netlong));
  return ntohl (netlong);
}

static void WINAPI
expVariantInit (void *p)
{
  TRACE ("InitCommonControls called!\n");
  return;
}

int
expRegisterClassA (const void /*WNDCLASSA*/ *wc)
{
  TRACE ("RegisterClassA(%p) => random id\n", wc);
  return time (NULL);           /* be precise ! */
}

int
expUnregisterClassA (const char *className, HINSTANCE hInstance)
{
  TRACE ("UnregisterClassA(%s, 0x%x) => 0\n", className, hInstance);
  return 0;
}

#ifdef QTX
/* should be fixed bcs it's not fully strlen equivalent */
static int
expSysStringByteLen (void *str)
{
  TRACE ("SysStringByteLen(%p) => %d\n", str, strlen (str));
  return strlen (str);
}

static int
expDirectDrawCreate (void)
{
  TRACE ("DirectDrawCreate(...) => NULL\n");
  return 0;
}

#if 1
typedef struct tagPALETTEENTRY
{
  BYTE peRed;
  BYTE peGreen;
  BYTE peBlue;
  BYTE peFlags;
} PALETTEENTRY;

/* reversed the first 2 entries */
typedef struct tagLOGPALETTE
{
  WORD palNumEntries;
  WORD palVersion;
  PALETTEENTRY palPalEntry[1];
} LOGPALETTE;

static HPALETTE WINAPI
expCreatePalette (CONST LOGPALETTE * lpgpl)
{
  HPALETTE test;
  int i;

  TRACE ("CreatePalette(%p) => NULL\n", lpgpl);

  i = sizeof (LOGPALETTE) + ((lpgpl->palNumEntries -
          1) * sizeof (PALETTEENTRY));
  test = (HPALETTE) malloc (i);
  /* preventive expect bad values leading to overapping */
  memmove ((void *) test, lpgpl, i);

  return test;
}
#else
static int
expCreatePalette (void)
{
  TRACE ("CreatePalette(...) => NULL\n");
  return NULL;
}
#endif

static int WINAPI
expGetClientRect (HWND win, RECT * r)
{
  TRACE ("GetClientRect(0x%x, %p) => 1\n", win, r);
  r->right = PSEUDO_SCREEN_WIDTH;
  r->left = 0;
  r->bottom = PSEUDO_SCREEN_HEIGHT;
  r->top = 0;
  return 1;
}

#if 0
typedef struct tagPOINT
{
  LONG x;
  LONG y;
} POINT, *PPOINT;
#endif

static int WINAPI
expClientToScreen (HWND win, POINT * p)
{
  TRACE ("ClientToScreen(0x%x, %p = %ld,%ld) => 1\n", win, p, p->x, p->y);
  p->x = 0;
  p->y = 0;
  return 1;
}
#endif

/* for m3jpeg */
static int WINAPI
expSetThreadIdealProcessor (HANDLE thread, int proc)
{
  TRACE ("SetThreadIdealProcessor(0x%x, %x) => 0\n", thread, proc);
  return 0;
}

static int WINAPI
expMessageBeep (int type)
{
  TRACE ("MessageBeep(%d) => 1\n", type);
  return 1;
}

static int WINAPI
expDialogBoxParamA (void *inst, const char *name,
    HWND parent, void *dialog_func, void *init_param)
{
  TRACE ("DialogBoxParamA(%p, %p = %s, 0x%x, %p, %p) => 0x42424242\n",
      inst, name, name, parent, dialog_func, init_param);
  return 0x42424242;
}

static unsigned int
expRegisterClipboardFormatA (char *format)
{
  static unsigned int id = 0xBFFF;

  // FIXME: need a hashmap
  if (id < 0xFFFF)
    id++;

  TRACE ("RegisterClipboardFormatA(%s) => %d\n", format, id);
  return id;
}


/* needed by imagepower mjpeg2k */
static void *
exprealloc (void *ptr, size_t size)
{
  TRACE ("realloc(%p, %x)\n", ptr, size);
  if (!ptr)
    return my_mreq (size, 0);
  else
    return my_realloc (ptr, size);
}

static double
expfloor (double x)
{
  TRACE ("floor(%lf)\n", x);
  return floor (x);
}

#define FPU_DOUBLE(var) double var; \
  __asm__ __volatile__( "fstpl %0;fwait" : "=m" (var) : )

static int64_t
exp_ftol (void)
{
  FPU_DOUBLE (x);
  return (int64_t) x;
}

static double
exp_CIcos (void)
{
  FPU_DOUBLE (x);

  /*TRACE("_CIcos(%lf)\n", x); */
  return cos (x);
}

static double
exp_CIsin (void)
{
  FPU_DOUBLE (x);

  /*TRACE("_CIsin(%lf)\n", x); */
  return sin (x);
}

static double
exp_CIcosh (void)
{
  FPU_DOUBLE (x);

  /*TRACE("_CIcosh(%lf)\n", x); */
  return cosh (x);
}

static double
exp_CIsinh (void)
{
  FPU_DOUBLE (x);

  /*TRACE("_CIsinh(%lf)\n", x); */
  return sinh (x);
}

static double
exp_CItanh (void)
{
  FPU_DOUBLE (x);

  /*TRACE("_CItanh(%lf)\n", x); */
  return tanh (x);
}

static double
exp_CIacos (void)
{
  FPU_DOUBLE (x);

  /*TRACE("_CIacos(%lf)\n", x); */
  return acos (x);
}

static double
exp_CIasin (void)
{
  FPU_DOUBLE (x);

  /*TRACE("_CIasin(%lf)\n", x); */
  return asin (x);
}

static double
exp_CIfmod (void)
{
  FPU_DOUBLES (x, y);

  /*TRACE("_CIfmod(%lf)\n", x); */
  return fmod (x, y);
}

/*
//static void exp_CxxThrowException( void *object, cxx_exception_type *type )
static void exp_CxxThrowException( void *object, void *type )
{
  TRACE("_CxxThrowException\n");

#if 0
//we don't have RaiseException() :/
#define CXX_FRAME_MAGIC    0x19930520
#define CXX_EXCEPTION      0xe06d7363
  DWORD args[3];
  args[0] = CXX_FRAME_MAGIC;
  args[1] = (DWORD)object;
  args[2] = (DWORD)type;
  RaiseException(CXX_EXCEPTION, EH_NONCONTINUABLE, 3, args);
#endif
}
*/


struct exports
{
  char name[64];
  int id;
  void *func;
};
struct libs
{
  char name[64];
  int length;
  struct exports *exps;
};

#define FF(X,Y) \
    {#X, Y, (void*)exp##X},

struct exports exp_kernel32[] = {
  FF (IsBadWritePtr, 357)
      FF (IsBadReadPtr, 354)
      FF (IsBadStringPtrW, -1)
      FF (IsBadStringPtrA, -1)
      FF (DisableThreadLibraryCalls, -1)
      FF (CreateThread, -1)
      FF (CreateEventA, -1)
      FF (SetEvent, -1)
      FF (ResetEvent, -1)
      FF (WaitForSingleObject, -1)
#ifdef QTX
      FF (WaitForMultipleObjects, -1)
      FF (ExitThread, -1)
      FF (CreateMutexA, -1)
      FF (ReleaseMutex, -1)
#endif
      FF (GetSystemInfo, -1)
      FF (GetVersion, 332)
      FF (HeapCreate, 461)
      FF (HeapAlloc, -1)
      FF (HeapDestroy, -1)
      FF (HeapFree, -1)
      FF (HeapSize, -1)
      FF (HeapReAlloc, -1)
      FF (GetProcessHeap, -1)
      FF (VirtualAlloc, -1)
      FF (VirtualFree, -1)
      FF (VirtualQuery, -1)
      FF (InitializeCriticalSection, -1)
      FF (EnterCriticalSection, -1)
      FF (LeaveCriticalSection, -1)
      FF (DeleteCriticalSection, -1)
      FF (TlsAlloc, -1)
      FF (TlsFree, -1)
      FF (TlsGetValue, -1)
      FF (TlsSetValue, -1)
      FF (GetCurrentThreadId, -1)
      FF (GetCurrentProcess, -1)
      FF (LocalAlloc, -1)
      FF (LocalReAlloc, -1)
      FF (LocalLock, -1)
      FF (GlobalAlloc, -1)
      FF (GlobalReAlloc, -1)
      FF (GlobalLock, -1)
      FF (GlobalSize, -1)
      FF (MultiByteToWideChar, 427)
      FF (WideCharToMultiByte, -1)
      FF (GetVersionExA, -1)
      FF (CreateSemaphoreA, -1)
      FF (QueryPerformanceCounter, -1)
      FF (QueryPerformanceFrequency, -1)
      FF (LocalHandle, -1)
      FF (LocalUnlock, -1)
      FF (LocalFree, -1)
      FF (GlobalHandle, -1)
      FF (GlobalUnlock, -1)
      FF (GlobalFree, -1)
      FF (LoadResource, -1)
      FF (ReleaseSemaphore, -1)
      FF (FindResourceA, -1)
      FF (FindResourceExA, -1)
      FF (LockResource, -1)
      FF (FreeResource, -1)
      FF (SizeofResource, -1)
      FF (CloseHandle, -1)
      FF (GetCommandLineA, -1)
      FF (GetCommandLineW, -1)
      FF (GetEnvironmentStringsW, -1)
      FF (FreeEnvironmentStringsW, -1)
      FF (FreeEnvironmentStringsA, -1)
      FF (GetEnvironmentStrings, -1)
      FF (GetStartupInfoA, -1)
      FF (GetStdHandle, -1)
      FF (GetFileType, -1)
#ifdef QTX
      FF (GetFileAttributesA, -1)
#endif
      FF (SetHandleCount, -1)
      FF (GetACP, -1)
      FF (GetModuleFileNameA, -1)
      FF (SetUnhandledExceptionFilter, -1)
      FF (LoadLibraryA, -1)
      FF (GetProcAddress, -1)
      FF (FreeLibrary, -1)
      FF (CreateFileMappingA, -1)
      FF (OpenFileMappingA, -1)
      FF (MapViewOfFile, -1)
      FF (UnmapViewOfFile, -1)
      FF (Sleep, -1)
      FF (GetModuleHandleA, -1)
      FF (GetProfileIntA, -1)
      FF (GetPrivateProfileIntA, -1)
      FF (GetPrivateProfileStringA, -1)
      FF (WritePrivateProfileStringA, -1)
      FF (GetLastError, -1)
      FF (SetLastError, -1)
      FF (InterlockedIncrement, -1)
      FF (InterlockedDecrement, -1)
      FF (GetTimeZoneInformation, -1)
      FF (OutputDebugStringA, -1)
      FF (SetLocalTime, -1)
      FF (GetLocalTime, -1)
      FF (GetSystemTime, -1)
      FF (GetSystemTimeAsFileTime, -1)
      FF (SystemTimeToFileTime, -1)
      FF (LocalFileTimeToFileTime, -1)
      FF (SetFileTime, -1)
      FF (GetEnvironmentVariableA, -1)
      FF (SetEnvironmentVariableA, -1)
      FF (RtlZeroMemory, -1)
      FF (RtlMoveMemory, -1)
      FF (RtlFillMemory, -1)
      FF (GetTempPathA, -1)
      FF (FindFirstFileA, -1)
      FF (FindNextFileA, -1)
      FF (FindClose, -1)
      FF (FileTimeToLocalFileTime, -1)
      FF (DeleteFileA, -1)
      FF (ReadFile, -1)
      FF (WriteFile, -1)
      FF (SetFilePointer, -1)
      FF (GetTempFileNameA, -1)
      FF (CreateFileA, -1)
      FF (CreateFileW, -1)
      FF (CreatePipe, -1)
      FF (LockFile, -1)
      FF (UnlockFile, -1)
      FF (GetNumberOfConsoleInputEvents, -1)
      FF (ReadConsoleInputA, -1)
      FF (PeekConsoleInputA, -1)
      FF (PeekNamedPipe, -1)
      FF (GetFileInformationByHandle, -1)
      FF (WriteConsoleA, -1)
      FF (SetEndOfFile, -1)
      FF (GetSystemDirectoryA, -1)
      FF (GetWindowsDirectoryA, -1)
#ifdef QTX
      FF (GetCurrentDirectoryA, -1)
      FF (SetCurrentDirectoryA, -1)
      FF (CreateDirectoryA, -1)
#endif
      FF (GetShortPathNameA, -1)
      FF (GetFullPathNameA, -1)
      FF (SetErrorMode, -1)
      FF (IsProcessorFeaturePresent, -1)
      FF (GetProcessAffinityMask, -1)
      FF (InterlockedExchange, -1)
      FF (InterlockedCompareExchange, -1)
      FF (MulDiv, -1)
      FF (lstrcmpiA, -1)
      FF (lstrlenA, -1)
      FF (lstrcpyA, -1)
      FF (lstrcatA, -1)
      FF (lstrcpynA, -1)
      FF (GetProcessVersion, -1)
      FF (GetCurrentThread, -1)
      FF (GetOEMCP, -1)
      FF (GetCPInfo, -1)
      FF (DuplicateHandle, -1)
      FF (GetTickCount, -1)
      FF (SetThreadAffinityMask, -1)
      FF (GetCurrentProcessId, -1)
      FF (GlobalMemoryStatus, -1)
      FF (SetThreadPriority, -1)
  FF (ExitProcess, -1) {"LoadLibraryExA", -1, (void *) &LoadLibraryExA},
  FF (SetThreadIdealProcessor, -1)
      FF (TerminateProcess, -1)
      FF (SetCriticalSectionSpinCount, -1)
};

/* TODO:
    FlsAlloc, FlsSetValue, FlsGetVale, FlsFree
*/

struct exports exp_msvcrt[] = {
  FF (malloc, -1)
      FF (_malloc_dbg, -1)
      FF (_initterm, -1)
      FF (__dllonexit, -1)
  FF (free, -1) {"??3@YAXPAX@Z", -1, expdelete},
  {"??2@YAPAXI@Z", -1, expnew},
  {"_adjust_fdiv", -1, (void *) &_adjust_fdiv},
  FF (strrchr, -1)
      FF (strchr, -1)
      FF (strlen, -1)
      FF (strcpy, -1)
      FF (strncpy, -1)
      FF (wcscpy, -1)
      FF (strcmp, -1)
      FF (strncmp, -1)
      FF (strcat, -1)
      FF (_stricmp, -1)
      FF (_strdup, -1)
      FF (_setjmp3, -1)
      FF (isalnum, -1)
      FF (isspace, -1)
      FF (isalpha, -1)
      FF (isdigit, -1)
      FF (memmove, -1)
      FF (memcmp, -1)
      FF (memset, -1)
      FF (memcpy, -1)
  FF (time, -1) {"rand", -1, (void *) &rand},
  {"srand", -1, (void *) &srand},
  {"atoi", -1, (void *) &atoi},
  FF (_control87, -1)
      FF (log10, -1)
      FF (log, -1)
      FF (pow, -1)
      FF (sin, -1)
      FF (cos, -1)
      FF (exp, -1)
      FF (_ftol, -1)
      FF (_CIpow, -1)
      FF (_CIcos, -1)
      FF (_CIsin, -1)
      FF (_CIcosh, -1)
      FF (_CIsinh, -1)
      FF (_CItanh, -1)
      FF (_CIacos, -1)
      FF (_CIasin, -1)
      FF (_CIfmod, -1)
      FF (ldexp, -1)
      FF (frexp, -1)
      FF (sprintf, -1)
      FF (sscanf, -1)
      FF (fopen, -1)
      FF (fprintf, -1)
      FF (printf, -1)
      FF (getenv, -1)
      FF (floor, -1)
      FF (_EH_prolog, -1)
  FF (calloc, -1) {"ceil", -1, (void *) &ceil},
/* needed by imagepower mjpeg2k */
  {"clock", -1, (void *) &clock},
  {"memchr", -1, (void *) &memchr},
  {"vfprintf", -1, (void *) &vfprintf},
  //{"realloc",-1,(void*)&realloc},
  FF (realloc, -1) {"puts", -1, (void *) &puts},
  //FF(_CxxThrowException,-1)
};

struct exports exp_winmm[] = {
  FF (GetDriverModuleHandle, -1)
      FF (timeGetTime, -1)
      FF (DefDriverProc, -1)
      FF (OpenDriverA, -1)
      FF (OpenDriver, -1)
      FF (timeGetDevCaps, -1)
      FF (timeBeginPeriod, -1)
#ifdef QTX
      FF (timeEndPeriod, -1)
      FF (waveOutGetNumDevs, -1)
#endif
};

struct exports exp_user32[] = {
  FF (LoadStringA, -1)
      FF (wsprintfA, -1)
      FF (GetDC, -1)
      FF (GetDesktopWindow, -1)
      FF (ReleaseDC, -1)
      FF (IsRectEmpty, -1)
      FF (LoadCursorA, -1)
      FF (SetCursor, -1)
      FF (GetCursorPos, -1)
#ifdef QTX
      FF (ShowCursor, -1)
#endif
      FF (RegisterWindowMessageA, -1)
      FF (GetSystemMetrics, -1)
      FF (GetSysColor, -1)
      FF (GetSysColorBrush, -1)
      FF (GetWindowDC, -1)
      FF (DrawTextA, -1)
      FF (MessageBoxA, -1)
      FF (RegisterClassA, -1)
      FF (UnregisterClassA, -1)
#ifdef QTX
      FF (GetWindowRect, -1)
      FF (MonitorFromWindow, -1)
      FF (MonitorFromRect, -1)
      FF (MonitorFromPoint, -1)
      FF (EnumDisplayMonitors, -1)
      FF (GetMonitorInfoA, -1)
      FF (EnumDisplayDevicesA, -1)
      FF (GetClientRect, -1)
      FF (ClientToScreen, -1)
      FF (IsWindowVisible, -1)
      FF (GetActiveWindow, -1)
      FF (GetClassNameA, -1)
      FF (GetClassInfoA, -1)
      FF (GetWindowLongA, -1)
      FF (EnumWindows, -1)
      FF (GetWindowThreadProcessId, -1)
      FF (CreateWindowExA, -1)
#endif
      FF (MessageBeep, -1)
      FF (DialogBoxParamA, -1)
      FF (RegisterClipboardFormatA, -1)
};

/* TODO:
  BOOL WINAPI EnableWindow(__in  HWND hWnd,__in  BOOL bEnable);
  BOOL WINAPI IsWindow(__in_opt  HWND hWnd);
  LRESULT WINAPI SendMessage(__in  HWND hWnd,__in  UINT Msg,__in  WPARAM wParam,__in  LPARAM lParam
*/

struct exports exp_advapi32[] = {
  FF (RegCloseKey, -1)
      FF (RegCreateKeyExA, -1)
      FF (RegEnumKeyExA, -1)
      FF (RegEnumValueA, -1)
      FF (RegOpenKeyA, -1)
      FF (RegOpenKeyExA, -1)
      FF (RegQueryValueExA, -1)
      FF (RegSetValueExA, -1)
};

struct exports exp_gdi32[] = {
  FF (CreateCompatibleDC, -1)
      FF (CreateFontA, -1)
      FF (DeleteDC, -1)
      FF (DeleteObject, -1)
      FF (GetDeviceCaps, -1)
      FF (GetSystemPaletteEntries, -1)
#ifdef QTX
      FF (CreatePalette, -1)
      FF (GetObjectA, -1)
      FF (CreateRectRgn, -1)
#endif
};

struct exports exp_version[] = {
  FF (GetFileVersionInfoSizeA, -1)
};

struct exports exp_ole32[] = {
  FF (CoCreateFreeThreadedMarshaler, -1)
      FF (CoCreateInstance, -1)
      FF (CoInitialize, -1)
      FF (CoTaskMemAlloc, -1)
      FF (CoTaskMemFree, -1)
      FF (StringFromGUID2, -1)
};

// do we really need crtdll ???
// msvcrt is the correct place probably...
struct exports exp_crtdll[] = {
  FF (memcpy, -1)
      FF (wcscpy, -1)
};

struct exports exp_comctl32[] = {
  FF (StringFromGUID2, -1)
      FF (InitCommonControls, 17)
#ifdef QTX
      FF (CreateUpDownControl, 16)
#endif
};

struct exports exp_wsock32[] = {
  FF (htonl, 8)
      FF (ntohl, 14)
};

struct exports exp_msdmo[] = {
  FF (memcpy, -1)               // just test
      FF (MoCopyMediaType, -1)
      FF (MoCreateMediaType, -1)
      FF (MoDeleteMediaType, -1)
      FF (MoDuplicateMediaType, -1)
      FF (MoFreeMediaType, -1)
      FF (MoInitMediaType, -1)
};

struct exports exp_oleaut32[] = {
  FF (VariantInit, 8)
#ifdef QTX
      FF (SysStringByteLen, 149)
#endif
};

/*  realplayer8:
	DLL Name: PNCRT.dll
	vma:  Hint/Ord Member-Name
	22ff4	  615  free
	2302e	  250  _ftol
	22fea	  666  malloc
	2303e	  609  fprintf
	2305e	  167  _adjust_fdiv
	23052	  280  _initterm

	22ffc	  176  _beginthreadex
	23036	  284  _iob
	2300e	   85  __CxxFrameHandler
	23022	  411  _purecall
*/
#ifdef REALPLAYER
struct exports exp_pncrt[] = {
  FF (malloc, -1)               // just test
      FF (free, -1)             // just test
      FF (fprintf, -1)          // just test
  {"_adjust_fdiv", -1, (void *) &_adjust_fdiv},
  FF (_ftol, -1)
      FF (_initterm, -1)
};
#endif

#ifdef QTX
struct exports exp_ddraw[] = {
  FF (DirectDrawCreate, -1)
};
#endif

#define LL(X) \
    {#X".dll", sizeof(exp_##X)/sizeof(struct exports), exp_##X},

struct libs libraries[] = {
  LL (kernel32)
      LL (msvcrt)
      LL (winmm)
      LL (user32)
      LL (advapi32)
      LL (gdi32)
      LL (version)
      LL (ole32)
      LL (oleaut32)
      LL (crtdll)
      LL (comctl32)
      LL (wsock32)
      LL (msdmo)
#ifdef REALPLAYER
      LL (pncrt)
#endif
#ifdef QTX
      LL (ddraw)
#endif
  {"msvcrtd.dll", sizeof (exp_msvcrt) / sizeof (struct exports), exp_msvcrt},
//    LL(Dsplib)
};

#if defined(__CYGWIN__) || defined(__OS2__) || defined (__OpenBSD__)
#define MANGLE(a) "_" #a
#else
#define MANGLE(a) #a
#endif
static void
ext_stubs (void)
{
  // expects:
  //  ax  position index
  //  cx  address of printf function
#if 1
  __asm__ __volatile__ ("push %%edx		\n\t" "movl $0xdeadbeef, %%eax \n\t" "movl $0xdeadbeef, %%edx \n\t" "shl $5, %%eax		\n\t"   // ax * 32
      "addl $0xdeadbeef, %%eax \n\t"    // overwrite export_names
      "pushl %%eax		\n\t" "pushl $0xdeadbeef   	\n\t"   // overwrite called_unk
      "call *%%edx		\n\t"   // printf (via dx)
      "addl $8, %%esp	\n\t"
      "xorl %%eax, %%eax	\n\t" "pop %%edx             \n\t":::"eax");
#else
  __asm__ __volatile__ ("push %%edx		\n\t" "movl $0, %%eax	\n\t" "movl $0, %%edx	\n\t" "shl $5, %%eax		\n\t"   // ax * 32
      "addl %0, %%eax	\n\t" "pushl %%eax		\n\t" "pushl %1		\n\t" "call *%%edx		\n\t"   // printf (via dx)
      "addl $8, %%esp	\n\t"
      "xorl %%eax, %%eax	\n\t"
      "pop %%edx		\n\t"::"m" (*export_names), "m" (*called_unk)
      :"memory", "edx", "eax");
#endif

}

//static void add_stub(int pos)

//extern int unk_exp1;
static int pos = 0;
// place for unresolved exports stub codes
static char extcode[NUM_STUB_ENTRIES * 0x30];
static const char *called_unk = "Called unk_%s\n";

static void *
add_stub (void)
{
  // generated code in runtime!
  char *answ = (char *) extcode + pos * 0x30;
  int i;

  /* xine: check if stub for this export was created before */
  for (i = 0; i < pos; i++) {
    if (strcmp (export_names[pos], export_names[i]) == 0)
      return extcode + i * 0x30;        /* return existing stub */
  }

  /* xine: side effect of the stub fix. we must not
   * allocate a stub for this function otherwise QT dll
   * will try to call it.
   */
  if (strcmp (export_names[pos], "AllocateAndInitializeSid") == 0) {
    return 0;
  }
#if 0
  memcpy (answ, &unk_exp1, 0x64);
  *(int *) (answ + 9) = pos;
  *(int *) (answ + 47) -= ((int) answ - (int) &unk_exp1);
#endif
  memcpy (answ, ext_stubs, 0x2f);       // 0x2c is current size
  //answ[4] = 0xb8; // movl $0, eax  (0xb8 0x00000000)
  *((int *) (answ + 5)) = pos;
  //answ[9] = 0xba; // movl $0, edx  (0xba 0x00000000)
  *((long *) (answ + 10)) = (long) printf;
  //answ[17] = 0x05; // addl $0, eax  (0x05 0x00000000)
  *((long *) (answ + 18)) = (long) export_names;
  //answ[23] = 0x68; // pushl $0  (0x68 0x00000000)
  *((long *) (answ + 24)) = (long) called_unk;

  /* xine: don't overflow the stub tables */
  if ((pos + 1) < sizeof (extcode) / 0x30 &&
      (pos + 1) < sizeof (export_names) / sizeof (export_names[0])) {
    pos++;
  } else {
    strcpy (export_names[pos], " too many unresolved exports");
  }

  return (void *) answ;
}

static void *
LookupExternalNative (const char *library, LPCSTR name)
{
#ifdef LOADLIB_TRY_NATIVE
  /* ok, this is a hack, and a big memory leak. should be fixed. - alex */
  int hand;
  WINE_MODREF *wm;
  void *func;

  hand = LoadLibraryA (library);
  if (!hand)
    goto no_dll;
  wm = MODULE32_LookupHMODULE (hand);
  if (!wm) {
    FreeLibrary (hand);
    goto no_dll;
  }
  func = PE_FindExportedFunction (wm, name, 0);
  if (!func) {
    TRACE ("No such ordinal in external dll\n");
    FreeLibrary ((int) hand);
    goto no_dll;
  }

  TRACE ("External dll loaded (offset: 0x%x, func: %p)\n", hand, func);
  return func;

no_dll:
  TRACE ("Dll not found: '%s'\n", library);
#endif
  return NULL;
}

void *
LookupExternal (const char *library, int ordinal)
{
  int i, j;
  void *func;

  if (library == 0) {
    TRACE ("ERROR: library=0\n");
    return (void *) ext_unknown;
  }
  //TRACE("%x %x\n", &unk_exp1, &unk_exp2);

  TRACE ("External func %s:%d\n", library, ordinal);

  for (i = 0; i < sizeof (libraries) / sizeof (struct libs); i++) {
    if (strcasecmp (library, libraries[i].name))
      continue;
    for (j = 0; j < libraries[i].length; j++) {
      if (ordinal != libraries[i].exps[j].id)
        continue;
      //TRACE("Hit: 0x%p\n", libraries[i].exps[j].func);
      return libraries[i].exps[j].func;
    }
  }
  if ((func = LookupExternalNative (library, (LPCSTR) ordinal))) {
    return func;
  }

  /* xine: pos is now tested inside add_stub()
     if(pos>NUM_STUB_ENTRIES)return 0;
   */
  /* real names can be checked from corresponding .DEF file */
  snprintf (export_names[pos], sizeof(export_names[pos]), "%s:%d", library, ordinal);
  return add_stub ();
}

void *
LookupExternalByName (const char *library, const char *name)
{
  int i, j;
  void *func;

  if (library == 0) {
    TRACE ("ERROR: library=0\n");
    return (void *) ext_unknown;
  }
  if (name == 0) {
    TRACE ("ERROR: name=0\n");
    return (void *) ext_unknown;
  }
  TRACE ("External func %s:%s\n", library, name);
  for (i = 0; i < sizeof (libraries) / sizeof (struct libs); i++) {
    if (strcasecmp (library, libraries[i].name))
      continue;
    for (j = 0; j < libraries[i].length; j++) {
      if (strcmp (name, libraries[i].exps[j].name))
        continue;
      //          TRACE("Hit: 0x%08X\n", libraries[i].exps[j].func);
      return libraries[i].exps[j].func;
    }
  }
  if ((func = LookupExternalNative (library, (LPCSTR) name))) {
    return func;
  }
  /* xine: pos is now tested inside add_stub()
     if(pos>NUM_STUB_ENTRIES)return 0;// to many symbols
   */
  if (strlen (name) > 31) {
    TRACE ("ERROR: name=%s longer that 32 chars (%d)! (fix export_names)\n",
        name, strlen (name));
  }
  strcpy (export_names[pos], name);
  return add_stub ();
}

void
my_garbagecollection (void)
{
#ifdef GARBAGE
  int unfree = 0, unfreecnt = 0;

  int max_fatal = 8;
  free_registry ();
  while (last_alloc) {
    alloc_header *mem = last_alloc + 1;
    unfree += my_size (mem);
    unfreecnt++;
    if (my_release (mem) != 0)
      // avoid endless loop when memory is trashed
      if (--max_fatal < 0)
        break;
  }
  TRACE ("Total Unfree %d bytes cnt %d [%p,%d]\n", unfree, unfreecnt,
      last_alloc, alccnt);
#endif
  g_tls = NULL;
  list = NULL;
}
