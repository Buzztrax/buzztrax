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

#include "mdk.h"
#include "mdkimp.h"
#include "debug.h"

/*
CMDKImplementation *NewMDKImp()
{
	return new CMDKImplementation;
}
*/

void CopyStereoToMono(float *pout, float *pin, int numsamples, float amp)
{
	do
	{
		*pout++ = (pin[0] + pin[1]) * amp;
		pin += 2;
	} while(--numsamples);
}

void AddStereoToMono(float *pout, float *pin, int numsamples, float amp)
{
	do
	{
		*pout++ += (pin[0] + pin[1]) * amp;
		pin += 2;
	} while(--numsamples);
}

void CopyM2S(float *pout, float *pin, int numsamples, float amp)
{
	do
	{
		double s = *pin++ * amp;
		pout[0] = (float)s;
		pout[1] = (float)s;
		pout += 2;
	} while(--numsamples);

}

void Add(float *pout, float *pin, int numsamples, float amp)
{
	do
	{
		*pout++ += *pin++ * amp;
	} while(--numsamples);
}

// begin dsplib --- 

#define UNROLL		4			// how many times to unroll inner loops
#define SUNROLL		2			// same for loops with stereo output

void DSP_Copy(float *pout, float const *pin, dword const n, float const a)
{

	double const amp = a;	// copy a to fpu stack 

	if (n >= UNROLL)
	{
		int c = n / UNROLL;
		do
		{
			pout[0] = (float)(pin[0] * amp);
			pout[1] = (float)(pin[1] * amp);
			pout[2] = (float)(pin[2] * amp);
			pout[3] = (float)(pin[3] * amp);
			pin += UNROLL;
			pout += UNROLL; 
		} while(--c);
	}

	int c = n & (UNROLL-1);
	while(c--)
		*pout++ = (float)(*pin++ * amp);

 
}


void DSP_Add(float *pout, float const *pin, dword const n, float const a)
{

	double const amp = a;	// copy a to fpu stack 


	if (n >= UNROLL)
	{
		int c = n / UNROLL;
		do
		{
			pout[0] += (float)(pin[0] * amp);
			pout[1] += (float)(pin[1] * amp);
			pout[2] += (float)(pin[2] * amp);
			pout[3] += (float)(pin[3] * amp);
			pin += UNROLL;
			pout += UNROLL; 
		} while(--c);
	}

	int c = n & (UNROLL-1);
	while(c--)
		*pout++ += (float)(*pin++ * amp);

 
}


void DSP_AddM2S(float *pout, float const *pin, dword const n, float const a)
{
	double const amp = a;	// copy a to fpu stack 

	if (n >= SUNROLL)
	{
		int c = n / SUNROLL;
		do
		{
			double s = pin[0] * amp;
			pout[0] += (float)s;
			pout[1] += (float)s;
			
			s = pin[1] * amp;
			pout[2] += (float)s;
			pout[3] += (float)s;
			
			pin += SUNROLL;
			pout += SUNROLL*2; 
		} while(--c);
	}
 
	int c = n & (SUNROLL-1);
	while(c--)
	{
		double const s = *pin++ * amp;
		pout[0] += (float)s;
		pout[1] += (float)s;
		pout += 2;
	}
} 


void DSP_Copy(float *pout, float const *pin, dword const n)
{
	memcpy(pout, pin, n*sizeof(float));
}

// --- end dsplib


void CMDKImplementation::AddInput(char const *macname, bool stereo)
{
	if (macname == NULL)
		return;

	Inputs.push_back(CInput(macname, stereo));

	SetMode();
}

void CMDKImplementation::DeleteInput(char const *macname)
{
	for (InputList::iterator i = Inputs.begin(); i != Inputs.end(); i++)
	{
		if ((*i).Name.compare(macname) == 0)
		{

			Inputs.erase(i);

			SetMode();
			return;
		}
	}
}

void CMDKImplementation::RenameInput(char const *macoldname, char const *macnewname)
{
	for (InputList::iterator i = Inputs.begin(); i != Inputs.end(); i++)
	{
		if ((*i).Name.compare(macoldname) == 0)
		{
			(*i).Name = macnewname;
			return;
		}
	}
}

void CMDKImplementation::SetInputChannels(char const *macname, bool stereo)
{
	for (InputList::iterator i = Inputs.begin(); i != Inputs.end(); i++)
	{
		if ((*i).Name.compare(macname) == 0)
		{
			(*i).Stereo = stereo;
			SetMode();
			return;
		}
	}
}

void CMDKImplementation::Input(float *psamples, int numsamples, float amp)
{
	assert(InputIterator != Inputs.end());

	if (psamples == NULL)
	{ 
		InputIterator++;
		return;
	}

    DBG2("numChannels=%d, HaveInput=%d",numChannels,HaveInput);
	if (numChannels == 1)
	{
		if (HaveInput == 0)
		{
			if ((*InputIterator).Stereo)
				CopyStereoToMono(Buffer, psamples, numsamples, amp);
			else
				DSP_Copy(Buffer, psamples, numsamples, amp);
		}
		else
		{
			if ((*InputIterator).Stereo)
				AddStereoToMono(Buffer, psamples, numsamples, amp);
			else
				DSP_Add(Buffer, psamples, numsamples, amp);
		}
	}
	else
	{
		if (HaveInput == 0)
		{
			if ((*InputIterator).Stereo)
				DSP_Copy(Buffer, psamples, numsamples*2, amp);
			else
				CopyM2S(Buffer, psamples, numsamples, amp);
		}
		else
		{
			if ((*InputIterator).Stereo) 
				DSP_Add(Buffer, psamples, numsamples*2, amp);
			else
				DSP_AddM2S(Buffer, psamples, numsamples, amp);
		}
	}

	HaveInput++;
	InputIterator++;

}

bool CMDKImplementation::Work(float *psamples, int numsamples, int const mode)
{
    DBG4("(%p,%d,%d), HaveInput=%d\n",psamples,numsamples,mode,HaveInput);
  
	if ((mode & WM_READ) && HaveInput)
		DSP_Copy(psamples, Buffer, numsamples);

	bool ret = pmi->MDKWork(psamples, numsamples, mode);

	InputIterator = Inputs.begin();
	HaveInput = 0;

	return ret;
}

bool CMDKImplementation::WorkMonoToStereo(float *pin, float *pout, int numsamples, int const mode)
{
    DBG5("(%p,%p,%d,%d), HaveInput=%d\n",pin,pout,numsamples,mode,HaveInput);

    // fill pout from mdkBuffer
	if ((mode & WM_READ) && HaveInput)
		DSP_Copy(pout, Buffer, 2*numsamples);

	bool ret = pmi->MDKWorkStereo(pout, numsamples, mode);

    // we don't use that right now
	InputIterator = Inputs.begin();
	HaveInput = 0;
	
	return ret;
}

	
void CMDKImplementation::Init(CMachineDataInput * const pi)
{
    DBG("  CMDKImplementation::Init() called\n");
	ThisMachine = pmi->pCB->GetThisMachine();
	
	numChannels = 1;

	InputIterator = Inputs.begin();
	HaveInput = 0;
	MachineWantsChannels = 1;

	if (pi != NULL)
	{
		byte ver;
		pi->Read(ver);
	}
	

	pmi->MDKInit(pi);
}

void CMDKImplementation::Save(CMachineDataOutput * const po)
{
	po->Write((byte)MDK_VERSION);

	pmi->MDKSave(po);
}

void CMDKImplementation::SetOutputMode(bool stereo)
{
	numChannels = stereo ? 2 : 1;
	MachineWantsChannels = numChannels;
	
	pmi->OutputModeChanged(stereo);
}

void CMDKImplementation::SetMode()
{	
	InputIterator = Inputs.begin();
	HaveInput = 0;
	
	if (MachineWantsChannels > 1)
	{
		numChannels = MachineWantsChannels;
		pmi->pCB->SetnumOutputChannels(ThisMachine, numChannels);
		pmi->OutputModeChanged(numChannels > 1 ? true : false);
		return;
	}


	for (InputList::iterator i = Inputs.begin(); i != Inputs.end(); i++)
	{
		if ((*i).Stereo)
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

}

CMDKImplementation::~CMDKImplementation()
{
}


void CMDKMachineInterfaceEx::AddInput(char const *macname, bool stereo) { pImp->AddInput(macname, stereo); }
void CMDKMachineInterfaceEx::DeleteInput(char const *macename) { pImp->DeleteInput(macename); }
void CMDKMachineInterfaceEx::RenameInput(char const *macoldname, char const *macnewname) { pImp->RenameInput(macoldname, macnewname); }
void CMDKMachineInterfaceEx::SetInputChannels(char const *macname, bool stereo) { pImp->SetInputChannels(macname, stereo); }
void CMDKMachineInterfaceEx::Input(float *psamples, int numsamples, float amp) { pImp->Input(psamples, numsamples, amp); }

bool CMDKMachineInterface::Work(float *psamples, int numsamples, int const mode) { return pImp->Work(psamples, numsamples, mode); }
bool CMDKMachineInterface::WorkMonoToStereo(float *pin, float *pout, int numsamples, int const mode) { return pImp->WorkMonoToStereo(pin, pout, numsamples, mode); }
void CMDKMachineInterface::Save(CMachineDataOutput * const po) { pImp->Save(po); }

void CMDKMachineInterface::SetOutputMode(bool stereo) { pImp->SetOutputMode(stereo); }


CMDKMachineInterface::~CMDKMachineInterface()
{
	delete pImp;
}

void CMDKMachineInterface::Init(CMachineDataInput * const pi)
{
    DBG("  CMDKMachineInterface::Init() called\n");
    pImp = (CMDKImplementation*)pCB->GetNearestWaveLevel(-1,-1);
	pImp->pmi = this;

	CMDKMachineInterfaceEx *pex = GetEx();
	pex->pImp = pImp;
	pCB->SetMachineInterfaceEx(pex);

	pImp->Init(pi);
}
