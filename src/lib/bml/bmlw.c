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

#include <stdio.h>

#include "win32.h"
#include "windef.h"
#include "ldt_keeper.h"

#define BML_C
#include "bml.h"
#include "bmllog.h"

static HINSTANCE emu_dll = 0L;
static ldt_fs_t *ldt_fs;
#define LoadDLL(name) LoadLibraryA(name)
#define GetSymbol(dll,name) GetProcAddress(dll,name)
#define FreeDLL(dll) FreeLibrary(dll)
#define BMLX(a) fptr_ ## a

// FIXME: without the mutex and the Check_FS_Segment() things are obviously faster
// it seems to work more or less, but some songs crash when disposing them without
// without the Check_FS_Segment(ldt_fs)
#if 0
static pthread_mutex_t ldt_mutex = PTHREAD_MUTEX_INITIALIZER;
#define win32_prolog(_nop_) \
  pthread_mutex_lock(&ldt_mutex); \
  Check_FS_Segment(ldt_fs)

#define win32_eliplog(_nop_) \
  pthread_mutex_unlock(&ldt_mutex)
#else
#define win32_prolog(_nop_) \
  Check_FS_Segment(ldt_fs)

#define win32_eliplog(_nop_) \
  do {} while(0)
#endif

// windows plugin API method pointers (called through local wrappers)
BMSetLogger BMLX (bmlw_set_logger);
BMSetMasterInfo BMLX (bmlw_set_master_info);

BMOpen BMLX (bmlw_open);
BMClose BMLX (bmlw_close);

BMGetMachineInfo BMLX (bmlw_get_machine_info);
BMGetGlobalParameterInfo BMLX (bmlw_get_global_parameter_info);
BMGetTrackParameterInfo BMLX (bmlw_get_track_parameter_info);
BMGetAttributeInfo BMLX (bmlw_get_attribute_info);

BMNew BMLX (bmlw_new);
BMFree BMLX (bmlw_free);

BMInit BMLX (bmlw_init);

BMGetTrackParameterValue BMLX (bmlw_get_track_parameter_value);
BMSetTrackParameterValue BMLX (bmlw_set_track_parameter_value);

BMGetGlobalParameterValue BMLX (bmlw_get_global_parameter_value);
BMSetGlobalParameterValue BMLX (bmlw_set_global_parameter_value);

BMGetAttributeValue BMLX (bmlw_get_attribute_value);
BMSetAttributeValue BMLX (bmlw_set_attribute_value);

BMTick BMLX (bmlw_tick);
BMWork BMLX (bmlw_work);
BMWorkM2S BMLX (bmlw_work_m2s);
BMStop BMLX (bmlw_stop);

BMAttributesChanged BMLX (bmlw_attributes_changed);

BMSetNumTracks BMLX (bmlw_set_num_tracks);

BMDescribeGlobalValue BMLX (bmlw_describe_global_value);
BMDescribeTrackValue BMLX (bmlw_describe_track_value);

BMSetCallbacks BMLX (bmlw_set_callbacks);

// passthrough functions

// global API

void
bmlw_set_master_info (long bpm, long tpb, long srat)
{
  win32_prolog ();
  BMLX (bmlw_set_master_info (bpm, tpb, srat));
  win32_eliplog ();
}

// library api

BuzzMachineHandle *
bmlw_open (char *bm_file_name)
{
  BuzzMachineHandle *bmh;

  win32_prolog ();
  bmh = BMLX (bmlw_open (bm_file_name));
  win32_eliplog ();
  return (bmh);
}

void
bmlw_close (BuzzMachineHandle * bmh)
{
  win32_prolog ();
  BMLX (bmlw_close (bmh));
  win32_eliplog ();
}


int
bmlw_get_machine_info (BuzzMachineHandle * bmh, BuzzMachineProperty key,
    void *value)
{
  int ret;

  win32_prolog ();
  ret = BMLX (bmlw_get_machine_info (bmh, key, value));
  win32_eliplog ();
  return (ret);
}

int
bmlw_get_global_parameter_info (BuzzMachineHandle * bmh, int index,
    BuzzMachineParameter key, void *value)
{
  int ret;

  win32_prolog ();
  ret = BMLX (bmlw_get_global_parameter_info (bmh, index, key, value));
  win32_eliplog ();
  return (ret);
}

int
bmlw_get_track_parameter_info (BuzzMachineHandle * bmh, int index,
    BuzzMachineParameter key, void *value)
{
  int ret;

  win32_prolog ();
  ret = BMLX (bmlw_get_track_parameter_info (bmh, index, key, value));
  win32_eliplog ();
  return (ret);
}

int
bmlw_get_attribute_info (BuzzMachineHandle * bmh, int index,
    BuzzMachineAttribute key, void *value)
{
  int ret;

  win32_prolog ();
  ret = BMLX (bmlw_get_attribute_info (bmh, index, key, value));
  win32_eliplog ();
  return (ret);
}


const char *
bmlw_describe_global_value (BuzzMachineHandle * bmh, int const param,
    int const value)
{
  const char *ret;

  win32_prolog ();
  ret = BMLX (bmlw_describe_global_value (bmh, param, value));
  win32_eliplog ();
  return (ret);
}

const char *
bmlw_describe_track_value (BuzzMachineHandle * bmh, int const param,
    int const value)
{
  const char *ret;

  win32_prolog ();
  ret = BMLX (bmlw_describe_track_value (bmh, param, value));
  win32_eliplog ();
  return (ret);
}


// instance api

BuzzMachine *
bmlw_new (BuzzMachineHandle * bmh)
{
  BuzzMachine *bm;

  win32_prolog ();
  bm = BMLX (bmlw_new (bmh));
  win32_eliplog ();
  return (bm);
}

void
bmlw_free (BuzzMachine * bm)
{
  win32_prolog ();
  BMLX (bmlw_free (bm));
  win32_eliplog ();
}


void
bmlw_init (BuzzMachine * bm, unsigned long blob_size, unsigned char *blob_data)
{
  win32_prolog ();
  BMLX (bmlw_init (bm, blob_size, blob_data));
  win32_eliplog ();
}


int
bmlw_get_track_parameter_value (BuzzMachine * bm, int track, int index)
{
  int ret;

  win32_prolog ();
  ret = BMLX (bmlw_get_track_parameter_value (bm, track, index));
  win32_eliplog ();
  return (ret);
}

void
bmlw_set_track_parameter_value (BuzzMachine * bm, int track, int index,
    int value)
{
  win32_prolog ();
  BMLX (bmlw_set_track_parameter_value (bm, track, index, value));
  win32_eliplog ();
}


int
bmlw_get_global_parameter_value (BuzzMachine * bm, int index)
{
  int ret;

  win32_prolog ();
  ret = BMLX (bmlw_get_global_parameter_value (bm, index));
  win32_eliplog ();
  return (ret);
}

void
bmlw_set_global_parameter_value (BuzzMachine * bm, int index, int value)
{
  win32_prolog ();
  BMLX (bmlw_set_global_parameter_value (bm, index, value));
  win32_eliplog ();
}


int
bmlw_get_attribute_value (BuzzMachine * bm, int index)
{
  int ret;

  win32_prolog ();
  ret = BMLX (bmlw_get_attribute_value (bm, index));
  win32_eliplog ();
  return (ret);
}

void
bmlw_set_attribute_value (BuzzMachine * bm, int index, int value)
{
  win32_prolog ();
  BMLX (bmlw_set_attribute_value (bm, index, value));
  win32_eliplog ();
}


void
bmlw_tick (BuzzMachine * bm)
{
  win32_prolog ();
  BMLX (bmlw_tick (bm));
  win32_eliplog ();
}

int
bmlw_work (BuzzMachine * bm, float *psamples, int numsamples, int const mode)
{
  int ret;

  win32_prolog ();
  ret = BMLX (bmlw_work (bm, psamples, numsamples, mode));
  win32_eliplog ();
  return (ret);
}

int
bmlw_work_m2s (BuzzMachine * bm, float *pin, float *pout, int numsamples,
    int const mode)
{
  int ret;

  win32_prolog ();
  ret = BMLX (bmlw_work_m2s (bm, pin, pout, numsamples, mode));
  win32_eliplog ();
  return (ret);
}

void
bmlw_stop (BuzzMachine * bm)
{
  win32_prolog ();
  BMLX (bmlw_stop (bm));
  win32_eliplog ();
}


void
bmlw_attributes_changed (BuzzMachine * bm)
{
  win32_prolog ();
  BMLX (bmlw_attributes_changed (bm));
  win32_eliplog ();
}


void
bmlw_set_num_tracks (BuzzMachine * bm, int num)
{
  win32_prolog ();
  BMLX (bmlw_set_num_tracks (bm, num));
  win32_eliplog ();
}


void
bmlw_set_callbacks (BuzzMachine * bm, CHostCallbacks * callbacks)
{
  win32_prolog ();
  // @todo: remove after rebuild
  if (BMLX (bmlw_set_callbacks) != NULL)
    BMLX (bmlw_set_callbacks (bm, callbacks));
  win32_eliplog ();
}

int
_bmlw_setup (BMLDebugLogger logger)
{
  ldt_fs = Setup_LDT_Keeper ();
  TRACE ("   wrapper initialized: 0x%p\n", ldt_fs);
  //Check_FS_Segment(ldt_fs);

  if (!(emu_dll = LoadDLL ("BuzzMachineLoader.dll"))) {
    TRACE ("   failed to load window bml\n");
    return FALSE;
  }
  TRACE ("   windows bml loaded\n");

  if (!(BMLX (bmlw_set_logger) =
          (BMSetLogger) GetSymbol (emu_dll, "bm_set_logger"))) {
    TRACE ("bm_set_logger is missing\n");
    return (FALSE);
  }

  if (!(BMLX (bmlw_set_master_info) =
          (BMSetMasterInfo) GetSymbol (emu_dll, "bm_set_master_info"))) {
    TRACE ("bm_set_master_info is missing\n");
    return (FALSE);
  }


  if (!(BMLX (bmlw_open) = (BMOpen) GetSymbol (emu_dll, "bm_open"))) {
    TRACE ("bm_open is missing\n");
    return (FALSE);
  }
  if (!(BMLX (bmlw_close) = (BMClose) GetSymbol (emu_dll, "bm_close"))) {
    TRACE ("bm_close is missing\n");
    return (FALSE);
  }

  if (!(BMLX (bmlw_get_machine_info) =
          (BMGetMachineInfo) GetSymbol (emu_dll, "bm_get_machine_info"))) {
    TRACE ("bm_get_machine_info is missing\n");
    return (FALSE);
  }
  if (!(BMLX (bmlw_get_global_parameter_info) =
          (BMGetGlobalParameterInfo) GetSymbol (emu_dll,
              "bm_get_global_parameter_info"))) {
    TRACE ("bm_get_global_parameter_info is missing\n");
    return (FALSE);
  }
  if (!(BMLX (bmlw_get_track_parameter_info) =
          (BMGetTrackParameterInfo) GetSymbol (emu_dll,
              "bm_get_track_parameter_info"))) {
    TRACE ("bm_get_track_parameter_info is missing\n");
    return (FALSE);
  }
  if (!(BMLX (bmlw_get_attribute_info) =
          (BMGetAttributeInfo) GetSymbol (emu_dll, "bm_get_attribute_info"))) {
    TRACE ("bm_get_attribute_info is missing\n");
    return (FALSE);
  }

  if (!(BMLX (bmlw_describe_global_value) =
          (BMDescribeGlobalValue) GetSymbol (emu_dll,
              "bm_describe_global_value"))) {
    TRACE ("bm_describe_global_value is missing\n");
    return (FALSE);
  }
  if (!(BMLX (bmlw_describe_track_value) =
          (BMDescribeTrackValue) GetSymbol (emu_dll,
              "bm_describe_track_value"))) {
    TRACE ("bm_describe_track_value is missing\n");
    return (FALSE);
  }


  if (!(BMLX (bmlw_new) = (BMNew) GetSymbol (emu_dll, "bm_new"))) {
    TRACE ("bm_new is missing\n");
    return (FALSE);
  }
  if (!(BMLX (bmlw_free) = (BMFree) GetSymbol (emu_dll, "bm_free"))) {
    TRACE ("bm_free is missing\n");
    return (FALSE);
  }

  if (!(BMLX (bmlw_init) = (BMInit) GetSymbol (emu_dll, "bm_init"))) {
    TRACE ("bm_init is missing\n");
    return (FALSE);
  }

  if (!(BMLX (bmlw_get_track_parameter_value) =
          (BMGetTrackParameterValue) GetSymbol (emu_dll,
              "bm_get_track_parameter_value"))) {
    TRACE ("bm_get_track_parameter_value is missing\n");
    return (FALSE);
  }
  if (!(BMLX (bmlw_set_track_parameter_value) =
          (BMSetTrackParameterValue) GetSymbol (emu_dll,
              "bm_set_track_parameter_value"))) {
    TRACE ("bm_set_track_parameter_value is missing\n");
    return (FALSE);
  }

  if (!(BMLX (bmlw_get_global_parameter_value) =
          (BMGetGlobalParameterValue) GetSymbol (emu_dll,
              "bm_get_global_parameter_value"))) {
    TRACE ("bm_get_global_parameter_value is missing\n");
    return (FALSE);
  }
  if (!(BMLX (bmlw_set_global_parameter_value) =
          (BMSetGlobalParameterValue) GetSymbol (emu_dll,
              "bm_set_global_parameter_value"))) {
    TRACE ("bm_set_global_parameter_value is missing\n");
    return (FALSE);
  }

  if (!(BMLX (bmlw_get_attribute_value) =
          (BMGetAttributeValue) GetSymbol (emu_dll,
              "bm_get_attribute_value"))) {
    TRACE ("bm_get_attribute_value is missing\n");
    return (FALSE);
  }
  if (!(BMLX (bmlw_set_attribute_value) =
          (BMSetAttributeValue) GetSymbol (emu_dll,
              "bm_set_attribute_value"))) {
    TRACE ("bm_set_attribute_value is missing\n");
    return (FALSE);
  }

  if (!(BMLX (bmlw_tick) = (BMTick) GetSymbol (emu_dll, "bm_tick"))) {
    TRACE ("bm_tick is missing\n");
    return (FALSE);
  }
  if (!(BMLX (bmlw_work) = (BMWork) GetSymbol (emu_dll, "bm_work"))) {
    TRACE ("bm_work is missing\n");
    return (FALSE);
  }
  if (!(BMLX (bmlw_work_m2s) = (BMWorkM2S) GetSymbol (emu_dll, "bm_work_m2s"))) {
    TRACE ("bm_work_m2s is missing\n");
    return (FALSE);
  }
  if (!(BMLX (bmlw_stop) = (BMStop) GetSymbol (emu_dll, "bm_stop"))) {
    TRACE ("bm_stop is missing\n");
    return (FALSE);
  }

  if (!(BMLX (bmlw_attributes_changed) =
          (BMAttributesChanged) GetSymbol (emu_dll, "bm_attributes_changed"))) {
    TRACE ("bm_attributes_changed is missing\n");
    return (FALSE);
  }

  if (!(BMLX (bmlw_set_num_tracks) =
          (BMSetNumTracks) GetSymbol (emu_dll, "bm_set_num_tracks"))) {
    TRACE ("bm_set_num_tracks is missing\n");
    return (FALSE);
  }

  if (!(BMLX (bmlw_set_callbacks) =
          (BMSetCallbacks) GetSymbol (emu_dll, "bm_set_callbacks"))) {
    TRACE ("bm_set_callbacks is missing\n");
    return (FALSE);
  }

  TRACE ("   symbols connected\n");
  BMLX (bmlw_set_logger (logger));

  return TRUE;
}

void
_bmlw_finalize (void)
{
  FreeDLL (emu_dll);
  Restore_LDT_Keeper (ldt_fs);
}
