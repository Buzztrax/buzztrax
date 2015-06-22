/* Buzz Machine Loader
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@buzztrax.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <dlfcn.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef USE_DLLWRAPPER_IPC
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#define BML_C
#include "bml.h"
#include "bmllog.h"
#ifdef USE_DLLWRAPPER
#include "bmlw.h"
#endif
#ifdef USE_DLLWRAPPER_IPC
#include "strpool.h"
#include "bmlipc.h"
#endif

// wrapped(ipc)
#ifdef USE_DLLWRAPPER_IPC
#define SOCKET_PATH_MAX	sizeof((((struct sockaddr_un *) 0)->sun_path))
static char socket_file[SOCKET_PATH_MAX];
static int server_socket;
static StrPool *sp;
#endif
// native
static void *emu_so = NULL;

// native plugin API method pointers
BMSetLogger bmln_set_logger;
BMSetMasterInfo bmln_set_master_info;

BMOpen bmln_open;
BMClose bmln_close;

BMGetMachineInfo bmln_get_machine_info;
BMGetGlobalParameterInfo bmln_get_global_parameter_info;
BMGetTrackParameterInfo bmln_get_track_parameter_info;
BMGetAttributeInfo bmln_get_attribute_info;

BMNew bmln_new;
BMFree bmln_free;

BMInit bmln_init;

BMGetTrackParameterValue bmln_get_track_parameter_value;
BMSetTrackParameterValue bmln_set_track_parameter_value;

BMGetGlobalParameterValue bmln_get_global_parameter_value;
BMSetGlobalParameterValue bmln_set_global_parameter_value;

BMGetAttributeValue bmln_get_attribute_value;
BMSetAttributeValue bmln_set_attribute_value;

BMTick bmln_tick;
BMWork bmln_work;
BMWorkM2S bmln_work_m2s;
BMStop bmln_stop;

BMAttributesChanged bmln_attributes_changed;

BMSetNumTracks bmln_set_num_tracks;

BMDescribeGlobalValue bmln_describe_global_value;
BMDescribeTrackValue bmln_describe_track_value;

BMSetCallbacks bmln_set_callbacks;

#ifdef USE_DLLWRAPPER_IPC
// ipc wrapper functions

static int
bmpipc_connect (void)
{
  struct sockaddr_un address = { 0, };
  pid_t child_pid;
  int retries = 0;

  if (!getenv ("BMLIPC_DEBUG")) {
    // TODO(ensonic): if this crashes (send returns EPIPE) we need to respawn it
    // spawn the server
    child_pid = fork ();
    if (child_pid == 0) {
      char *args[] = { "bmlhost", socket_file, NULL };
      execvp ("bmlhost", args);
      TRACE ("an error occurred in execvp: %s\n", strerror (errno));
      return FALSE;
    } else if (child_pid < 0) {
      TRACE ("fork failed: %s\n", strerror (child_pid));
      return FALSE;
    }
  }

  if ((server_socket = socket (PF_LOCAL, SOCK_STREAM, 0)) > 0) {
    TRACE ("server socket created\n");
  } else {
    TRACE ("server socket creation failed\n");
    return FALSE;
  }
  address.sun_family = PF_LOCAL;
  strcpy (&address.sun_path[1], socket_file);
  while (retries < 3) {
    int res;
    if ((res =
            connect (server_socket, (struct sockaddr *) &address,
                sizeof (sa_family_t) + strlen (socket_file) + 1)) == 0) {
      TRACE ("server connected after %d retries\n", retries);
      break;
    } else {
      TRACE ("connection failed: %s\n", strerror (errno));
      retries++;
      sleep (1);
    }
  }

  return TRUE;
}

// global API

// TODO(ensonic): separate bmlhost processes:
// - in bmlw_set_master_info() store params and send the, from bmlw_open() for
//   each instance
// - in bmlw_open() create a host instance and return a struct that contains
//   the server_socket, the ipc-buf and the actual BuzzMachineHandle
// - in bmlw_new() return a struct that contains the the server_socket, the
//   ipc-buf and the actual BuzzMachine
// - we could expand the BmlIpcBuf and add a union for
//   { BuzzMachineHandle, BuzzMachine}
// - the struct will be freed in bmlw_close() and bmlw_free()

// TODO(ensonic): ipc performance
// - add varargs version of setters/getters handle multiple attributes/parameters
//   in one call (mostly helpful for introspection)
// - we could also fetch all params in bmlw_init(), update them in the library
//   side and only send them if changed before bmlw_tick()

void
bmlw_set_master_info (long bpm, long tpb, long srat)
{
  BmlIpcBuf bo = IPC_BUF_INIT;
  ssize_t size;

  TRACE ("bmlw_set_master_info(%d, %d, %d)...\n", bpm, tpb, srat);
  bmlipc_write (&bo, "iiii", BM_SET_MASTER_INFO, bpm, tpb, srat);
  size = send (server_socket, bo.buffer, bo.size, MSG_NOSIGNAL);
  TRACE ("sent %d of %d bytes\n", size, bo.size);
  if (size > 0) {
    BmlIpcBuf bi = IPC_BUF_INIT;

    bi.size = (int) recv (server_socket, bi.buffer, IPC_BUF_SIZE, 0);
    TRACE ("got %d bytes\n", bi.size);
  }
}

// library api

BuzzMachineHandle *
bmlw_open (char *bm_file_name)
{
  BmlIpcBuf bo = IPC_BUF_INIT;
  ssize_t size;

  TRACE ("bmlw_open('%s')...\n", bm_file_name);
  bmlipc_write (&bo, "is", BM_OPEN, bm_file_name);
retry:
  size = send (server_socket, bo.buffer, bo.size, MSG_NOSIGNAL);
  TRACE ("sent %d of %d bytes\n", size, bo.size);
  if (size > 0) {
    BmlIpcBuf bi = IPC_BUF_INIT;

    bi.size = (int) recv (server_socket, bi.buffer, IPC_BUF_SIZE, 0);
    TRACE ("got %d bytes\n", bi.size);
    return (BuzzMachineHandle *) ((long) bmlipc_read_int (&bi));
  } else if (errno == EPIPE) {
    TRACE ("bmlhost is dead, respawning\n");
    if (bmpipc_connect ()) {
      goto retry;
    }
  }
  return NULL;
}

void
bmlw_close (BuzzMachineHandle * bmh)
{
  BmlIpcBuf bo = IPC_BUF_INIT;
  ssize_t size;

  bmlipc_write (&bo, "ii", BM_CLOSE, (int) ((long) bmh));
  size = send (server_socket, bo.buffer, bo.size, MSG_NOSIGNAL);
  TRACE ("sent %d of %d bytes\n", size, bo.size);
  if (size > 0) {
    BmlIpcBuf bi = IPC_BUF_INIT;

    bi.size = (int) recv (server_socket, bi.buffer, IPC_BUF_SIZE, 0);
    TRACE ("got %d bytes\n", bi.size);
  }
}


int
bmlw_get_machine_info (BuzzMachineHandle * bmh, BuzzMachineProperty key,
    void *value)
{
  BmlIpcBuf bo = IPC_BUF_INIT;
  ssize_t size;
  int ret = 0;

  bmlipc_write (&bo, "iii", BM_GET_MACHINE_INFO, (int) ((long) bmh), key);
  size = send (server_socket, bo.buffer, bo.size, MSG_NOSIGNAL);
  TRACE ("sent %d of %d bytes\n", size, bo.size);
  if (size > 0) {
    BmlIpcBuf bi = IPC_BUF_INIT;

    bi.size = (int) recv (server_socket, bi.buffer, IPC_BUF_SIZE, 0);
    TRACE ("got %d bytes\n", bi.size);
    ret = bmlipc_read_int (&bi);
    // function writes result into value which is either an int or string
    switch (ret) {
      case 0:
        break;
      case 1:
        *((int *) value) = bmlipc_read_int (&bi);
        break;
      case 2:
        *((const char **) value) = sp_intern (sp, bmlipc_read_string (&bi));
        break;
      default:
        TRACE ("unhandled value type: %d", ret);
    }
  }
  return (ret ? 1 : 0);
}

int
bmlw_get_global_parameter_info (BuzzMachineHandle * bmh, int index,
    BuzzMachineParameter key, void *value)
{
  BmlIpcBuf bo = IPC_BUF_INIT;
  ssize_t size;
  int ret = 0;

  bmlipc_write (&bo, "iiii", BM_GET_GLOBAL_PARAMETER_INFO, (int) ((long) bmh),
      index, key);
  size = send (server_socket, bo.buffer, bo.size, MSG_NOSIGNAL);
  TRACE ("sent %d of %d bytes\n", size, bo.size);
  if (size > 0) {
    BmlIpcBuf bi = IPC_BUF_INIT;

    bi.size = (int) recv (server_socket, bi.buffer, IPC_BUF_SIZE, 0);
    TRACE ("got %d bytes\n", bi.size);
    ret = bmlipc_read_int (&bi);
    // function writes result into value which is either an int or string
    switch (ret) {
      case 0:
        break;
      case 1:
        *((int *) value) = bmlipc_read_int (&bi);
        break;
      case 2:
        *((const char **) value) = sp_intern (sp, bmlipc_read_string (&bi));
        break;
      default:
        TRACE ("unhandled value type: %d", ret);
    }
  }
  return (ret ? 1 : 0);
}

int
bmlw_get_track_parameter_info (BuzzMachineHandle * bmh, int index,
    BuzzMachineParameter key, void *value)
{
  BmlIpcBuf bo = IPC_BUF_INIT;
  ssize_t size;
  int ret = 0;

  bmlipc_write (&bo, "iiii", BM_GET_TRACK_PARAMETER_INFO, (int) ((long) bmh),
      index, key);
  size = send (server_socket, bo.buffer, bo.size, MSG_NOSIGNAL);
  TRACE ("sent %d of %d bytes\n", size, bo.size);
  if (size > 0) {
    BmlIpcBuf bi = IPC_BUF_INIT;

    bi.size = (int) recv (server_socket, bi.buffer, IPC_BUF_SIZE, 0);
    TRACE ("got %d bytes\n", bi.size);
    ret = bmlipc_read_int (&bi);
    // function writes result into value which is either an int or string
    switch (ret) {
      case 0:
        break;
      case 1:
        *((int *) value) = bmlipc_read_int (&bi);
        break;
      case 2:
        *((const char **) value) = sp_intern (sp, bmlipc_read_string (&bi));
        break;
      default:
        TRACE ("unhandled value type: %d", ret);
    }
  }
  return (ret ? 1 : 0);
}

int
bmlw_get_attribute_info (BuzzMachineHandle * bmh, int index,
    BuzzMachineAttribute key, void *value)
{
  BmlIpcBuf bo = IPC_BUF_INIT;
  ssize_t size;
  int ret = 0;

  bmlipc_write (&bo, "iiii", BM_GET_ATTRIBUTE_INFO, (int) ((long) bmh), index,
      key);
  size = send (server_socket, bo.buffer, bo.size, MSG_NOSIGNAL);
  TRACE ("sent %d of %d bytes\n", size, bo.size);
  if (size > 0) {
    BmlIpcBuf bi = IPC_BUF_INIT;

    bi.size = (int) recv (server_socket, bi.buffer, IPC_BUF_SIZE, 0);
    TRACE ("got %d bytes\n", bi.size);
    ret = bmlipc_read_int (&bi);
    // function writes result into value which is either an int or string
    switch (ret) {
      case 0:
        break;
      case 1:
        *((int *) value) = bmlipc_read_int (&bi);
        break;
      case 2:
        *((const char **) value) = sp_intern (sp, bmlipc_read_string (&bi));
        break;
      default:
        TRACE ("unhandled value type: %d", ret);
    }
  }
  return (ret ? 1 : 0);
}


const char *
bmlw_describe_global_value (BuzzMachineHandle * bmh, int const param,
    int const value)
{
  BmlIpcBuf bo = IPC_BUF_INIT;
  ssize_t size;
  int ret = 0;
  static const char *empty = "";
  static char desc[1024];

  bmlipc_write (&bo, "iiii", BM_DESCRIBE_GLOBAL_VALUE, (int) ((long) bmh),
      param, value);
  size = send (server_socket, bo.buffer, bo.size, MSG_NOSIGNAL);
  TRACE ("sent %d of %d bytes\n", size, bo.size);
  if (size > 0) {
    BmlIpcBuf bi = IPC_BUF_INIT;

    bi.size = (int) recv (server_socket, bi.buffer, IPC_BUF_SIZE, 0);
    TRACE ("got %d bytes\n", bi.size);
    if ((ret = bmlipc_read_int (&bi))) {
      strncpy (desc, bmlipc_read_string (&bi), 1024);
      desc[1023] = '\0';
    }
  }
  return ret ? desc : empty;
}

const char *
bmlw_describe_track_value (BuzzMachineHandle * bmh, int const param,
    int const value)
{
  BmlIpcBuf bo = IPC_BUF_INIT;
  ssize_t size;
  int ret = 0;
  static const char *empty = "";
  static char desc[1024];

  bmlipc_write (&bo, "iiii", BM_DESCRIBE_TRACK_VALUE, (int) ((long) bmh), param,
      value);
  size = send (server_socket, bo.buffer, bo.size, MSG_NOSIGNAL);
  TRACE ("sent %d of %d bytes\n", size, bo.size);
  if (size > 0) {
    BmlIpcBuf bi = IPC_BUF_INIT;

    bi.size = (int) recv (server_socket, bi.buffer, IPC_BUF_SIZE, 0);
    TRACE ("got %d bytes\n", bi.size);
    if ((ret = bmlipc_read_int (&bi))) {
      strncpy (desc, bmlipc_read_string (&bi), 1024);
      desc[1023] = '\0';
    }
  }
  return ret ? desc : empty;
}

// instance api

BuzzMachine *
bmlw_new (BuzzMachineHandle * bmh)
{
  BmlIpcBuf bo = IPC_BUF_INIT;
  ssize_t size;

  bmlipc_write (&bo, "ii", BM_NEW, (int) ((long) bmh));
  size = send (server_socket, bo.buffer, bo.size, MSG_NOSIGNAL);
  TRACE ("sent %d of %d bytes\n", size, bo.size);
  if (size > 0) {
    BmlIpcBuf bi = IPC_BUF_INIT;

    bi.size = (int) recv (server_socket, bi.buffer, IPC_BUF_SIZE, 0);
    TRACE ("got %d bytes\n", bi.size);
    return (BuzzMachine *) ((long) bmlipc_read_int (&bi));
  }
  return NULL;
}

void
bmlw_free (BuzzMachine * bm)
{
  BmlIpcBuf bo = IPC_BUF_INIT;
  ssize_t size;

  bmlipc_write (&bo, "ii", BM_FREE, (int) ((long) bm));
  size = send (server_socket, bo.buffer, bo.size, MSG_NOSIGNAL);
  TRACE ("sent %d of %d bytes\n", size, bo.size);
  if (size > 0) {
    BmlIpcBuf bi = IPC_BUF_INIT;

    bi.size = (int) recv (server_socket, bi.buffer, IPC_BUF_SIZE, 0);
    TRACE ("got %d bytes\n", bi.size);
  }
}


void
bmlw_init (BuzzMachine * bm, unsigned long blob_size, unsigned char *blob_data)
{
  BmlIpcBuf bo = IPC_BUF_INIT;
  ssize_t size;

  bmlipc_write (&bo, "iid", BM_INIT, (int) ((long) bm), (int) blob_size,
      (char *) blob_data);
  size = send (server_socket, bo.buffer, bo.size, MSG_NOSIGNAL);
  TRACE ("sent %d of %d bytes\n", size, bo.size);
  if (size > 0) {
    BmlIpcBuf bi = IPC_BUF_INIT;

    bi.size = (int) recv (server_socket, bi.buffer, IPC_BUF_SIZE, 0);
    TRACE ("got %d bytes\n", bi.size);
  }
}


int
bmlw_get_track_parameter_value (BuzzMachine * bm, int track, int index)
{
  BmlIpcBuf bo = IPC_BUF_INIT;
  ssize_t size;

  bmlipc_write (&bo, "iiii", BM_GET_TRACK_PARAMETER_VALUE, (int) ((long) bm),
      track, index);
  size = send (server_socket, bo.buffer, bo.size, MSG_NOSIGNAL);
  TRACE ("sent %d of %d bytes\n", size, bo.size);
  if (size > 0) {
    BmlIpcBuf bi = IPC_BUF_INIT;

    bi.size = (int) recv (server_socket, bi.buffer, IPC_BUF_SIZE, 0);
    TRACE ("got %d bytes\n", bi.size);
    return bmlipc_read_int (&bi);
  }
  return 0;
}

void
bmlw_set_track_parameter_value (BuzzMachine * bm, int track, int index,
    int value)
{
  BmlIpcBuf bo = IPC_BUF_INIT;
  ssize_t size;

  TRACE ("(%d,%d,%d)\n", track, index, value);
  bmlipc_write (&bo, "iiiii", BM_SET_TRACK_PARAMETER_VALUE, (int) ((long) bm),
      track, index, value);
  size = send (server_socket, bo.buffer, bo.size, MSG_NOSIGNAL);
  TRACE ("sent %d of %d bytes\n", size, bo.size);
  if (size > 0) {
    BmlIpcBuf bi = IPC_BUF_INIT;

    bi.size = (int) recv (server_socket, bi.buffer, IPC_BUF_SIZE, 0);
    TRACE ("got %d bytes\n", bi.size);
  }
}


int
bmlw_get_global_parameter_value (BuzzMachine * bm, int index)
{
  BmlIpcBuf bo = IPC_BUF_INIT;
  ssize_t size;

  bmlipc_write (&bo, "iii", BM_GET_GLOBAL_PARAMETER_VALUE, (int) ((long) bm),
      index);
  size = send (server_socket, bo.buffer, bo.size, MSG_NOSIGNAL);
  TRACE ("sent %d of %d bytes\n", size, bo.size);
  if (size > 0) {
    BmlIpcBuf bi = IPC_BUF_INIT;

    bi.size = (int) recv (server_socket, bi.buffer, IPC_BUF_SIZE, 0);
    TRACE ("got %d bytes\n", bi.size);
    return bmlipc_read_int (&bi);
  }
  return 0;
}

void
bmlw_set_global_parameter_value (BuzzMachine * bm, int index, int value)
{
  BmlIpcBuf bo = IPC_BUF_INIT;
  ssize_t size;

  TRACE ("(%d,%d)\n", index, value);
  bmlipc_write (&bo, "iiii", BM_SET_GLOBAL_PARAMETER_VALUE, (int) ((long) bm),
      index, value);
  size = send (server_socket, bo.buffer, bo.size, MSG_NOSIGNAL);
  TRACE ("sent %d of %d bytes\n", size, bo.size);
  if (size > 0) {
    BmlIpcBuf bi = IPC_BUF_INIT;

    bi.size = (int) recv (server_socket, bi.buffer, IPC_BUF_SIZE, 0);
    TRACE ("got %d bytes\n", bi.size);
  }
}


int
bmlw_get_attribute_value (BuzzMachine * bm, int index)
{
  BmlIpcBuf bo = IPC_BUF_INIT;
  ssize_t size;

  bmlipc_write (&bo, "iii", BM_GET_ATTRIBUTE_VALUE, (int) ((long) bm), index);
  size = send (server_socket, bo.buffer, bo.size, MSG_NOSIGNAL);
  TRACE ("sent %d of %d bytes\n", size, bo.size);
  if (size > 0) {
    BmlIpcBuf bi = IPC_BUF_INIT;

    bi.size = (int) recv (server_socket, bi.buffer, IPC_BUF_SIZE, 0);
    TRACE ("got %d bytes\n", bi.size);
    return bmlipc_read_int (&bi);
  }
  return 0;
}

void
bmlw_set_attribute_value (BuzzMachine * bm, int index, int value)
{
  BmlIpcBuf bo = IPC_BUF_INIT;
  ssize_t size;

  TRACE ("(%d,%d)\n", index, value);
  bmlipc_write (&bo, "iiii", BM_SET_ATTRIBUTE_VALUE, (int) ((long) bm), index,
      value);
  size = send (server_socket, bo.buffer, bo.size, MSG_NOSIGNAL);
  TRACE ("sent %d of %d bytes\n", size, bo.size);
  if (size > 0) {
    BmlIpcBuf bi = IPC_BUF_INIT;

    bi.size = (int) recv (server_socket, bi.buffer, IPC_BUF_SIZE, 0);
    TRACE ("got %d bytes\n", bi.size);
  }
}


void
bmlw_tick (BuzzMachine * bm)
{
  BmlIpcBuf bo = IPC_BUF_INIT;
  ssize_t size;

  bmlipc_write (&bo, "ii", BM_TICK, (int) ((long) bm));
  size = send (server_socket, bo.buffer, bo.size, MSG_NOSIGNAL);
  TRACE ("sent %d of %d bytes\n", size, bo.size);
  if (size > 0) {
    BmlIpcBuf bi = IPC_BUF_INIT;

    bi.size = (int) recv (server_socket, bi.buffer, IPC_BUF_SIZE, 0);
    TRACE ("got %d bytes\n", bi.size);
  }
}

int
bmlw_work (BuzzMachine * bm, float *psamples, int numsamples, int const mode)
{
  BmlIpcBuf bo = IPC_BUF_INIT;
  ssize_t size;
  int data_size = numsamples * sizeof (float);
  int ret = 0;

  bmlipc_write (&bo, "iidi", BM_WORK, (int) ((long) bm), data_size,
      (char *) psamples, mode);
  size = send (server_socket, bo.buffer, bo.size, MSG_NOSIGNAL);
  TRACE ("sent %d of %d bytes\n", size, bo.size);
  if (size > 0) {
    BmlIpcBuf bi = IPC_BUF_INIT;

    bi.size = (int) recv (server_socket, bi.buffer, IPC_BUF_SIZE, 0);
    TRACE ("got %d bytes\n", bi.size);
    bmlipc_read (&bi, sp, "id", &ret, &data_size, psamples);
    TRACE ("got %d bytes, data_size=%d\n", bi.size, data_size);
  }
  return ret;
}

int
bmlw_work_m2s (BuzzMachine * bm, float *pin, float *pout, int numsamples,
    int const mode)
{
  BmlIpcBuf bo = IPC_BUF_INIT;
  ssize_t size;
  int data_size = numsamples * sizeof (float);
  int ret = 0;

  bmlipc_write (&bo, "iidi", BM_WORK_M2S, (int) ((long) bm), data_size,
      (char *) pin, mode);
  size = send (server_socket, bo.buffer, bo.size, MSG_NOSIGNAL);
  TRACE ("sent %d of %d bytes\n", size, bo.size);
  if (size > 0) {
    BmlIpcBuf bi = IPC_BUF_INIT;

    bi.size = (int) recv (server_socket, bi.buffer, IPC_BUF_SIZE, 0);
    TRACE ("got %d bytes\n", bi.size);
    bmlipc_read (&bi, sp, "id", &ret, &data_size, pout);
    TRACE ("got %d bytes, data_size=%d\n", bi.size, data_size);
  }
  return ret;
}

void
bmlw_stop (BuzzMachine * bm)
{
  BmlIpcBuf bo = IPC_BUF_INIT;
  ssize_t size;

  bmlipc_write (&bo, "ii", BM_STOP, (int) ((long) bm));
  size = send (server_socket, bo.buffer, bo.size, MSG_NOSIGNAL);
  TRACE ("sent %d of %d bytes\n", size, bo.size);
  if (size > 0) {
    BmlIpcBuf bi = IPC_BUF_INIT;

    bi.size = (int) recv (server_socket, bi.buffer, IPC_BUF_SIZE, 0);
    TRACE ("got %d bytes\n", bi.size);
  }
}

void
bmlw_attributes_changed (BuzzMachine * bm)
{
  BmlIpcBuf bo = IPC_BUF_INIT;
  ssize_t size;

  bmlipc_write (&bo, "ii", BM_ATTRIBUTES_CHANGED, (int) ((long) bm));
  size = send (server_socket, bo.buffer, bo.size, MSG_NOSIGNAL);
  TRACE ("sent %d of %d bytes\n", size, bo.size);
  if (size > 0) {
    BmlIpcBuf bi = IPC_BUF_INIT;

    bi.size = (int) recv (server_socket, bi.buffer, IPC_BUF_SIZE, 0);
    TRACE ("got %d bytes\n", bi.size);
  }
}

void
bmlw_set_num_tracks (BuzzMachine * bm, int num)
{
  BmlIpcBuf bo = IPC_BUF_INIT;
  ssize_t size;

  bmlipc_write (&bo, "iii", BM_ATTRIBUTES_CHANGED, (int) ((long) bm), num);
  size = send (server_socket, bo.buffer, bo.size, MSG_NOSIGNAL);
  TRACE ("sent %d of %d bytes\n", size, bo.size);
  if (size > 0) {
    BmlIpcBuf bi = IPC_BUF_INIT;

    bi.size = (int) recv (server_socket, bi.buffer, IPC_BUF_SIZE, 0);
    TRACE ("got %d bytes\n", bi.size);
  }
}

void
bmlw_set_callbacks (BuzzMachine * bm, CHostCallbacks * callbacks)
{
  // TODO(ensonic): maybe a proxy
}

#endif /* USE_DLLWRAPPER_IPC */

int
bml_setup (void)
{
#ifdef USE_DEBUG
  const char *debug_log_flag_str = getenv ("BML_DEBUG");
  const int debug_log_flags =
      debug_log_flag_str ? atoi (debug_log_flag_str) : 0;
#endif
  BMLDebugLogger logger;

  logger = TRACE_INIT (debug_log_flags);
  TRACE ("beg\n");

#ifdef USE_DLLWRAPPER_DIRECT
  if (!_bmlw_setup (logger)) {
    return FALSE;
  }
#endif /* USE_DLLWRAPPER_DIRECT */
#ifdef USE_DLLWRAPPER_IPC
  // TODO(ensonic): /tmp is not ideal as everyone can read/write there and could
  // steal the socket, we could create a user-owned subdir first to mitigate
#ifdef DEV_BUILD
  if (getenv ("BMLIPC_DEBUG")) {
    // allows to run this manually, we will then not spawn a new one here
    snprintf (socket_file, (SOCKET_PATH_MAX - 1), "bml.sock");
  } else {
#endif
    snprintf (socket_file, (SOCKET_PATH_MAX - 1), "bml.%d", (int) getpid ());
#ifdef DEV_BUILD
  }
#endif
  if (!bmpipc_connect ()) {
    return FALSE;
  }

  sp = sp_new (25);
#endif /* USE_DLLWRAPPER_IPC */

  if (!(emu_so = dlopen (NATIVE_BML_DIR "/libbuzzmachineloader.so", RTLD_LAZY))) {
    TRACE ("   failed to load native bml : %s\n", dlerror ());
    return (FALSE);
  }
  TRACE ("   native bml loaded\n");

  if (!(bmln_set_logger = (BMSetLogger) dlsym (emu_so, "bm_set_logger"))) {
    TRACE ("bm_set_logger is missing\n");
    return (FALSE);
  }

  if (!(bmln_set_master_info =
          (BMSetMasterInfo) dlsym (emu_so, "bm_set_master_info"))) {
    TRACE ("bm_set_master_info is missing\n");
    return (FALSE);
  }


  if (!(bmln_open = (BMOpen) dlsym (emu_so, "bm_open"))) {
    TRACE ("bm_open is missing\n");
    return (FALSE);
  }
  if (!(bmln_close = (BMClose) dlsym (emu_so, "bm_close"))) {
    TRACE ("bm_close is missing\n");
    return (FALSE);
  }

  if (!(bmln_get_machine_info =
          (BMGetMachineInfo) dlsym (emu_so, "bm_get_machine_info"))) {
    TRACE ("bm_get_machine_info is missing\n");
    return (FALSE);
  }
  if (!(bmln_get_global_parameter_info =
          (BMGetGlobalParameterInfo) dlsym (emu_so,
              "bm_get_global_parameter_info"))) {
    TRACE ("bm_get_global_parameter_info is missing\n");
    return (FALSE);
  }
  if (!(bmln_get_track_parameter_info =
          (BMGetTrackParameterInfo) dlsym (emu_so,
              "bm_get_track_parameter_info"))) {
    TRACE ("bm_get_track_parameter_info is missing\n");
    return (FALSE);
  }
  if (!(bmln_get_attribute_info =
          (BMGetAttributeInfo) dlsym (emu_so, "bm_get_attribute_info"))) {
    TRACE ("bm_get_attribute_info is missing\n");
    return (FALSE);
  }

  if (!(bmln_describe_global_value =
          (BMDescribeGlobalValue) dlsym (emu_so, "bm_describe_global_value"))) {
    TRACE ("bm_describe_global_value is missing\n");
    return (FALSE);
  }
  if (!(bmln_describe_track_value =
          (BMDescribeTrackValue) dlsym (emu_so, "bm_describe_track_value"))) {
    TRACE ("bm_describe_track_value is missing\n");
    return (FALSE);
  }


  if (!(bmln_new = (BMNew) dlsym (emu_so, "bm_new"))) {
    TRACE ("bm_new is missing\n");
    return (FALSE);
  }
  if (!(bmln_free = (BMFree) dlsym (emu_so, "bm_free"))) {
    TRACE ("bm_free is missing\n");
    return (FALSE);
  }

  if (!(bmln_init = (BMInit) dlsym (emu_so, "bm_init"))) {
    TRACE ("bm_init is missing\n");
    return (FALSE);
  }

  if (!(bmln_get_track_parameter_value =
          (BMGetTrackParameterValue) dlsym (emu_so,
              "bm_get_track_parameter_value"))) {
    TRACE ("bm_get_track_parameter_value is missing\n");
    return (FALSE);
  }
  if (!(bmln_set_track_parameter_value =
          (BMSetTrackParameterValue) dlsym (emu_so,
              "bm_set_track_parameter_value"))) {
    TRACE ("bm_set_track_parameter_value is missing\n");
    return (FALSE);
  }

  if (!(bmln_get_global_parameter_value =
          (BMGetGlobalParameterValue) dlsym (emu_so,
              "bm_get_global_parameter_value"))) {
    TRACE ("bm_get_global_parameter_value is missing\n");
    return (FALSE);
  }
  if (!(bmln_set_global_parameter_value =
          (BMSetGlobalParameterValue) dlsym (emu_so,
              "bm_set_global_parameter_value"))) {
    TRACE ("bm_set_global_parameter_value is missing\n");
    return (FALSE);
  }

  if (!(bmln_get_attribute_value =
          (BMGetAttributeValue) dlsym (emu_so, "bm_get_attribute_value"))) {
    TRACE ("bm_get_attribute_value is missing\n");
    return (FALSE);
  }
  if (!(bmln_set_attribute_value =
          (BMSetAttributeValue) dlsym (emu_so, "bm_set_attribute_value"))) {
    TRACE ("bm_set_attribute_value is missing\n");
    return (FALSE);
  }

  if (!(bmln_tick = (BMTick) dlsym (emu_so, "bm_tick"))) {
    TRACE ("bm_tick is missing\n");
    return (FALSE);
  }
  if (!(bmln_work = (BMWork) dlsym (emu_so, "bm_work"))) {
    TRACE ("bm_work is missing\n");
    return (FALSE);
  }
  if (!(bmln_work_m2s = (BMWorkM2S) dlsym (emu_so, "bm_work_m2s"))) {
    TRACE ("bm_work_m2s is missing\n");
    return (FALSE);
  }
  if (!(bmln_stop = (BMStop) dlsym (emu_so, "bm_stop"))) {
    TRACE ("bm_stop is missing\n");
    return (FALSE);
  }

  if (!(bmln_attributes_changed =
          (BMAttributesChanged) dlsym (emu_so, "bm_attributes_changed"))) {
    TRACE ("bm_attributes_changed is missing\n");
    return (FALSE);
  }

  if (!(bmln_set_num_tracks =
          (BMSetNumTracks) dlsym (emu_so, "bm_set_num_tracks"))) {
    TRACE ("bm_set_num_tracks is missing\n");
    return (FALSE);
  }

  if (!(bmln_set_callbacks =
          (BMSetCallbacks) dlsym (emu_so, "bm_set_callbacks"))) {
    TRACE ("bm_set_callbacks is missing\n");
    return (FALSE);
  }

  TRACE ("   symbols connected\n");
  bmln_set_logger (logger);

  return TRUE;
}

void
bml_finalize (void)
{
#ifdef USE_DLLWRAPPER_DIRECT
  _bmlw_finalize ();
#endif /* USE_DLLWRAPPER_DIRECT */
#ifdef USE_DLLWRAPPER_IPC
  TRACE ("string pool size: %d\n", sp_get_count (sp));
  sp_delete (sp);
  TRACE ("closing socket\n");
  //shutdown(server_socket,SHUT_RDWR);
  close (server_socket);
#endif /* USE_DLLWRAPPER_IPC */
  dlclose (emu_so);
  TRACE ("bml unloaded\n");
}
