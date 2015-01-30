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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#include <windows.h>
typedef DWORD dword;
#else
#include "windef.h"
#include <stdint.h>
typedef uint32_t dword;
#endif

#if defined(_MSC_VER) || defined(_MINGW_VER)
#define DE __declspec(dllexport)
#else
#define DE 
#endif

#ifndef M_PI
#define M_PI 3.141592654
#endif

// work modes
#define WM_NOIO		0
#define WM_READ		1
#define WM_WRITE	2
#define WM_READWRITE	3

#define BW_SETTLE_TIME          256

#define QUIET 0.1

static int sampleRate = 1;

extern "C" {

DE void __fastcall DSP_Init(int const samplerate)
{
	sampleRate = samplerate;
}

};

// basic stuff

DE void __fastcall DSP_Zero(float *pout, dword const n)
{
    for (dword i = 0; i < n; i++)
        pout[i] = 0.f;
}

DE void __fastcall DSP_Copy(float *pout, float const *pin, dword const n)
{
    for (dword i = 0; i < n; i++)
        pout[i] = pin[i];
}

DE void __fastcall DSP_Copy(float *pout, float const *pin, dword const n, float const a)
{
    for (dword i = 0; i < n; i++)
        pout[i] = pin[i] * a;
}

DE void __fastcall DSP_Add(float *pout, float const *pin, dword const n)
{
    for (dword i = 0; i < n; i++)
        pout[i] += pin[i];
}

DE void __fastcall DSP_Add(float *pout, float const *pin, dword const n, float const a)
{
    for (dword i = 0; i < n; i++)
        pout[i] += pin[i] * a;
}

DE void __fastcall DSP_CopyM2S(float *pout, float const *pin, dword const n)
{
    for (dword i = 0; i < n; i++)
        pout[2 * i] = pout[2 * i + 1] = pin[i];
}

DE void __fastcall DSP_CopyM2S(float *pout, float const *pin, dword const n, float const a)
{
    for (dword i = 0; i < n; i++)
        pout[2 * i] = pout[2 * i + 1] = pin[i] * a;
}

DE void __fastcall DSP_CopyM2S(float *pout, float const *pin, dword const n, float const la, float const ra)
{
    for (dword i = 0; i < n; i++)
    {
        pout[2 * i] = pin[i] * la;
        pout[2 * i + 1] = pin[i] * ra;
    }
}

DE void __fastcall DSP_CopyS2MOneChannel(float *pout, float const *pin, dword const n, float const a)
{
    for (dword i = 0; i < n; i++)
        pout[i] = pin[2 * i] * a;
}

DE void __fastcall DSP_AddM2S(float *pout, float const *pin, dword const n)
{
    for (dword i = 0; i < n; i++)
    {
        pout[2 * i] += pin[i];
        pout[2 * i + 1] += pin[i];
    }
}

DE void __fastcall DSP_AddM2S(float *pout, float const *pin, dword const n, float const a)
{
    for (dword i = 0; i < n; i++)
    {
        float ia = pin[i] * a;
        pout[2 * i] += ia;
        pout[2 * i + 1] += ia;
    }
}

DE void __fastcall DSP_AddM2S(float *pout, float const *pin, dword const n, float const la, float const ra)
{
    for (dword i = 0; i < n; i++)
    {
        pout[2 * i] += pin[i] * la;
        pout[2 * i + 1] += pin[i] * ra;
    }
}

DE void __fastcall DSP_AddS2S(float *pout, float const *pin, dword const n)
{
    for (dword i = 0; i < 2 * n; i += 2)
    {
        pout[i] += pin[i];
        pout[i + 1] += pin[i + 1];
    }
}

DE void __fastcall DSP_AddS2S(float *pout, float const *pin, dword const n, float const a)
{
    for (dword i = 0; i < 2 * n; i += 2)
    {
        pout[i] += pin[i] * a;
        pout[i + 1] += pin[i + 1] * a;
    }
}

DE void __fastcall DSP_AddS2S(float *pout, float const *pin, dword const n, float const la, float const ra)
{
    for (dword i = 0; i < 2 * n; i += 2)
    {
        pout[i] += pin[i] * la;
        pout[i + 1] += pin[i + 1] * ra;
    }
}

DE void __fastcall DSP_Amp(float *ps, dword const n, float const a)
{
    for (dword i = 0; i < n; i++)
        ps[i] *= a;
}

DE void __fastcall DSP_AddS2MOneChannel(float *pout, float const *pin, dword const n, float const a)
{
    for (dword i = 0; i < n; i++)
        pout[i] += pin[2 * i] * a;
}

DE void __fastcall DSP_AddS2SOneChannel(float *pout, float const *pin, dword const n, float const a)
{
    // huh?
    for (dword i = 0; i < 2 * n; i += 2) {
        pout[i] += pin[i] * a;
        pout[i + 1] += pin[i] * a;
    }
}

// second order butterworth filters

extern "C" {

class CBWState
{
public:
	float a[5];	// coefficients
	float i[2];	// past inputs
	float o[2];	// past outputs
	float ri[2];	// past right inputs (for stereo mode)
	float ro[2];	// past right outputs
	int IdleCount;
};

DE void  __fastcall DSP_BW_Reset(CBWState &s)
{
 	s.i[0] = 0;
	s.i[1] = 0;
	s.o[0] = 0;
	s.o[1] = 0;
	s.ri[0] = 0;
	s.ri[1] = 0;
	s.ro[0] = 0;
	s.ro[1] = 0;
	s.IdleCount = 0;
}

/* rbj */
DE void  __fastcall DSP_BW_InitLowpass(CBWState &s, float const f)
{
	float w0 = 2.0 * M_PI * f / (float)sampleRate;
	float alpha = sin(w0) / (2 * 1.0 / sqrt(2.0));

	float b0 = (1 - cos(w0)) / 2.0;
	float b1 =  1 - cos(w0);
	float b2 = (1 - cos(w0)) / 2.0;
	
	float a0 =  1 + alpha;
	float a1 = -2 * cos(w0);
	float a2 =  1 - alpha;

	b0 /= a0;
	b1 /= a0;
	b2 /= a0;
	a1 /= a0;
	a2 /= a0;

	s.a[0] = b0;
	s.a[1] = b1;
	s.a[2] = b2;
	s.a[3] = a1;
	s.a[4] = a2;
}

/* rbj */
DE void  __fastcall DSP_BW_InitHighpass(CBWState &s, float const f)
{
	float w0 = 2.0 * M_PI * f / (float)sampleRate;
	float alpha = sin(w0) / (2 * 1.0 / sqrt(2.0));

	float b0 =  (1 + cos(w0)) / 2.0;
	float b1 = -(1 + cos(w0));
	float b2 =  (1 + cos(w0)) / 2.0;
	
	float a0 =  1 + alpha;
	float a1 = -2 * cos(w0);
	float a2 =  1 - alpha;

	b0 /= a0;
	b1 /= a0;
	b2 /= a0;
	a1 /= a0;
	a2 /= a0;

	s.a[0] = b0;
	s.a[1] = b1;
	s.a[2] = b2;
	s.a[3] = a1;
	s.a[4] = a2;
}

/* hacked by calvin */
DE void  __fastcall DSP_BW_InitBandpass(CBWState &s, float const f, float const bw)
{
	float a = tan((bw * M_PI) / sampleRate);
	float b = 1.0f / a;
	float c = f * M_PI;
	float d = cos((c + c) / sampleRate);
	s.a[0] = 1.0f / (b + 1.0f);
	s.a[1] = 0.0f;
	s.a[2] = -s.a[0];
	s.a[3] = -((d + d) * s.a[0] * b);
	s.a[4] = (b - 1.0f) * s.a[0];

}

/* hacked by calvin */
DE void  __fastcall DSP_BW_InitBandreject(CBWState &s, float const f, float const bw)
{
	float a = tan((bw * M_PI) / sampleRate);
	float b = f * M_PI;
	float c = cos((b + b) / sampleRate);
	s.a[0] = 1.0 / (a + 1.0f);
	s.a[1] = -(c + c) * s.a[0];
	s.a[2] = s.a[0];
	s.a[3] = s.a[1];
	s.a[4] = (1 - a) * s.a[0];
}

DE bool  __fastcall DSP_BW_Work(CBWState &s, float *ps, dword const n, int const mode)
{
	int i;
	float y;

	for(i = 0; i < n; i++) {
		float in;

		if(mode & WM_READ)
			in = *ps;
		else
			in = 0;
	
		if(fabs(in) > QUIET) {
			s.IdleCount = 0;
		} else {
			if(s.IdleCount >= BW_SETTLE_TIME) {
				if(mode & WM_WRITE) {
					*ps = 0;
					ps++;
					continue;
				}
			} else {
				s.IdleCount++;
			}
		}

		y = in * s.a[0] + s.i[0] * s.a[1] + s.i[1] * s.a[2] -
			s.o[0] * s.a[3] - s.o[1] * s.a[4];
		s.i[1] = s.i[0];
		s.i[0] = in;
		s.o[1] = s.o[0];
		s.o[0] = y;

		if(mode & WM_WRITE)
			*ps = y;

		ps++;
	}
    return TRUE;
}

DE bool  __fastcall DSP_BW_WorkStereo(CBWState &s, float *ps, dword const n, int const mode)
{
	int i;
	float y, yr;

	for(i = 0; i < n; i++) {
		float in, inr;

		if(mode & WM_READ) {
			in = *ps;
			inr = *(ps+1);
		} else {
			in = 0;
			inr= 0;
		}
	
		if((fabs(in) > QUIET) || (fabs(inr) > QUIET)) {
			s.IdleCount = 0;
		} else {
			if(s.IdleCount >= BW_SETTLE_TIME) {
				if(mode & WM_WRITE) {
					*ps = 0;
					*(ps+1) = 0;
					ps+=2;
					continue;
				}
			} else {
				s.IdleCount++;
			}
		}

		y = in * s.a[0] + s.i[0] * s.a[1] + s.i[1] * s.a[2] -
			s.o[0] * s.a[3] - s.o[1] * s.a[4];
		s.i[1] = s.i[0];
		s.i[0] = in;
		s.o[1] = s.o[0];
		s.o[0] = y;
		
		yr = inr * s.a[0] + s.ri[0] * s.a[1] + s.ri[1] * s.a[2] -
			s.ro[0] * s.a[3] - s.ro[1] * s.a[4];
		s.ri[1] = s.ri[0];
		s.ri[0] = inr;
		s.ro[1] = s.ro[0];
		s.ro[0] = yr;

		if(mode & WM_WRITE) {
			*ps = y;
			*(ps+1) = yr;
		}

		ps+=2;
	}
    return TRUE;
}

// resampler

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

DE void  __fastcall DSP_Resample(float *pout, int numsamples, CResamplerState &state, CResamplerParams const &params)
{
    // printf("STUB: DSP_Resample\n");
}

}

#if 0
int main(void)
{
	CBWState s;
	float samples[500];
	int i;

	memset(samples, 0, sizeof(float)*500);
	samples[0] = 1;
	
	DSP_Init(44100);
	DSP_BW_InitBandreject(s, 500, 500);
	DSP_BW_Reset(s);
	DSP_BW_WorkStereo(s, samples, 100, WM_READWRITE);

	for(i = 0; i < 20; i++)
		printf("%.10f\n", samples[i]);
}
#endif

extern "C" {

DE BOOL WINAPI DllMain(HINSTANCE, DWORD dwReason, LPVOID lpReserved) { 
  return 1;
}
};
