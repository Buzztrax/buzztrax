/* Buzz Machine Loader
 * Copyright (C) 2003-2007 Anders Ervik <calvin@countzero.no>
 * Copyright (C) 2006-2007 Leonard Ritter
 * Copyright (C) 2008-2015 Buzztrax team <buzztrax-devel@buzztrax.org>
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
/* This is a cutdown stub of the machine interface derived from buzzmachine
 * sources and armstrong/libzubb (LGPL) - see 
 * https://bitbucket.org/paniq/armstrong/src/libzzub
 * It is not sufficient to compile machines. Use the buzzmachine sdk
 * for that.
 */

#ifndef MACHINE_INTERFACE_H
#define MACHINE_INTERFACE_H     

#include <string.h>

typedef unsigned long dword;

enum CMPType { };

struct CMachineParameter
{
	CMPType Type;
	char const *Name, *Description;
	int MinValue, MaxValue, NoValue, Flags, DefValue;
};

struct CMachineAttribute
{
	char const *Name;
	int MinValue, MaxValue, DefValue;
};

struct CMasterInfo
{
	int BeatsPerMin, TicksPerBeat, SamplesPerSec, SamplesPerTick, PosInTick;
	float TicksPerSec;
};

class CMachineInterfaceEx;
class CMachineDataInput;
class CMachineDataOutput;

#include "CLibInterface.h" 

struct CMachineInfo
{
	int Type, Version, Flags, minTracks, maxTracks;
	int numGlobalParameters, numTrackParameters;
	CMachineParameter const **Parameters;
	int numAttributes;
	CMachineAttribute const **Attributes;
	char const *Name, *ShortName, *Author, *Commands;
	CLibInterface *pLI;
};

struct CEnvelopeInfo
{
	char const *Name;
	int Flags;
};

#include "CMICallbacks.h"

class CMachineInterface
{
public:
	virtual ~CMachineInterface() {}
	virtual void Init(CMachineDataInput * const pi) {}
	virtual void Tick() {}
	virtual bool Work(float *psamples, int numsamples, int const mode) { return false; }
	virtual bool WorkMonoToStereo(float *pin, float *pout, int numsamples, int const mode) { return false; }
	virtual void Stop() {}
	virtual void Save(CMachineDataOutput * const po) {}
	virtual void AttributesChanged() {}
	virtual void Command(int const i) {}
	virtual void SetNumTracks(int const n) {}
	virtual void MuteTrack(int const i) {}
	virtual bool IsTrackMuted(int const i) const { return false; }
	virtual void MidiNote(int const channel, int const value, int const velocity) {}
	virtual void Event(dword const data) {}
	virtual char const *DescribeValue(int const param, int const value) { return NULL; }
	virtual CEnvelopeInfo const **GetEnvelopeInfos() { return NULL; }
	virtual bool PlayWave(int const wave, int const note, float const volume) { return false; }
	virtual void StopWave() {}
	virtual int GetWaveEnvPlayPos(int const env) { return -1; }
public:
	void *GlobalVals;
	void *TrackVals;
	int *AttrVals;
	CMasterInfo *pMasterInfo;
	CMICallbacks *pCB;
};

class CMachineInterfaceEx
{
public:
  //virtual ~CMachineInterfaceEx() {}
	virtual char const *DescribeParam(int const param) { return NULL; }
	virtual bool SetInstrument(char const *name) { return false; }
	virtual void GetSubMenu(int const i, CMachineDataOutput *pout) {}
	virtual void AddInput(char const *macname, bool stereo) {}
	virtual void DeleteInput(char const *macename) {}
	virtual void RenameInput(char const *macoldname, char const *macnewname) {}
	virtual void Input(float *psamples, int numsamples, float amp) {}
	virtual void MidiControlChange(int const ctrl, int const channel, int const value) {}
	virtual void SetInputChannels(char const *macname, bool stereo) {}
	virtual bool HandleInput(int index, int amp, int pan) { return false; }
	virtual void Dummy1() {}
	virtual void Dummy2() {}
	virtual void Dummy3() {}
	virtual void Dummy4() {}
	virtual void Dummy5() {}
	virtual void Dummy6() {}
	virtual void Dummy7() {}
	virtual void Dummy8() {}
	virtual void Dummy9() {}
	virtual void Dummy10() {}
	virtual void Dummy11() {}
	virtual void Dummy12() {}
	virtual void Dummy13() {}
	virtual void Dummy14() {}
	virtual void Dummy15() {}
	virtual void Dummy16() {}
	virtual void Dummy17() {}
	virtual void Dummy18() {}
	virtual void Dummy19() {}
	virtual void Dummy20() {}
	virtual void Dummy21() {}
	virtual void Dummy22() {}
	virtual void Dummy23() {}
	virtual void Dummy24() {}
	virtual void Dummy25() {}
	virtual void Dummy26() {}
	virtual void Dummy27() {}
	virtual void Dummy28() {}
	virtual void Dummy29() {}
	virtual void Dummy30() {}
	virtual void Dummy31() {}
	virtual void Dummy32() {}
};

#endif /* MACHINE_INTERFACE_H */
