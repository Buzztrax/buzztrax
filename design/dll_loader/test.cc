#include <stdio.h>
#include <windef.h>
#include <winbase.h>
#include <windows.h>

#include "libwinelib.h"

#define GET_SYMBOL(res,handle,name) if ((res = WineGetProcAddress(handle,name)) == NULL) { printf("cannot find Windows function %s", name); return -1; }

typedef DWORD dword;
typedef WORD  word;
typedef BYTE  byte;

class CMachineDataOutput
{
public:
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
	void Write(char const *str) { Write((void *)str, strlen(str)+1); }

};

class CLibInterface
{
public:
	virtual void GetInstrumentList(CMachineDataOutput *pout) {}

	// make some space to vtable so this interface can be extended later
	virtual void Dummy1() {}
	virtual void Dummy2() {}
	virtual void Dummy3() {}
	virtual void Dummy4() {}
	virtual void Dummy5() {}
	virtual void Dummy6() {}
	virtual void Dummy7() {}
	virtual void Dummy8() {}
	virtual void Dummy9() {}
	virtual void Dummy10() {}
	virtual void Dummy11() {}
	virtual void Dummy12() {}
	virtual void Dummy13() {}
	virtual void Dummy14() {}
	virtual void Dummy15() {}
	virtual void Dummy16() {}
	virtual void Dummy17() {}
	virtual void Dummy18() {}
	virtual void Dummy19() {}
	virtual void Dummy20() {}
	virtual void Dummy21() {}
	virtual void Dummy22() {}
	virtual void Dummy23() {}
	virtual void Dummy24() {}
	virtual void Dummy25() {}
	virtual void Dummy26() {}
	virtual void Dummy27() {}
	virtual void Dummy28() {}
	virtual void Dummy29() {}
	virtual void Dummy30() {}
	virtual void Dummy31() {}
	virtual void Dummy32() {}
};

typedef enum { pt_note, pt_switch, pt_byte, pt_word } CMPType;

class CMachineParameter
{
public:
	CMPType Type;			// pt_byte
	char const *Name;		// Short name: "Cutoff"
	char const *Description;// Longer description: "Cutoff Frequency (0-7f)"
	int MinValue;			// 0
	int MaxValue;			// 127
	int NoValue;			// 255
	int Flags;
	int DefValue;			// default value for params that have MPF_STATE flag set
};

class CMachineAttribute
{
public:
	char const *Name;
	int MinValue;
	int MaxValue;
	int DefValue;
};

class CMachineInfo
{
public:
	int Type;                                             // MT_GENERATOR or MT_EFFECT,
	int Version;                                          // MI_VERSION
	// v1.2: high word = internal machine version
	// higher version wins if two copies of machine found
	int Flags;
	int minTracks;                                        // [0..256] must be >= 1 if numTrackParameters > 0
	int maxTracks;                                        // [minTracks..256]
	int numGlobalParameters;
	int numTrackParameters;
	CMachineParameter const **Parameters;
	int numAttributes;
	CMachineAttribute const **Attributes;
	char const *Name;                                     // "Jeskola Reverb"
	char const *ShortName;                                // "Reverb"
	char const *Author;                                   // "Oskari Tammelin"
	char const *Commands;                                 // "Command1\nCommand2\nCommand3..."
	CLibInterface *pLI;                                   // ignored if MIF_USES_LIB_INTERFACE is not set
};

typedef CMachineInfo* (*GetInfoFunctionType)();

int load_dll(char *name)
{
	int i;
	void *h;
	DWORD le;

	// typedef of function-paramter-prototyp
	//typedef long (VSTCALLBACK *audioMasterCallback)(AEffect *effect, long opcode, long index, long value, void *ptr, float opt);

	GetInfoFunctionType fptr;
	CMachineInfo* frptr = NULL;

	printf("START\n");
  
  SharedWineInit(NULL);
  
  printf("wine wrapper lib initialized\n");

	// load
	h = WineLoadLibrary(name);
	//le = GetLastError();

	if ( h == NULL)
	{
		printf("load Windows DLL %s failed\n",name);
		//printf("last error %ld\n",le);
		return 1;
	}

	printf("load Windows DLL %s successful\n", name);
	printf("handle: %ld\n", (long int)h);

	// load function
	fptr = (GetInfoFunctionType) WineGetProcAddress(h, "GetInfo");
	//GET_SYMBOL (fptr, h, "GetInfo");
	le = GetLastError();

	if ( fptr == NULL)
	{
		printf ("load Function %s failed\n", "GetInfo");
		printf ("last error %ld\n",le);
		return 2;
	}

	printf("load Function %s successful\n", "GetInfo");
	printf("fptr: %p\n", fptr);

	printf("result: %p\n", frptr);
	frptr = fptr();
	printf("result: %p\n", frptr);

	printf("result: %p\n", frptr);


	printf("Type: %i\n",frptr->Type);
	printf("Version: %i\n",frptr->Version);
	printf("Flags: %i\n",frptr->Flags);
	printf("minTracks: %i\n",frptr->minTracks);
	printf("maxTracks: %i\n",frptr->maxTracks);
	printf("numGlobalParameters: %i\n",frptr->numGlobalParameters);
	printf("numTrackParameters: %i\n",frptr->numTrackParameters);
	//CMachineParameter const **Parameters;
	printf("numAttributes: %i\n",frptr->numAttributes);
	//CMachineAttribute const **Attributes;
	printf("Name: %s\n",frptr->Name);
	printf("ShortName: %s\n",frptr->ShortName);
	printf("Author: %s\n",frptr->Author);
	printf("Commands: %s\n",frptr->Commands);
	//CLibInterface *pLI;                                   // ignored if MIF_USES_LIB_INTERFACE is not set

	// unload
  /*
	FreeLibrary(h);
	le = GetLastError();

	if ( le )
	{
		printf ("unload Windows DLL %s failed\n",name);
		printf ("last error %ld\n",le);
		return 3;
	}

	h=NULL;

	printf("unload Windows DLL %s successful\n", name);
	printf("handle: %ld\n", (long int)h);
  */
	return 0;
}
