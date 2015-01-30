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

#ifndef BUZZ_MACHINE_LOADER_H
#define BUZZ_MACHINE_LOADER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _CHostCallbacks CHostCallbacks;

struct _CHostCallbacks {
    void *user_data;
    /* callbacks */
    void const *(*GetWave)(CHostCallbacks *self, int const i);
    void const *(*GetWaveLevel)(CHostCallbacks *self, int const i, int const level);
    void const *(*GetNearestWaveLevel)(CHostCallbacks *self, int const i, int const note);
    /* add more */
};

#ifdef BUZZ_MACHINE_LOADER

typedef CMachineInfo* (*GetInfoPtr)();
typedef CMachineInterface *(*CreateMachinePtr)();

class BuzzMachine;

// machine library handle
class BuzzMachineHandle {
public:
	// library handle
	void *h;
	char *lib_name;
    // machine info
    CMachineInfo *machine_info;
    int mdk_num_channels;
    // functions
    CreateMachinePtr CreateMachine;
    // dummy machine instance, so that we can call DescribeValue
    BuzzMachine *bm;
};

// machine instance handle
class BuzzMachine {
public:
    /* FIXME: what about adding a 4 byte cookie here
     * which bm_new() could set and
     * a macro BM_IS_BM(bm) could check
     */
	// library handle
    BuzzMachineHandle *bmh;
	// callback instance
	CMICallbacks *callbacks;
	// classes
	CMachineInfo *machine_info;
	CMachineInterface *machine_iface;
	CMachine *machine;
	CMDKImplementation *mdkHelper;
	CHostCallbacks *host_callbacks;
	//callbacks->machine_ex;
	//CMachineInterfaceEx *machine_ex;
};
#else
typedef void BuzzMachineHandle;
typedef void BuzzMachine;
#endif

typedef enum {
	BM_PROP_TYPE=0,
	BM_PROP_VERSION,
	BM_PROP_FLAGS,
	BM_PROP_MIN_TRACKS,
	BM_PROP_MAX_TRACKS,
	BM_PROP_NUM_GLOBAL_PARAMS,
	BM_PROP_NUM_TRACK_PARAMS,
	BM_PROP_NUM_ATTRIBUTES,
	BM_PROP_NAME,
	BM_PROP_SHORT_NAME,
	BM_PROP_AUTHOR,
	BM_PROP_COMMANDS,
	BM_PROP_DLL_NAME,
	BM_PROP_NUM_INPUT_CHANNELS,
	BM_PROP_NUM_OUTPUT_CHANNELS
} BuzzMachineProperty;

typedef enum {
	BM_PARA_TYPE=0,
	BM_PARA_NAME,
	BM_PARA_DESCRIPTION,
	BM_PARA_MIN_VALUE,
	BM_PARA_MAX_VALUE,
	BM_PARA_NO_VALUE,
	BM_PARA_FLAGS,
	BM_PARA_DEF_VALUE
} BuzzMachineParameter;

typedef enum {
	BM_ATTR_NAME=0,
	BM_ATTR_MIN_VALUE,
	BM_ATTR_MAX_VALUE,
	BM_ATTR_DEF_VALUE
} BuzzMachineAttribute;

#ifdef __cplusplus
}
#endif

#endif // BUZZ_MACHINE_LOADER_H
