
/*
 * header file for winelib
 * simple dlopen like interface for dlls
 *
 * This file is released under the LGPL.
 * Author: Torben Hohn <torbenh@informatik.uni-bremen.de>
 */

#ifndef WINELIB_H
#define WINELIB_H

#include <signal.h>
#include <windows.h>

extern HANDLE gui_thread;
extern DWORD  gui_thread_id;

void * WineLoadLibrary(char *dll);
void * WineGetProcAddress(void *handle, char *function);
int SharedWineInit(void (*sighandler)(int, siginfo_t*, void*));
int WineAdoptThread ();

#endif
