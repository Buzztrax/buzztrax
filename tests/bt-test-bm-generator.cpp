/* Buzztrax
 * Copyright (C) 2016 Buzztrax team <buzztrax-devel@buzztrax.org>
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

#include "MachineInterface.h"

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long dword;

#define pt_note (CMPType)0
#define pt_word (CMPType)3
#define MPF_STATE				2	// is continuously changing (not used for notes and triggers)

// global values

CMachineParameter const paraTest =
{
  pt_word,      // type
  "Test",
  "Test value", // description
  1,            // MinValue
  0xffff,       // MaxValue
  0,            // NoValue
  MPF_STATE,    // Flags
  16            // DefValue
};

// track values

CMachineParameter const paraNote =
{
  pt_note,      // type
  "Note",
  "Note",       // description
  0,            // MinValue
  240,          // MaxValue
  0,            // NoValue
  0,            // Flags
  0             // DefValue
};

CMachineParameter const *pParameters[] = {
  // global
  &paraTest,
  // track
  &paraNote,
};

CMachineAttribute const attrMaxDelay =
{
  "Max Delay (ms)",
  1,
  100000,
  1000
};

CMachineAttribute const *pAttributes[] =
{
  &attrMaxDelay
};

#pragma pack(1)

class gvals
{
public:
  word test;
};

class tvals
{
public:
  byte note;

};

class avals
{
public:
  int maxdelay;
};

#pragma pack()

CMachineInfo const MacInfo =
{
  1,                    // type == MT_GENERATOR
  15,                   // version
  0,                    // flags
  1,                    // min tracks
  1,                    // max tracks
  1,                    // numGlobalParameters
  1,                    // numTrackParameters
  pParameters,
  1,
  pAttributes,
  "Buzz Tester G",
  "BT G",               // short name
  "Stefan Sauer",       // author
  NULL
};

class mi : public CMachineInterface
{
public:
  mi();
  virtual ~mi();

  virtual void Init(CMachineDataInput * const pi);
  virtual void Tick();
  virtual bool Work(float *psamples, int numsamples, int const mode);
  virtual void Stop();
  virtual void SetNumTracks(int const n);

public:
  avals aval;
  gvals gval;
  tvals tval[1];
};

mi::mi()
{
  GlobalVals = &gval;
  TrackVals = tval;
  AttrVals = (int *)&aval;
}

mi::~mi()
{
}

void mi::Init(CMachineDataInput * const pi)
{
}

void mi::Tick()
{
}

bool mi::Work(float *psamples, int numsamples, int const)
{
  float v1=0.0, v2=0.0;

  switch (gval.test) {
  case 0:
    v1 = v2 = 0.0;
    break;
  case 1:
    v1 = v2 = 1.0;
    break;
  case 2:
    v1 = v2 = -1.0;
    break;
  case 3:
    v1 = 1.0;
    v2 = -1.0;
    break;
  }
  for (int i = 0; i < numsamples; i+=2) {
    psamples[i] = v1;
    psamples[i+1] = v2;
  }
  return true;
}

void mi::Stop()
{
}

void mi::SetNumTracks(int const n)
{
}

extern "C" {
CMachineInfo const *GetInfo() { return &MacInfo; }
CMachineInterface *CreateMachine() { return new mi; }
}