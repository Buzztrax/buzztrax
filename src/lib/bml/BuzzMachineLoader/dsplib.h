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
#ifndef __BUZZ_DSPLIB_H
#define __BUZZ_DSPLIB_H

/*

	How to use dsplib in Buzz machines:

		Add dsplib.lib to library modules list of your project and #include 
		this file. After that you can simply call the functions you need.

		Note: Some of these functions may not be implemented yet. You will 
		get a link error if you try to use them.
  
	Descriptions for the abbrvs. used in this file:
  
		ps - pointer to samples
		pin - pointer to input samples
		pout - pointer to output samples
		n - number of samples 
		a - amplitude scaling factor
		la - left a
		ra - right a
		M - mono 
		2 - to
		S - stereo

*/

typedef unsigned long dword;

#if defined(_MSC_VER) || defined(_MINGW_VER)
#define DI __declspec(dllimport)
#else
#define DI 
#endif

// initialization

// you don't need to call DSP_Init in machines 
// buzz uses the same dll so it has done it already 
#if defined(_MSC_VER) || defined(_MINGW_VER)
DI void __fastcall DSP_Init(int const samplerate);
#else
// fastcall seems to be x86 only
#if defined(__i386__) && !defined(__LP64__) 
DI void __attribute__ ((fastcall)) DSP_Init(int const samplerate);
#else
DI void DSP_Init(int const samplerate);
#endif
#endif

// basic stuff

DI void DSP_Zero(float *ps, dword const n);
	
DI void DSP_Copy(float *pout, float const *pin, dword const n);
DI void DSP_Copy(float *pout, float const *pin, dword const n, float const a);

DI void DSP_CopyM2S(float *pout, float const *pin, dword const n);
DI void DSP_CopyM2S(float *pout, float const *pin, dword const n, float const a);
DI void DSP_CopyM2S(float *pout, float const *pin, dword const n, float const la, float const ra); 

DI void DSP_CopyS2MOneChannel(float *pout, float const *pin, dword const n, float const a);

DI void DSP_Add(float *pout, float const *pin, dword const n);
DI void DSP_Add(float *pout, float const *pin, dword const n, float const a);

DI void DSP_AddM2S(float *pout, float const *pin, dword const n);
DI void DSP_AddM2S(float *pout, float const *pin, dword const n, float const a);
DI void DSP_AddM2S(float *pout, float const *pin, dword const n, float const la, float const ra); 

DI void DSP_AddS2S(float *pout, float const *pin, dword const n);
DI void DSP_AddS2S(float *pout, float const *pin, dword const n, float const a);
DI void DSP_AddS2S(float *pout, float const *pin, dword const n, float const la, float const ra); 

DI void DSP_AddS2MOneChannel(float *pout, float const *pin, dword const n, float const a);
DI void DSP_AddS2SOneChannel(float *pout, float const *pin, dword const n, float const a);

DI void DSP_Amp(float *ps, dword const n, float const a);

// second order butterworth filters

#include "bw.h"
 
DI void DSP_BW_Reset(CBWState &s);	// clears past inputs & outputs 

DI void DSP_BW_InitLowpass(CBWState &s, float const f);
DI void DSP_BW_InitHighpass(CBWState &s, float const f);
DI void DSP_BW_InitBandpass(CBWState &s, float const f, float const bw);
DI void DSP_BW_InitBandreject(CBWState &s, float const f, float const bw);

DI bool DSP_BW_Work(CBWState &s, float *ps, dword const n, int const mode);
DI bool DSP_BW_WorkStereo(CBWState &s, float *ps, dword const n, int const mode);

// resampler

#include "resample.h"

DI void DSP_Resample(float *pout, int numsamples, CResamplerState &state, CResamplerParams const &params);

#undef DI

#endif

