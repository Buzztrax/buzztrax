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

#define pt_word (CMPType)3
#define MPF_STATE				2	// is continuously changing (not used for notes and triggers)

// global values

CMachineParameter const paraTest =
{
  pt_word,                    // type
  "Test",
  "Test value",                      // description
  1,                        // MinValue
  0xffff,                      // MaxValue
  0,                        // NoValue
  MPF_STATE,                    // Flags
  16                                              // DefValue
};


CMachineParameter const *pParameters[] = {
  // global
  &paraTest,
  // track
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

class gvals
{
public:
  word test;
};

class tvals
{
public:
  word attack;
  word sustain;
  word release;
  word color;
  byte volume;
  byte trigger;

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
  0,                    // min tracks
  0,                    // max tracks
  1,                    // numGlobalParameters
  0,                    // numTrackParameters
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

public:
  int numTracks;

  avals aval;
  gvals gval;
};

mi::mi()
{
  GlobalVals = &gval;
  TrackVals = NULL;
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
  return true;
}

void mi::Stop()
{
}

extern "C" {
CMachineInfo const *GetInfo() { return &MacInfo; }
CMachineInterface *CreateMachine() { return new mi; }
}