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

    cout << " reading BVER Section\n";

    BmxSectionEntry *entry = &entries[section];
    fseek(file, entry->offset, SEEK_SET);
    read_asciiz(buzzversion);
}


void BmxFile::readParaSection()
{
    int section = getNumberOfSection("PARA");

    cout << " reading PARA Section\n";
    
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

    cout << " reading MACH Section\n";

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

    cout << " reading WAVT Section\n";

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
          envelope->disabled=(envelope->numberOfPoints&0x8000)?1:0;
          
          envelope->numberOfPoints&=0x7FFF;
          
          if(envelope->numberOfPoints) {
            envelope->points = new BmxWavtableEnvelopePoints[envelope->numberOfPoints];

            for (unsigned int k = 0; k < envelope->numberOfPoints; k++) {
              BmxWavtableEnvelopePoints *point = &envelope->points[k];
          
              point->x = read_word();
              point->y = read_word();
              point->flags = read_byte();
            }
          }
				}
			}
			// read levels
			wavtable->numberOfLevels = read_byte();
			wavtable->levels = new BmxWavtableLevel[wavtable->numberOfLevels];

      for (unsigned int j = 0; j < wavtable->numberOfLevels; j++) {
        BmxWavtableLevel *level = &wavtable->levels[j];

				level->numberOfSamples = read_dword();
				level->loopBegin = read_dword();
				level->loopEnd = read_dword();
				level->samplesPerSec = read_dword();
				level->rootNote = read_byte();
      }
    }
}


void BmxFile::readCwavSection()
{
    int section = getNumberOfSection("CWAV");
    
    if (section == -1)
        section = getNumberOfSection("WAVE");
    
    if (section == -1)
        return;

    cout << " reading WAVE/CWAV Section\n";

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

				cout << " \treading wave" << i << endl;

				//Decomp.cpp::InitWaveUnpack()
				cwav->dwMaxBytes = MAXPACKEDBUFFER;
				cwav->dwBytesInFileRemain=entries[section].size;
				cwav->dwCurBit = 0;
				// set up so that call to UnpackBits() will force an immediate read from file
				cwav->dwCurIndex = MAXPACKEDBUFFER;
				cwav->dwBytesInBuffer = 0;
        
        for (int j = 0; j < wavt[i].numberOfLevels; j++) {
          dword len;

					cout << " \t\treading level" << j ;

          //calc size in bytes of decompressed data
          len =  wavt[i].levels[j].numberOfSamples * sizeof(word);
					
					cout << " len is " << len;
					
          //size x2 for stereo
          len *= (wavt[i].flags & 8) ? 2 : 1;
					
					cout << ", " << ((wavt[i].flags & 8) ? "stereo":"mono");
					
					wavt[i].levels[j].data = new BmxWavData(len);
          //wavt[i].levels[j].data.buffer = new byte[len];
					
					cout << endl;

					if (cwav->format[i] == 1) {
						// compressed data
						decompressWave((word *)(wavt[i].levels[j].data->buffer),
													wavt[i].levels[j].numberOfSamples,
													((wavt[i].flags & 8)!=0));
					}
					else {
						// uncompressed data
						fread(wavt[i].levels[j].data->buffer,1,len,file);
					}
        }
				if(cwav->format[i]==1) {
					//reset file pointer, ready for next wave data			
					adjustFilePointer();
				}

				/* old code        
        // compressed data
        if (cwav->format[i] == 1) {
            int len = entries[section].size - (2 * sizeof(word)) 
                - sizeof(byte);
            cwav->data[i].buffer = new byte[len];

            for (int j = 0; j < len; j++)
                cwav->data[i].buffer[j] = read_byte();
            break;
        }
        
        // uncompressed data
        if (cwav->format[i] == 0) {
            compressedWaveTable = false;
            break;
        }
				*/
    }
}


void BmxFile::readConnSection()
{
    int section = getNumberOfSection("CONN");
    fseek(file, entries[section].offset, SEEK_SET);

    cout << " reading CONN Section\n";

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

    cout << " reading SEQU Section\n";

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
				cout << " Levels: " << static_cast<int>(wavetable->numberOfLevels) << endl;
				cout << endl;

				for (unsigned int j = 0; j < wavetable->numberOfEnvelopes; j++) {
					BmxWavtableEnvelope *envelope=&wavetable->envelopes[j];
					
					cout << " \tEnvelope " << j << endl;
					cout << " \tADSR: " << envelope->attack
					     << ","  << envelope->decay
					     << ","  << envelope->sustain
					     << ","  << envelope->release
							 << endl;
          cout << " \tSubdiv: " << static_cast<int>(envelope->subdiv) << endl;
          cout << " \tFlags: " << static_cast<int>(envelope->flags) << endl;
          cout << " \tDisabled: " << static_cast<int>(envelope->disabled) << endl;
          cout << " \tPoints: " << envelope->numberOfPoints << endl;
					cout << endl;
				}

				for (unsigned int j = 0; j < wavetable->numberOfLevels; j++) {
					BmxWavtableLevel *level=&wavetable->levels[j];
					
					cout << " \tLevel " << j << endl;
          cout << " \tSamples: " << level->numberOfSamples << endl;
          cout << " \tLoopBeg: " << level->loopBegin << endl;
          cout << " \tLoopEnd: " << level->loopEnd << endl;
          cout << " \tSamplesPerSec: " << level->samplesPerSec << endl;
          cout << " \tRootNote: " << static_cast<int>(level->rootNote) << endl;
					cout << endl;
				}
		}
}


void BmxFile::dumpCwavSection()
{
		char filename[1024];
		char basename[1024];
		int sl;
		
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

		// basename=filepath without 'extension'
		sl=filepath.length();
		if(sl>1023) {
        cerr << " base filename is too long\n";
        return;
		}
		if(sl<5) {
        cerr << " base filename is non-sense\n";
        return;
		}
		strncpy(basename,filepath.c_str(),sl-4);basename[sl]='\0';
		//mkdir(basename);
		
		for (int i = 0; i < cwav->numberOfWavs; i++) {
			for (int j = 0; j < wavt[i].numberOfLevels; j++) {
				// filename=dirname/samplename-level.raw
				//sprintf(filename,"%s/%s-%02d.raw",basename,wavt[i].name.c_str(),j);
				sprintf(filename,"%s-%s-%02d.raw",basename,wavt[i].name.c_str(),j);

				FILE * file = fopen(filename, "wb");
        if (file == 0x0) {
					cerr << "Could not open dump file\n";
          return;
        }
				fwrite(wavt[i].levels[j].data->buffer,1,wavt[i].levels[j].data->size,file),
				fclose(file);
			}
		}
		/* old code
    if (compressedWaveTable == true) {
        cout << " The CWAV section contains compressed data.\n";
        
        if (cwav->numberOfWavs == 1) {
            const string dumpfilename = "cwavdump.raw";
            cout << " Found one compressed wave."
                << "Dumping to " << dumpfilename << endl;

            FILE * dumpfile = fopen(dumpfilename.c_str(), "w");
            if (dumpfile == 0x0) {
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
		*/
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
    if (file == 0x0) {
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


//==================================BIT UNPACKING===================================

dword BmxFile::unpackBits(dword dwAmount)
{	
	dword dwRet,dwReadAmount,dwSize,dwMask,dwVal;
	dword dwFileReadAmnt,dwReadFile,dwShift;
	dword dwMax = 8;

	if((cwav->dwBytesInFileRemain == 0) && (cwav->dwCurIndex == MAXPACKEDBUFFER))
	{
		return 0;
	}
	
	dwReadAmount = dwAmount;
	dwRet = 0;
	dwShift = 0;
	while(dwReadAmount > 0)
	{
		//check to see if we need to update buffer and/or index
		if((cwav->dwCurBit == dwMax) || (cwav->dwBytesInBuffer == 0))
		{	
			cwav->dwCurBit = 0;
			cwav->dwCurIndex++;
			if(cwav->dwCurIndex >= cwav->dwBytesInBuffer )
			{	//run out of buffer... read more file into buffer
				dwFileReadAmnt= (cwav->dwBytesInFileRemain > cwav->dwMaxBytes ) ? cwav->dwMaxBytes : cwav->dwBytesInFileRemain;
				
				dwReadFile=fread(cwav->abtPackedBuffer,1,dwFileReadAmnt,file);

				cwav->dwBytesInFileRemain -= dwReadFile;	
				cwav->dwBytesInBuffer = dwReadFile;
				cwav->dwCurIndex = 0;

				//if we didnt read anything then exit now
				if(dwReadFile == 0)
				{	//make sure nothing else is read
					cwav->dwBytesInFileRemain = 0;
					cwav->dwCurIndex = MAXPACKEDBUFFER;
					return 0;
				}
			}
		}
		
		//calculate size to read from current dword
		dwSize = ((dwReadAmount + cwav->dwCurBit) > dwMax) ? dwMax - cwav->dwCurBit : dwReadAmount;
		
		//calculate bitmask
		dwMask = (1 << dwSize) - 1;

		//Read value from buffer
		dwVal = cwav->abtPackedBuffer[cwav->dwCurIndex];
		dwVal = dwVal >> cwav->dwCurBit;

		//apply mask to value
		dwVal &= dwMask;

		//shift value to correct position
		dwVal = dwVal << dwShift;
		
		//update return value
		dwRet |= dwVal;

		//update info
		cwav->dwCurBit += dwSize;
		dwShift += dwSize;
		dwReadAmount -= dwSize;
	}

	return dwRet;
}


dword BmxFile::countZeroBits(void)
{
	dword dwBit;
	dword dwCount = 0;

	dwBit = unpackBits(1);
	while(dwBit == 0)
	{
		dwCount++;
		dwBit = unpackBits(1);
	}

	return dwCount;
}


void BmxFile::adjustFilePointer(void)
{
	int iRemain = (cwav->dwCurIndex - cwav->dwBytesInBuffer) + 1;			
	// skip <iRemain> bytes
	fseek(file,iRemain,SEEK_CUR);
}


//==================================WAVE DECOMPRESSING===================================

void BmxFile::zeroCompressionValues(BmxCompressionValues *lpcv,dword dwBlockSize)
{
	lpcv->wResult = 0;
	lpcv->wSum1 = 0;
	lpcv->wSum2 = 0;

	//If block size is given, then allocate specfied temporary data
	if (dwBlockSize > 0)
	{
		lpcv->lpwTempData = new word[dwBlockSize]; 
	}
	else
	{
		lpcv->lpwTempData=0x0;
	}
}


void BmxFile::tidyCompressionValues(BmxCompressionValues * lpcv)
{
	//if there is temporary data - then free it.
	if (lpcv->lpwTempData != 0x0)
	{
		delete [] lpcv->lpwTempData;
	}
}


bool BmxFile::decompressSwitch(BmxCompressionValues *lpcv,word *lpwOutputBuffer,dword dwBlockSize)
{
	dword dwSwitchValue,dwBits,dwSize,dwZeroCount;
	word wValue;
	word *lpwaddress;
	if(dwBlockSize == 0)
	{
		return false;
	}

	//Get compression method
	dwSwitchValue = unpackBits(2);

	//read size (in bits) of compressed values
	dwBits = unpackBits(4);

	dwSize = dwBlockSize;
	lpwaddress = lpwOutputBuffer;
	while(dwSize > 0)
	{
		//read compressed value
		// ejp: cast added to suppress compiler warning
		wValue = (word)unpackBits(dwBits);
		
		//count zeros
		dwZeroCount = countZeroBits();
		
		//Construct
		// ejp: cast added to suppress compiler warning
		wValue = (word)((dwZeroCount << dwBits) | wValue);

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
				return false;
		}

		//store value into output buffer
		*lpwOutputBuffer = lpcv->wResult;
		
		//prepare for next loop...
		lpwOutputBuffer++;
		dwSize--;
	}

	return true;
}


bool BmxFile::decompressWave(word *lpwOutputBuffer,dword dwNumSamples,bool bStereo)
{
	dword dwZeroCount,dwShift,dwBlockSize,dwBlockCount,dwLastBlockSize;
	dword dwResultShift,dwCount,i,ixx;
	byte btSumChannels;
	BmxCompressionValues cv1,cv2;

	if(lpwOutputBuffer == 0x0)
	{
		return false;
	}

	dwZeroCount = countZeroBits();
	if (dwZeroCount != 0)
	{
		//printf("Unknown compressed wave data format \n");
		return false;
	}

	//get size shifter
	dwShift = unpackBits(4);

	//get size of compressed blocks
	dwBlockSize = 1 << dwShift;

	//get number of compressed blocks
	dwBlockCount = dwNumSamples >> dwShift;

	//get size of last compressed block
	dwLastBlockSize = (dwBlockSize - 1) & dwNumSamples;

	//get result shifter value (used to shift data after decompression)
	dwResultShift = unpackBits(4);		

	if(!bStereo)
	{	//MONO HANDLING

		//zero internal compression values
		zeroCompressionValues(&cv1,0);

		//If there's a remainder... then handle number of blocks + 1
		dwCount = (dwLastBlockSize == 0) ? dwBlockCount : dwBlockCount +1;
		while(dwCount > 0)
		{
			if (!decompressSwitch(&cv1,lpwOutputBuffer,dwBlockSize))
			{
				return false;
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
		btSumChannels = (byte)unpackBits(1);
		
		//zero internal compression values and alloc some temporary space
		zeroCompressionValues(&cv1,dwBlockSize);
		zeroCompressionValues(&cv2,dwBlockSize);

		//If there's a remainder... then handle number of blocks + 1
		dwCount = (dwLastBlockSize == 0) ? dwBlockCount : dwBlockCount +1;
		while(dwCount > 0)
		{
			//decompress both channels into temporary area
			if(!decompressSwitch(&cv1,cv1.lpwTempData,dwBlockSize))
			{
				return false;
			}

			if (!decompressSwitch(&cv2,cv2.lpwTempData,dwBlockSize))
			{
				return false;
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
		tidyCompressionValues(&cv1);
		tidyCompressionValues(&cv2);
	}

	return true;
}

