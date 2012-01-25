/*
 * Copyright (C) 2004 Stefan Sperling <stsp@binarchy.net>
 *                          
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or 
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

// Reads .bmx and .bmw files (the native file format of Jeskola Buzz)

#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define DELPTR(p)\
if (p != 0x0) \
    delete p;\
p = 0x0

#define DELARR(p)\
if (p != 0x0) \
    delete [] p;\
p = 0x0

typedef unsigned int dword;
typedef unsigned short word;
typedef unsigned char byte;


//////////////////////////////////////////////////////////////////////////////
// bmx file data structures

struct BmxSectionEntry
{
    std::string name;
    dword offset;
    dword size;
};

struct BmxCompressionValues
{
  word  wSum1;
  word  wSum2;
  word  wResult;

  word  *lpwTempData;
};

struct BmxWavData
{
    BmxWavData();
    BmxWavData(int len);
    ~BmxWavData();
    byte *buffer;
    word size;
};

inline BmxWavData::BmxWavData() { 
    buffer = 0x0;
    size = 0;
}

inline BmxWavData::BmxWavData(int len) { 
    buffer = new byte[len];
    size = len;
}

inline BmxWavData::~BmxWavData() {
  DELARR(buffer);
}

#define MAXPACKEDBUFFER 2048

struct BmxCwavSection
{
    BmxCwavSection();
    ~BmxCwavSection();
    word numberOfWavs;
    word *index;
    byte *format;
    //BmxWavData *data; // old code
    
    byte abtPackedBuffer[MAXPACKEDBUFFER];
    dword dwCurIndex;
    dword dwCurBit;

    dword dwBytesInBuffer;
    dword dwMaxBytes;
    dword dwBytesInFileRemain;
};

inline BmxCwavSection::BmxCwavSection() { 
    index = 0x0; 
    format = 0x0;
    //data = 0x0; // old code
    
    dwMaxBytes = MAXPACKEDBUFFER;
    //dwBytesInFileRemain = dwSectionSize;  // in BmxFile::readCwavSection
    dwCurBit = 0;

    // set up so that call to UnpackBits() will force an immediate read from file
    dwCurIndex = MAXPACKEDBUFFER;
    dwBytesInBuffer = 0;
}

inline BmxCwavSection::~BmxCwavSection()  { 
    //DELARR(data); // old code
    DELARR(index);
    DELARR(format);
}


struct BmxWavtableEnvelopePoints {
    word x,y;
    byte flags;
};

struct BmxWavtableEnvelope {
    BmxWavtableEnvelope();
    ~BmxWavtableEnvelope();
    word attack;
    word decay;
    word sustain;
    word release;
    byte subdiv;
    byte flags;
    word numberOfPoints;
    byte disabled;
    BmxWavtableEnvelopePoints *points;
};

inline BmxWavtableEnvelope::BmxWavtableEnvelope() {
    points = 0x0;
}

inline BmxWavtableEnvelope::~BmxWavtableEnvelope() {
    DELARR(points);
}

struct BmxWavtableLevel {
    public:
      BmxWavtableLevel();
      ~BmxWavtableLevel();
      dword numberOfSamples;
      dword loopBegin;
      dword loopEnd;
      dword samplesPerSec;
      byte rootNote;
      BmxWavData *data;
};

inline BmxWavtableLevel::BmxWavtableLevel() {
    data = 0x0;
}

inline BmxWavtableLevel::~BmxWavtableLevel() {
    if(data != 0x0) delete data;
}

struct BmxWavtSection
{
    public:
      BmxWavtSection();
      ~BmxWavtSection();
      word index;
      std::string filename;
      std::string name;
      float volume;
      byte flags;
      
      word  numberOfEnvelopes;
      BmxWavtableEnvelope *envelopes;
      
      byte  numberOfLevels;
      BmxWavtableLevel *levels;
};

inline BmxWavtSection::BmxWavtSection() {
    envelopes = 0x0;
    levels = 0x0;
}

inline BmxWavtSection::~BmxWavtSection() {
    DELARR(envelopes);
    DELARR(levels);
}


struct BmxMachineAttributes
{
    std::string key;
    dword value;
};

class BmxMachTrackState
{
    public:
        BmxMachTrackState();
        ~BmxMachTrackState();
        word *parameterState;
};

inline BmxMachTrackState::BmxMachTrackState() {
    parameterState = 0x0;
}

inline BmxMachTrackState::~BmxMachTrackState() {
    DELARR(parameterState);
}

class BmxMachSection
{
    public:
        BmxMachSection();
        ~BmxMachSection();
        std::string name;
        byte type; // 0 = master, 1 = generator, 2 = effect
        std::string dllname; // name of DLL file if type is 1 or 2
        float xpos;
        float ypos;
        dword sizeOfMachineSpecificData;
        // machine specific data is still being ignored
        // byte *machineSpecificData;
        
        word numberOfAttributes;
        BmxMachineAttributes *attributes;
        
        word *globalParameterState;
        
        word numberOfTracks;
        
        BmxMachTrackState *tracks;
};

inline BmxMachSection::BmxMachSection() { 
    attributes = 0x0;
    globalParameterState = 0x0; 
    tracks = 0x0;
}

inline BmxMachSection::~BmxMachSection() { 
    DELARR(attributes);
    DELARR(globalParameterState);
    DELARR(tracks);
}


struct BmxParameter
{
    byte type;
    std::string name;
    int minvalue;
    int maxvalue;
    int novalue;
    int flags;
    int defvalue;
};

class BmxParaSection
{
    public:
        BmxParaSection();
        ~BmxParaSection();
        std::string name;

        std::string type;
        dword numberOfGlobalParams;
        dword numberOfTrackParams;
        int size;
        BmxParameter *globalParameter;
        BmxParameter *trackpara;
};

inline BmxParaSection::BmxParaSection() {
    globalParameter = 0x0;
    trackpara = 0x0;
}

inline BmxParaSection::~BmxParaSection() { 
    DELARR(globalParameter);
    DELARR(trackpara);
}


struct BmxPattSection
{
    word numberOfPatterns;
    word numberOfTracks;
    char* name;
    word length;
    // FIXME(ensonic): missing pattern data!
};


struct BmxMachineConnection
{
    word sourceIndex;
    word destIndex;
    word amp;
    word pan;
};


struct BmxSequence
{
    word indexOfMachine;
    dword numberOfEvents;
    byte bytesPerPos;
    byte bytesPerEvent; // 2 if there are more than 112 patterns
    word *eventPos;
    word *event;
};

//////////////////////////////////////////////////////////////////////////////
// interface

class BmxFile
{
    FILE* file;
    std::string filepath;
    
    // DIR ENTRIES
    dword numberOfSections;
    BmxSectionEntry *entries;
    
    // BVER
    std::string buzzversion;
    
    // PARA
    dword numberOfMachines;
    BmxParaSection *para;
    bool parasection; // PARA Section present
    
    // MACH
    BmxMachSection *mach;
    
    // WAVT
    dword numberOfWaves;
    BmxWavtSection *wavt;
    
    // CWAV / WAVE
    BmxCwavSection *cwav;
    bool compressedWaveTable;

    // CONN
    word numberOfMachineConnections;
    BmxMachineConnection *conn;

    // SEQU
    dword endOfSong;
    dword beginOfLoop;
    dword endOfLoop;
    word numberOfSequences;
    BmxSequence *sequ; 
    
    // BLAH 
    dword lengthOfBlahSection; // in characters
    unsigned char *blahText;
    
    protected:
    void initvars();
    
    dword read_dword();
    word read_word();
    float read_float();
    int read_int();
    byte read_byte();
    void read_asciiz(std::string &buffer);

    bool setupSections();
    void readSectionName(std::string &buffer);
    int getNumberOfSection(const char* name);

    void readMachSection();
    void readParaSection();
    void readBverSection();
    void readWavtSection();
    void readCwavSection();
    void readConnSection();
    void readSequSection();
    void readBlahSection();
    
    //BOOL decompressWave(LPWORD lpwOutputBuffer,DWORD dwNumSamples,BOOL bStereo)

    public:

    BmxFile();
    ~BmxFile();
    
    bool open(const char* path);
    // Opens and verifies a bmx/bmw file.
    // If an error occurs, false is returned.

    int close();

    inline dword getNumberOfSections() {
        return numberOfSections;
    }
    
    inline std::string getFilePath() {
        return filepath;
    }

    void printSectionEntries();
    void printParaSection();
    void printMachSection();
    void printWavtSection();
    void dumpCwavSection();
    void printConnSection();
    void printSequSection();
    
    inline void printBlahSection() { 
        if (blahText != 0x0)
            std::cout << " -- BLAH Section\n\n" << blahText << std::endl;
    }
    
    inline void printBverSection() {
        std::cout << " -- BVER Section\n " << buzzversion << std::endl << std::endl;
    }
    
    dword unpackBits(dword dwAmount);
    dword countZeroBits(void);
    void adjustFilePointer(void);
    
    void zeroCompressionValues(BmxCompressionValues *lpcv,dword dwBlockSize);
    void tidyCompressionValues(BmxCompressionValues * lpcv);
    bool decompressSwitch(BmxCompressionValues *lpcv,word *lpwOutputBuffer,dword dwBlockSize);
    bool decompressWave(word *lpwOutputBuffer,dword dwNumSamples,bool bStereo);

};

