/* BuzzDecomp by Mark Collier
** http://www.marcnetsystem.co.uk/
** Some stuff added by Ed Powley (ejp), http://btdsys.cjb.net
*/

#include <windows.h>
#include <stdio.h>

//=====================================DEFINITIONS================================
#define MAXPACKEDBUFFER 2048

//=====================================STRUCTURES================================

typedef struct _ENVPOINT
{
	WORD	wX;
	WORD	wY;
	BYTE	btFlags;
}ENVPOINT;

typedef struct _ENVELOPE
{
	WORD	wAttackTime;
	WORD	wDecayTime;
	WORD	wSustainLevel;
	WORD	wReleaseTime;
	BYTE	btADSRSubdivide;
	BYTE	btADSRFlags;
		
	WORD wNumPoints;
	ENVPOINT	* lpEnvPoints;

}ENVELOPE;

typedef struct _LEVEL
{
	DWORD dwNumSamples;
	DWORD dwLoopBegin;
	DWORD dwLoopEnd;
	DWORD dwSampsPerSecond;
	BYTE btRootNote;

	LPWORD lpwWaveData;

}LEVEL;

typedef struct _WAVT_ENTRY
{
	WORD	wIndex;
	char	acFullName[255];
	char	acName[255];
	float	volume;
	BYTE	btFlags;

	
	WORD	wNumEnvelopes;
	ENVELOPE * lpEnvelopes;
	
	BYTE	btNumLevels;
	LEVEL	* lpLevels;

}WAVT_ENTRY;

typedef struct _COMPRESSIONVALUES
{
	WORD	wSum1;
	WORD	wSum2;
	WORD	wResult;

	LPWORD	lpwTempData;
}COMPRESSIONVALUES;

typedef struct _WAVEUNPACK
{
	HANDLE hFile;
	BYTE abtPackedBuffer[MAXPACKEDBUFFER];
	DWORD dwCurIndex;
	DWORD dwCurBit;

	DWORD dwBytesInBuffer;
	DWORD dwMaxBytes;
	DWORD dwBytesInFileRemain;

}WAVEUNPACK;

//=====================================UTILITY CODE====================================
void TidyEntries(WAVT_ENTRY * entries,DWORD dwAmount)
{
	DWORD i,j;

	if(entries != NULL)
	{
		for(i=0;i<dwAmount;i++)
		{
			//free envelope data
			if (entries[i].lpEnvelopes != NULL)
			{
				for(j=0;j<entries[i].wNumEnvelopes;j++)
				{
					//free envelope point data 
					if (entries[i].lpEnvelopes[j].lpEnvPoints != NULL)
					{				
						LocalFree(entries[i].lpEnvelopes[j].lpEnvPoints);
					}
				}

				LocalFree(entries[i].lpEnvelopes);
			}
			

			//free level data
			if (entries[i].lpLevels != NULL)
			{
				//free wave data inside each level
				for(j=0;j<entries[i].btNumLevels;j++)
				{
					if (entries[i].lpLevels[j].lpwWaveData != NULL)
					{
						LocalFree(entries[i].lpLevels[j].lpwWaveData);
					}
				}
				LocalFree(entries[i].lpLevels);
			}
		}			

		LocalFree(entries);
	}
}

//==================================BIT UNPACKING===================================
BOOL InitWaveUnpack(WAVEUNPACK * waveunpackinfo,HANDLE hFile,DWORD dwSectionSize)
{
	waveunpackinfo->dwMaxBytes = MAXPACKEDBUFFER;
	waveunpackinfo->dwBytesInFileRemain = dwSectionSize ;
	waveunpackinfo->hFile = hFile;
	waveunpackinfo->dwCurBit = 0;

	//set up so that call to UnpackBits() will force an immediate read from file
	waveunpackinfo->dwCurIndex = MAXPACKEDBUFFER;
	waveunpackinfo->dwBytesInBuffer = 0;

	return TRUE;
}

DWORD UnpackBits(WAVEUNPACK * unpackinfo,DWORD dwAmount)
{	
	DWORD dwRet,dwReadAmount,dwSize,dwMask,dwVal;
	DWORD dwFileReadAmnt,dwReadFile,dwShift;
	DWORD dwMax = 8;

	if((unpackinfo->dwBytesInFileRemain == 0) && (unpackinfo->dwCurIndex == MAXPACKEDBUFFER))
	{
		return 0;
	}
	
	dwReadAmount = dwAmount;
	dwRet = 0;
	dwShift = 0;
	while(dwReadAmount > 0)
	{
		//check to see if we need to update buffer and/or index
		if((unpackinfo->dwCurBit == dwMax) || (unpackinfo->dwBytesInBuffer == 0))
		{	
			unpackinfo->dwCurBit = 0;
			unpackinfo->dwCurIndex++;
			if(unpackinfo->dwCurIndex >= unpackinfo->dwBytesInBuffer )
			{	//run out of buffer... read more file into buffer
				dwFileReadAmnt= (unpackinfo->dwBytesInFileRemain > unpackinfo->dwMaxBytes ) ? unpackinfo->dwMaxBytes : unpackinfo->dwBytesInFileRemain;
				
				ReadFile(unpackinfo->hFile,unpackinfo->abtPackedBuffer,
						 dwFileReadAmnt,&dwReadFile,NULL);

				unpackinfo->dwBytesInFileRemain -= dwReadFile;	
				unpackinfo->dwBytesInBuffer = dwReadFile;
				unpackinfo->dwCurIndex = 0;

				//if we didnt read anything then exit now
				if(dwReadFile == 0)
				{	//make sure nothing else is read
					unpackinfo->dwBytesInFileRemain = 0;
					unpackinfo->dwCurIndex = MAXPACKEDBUFFER;
					return 0;
				}
			}
		}
		
		//calculate size to read from current dword
		dwSize = ((dwReadAmount + unpackinfo->dwCurBit) > dwMax) ? dwMax - unpackinfo->dwCurBit : dwReadAmount;
		
		//calculate bitmask
		dwMask = (1 << dwSize) - 1;

		//Read value from buffer
		dwVal = unpackinfo->abtPackedBuffer[unpackinfo->dwCurIndex];
		dwVal = dwVal >> unpackinfo->dwCurBit;

		//apply mask to value
		dwVal &= dwMask;

		//shift value to correct position
		dwVal = dwVal << dwShift;
		
		//update return value
		dwRet |= dwVal;

		//update info
		unpackinfo->dwCurBit += dwSize;
		dwShift += dwSize;
		dwReadAmount -= dwSize;
	}

	return dwRet;
}

DWORD CountZeroBits(WAVEUNPACK * unpackinfo)
{
	DWORD dwBit;
	DWORD dwCount = 0;

	dwBit = UnpackBits(unpackinfo,1);
	while(dwBit == 0)
	{
		dwCount++;
		dwBit = UnpackBits(unpackinfo,1);
	}

	return dwCount;
}

void AdjustFilePointer(WAVEUNPACK * unpackinfo)
{
	int iRemain = unpackinfo->dwCurIndex - unpackinfo->dwBytesInBuffer;			
	iRemain++;
	SetFilePointer(unpackinfo->hFile,iRemain,NULL,FILE_CURRENT);
}

//==================================WAVE DECOMPRESSING===================================
void ZeroCompressionValues(COMPRESSIONVALUES * lpcv,DWORD dwBlockSize)
{
	lpcv->wResult = 0;
	lpcv->wSum1 = 0;
	lpcv->wSum2 = 0;

	//If block size is given, then allocate specfied temporary data
	if (dwBlockSize > 0)
	{
		lpcv->lpwTempData = (LPWORD)LocalAlloc(LPTR,dwBlockSize * sizeof(WORD));
	}
	else
	{
		lpcv->lpwTempData=NULL;
	}
}

void TidyCompressionValues(COMPRESSIONVALUES * lpcv)
{
	//if there is temporary data - then free it.
	if (lpcv->lpwTempData != NULL)
	{
		LocalFree(lpcv->lpwTempData);
	}
}


BOOL DecompressSwitch(WAVEUNPACK * unpackinfo,COMPRESSIONVALUES * lpcv,
					  LPWORD lpwOutputBuffer,DWORD dwBlockSize)
{
	DWORD dwSwitchValue,dwBits,dwSize,dwZeroCount;
	WORD wValue;
	LPWORD lpwaddress;
	if(dwBlockSize == 0)
	{
		return FALSE;
	}

	//Get compression method
	dwSwitchValue = UnpackBits(unpackinfo,2);

	//read size (in bits) of compressed values
	dwBits = UnpackBits(unpackinfo,4);

	dwSize = dwBlockSize;
	lpwaddress = lpwOutputBuffer;
	while(dwSize > 0)
	{
		//read compressed value
		// ejp: cast added to suppress compiler warning
		wValue = (WORD)UnpackBits(unpackinfo,dwBits);
		
		//count zeros
		dwZeroCount = CountZeroBits(unpackinfo);
		
		//Construct
		// ejp: cast added to suppress compiler warning
		wValue = (WORD)((dwZeroCount << dwBits) | wValue);

		//is value supposed to be positive or negative?
		if((wValue & 1) == 0)
		{	//its positive
			wValue = wValue >> 1;
		}
		else
		{	//its negative. Convert into a negative value.
			wValue++;
			wValue = wValue >> 1;
			wValue = ~wValue; //invert bits
			wValue++; //add one to make 2's compliment
		}

		//Now do stuff depending on which method we're using....
		switch(dwSwitchValue )
		{
			case 0:
				lpcv->wSum2 = ((wValue - lpcv->wResult) - lpcv->wSum1);
				lpcv->wSum1 = wValue - lpcv->wResult;
				lpcv->wResult = wValue;
				break;
			case 1:
				lpcv->wSum2 = wValue - lpcv->wSum1;
				lpcv->wSum1 = wValue;
				lpcv->wResult += wValue;
				break;
			case 2:
				lpcv->wSum2 = wValue;
				lpcv->wSum1 += wValue;
				lpcv->wResult += lpcv->wSum1;
				break;
			case 3:
				lpcv->wSum2 += wValue;
				lpcv->wSum1 += lpcv->wSum2;
				lpcv->wResult += lpcv->wSum1;
				break;
			default: //error
				return FALSE;
		}

		//store value into output buffer
		*lpwOutputBuffer = lpcv->wResult;
		
		//prepare for next loop...
		lpwOutputBuffer++;
		dwSize--;
	}

	return TRUE;
}


BOOL DecompressWave(WAVEUNPACK * unpackinfo,LPWORD lpwOutputBuffer,
					  DWORD dwNumSamples,BOOL bStereo)
{
	DWORD dwZeroCount,dwShift,dwBlockSize,dwBlockCount,dwLastBlockSize;
	DWORD dwResultShift,dwCount,i,ixx;
	BYTE btSumChannels;
	COMPRESSIONVALUES cv1,cv2;

	if(lpwOutputBuffer == NULL)
	{
		return FALSE;
	}

	dwZeroCount = CountZeroBits(unpackinfo);
	if (dwZeroCount != 0)
	{
		//printf("Unknown compressed wave data format \n");
		return FALSE;
	}

	//get size shifter
	dwShift = UnpackBits(unpackinfo,4);

	//get size of compressed blocks
	dwBlockSize = 1 << dwShift;

	//get number of compressed blocks
	dwBlockCount = dwNumSamples >> dwShift;

	//get size of last compressed block
	dwLastBlockSize = (dwBlockSize - 1) & dwNumSamples;

	//get result shifter value (used to shift data after decompression)
	dwResultShift = UnpackBits(unpackinfo,4);		

	if(!bStereo)
	{	//MONO HANDLING

		//zero internal compression values
		ZeroCompressionValues(&cv1,0);

		//If there's a remainder... then handle number of blocks + 1
		dwCount = (dwLastBlockSize == 0) ? dwBlockCount : dwBlockCount +1;
		while(dwCount > 0)
		{
			if (!DecompressSwitch(unpackinfo,&cv1,lpwOutputBuffer,dwBlockSize))
			{
				return FALSE;
			}

			for(i=0;i<dwBlockSize;i++)
			{	//shift end result
				lpwOutputBuffer[i] = lpwOutputBuffer[i] << dwResultShift;
			}
			
			//proceed to next block...
			lpwOutputBuffer += dwBlockSize;
			dwCount--;

			//check to see if we are handling the last block
			if((dwCount == 1) && (dwLastBlockSize != 0))
			{	//we are... set block size to size of last block
				dwBlockSize = dwLastBlockSize;
			}
		}		
	}
	else
	{	//STEREO HANDLING

		//Read "channel sum" flag
		// ejp: cast added to suppress compiler warning
		btSumChannels = (BYTE)UnpackBits(unpackinfo,1);
		
		//zero internal compression values and alloc some temporary space
		ZeroCompressionValues(&cv1,dwBlockSize);
		ZeroCompressionValues(&cv2,dwBlockSize);

		//If there's a remainder... then handle number of blocks + 1
		dwCount = (dwLastBlockSize == 0) ? dwBlockCount : dwBlockCount +1;
		while(dwCount > 0)
		{
			//decompress both channels into temporary area
			if(!DecompressSwitch(unpackinfo,&cv1,cv1.lpwTempData,dwBlockSize))
			{
				return FALSE;
			}

			if (!DecompressSwitch(unpackinfo,&cv2,cv2.lpwTempData,dwBlockSize))
			{
				return FALSE;
			}
			
			for(i=0;i<dwBlockSize;i++)
			{	
				//store channel 1 and apply result shift
				ixx = i * 2;
				lpwOutputBuffer[ixx] = cv1.lpwTempData[i] << dwResultShift;
				
				//store channel 2
				ixx++;
				lpwOutputBuffer[ixx] = cv2.lpwTempData[i];
				
				//if btSumChannels flag is set then the second channel is
				//the sum of both channels
				if(btSumChannels != 0)
				{
					lpwOutputBuffer[ixx] += cv1.lpwTempData[i];
				}
				
				//apply result shift to channel 2
				lpwOutputBuffer[ixx] = lpwOutputBuffer[ixx] << dwResultShift;

			}

			//proceed to next block
			lpwOutputBuffer += dwBlockSize * 2;
			dwCount--;

			//check to see if we are handling the last block
			if((dwCount == 1) && (dwLastBlockSize != 0))
			{	//we are... set block size to size of last block
				dwBlockSize = dwLastBlockSize;
			}

		}
		
		//tidy
		TidyCompressionValues(&cv1);
		TidyCompressionValues(&cv2);
	}

	return TRUE;
}


//=====================================FILE PARSING===================================
BOOL ReadNullTermString(HANDLE hFile,LPSTR lpstrBuffer)
{
	char character;
	DWORD dwRead;
	
	while(TRUE)
	{
		if(!ReadFile(hFile,&character,1,&dwRead,NULL)) 
		{
			return FALSE;
		}

		if (character == 0)
		{
			return TRUE;
		}
		
		*lpstrBuffer = character;
		lpstrBuffer++;
	}

	return FALSE;
}


BOOL ParseHeader(HANDLE hFile,DWORD * lpdwWAVTpos, DWORD * lpdwCWAVpos,DWORD * lpdwCWAVsize)
{
	char acSectionName[5];
	DWORD dwCount;
	DWORD dwRead;
	DWORD dwOffset;
	DWORD dwSize;
	DWORD i;
	
	//start from begining of file
	SetFilePointer(hFile,0,NULL,FILE_BEGIN);

	//read "Buzz" header
	acSectionName[4] = 0;
	if( (!ReadFile(hFile,acSectionName,4,&dwRead,NULL)) || 
		 (strcmp(acSectionName,"Buzz") != 0) )
	{
		if(strcmp(acSectionName,"RawW") == 0)
		{
			//its a raw file... make up some values
			*lpdwWAVTpos = 0xffffffff;
			*lpdwCWAVpos = 8;
			*lpdwCWAVsize = GetFileSize(hFile,NULL) - 4;
		
			return TRUE;

		}
		return FALSE;
	}

	//read number of sections
	if( (!ReadFile(hFile,&dwCount,4,&dwRead,NULL)) || (dwCount < 1) || (dwCount > 31) )
	{
		return FALSE;
	}

	//read and parse each entry
	for(i=0;i<dwCount;i++)
	{
		if( (!ReadFile(hFile,acSectionName,4,&dwRead,NULL)) ||
			(!ReadFile(hFile,&dwOffset,4,&dwRead,NULL)) ||
			(!ReadFile(hFile,&dwSize,4,&dwRead,NULL)) )
		{
			return FALSE;
		}	
		
		switch( *(LPDWORD)acSectionName)
		{
			case 'HCAM':	//MACH
			case 'REVB':	//BVER
			case 'ARAP':	//PARA
			case 'NNOC':	//CONN
			case 'UQES':	//SEQU
			case 'HALB':	//BLAH
			case 'GLDP':	//PDLG
			case 'IDIM':	//MIDI
			case 'TTAP':	//PATT
				break;
			case 'TVAW':	// WAVT - wave table 
				*lpdwWAVTpos = dwOffset;//give offset to caller
				break;
			case 'EVAW':	// WAVE - uncompressed wave data
			case 'VAWC':	// CWAV -compressed wave
				*lpdwCWAVpos = dwOffset;//give offset to caller
				*lpdwCWAVsize = dwSize; //give size to caller
				break;
			default: //unknown header
				return FALSE;

		}
	}

	return TRUE;
}

ENVPOINT * ParseEnvelopePoints(HANDLE hFile,DWORD dwAmount)
{
	ENVPOINT * points;
	DWORD i,dwRead;

	points = (ENVPOINT *)LocalAlloc(LPTR,dwAmount * sizeof(ENVPOINT));
	if(points == NULL)
	{
		return NULL;
	}
	memset(points,0,dwAmount * sizeof(ENVPOINT));

	//for each point
	for(i=0;i<dwAmount;i++)
	{
		//read X value
		if(!ReadFile(hFile,&points[i].wX,2,&dwRead,NULL)) 
		{
			return NULL;
		}

		//read Y value
		if(!ReadFile(hFile,&points[i].wY,2,&dwRead,NULL)) 
		{
			return NULL;
		}

		//read flags
		if(!ReadFile(hFile,&points[i].btFlags,1,&dwRead,NULL)) 
		{
			return NULL;
		}
	}

	return points;
}

ENVELOPE * ParseEnvelopeData(HANDLE hFile,DWORD dwAmount)
{
	ENVELOPE * envelopes = NULL;
	DWORD i,dwRead;
	
	envelopes = (ENVELOPE *)LocalAlloc(LPTR,dwAmount * sizeof(ENVELOPE));
	if(envelopes == NULL)
	{
		return NULL;
	}
	memset(envelopes,0,dwAmount * sizeof(ENVELOPE));

	//for each wave
	for(i=0;i<dwAmount;i++)
	{
		//read attack time
		if(!ReadFile(hFile,&envelopes[i].wAttackTime,2,&dwRead,NULL)) 
		{
			return NULL;
		}

		//read decay time
		if(!ReadFile(hFile,&envelopes[i].wDecayTime,2,&dwRead,NULL)) 
		{
			return NULL;
		}

		//read sustain level
		if(!ReadFile(hFile,&envelopes[i].wSustainLevel,2,&dwRead,NULL)) 
		{
			return NULL;
		}

		//read release time
		if(!ReadFile(hFile,&envelopes[i].wReleaseTime,2,&dwRead,NULL)) 
		{
			return NULL;
		}

		//read ADSR subdivide
		if(!ReadFile(hFile,&envelopes[i].btADSRSubdivide,1,&dwRead,NULL)) 
		{
			return NULL;
		}

		//read ADSR flags
		if(!ReadFile(hFile,&envelopes[i].btADSRFlags,1,&dwRead,NULL)) 
		{
			return NULL;
		}

		//read number of points
		if(!ReadFile(hFile,&envelopes[i].wNumPoints,2,&dwRead,NULL)) 
		{
			return NULL;
		}

		if ((envelopes[i].wNumPoints & 0x7fff) != 0)
		{	//parse envelope points
			envelopes[i].lpEnvPoints = ParseEnvelopePoints(hFile,(envelopes[i].wNumPoints & 0x7fff));
			if (envelopes[i].lpEnvPoints == NULL)
			{
				return NULL;
			}
		}
		else
		{	//no envelope point data
			envelopes[i].lpEnvPoints =NULL;
			envelopes[i].wNumPoints = 0;
		}
	}


	return envelopes;
}

LEVEL * ParseLevelData(HANDLE hFile,DWORD dwAmount)
{
	LEVEL * levels;
	DWORD i,dwRead;

	levels= (LEVEL *)LocalAlloc(LPTR,sizeof(LEVEL) * dwAmount);
	if(levels==NULL)
	{
		return NULL;
	}
	memset(levels,0,sizeof(LEVEL) * dwAmount);

	//for each level..
	for(i=0;i<dwAmount;i++)
	{
		//read number of samples
		if(!ReadFile(hFile,&levels[i].dwNumSamples,4,&dwRead,NULL)) 
		{
			return NULL;
		}

		//read loop begin
		if(!ReadFile(hFile,&levels[i].dwLoopBegin,4,&dwRead,NULL)) 
		{
			return NULL;
		}

		//read loop end
		if(!ReadFile(hFile,&levels[i].dwLoopEnd,4,&dwRead,NULL)) 
		{
			return NULL;
		}

		//read samples per second
		if(!ReadFile(hFile,&levels[i].dwSampsPerSecond,4,&dwRead,NULL)) 
		{
			return NULL;
		}

		//read root note
		if(!ReadFile(hFile,&levels[i].btRootNote,1,&dwRead,NULL)) 
		{
			return NULL;
		}
	}
	
	return levels;
}


WAVT_ENTRY * ParseWAVTSection(HANDLE hFile,DWORD dwSectionPos,DWORD * lpdwCount)
{
	DWORD dwRead,i;
	DWORD dwCount = 0;
	WAVT_ENTRY * entries;

	if(dwSectionPos == 0xffffffff)
	{	//raw file mode... make up some values
		
		dwCount  = 1;
		*lpdwCount = 1;
		entries = (WAVT_ENTRY *)LocalAlloc(LPTR,dwCount * sizeof(WAVT_ENTRY));
		sprintf(entries->acFullName,"c:\\files\\rawfile.raw");
		sprintf(entries->acName,"rawcompressedfile");
		entries->btFlags = 0x08;
		entries->btNumLevels = 1;
		entries->lpEnvelopes = NULL;
		entries->lpLevels =(LEVEL *)LocalAlloc(LPTR,sizeof(LEVEL));
		entries->volume = 1.0;
		entries->wIndex = 0;
		entries->wNumEnvelopes = 0;

		//read number of samples
		if(!ReadFile(hFile,&entries->lpLevels[0].dwNumSamples,4,&dwRead,NULL)) 
		{
			return NULL;
		}

		//and make up level data....
		entries->lpLevels[0].btRootNote = 0;
		entries->lpLevels[0].dwLoopBegin = 0;
		entries->lpLevels[0].dwLoopEnd = entries->lpLevels[0].dwNumSamples;
		entries->lpLevels[0].dwSampsPerSecond = 44100;
		entries->lpLevels[0].lpwWaveData = NULL;

		return entries;
	}
	
	//make sure we start reading from the start of the WAVT section of the file
	SetFilePointer(hFile,dwSectionPos,NULL,FILE_BEGIN);

	//how many wave table entries are there?
	if( (!ReadFile(hFile,&dwCount,2,&dwRead,NULL)) || (dwCount < 1) )
	{
		return NULL;
	}

	//give number of waves to caller
	*lpdwCount = dwCount;

	//alloc memory for all entries
	entries = (WAVT_ENTRY *)LocalAlloc(LPTR,dwCount * sizeof(WAVT_ENTRY));
	if (entries == NULL)
	{
		return NULL;
	}
	memset(entries,0,dwCount * sizeof(WAVT_ENTRY));
	
	//for each wave....
	for(i=0;i<dwCount;i++)
	{
		//read index
		if(!ReadFile(hFile,&entries[i].wIndex,2,&dwRead,NULL)) 
		{
			TidyEntries(entries,i);
			return NULL;
		}
		
		//read full name
		if(!ReadNullTermString(hFile,entries[i].acFullName))
		{
			TidyEntries(entries,i);
			return NULL;
		}

		//read name
		if(!ReadNullTermString(hFile,entries[i].acName))
		{
			TidyEntries(entries,i);
			return NULL;
		}

		//read volume
		if(!ReadFile(hFile,&entries[i].volume,sizeof(float),&dwRead,NULL)) 
		{
			TidyEntries(entries,i);
			return NULL;
		}

		//read flags
		if(!ReadFile(hFile,&entries[i].btFlags,1,&dwRead,NULL)) 
		{
			TidyEntries(entries,i);
			return NULL;
		}

		//if we have envelope data, then process that now
		if((entries[i].btFlags & 0x80) != 0)
		{
			//get number of envelopes
			if(!ReadFile(hFile,&entries[i].wNumEnvelopes,2,&dwRead,NULL)) 
			{
				TidyEntries(entries,i);
				return NULL;
			}

			//parse envelope data
			entries[i].lpEnvelopes = ParseEnvelopeData(hFile,entries[i].wNumEnvelopes);
			if (entries[i].lpEnvelopes == NULL)
			{
				TidyEntries(entries,i);
				return NULL;
			}
		}
		else
		{	//no envelope data
			entries[i].wNumEnvelopes=0;
			entries[i].lpEnvelopes = NULL;
		}

		//read number of levels
		if( (!ReadFile(hFile,&entries[i].btNumLevels,1,&dwRead,NULL)) ||
			(entries[i].btNumLevels == 0))
		{
			TidyEntries(entries,i);
			return NULL;
		}
	
		//parse level data
		entries[i].lpLevels = ParseLevelData(hFile,entries[i].btNumLevels);
		if(entries[i].lpLevels == NULL)
		{
			TidyEntries(entries,i);
			return NULL;
		}
	}

	return entries;
}


BOOL ParseWaveData(HANDLE hFile,DWORD dwSectionPos,DWORD dwSectionSize,
				   WAVT_ENTRY * entries, char *dirname)
{
	DWORD i,j,dwRead,dwBytesInLevels,dwWaveMemSize,dwWrit;
	WORD wCount,wIndex;
	BYTE btFormat;
	char acName[255];
	HANDLE hnd;
	WAVEUNPACK unpackinfo;

	//make sure we start reading from the start of the CWAV section of the file
	SetFilePointer(hFile,dwSectionPos,NULL,FILE_BEGIN);

	//read wave count
	if(!ReadFile(hFile,&wCount,2,&dwRead,NULL)) 
	{
		return NULL;
	}
	
	// ejp: Create a folder for the waves
	CreateDirectory(dirname,NULL);
	
	//for each wave...
	for(i=0;i<wCount;i++)
	{
		//read wave index
		if(!ReadFile(hFile,&wIndex,2,&dwRead,NULL)) 
		{
			return FALSE;
		}

		//read format
		if(!ReadFile(hFile,&btFormat,1,&dwRead,NULL)) 
		{
			return FALSE;
		}

		if(btFormat == 1)
		{	//init unpacker
			InitWaveUnpack(&unpackinfo,hFile,dwSectionSize);
		}
		else
		{	//read number of bytes in all levels
			if(!ReadFile(hFile,&dwBytesInLevels,4,&dwRead,NULL)) 
			{
				return FALSE;
			}
		}
			
		//for each level...
		for(j=0;j<entries[i].btNumLevels;j++)
		{
			//calc size in bytes of decompressed data
			dwWaveMemSize =  entries[i].lpLevels[j].dwNumSamples * sizeof(WORD);
			
			//size x2 for stereo
			dwWaveMemSize *= (entries[i].btFlags & 8) ? 2 : 1;

			//alloc mem for decompressed data	
			entries[i].lpLevels[j].lpwWaveData = (LPWORD)LocalAlloc(LPTR,dwWaveMemSize);
			
			if(btFormat == 1)
			{	//handle compressed data
				if(!DecompressWave(&unpackinfo,	 
								   entries[i].lpLevels[j].lpwWaveData,
								   entries[i].lpLevels[j].dwNumSamples,
							       entries[i].btFlags & 8))
				{
					return FALSE;
				}
				
				//save decompressed wave data
				// ejp: save in folder with shorter name and .wav extension
				sprintf(acName,"%s\\%03i_%03i_%s.wav", dirname, i,j,entries[i].acName);

				hnd = CreateFile(acName,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
				if(hnd != INVALID_HANDLE_VALUE)
				{
					// ejp: wav header stuff (http://www.lightlink.com/tjweber/StripWav/Canon.html)
					int fred;
					WriteFile(hnd,"RIFF",4,&dwWrit,NULL);
					fred = dwWaveMemSize + 36;
					WriteFile(hnd,&fred,4,&dwWrit,NULL);
					WriteFile(hnd,"WAVEfmt ",8,&dwWrit,NULL);
					fred = 16;
					WriteFile(hnd,&fred,4,&dwWrit,NULL);
					fred = 1;
					WriteFile(hnd,&fred,2,&dwWrit,NULL);
					int chans = (entries[i].btFlags) & 8 ? 2 : 1;
					WriteFile(hnd,&chans,2,&dwWrit,NULL);
					WriteFile(hnd,&entries[i].lpLevels[j].dwSampsPerSecond,4,&dwWrit,NULL);
					int blockalign = chans * 2;
					fred = entries[i].lpLevels[j].dwSampsPerSecond * blockalign;
					WriteFile(hnd,&fred,4,&dwWrit,NULL);
					WriteFile(hnd,&blockalign,2,&dwWrit,NULL);
					fred = 16;
					WriteFile(hnd,&fred,2,&dwWrit,NULL);

					WriteFile(hnd,"data",4,&dwWrit,NULL);
					WriteFile(hnd,&dwWaveMemSize,4,&dwWrit,NULL);
					// ejp: end of wav header stuff

					WriteFile(hnd,entries[i].lpLevels[j].lpwWaveData,dwWaveMemSize,&dwWrit,NULL);
					CloseHandle(hnd);
				}		
			}
			else
			{	//not compressed data
				
				//read data into buffer
				if(!ReadFile(hFile,entries[i].lpLevels[j].lpwWaveData,dwWaveMemSize,
							 &dwRead,NULL))
				{
					return FALSE;
				}

				// ejp todo: wav stuff here

				sprintf(acName,"c:\\files\\ucwave%u-%s-%u.raw",i,entries[i].acName,j);
				hnd = CreateFile(acName,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
				if(hnd != INVALID_HANDLE_VALUE)
				{
					WriteFile(hnd,entries[i].lpLevels[j].lpwWaveData,dwWaveMemSize,&dwWrit,NULL);
					CloseHandle(hnd);
				}	
			}
		}

		if(btFormat == 1)
		{	//reset file pointer, ready for next wave data			
			AdjustFilePointer(&unpackinfo);
		}

	}

	return TRUE;
}

//=====================================MAIN=======================================
int main(int argc, char * argv[])
{
	// ejp: Modified most of main() to use FindFirst/FindNext etc, to allow filename or wildcard specification
	printf("BuzzDecomp by Mark Collier (http://www.marcnetsystem.co.uk)\n"
		"Remixed by Ed Powley\n");

	if (argc != 2)
	{
		printf("Usage: BuzzDecomp file.bmx\nor BuzzDecomp *.bmx");
		return 0;
	}

	HANDLE hnd;
	DWORD dwWAVTpos,dwCWAVpos,dwNumEntries,dwCWAVsize;
	WAVT_ENTRY * lpWaveTableEntries;

	WIN32_FIND_DATA fd;
	HANDLE hFind = FindFirstFile(argv[1],&fd);

	int nFilesProcessed = 0;

	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			printf("%s\n", fd.cFileName);

			hnd = CreateFile(fd.cFileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
			if (hnd == INVALID_HANDLE_VALUE)
			{
				printf("ERROR: Could not open file\n");
			}
			else
			{

				//get file position of WAVT and CWAV entries
				if (!ParseHeader(hnd,&dwWAVTpos,&dwCWAVpos,&dwCWAVsize))
				{
					printf("ERROR: failed to parse file header\n");
					CloseHandle(hnd);
				}
				else
				{

					//parse and store wave table section (WAVT)
					lpWaveTableEntries = ParseWAVTSection(hnd,dwWAVTpos,&dwNumEntries);
					if (lpWaveTableEntries == NULL)
					{
						printf("ERROR: failed to parse WAVT section \n");
						CloseHandle(hnd);
					}
					else
					{
						char dirname[256];
						strcpy(dirname, fd.cFileName);
						dirname[strlen(dirname) - 4] = 0;

						//parse and store wave data section (CWAV)
						if(!ParseWaveData(hnd,dwCWAVpos,dwCWAVsize,lpWaveTableEntries,dirname))
						{
							printf("ERROR: failed to parse WAVE/CWAV section \n");
							CloseHandle(hnd);
							TidyEntries(lpWaveTableEntries,dwNumEntries);
						}
						else
						{
						
							//success!... tidy and exit
							TidyEntries(lpWaveTableEntries,dwNumEntries);
							CloseHandle(hnd);

							nFilesProcessed ++;
						}
					}
				}
			}
		}
		while (FindNextFile(hFind, &fd));

		FindClose(hFind);
	}

	return 0;
}