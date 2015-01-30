#ifndef wine_resource_h
#define	wine_resource_h

#include "winbase.h"

extern INT WINAPI LoadStringA( HINSTANCE instance, UINT resource_id,
			       LPSTR buffer, INT buflen );

#endif
