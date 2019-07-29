/*
 * Copyright (C) 2000 Oskari Tammelin (ot@iki.fi)
 *
 * This header file may be used to write _freeware_ DLL "machines" for Buzz.
 * Using it for anything else is not allowed without a permission from the
 * author.
 *
 * http://jeskola.net/archive/buzz/a15/Dev/
 *
 * The buzztrax project owns a buzzlib license to use this header inside libbml.
 * Contact: Stefan Sauer <ensonic@users.sf.net>
 */
#ifndef __BUZZ_DSPLIB_RESAMPLE_H
#define __BUZZ_DSPLIB_RESAMPLE_H

#include <stdint.h>
typedef uint8_t byte;
typedef uint16_t word;
typedef uint32_t dword;

// interpolation modes
#define RSI_NONE		0		// nearest sample (no interpolation) [fast, poor quality]
#define RSI_LINEAR		1		// linear interpolation				 [quite fast, better quality]

// amp modes
#define RSA_ONE			0		// amp = 1.0
#define RSA_CONSTANT	1		// constant amp
#define RSA_LINEAR_INTP	2		// linear amp interpolation

// step modes
#define RSS_ONE			0		// step = 1.0
#define RSS_CONSTANT	1		// constant step

// flags
#define RSF_ADD			1		// add input to output instead of moving
#define RSF_FLOAT		2		// input samples are 32bit floats (16bit integers by default)

#define RS_STEP_FRAC_BITS	24

class CResamplerParams
{
public:
	void SetStep(double const s)
	{
		StepInt = (int)s;
		StepFrac = (int)((s - StepInt) * (1 << RS_STEP_FRAC_BITS));
	}

public:
	void *Samples;				// ptr to first sample
	int numSamples;				// number of samples (or loop)  
	int LoopBegin;				// zero based index to Samples, -1 = no loop
	int StepInt;
	dword StepFrac;
	float AmpStep;				// used if AmpMode == RSA_LINEAR_INTP

	byte Interpolation;			// one of RSI_*
	byte AmpMode;				// one of RSA_*
	byte StepMode;				// one of RSS_*
	byte Flags;					// any of RSF_* ORred

};

class CResamplerState
{
public:
	int PosInt; 
	dword PosFrac;
	float Amp;
	bool Active;					
};

#endif
