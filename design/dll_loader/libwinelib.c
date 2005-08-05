/*
 * winelib code - Initializes wine as shared library
 *
 * Copyright 2004 Novell, Inc. (http://www.novell.com/)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include <stdio.h>
#include <windows.h>
#include <windows/ntstatus.h>
#include <setjmp.h>
#include <signal.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <fcntl.h>
#include <wine/library.h>
#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif
#include <dlfcn.h>
#include <limits.h>
#include <errno.h>
#include <string.h>

#include "thread.h"
#include "pthreademu.h"
#include "server.h"
#include "libwinelib.h"

#include <winternl.h>

typedef NTSTATUS WINAPI (*NtProtectVirtualMemory_ptr)(HANDLE, PVOID*, ULONG*, ULONG, ULONG*);
typedef NTSTATUS WINAPI (*NtFreeVirtualMemory_ptr)(HANDLE, PVOID, ULONG*, ULONG);
typedef NTSTATUS WINAPI (*NtAllocateVirtualMemory_ptr)(HANDLE, PVOID*, PVOID, ULONG*, ULONG, ULONG);
typedef VOID WINAPI (*RtlAcquirePebLock_ptr) (VOID);
typedef VOID WINAPI (*RtlReleasePebLock_ptr) (VOID);

static NtProtectVirtualMemory_ptr ntprotectvm;
static NtAllocateVirtualMemory_ptr ntallocatevm;
static NtFreeVirtualMemory_ptr ntfreevm;
static RtlAcquirePebLock_ptr acquirepeblock;
static RtlReleasePebLock_ptr releasepeblock;
static LIST_ENTRY tls_link_head;
static PEB* peb;
static HTASK16 htask16;

static unsigned int (*server_call)(void*);
static void (*send_fd)(int);
static void (*init_thread)(int,int,void*);

static jmp_buf jump; 
static void (*sharedwine_signal_handler)(int signal, siginfo_t*, void*) = NULL;

void* initial_teb;

static void
setup_signal (int sig)
{
  struct sigaction sigact;
  struct sigaction oldact;

  sigact.sa_handler = (void (*)(int)) sharedwine_signal_handler;
  sigact.sa_flags = SA_RESTART|SA_SIGINFO;
  sigact.sa_sigaction = (void (*)(int signal, siginfo_t* info, void *ptr)) sharedwine_signal_handler;
  sigemptyset (&sigact.sa_mask);
  sigaddset  (&sigact.sa_mask, SIGINT);
  sigaddset  (&sigact.sa_mask, SIGUSR2);

  if (sigaction (sig, &sigact, NULL)) {
    printf("could not install signal handler for signal %d", sig);
  }
}

void
redirect_signals ()
{
  setup_signal (SIGINT);
  setup_signal (SIGFPE);
  setup_signal (SIGILL);
  setup_signal (SIGSEGV);
  setup_signal (SIGABRT);
  setup_signal (SIGTERM);
  setup_signal (SIGBUS);
  setup_signal (SIGTRAP);
}

/* The per-thread signal stack size */
#ifdef __i386__
#define SIGNAL_STACK_SIZE  4096
#else
#define SIGNAL_STACK_SIZE  0  /* we don't need a signal stack on non-i386 */
#endif

/* from winnt.h, but hard to make it appear */

#define MEM_SYSTEM              0x80000000

/***********************************************************************
 *           alloc_teb (no difference from libwine, but its static there)
 */

static TEB *shared_alloc_teb( ULONG *size )
{
    TEB *teb;

    *size = SIGNAL_STACK_SIZE + sizeof(TEB);
    teb = (TEB *)wine_anon_mmap( NULL, *size, PROT_READ | PROT_WRITE | PROT_EXEC, 0 );
    if (teb == (TEB *)-1) return NULL;
    if (!(teb->teb_sel = wine_ldt_alloc_fs()))
    {
        munmap( teb, *size );
        return NULL;
    }
    teb->Tib.ExceptionList = (_EXCEPTION_REGISTRATION_RECORD*)~0UL;
    teb->Tib.StackBase     = (void *)~0UL;
    teb->Tib.Self          = &teb->Tib;
    teb->Peb               = peb;
    teb->StaticUnicodeString.Buffer        = teb->StaticUnicodeBuffer;
    teb->StaticUnicodeString.MaximumLength = sizeof(teb->StaticUnicodeBuffer);

    return teb;
}

/***********************************************************************
 *           free_teb (no change from libwine, but its static there)
 */
static inline void shared_free_teb( TEB *teb )
{
    ULONG size = 0;
    void *addr = teb;

    ntfreevm ( GetCurrentProcess(), &addr, &size, MEM_RELEASE );
    wine_ldt_free_fs( teb->teb_sel );
    munmap( teb, SIGNAL_STACK_SIZE + sizeof(TEB) );
}

static int proxy_pripe_request[2];
static int proxy_pripe_status[2];
static pthread_mutex_t proxy_thread_lock;

static DWORD WINAPI new_thread_proxy (LPVOID param)
{
  int request_pipe[2];
  NTSTATUS status;
  struct __server_request_info sri;
  struct new_thread_request *req = &sri.u.req.new_thread_request;
  int fd = -1;
  TEB *teb;
  struct wine_pthread_thread_info thread_info;
  ULONG size;

  if (pipe (proxy_pripe_request)) {
    printf("cannot set up proxy request pipe (%s)", strerror (errno));
    return (DWORD)-1;
  }

  if (pipe (proxy_pripe_status)) {
    printf("cannot set up proxy reply pipe (%s)", strerror (errno));
    return (DWORD)-1;
  }

  while (1) {
    
    /* wait for a request size to be written */
    
    if (read (proxy_pripe_request[0], &sri, sizeof (sri)) != sizeof (sri)) {
      printf("cannot read new thread request from proxy request pipe (%s)", strerror (errno));
      return (DWORD)-1;
    }

    if (pipe (request_pipe) >= 0) {

      /* set up a new request pipe */
      
      fcntl (request_pipe[1], F_SETFD, 1); /* set close on exec flag */
      /* server_*/ send_fd (request_pipe[0]);
      
      req->request_fd = request_pipe[0];
      
      if ((status = server_call (req)) == 0) {
        fd = request_pipe[1];
      } else {
        close (request_pipe[0]);
        close (request_pipe[1]);
      }
    }

    /* create a new TEB. the "thread_info" structure seems to be
       the most convenient way of passing back the result.
     */

    teb = shared_alloc_teb (&size);

    thread_info.stack_base = NULL;
    thread_info.stack_size = 0;
    thread_info.teb_base   = teb;
    thread_info.teb_size   = size;
    thread_info.teb_sel    = teb->teb_sel;
    
    /* send back the result, request fd and the newly allocated TEB info */

    if (write (proxy_pripe_status[1], &sri, sizeof (sri)) != sizeof (sri)) {
      printf("cannot write request/reply back to proxy reply pipe (%s)", strerror (errno));
      /* XXX free teb */
      return (DWORD)-1;
    }
    
    if (write (proxy_pripe_status[1], &fd, sizeof (fd)) != sizeof (status)) {
      printf("cannot write status back to proxy reply pipe (%s)", strerror (errno));
      /* XXX free teb */
      return (DWORD)-1;
    }

    if (write (proxy_pripe_status[1], &thread_info, sizeof (thread_info)) != sizeof (thread_info)) {
      printf("cannot write thread info to proxy reply pipe (%s)", strerror (errno));
      /* XXX free teb */
      return (DWORD)-1;
    }

    close (request_pipe[0]);
  }

  return 0;
}

extern int wine_shared_premain ();

int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
  int jmpstat = 1;

  if (CreateThread (NULL, 0, new_thread_proxy, NULL, 0, NULL) == NULL) {
    printf("could not create new thread proxy");
  }

  if (wine_shared_premain ()) {
    jmpstat = 2;
  } 
  
  // return to caller.

  longjmp (jump, jmpstat);
}

/*
  WineLoadLibrary is used by System.Windows.Forms to import the Wine dlls
*/
void *
WineLoadLibrary(char *dll)
{
  return(LoadLibraryA(dll));
}

void *
WineGetProcAddress(void *handle, char *function)
{
  return((void *)GetProcAddress((HINSTANCE__*)handle, function));
}

static void*
get_stack_base ()
{
  void* ret;
  __asm__ ( "movl %%ebp, %0" : "=r" (ret));
  return ret;
}

static inline void*
get_stack_ptr ()
{
  void* ret;
  __asm__ ( "movl %%esp, %0" : "=r" (ret));
  return ret;
}

int
SharedWineInit(void (*sighandler)(int,siginfo_t*,void*))
{
  char Error[1024]="";
  char *WineArguments[] = {"sharedapp", LIBPATH "/libtest.so", NULL};
  void* stackbase;
  size_t stacksize;
  void *ntdll;
  void *ntso;
  char ntdllpath[PATH_MAX+1];
  char* dlerr;
  
  sharedwine_signal_handler = sighandler;

  if (setjmp(jump) == 0) {

    wine_init(2, WineArguments, Error, sizeof(Error));
    
    if (Error[0]!='\0') {
      printf("Wine initialization error:%s\n", Error);
      return -1;
    }
  }

  redirect_signals ();
  
  initial_teb = NtCurrentTeb();

  /* set the stack limits so that any exception handling based on
     tracking stack boundaries has a chance of working.
  */

  NtCurrentTeb()->Tib.StackBase  = (void*) ~0UL;
  NtCurrentTeb()->Tib.StackLimit = 0;

  putenv ("_WINE_SHAREDLIB_PATH=" DLLPATH);

  /* Loading this will pull in many other DLLs that would
     otherwise be loaded+unloaded over and over again
     during plugin discovery.
  */

  if (WineLoadLibrary ("user32.dll") == NULL) {
    printf("cannot load Windows DLL user32");
    return 1;
  }

  if (WineLoadLibrary ("comdlg32.dll") == NULL) {
    printf("cannot load Windows DLL comdlg32");
    return 1;
  }

  if (WineLoadLibrary ("oleaut32.dll") == NULL) {
    printf("cannot load Windows DLL oleaut32");
    return 1;
  }

  /* set up the functions and data we need to be able to adopt
     threads.
  */

  #define GET_SYMBOL(res,type,handle,name) if((res=(type)WineGetProcAddress (handle,name))==NULL) { printf("cannot find Windows function %s", name); return -1; }
  
  if ((ntdll = WineLoadLibrary( "ntdll.dll")) == NULL) {
    printf("cannot load Windows DLL ntdll");
    return 1;
  }

  GET_SYMBOL(ntallocatevm, NtAllocateVirtualMemory_ptr, ntdll, "NtAllocateVirtualMemory");
  GET_SYMBOL(ntfreevm, NtFreeVirtualMemory_ptr, ntdll, "NtFreeVirtualMemory");
  GET_SYMBOL(ntprotectvm, NtProtectVirtualMemory_ptr, ntdll, "NtProtectVirtualMemory");
  GET_SYMBOL(acquirepeblock, RtlAcquirePebLock_ptr, ntdll, "RtlAcquirePebLock");
  GET_SYMBOL(releasepeblock, RtlReleasePebLock_ptr, ntdll, "RtlReleasePebLock");

  tls_link_head = NtCurrentTeb()->TlsLinks;
  peb = NtCurrentTeb()->Peb;
  htask16 = NtCurrentTeb()->htask16;

  sprintf (ntdllpath, "%s/ntdll.dll.so", DLLPATH);

  if ((ntso = dlopen (ntdllpath, RTLD_NOW|RTLD_GLOBAL)) == NULL) {
    printf("cannot open NT DLL (%s)", ntdllpath);
    return -1;
  }

  server_call = (unsigned int (*)(void*))dlsym (ntso, "wine_server_call");
  if ((dlerr = dlerror ()) != NULL) {
    printf("cannot find wine_server_call (%s)", dlerr);
    return -1;
  }
  send_fd = (void (*)(int))dlsym (ntso, "wine_server_send_fd");
  if ((dlerr = dlerror ()) != NULL) {
    printf("cannot find wine_server_send_fd (%s)", dlerr);
    return -1;
  }
  init_thread = (void (*)(int, int, void*))dlsym (ntso, "server_init_thread");
  if ((dlerr = dlerror ()) != NULL) {
    printf("cannot find server_init_thread (%s)", dlerr);
    return -1;
  }

        return 0;
}

/***********************************************************************
 *  adopt_thread - take an existing linux thread (of some kind),
 *  make the wineserver aware of it, and set up the TEB so that
 *  we can execute win32 functions in it.
 */
int
wine_adopt_thread (void)
{
    struct wine_pthread_thread_info thread_info;
    struct debug_info* debug_info;
    TEB* teb;
    int request_fd;
    int tid;
    stack_t altstack;
    void *addr;
    ULONG size;

    SERVER_START_REQ( new_thread )
    {
        req->suspend    = FALSE;
        req->inherit    = 0;  /* FIXME */

  pthread_mutex_lock (&proxy_thread_lock);

  if (write (proxy_pripe_request[1], &__req, sizeof (__req)) != sizeof (__req) ||
      read (proxy_pripe_status[0], &__req, sizeof (__req)) != sizeof (__req) ||
      read (proxy_pripe_status[0], &request_fd, sizeof (request_fd)) != sizeof (request_fd) ||
      read (proxy_pripe_status[0], &thread_info, sizeof (thread_info)) != sizeof (thread_info))

  {
    printf("cannot read/write proxy thread request (%s)", strerror (errno));
  }

  pthread_mutex_unlock (&proxy_thread_lock);
  
        if (request_fd >= 0)
        {
            tid = reply->tid;
        }
  
    }
    SERVER_END_REQ;

    if (request_fd < 0) goto error;
    
    debug_info = (struct debug_info *) malloc (sizeof (struct debug_info));
    
    debug_info->str_pos = debug_info->strings;
    debug_info->out_pos = debug_info->output;
    
    teb = (TEB*) thread_info.teb_base;
    size = thread_info.teb_size;

    teb->tibflags      = TEBF_WIN32;
    teb->exit_code     = STILL_ACTIVE;
    teb->request_fd    = -1;
    teb->request_fd    = -1;
    teb->reply_fd      = -1;
    teb->wait_fd[0]    = -1;
    teb->wait_fd[1]    = -1;
    teb->debug_info    = debug_info;
    teb->htask16       = htask16;

    wine_pthread_init_current_teb (&thread_info);

    teb->ClientId.UniqueProcess = (HANDLE)GetCurrentProcessId();
    teb->ClientId.UniqueThread  = (HANDLE)tid;
    teb->request_fd = request_fd;

    /* don't bother with signal initialization since we redirect all signals
       back to linux anyway.
    */
    /* SIGNAL_Init(); */

    /* server_*/ init_thread( thread_info.pid, thread_info.tid, NULL );
    wine_pthread_init_thread( &thread_info );

    /* create a memory view for the TEB */
    ntallocatevm ( GetCurrentProcess(), &addr, teb, &size,
                             MEM_SYSTEM, PAGE_EXECUTE_READWRITE );

    /* allocate a memory view for the stack */
    size = thread_info.stack_size;
    ntallocatevm ( GetCurrentProcess(), &teb->DeallocationStack, thread_info.stack_base,
                             &size, MEM_SYSTEM, PAGE_EXECUTE_READWRITE );
    /* limit is lower than base since the stack grows down */
    teb->Tib.StackBase  = (char *)thread_info.stack_base + thread_info.stack_size;
    teb->Tib.StackLimit = thread_info.stack_base;

    /* setup the guard page */
    size = 1;
    ntprotectvm ( GetCurrentProcess(), &teb->DeallocationStack, &size,
                            PAGE_EXECUTE_READWRITE | PAGE_GUARD, NULL );

    acquirepeblock ();
    InsertHeadList( &tls_link_head, &teb->TlsLinks );
    releasepeblock ();
    
    return 0;

  error:
    shared_free_teb (teb);
    /* XXX deallocate VM */
    /* close thread handle */
    close (request_fd);
    return -1;
}
