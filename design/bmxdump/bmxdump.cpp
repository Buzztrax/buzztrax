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

//////////////////////////////////////////////////////////////////////////////
// test program for the BmxFile class

#include "bmxfile.h"
using namespace std;

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        cerr << "Please pass me a .bmx or .bmw file\n";
        exit(1);
    }

    BmxFile bmx;
    if (bmx.open(argv[1]) == false)
        exit(1);

    cout << bmx.getFilePath() << " has " << bmx.getNumberOfSections() 
        << " sections\n";

    cout << endl;
    bmx.printSectionEntries();
    bmx.printBverSection();
    bmx.printParaSection();
    bmx.printMachSection();
    bmx.printConnSection();
    bmx.printSequSection();
    bmx.printBlahSection();
    bmx.dumpCwavSection();
}
 
