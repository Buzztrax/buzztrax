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
#ifndef MACHINE_DATA_IMPL_H
#define MACHINE_DATA_IMPL_H

#ifdef WIN32
#include "stdafx.h"
#include <windows.h>
#else
#include "windef.h"
#endif
#include <stdio.h>
#include "MachineInterface.h"

class CMachineDataInputImpl : public CMachineDataInput
{
public:
	CMachineDataInputImpl(HANDLE hFile);
	CMachineDataInputImpl(BYTE * pbyBuffer, DWORD dwBufferLen);
	virtual ~CMachineDataInputImpl();
	
	virtual void Read(void* pmem, int const n_size);
	
private:
	const HANDLE m_hFile;
	const BYTE * m_pbyBuffer;
	DWORD m_dwBufferLen;
};

class CMachineDataOutputImpl : public CMachineDataOutput
{
public:
	CMachineDataOutputImpl(HANDLE hFile);
	CMachineDataOutputImpl(void);
	DWORD GetCount() const;
	const BYTE * GetOutputBuffer() const;
	virtual ~CMachineDataOutputImpl();
	
	virtual void Write(void *pmem, int const n_size);
private:
	HANDLE m_hFile;
	BYTE * m_pbyBuffer;
	DWORD m_dwBufferLen;
};


#endif // BUZZ_MACHINE_DATA_H
