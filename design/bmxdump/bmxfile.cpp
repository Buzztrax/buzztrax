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
// and can print some info to stdout.

#include "bmxfile.h"
using namespace std;

//////////////////////////////////////////////////////////////////////////////
// reading bmx file sections
 
bool BmxFile::setupSections()
{
    numberOfSections = read_dword();
    
    if (numberOfSections == 0) {
        cerr << file << " has no sections - it may have been corrupted.\n";
        return false;
    }

    entries = new BmxSectionEntry[numberOfSections];

    fseek(file, 4 + sizeof(dword), SEEK_SET);
    
    for (unsigned int i = 0; i < numberOfSections; i++) {
        BmxSectionEntry *entry = &entries[i];
        readSectionName(entry->name);
        entry->offset = read_dword();
        entry->size = read_dword();
    }
    
    // now read all sections
    readBverSection();
    readParaSection();
    if (parasection == false) {
        cerr << "This file does not comply with Buzz v1.2\n"
            << "Please save it using Buzz v1.2 and retry\n";
        return false;
    }
    readMachSection();
    readConnSection();
    readSequSection();
    readWavtSection();
    readCwavSection();
    readBlahSection();
        
    return true; 
}


void BmxFile::readSectionName(string &str)
{
    char tmp[4];
    if (fread(tmp, sizeof(char), 4, file) != 4)
        return;
    str.assign(tmp, 4);
}


int BmxFile::getNumberOfSection(const char* sectionName)
{
    for (unsigned int i = 0; i< numberOfSections; i++) {
        BmxSectionEntry *entry = &entries[i];
        if (strcmp(entry->name.c_str(), sectionName) == 0)
                return i;
    }
    return -1;            
}


void BmxFile::readBverSection()
{
    int section = getNumberOfSection("BVER");
    if (section == -1) {
        buzzversion = "No version info available";
        return;
    }

    BmxSectionEntry *entry = &entries[section];
    fseek(file, entry->offset, SEEK_SET);
    read_asciiz(buzzversion);
}


void BmxFile::readParaSection()
{
    int section = getNumberOfSection("PARA");
    
    if (section == -1) // There's no PARA Section in this file. Beat it!
        return;
   
    parasection = true; // PARA Section present, let everyone know!
    
    BmxSectionEntry *entry = &entries[section];
    fseek(file, entry->offset, SEEK_SET);

    numberOfMachines = read_dword();

    para = new BmxParaSection[numberOfMachines];

    for (unsigned int i = 0; i < numberOfMachines; i++) {
        read_asciiz(para[i].name);
        read_asciiz(para[i].type);
        para[i].numberOfGlobalParams = read_dword();
        para[i].numberOfTrackParams = read_dword();
        
        para[i].globalParameter = new BmxParameter[para[i].numberOfGlobalParams];
        para[i].trackpara = new BmxParameter[para[i].numberOfTrackParams];

        for (unsigned int j = 0; j < para[i].numberOfGlobalParams; j++) {
            BmxParameter *param = &para[i].globalParameter[j];
            param->type = read_byte();
            read_asciiz(param->name);
            param->minvalue = read_int();
            param->maxvalue = read_int();
            param->novalue = read_int();
            param->flags = read_int();
            param->defvalue = read_int();
        }
        
        for (unsigned int j = 0; j < para[i].numberOfTrackParams; j++) {
            BmxParameter *param = &para[i].trackpara[j];
            param->type = read_byte();
            read_asciiz(param->name);
            param->minvalue = read_int();
            param->maxvalue = read_int();
            param->novalue = read_int();
            param->flags = read_int();
            param->defvalue = read_int();
        }
    }
}


void BmxFile::readMachSection()
{
    BmxSectionEntry *entry = &entries[getNumberOfSection("MACH")];
    int offset = entry->offset;

    if (parasection == true) {
        offset += sizeof(word); // we already know the number of machines
        fseek(file, offset, SEEK_SET);
    } else {
        fseek(file, offset, SEEK_SET);
        numberOfMachines = static_cast<dword>(read_word());
    }

    mach = new BmxMachSection[numberOfMachines];
    
    for (unsigned int i = 0; i < numberOfMachines; i++) {
       BmxMachSection *machine = &mach[i];
       
       read_asciiz(machine->name);
       machine->type = read_byte();
       if (machine->type == 0) // machine is Master
           machine->dllname = "none";
       else if (machine->type == 1 || machine->type == 2)
           read_asciiz(machine->dllname);

       machine->xpos = read_float();
       machine->ypos = read_float();
       machine->sizeOfMachineSpecificData = read_dword();
       
       // FIXME: skipping machine specific data
       if (machine->sizeOfMachineSpecificData > 0)
           fseek(file, machine->sizeOfMachineSpecificData, SEEK_CUR);
            
       machine->numberOfAttributes = read_word();
       if (machine->numberOfAttributes > 0) {
           machine->attributes = 
               new BmxMachineAttributes[machine->numberOfAttributes];

           for (unsigned int j = 0; j < machine->numberOfAttributes; j++) {
               BmxMachineAttributes *attribute = &machine->attributes[j];
               read_asciiz(attribute->key);
               attribute->value = read_dword();
           }
       }

       if (parasection == false)
               return;

       if (para[i].numberOfGlobalParams > 0) {
           machine->globalParameterState = new word[para[i].numberOfGlobalParams];
       
           for (unsigned int k = 0; k < para[i].numberOfGlobalParams; k++) {
               BmxParameter *param = &para[i].globalParameter[k];
               
               if (param->type == 0 || param->type == 1 || param->type == 2)
                   machine->globalParameterState[k] = static_cast<word>(read_byte());
               else if (param->type == 3)
                   machine->globalParameterState[k] = read_word();
           }
       }

       machine->numberOfTracks = read_word();
       
       if (machine->numberOfTracks > 0) {
           machine->tracks = new BmxMachTrackState[machine->numberOfTracks];

           for (unsigned int t = 0; t < machine->numberOfTracks; t++) {
               BmxMachTrackState *track = &machine->tracks[t];

               track->parameterState = new word[para[i].numberOfTrackParams];
               
               for (unsigned int k = 0; k < para[i].numberOfTrackParams; k++) {
                   BmxParameter *param = &para[i].trackpara[k];
                   
                   if (param->type == 0 || param->type == 1 
                           || param->type == 2)
                       track->parameterState[k] = static_cast<word>(read_byte());
                   else if (param->type == 3)
                       track->parameterState[k] = read_word();
               }
           }
       }
    }
}


void BmxFile::readWavtSection()
{
		BmxSectionEntry *entry = &entries[getNumberOfSection("WAVT")];
		
		fseek(file, entry->offset, SEEK_SET);
		numberOfWaves = static_cast<dword>(read_word());
		
		wavt = new BmxWavtSection[numberOfWaves];
    for (unsigned int i = 0; i < numberOfMachines; i++) {
       BmxWavtSection *wavtable = &wavt[i];
       
			 // reader header
       wavtable->index = read_word();
       read_asciiz(wavtable->filename);
       read_asciiz(wavtable->name);
       wavtable->volume = read_float();
       wavtable->flags = read_byte();
			 if(wavtable->flags&0x80) {
				 // read envelopes
				 wavtable->numberOfEnvelopes = read_word();
				 wavtable->envelopes = new BmxWavtableEnvelope[wavtable->numberOfEnvelopes];

				 for (unsigned int j = 0; j < wavtable->numberOfEnvelopes; j++) {
					 	BmxWavtableEnvelope *envelope = &wavtable->envelopes[j];
						
						envelope->attack = read_word();
						envelope->decay = read_word();
						envelope->sustain = read_word();
						envelope->release = read_word();
						envelope->subdiv = read_byte();
						envelope->flags = read_byte();
						envelope->numberOfPoints = read_word();
				 }
			 }
			 // read levels
			 //wavtable->numberOfLevels = read_word();
			 //wavtable->levels = new BmxWavtableLevel[wavtable->numberOfLevels];
		}
}


void BmxFile::readCwavSection()
{
    int section = getNumberOfSection("CWAV");
    
    if (section == -1)
        section = getNumberOfSection("WAVE");
    
    if (section == -1)
        return;

    fseek(file, entries[section].offset, SEEK_SET);
  
    cwav = new BmxCwavSection;
    cwav->numberOfWavs = read_word();
    
    if (cwav->numberOfWavs == 0)
        return;

    cwav->index = new word[cwav->numberOfWavs];
    cwav->format = new byte[cwav->numberOfWavs];
    cwav->data = new BmxWavData[cwav->numberOfWavs];
    
    for (int i = 0; i < cwav->numberOfWavs; i++) {
        cwav->index[i] = read_word();
        cwav->format[i] = read_byte();
    
        if (cwav->format[i] == 1) {
            int len = entries[section].size - (2 * sizeof(word)) 
                - sizeof(byte);
            cwav->data[i].buffer = new byte[len];

            for (int j = 0; j < len; j++)
                cwav->data[i].buffer[j] = read_byte();
            break;
        }
        
        if (cwav->format[i] == 0) {
            compressedWaveTable = false;
            break;
        }
    }
}


void BmxFile::readConnSection()
{
    int section = getNumberOfSection("CONN");
    fseek(file, entries[section].offset, SEEK_SET);

    numberOfMachineConnections = read_word();

    conn = new BmxMachineConnection[numberOfMachineConnections];

    for (int i = 0; i < numberOfMachineConnections; i++) {
        conn[i].sourceIndex = read_word();
        conn[i].destIndex = read_word();
        conn[i].amp = read_word();
        conn[i].pan = read_word();
    }
}


void BmxFile::readSequSection()
{
    int section = getNumberOfSection("SEQU");
    fseek(file, entries[section].offset, SEEK_SET);
   
    endOfSong = read_dword();
    beginOfLoop = read_dword();
    endOfLoop = read_dword();
    numberOfSequences = read_word();

    sequ = new BmxSequence[numberOfSequences];

    for (int i = 0; i < numberOfSequences; i++) {
        sequ[i].indexOfMachine = read_word();
        sequ[i].numberOfEvents = read_dword();
        sequ[i].bytesPerPos = read_byte();
        sequ[i].bytesPerEvent = read_byte();

        sequ[i].eventPos 
            = new word[sequ[i].numberOfEvents * sequ[i].bytesPerPos];
        sequ[i].event 
            = new word[sequ[i].numberOfEvents * sequ[i].bytesPerEvent];

        for (unsigned int j = 0; j < sequ[i].numberOfEvents; j++) {
            if (sequ[i].bytesPerPos == 1)
                sequ[i].eventPos[j] = static_cast<word>(read_byte());
            else if (sequ[i].bytesPerPos == 2)
                sequ[i].eventPos[j] = read_word();
            else {
                cerr << "Bytes per Position in SEQU section is " 
                    << sequ[i].bytesPerPos << endl
                    << " - I don't know how to handle this - skipping!\n";
                byte tmp;
                for (int k = 0; k < sequ[i].bytesPerPos; k++)
                    tmp = read_byte();
            }
            
            if (sequ[i].bytesPerEvent == 1)
                sequ[i].event[j] = static_cast<word>(read_byte());
            else if (sequ[i].bytesPerEvent == 2)
                sequ[i].eventPos[j] = read_word();
            else {
                cerr << "Bytes per Event in SEQU section is " 
                    << sequ[i].bytesPerEvent << endl
                    << " - I don't know how to handle this - skipping!\n";
                byte tmp;
                for (int k = 0; k < sequ[i].bytesPerEvent; k++)
                    tmp = read_byte();
            }
        }
    }
}


void BmxFile::readBlahSection()
{
    int section = getNumberOfSection("BLAH");
    fseek(file, entries[section].offset, SEEK_SET);

    lengthOfBlahSection = read_dword();

    blahText = new unsigned char[lengthOfBlahSection + 1];

    for (unsigned int i = 0; i < lengthOfBlahSection; i++)
        blahText[i] = read_byte();
    
    blahText[lengthOfBlahSection +1] = '\0';
}


//////////////////////////////////////////////////////////////////////////////
// printing bmx file sections to stdout

void BmxFile::printSectionEntries()
{
    if (entries == 0x0) {
        cerr << "Section entries have not been set up!\n";
        return;
    }
    
    for (unsigned int i = 0; i < numberOfSections; i++) {
        BmxSectionEntry *entry = &entries[i];
        cout << " Section " << i << endl;
        cout << " Name: " << entry->name << endl;
        cout << " Offset: " << entry->offset << endl;
        cout << " Size: " << entry->size << endl;
        cout << endl;
    }
}


void BmxFile::printParaSection()
{
    cout << " -- PARA Section\n";
    cout << endl;
    
    if (parasection == false) {
        cerr << "PARA Section has not been read yet or does not exist!\n";
        return;
    }

    cout << " Number of Machines : " << numberOfMachines << endl;
    cout << endl;
    for (unsigned int i = 0; i < numberOfMachines; i++) {
        cout << " Machine " << i << endl;
        cout << " Name: " << para[i].name << endl;
        cout << " Type: " << para[i].type << endl;
        cout << " Global Parameters: " << para[i].numberOfGlobalParams << endl;
        cout << " Track Parameters: " << para[i].numberOfTrackParams << endl;
        cout << endl;
        
        for (unsigned int j = 0; j < para[i].numberOfGlobalParams; j++) {
            BmxParameter *param = &para[i].globalParameter[j];
            cout << " \tGlobal Parameter " << j << endl;
            cout << " \tType: " << static_cast<int>(param->type) << endl;
            cout << " \tName: " << param->name << endl;
            cout << " \tminvalue: " << param->minvalue << endl;
            cout << " \tmaxvalue: " << param->maxvalue << endl;
            cout << " \tnovalue: " << param->novalue << endl;
            cout << " \tflags: " << param->flags << endl;
            cout << " \tdefvalue: " << param->defvalue << endl;
            cout << endl;
        }
        
        for (unsigned int j = 0; j < para[i].numberOfTrackParams; j++) {
            BmxParameter *param = &para[i].trackpara[j];
            cout << " \tTrack Parameter " << j << endl;
            cout << " \tType: " << static_cast<int>(param->type) << endl;
            cout << " \tName: " << param->name << endl;
            cout << " \tminvalue: " << param->minvalue << endl;
            cout << " \tmaxvalue: " << param->maxvalue << endl;
            cout << " \tnovalue: " << param->novalue << endl;
            cout << " \tflags: " << param->flags << endl;
            cout << " \tdefvalue: " << param->defvalue << endl;
            cout << endl;
        }
    }
}


void BmxFile::printMachSection()
{
    cout << " -- MACH Section\n";
    cout << endl;

    if (mach == 0x0) {
        cerr << "MACH Section has not been read yet!\n";
        return;
    } 
    
    for (unsigned int i = 0; i < numberOfMachines; i++) {
        BmxMachSection *machine = &mach[i];

        cout << " Machine " << i << endl;
        cout << " Name: " << machine->name << endl;
        cout << " Type: " << static_cast<int>(machine->type) << endl;
        cout << " DLL: " << machine->dllname << ".dll" << endl;
        cout << " X position in machine view: " << machine->xpos << endl;
        cout << " Y position in machine view: " << machine->ypos << endl;
        cout << " Size of machine specific data: " 
            << machine->sizeOfMachineSpecificData << endl;
        cout << " Number of attributes: " << machine->numberOfAttributes 
            << endl;
        cout << " Number of tracks: " << machine->numberOfTracks << endl;
        cout << endl;

        cout << " Parameter states" << endl << endl;

        for (unsigned int j = 0; j < machine->numberOfAttributes; j++) {
            BmxMachineAttributes *attr = &machine->attributes[j];
            cout << " \tAttribute " << j << " (" << attr->key 
                << ") = " <<  attr->value << endl;
        }
        cout << endl;

        for (unsigned int k = 0; k < para[i].numberOfGlobalParams; k++) {
            BmxParameter *param = &para[i].globalParameter[k];
       
            cout << " \tGlobal parameter " << k << " (" 
                << param->name << ") = " << machine->globalParameterState[k] 
                << endl;
        }
        cout << endl;
        for (unsigned int t = 0; t < machine->numberOfTracks; t++) {
            cout << " \tTrack " << t << endl;

            for (unsigned int k = 0; k < para[i].numberOfTrackParams; k++) {
                BmxParameter *param = &para[i].trackpara[k];
           
                cout << " \t\tTrack parameter " << k << " (" 
                    << param->name << ") = " 
                    << machine->tracks->parameterState[k] << endl;
            }
            cout << endl;
        }
        cout << endl;
    }
}


void BmxFile::printWavtSection()
{
    cout << " -- WAVT Section\n";
    cout << endl;
		
    if (wavt == 0x0) {
        cerr << "WAVT Section has not been read yet!\n";
        return;
    } 

    for (unsigned int i = 0; i < numberOfWaves; i++) {
        BmxWavtSection *wavetable = &wavt[i];

        cout << " Wave " << i << endl;
				cout << " Index: " << wavetable->index << endl;
				cout << " FileName: " << wavetable->filename << endl;
				cout << " Name: " << wavetable->name << endl;
				cout << " Volume: " << wavetable->volume << endl;
				cout << " Flags: " << static_cast<int>(wavetable->flags) << endl;
				cout << " Envelopes: " << wavetable->numberOfEnvelopes << endl;
				cout << " Levels: " << wavetable->numberOfLevels << endl;
				cout << endl;

				for (unsigned int j = 0; j < wavetable->numberOfEnvelopes; j++) {
					BmxWavtableEnvelope *envelope=&wavetable->envelope[j];
					
					cout << " \tEnvelope " << j << endl;
					cout << " \tADSR: " << envelope->attack
					     << ","  << envelope->decay
					     << ","  << envelope->sustain
					     << ","  << envelope->release
							 << endl;
					cout << endl;
				}

				for (unsigned int j = 0; j < wavetable->numberOfLevels; j++) {
					BmxWavtableLevel *level=&wavetable->level[j];
					
					cout << " \tLevel " << j << endl;
					cout << endl;
				}
		}
}


void BmxFile::dumpCwavSection()
{
    cout << " -- CWAV Section\n";
    cout << endl;
      
    if (cwav == 0x0) {
        cerr << " CWAV Section has not been read yet or does not exist!\n";
        return;
    }
   
    if (cwav->numberOfWavs == 0) {
        cout << " There is no audio data in this file\n";
        return;
    }
  
    if (compressedWaveTable == true) {
        cout << " The CWAV section contains compressed data.\n";
        
        if (cwav->numberOfWavs == 1) {
            const string dumpfilename = "cwavdump.raw";
            cout << " Found one compressed wave."
                << "Dumping to " << dumpfilename << endl;

            FILE * dumpfile = fopen(dumpfilename.c_str(), "w");
            if (dumpfile == NULL) {
                cerr << "Could not open dump file\n";
                return;
            }

            int section = getNumberOfSection("CWAV");
            
            int len = entries[section].size - (2 * sizeof(word)) - sizeof(byte);
            for (int i = 0; i < len; i++)
                fputc(cwav->data[0].buffer[i], dumpfile);
            cout << endl;
        }
        else
            cout << " Found " << cwav->numberOfWavs << " compressed waves.\n"
                << " There's more than one wave in this file, so I won't dump!\n"
                << " I don't know how to figure out the length of each wave.\n"
                << " Pass me a bmx with only one wave in the wavetable if you want\n"
                << " a raw dump of a compressed wave.\n";
    }
    else if (compressedWaveTable == false)
            cout << " The CWAV section contains uncompressed data!!!\n"
                << " Ignoring it though, for the time being...\n";
}


void BmxFile::printConnSection()
{
    cout << " -- CONN Section\n";
    cout << endl;
    
    if (conn == 0x0) {
        cerr << "CONN Section has not been read yet!\n";
        return;
    }

    for (int i = 0; i < numberOfMachineConnections; i++) {
        cout << mach[conn[i].sourceIndex].name << " --> "
            << mach[conn[i].destIndex].name << "\nVolume = " << conn[i].amp
            << "\tPan = " << conn[i].pan << endl << endl;
    }
    cout << endl;
}


void BmxFile::printSequSection()
{
    cout << " -- SEQU Section\n" << endl;

    if (sequ == 0x0) {
        cerr << "SEQU Section has no been read yet!\n";
        return;
    }

    
    cout << " Possible event values: \n"
        << " 00 = mute, 01 = break, 02 = thru \n"
        << " 0x10 = first pattern, 0x11 = second pattern, etc.\n"
		<< " msb=1 indicates loop (what on earth does msb mean?)\n" << endl;
    
    cout << " end of song: " << endOfSong << endl;
    cout << " begin of loop: " << beginOfLoop << endl;
    cout << " end of loop: " << endOfLoop << endl;
    cout << " number of sequences: " << numberOfSequences 
        << endl << endl;

    for (int i = 0; i < numberOfSequences; i++) {
        cout << " Sequence " << i << endl;
        cout << " \tMachine: " << mach[sequ[i].indexOfMachine].name << endl;
        cout << " \tNumber of Events: " << sequ[i].numberOfEvents << endl;
        cout << " \tBytes per Event position: " 
            << static_cast<int>(sequ[i].bytesPerPos) << endl;
        cout << " \tBytes per Event: " 
            << static_cast<int>(sequ[i].bytesPerEvent) << endl;

        for (unsigned int j = 0; j < sequ[i].numberOfEvents; j++) {
            cout << " \tEvent " << j << " \tPosition: " 
                << static_cast<int>(sequ[i].eventPos[j]) 
                << " \tValue: 0x" << hex << static_cast<int>(sequ[i].event[j]) 
                << dec << endl;
        }
        cout << endl;
    }
    cout << endl;
}


//////////////////////////////////////////////////////////////////////////////
// initialisation

BmxFile::BmxFile()
{
    initvars();
}


BmxFile::~BmxFile()
{

    int r = close();
    if (r != 0) {
        cerr << "BmxFile: Whooops! Error closing " << filepath 
            << " in destructor. Dying anyway!\n";
        file = 0x0;
    }
        
    DELARR(entries);
    DELARR(para);
    DELARR(conn);
		DELPTR(wavt);
    DELPTR(cwav);
    DELARR(blahText);
}


inline void BmxFile::initvars()
{
    file = 0x0;
    numberOfSections = 0;
    entries = 0x0;
    para = 0x0;
    parasection = false;
    conn = 0x0;
		wavt = 0x0;
    cwav = 0x0;
    compressedWaveTable = true;
    blahText = 0x0;
}


//////////////////////////////////////////////////////////////////////////////
// file handling

bool BmxFile::open(const char* path)
{
    filepath = path;
    string tmp;
    
    file = fopen(path, "r");
    if (file == NULL) {
        cerr << "Cannot open " << path << endl;
        return false;
    }

    readSectionName(tmp);
    if (tmp != "Buzz") {
        cerr << path << " is not a valid bmx file\n";
        return false;
    }

    if (setupSections() == false)
        return false;
    else
        cout << "Sucessfully opened " << path << endl;
    return true;
}      


int BmxFile::close()
{
    if (file != 0x0) {
        int r = fclose(file);
        if (r != 0)
            cerr << "Could not close " << file << endl;
        else
            file = 0x0;
        return r;
    }
    else
        return -1;
}


dword BmxFile::read_dword()
{
    dword buffer;
    int r = fread(&buffer, sizeof(dword), 1, file);
    if (r != 1)
        if (ferror(file) != 0 || feof(file) != 0)
            cerr << "Error reading from " << filepath << endl;
    return buffer;
}


word BmxFile::read_word()
{
    word buffer;
    int r = fread(&buffer, sizeof(word), 1, file);
    if (r != 1)
        if (ferror(file) != 0 || feof(file) != 0)
            cerr << "Error reading from " << filepath << endl;
    return buffer;
}


int BmxFile::read_int()
{
    int buffer;
    int r = fread(&buffer, sizeof(int), 1, file);
    if (r != 1)
        if (ferror(file) != 0 || feof(file) != 0)
            cerr << "Error reading from " << filepath << endl;
    return buffer;
}


byte BmxFile::read_byte()
{
    byte buffer;
    int r = fread(&buffer, sizeof(byte), 1, file);
    if (r != 1)
        if (ferror(file) != 0 || feof(file) != 0)
            cerr << "Error reading from " << filepath << endl;
    return buffer;
}


float BmxFile::read_float()
{
    float buffer;
    int r = fread(&buffer, sizeof(float), 1, file);
    if (r != 1)
        if (ferror(file) != 0 || feof(file) != 0)
            cerr << "Error reading from " << filepath << endl;
    return buffer;
}


void BmxFile::read_asciiz(string &str)
{
   char tmp;
   str.erase();

   while (true) {
       tmp = static_cast<char>(fgetc(file));
       if (tmp == EOF || tmp == '\0') {
        if (ferror(file) != 0 || feof(file) != 0)
            cerr << "Error reading from " << filepath << endl;
           break;
       }
       else 
           str += tmp;
   }
}

