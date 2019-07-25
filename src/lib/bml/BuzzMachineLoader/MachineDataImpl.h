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
#include <string.h>
#include <stdint.h>

typedef uint8_t byte;
typedef uint16_t word;
typedef uint32_t dword;

class CMachineDataInput
{
public:
  //virtual ~CMachineDataInput() {}
	virtual void Read(void *pbuf, int const numbytes);

	void Read(int &d) { Read(&d, sizeof(int)); }
	void Read(dword &d) { Read(&d, sizeof(dword)); }
	void Read(short &d) { Read(&d, sizeof(short)); }
	void Read(word &d) { Read(&d, sizeof(word)); }
	void Read(char &d) { Read(&d, sizeof(char)); }
	void Read(byte &d) { Read(&d, sizeof(byte)); }
	void Read(float &d) { Read(&d, sizeof(float)); }
	void Read(double &d) { Read(&d, sizeof(double)); }
	void Read(bool &d) { Read(&d, sizeof(bool)); }

};

class CMachineDataOutput
{
public:
  //virtual ~CMachineDataOutput() {}
	virtual void Write(void *pbuf, int const numbytes);
	void Write(int d) { Write(&d, sizeof(int)); }
	void Write(dword d) { Write(&d, sizeof(dword)); }
	void Write(short d) { Write(&d, sizeof(short)); }
	void Write(word d) { Write(&d, sizeof(word)); }
	void Write(char d) { Write(&d, sizeof(char)); }
	void Write(byte d) { Write(&d, sizeof(byte)); }
	void Write(float d) { Write(&d, sizeof(float)); }
	void Write(double d) { Write(&d, sizeof(double)); }
	void Write(bool d) { Write(&d, sizeof(bool)); }
	void Write(char const *str) { Write((void *)str, (int)(strlen(str)+1)); }

};

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
