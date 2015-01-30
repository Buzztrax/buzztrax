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

#define BUZZ_MACHINE_CALLBACKS_CPP
#define BUZZ_MACHINE_LOADER

#ifdef WIN32
#include "stdafx.h"
#include <windows.h>
#else
#include "windef.h"
#endif

#include <stdio.h>
#include "debug.h"

#include "BuzzMachineCallbacks.h"
//#include "BuzzMDKHelper.h"
#include "mdkimp.h"
#include "BuzzMachineLoader.h"
#include "CMachine.h"
#include "OscTable.h"

CWaveInfo const *BuzzMachineCallbacks::GetWave(int const i) {
	static CWaveInfo wi={0,};

    DBG1("(i=%d)\n",i);

    if(host_callbacks && *host_callbacks) {
        return (CWaveInfo *)(*host_callbacks)->GetWave(*host_callbacks,i);
    }
    return(&wi);
}

CWaveLevel const *BuzzMachineCallbacks::GetWaveLevel(int const i, int const level) {
    DBG2("(i=%d,level=%d)\n",i,level);

    if(host_callbacks && *host_callbacks) {
        return (CWaveLevel *)(*host_callbacks)->GetWaveLevel(*host_callbacks,i,level);
    }
    return(&defaultWaveLevel);
}

CWaveLevel const *BuzzMachineCallbacks::GetNearestWaveLevel(int const i, int const note) {
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

int BuzzMachineCallbacks::GetFreeWave() {
    DBG("()\n");
    FIXME;
    return(0);
}

bool BuzzMachineCallbacks::AllocateWave(int const i, int const size, char const *name) {
    DBG3("(i=%d,size=%d,name=\"%s\")\n",i,size,name);
    FIXME;
    return(FALSE);
}

//-- since buzz 1.2
char const *BuzzMachineCallbacks::GetWaveName(int const i) {
    DBG1("(i=%d)\n",i);
    FIXME;
    return(NULL);
}

// i >= 1, NULL name to clear
void BuzzMachineCallbacks::SetInternalWaveName(CMachine *pmac, int const i, char const *name) {
    DBG3("(pmac=%p,i=%d,name=\"%s\")\n",pmac,i,name);
}


/* GUI:
 *
 */
void BuzzMachineCallbacks::MessageBox(char const *txt) {
    DBG1("(txt=\"%s\")\n",txt);
    if(txt) puts(txt);
}

/* Misc/Unknown:
 */

void BuzzMachineCallbacks::Lock() {
    DBG("()\n");
    FIXME;
    // used by : Rebirth MIDI.dll
}

void BuzzMachineCallbacks::Unlock() {
    DBG("()\n");
    FIXME;
    // used by : Rebirth MIDI.dll
}

void BuzzMachineCallbacks::ScheduleEvent(int const time, dword const data) {
    DBG2("(time=%d,data=%d)\n",time,data);
    FIXME;
}

void BuzzMachineCallbacks::MidiOut(int const dev, dword const data) {
    DBG2("(dev=%d,data=%d)\n",dev,data);
    FIXME;
}

/* common Oszillators
 */
short const *BuzzMachineCallbacks::GetOscillatorTable(int const waveform) {
    DBG1("(waveform=%d)\n",waveform);
    return OscTable[waveform];
}


/* Application State:
 */
int BuzzMachineCallbacks::GetWritePos() {
    DBG("()\n");
    FIXME;
    return(0);
}

int BuzzMachineCallbacks::GetPlayPos() {
    DBG("()\n");
    FIXME;
    return(0);
}

//-- since buzz 1.2
// only call this in Init()!
CMachine *BuzzMachineCallbacks::GetThisMachine() {
    DBG("()\n");
    return(machine);
}

// returns pointer to the sequence if there is a pattern playing
CSequence *BuzzMachineCallbacks::GetPlayingSequence(CMachine *pmac) {
    DBG1("(pmac=%p)\n",pmac);
    FIXME;
    return(NULL);
}

// gets ptr to raw pattern data for row of a track of a currently playing pattern (or something like that)
void *BuzzMachineCallbacks::GetPlayingRow(CSequence *pseq, int group, int track) {
    DBG3("(pseq=%p,group=%d,track=%d)\n",pseq,group,track);
    FIXME;
    return(NULL);
}

int BuzzMachineCallbacks::GetStateFlags() {
    DBG("()\n");
    FIXME;
    return(0);
}

/* AuxBus:
 */
float *BuzzMachineCallbacks::GetAuxBuffer() {
    DBGO1(machine_info->Name,"()=%p\n",auxBuffer);
    return(auxBuffer);
}

void BuzzMachineCallbacks::ClearAuxBuffer() {
    DBGO(machine_info->Name,"()\n");
    for (unsigned int i=0; i<2*BMC_AUXBUFFER_SIZE; i++) {
        auxBuffer[i]=0.0f;
    }
}

/* Envelopes:
 */
int BuzzMachineCallbacks::GetEnvSize(int const wave, int const env) {
    DBG2("(wave=%d,env=%d)\n",wave,env);
    FIXME;
    return(0);
}
bool BuzzMachineCallbacks::GetEnvPoint(int const wave, int const env, int const i, word &x, word &y, int &flags) {
    DBG3("(wave=%d,env=%d,i=%d,&x,&y,&flags)\n",wave,env,i);
    FIXME;
    return(FALSE);
}


/* Pattern editing:
 */
void BuzzMachineCallbacks::SetNumberOfTracks(int const n) {
    DBG1("(n=%d)\n",n);
    FIXME;
}

CPattern *BuzzMachineCallbacks::CreatePattern(char const *name, int const length) {
    DBG2("(name=\"%s\",length=%d)\n",name,length);
    FIXME;
    return(NULL);
}

CPattern *BuzzMachineCallbacks::GetPattern(int const index) {
    DBG1("(index=%d)\n",index);
    FIXME;
    return(NULL);
}

char const *BuzzMachineCallbacks::GetPatternName(CPattern *ppat) {
    DBG1("(ppat=%p)\n",ppat);
    FIXME;
    return(NULL);
}

void BuzzMachineCallbacks::RenamePattern(char const *oldname, char const *newname) {
    DBG2("(oldname=\"%s\",newname=\"%s\")\n",oldname,newname);
    FIXME;
}

void BuzzMachineCallbacks::DeletePattern(CPattern *ppat) {
    DBG1("(ppat=%p)\n",ppat);
    FIXME;
}

int BuzzMachineCallbacks::GetPatternData(CPattern *ppat, int const row, int const group, int const track, int const field) {
    DBG5("(ppat=%p,row=%d,group=%d,track=%d,field=%d)\n",ppat,row,group,track,field);
    FIXME;
    return(0);
}

void BuzzMachineCallbacks::SetPatternData(CPattern *ppat, int const row, int const group, int const track, int const field, int const value) {
    DBG6("(ppat=%p,row=%d,group=%d,track=%d,field=%d,value=%d)\n",ppat,row,group,track,field,value);
    FIXME;
}


/* Sequence editing:
 */
CSequence *BuzzMachineCallbacks::CreateSequence() {
    DBG("()\n");
    FIXME;
    return(NULL);
}
void BuzzMachineCallbacks::DeleteSequence(CSequence *pseq) {
    DBG1("(pseq=%p)\n",pseq);
    FIXME;
}

// special ppat values for GetSequenceData and SetSequenceData
// empty = NULL
// <break> = (CPattern *)1
// <mute> = (CPattern *)2
// <thru> = (CPattern *)3
CPattern *BuzzMachineCallbacks::GetSequenceData(int const row) {
    DBG1("(row=%d)\n",row);
    FIXME;
    return(NULL);
}

void BuzzMachineCallbacks::SetSequenceData(int const row, CPattern *ppat) {
    DBG2("(row=%d,ppat=%p)\n",row,ppat);
}


//-- buzz v1.2 (MI_VERSION 15) additions start here ---

void BuzzMachineCallbacks::SetMachineInterfaceEx(CMachineInterfaceEx *pex) {
    DBG1("(pex=%p)\n",pex);
    machine_ex=pex;
}


// set value of parameter
// group 1=global, 2=track
void BuzzMachineCallbacks::ControlChange__obsolete__(int group, int track, int param, int value) {
    DBG4("(group=%d,track=%d,param=%d,value=%d)\n",group,track,param,value);
    FIXME;
}
// set value of parameter (group & 16 == don't record)
void BuzzMachineCallbacks::ControlChange(CMachine *pmac, int group, int track, int param, int value) {
    DBG5("(pmac=%p,group=%d,track=%d,param=%d,value=%d)\n",pmac,group,track,param,value);
    FIXME;
}

// direct calls to audiodriver, used by WaveInput and WaveOutput
// shouldn't be used for anything else
int BuzzMachineCallbacks::ADGetnumChannels(bool input) {
    DBG1("(input=%d)\n",input);
    FIXME;
    return(0);
}

void BuzzMachineCallbacks::ADWrite(int channel, float *psamples, int numsamples) {
    DBG3("(channel=%d,psamples=%p,numsamples=%d)\n",channel,psamples,numsamples);
    FIXME;
}

void BuzzMachineCallbacks::ADRead(int channel, float *psamples, int numsamples) {
    DBG3("(channel=%d,psamples=%p,numsamples=%d)\n",channel,psamples,numsamples);
    FIXME;
}

// if n=1 Work(), n=2 WorkMonoToStereo()
void BuzzMachineCallbacks::SetnumOutputChannels(CMachine *pmac, int n) {
    DBG2("(pmac=%p,n=%d)\n",pmac,n);
    FIXME;
    // used by : ld vocoder xp.dll
    // would this need to use mdk?
}

void BuzzMachineCallbacks::SetEventHandler(CMachine *pmac, BEventType et, EVENT_HANDLER_PTR p, void *param) {
    DBG4("(pmac=%p,et=%d,p=%p,param=%p)\n",pmac,et,p,param);
    FIXME;
}

/* Machines:
 */
// *pout will get one name per Write()
void BuzzMachineCallbacks::GetMachineNames(CMachineDataOutput *pout) {
    DBG1("(pout=%p)\n",pout);
    FIXME;
}

CMachine *BuzzMachineCallbacks::GetMachine(char const *name) {
    DBG1("(name=\"%s\")\n",name);
    FIXME;
    return(NULL);
}

CMachineInfo const *BuzzMachineCallbacks::GetMachineInfo(CMachine *pmac) {
    DBG1("(pmac=%p)\n",pmac);
    //FIXME;
    return(pmac->machine_info);
}

char const *BuzzMachineCallbacks::GetMachineName(CMachine *pmac) {
    DBG1("(pmac=%p)\n",pmac);
    FIXME;
    return(NULL);
}

bool BuzzMachineCallbacks::GetInput(int index, float *psamples, int numsamples, bool stereo, float *extrabuffer) {
    DBG5("(index=%d,psamples=%p,numsamples=%d,stereo=%d,extrabuffer=%p)\n",index,psamples,numsamples,stereo,extrabuffer);
    FIXME;
    return(FALSE);
}
