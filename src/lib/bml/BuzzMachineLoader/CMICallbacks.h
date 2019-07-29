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

#ifndef MI_CALLBACKS_H
#define MI_CALLBACKS_H

#include <stdint.h>

typedef uint16_t word;
typedef uint32_t dword;

struct CWaveInfo
{
	int Flags;
	float Volume;
};

struct CWaveLevel
{
	int numSamples;
	short *pSamples;
	int RootNote;
	int SamplesPerSec;
	int LoopStart;
	int LoopEnd;
};

class CMachine;
class CMachineDataOutput;
struct CMachineInfo;
class CMachineInterface;
class CMachineInterfaceEx;
class CPattern;
class CSequence;

enum BEventType { };
typedef bool (CMachineInterface::*EventHandlerPtr)(void *);

class CMICallbacks
{
public:
	virtual CWaveInfo const *GetWave(int const i) { return NULL; }
	virtual CWaveLevel const *GetWaveLevel(int const i, int const level) { return NULL; }
	virtual void MessageBox(char const *txt) {}
	virtual void Lock() {}
	virtual void Unlock() {}
	virtual int GetWritePos() { return 0; }
	virtual int GetPlayPos() { return 0; }
	virtual float *GetAuxBuffer() { return NULL; }
	virtual void ClearAuxBuffer() {}
	virtual int GetFreeWave() { return 0; }
	virtual bool AllocateWave(int const i, int const size, char const *name) { return false; }
	virtual void ScheduleEvent(int const time, dword const data) {}
	virtual void MidiOut(int const dev, dword const data) {}
	virtual short const *GetOscillatorTable(int const waveform) { return NULL; }
  virtual int GetEnvSize(int const wave, int const env) { return 0; }
	virtual bool GetEnvPoint(int const wave, int const env, int const i, word &x, word &y, int &flags) { return false; }
	virtual CWaveLevel const *GetNearestWaveLevel(int const i, int const note) { return NULL; }
  virtual void SetNumberOfTracks(int const n) {}
	virtual CPattern *CreatePattern(char const *name, int const length) { return NULL; }
	virtual CPattern *GetPattern(int const index) { return NULL; }
	virtual char const *GetPatternName(CPattern *ppat) { return NULL; }
	virtual void RenamePattern(char const *oldname, char const *newname) {}
	virtual void DeletePattern(CPattern *ppat) {}
	virtual int GetPatternData(CPattern *ppat, int const row, int const group, int const track, int const field) { return 0; }
	virtual void SetPatternData(CPattern *ppat, int const row, int const group, int const track, int const field, int const value) {}
  virtual CSequence *CreateSequence() { return NULL; }
	virtual void DeleteSequence(CSequence *pseq) {}
  virtual CPattern *GetSequenceData(int const row) { return NULL; }
	virtual void SetSequenceData(int const row, CPattern *ppat) {}
  virtual void SetMachineInterfaceEx(CMachineInterfaceEx *pex) {}
	virtual void ControlChange__obsolete__(int group, int track, int param, int value) {}
	virtual int ADGetnumChannels(bool input) { return 0; }
	virtual void ADWrite(int channel, float *psamples, int numsamples) {}
	virtual void ADRead(int channel, float *psamples, int numsamples) {}
	virtual CMachine *GetThisMachine() { return NULL; }
	virtual void ControlChange(CMachine *pmac, int group, int track, int param, int value) {}
	virtual CSequence *GetPlayingSequence(CMachine *pmac) { return NULL; }
	virtual void *GetPlayingRow(CSequence *pseq, int group, int track) { return NULL; }
	virtual int GetStateFlags() { return 0; }
	virtual void SetnumOutputChannels(CMachine *pmac, int n) {}
	virtual void SetEventHandler(CMachine *pmac, BEventType et, EventHandlerPtr p, void *param) {}
	virtual char const *GetWaveName(int const i) { return NULL; }
	virtual void SetInternalWaveName(CMachine *pmac, int const i, char const *name) {}
	virtual void GetMachineNames(CMachineDataOutput *pout) {}
	virtual CMachine *GetMachine(char const *name) { return NULL; }
	virtual CMachineInfo const *GetMachineInfo(CMachine *pmac) { return NULL; }
	virtual char const *GetMachineName(CMachine *pmac) { return NULL; }
	virtual bool GetInput(int index, float *psamples, int numsamples, bool stereo, float *extrabuffer) { return false; }
};

#endif // MI_CALLBACKS_H
