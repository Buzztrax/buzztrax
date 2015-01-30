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

#ifndef BUZZ_MDK_HELPER_H
#define BUZZ_MDK_HELPER_H

#include "MachineInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

class BuzzMDKHelper {
private:
public:
	BuzzMDKHelper(void);
	virtual ~BuzzMDKHelper();
	virtual void AddInput(const char *szMachine, bool bStereo);
	virtual void DeleteInput(const char *szMachine);
	virtual void RenameInput(const char *szMachine, const char *szMachineNewName);
	virtual void SetInputChannels(const char *szMachine, bool bStereo);
	virtual void Input(float *psamples, int numsamples, float fAmp);
	virtual bool Work(float *psamples, int numsamples, int wm);
	virtual bool WorkMonoToStereo(float *pin, float *pout, int numsamples, int wm);
	virtual void Init(CMachineDataInput * const pi);
	virtual void Save(CMachineDataOutput * const po);
	virtual void SetOutputMode(bool bStereo);
	virtual void SetMode();
    // CMDKImplementation does not have this
	virtual void MidiControlChange(int const ctrl, int const channel, int const value);

public:
private:
    // CMDKImplementation has different members here
    CMachine *ThisMachine;
    int numChannels;
    float Buffer[2*8192];
};

#ifdef __cplusplus
}
#endif


#endif // BUZZ_MDK_HELPER_H

