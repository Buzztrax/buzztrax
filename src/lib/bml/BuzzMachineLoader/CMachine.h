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

#include "MachineInterface.h"

class CMachine {
    // Jeskola Buzz compatible CMachine header.
    // Some machines look up these by reading directly from zzub::metaplugin memory.
    
    char _placeholder[12];
    char* _internal_name;           // 0x14: polac's VST reads this string, set to 0
    char _placeholder2[52];
    void* _internal_machine;        // pointer to CMachine*, scanned for by some plugins
    void* _internal_machine_ex;     // 0x50: same as above, but is not scanned for
    char _placeholder3[20];
    char* _internal_globalState;    // 0x68: copy of machines global state
    char* _internal_trackState;     // 0x6C: copy of machines track state
    char _placeholder4[120];
    int _internal_seqCommand;       // 0xE8: used by mooter, 0 = --, 1 = mute, 2 = thru
    char _placeholder5[17];
    bool hardMuted;                 // 0xFD: true when muted by user, used by mooter
    // End of Buzz compatible header
public:
    CMachineInterface *machine_interface;
    CMachineInfo *machine_info;

public:
    CMachine() {
        machine_interface=NULL;
        machine_info=NULL;
        _placeholder[0] = 0;
        _internal_name = NULL;
        _placeholder2[0] = 0;
        _internal_machine = this;
        _internal_machine_ex = NULL;
        _placeholder3[0] = 0;
        _internal_globalState = NULL;
        _internal_trackState = NULL;
        _placeholder4[0] = 0;
        _internal_seqCommand = 0;
        _placeholder5[0] = 0;
        hardMuted = 0;
    }
    
    CMachine(CMachineInterface *_machine_interface,CMachineInfo *_machine_info) {
        machine_interface=_machine_interface;
        machine_info=_machine_info;
        _internal_machine = this;
    }
};

