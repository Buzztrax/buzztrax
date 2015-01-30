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

#include "MachineDataImpl.h"
#include "debug.h"

// virtual 'null' base implementation of CMachineDataInput and CMachineDataOutput
void CMachineDataInput::Read(void * pmem, const int n_size) {
	// clear ...
    DBG2("(pbuf=%p,numbytes=%d) : no buffer\n",pmem,n_size);
	memset(pmem, 0, n_size);
}
void CMachineDataOutput::Write(void * pmem, const int n_size) {
	// do nothing ...
    DBG2("(pbuf=%p,numbytes=%d) : no buffer\n",pmem,n_size);
}


// Implementation of Read/Write classes for internal machine data load/save operations

CMachineDataInputImpl::CMachineDataInputImpl(HANDLE hFile) : m_hFile(hFile), m_pbyBuffer(NULL), m_dwBufferLen(0) {};
CMachineDataInputImpl::CMachineDataInputImpl(BYTE * pbyBuffer, DWORD dwBufferLen) : m_hFile(0), m_pbyBuffer(pbyBuffer), m_dwBufferLen(dwBufferLen) {};
CMachineDataInputImpl::~CMachineDataInputImpl() {
	//
}
	
void CMachineDataInputImpl::Read(void* pmem, const int n_size) {
	if (m_pbyBuffer) {
		if (m_dwBufferLen >= (DWORD)n_size) {
			memcpy(pmem, m_pbyBuffer, n_size);
			m_pbyBuffer += n_size;
			m_dwBufferLen -= n_size;
		}
		else {
			DBG2("(pbuf=%p,numbytes=%d) : out of buffer\n",pmem,n_size);
			memcpy(pmem, m_pbyBuffer, m_dwBufferLen);
			m_pbyBuffer += m_dwBufferLen;
			m_dwBufferLen = 0;
		}
	}
	else if (m_hFile) {
		// clear ...
		DBG2("(pbuf=%p,numbytes=%d) : no file\n",pmem,n_size);
		memset(pmem, 0, n_size);
	}
	else {
		// clear ...
		DBG2("(pbuf=%p,numbytes=%d) : no buffer\n",pmem,n_size);
		memset(pmem, 0, n_size);
	}
}
	


CMachineDataOutputImpl::CMachineDataOutputImpl(HANDLE hFile) : m_hFile(hFile), m_pbyBuffer(NULL), m_dwBufferLen(0) {};
CMachineDataOutputImpl::CMachineDataOutputImpl(void) : m_hFile(0), m_pbyBuffer(NULL), m_dwBufferLen(0) {};

DWORD CMachineDataOutputImpl::GetCount() const {
	return m_dwBufferLen;
}

const BYTE * CMachineDataOutputImpl::GetOutputBuffer() const {
	return m_pbyBuffer;
}

CMachineDataOutputImpl::~CMachineDataOutputImpl() {
}

void CMachineDataOutputImpl::Write(void *pmem, int const n_size) {
	if (m_pbyBuffer) {
		DBG2("(pbuf=%p,numbytes=%d) : no file\n",pmem,n_size);
	}
	else if (m_hFile) {
		DBG2("(pbuf=%p,numbytes=%d) : no file\n",pmem,n_size);
	}
	else {
		DBG2("(pbuf=%p,numbytes=%d) : no buffer\n",pmem,n_size);
	}
}
