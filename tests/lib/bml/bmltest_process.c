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
/*
 * process data with a buzz machine
 *
 * invoke it e.g. as
 *   LD_LIBRARY_PATH=".:./BuzzMachineLoader/.libs" ./bmltest_process ../machines/elak_svf.dll input.raw output1.raw
 *   LD_LIBRARY_PATH=".:./BuzzMachineLoader/.libs" ./bmltest_process ../../buzzmachines/Elak/SVF/libelak_svf.so input.raw output2.raw
 *   LD_LIBRARY_PATH=".:./BuzzMachineLoader/.libs" ./bmltest_process ~/buzztrax/lib/Gear/Effects/Jeskola\ NiNjA\ dElaY.dll input.raw output1.raw
 *   LD_LIBRARY_PATH=".:./BuzzMachineLoader/.libs" ./bmltest_process ~/buzztrax/lib/Gear/Jeskola_Ninja.so input.raw output1.raw
 *
 * aplay -fS16_LE -r44100 input.raw
 * aplay -fS16_LE -r44100 output1.raw
 *
 * plot [0:] [-35000:35000] 'output1.raw' binary format="%short" using 1 with lines
 *
 * create test-data:
 * gst-launch-1.0 filesrc location=/usr/share/sounds/info.wav ! decodebin ! filesink location=input1.raw
 * dd count=10 if=/dev/zero of=input2.raw
 */

#include "config.h"

#define _ISOC99_SOURCE          /* for isinf() and co. */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <math.h>

//#include <fpu_control.h>

#include "bml/bml.h"

// like MachineInterface.h::MAX_BUFFER_LENGTH
#define BUFFER_SIZE 256

#ifdef USE_DLLWRAPPER
#define bml(a) bmlw_ ## a
#include "bmltest_process.h"
#undef bml
#endif /* USE_DLLWRAPPER */

#define bml(a) bmln_ ## a
#include "bmltest_process.h"
#undef bml

int
main (int argc, char **argv)
{
  int okay = 0;
  setvbuf (stderr, NULL, _IOLBF, 0);
  puts ("main beg");

  if (bml_setup ()) {
    char *lib_name;
    int sl;

#ifdef USE_DLLWRAPPER
    bmlw_set_master_info (120, 4, 44100);
#endif /* USE_DLLWRAPPER */
    bmln_set_master_info (120, 4, 44100);
    puts ("  master info initialized");

    if (argc > 2) {
      lib_name = argv[1];
      sl = strlen (lib_name);
      if (sl > 4) {
        if (!strcasecmp (&lib_name[sl - 4], ".dll")) {
#ifdef USE_DLLWRAPPER
          okay = bmlw_test_process (lib_name, argv[2], argv[3]);
#else
          puts ("no dll emulation on non x86 platforms");
#endif /* USE_DLLWRAPPER */
        } else {
          okay = bmln_test_process (lib_name, argv[2], argv[3]);
        }
      }
    } else
      puts ("    not enough args!");
    bml_finalize ();
  }

  puts ("main end");
  return okay ? 0 : 1;
}
