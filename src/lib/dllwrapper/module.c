/*
 * Modules
 *
 * Copyright 1995 Alexandre Julliard
 */

// define for quicktime calls debugging and/or MacOS-level emulation:
#define EMU_QTX_API

// define for quicktime debugging (verbose logging):
//#define DEBUG_QTX_API

#include "config.h"
#define _GNU_SOURCE

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>


#include "windef.h"
#include "winerror.h"
#include "heap.h"
#include "module.h"
#include "pe_image.h"
#include "debugtools.h"
#ifdef HAVE_LIBDL
#include <dlfcn.h>
#include "elfdll.h"
#endif
#include "win32.h"
#include "driver.h"
#include "ext.h"

#ifdef EMU_QTX_API
#include "wrapper.h"
int report_func (void *stack_base, int stack_size, reg386_t * reg,
    u_int32_t * flags);
int report_func_ret (void *stack_base, int stack_size, reg386_t * reg,
    u_int32_t * flags);
#endif

//#undef TRACE
//#define TRACE printf

//WINE_MODREF *local_wm=NULL;
modref_list *local_wm = NULL;

HANDLE SegptrHeap;

WINE_MODREF *
MODULE_FindModule (LPCSTR m)
{
  modref_list *list = local_wm;
  //TRACE("FindModule: Module %s request\n", m);
  if (list == NULL)
    return NULL;
  //while(strcmp(m, list->wm->filename))
  while (!strcasestr (list->wm->filename, m)) {
    //TRACE("%s: %x\n", list->wm->filename, list->wm->module);
    list = list->prev;
    if (list == NULL)
      return NULL;
  }
  //TRACE("Resolved to %s\n", list->wm->filename);
  return list->wm;
}

static void
MODULE_RemoveFromList (WINE_MODREF * mod)
{
  modref_list *list = local_wm;
  if (list == 0)
    return;
  if (mod == 0)
    return;
  if ((list->prev == NULL) && (list->next == NULL)) {
    free (list);
    local_wm = NULL;
//      uninstall_fs();
    return;
  }
  for (; list; list = list->prev) {
    if (list->wm == mod) {
      if (list->prev)
        list->prev->next = list->next;
      if (list->next)
        list->next->prev = list->prev;
      if (list == local_wm)
        local_wm = list->prev;
      free (list);
      return;
    }
  }

}

WINE_MODREF *
MODULE32_LookupHMODULE (HMODULE m)
{
  modref_list *list = local_wm;
  //TRACE("LookupHMODULE: Module %X request\n", m);
  if (list == NULL) {
    TRACE ("LookupHMODULE %X failed\n", m);
    return NULL;
  }
  while (m != list->wm->module) {
//      printf("Checking list %X wm %X module %X\n",
//             list, list->wm, list->wm->module);
    list = list->prev;
    if (list == NULL) {
      TRACE ("LookupHMODULE %X failed\n", m);
      return NULL;
    }
  }
  //TRACE("LookupHMODULE hit %p\n", list->wm);
  return list->wm;
}

/*************************************************************************
 *		MODULE_InitDll
 */
static WIN_BOOL
MODULE_InitDll (WINE_MODREF * wm, DWORD type, LPVOID lpReserved)
{
  WIN_BOOL retv = TRUE;

#ifdef USE_DEBUG
  static LPCSTR typeName[] =
      { "PROCESS_DETACH", "PROCESS_ATTACH", "THREAD_ATTACH", "THREAD_DETACH" };
#endif
  assert (wm);

  /* Skip calls for modules loaded with special load flags */

  if ((wm->flags & WINE_MODREF_DONT_RESOLVE_REFS)
      || (wm->flags & WINE_MODREF_LOAD_AS_DATAFILE))
    return TRUE;

  TRACE ("(%s,%s,%p) - CALL\n", wm->modname, typeName[type], lpReserved);

  /* Call the initialization routine */
  switch (wm->type) {
    case MODULE32_PE:
      retv = PE_InitDLL (wm, type, lpReserved);
      break;

    case MODULE32_ELF:
      /* no need to do that, dlopen() already does */
      break;

    default:
      ERR ("wine_modref type %d not handled.\n", wm->type);
      retv = FALSE;
      break;
  }

  /* The state of the module list may have changed due to the call
     to PE_InitDLL. We cannot assume that this module has not been
     deleted.  */
  TRACE ("(%p,%s,%p) - RETURN %d\n", wm, typeName[type], lpReserved, retv);

  return retv;
}

/*************************************************************************
 *		MODULE_DllProcessAttach
 *
 * Send the process attach notification to all DLLs the given module
 * depends on (recursively). This is somewhat complicated due to the fact that
 *
 * - we have to respect the module dependencies, i.e. modules implicitly
 *   referenced by another module have to be initialized before the module
 *   itself can be initialized
 *
 * - the initialization routine of a DLL can itself call LoadLibrary,
 *   thereby introducing a whole new set of dependencies (even involving
 *   the 'old' modules) at any time during the whole process
 *
 * (Note that this routine can be recursively entered not only directly
 *  from itself, but also via LoadLibrary from one of the called initialization
 *  routines.)
 *
 * Furthermore, we need to rearrange the main WINE_MODREF list to allow
 * the process *detach* notifications to be sent in the correct order.
 * This must not only take into account module dependencies, but also
 * 'hidden' dependencies created by modules calling LoadLibrary in their
 * attach notification routine.
 *
 * The strategy is rather simple: we move a WINE_MODREF to the head of the
 * list after the attach notification has returned.  This implies that the
 * detach notifications are called in the reverse of the sequence the attach
 * notifications *returned*.
 *
 * NOTE: Assumes that the process critical section is held!
 *
 */
static WIN_BOOL
MODULE_DllProcessAttach (WINE_MODREF * wm, LPVOID lpReserved)
{
  WIN_BOOL retv = TRUE;
  /* int i; -- not used */
  assert (wm);

  /* prevent infinite recursion in case of cyclical dependencies */
  if ((wm->flags & WINE_MODREF_MARKER)
      || (wm->flags & WINE_MODREF_PROCESS_ATTACHED))
    return retv;

  TRACE ("DllProcessAttach for (%s,%p) - START\n", wm->modname, lpReserved);

  /* Tag current MODREF to prevent recursive loop */
  wm->flags |= WINE_MODREF_MARKER;

  /* Recursively attach all DLLs this one depends on */
/*    for ( i = 0; retv && i < wm->nDeps; i++ )
        if ( wm->deps[i] )
            retv = MODULE_DllProcessAttach( wm->deps[i], lpReserved );
*/
  /* Call DLL entry point */

  //local_wm=wm;
  if (local_wm) {
    local_wm->next = (modref_list *) malloc (sizeof (modref_list));
    local_wm->next->prev = local_wm;
    local_wm->next->next = NULL;
    local_wm->next->wm = wm;
    local_wm = local_wm->next;
  } else {
    local_wm = (modref_list *) malloc (sizeof (modref_list));
    local_wm->next = local_wm->prev = NULL;
    local_wm->wm = wm;
  }
  /* Remove recursion flag */
  wm->flags &= ~WINE_MODREF_MARKER;

  if (retv) {
    retv = MODULE_InitDll (wm, DLL_PROCESS_ATTACH, lpReserved);
    if (retv)
      wm->flags |= WINE_MODREF_PROCESS_ATTACHED;
  }


  TRACE ("DllProcessAttach for  (%s,%p) = %s - END\n", wm->modname, lpReserved,
      (retv ? "TRUE" : "FALSE"));

  return retv;
}

/*************************************************************************
 *		MODULE_DllProcessDetach
 *
 * Send DLL process detach notifications.  See the comment about calling
 * sequence at MODULE_DllProcessAttach.  Unless the bForceDetach flag
 * is set, only DLLs with zero refcount are notified.
 */
static void
MODULE_DllProcessDetach (WINE_MODREF * wm, WIN_BOOL bForceDetach,
    LPVOID lpReserved)
{
  //    WINE_MODREF *wm=local_wm;
  /* modref_list* l = local_wm; -- not used */
  wm->flags &= ~WINE_MODREF_PROCESS_ATTACHED;
  MODULE_InitDll (wm, DLL_PROCESS_DETACH, lpReserved);
/*    while (l)
    {
	modref_list* f = l;
	l = l->next;
	free(f);
    }
    local_wm = 0;*/
}

/***********************************************************************
 *	MODULE_LoadLibraryExA	(internal)
 *
 * Load a PE style module according to the load order.
 *
 * The HFILE parameter is not used and marked reserved in the SDK. I can
 * only guess that it should force a file to be mapped, but I rather
 * ignore the parameter because it would be extremely difficult to
 * integrate this with different types of module represenations.
 *
 */
static WINE_MODREF *
MODULE_LoadLibraryExA (LPCSTR libname, HFILE hfile, DWORD flags)
{
  DWORD err = GetLastError ();
  WINE_MODREF *pwm;
  /* int i; -- not used */
  //module_loadorder_t *plo;

  SetLastError (ERROR_FILE_NOT_FOUND);
  //TRACE("Trying native dll '%s'\n", libname);
  pwm = PE_LoadLibraryExA (libname, flags);
#ifdef HAVE_LIBDL
  if (!pwm) {
    TRACE ("Trying ELF dll '%s'\n", libname);
    pwm = (WINE_MODREF *) ELFDLL_LoadLibraryExA (libname, flags);
  }
#endif
  if (pwm) {
    /* Initialize DLL just loaded */
    TRACE ("Loaded module '%s' at 0x%08x, \n", libname, pwm->module);
    /* Set the refCount here so that an attach failure will */
    /* decrement the dependencies through the MODULE_FreeLibrary call. */
    pwm->refCount++;

    SetLastError (err);         /* restore last error */
    return pwm;
  }
  //TRACE("Failed to load module '%s'; error=0x%08lx, \n", libname, GetLastError());
  return NULL;
}

/***********************************************************************
 *           MODULE_FreeLibrary
 *
 * NOTE: Assumes that the process critical section is held!
 */
static WIN_BOOL
MODULE_FreeLibrary (WINE_MODREF * wm)
{
  //TRACE("(%s) - START\n", wm->modname );

  /* Recursively decrement reference counts */
  //MODULE_DecRefCount( wm );

  /* Call process detach notifications */
  MODULE_DllProcessDetach (wm, FALSE, NULL);

  PE_UnloadLibrary (wm);

  //TRACE("END\n");

  return TRUE;
}

/***********************************************************************
 *           LoadLibraryExA   (KERNEL32)
 */
HMODULE WINAPI
LoadLibraryExA (LPCSTR libname, HANDLE hfile, DWORD flags)
{
  WINE_MODREF *wm = 0;
  char *listpath[] =
      { "", "", WIN32_LIB_PATH, WIN32_PATH, "/usr/local/lib/win32", NULL };
  char path[512];
  int i;
  static char *unavailable_lib_names[100];
  static int unavailable_libs = 0;

  if (!libname) {
    SetLastError (ERROR_INVALID_PARAMETER);
    return 0;
  }

  /* see if this lib has already failed before */
  for (i = 0; i < unavailable_libs; i++) {
    if (!strcmp (unavailable_lib_names[i], libname)) {
      return 0;
    }
  }

  //TRACE("calling FindModule(%s)\n",libname);
  wm = MODULE_FindModule (libname);
  if (wm)
    return wm->module;

  //if(fs_installed==0)
  //    install_fs();

  TRACE ("module '%s' not already loaded\n", libname);

  i = -1;
  /* check search path */
  while (wm == 0 && listpath[++i]) {
    memset (&path, 0, sizeof (path));

    if (i < 2) {
      if (i == 0)
        /* check just original file name */
        strncpy (path, libname, sizeof (path) - 1);
      else
        /* check default user path */
        strncpy (path, win32_def_path, sizeof (path) - 2);
    } else if (strcmp (win32_def_path, listpath[i]))
      /* path from the list */
      strncpy (path, listpath[i], 300);
    else
      continue;

    if (i > 0) {
      strcat (path, "/");
      strncat (path, libname, 100);
    }
    path[511] = 0;

    TRACE ("trying to load module '%s'\n", path);
    wm = MODULE_LoadLibraryExA (path, hfile, flags);

    if (!wm) {
      TRACE ("loading module '%s' failed\n", path);
    }
  }
  if (wm) {
    TRACE ("Loaded module '%s'\n", libname);

    if (!MODULE_DllProcessAttach (wm, NULL)) {
      WARN_ (module) ("Attach failed for module '%s', \n", libname);
      MODULE_RemoveFromList (wm);
      MODULE_FreeLibrary (wm);
      SetLastError (ERROR_DLL_INIT_FAILED);
      wm = NULL;
    }
  }

  if (!wm) {
    if (unavailable_libs < 100) {
      unavailable_lib_names[unavailable_libs++] = strdup (libname);
    } else {
      TRACE ("more than 100 missing libs\n");
    }
    /* idealy we remember this */
    TRACE ("wine/module: Win32 LoadLibrary failed to load: %s\n", libname);
  }
  // remove a few divs in the VP codecs that make trouble
  if (strstr (libname, "vp5vfw.dll") && wm) {
    int i;
    if (PE_FindExportedFunction (wm, "DriverProc", TRUE) == (void *) 0x10003930) {
      for (i = 0; i < 3; i++)
        ((char *) 0x10004e86)[i] = 0x90;
      for (i = 0; i < 3; i++)
        ((char *) 0x10005a23)[i] = 0x90;
      for (i = 0; i < 3; i++)
        ((char *) 0x10005bff)[i] = 0x90;
    } else {
      printf ("wine/module: Unsupported VP5 version\n");
      return 0;
    }
  }

  if (strstr (libname, "vp6vfw.dll") && wm) {
    int i;
    if (PE_FindExportedFunction (wm, "DriverProc", TRUE) == (void *) 0x10003ef0) {
      // looks like VP 6.1.0.2
      for (i = 0; i < 6; i++)
        ((char *) 0x10007268)[i] = 0x90;
      for (i = 0; i < 6; i++)
        ((char *) 0x10007e83)[i] = 0x90;
      for (i = 0; i < 6; i++)
        ((char *) 0x1000806a)[i] = 0x90;
    } else if (PE_FindExportedFunction (wm, "DriverProc",
            TRUE) == (void *) 0x10004120) {
      // looks like VP 6.2.0.10
      for (i = 0; i < 6; i++)
        ((char *) 0x10007688)[i] = 0x90;
      for (i = 0; i < 6; i++)
        ((char *) 0x100082c3)[i] = 0x90;
      for (i = 0; i < 6; i++)
        ((char *) 0x100084aa)[i] = 0x90;
    } else if (PE_FindExportedFunction (wm, "DriverProc",
            TRUE) == (void *) 0x10003e70) {
      // looks like VP 6.0.7.3
      for (i = 0; i < 6; i++)
        ((char *) 0x10007559)[i] = 0x90;
      for (i = 0; i < 6; i++)
        ((char *) 0x100081c3)[i] = 0x90;
      for (i = 0; i < 6; i++)
        ((char *) 0x1000839e)[i] = 0x90;
    } else {
      printf ("wine/module: Unsupported VP6 version\n");
      return 0;
    }
  }

  if (strstr (libname, "QuickTime.qts") && wm) {
    void **ptr;
    void *dispatch_addr;
    int i;

    //dispatch_addr = GetProcAddress(wm->module, "theQuickTimeDispatcher", TRUE);
    dispatch_addr =
        PE_FindExportedFunction (wm, "theQuickTimeDispatcher", TRUE);
    if (dispatch_addr == (void *) 0x62924c30) {
      printf ("wine/module: QuickTime5 DLLs found\n");
      ptr = (void **) 0x62b75ca4;       // dispatch_ptr
      for (i = 0; i < 5; i++)
        ((char *) 0x6299e842)[i] = 0x90;        // make_new_region ?
      for (i = 0; i < 28; i++)
        ((char *) 0x6299e86d)[i] = 0x90;        // call__call_CreateCompatibleDC ?
      for (i = 0; i < 5; i++)
        ((char *) 0x6299e898)[i] = 0x90;        // jmp_to_call_loadbitmap ?
      for (i = 0; i < 9; i++)
        ((char *) 0x6299e8ac)[i] = 0x90;        // call__calls_OLE_shit ?
      for (i = 0; i < 106; i++)
        ((char *) 0x62a61b10)[i] = 0x90;        // disable threads
#if 0
      /* CreateThread callers */
      for (i = 0; i < 5; i++)
        ((char *) 0x629487c5)[i] = 0x90;
      for (i = 0; i < 5; i++)
        ((char *) 0x6294b275)[i] = 0x90;
      for (i = 0; i < 5; i++)
        ((char *) 0x629a24b1)[i] = 0x90;
      for (i = 0; i < 5; i++)
        ((char *) 0x629afc5a)[i] = 0x90;
      for (i = 0; i < 5; i++)
        ((char *) 0x62af799c)[i] = 0x90;
      for (i = 0; i < 5; i++)
        ((char *) 0x62af7efe)[i] = 0x90;
      for (i = 0; i < 5; i++)
        ((char *) 0x62afa33e)[i] = 0x90;
#endif

#if 0
      /* TerminateQTML fix */
      for (i = 0; i < 47; i++)
        ((char *) 0x62afa3b8)[i] = 0x90;        // terminate thread
      for (i = 0; i < 47; i++)
        ((char *) 0x62af7f78)[i] = 0x90;        // terminate thread
      for (i = 0; i < 77; i++)
        ((char *) 0x629a13d5)[i] = 0x90;
      ((char *) 0x6288e0ae)[0] = 0xc3;  // font/dc remover
      for (i = 0; i < 24; i++)
        ((char *) 0x6287a1ad)[i] = 0x90;        // destroy window
#endif
    } else if (dispatch_addr == (void *) 0x6693b330) {
      printf ("wine/module: QuickTime6 DLLs found\n");
      ptr = (void **) 0x66bb9524;       // dispatcher_ptr
      for (i = 0; i < 5; i++)
        ((char *) 0x66a730cc)[i] = 0x90;        // make_new_region
      for (i = 0; i < 28; i++)
        ((char *) 0x66a730f7)[i] = 0x90;        // call__call_CreateCompatibleDC
      for (i = 0; i < 5; i++)
        ((char *) 0x66a73122)[i] = 0x90;        // jmp_to_call_loadbitmap
      for (i = 0; i < 9; i++)
        ((char *) 0x66a73131)[i] = 0x90;        // call__calls_OLE_shit
      for (i = 0; i < 96; i++)
        ((char *) 0x66aac852)[i] = 0x90;        // disable threads
    } else if (dispatch_addr == (void *) 0x6693c3e0) {
      printf ("wine/module: QuickTime6.3 DLLs found\n");
      ptr = (void **) 0x66bca01c;       // dispatcher_ptr
      for (i = 0; i < 5; i++)
        ((char *) 0x66a68f6c)[i] = 0x90;        // make_new_region
      for (i = 0; i < 28; i++)
        ((char *) 0x66a68f97)[i] = 0x90;        // call__call_CreateCompatibleDC
      for (i = 0; i < 5; i++)
        ((char *) 0x66a68fc2)[i] = 0x90;        // jmp_to_call_loadbitmap
      for (i = 0; i < 9; i++)
        ((char *) 0x66a68fd1)[i] = 0x90;        // call__calls_OLE_shit
      for (i = 0; i < 96; i++)
        ((char *) 0x66ab4722)[i] = 0x90;        // disable threads
    } else {
      printf ("wine/module: Unsupported QuickTime version (%p)\n",
          dispatch_addr);
      return 0;
    }

    printf ("wine/module: QuickTime.qts patched!!! old entry=%p\n", ptr[0]);

#ifdef EMU_QTX_API
    wrapper_target = ptr[0];
    ptr[0] = wrapper;
#endif
  }

  return wm ? wm->module : 0;
}


/***********************************************************************
 *           LoadLibraryA         (KERNEL32)
 */
HMODULE WINAPI
LoadLibraryA (LPCSTR libname)
{
  return LoadLibraryExA (libname, 0, 0);
}

/***********************************************************************
 *           FreeLibrary
 */
WIN_BOOL WINAPI
FreeLibrary (HINSTANCE hLibModule)
{
  WIN_BOOL retv = FALSE;
  WINE_MODREF *wm;

  wm = MODULE32_LookupHMODULE (hLibModule);

  MODULE_RemoveFromList (wm);

  if (!wm || !hLibModule) {
    SetLastError (ERROR_INVALID_HANDLE);
    return 0;
  } else
    retv = MODULE_FreeLibrary (wm);

  /* garbage... */
  if (local_wm == NULL)
    my_garbagecollection ();

  return retv;
}

/***********************************************************************
 *           MODULE_DecRefCount
 *
 * NOTE: Assumes that the process critical section is held!
 */
#if 0
static void
MODULE_DecRefCount (WINE_MODREF * wm)
{
  int i;

  if (wm->flags & WINE_MODREF_MARKER)
    return;

  if (wm->refCount <= 0)
    return;

  --wm->refCount;
  //TRACE("(%s) refCount: %d\n", wm->modname, wm->refCount );

  if (wm->refCount == 0) {
    wm->flags |= WINE_MODREF_MARKER;

    for (i = 0; i < wm->nDeps; i++)
      if (wm->deps[i])
        MODULE_DecRefCount (wm->deps[i]);

    wm->flags &= ~WINE_MODREF_MARKER;
  }
}
#endif

/***********************************************************************
 *           GetProcAddress   		(KERNEL32.257)
 */
FARPROC WINAPI
GetProcAddress (HMODULE hModule, LPCSTR function)
{
  return MODULE_GetProcAddress (hModule, function, TRUE);
}

#ifdef DEBUG_QTX_API

struct ComponentParameters
{
  unsigned char flags;          /* call modifiers: sync/async, deferred, immed, etc */
  unsigned char paramSize;      /* size in bytes of actual parameters passed to this call */
  short what;                   /* routine selector, negative for Component management calls */
  long params[1];               /* actual parameters for the indicated routine */
};
typedef struct ComponentParameters ComponentParameters;

static char *
component_func (int what)
{
  switch (what) {
    case -1:
      return "kComponentOpenSelect";
    case -2:
      return "kComponentCloseSelect";
    case -3:
      return "kComponentCanDoSelect";
    case -4:
      return "kComponentVersionSelect";
    case -5:
      return "kComponentRegisterSelect";
    case -6:
      return "kComponentTargetSelect";
    case -7:
      return "kComponentUnregisterSelect";
  }
  return "???";
}

static char *
component_func_type (int type, int what)
{
  if (type == 1)
    switch (what) {
      case 0:
        return "kImageCodecGetCodecInfoSelect";
      case 1:
        return "kImageCodecGetCompressionTimeSelect";
      case 5:
        return "kImageCodecPreDecompressSelect";
      case 6:
        return "kImageCodecBandDecompressSelect";
      case 0x12:
        return "kImageCodecDisposeMemorySelect";
      case 0x10:
        return "kImageCodecIsImageDescriptionEquivalentSelect";
      case 0x14:
        return "kImageCodecNewImageBufferMemorySelect";
      case 0x28:
        return "kImageCodecRequestGammaLevelSelect";
    }
  return "???";
}


static int c_level = 0;

static int
dump_component (char *name, int type, void *_orig, ComponentParameters * params,
    void **glob)
{
  int (*orig) (ComponentParameters * params, void **glob) = _orig;
  int ret, i;

  if (params->what < 0)
    fprintf (stderr, "%*sComponentCall: %s  flags=0x%X  size=%d  what=%d %s\n",
        3 * c_level, "", name, params->flags, params->paramSize, params->what,
        component_func (params->what));
  else
    fprintf (stderr,
        "%*sComponentCall: %s  flags=0x%X  size=%d  what=0x%X %s\n",
        3 * c_level, "", name, params->flags, params->paramSize, params->what,
        component_func_type (type, params->what));

  for (i = 0; i < params->paramSize / 4; i++)
    fprintf (stderr, "%*s param[%d] = 0x%X\n", 3 * c_level, "", i,
        params->params[i]);

  ++c_level;
  ret = orig (params, glob);
  --c_level;

  if (ret >= 0x1000)
    fprintf (stderr, "%*s return=0x%X\n", 3 * c_level, "", ret);
  else
    fprintf (stderr, "%*s return=%d\n", 3 * c_level, "", ret);
  return ret;
}

#define DECL_COMPONENT(sname,name,type) \
    static void* real_ ## sname = NULL; \
    static int fake_ ## sname(ComponentParameters *params,void** glob){ \
	return dump_component(name,type,real_ ## sname, params, glob); \
    }

#include "qt_comp.h"

#undef DECL_COMPONENT

#include "qt_fv.h"

#endif

#ifdef EMU_QTX_API

static u_int32_t ret_array[4096];
static int ret_i = 0;

int
report_func (void *stack_base, int stack_size, reg386_t * reg,
    u_int32_t * flags)
{
#ifdef DEBUG_QTX_API
  int i;
  int *dptr;
  void *pwrapper = NULL;
  void *pptr = NULL;
  char *pname = NULL;
  int plen = -1;
  // find the code:

  dptr = 0x62b67ae0;
  dptr += 2 * ((reg->eax >> 16) & 255);
//  printf("FUNC: flag=%d ptr=%p\n",dptr[0],dptr[1]);
  if (dptr[0] & 255) {
    dptr = dptr[1];
    dptr += 4 * (reg->eax & 65535);
//      printf("FUNC: ptr2=%p  eax=%p  edx=%p\n",dptr[1],dptr[0],dptr[2]);
    pwrapper = dptr[1];
    pptr = dptr[0];
    plen = dptr[2];
  } else {
    pwrapper = 0x62924910;
    switch (dptr[1]) {
      case 0x629248d0:
        dptr = 0x62b672c0;
        dptr += 2 * (reg->eax & 65535);
//          printf("FUNC: ptr2=%p  eax=%p  edx=%p\n",0x62924910,dptr[0],dptr[1]);
        pptr = dptr[0];
        plen = dptr[1];
        break;
      case 0x62924e40:
        dptr = 0x62b67c70;
        dptr += 2 * (reg->eax & 65535);
//          printf("FUNC: ptr2=%p  eax=%p  edx=%p\n",0x62924910,dptr[0],dptr[1]);
        pptr = dptr[0];
        plen = dptr[1];
        break;
      case 0x62924e60:
        dptr = 0x62b68108;
        if (reg->eax & 0x8000)
          dptr += 2 * (reg->eax | 0xffff0000);
        else
          dptr += 2 * (reg->eax & 65535);
//          printf("FUNC: ptr2=%p  eax=%p  edx=%p\n",0x62924910,dptr[0],dptr[1]);
        pptr = dptr[0];
        plen = dptr[1];
        break;
      case 0x62924e80:
        dptr = 0x62b68108;
        if (reg->eax & 0x8000)
          dptr += 2 * (reg->eax | 0xffff0000);
        else
          dptr += 2 * (reg->eax & 65535);
//          printf("FUNC: ptr2=%p  eax=%p  edx=%p\n",0x62924910,dptr[0],dptr[1]);
        pptr = dptr[0];
        plen = dptr[1];
        break;
      default:
        printf ("FUNC: unknown ptr & psize!\n");
        pwrapper = dptr[1];
    }
  }

  for (i = 0; qt_fv_list[i].name; i++) {
    if (qt_fv_list[i].id == reg->eax) {
      pname = qt_fv_list[i].name;
      break;
    }
  }

  printf ("FUNC[%X/%s]: wrapper=%p  func=%p  len=%d\n", reg->eax,
      pname ? pname : "???", pwrapper, pptr, plen);

  printf ("FUNC: caller=%p  ebx=%p\n", ((u_int32_t *) stack_base)[0], reg->ebx);

  if (pname)
    printf ("%*sENTER(%d): %s(", ret_i * 2, "", ret_i, pname);
  else
    printf ("%*sENTER(%d): %X(", ret_i * 2, "", ret_i, reg->eax);
  for (i = 0; i < plen / 4; i++) {
    unsigned int val = ((u_int32_t *) stack_base)[1 + i];
    unsigned char *fcc = &val;
    printf ("%s0x%X", i ? ", " : "", val);
    if (fcc[0] >= 0x20 && fcc[0] < 128 &&
        fcc[1] >= 0x20 && fcc[1] < 128 &&
        fcc[2] >= 0x20 && fcc[2] < 128 && fcc[3] >= 0x20 && fcc[3] < 128)
      printf ("='%c%c%c%c'", fcc[3], fcc[2], fcc[1], fcc[0]);
    else if (val >= 8 && val < 65536)
      printf ("=%d", val);
  }
  printf (")\n");
  fflush (stdout);

#endif

#if 1
  // emulate some functions:
  switch (reg->eax) {
      // memory management:
    case 0x150011:             //NewPtrClear
    case 0x150012:             //NewPtrSysClear
      reg->eax = (u_int32_t) malloc (((u_int32_t *) stack_base)[1]);
      memset ((void *) reg->eax, 0, ((u_int32_t *) stack_base)[1]);
#ifdef DEBUG_QTX_API
      printf ("%*sLEAVE(%d): EMULATED! 0x%X\n", ret_i * 2, "", ret_i, reg->eax);
#endif
      return 1;
    case 0x15000F:             //NewPtr
    case 0x150010:             //NewPtrSys
      reg->eax = (u_int32_t) malloc (((u_int32_t *) stack_base)[1]);
#ifdef DEBUG_QTX_API
      printf ("%*sLEAVE(%d): EMULATED! 0x%X\n", ret_i * 2, "", ret_i, reg->eax);
#endif
      return 1;
    case 0x15002f:             //DisposePtr
      if (((u_int32_t *) stack_base)[1] >= 0x60000000)
        printf ("WARNING! Invalid Ptr handle!\n");
      else
        free ((void *) (((u_int32_t *) stack_base)[1]));
      reg->eax = 0;
#ifdef DEBUG_QTX_API
      printf ("%*sLEAVE(%d): EMULATED! 0x%X\n", ret_i * 2, "", ret_i, reg->eax);
#endif
      return 1;
      // mutexes:
    case 0x1d0033:             //QTMLCreateMutex
      reg->eax = 0xdeadbabe;
#ifdef DEBUG_QTX_API
      printf ("%*sLEAVE(%d): EMULATED! 0x%X\n", ret_i * 2, "", ret_i, reg->eax);
#endif
      return 1;
    case 0x1d0034:             //QTMLDestroyMutex
    case 0x1d0035:             //QTMLGrabMutex
    case 0x1d0036:             //QTMLReturnMutex
    case 0x1d003d:             //QTMLTryGrabMutex
      reg->eax = 0;
#ifdef DEBUG_QTX_API
      printf ("%*sLEAVE(%d): EMULATED! 0x%X\n", ret_i * 2, "", ret_i, reg->eax);
#endif
      return 1;
  }
#endif

#if 0
  switch (reg->eax) {
//  case 0x00010000:
//      printf("FUNC: ImageCodecInitialize/ImageCodecGetCodecInfo(ci=%p,&icap=%p)\n",((u_int32_t *)stack_base)[1],((u_int32_t *)stack_base)[4]);
//      break;
    case 0x00010003:
      printf ("FUNC: CountComponents(&desc=%p)\n",
          ((u_int32_t *) stack_base)[1]);
      break;
    case 0x00010004:
      printf ("FUNC: FindNextComponent(prev=%p,&desc=%p)\n",
          ((u_int32_t *) stack_base)[1], ((u_int32_t *) stack_base)[2]);
      break;
    case 0x00010007:
      printf ("FUNC: OpenComponent(prev=%p)\n", ((u_int32_t *) stack_base)[1]);
      break;
    case 0x0003008b:
      printf
          ("FUNC: QTNewGWorldFromPtr(&pts=%p,fourcc=%.4s,&rect=%p,x1=%p,x2=%p,x3=%p,plane=%p,stride=%d)\n",
          ((u_int32_t *) stack_base)[1], &(((u_int32_t *) stack_base)[2]),
          ((u_int32_t *) stack_base)[3], ((u_int32_t *) stack_base)[4],
          ((u_int32_t *) stack_base)[5], ((u_int32_t *) stack_base)[6],
          ((u_int32_t *) stack_base)[7], ((u_int32_t *) stack_base)[8]);
      break;
    case 0x001c0018:
      printf ("FUNC: GetGWorldPixMap(gworld=%p)\n",
          ((u_int32_t *) stack_base)[1]);
      break;
    case 0x00110001:
      printf ("FUNC: Gestalt(fourcc=%.4s, &ret=%p)\n",
          &(((u_int32_t *) stack_base)[1]), ((u_int32_t *) stack_base)[2]);
      break;
    default:{
      int i;
      for (i = 0; qt_fv_list[i].name; i++) {
        if (qt_fv_list[i].id == reg->eax) {
          printf ("FUNC: %s\n", qt_fv_list[i].name);
          break;
        }
      }
    }
  }

  // print stack/reg information
  printf ("ENTER(%d) stack = %d bytes @ %p\n"
      "eax = 0x%08x edx = 0x%08x ebx = 0x%08x ecx = 0x%08x\n"
      "esp = 0x%08x ebp = 0x%08x esi = 0x%08x edi = 0x%08x\n"
      "flags = 0x%08x\n", ret_i,
      stack_size, stack_base,
      reg->eax, reg->edx, reg->ebx, reg->ecx,
      reg->esp, reg->ebp, reg->esi, reg->edi, *flags);
#endif

  // save ret addr:
  ret_array[ret_i] = ((u_int32_t *) stack_base)[0];
  ++ret_i;

#if 0
  // print first 7 longs in the stack (return address, arg[1], arg[2] ... )
  printf ("stack[] = { ");
  for (i = 0; i < 7; i++) {
    printf ("%08x ", ((u_int32_t *) stack_base)[i]);
  }
  printf ("}\n\n");
#endif

//  // mess with function parameters
//  ((u_int32_t *)stack_base)[1] = 0x66554433;

//  // mess with return address...
//  reg->eax = 0x11223344;
  return 0;
}

int
report_func_ret (void *stack_base, int stack_size, reg386_t * reg,
    u_int32_t * flags)
{
#ifdef DEBUG_QTX_API
  int i;
  short err;
#endif

  // restore ret addr:
  --ret_i;
  ((u_int32_t *) stack_base)[0] = ret_array[ret_i];

#ifdef DEBUG_QTX_API

#if 1
  printf ("%*sLEAVE(%d): 0x%X", ret_i * 2, "", ret_i, reg->eax);
  err = reg->eax;
  if (err && (reg->eax >> 16) == 0)
    printf (" = %d", err);
  printf ("\n");
  fflush (stdout);
#else
  // print stack/reg information
  printf ("LEAVE(%d) stack = %d bytes @ %p\n"
      "eax = 0x%08x edx = 0x%08x ebx = 0x%08x ecx = 0x%08x\n"
      "esp = 0x%08x ebp = 0x%08x esi = 0x%08x edi = 0x%08x\n"
      "flags = 0x%08x\n", ret_i,
      stack_size, stack_base,
      reg->eax, reg->edx, reg->ebx, reg->ecx,
      reg->esp, reg->ebp, reg->esi, reg->edi, *flags);
#endif

#if 0
  // print first 7 longs in the stack (return address, arg[1], arg[2] ... )
  printf ("stack[] = { ");
  for (i = 0; i < 7; i++) {
    printf ("%08x ", ((u_int32_t *) stack_base)[i]);
  }
  printf ("}\n\n");
#endif

#endif

//  // mess with function parameters
//  ((u_int32_t *)stack_base)[1] = 0x66554433;

//  // mess with return address...
//  reg->eax = 0x11223344;
  return 0;
}

#endif

/***********************************************************************
 *           MODULE_GetProcAddress   		(internal)
 */
FARPROC
MODULE_GetProcAddress (HMODULE hModule, /* [in] current module handle */
    LPCSTR function,            /* [in] function to be looked up */
    WIN_BOOL snoop)
{
  WINE_MODREF *wm = MODULE32_LookupHMODULE (hModule);
  //WINE_MODREF *wm=local_wm;
  FARPROC retproc;

  if (HIWORD (function))
    TRACE ("GetProcAddress(%08lx,%s)\n", (DWORD) hModule, function);
  else
    TRACE ("GetProcAddress(%08lx,%p)\n", (DWORD) hModule, function);

  if (!wm) {
    SetLastError (ERROR_INVALID_HANDLE);
    return (FARPROC) 0;
  }
  switch (wm->type) {
    case MODULE32_PE:
      retproc = PE_FindExportedFunction (wm, function, snoop);
      if (!retproc)
        SetLastError (ERROR_PROC_NOT_FOUND);
      break;
#ifdef HAVE_LIBDL
    case MODULE32_ELF:
      retproc = (FARPROC) dlsym ((void *) wm->module, function);
      if (!retproc)
        SetLastError (ERROR_PROC_NOT_FOUND);
      return retproc;
#endif
    default:
      ERR ("wine_modref type %d not handled.\n", wm->type);
      SetLastError (ERROR_INVALID_HANDLE);
      return (FARPROC) 0;
  }

#ifdef EMU_QTX_API
  if (HIWORD (function) && retproc) {

#ifdef DEBUG_QTX_API
#define DECL_COMPONENT(sname,name,type) \
    if(!strcmp(function,name)){ \
	TRACE(name "dispatcher caught -> %p\n",retproc); \
	real_ ## sname = retproc; retproc = fake_ ## sname; \
    }
#include "qt_comp.h"
#undef DECL_COMPONENT
#endif

    if (!strcmp (function, "theQuickTimeDispatcher")
//      || !strcmp(function,"_CallComponentFunctionWithStorage")
//      || !strcmp(function,"_CallComponent")
        ) {
      fprintf (stderr, "theQuickTimeDispatcher caught -> %p\n", retproc);
      wrapper_target = (void *) retproc;
      retproc = (void *) wrapper;
    }

  }
#endif
  TRACE ("GetProcAddress()=%p\n", retproc);
  return retproc;
}

static int acounter = 0;
void
CodecAlloc (void)
{
  acounter++;
  //printf("**************CODEC ALLOC %d\n", acounter);
}

void
CodecRelease (void)
{
  acounter--;
  //printf("**************CODEC RELEASE %d\n", acounter);
  if (acounter == 0) {
    for (;;) {
      WINE_MODREF *wm;
      modref_list *list = local_wm;
      if (!local_wm)
        break;
      //printf("CODECRELEASE %p\n", list);
      wm = list->wm;
      MODULE_RemoveFromList (wm);
      MODULE_FreeLibrary (wm);
      if (local_wm == NULL)
        my_garbagecollection ();
    }
  }
}
