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

/* This file implements the bizarre object used by all Buzz machines that
   are statically linked to MDK.LIB.  This is necessary because, as part
   of its initialisation process, MDK.LIB makes a call to buzz with
   GetNearestWaveLevel(-1,-1), and it expects the return to be a pointer
   to some kind of 'helper' class that handles the core MDK functionality.

   The implementation below **SEEMS TO** work.  Note that definition of
   CBuzzMDKHelper, CBuzzMDKHelperInterface and CBuzzMDKHelperInterfaceEx
   *MUST NOT UNDER ANY CIRCUMSTANCES* change, because the Buzz plugins that
   make use of MDK.LIB will already assume binary compatibility with whatever
   GetNearestWaveLevel(-1,-1) has returned to it.

   Any problems with MDK-built buzz machines, this file is the most likely
   cause

*/


#include "BuzzMDKHelper.h"
#include "debug.h"

BuzzMDKHelper::BuzzMDKHelper(void) {
    ThisMachine=NULL;
	numChannels=0;
}

BuzzMDKHelper::~BuzzMDKHelper() {
}

void BuzzMDKHelper::AddInput(char const *szMachine, bool bStereo) {
	DBG2("(szMachine=%s,bStereo=%d)\n",szMachine,bStereo);
/*
	if (szMachine != NULL)
	{
		Inputs.push_back(CMachineInputInfo(szMachine, bStereo));
		SetMode();
	}
	InputIterator = Inputs.begin();
*/
}

void BuzzMDKHelper::DeleteInput(char const *szMachine) {
	DBG1("(szMachine=%s)\n",szMachine);
/*
	for (INPUTITERATOR i = Inputs.begin(); i != Inputs.end(); ++i)
	{
		if (i->strName.compare(szMachine) == 0)
		{
			Inputs.erase(i);
			SetMode();
			break;
		}
	}
	InputIterator = Inputs.begin();
*/
}

void BuzzMDKHelper::RenameInput(char const *szMachine, char const *szMachineNewName) {
	DBG2("(szMachine=%s,szMachineNewName=%s)\n",szMachine,szMachineNewName);
/*
	for (INPUTITERATOR i = Inputs.begin(); i != Inputs.end(); ++i)
	{
		if (i->strName.compare(szMachine) == 0)
		{
			i->strName = szMachineNewName;
			break;
		}
	}
*/
}

void BuzzMDKHelper::SetInputChannels(char const *szMachine, bool bStereo) {
	DBG2("(szMachine=%s,bStereo=%d)\n",szMachine,bStereo);
/*
	CMachine * pMachine = pmi->pCB->GetMachine(szMachine);
	if (!pMachine)
	{
		// machine doesn't exist!
		return;
	}

//	if (!(pMachine->cMachineInfo->Flags & MIF_MONO_TO_STEREO) )
//	{
//		// machine can't do stereo so leave this input channel as mono
//		bStereo = false;
//	}
//	else
	{
		for (INPUTITERATOR i = Inputs.begin(); i != Inputs.end(); ++i)
		{
			if (i->strName.compare(szMachine) == 0)
			{
				i->bStereo = bStereo;
				break;
			}
		}
	}

	// change from mono output to stereo output if this input is itself stereo
	// and the machine (up until now) would've been mono-output
	if (MachineWantsChannels==1)
	{
		SetOutputMode(bStereo);
	}
*/
}

void BuzzMDKHelper::Input(float *psamples, int numsamples, float fAmp) {
	DBG3("(psamples=%p,numsamples=%d,fAmp=%f)\n",psamples,numsamples,fAmp);
/*
	if (InputIterator==NULL)
	{
		return;
	}

	const double dfAmp = static_cast<double>(fAmp);
	
	if (psamples != NULL)
	{
		if (numChannels == 1)
		{
			if (HaveInput == 0)
			{
				if (InputIterator->bStereo)
				{
					CopyStereoToMono(Buffer, psamples, numsamples, dfAmp);
				}
				else
				{
					Copy(Buffer, psamples, numsamples, dfAmp);
				}
			}
			else
			{
				if (InputIterator->bStereo)
				{
					AddStereoToMono(Buffer, psamples, numsamples, dfAmp);
				}
				else
				{
					Add(Buffer, psamples, numsamples, dfAmp);
				}
			}
		}
		else
		{
			if (HaveInput == 0)
			{
				if (InputIterator->bStereo)
				{
					Copy(Buffer, psamples, numsamples*2, dfAmp);
				}
				else
				{
					CopyMonoToStereo(Buffer, psamples, numsamples, dfAmp);
				}
			}
			else
			{
				if (InputIterator->bStereo) 
				{
					Add(Buffer, psamples, numsamples*2, dfAmp);
				}
				else
				{
					AddMonoToStereo(Buffer, psamples, numsamples, dfAmp);
				}
			}
		}

		++HaveInput;
	}

	++InputIterator;
*/
}

bool BuzzMDKHelper::Work(float *psamples, int numsamples, int const wm) {
	DBG3("(psamples=%p,numsamples=%d,wm=%d)\n",psamples,numsamples,wm);
/*
    if ((wm & WM_READ) && HaveInput)
	{
		Copy(psamples, Buffer, numsamples);
	}
	else
	{
		// shouldn't need to do this, but .. some Buzz machines are crap and need it
		Zero(psamples, numsamples);
	}

	bool bOutputValid = pmi->MDKWork(psamples, numsamples, wm);

    InputIterator = Inputs.begin();
	HaveInput = 0;

	return bOutputValid;
*/
    return true;
}

bool BuzzMDKHelper::WorkMonoToStereo(float *pin, float *pout, int numsamples, int const wm) {
	DBG4("(pin=%p,pout=%p,numsamples=%d,wm=%d)\n",pin,pout,numsamples,wm);
/*
	if ((wm & WM_READ) && HaveInput)
	{
		Copy(pout, Buffer, 2*numsamples);
	}
	else
	{
		// shouldn't need to do this, but .. some Buzz machines are crap and need it
		Zero(pout, 2*numsamples);
	}

	bool bOutputValid = pmi->MDKWorkStereo(pout, numsamples, wm);

	InputIterator = Inputs.begin();
	HaveInput = 0;
	return bOutputValid;
*/
	return(true);
}

void BuzzMDKHelper::Init(CMachineDataInput * const pi) {
	DBG1("(pi=%p)\n",pi);
/*
    ThisMachine = pmi->pCB->GetThisMachine();

	numChannels = 1;

    Inputs.clear();
	InputIterator = Inputs.begin();
	HaveInput = 0;
	MachineWantsChannels = 1;

    // Buzz seems to store a dummy initial byte here - maybe
	// some kind of version tag?
	byte byDummy;
	pi->Read(byDummy);

	pmi->MDKInit(pi);

	pInnerEx = pmi->GetEx();
*/
}

void BuzzMDKHelper::Save(CMachineDataOutput * const po) {
	DBG1("(po=%p)\n",po);
/*
	pmi->MDKSave(po);
*/
}

void BuzzMDKHelper::SetOutputMode(bool bStereo) {
	DBG1("(bStereo=%d)\n",bStereo);
/*
//	if ( !(ThisMachine->cMachineInfo->Flags & MIF_MONO_TO_STEREO) )
//	{
//		// even MDK machines need to specify MIF_MONO_TO_STEREO if they are stereo!
//		// Otherwise SetOutputMode(true) will be ignored by Buzz
//		bStereo = false;
//		return;
//	}
	numChannels = bStereo ? 2 : 1;
	MachineWantsChannels = numChannels;

	pmi->pCB->SetnumOutputChannels(ThisMachine, numChannels);
	pmi->OutputModeChanged(bStereo);
*/
}

void BuzzMDKHelper::SetMode() {
	DBG("()\n");
/*
	InputIterator = Inputs.begin();
	HaveInput = 0;
	
	if (MachineWantsChannels > 1)
	{
		numChannels = MachineWantsChannels;
		pmi->pCB->SetnumOutputChannels(ThisMachine, numChannels);
		pmi->OutputModeChanged(numChannels > 1 ? true : false);
		return;
	}


	for (INPUTITERATOR i = Inputs.begin(); i != Inputs.end(); ++i)
	{
		if (i->bStereo)
		{
			numChannels = 2;
			pmi->pCB->SetnumOutputChannels(ThisMachine, numChannels);
			pmi->OutputModeChanged(numChannels > 1 ? true : false);
			return;
		}
	}

	numChannels = 1;
	pmi->pCB->SetnumOutputChannels(ThisMachine, numChannels);
	pmi->OutputModeChanged(numChannels > 1 ? true : false);
*/
}

void BuzzMDKHelper::MidiControlChange(const int ctrl, const int channel, const int value ) {
	DBG3("(ctrl=%d,channel=%d,value=%d)\n",ctrl,channel,value);
/*
	if( pInnerEx != NULL )
		pInnerEx->MidiControlChange( ctrl, channel, value );
*/
}
