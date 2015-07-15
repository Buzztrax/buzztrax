/* Buzz Machine Loader
 * Copyright (C) 2003-2007 Anders Ervik <calvin@countzero.no>
 * Copyright (C) 2008 Buzztrax team <buzztrax-devel@buzztrax.org>*
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef MACHINE_H
#define MACHINE_H

struct CMachineInfo;
class CMachineInterface;

class CMachine {
    // Jeskola Buzz compatible CMachine header.
    // Some machines look up these by reading directly from the memory.

    dword header;                   // 0x00: either 0x004b3758 (generator) or 0x004b3708 (Effect)
    char _placeholder[16];          // 0x04: all 0
    const char* shortName;          // 0x14: CMachineInfo.SHortName, polac's VST reads this string
    char _placeholder2[52];         // 0x18:
    void* machineInterface;         // 0x4C: pointer to CMachineInterface*, scanned for by some plugins
    void* _internal_machine_ex;     // 0x50: NULL
    char _placeholder3[20];         // 0x54: ptr ptr? ptr NULL NULL
    void* _internal_globalState;    // 0x68: copy of machines global state
    void* _internal_trackState;     // 0x6C: copy of machines track state
    char _placeholder4[16];         // 0x70: ptr ptr NULL 0x28d13d00
    void* machineInterface2;        // 0x80: another copy of CMachineInterface*
    dword globalSize;               // 0x84: global param size
    dword trackSize;                // 0x88: track param size
    dword unk1;                     // 0x8C: ?
    dword unk2;                     // 0x90: ?
    dword globalParams;             // 0x94: number of global params
    dword trackParams;              // 0x98: number of track params
    char _placeholder5[76];         // 0x9C:
    int _internal_seqCommand;       // 0xE8: used by mooter, 0 = --, 1 = mute, 2 = thru
    char _placeholder6[17];
    bool hardMuted;                 // 0xFD: true when muted by user, used by mooter
    // End of Buzz compatible header
public:
    CMachineInterface *machine_interface;
    CMachineInfo *machine_info;

public:
    CMachine() {
        header = 0;
        machine_interface=NULL;
        machine_info=NULL;
        memset (_placeholder, 0,  sizeof(_placeholder));
        shortName = NULL;
    		memset (_placeholder2, 0,  sizeof(_placeholder2));
        machineInterface = NULL;
        _internal_machine_ex = NULL;
    		memset (_placeholder3, 0,  sizeof(_placeholder3));
        _internal_globalState = NULL;
        _internal_trackState = NULL;
    		memset (_placeholder4, 0,  sizeof(_placeholder4));
    		machineInterface2 = NULL;
    		memset (_placeholder5, 0,  sizeof(_placeholder5));
        _internal_seqCommand = 0;
    		memset (_placeholder6, 0,  sizeof(_placeholder6));
        hardMuted = 0;
    }

    CMachine(CMachineInterface *_machine_interface,CMachineInfo *_machine_info) {
        header = (_machine_info->Type == /* GENERATOR*/ 1) ? 0x004b3758 : 0x004b3708;
        machine_interface=_machine_interface;
        machine_info=_machine_info;
        memset (_placeholder, 0,  sizeof(_placeholder));
        shortName = _machine_info->ShortName;
        memset (_placeholder2, 0,  sizeof(_placeholder2));
        machineInterface = _machine_interface;
        _internal_machine_ex = NULL;
        memset (_placeholder3, 0,  sizeof(_placeholder3));
        _internal_globalState = _machine_interface->GlobalVals;
        _internal_trackState = _machine_interface->TrackVals;
        memset (_placeholder4, 0,  sizeof(_placeholder4));
        machineInterface2 = _machine_interface;
        globalParams = _machine_info->numGlobalParameters;
        trackParams = _machine_info->numTrackParameters;
        memset (_placeholder5, 0,  sizeof(_placeholder5));
        _internal_seqCommand = 0;
        memset (_placeholder6, 0,  sizeof(_placeholder6));
        hardMuted = 0;
    }
};

#endif // MACHINE_H
