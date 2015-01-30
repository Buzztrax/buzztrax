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
#ifndef __MDK_IMP_H
#define __MDK_IMP_H

#include <list>
#include <string>

#include "MachineInterface.h"
#include "mdk.h"

#define MDK_VERSION		2

class CInput
{
public:
	CInput(char const *n, bool st) : Name(n), Stereo(st) {}

public:
	std::string Name;
	bool Stereo;

};

typedef std::list<CInput> InputList;

class CMDKImplementation
{
	friend class CMDKMachineInterface;
	friend class CMDKMachineInterfaceEx;
public:
    // hmm, we need to call mdkHelper->Init(pcmdii); instead but that crashes
    /*
    CMDKImplementation() {
      HaveInput=0;
      numChannels=1;
      MachineWantsChannels=1;
    }
    */
	virtual ~CMDKImplementation();

	virtual void AddInput(char const *macname, bool stereo);
	virtual void DeleteInput(char const *macname);
	virtual void RenameInput(char const *macoldname, char const *macnewname);
	virtual void SetInputChannels(char const *macname, bool stereo);
	virtual void Input(float *psamples, int numsamples, float amp);

	virtual bool Work(float *psamples, int numsamples, int const mode);
	virtual bool WorkMonoToStereo(float *pin, float *pout, int numsamples, int const mode);
	virtual void Init(CMachineDataInput * const pi);
	virtual void Save(CMachineDataOutput * const po);
	
	virtual void SetOutputMode(bool stereo);
	
protected:	
	void SetMode();


public:
	CMDKMachineInterface *pmi;

	InputList Inputs;
	InputList::iterator InputIterator;

	int HaveInput;
	int numChannels;
	int MachineWantsChannels;

	CMachine *ThisMachine;
	
	float Buffer[2*MAX_BUFFER_LENGTH];
};

#endif
