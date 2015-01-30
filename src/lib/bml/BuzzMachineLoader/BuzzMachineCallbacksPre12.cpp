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

#define BUZZ_MACHINE_CALLBACKS_PRE12_CPP
#define BUZZ_MACHINE_LOADER

#ifdef WIN32
#include "stdafx.h"
#include <windows.h>
#else
#include "windef.h"
#endif
#include <stdio.h>
#include "debug.h"

#include "BuzzMachineCallbacksPre12.h"
//#include "BuzzMDKHelper.h"
#include "mdkimp.h"
#include "BuzzMachineLoader.h"
#include "OscTable.h"

CWaveInfo const *BuzzMachineCallbacksPre12::GetWave(int const i) {
	static CWaveInfo wi={0,};

    DBG1("(i=%d)\n",i);

    if(host_callbacks && *host_callbacks) {
        return (CWaveInfo *)(*host_callbacks)->GetWave(*host_callbacks,i);
    }
    return(&wi);
}

CWaveLevel const *BuzzMachineCallbacksPre12::GetWaveLevel(int const i, int const level) {
    DBG2("(i=%d,level=%d)\n",i,level);

    if(host_callbacks && *host_callbacks) {
        return (CWaveLevel *)(*host_callbacks)->GetWaveLevel(*host_callbacks,i,level);
    }
    return(&defaultWaveLevel);
}

CWaveLevel const *BuzzMachineCallbacksPre12::GetNearestWaveLevel(int const i, int const note) {
    DBG2("(i=%d,note=%d)\n",i,note);

    if((i==-1) && (note==-1)) {
        // the evil MDK hack that Buzz MDK machines rely upon
        if(!mdkHelper) {
            DBG("create the mdk helper\n");
            //mdkHelper = new BuzzMDKHelper;
            mdkHelper = new CMDKImplementation;
        }
        DBG1("return the mdk helper, %p\n",mdkHelper);
        return((CWaveLevel *)mdkHelper);
    }
    if((i==-2) && (note==-2)) {
      // if(pCB->GetHostVersion() >= 2) { newbuzz = 1; }
      FIXME;
    }

    if(host_callbacks && *host_callbacks) {
        return (CWaveLevel *)(*host_callbacks)->GetNearestWaveLevel(*host_callbacks,i,note);
    }
    return(&defaultWaveLevel);
}

int BuzzMachineCallbacksPre12::GetFreeWave() {
    DBG("()\n");
    FIXME;
    return(0);
}

bool BuzzMachineCallbacksPre12::AllocateWave(int const i, int const size, char const *name) {
    DBG3("(i=%d,size=%d,name=\"%s\")\n",i,size,name);
    FIXME;
    return(FALSE);
}

/* GUI:
 *
 */
void BuzzMachineCallbacksPre12::MessageBox(char const *txt) {
    DBG1("(txt=\"%s\")\n",txt);
    if(txt) puts(txt);
}

/* Misc/Unknown:
 */

void BuzzMachineCallbacksPre12::Lock() {
    DBG("()\n");
    FIXME;
}

void BuzzMachineCallbacksPre12::Unlock() {
    DBG("()\n");
    FIXME;
}

void BuzzMachineCallbacksPre12::ScheduleEvent(int const time, dword const data) {
    DBG2("(time=%d,data=%d)\n",time,data);
    FIXME;
}

void BuzzMachineCallbacksPre12::MidiOut(int const dev, dword const data) {
    DBG2("(dev=%d,data=%d)\n",dev,data);
    FIXME;
}

/* common Oszillators
 */
short const *BuzzMachineCallbacksPre12::GetOscillatorTable(int const waveform) {
    DBG1("(waveform=%d)\n",waveform);
    return OscTable[waveform];
}


/* Application State:
 */
int BuzzMachineCallbacksPre12::GetWritePos() {
    DBG("()\n");
    FIXME;
    return(0);
}

int BuzzMachineCallbacksPre12::GetPlayPos() {
    DBG("()\n");
    FIXME;
    return(0);
}

/* AuxBus:
 *
 */
float *BuzzMachineCallbacksPre12::GetAuxBuffer() {
    DBGO1(machine_info->Name,"()=%p\n",auxBuffer);
    return(auxBuffer);
}

void BuzzMachineCallbacksPre12::ClearAuxBuffer() {
    DBGO(machine_info->Name,"()\n");
    for (unsigned int i=0; i<2*BMC_AUXBUFFER_SIZE; i++) {
        auxBuffer[i]=0.0f;
    }
}

/* Envelopes:
 */
int BuzzMachineCallbacksPre12::GetEnvSize(int const wave, int const env) {
    DBG2("(wave=%d,env=%d)\n",wave,env);
    /*
      incredibly odd - i dont understand this, but jeskola raverb requires this to run =)
      we do not keep the value though, it may haunt us later. both raverb and the host keep their own static copies of this value
      the value seems to be combined from getlocaltime, getsystemtime, gettimezoneinfo and more.
      from buzz.exe disassembly of GetEnvSize implementation:
        00425028 69 C0 93 B1 39 3E imul        eax,eax,3E39B193h
        0042502E 05 3B 30 00 00   add         eax,303Bh
        00425033 25 FF FF FF 7F   and         eax,7FFFFFFFh
        00425038 A3 F0 26 4D 00   mov         dword ptr ds:[004D26F0h],eax
    */
    if (wave<0) {
      return ((wave*0x3E39B193) + 0x303b ) & 0x7FFFFFFF;
    }
    FIXME;
    return(0);
}
bool BuzzMachineCallbacksPre12::GetEnvPoint(int const wave, int const env, int const i, word &x, word &y, int &flags) {
    DBG3("(wave=%d,env=%d,i=%d,&x,&y,&flags)\n",wave,env,i);
    FIXME;
    return(FALSE);
}


/* Pattern editing:
 */
void BuzzMachineCallbacksPre12::SetNumberOfTracks(int const n) {
    DBG1("(n=%d)\n",n);
    FIXME;
}

CPattern *BuzzMachineCallbacksPre12::CreatePattern(char const *name, int const length) {
    DBG2("(name=\"%s\",length=%d)\n",name,length);
    FIXME;
    return(NULL);
}

CPattern *BuzzMachineCallbacksPre12::GetPattern(int const index) {
    DBG1("(index=%d)\n",index);
    FIXME;
    return(NULL);
}

char const *BuzzMachineCallbacksPre12::GetPatternName(CPattern *ppat) {
    DBG1("(ppat=%p)\n",ppat);
    FIXME;
    return(NULL);
}

void BuzzMachineCallbacksPre12::RenamePattern(char const *oldname, char const *newname) {
    DBG2("(oldname=\"%s\",newname=\"%s\")\n",oldname,newname);
    FIXME;
}

void BuzzMachineCallbacksPre12::DeletePattern(CPattern *ppat) {
    DBG1("(ppat=%p)\n",ppat);
    FIXME;
}

int BuzzMachineCallbacksPre12::GetPatternData(CPattern *ppat, int const row, int const group, int const track, int const field) {
    DBG5("(ppat=%p,row=%d,group=%d,track=%d,field=%d)\n",ppat,row,group,track,field);
    FIXME;
    return(0);
}

void BuzzMachineCallbacksPre12::SetPatternData(CPattern *ppat, int const row, int const group, int const track, int const field, int const value) {
    DBG6("(ppat=%p,row=%d,group=%d,track=%d,field=%d,value=%d)\n",ppat,row,group,track,field,value);
    FIXME;
}


/* Sequence editing:
 */
CSequence *BuzzMachineCallbacksPre12::CreateSequence() {
    DBG("()\n");
    FIXME;
    return(NULL);
}
void BuzzMachineCallbacksPre12::DeleteSequence(CSequence *pseq) {
    DBG1("(pseq=%p)\n",pseq);
    FIXME;
}

// special ppat values for GetSequenceData and SetSequenceData
// empty = NULL
// <break> = (CPattern *)1
// <mute> = (CPattern *)2
// <thru> = (CPattern *)3
CPattern *BuzzMachineCallbacksPre12::GetSequenceData(int const row) {
    DBG1("(row=%d)\n",row);
    FIXME;
    return(NULL);
}

void BuzzMachineCallbacksPre12::SetSequenceData(int const row, CPattern *ppat) {
    DBG2("(row=%d,ppat=%p)\n",row,ppat);
}
