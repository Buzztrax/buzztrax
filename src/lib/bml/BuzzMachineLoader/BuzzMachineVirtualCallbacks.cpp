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

#ifdef WIN32
#include "stdafx.h"
#include <windows.h>
#else
#include "windef.h"
#endif
#include "stdio.h"
#include "MachineInterface.h"

/* WaveTable:
 *  we could pass the data from the linux side to the windows side,
 *  so that it can emulate the buzz API
 */

CWaveInfo const *CMICallbacks::GetWave(int const i) {
	printf("CMICallbacks::%s(i=%d)\n",__FUNCTION__,i);
	return(NULL);
}

CWaveLevel const *CMICallbacks::GetWaveLevel(int const i, int const level) {
	printf("CMICallbacks::%s(i=%d,level=%d)\n",__FUNCTION__,i,level);
	return(NULL);
}

CWaveLevel const *CMICallbacks::GetNearestWaveLevel(int const i, int const note) {
	printf("CMICallbacks::%s(i=%d,note=%d)\n",__FUNCTION__,i,note);
	return(NULL);
}

int CMICallbacks::GetFreeWave() {
	printf("CMICallbacks::%s()\n",__FUNCTION__);
	return(0);
}

bool CMICallbacks::AllocateWave(int const i, int const size, char const *name) {
	printf("CMICallbacks::%s(i=%d,size=%d,name=\"%s\")\n",__FUNCTION__,i,size,name);
	return(FALSE);
}

//-- since buzz 1.2
char const *CMICallbacks::GetWaveName(int const i) {
	printf("CMICallbacks::%s(i=%d)\n",__FUNCTION__,i);
	return(NULL);
}

// i >= 1, NULL name to clear
void CMICallbacks::SetInternalWaveName(CMachine *pmac, int const i, char const *name) {
	printf("CMICallbacks::%s(pmac=%p,i=%d,name=\"%s\")\n",__FUNCTION__,pmac,i,name);
}


/* GUI:
 *
 */
void CMICallbacks::MessageBox(char const *txt) {
	printf("CMICallbacks::%s(txt=\"%s\")\n",__FUNCTION__,txt);
	if(txt) puts(txt);
}

/* Misc/Unknown:
 */

void CMICallbacks::Lock() {
	printf("CMICallbacks::%s()\n",__FUNCTION__);
}

void CMICallbacks::Unlock() {
	printf("CMICallbacks::%s()\n",__FUNCTION__);
}

void CMICallbacks::ScheduleEvent(int const time, dword const data) {
	printf("CMICallbacks::%s(time=%d,data=%ld)\n",__FUNCTION__,time,data);
}

void CMICallbacks::MidiOut(int const dev, dword const data) {
	printf("CMICallbacks::%s(dev=%d,data=%ld)\n",__FUNCTION__,dev,data);
}

short const *CMICallbacks::GetOscillatorTable(int const waveform) {
	printf("CMICallbacks::%s(waveform=%d)\n",__FUNCTION__,waveform);
	return(NULL);
}


/* Application State:
 */
int CMICallbacks::GetWritePos() {
	printf("CMICallbacks::%s()\n",__FUNCTION__);
	return(0);
}

int CMICallbacks::GetPlayPos() {
	printf("CMICallbacks::%s()\n",__FUNCTION__);
	return(0);
}

//-- since buzz 1.2
// only call this in Init()!
CMachine *CMICallbacks::GetThisMachine() {
	printf("CMICallbacks::%s()\n",__FUNCTION__);
	return(NULL);
}

// returns pointer to the sequence if there is a pattern playing
CSequence *CMICallbacks::GetPlayingSequence(CMachine *pmac) {
	printf("CMICallbacks::%s(pmac=%p)\n",__FUNCTION__,pmac);
	return(NULL);
}

// gets ptr to raw pattern data for row of a track of a currently playing pattern (or something like that)
void *CMICallbacks::GetPlayingRow(CSequence *pseq, int group, int track) {
	printf("CMICallbacks::%s(pseq=%p,group=%d,track=%d)\n",__FUNCTION__,pseq,group,track);
	return(NULL);
}

int CMICallbacks::GetStateFlags() {
	printf("CMICallbacks::%s()\n",__FUNCTION__);
	return(0);
}

/* AuxBus:
 *
 */
float *CMICallbacks::GetAuxBuffer() {
	printf("CMICallbacks::%s()\n",__FUNCTION__);
	return(NULL);
}

void CMICallbacks::ClearAuxBuffer() {
	printf("CMICallbacks::%s()\n",__FUNCTION__);
}

/* Envelopes:
 */
int CMICallbacks::GetEnvSize(int const wave, int const env) {
	printf("CMICallbacks::%s(wave=%d,env=%d)\n",__FUNCTION__,wave,env);
	return(0);
}
bool CMICallbacks::GetEnvPoint(int const wave, int const env, int const i, word &x, word &y, int &flags) {
	printf("CMICallbacks::%s(wave=%d,env=%d,i=%d,&x,&y,&flags)\n",__FUNCTION__,wave,env,i);
	return(FALSE);
}


/* Pattern editing:
 */
void CMICallbacks::SetNumberOfTracks(int const n) {
	printf("CMICallbacks::%s(n=%d)\n",__FUNCTION__,n);
}

CPattern *CMICallbacks::CreatePattern(char const *name, int const length) {
	printf("CMICallbacks::%s(name=\"%s\",length=%d)\n",__FUNCTION__,name,length);
	return(NULL);
}

CPattern *CMICallbacks::GetPattern(int const index) {
	printf("CMICallbacks::%s(index=%d)\n",__FUNCTION__,index);
	return(NULL);
}

char const *CMICallbacks::GetPatternName(CPattern *ppat) {
	printf("CMICallbacks::%s(ppat=%p)\n",__FUNCTION__,ppat);
	return(NULL);
}

void CMICallbacks::RenamePattern(char const *oldname, char const *newname) {
	printf("CMICallbacks::%s(oldname=\"%s\",newname=\"%s\")\n",__FUNCTION__,oldname,newname);
}

void CMICallbacks::DeletePattern(CPattern *ppat) {
	printf("CMICallbacks::%s(ppat=%p)\n",__FUNCTION__,ppat);
}

int CMICallbacks::GetPatternData(CPattern *ppat, int const row, int const group, int const track, int const field) {
	printf("CMICallbacks::%s(ppat=%p,row=%d,group=%d,track=%d,field=%d)\n",__FUNCTION__,ppat,row,group,track,field);
	return(0);
}

void CMICallbacks::SetPatternData(CPattern *ppat, int const row, int const group, int const track, int const field, int const value) {
	printf("CMICallbacks::%s(ppat=%p,row=%d,group=%d,track=%d,field=%d,value=%d)\n",__FUNCTION__,ppat,row,group,track,field,value);
}


/* Sequence editing:
 */
CSequence *CMICallbacks::CreateSequence() {
	printf("CMICallbacks::%s()\n",__FUNCTION__);
	return(NULL);
}
void CMICallbacks::DeleteSequence(CSequence *pseq) {
	printf("CMICallbacks::%s(pseq=%p)\n",__FUNCTION__,pseq);
}

// special ppat values for GetSequenceData and SetSequenceData 
// empty = NULL
// <break> = (CPattern *)1
// <mute> = (CPattern *)2
// <thru> = (CPattern *)3
CPattern *CMICallbacks::GetSequenceData(int const row) {
	printf("CMICallbacks::%s(row=%d)\n",__FUNCTION__,row);
	return(NULL);
}

void CMICallbacks::SetSequenceData(int const row, CPattern *ppat) {
	printf("CMICallbacks::%s(row=%d,ppat=%p)\n",__FUNCTION__,row,ppat);
}


//-- buzz v1.2 (MI_VERSION 15) additions start here ---

void CMICallbacks::SetMachineInterfaceEx(CMachineInterfaceEx *pex) {
	printf("CMICallbacks::%s(pex=%p)\n",__FUNCTION__,pex);
	// @todo where to store that ?
}


// set value of parameter
// group 1=global, 2=track
void CMICallbacks::ControlChange__obsolete__(int group, int track, int param, int value) {
	printf("CMICallbacks::%s(group=%d,track=%d,param=%d,value=%d)\n",__FUNCTION__,group,track,param,value);
}
// set value of parameter (group & 16 == don't record)
void CMICallbacks::ControlChange(CMachine *pmac, int group, int track, int param, int value) {
	printf("CMICallbacks::%s(pmac=%p,group=%d,track=%d,param=%d,value=%d)\n",__FUNCTION__,pmac,group,track,param,value);
}

// direct calls to audiodriver, used by WaveInput and WaveOutput
// shouldn't be used for anything else
int CMICallbacks::ADGetnumChannels(bool input) {
	printf("CMICallbacks::%s(input=%d)\n",__FUNCTION__,input);
	return(0);
}

void CMICallbacks::ADWrite(int channel, float *psamples, int numsamples) {
	printf("CMICallbacks::%s(channel=%d,psamples=%p,numsamples=%d)\n",__FUNCTION__,channel,psamples,numsamples);
}

void CMICallbacks::ADRead(int channel, float *psamples, int numsamples) {
	printf("CMICallbacks::%s(channel=%d,psamples=%p,numsamples=%d)\n",__FUNCTION__,channel,psamples,numsamples);
}

// if n=1 Work(), n=2 WorkMonoToStereo()
void CMICallbacks::SetnumOutputChannels(CMachine *pmac, int n) {
	printf("CMICallbacks::%s(pmac=%p,n=%d)\n",__FUNCTION__,pmac,n);
}

void CMICallbacks::SetEventHandler(CMachine *pmac, BEventType et, EVENT_HANDLER_PTR p, void *param) {
	printf("CMICallbacks::%s(pmac=%p,et=%d,p=?,param=%p)\n",__FUNCTION__,pmac,et,/*p,*/param);
}

/* Machines:
 */
// *pout will get one name per Write()
void CMICallbacks::GetMachineNames(CMachineDataOutput *pout) {
	printf("CMICallbacks::%s(pout=%p)\n",__FUNCTION__,pout);
}

CMachine *CMICallbacks::GetMachine(char const *name) {
	printf("CMICallbacks::%s(name=\"%s\")\n",__FUNCTION__,name);
	return(NULL);
}

CMachineInfo const *CMICallbacks::GetMachineInfo(CMachine *pmac) {
	printf("CMICallbacks::%s(pmac=%p)\n",__FUNCTION__,pmac);
	return(NULL);
}

char const *CMICallbacks::GetMachineName(CMachine *pmac) {
	printf("CMICallbacks::%s(pmac=%p)\n",__FUNCTION__,pmac);
	return(NULL);
}

bool CMICallbacks::GetInput(int index, float *psamples, int numsamples, bool stereo, float *extrabuffer) {
	printf("CMICallbacks::%s(index=%d,psamples=%p,numsamples=%d,stereo=%d,extrabuffer=%p)\n",__FUNCTION__,index,psamples,numsamples,stereo,extrabuffer);
	return(FALSE);
}
