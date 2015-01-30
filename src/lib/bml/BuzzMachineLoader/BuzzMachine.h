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

#ifndef BUZZ_MACHINE_H
#define BUZZ_MACHINE_H

#ifdef __cplusplus
extern "C" {
#endif

// our high level instance handle
#ifdef BUZZ_MACHINE_LOADER
class BuzzMachineCallbacks;

// @idea what about deriving this from CMICallbacks
class BuzzMachine {
public:
	// library handle
	HMODULE h;
	// callback instance
	BuzzMachineCallbacks *callbacks;
	// classes
	CMachineInfo *machine_info;
	CMachineInterface *machine;
	//callbacks->machine_ex;
	//CMachineInterfaceEx *machine_ex;
};
#else
typedef void BuzzMachine;
#endif

#ifdef __cplusplus
}
#endif


#endif // BUZZ_MACHINE_H
