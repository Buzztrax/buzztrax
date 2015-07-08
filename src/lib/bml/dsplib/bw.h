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
#ifndef __BUZZ_DSPLIB_BW_H
#define __BUZZ_DSPLIB_BW_H

class CBWState
{
public:
	float a[5];		// coefficients
	float i[2];		// past inputs
	float o[2];		// past outputs
	float ri[2];	// past right inputs (for stereo mode)
	float ro[2];	// past right outputs 
	int IdleCount;
};

// work modes
#define WM_NOIO           0
#define WM_READ           1
#define WM_WRITE          2
#define WM_READWRITE      3

#define BW_SETTLE_TIME    256		

#endif
