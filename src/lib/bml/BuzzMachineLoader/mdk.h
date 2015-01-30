/*
 * Copyright (C) 2000 Oskari Tammelin (ot@iki.fi)
 *
 * This header file may be used to write _freeware_ DLL "machines" for Buzz.
 * Using it for anything else is not allowed without a permission from the
 * author.
 *
 * The buzztrax project owns a buzzlib license to use this header
 * inside libbml.
 * Contact: Stefan Sauer <ensonic@users.sf.net>
 */
     
#ifndef __BUZZ_MDK
#define __BUZZ_MDK

#include "MachineInterface.h"

class CMDKImplementation;
class CMDKMachineInterfaceEx;

class CMDKMachineInterface : public CMachineInterface
{
public:
	virtual ~CMDKMachineInterface();
	virtual void Init(CMachineDataInput * const pi);
	virtual bool Work(float *psamples, int numsamples, int const mode);
	virtual bool WorkMonoToStereo(float *pin, float *pout, int numsamples, int const mode);

	virtual void Save(CMachineDataOutput * const po);

public:
	void SetOutputMode(bool stereo);

public:
	virtual CMDKMachineInterfaceEx *GetEx() = 0;
	virtual void OutputModeChanged(bool stereo) = 0;

	virtual bool MDKWork(float *psamples, int numsamples, int const mode) = 0;
	virtual bool MDKWorkStereo(float *psamples, int numsamples, int const mode) = 0;

	virtual void MDKInit(CMachineDataInput * const pi) = 0;
	virtual void MDKSave(CMachineDataOutput * const po) = 0;


private:
	CMDKImplementation *pImp;

};

class CMDKMachineInterfaceEx : public CMachineInterfaceEx
{
public:
	friend class CMDKMachineInterface;

	virtual void AddInput(char const *macname, bool stereo);	// called when input is added to a machine
	virtual void DeleteInput(char const *macename);			
	virtual void RenameInput(char const *macoldname, char const *macnewname);			

	virtual void Input(float *psamples, int numsamples, float amp); // if MIX_DOES_INPUT_MIXING


	virtual void SetInputChannels(char const *macname, bool stereo);

private:
	CMDKImplementation *pImp;
};


#endif
