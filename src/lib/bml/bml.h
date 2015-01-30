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

#ifndef BML_H
#define BML_H

#include "bml/BuzzMachineLoader/BuzzMachineLoader.h"

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE (!FALSE)
#endif

//#ifdef __cplusplus
//extern "C" {
//#endif

extern int bml_setup(void);
extern void bml_finalize(void);

typedef void (*BMLDebugLogger)(char *str);

// dll passthrough API method pointer types
typedef void (*BMSetLogger)(BMLDebugLogger func);
typedef void (*BMSetMasterInfo)(long bpm, long tpb, long srat);

typedef BuzzMachineHandle * (*BMOpen)(char *bm_file_name);
typedef void (*BMClose)(BuzzMachineHandle *bmh);

typedef int (*BMGetMachineInfo)(BuzzMachineHandle *bmh, BuzzMachineProperty key, void *value);
typedef int (*BMGetGlobalParameterInfo)(BuzzMachineHandle *bmh,int index,BuzzMachineParameter key,void *value);
typedef int (*BMGetTrackParameterInfo)(BuzzMachineHandle *bmh,int index,BuzzMachineParameter key,void *value);
typedef int (*BMGetAttributeInfo)(BuzzMachineHandle *bmh,int index,BuzzMachineAttribute key,void *value);

typedef const char *(*BMDescribeGlobalValue)(BuzzMachineHandle *bmh, int const param,int const value);
typedef const char *(*BMDescribeTrackValue)(BuzzMachineHandle *bmh, int const param,int const value);


typedef BuzzMachine * (*BMNew)(BuzzMachineHandle *bmh);
typedef void (*BMFree)(BuzzMachine *bm);

typedef void (*BMInit)(BuzzMachine *bm, unsigned long blob_size, unsigned char *blob_data);

typedef void * (*BMGetTrackParameterLocation)(BuzzMachine *bm,int track,int index);
typedef int  (*BMGetTrackParameterValue)(BuzzMachine *bm,int track,int index);
typedef void (*BMSetTrackParameterValue)(BuzzMachine *bm,int track,int index,int value);

typedef void * (*BMGetGlobalParameterLocation)(BuzzMachine *bm,int index);
typedef int  (*BMGetGlobalParameterValue)(BuzzMachine *bm,int index);
typedef void (*BMSetGlobalParameterValue)(BuzzMachine *bm,int index,int value);

typedef void * (*BMGetAttributeLocation)(BuzzMachine *bm,int index);
typedef int  (*BMGetAttributeValue)(BuzzMachine *bm,int index);
typedef void (*BMSetAttributeValue)(BuzzMachine *bm,int index,int value);

typedef void (*BMTick)(BuzzMachine *bm);
typedef int  (*BMWork)(BuzzMachine *bm,float *psamples, int numsamples, int const mode);
typedef int  (*BMWorkM2S)(BuzzMachine *bm,float *pin, float *pout, int numsamples, int const mode);
typedef void (*BMStop)(BuzzMachine *bm);

typedef void (*BMAttributesChanged)(BuzzMachine *bm);

typedef void (*BMSetNumTracks)(BuzzMachine *bm, int num);

typedef void *(*BMSetCallbacks)(BuzzMachine *bm, CHostCallbacks *callbacks);

// windows plugin API functions
extern void bmlw_set_master_info(long bpm, long tpb, long srat);

extern BuzzMachineHandle *bmlw_open(char *bm_file_name);
extern void bmlw_close(BuzzMachineHandle *bmh);


extern int bmlw_get_machine_info(BuzzMachineHandle *bmh, BuzzMachineProperty key, void *value);
extern int bmlw_get_global_parameter_info(BuzzMachineHandle *bmh,int index,BuzzMachineParameter key,void *value);
extern int bmlw_get_track_parameter_info(BuzzMachineHandle *bmh,int index,BuzzMachineParameter key,void *value);
extern int bmlw_get_attribute_info(BuzzMachineHandle *bmh,int index,BuzzMachineAttribute key,void *value);

extern const char *bmlw_describe_global_value(BuzzMachineHandle *bmh, int const param,int const value);
extern const char *bmlw_describe_track_value(BuzzMachineHandle *bmh, int const param,int const value);


extern BuzzMachine *bmlw_new(BuzzMachineHandle *bmh);
extern void bmlw_free(BuzzMachine *bm);

extern void bmlw_init(BuzzMachine *bm, unsigned long blob_size, unsigned char *blob_data);

extern int bmlw_get_track_parameter_value(BuzzMachine *bm,int track,int index);
extern void bmlw_set_track_parameter_value(BuzzMachine *bm,int track,int index,int value);

extern int bmlw_get_global_parameter_value(BuzzMachine *bm,int index);
extern void bmlw_set_global_parameter_value(BuzzMachine *bm,int index,int value);

extern int bmlw_get_attribute_value(BuzzMachine *bm,int index);
extern void bmlw_set_attribute_value(BuzzMachine *bm,int index,int value);

extern void bmlw_tick(BuzzMachine *bm);
extern int bmlw_work(BuzzMachine *bm,float *psamples, int numsamples, int const mode);
extern int bmlw_work_m2s(BuzzMachine *bm,float *pin, float *pout, int numsamples, int const mode);
extern void bmlw_stop(BuzzMachine *bm);

extern void bmlw_attributes_changed(BuzzMachine *bm);

extern void bmlw_set_num_tracks(BuzzMachine *bm, int num);

extern void bmlw_set_callbacks(BuzzMachine *bm, CHostCallbacks *callbacks);

// native plugin API functions
extern BMSetMasterInfo bmln_set_master_info;


extern BMOpen bmln_open;
extern BMClose bmln_close;

extern BMGetMachineInfo bmln_get_machine_info;
extern BMGetGlobalParameterInfo bmln_get_global_parameter_info;
extern BMGetTrackParameterInfo bmln_get_track_parameter_info;
extern BMGetAttributeInfo bmln_get_attribute_info;

extern BMDescribeGlobalValue bmln_describe_global_value;
extern BMDescribeTrackValue bmln_describe_track_value;


extern BMNew bmln_new;
extern BMFree bmln_free;

extern BMInit bmln_init;

extern BMGetTrackParameterValue bmln_get_track_parameter_value;
extern BMSetTrackParameterValue bmln_set_track_parameter_value;

extern BMGetGlobalParameterValue bmln_get_global_parameter_value;
extern BMSetGlobalParameterValue bmln_set_global_parameter_value;

extern BMGetAttributeValue bmln_get_attribute_value;
extern BMSetAttributeValue bmln_set_attribute_value;

extern BMTick bmln_tick;
extern BMWork bmln_work;
extern BMWorkM2S bmln_work_m2s;
extern BMStop bmln_stop;

extern BMAttributesChanged bmln_attributes_changed;

extern BMSetNumTracks bmln_set_num_tracks;

extern BMSetCallbacks bmln_set_callbacks;

//#ifdef __cplusplus
//}
//#endif

#endif // BML_H
