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
#define WM_NOIO                                 0
#define WM_READ                                 1
#define WM_WRITE                                2
#define WM_READWRITE							3

#define BW_SETTLE_TIME		256		

#endif
