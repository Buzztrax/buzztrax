#ifndef loader_win32_h
#define loader_win32_h

#include <time.h>

#include "windef.h"
#include "winbase.h"
#include "com.h"

extern char* win32_def_path;

extern void my_garbagecollection(void);

typedef struct {
    UINT             uDriverSignature;
    HINSTANCE        hDriverModule;
    DRIVERPROC       DriverProc;
    DWORD            dwDriverID;
} DRVR;

typedef DRVR  *PDRVR;
typedef DRVR  *NPDRVR;
typedef DRVR  *LPDRVR;

typedef struct tls_s tls_t;


extern void* LookupExternal(const char* library, int ordinal);
extern void* LookupExternalByName(const char* library, const char* name);
extern int expRegisterClassA(const void/*WNDCLASSA*/ *wc);
extern int expUnregisterClassA(const char *className, HINSTANCE hInstance);

#endif
