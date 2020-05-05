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
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bmllog.h"
#include "bmlipc.h"
#include "bmlw.h"

static void
_bmlw_set_master_info (BmlIpcBuf * bi, BmlIpcBuf * bo)
{
  long bpm = bmlipc_read_int (bi);
  long tpb = bmlipc_read_int (bi);
  long srat = bmlipc_read_int (bi);
  TRACE ("bmlw_set_master_info(%ld,%ld,%ld)\n", bpm, tpb, srat);
  bmlw_set_master_info (bpm, tpb, srat);
  bmlipc_write_int (bo, 0);
}

static void
_bmlw_open (BmlIpcBuf * bi, BmlIpcBuf * bo)
{
  char *bm_file_name = bmlipc_read_string (bi);
  TRACE ("bmlw_open('%s')\n", bm_file_name);
  BuzzMachineHandle *bmh = bmlw_open (bm_file_name);
  TRACE ("bmlw_open('%s')=%p\n", bm_file_name, bmh);
  bmlipc_write_int (bo, (int) bmh);
}

static void
_bmlw_close (BmlIpcBuf * bi, BmlIpcBuf * bo)
{
  BuzzMachineHandle *bmh = (BuzzMachineHandle *) bmlipc_read_int (bi);
  bmlw_close (bmh);
  bmlipc_write_int (bo, 0);
}

static void
_bmlw_get_machine_info (BmlIpcBuf * bi, BmlIpcBuf * bo)
{
  BuzzMachineHandle *bmh = (BuzzMachineHandle *) bmlipc_read_int (bi);
  BuzzMachineProperty key = bmlipc_read_int (bi);
  int ival, ret;
  char *sval;
  switch (key) {
    case BM_PROP_TYPE:
    case BM_PROP_VERSION:
    case BM_PROP_FLAGS:
    case BM_PROP_MIN_TRACKS:
    case BM_PROP_MAX_TRACKS:
    case BM_PROP_NUM_GLOBAL_PARAMS:
    case BM_PROP_NUM_TRACK_PARAMS:
    case BM_PROP_NUM_ATTRIBUTES:
    case BM_PROP_NUM_INPUT_CHANNELS:
    case BM_PROP_NUM_OUTPUT_CHANNELS:
      ret = bmlw_get_machine_info (bmh, key, &ival);
      bmlipc_write_int (bo, (ret ? 1 : 0));
      bmlipc_write_int (bo, ival);
      break;
    case BM_PROP_NAME:
    case BM_PROP_SHORT_NAME:
    case BM_PROP_AUTHOR:
    case BM_PROP_COMMANDS:
    case BM_PROP_DLL_NAME:
      ret = bmlw_get_machine_info (bmh, key, &sval);
      bmlipc_write_int (bo, (ret ? 2 : 0));
      if (sval) {
        bmlipc_write_string (bo, sval);
      }
      break;
  }
}

static void
_bmlw_get_global_parameter_info (BmlIpcBuf * bi, BmlIpcBuf * bo)
{
  BuzzMachineHandle *bmh = (BuzzMachineHandle *) bmlipc_read_int (bi);
  int index = bmlipc_read_int (bi);
  BuzzMachineParameter key = bmlipc_read_int (bi);
  int ival, ret;
  char *sval;

  switch (key) {
    case BM_PARA_TYPE:
    case BM_PARA_MIN_VALUE:
    case BM_PARA_MAX_VALUE:
    case BM_PARA_NO_VALUE:
    case BM_PARA_FLAGS:
    case BM_PARA_DEF_VALUE:
      ret = bmlw_get_global_parameter_info (bmh, index, key, &ival);
      bmlipc_write_int (bo, (ret ? 1 : 0));
      bmlipc_write_int (bo, ival);
      break;
    case BM_PARA_NAME:
    case BM_PARA_DESCRIPTION:
      ret = bmlw_get_global_parameter_info (bmh, index, key, &sval);
      bmlipc_write_int (bo, (ret ? 2 : 0));
      if (sval) {
        bmlipc_write_string (bo, sval);
      }
      break;
  }
}

static void
_bmlw_get_track_parameter_info (BmlIpcBuf * bi, BmlIpcBuf * bo)
{
  BuzzMachineHandle *bmh = (BuzzMachineHandle *) bmlipc_read_int (bi);
  int index = bmlipc_read_int (bi);
  BuzzMachineParameter key = bmlipc_read_int (bi);
  int ival, ret;
  char *sval;

  switch (key) {
    case BM_PARA_TYPE:
    case BM_PARA_MIN_VALUE:
    case BM_PARA_MAX_VALUE:
    case BM_PARA_NO_VALUE:
    case BM_PARA_FLAGS:
    case BM_PARA_DEF_VALUE:
      ret = bmlw_get_track_parameter_info (bmh, index, key, &ival);
      bmlipc_write_int (bo, (ret ? 1 : 0));
      bmlipc_write_int (bo, ival);
      break;
    case BM_PARA_NAME:
    case BM_PARA_DESCRIPTION:
      ret = bmlw_get_track_parameter_info (bmh, index, key, &sval);
      bmlipc_write_int (bo, (ret ? 2 : 0));
      if (sval) {
        bmlipc_write_string (bo, sval);
      }
      break;
  }
}

static void
_bmlw_get_attribute_info (BmlIpcBuf * bi, BmlIpcBuf * bo)
{
  BuzzMachineHandle *bmh = (BuzzMachineHandle *) bmlipc_read_int (bi);
  int index = bmlipc_read_int (bi);
  BuzzMachineAttribute key = bmlipc_read_int (bi);
  int ival, ret;
  char *sval;

  switch (key) {
    case BM_ATTR_MIN_VALUE:
    case BM_ATTR_MAX_VALUE:
    case BM_ATTR_DEF_VALUE:
      ret = bmlw_get_attribute_info (bmh, index, key, &ival);
      bmlipc_write_int (bo, (ret ? 1 : 0));
      bmlipc_write_int (bo, ival);
      break;
    case BM_ATTR_NAME:
      ret = bmlw_get_attribute_info (bmh, index, key, &sval);
      bmlipc_write_int (bo, (ret ? 2 : 0));
      if (sval) {
        bmlipc_write_string (bo, sval);
      }
      break;
  }
}

static void
_bmlw_describe_global_value (BmlIpcBuf * bi, BmlIpcBuf * bo)
{
  BuzzMachineHandle *bmh = (BuzzMachineHandle *) bmlipc_read_int (bi);
  int param = bmlipc_read_int (bi);
  int value = bmlipc_read_int (bi);
  char *res = (char *) bmlw_describe_global_value (bmh, param, value);
  bmlipc_write_int (bo, (res != NULL));
  if (res) {
    bmlipc_write_string (bo, res);
  }
}

static void
_bmlw_describe_track_value (BmlIpcBuf * bi, BmlIpcBuf * bo)
{
  BuzzMachineHandle *bmh = (BuzzMachineHandle *) bmlipc_read_int (bi);
  int param = bmlipc_read_int (bi);
  int value = bmlipc_read_int (bi);
  char *res = (char *) bmlw_describe_track_value (bmh, param, value);
  bmlipc_write_int (bo, (res != NULL));
  if (res) {
    bmlipc_write_string (bo, res);
  }
}

static void
_bmlw_new (BmlIpcBuf * bi, BmlIpcBuf * bo)
{
  BuzzMachineHandle *bmh = (BuzzMachineHandle *) bmlipc_read_int (bi);
  BuzzMachine *bm = bmlw_new (bmh);
  bmlipc_write_int (bo, (int) bm);
}

static void
_bmlw_free (BmlIpcBuf * bi, BmlIpcBuf * bo)
{
  BuzzMachine *bm = (BuzzMachine *) bmlipc_read_int (bi);
  bmlw_free (bm);
  bmlipc_write_int (bo, 0);
}

static void
_bmlw_init (BmlIpcBuf * bi, BmlIpcBuf * bo)
{
  BuzzMachine *bm = (BuzzMachine *) bmlipc_read_int (bi);
  unsigned long blob_size = bmlipc_read_int (bi);
  unsigned char *blob_data =
      (unsigned char *) bmlipc_read_data (bi, (int) blob_size);
  bmlw_init (bm, blob_size, blob_data);
  bmlipc_write_int (bo, 0);
}

static void
_bmlw_get_track_parameter_value (BmlIpcBuf * bi, BmlIpcBuf * bo)
{
  BuzzMachine *bm = (BuzzMachine *) bmlipc_read_int (bi);
  int track = bmlipc_read_int (bi);
  int index = bmlipc_read_int (bi);
  int value = bmlw_get_track_parameter_value (bm, track, index);
  bmlipc_write_int (bo, value);
}

static void
_bmlw_set_track_parameter_value (BmlIpcBuf * bi, BmlIpcBuf * bo)
{
  BuzzMachine *bm = (BuzzMachine *) bmlipc_read_int (bi);
  int track = bmlipc_read_int (bi);
  int index = bmlipc_read_int (bi);
  int value = bmlipc_read_int (bi);
  TRACE ("(%d,%d,%d)\n", track, index, value);
  bmlw_set_track_parameter_value (bm, track, index, value);
  bmlipc_write_int (bo, 0);
}

static void
_bmlw_get_global_parameter_value (BmlIpcBuf * bi, BmlIpcBuf * bo)
{
  BuzzMachine *bm = (BuzzMachine *) bmlipc_read_int (bi);
  int index = bmlipc_read_int (bi);
  int value = bmlw_get_global_parameter_value (bm, index);
  bmlipc_write_int (bo, value);
}

static void
_bmlw_set_global_parameter_value (BmlIpcBuf * bi, BmlIpcBuf * bo)
{
  BuzzMachine *bm = (BuzzMachine *) bmlipc_read_int (bi);
  int index = bmlipc_read_int (bi);
  int value = bmlipc_read_int (bi);
  TRACE ("(%d,%d)\n", index, value);
  bmlw_set_global_parameter_value (bm, index, value);
  bmlipc_write_int (bo, 0);
}

static void
_bmlw_get_attribute_value (BmlIpcBuf * bi, BmlIpcBuf * bo)
{
  BuzzMachine *bm = (BuzzMachine *) bmlipc_read_int (bi);
  int index = bmlipc_read_int (bi);
  int value = bmlw_get_attribute_value (bm, index);
  bmlipc_write_int (bo, value);
}

static void
_bmlw_set_attribute_value (BmlIpcBuf * bi, BmlIpcBuf * bo)
{
  BuzzMachine *bm = (BuzzMachine *) bmlipc_read_int (bi);
  int index = bmlipc_read_int (bi);
  int value = bmlipc_read_int (bi);
  bmlw_set_attribute_value (bm, index, value);
  bmlipc_write_int (bo, 0);
}

static void
_bmlw_tick (BmlIpcBuf * bi, BmlIpcBuf * bo)
{
  BuzzMachine *bm = (BuzzMachine *) bmlipc_read_int (bi);
  bmlw_tick (bm);
  bmlipc_write_int (bo, 0);
}

static void
_bmlw_work (BmlIpcBuf * bi, BmlIpcBuf * bo)
{
  BuzzMachine *bm = (BuzzMachine *) bmlipc_read_int (bi);
  int size = bmlipc_read_int (bi);
  float *psamples = (float *) bmlipc_read_data (bi, size);
  int mode = bmlipc_read_int (bi);
  int numsamples = size / sizeof (float);
  int ret = bmlw_work (bm, psamples, numsamples, mode);
  // DEBUG
#if 0
  {
    FILE *df;
    if ((df = fopen ("/tmp/bmlout_m.raw", "ab"))) {
      fwrite (psamples, size, 1, df);
      fclose (df);
    }
  }
#endif
  // DEBUG
  TRACE ("processed size=%d/numsamples=%d, in mode=%d\n", size, numsamples,
      mode);
  bmlipc_write_int (bo, ret);
  bmlipc_write_int (bo, size);
  bmlipc_write_data (bo, size, (char *) psamples);
}

static void
_bmlw_work_m2s (BmlIpcBuf * bi, BmlIpcBuf * bo)
{
  BuzzMachine *bm = (BuzzMachine *) bmlipc_read_int (bi);
  int size = bmlipc_read_int (bi);
  float *pin = (float *) bmlipc_read_data (bi, size);
  int mode = bmlipc_read_int (bi);
  float pout[256 + 256] = { 0.0, };     // MAX_BUFFER_LENGTH = 256
  int numsamples = size / sizeof (float);
  int ret = bmlw_work_m2s (bm, pin, pout, numsamples, mode);
  size += size;
  // DEBUG
#if 0
  {
    FILE *df;
    if ((df = fopen ("/tmp/bmlout_s.raw", "ab"))) {
      fwrite (pout, size, 1, df);
      fclose (df);
    }
  }
#endif
  // DEBUG
  TRACE ("processed size=%d/numsamples=%d, in mode=%d\n", size, numsamples,
      mode);
  bmlipc_write_int (bo, ret);
  bmlipc_write_int (bo, size);
  bmlipc_write_data (bo, size, (char *) pout);
}

static void
_bmlw_stop (BmlIpcBuf * bi, BmlIpcBuf * bo)
{
  BuzzMachine *bm = (BuzzMachine *) bmlipc_read_int (bi);
  bmlw_stop (bm);
  bmlipc_write_int (bo, 0);
}

static void
_bmlw_attributes_changed (BmlIpcBuf * bi, BmlIpcBuf * bo)
{
  BuzzMachine *bm = (BuzzMachine *) bmlipc_read_int (bi);
  bmlw_attributes_changed (bm);
  bmlipc_write_int (bo, 0);
}

static void
_bmlw_set_num_tracks (BmlIpcBuf * bi, BmlIpcBuf * bo)
{
  BuzzMachine *bm = (BuzzMachine *) bmlipc_read_int (bi);
  int num = bmlipc_read_int (bi);
  bmlw_set_num_tracks (bm, num);
  bmlipc_write_int (bo, 0);
}

int
main (int argc, char **argv)
{
  char *socket_file = NULL;
  const char *debug_log_flag_str = getenv ("BML_DEBUG");
  const int debug_log_flags =
      debug_log_flag_str ? atoi (debug_log_flag_str) : 0;
  BMLDebugLogger logger;
  int server_socket, client_socket;
  socklen_t addrlen;
  ssize_t size;
  struct sockaddr_un address = { 0, };
  int running = TRUE;
  BmlIpcBuf bo = IPC_BUF_INIT, bi = IPC_BUF_INIT;
  BmAPI id;

  logger = TRACE_INIT (debug_log_flags);
  TRACE ("beg\n");

  if (argc > 1) {
    socket_file = argv[1];
  } else {
    fprintf (stderr, "Usage: bmlhost <socket file>\n");
    return EXIT_FAILURE;
  }
  TRACE ("socket file: '%s'\n", socket_file);

  if (!_bmlw_setup (logger)) {
    TRACE ("bmlw setup failed\n");
    return EXIT_FAILURE;
  }
  // TODO: maybe switch to SOCK_SEQPACKET
  if ((server_socket = socket (PF_LOCAL, SOCK_STREAM, 0)) > 0) {
    TRACE ("server socket created\n");
  }

  unlink (socket_file);

  memset (&address, 0, sizeof (struct sockaddr_un));
  address.sun_family = PF_LOCAL;
  strncpy (address.sun_path, socket_file, sizeof (address.sun_path) - 1);
  if (bind (server_socket, (struct sockaddr *) &address,
          sizeof (struct sockaddr_un)) == -1) {
    TRACE ("socket path already in use!\n");
  }
  // number of pending connections
  // upper limmit is /proc/sys/net/core/somaxconn usually 128 == SOMAXCONN
  // right we just have one anyway
  listen (server_socket, /* backlog of pending connections */ SOMAXCONN);
  addrlen = sizeof (struct sockaddr_in);
  client_socket =
      accept (server_socket, (struct sockaddr *) &address, &addrlen);
  if (client_socket > 0) {
    TRACE ("client connected\n");
  }
  while (running) {
    TRACE ("waiting for command ====================\n");
    bmlipc_clear (&bi);
    size = recv (client_socket, bi.buffer, IPC_BUF_SIZE, 0);
    if (size == 0) {
      TRACE ("got EOF\n");
      running = FALSE;
      continue;
    }
    if (size == -1) {
      TRACE ("ERROR: recv returned %d: %s\n", errno, strerror (errno));
      // TODO(ensonic): specific action depending on error
      continue;
    }
    bi.size = (int) size;
    TRACE ("got %d bytes\n", bi.size);
    // parse message
    id = bmlipc_read_int (&bi);
    if (bi.io_error) {
      TRACE ("message should be at least 4 bytes");
      continue;
    }
    TRACE ("command: %d\n", id);
    bmlipc_clear (&bo);
    switch (id) {
      case 0:
        running = FALSE;
        break;
      case BM_SET_MASTER_INFO:
        _bmlw_set_master_info (&bi, &bo);
        break;
      case BM_OPEN:
        _bmlw_open (&bi, &bo);
        break;
      case BM_CLOSE:
        _bmlw_close (&bi, &bo);
        break;
      case BM_GET_MACHINE_INFO:
        _bmlw_get_machine_info (&bi, &bo);
        break;
      case BM_GET_GLOBAL_PARAMETER_INFO:
        _bmlw_get_global_parameter_info (&bi, &bo);
        break;
      case BM_GET_TRACK_PARAMETER_INFO:
        _bmlw_get_track_parameter_info (&bi, &bo);
        break;
      case BM_GET_ATTRIBUTE_INFO:
        _bmlw_get_attribute_info (&bi, &bo);
        break;
      case BM_DESCRIBE_GLOBAL_VALUE:
        _bmlw_describe_global_value (&bi, &bo);
        break;
      case BM_DESCRIBE_TRACK_VALUE:
        _bmlw_describe_track_value (&bi, &bo);
        break;
      case BM_NEW:
        _bmlw_new (&bi, &bo);
        break;
      case BM_FREE:
        _bmlw_free (&bi, &bo);
        break;
      case BM_INIT:
        _bmlw_init (&bi, &bo);
        break;
      case BM_GET_TRACK_PARAMETER_VALUE:
        _bmlw_get_track_parameter_value (&bi, &bo);
        break;
      case BM_SET_TRACK_PARAMETER_VALUE:
        _bmlw_set_track_parameter_value (&bi, &bo);
        break;
      case BM_GET_GLOBAL_PARAMETER_VALUE:
        _bmlw_get_global_parameter_value (&bi, &bo);
        break;
      case BM_SET_GLOBAL_PARAMETER_VALUE:
        _bmlw_set_global_parameter_value (&bi, &bo);
        break;
      case BM_GET_ATTRIBUTE_VALUE:
        _bmlw_get_attribute_value (&bi, &bo);
        break;
      case BM_SET_ATTRIBUTE_VALUE:
        _bmlw_set_attribute_value (&bi, &bo);
        break;
      case BM_TICK:
        _bmlw_tick (&bi, &bo);
        break;
      case BM_WORK:
        _bmlw_work (&bi, &bo);
        break;
      case BM_WORK_M2S:
        _bmlw_work_m2s (&bi, &bo);
        break;
      case BM_STOP:
        _bmlw_stop (&bi, &bo);
        break;
      case BM_ATTRIBUTES_CHANGED:
        _bmlw_attributes_changed (&bi, &bo);
        break;
      case BM_SET_NUM_TRACKS:
        _bmlw_set_num_tracks (&bi, &bo);
        break;
      case BM_SET_CALLBACKS:
        TRACE ("FIXME");
        break;
      default:
        break;
    }
    if (bo.size) {
      size = send (client_socket, bo.buffer, bo.size, MSG_NOSIGNAL);
      TRACE ("sent %d of %d bytes\n", size, bo.size);
      if (size == -1) {
        TRACE ("ERROR: send returned %d: %s\n", errno, strerror (errno));
        // TODO(ensonic): specific action depending on error
      }
    }
  }
  close (client_socket);
  close (server_socket);
  unlink (socket_file);

  _bmlw_finalize ();
  TRACE ("end\n");
  return EXIT_SUCCESS;
}
