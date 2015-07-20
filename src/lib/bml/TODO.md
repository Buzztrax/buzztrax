# general
* rename public API in BuzzMachineLoader.h
  BuzzMachine -> BMLMachine
  BuzzMachineProperty -> BMLMachineProperty
  ...

* charsets, should we convert all strings to UTF-8 in here already?

# more tools
## bmldump_src_skel
dump a source skelleton for rewriting a machine

## debugging
write a gdb script that enhances backtraces like:https://github.com/shinh/maloader/blob/master/gdb_maloader.py
https://github.com/shinh/maloader/blob/master/gdb_maloader.py

# testing
modify tests/testmachine.sh
eventually improve scrolling:
  http://www.litotes.demon.co.uk/example_scripts/tableScroll.html
result should go to a given directory (use date-time when not provided)

# unemulated used libs

grep -ho "Win32 LoadLibrary failed to load.*" testmachine/*.txt* | sort | uniq -c | sort -g

      3 wine/module: Win32 LoadLibrary failed to load: COMCTL32.dll
     28 wine/module: Win32 LoadLibrary failed to load: comdlg32.dll
      1 wine/module: Win32 LoadLibrary failed to load: DSOUND.dll
   2588 wine/module: Win32 LoadLibrary failed to load: GDI32.dll
  81803 wine/module: Win32 LoadLibrary failed to load: KERNEL32.dll
    147 wine/module: Win32 LoadLibrary failed to load: MSVCRT.dll
    254 wine/module: Win32 LoadLibrary failed to load: ole32.dll
     30 wine/module: Win32 LoadLibrary failed to load: OLEAUT32.dll
     20 wine/module: Win32 LoadLibrary failed to load: oledlg.dll
     18 wine/module: Win32 LoadLibrary failed to load: OLEPRO32.DLL
      2 wine/module: Win32 LoadLibrary failed to load: SHELL32.dll
      1 wine/module: Win32 LoadLibrary failed to load: SHLWAPI.dll
   4775 wine/module: Win32 LoadLibrary failed to load: USER32.dll
     44 wine/module: Win32 LoadLibrary failed to load: WINMM.dll
     63 wine/module: Win32 LoadLibrary failed to load: WINSPOOL.DRV

* ole stuff could be machines using auxbus
* DSOUND.dll
  Geonik's Dx Input.dll
* SHLWAPI.dll
  BuzzInAMovie.dll
* SHELL32.dll
  Rout VST Plugin Loader.dll
  mimo's miXo.dll

# missing window function entries

unk_??0_Lockit@std@@QAE@XZ
unk_??1_Lockit@std@@QAE@XZ

unk_joyGetDevCapsA
unk_joyGetNumDevs
unk_joyGetPos

unk_waveInOpen

unk_midiOutGetNumDevs
unk_midiOutOpen

MFC42.DLL (check MFC42.DEF file)
  unk_MFC42.DLL:823 : ??2@YAPAXI@Z
  unk_MFC42.DLL:1176 : ?AfxGetThreadState@@YGPAV_AFX_THREAD_STATE@@XZ
  unk_MFC42.DLL:1243 : ?AfxSetModuleState@@YGPAVAFX_MODULE_STATE@@PAV1@@Z

user32.dll
  unk_CreateDialogParamA : http://msdn.microsoft.com/en-us/library/ms645445(VS.85).aspx
  unk_GetWindowTextA : http://msdn.microsoft.com/en-us/library/ms633520(VS.85).aspx
  unk_LoadIconA : http://msdn.microsoft.com/en-us/library/ms648072.aspx
  unk_SendMessageA : http://msdn.microsoft.com/en-us/library/ms644950.aspx
  unk_SetWindowPos : http://msdn.microsoft.com/en-us/library/ms633545.aspx
  unk_VkKeyScanA : http://msdn.microsoft.com/en-us/library/ms646329.aspx

Microsoft DirectShow 9.0
  unk_DMOEnum : http://msdn.microsoft.com/en-us/library/ms783373(VS.85).aspx

Kernel32.dll
  unk_FlsAlloc : http://msdn.microsoft.com/en-us/library/ms682664(VS.85).aspx
  unk_FlsFree
  unk_FlsSetValue
  unk_ResumeThread : http://msdn.microsoft.com/en-us/library/ms685086(VS.85).aspx

Gdi32.lib
  unk_GetStockObject : http://msdn.microsoft.com/en-us/library/ms533223.aspx

= list missing machine for songs =
for file in *.bmx; do ~/buzztrax/bin/bt-cmd 2>/dev/null --command=info --input-file=$file | grep "  bml-"; done | sort | uniq -c | sort -n

# list songs that have no missing machines
for file in *.bmx; do ~/buzztrax/bin/bt-cmd 2>/dev/null --command=info --input-file=$file | grep -q "  bml-" || echo $file; done

= missing features =
* getting access to Patterns/Sequence

# getting better diagnostics on failure
http://www.eptacom.net/pubblicazioni/pub_eng/except.html
http://www.eptacom.net/pubblicazioni/pub_eng/assert.html
http://www.codeproject.com/KB/threads/StackWalker.aspx

## tools
use mingw-binutils to dump win32 binaries
/usr/local/i386-mingw32/bin/objdump -t MSVCRT.DLL
/usr/i586-mingw32msvc/bin/objdump ...


// when using fastcall
BuzzMachineLoader.cpp:83: undefined reference to `@_Z8DSP_Initi@4'
// normal symbol
BuzzMachineLoader.cpp:83: undefined reference to `DSP_Init(int)'
// name in lib
> i386-mingw32-nm dsplib.lib | grep DSP_Init
00000000 T ?DSP_Init@@YIXH@Z
00000000 I __imp_?DSP_Init@@YIXH@Z

http://sourceforge.net/project/shownotes.php?release_id=90402
However, the --enable-stdcall-fixup feature (which is on by default) has not yet
been extended to fastcall symbols. That is, an undefined symbol @foo@n will *not*
automatically be resolved to a defined symbol foo.

Maybe a own .def can help:
http://www.digitalmars.com/ctg/win32programming.html

what about using winegcc - e.g.:
winegcc -shared XXX.dll.spec -mnocygwin -o XXX.dll.so abc.o def.o -lole32 -lwinmm -lpthread -luuid

# inspecting the dlls
winedump -x $HOME/buzztrax/lib/Gear/Effects/cheapo\ amp.dll
./objconv -fgasm $HOME/buzztrax/lib/Gear/Effects/cheapo\ amp.dll
/usr/bin/i586-mingw32msvc-objdump -x $HOME/buzztrax/lib/Gear/Effects/cheapo\ amp.dll
/usr/bin/i586-mingw32msvc-objdump -d $HOME/buzztrax/lib/Gear/Effects/cheapo\ amp.dll

# debugging with gcc
BML_DEBUG=255 gdb --args ./bmlhost bml.sock
and from gdb: run
in 2nd terminal:
BMLIPC_DEBUG=1 BML_DEBUG=255 /home/ensonic/projects/buzztrax/buzztrax/bmltest_info $HOME/buzztrax/lib/Gear/Effects/cheapo\ amp.dll
upon crash back in gdb: bt, disas <addr>,<addr>+100

# missing dlls
wget kegel.com/wine/winetricks
sh winetricks mfc42

# x86_64 bit support for native plugins
## IPC based (this is what we're currently trying)
- ipc links
  - http://www.tldp.org/LDP/lpg/node7.html
  - http://openbook.galileocomputing.de/linux_unix_programmierung/Kap11-017.htm#RxxKap11017040003961F026100
  - http://beej.us/guide/bgipc/output/html/singlepage/bgipc.html#unixsock
  - http://troydhanson.github.io/misc/Unix_domain_sockets.html
- will we launch 1 helper per using process or 1 helper per machine
  - trying 1 helper per process right now, downside:
    - we have no multithreading for the running machines
    - one dll would kill all other already running plugins
- we need to handle callback support which is needed for wavetables
- we need to SIGPIPE handle cases (e.g. bad dll kills the bmlhost)

## calling into 32bit code from 64bit code
- with a specific trampoline, we can call into 32bit code:
  http://blog.oxff.net/#xwuqt3dgysac6jqbro5q
  if(!allocateLowStack(m_stack32)) {
    throw;
  }
  /* mov r8, rsp */
  * (uint16_t *) &m_trampoline[0] = 0x8949;
  m_trampoline[2] = 0xe0;
  /* mov rsp, ... */
  * (uint16_t *) &m_trampoline[3] = 0xbc48;
  * (uint64_t *) &m_trampoline[5] = (uint64_t) m_stack32;
  /* call far [rip+4] */
  * (uint16_t *) &m_trampoline[13] = 0x1dff;
  * (uint32_t *) &m_trampoline[15] = 4;
  /* mov rsp, r8 */
  * (uint16_t *) &m_trampoline[19] = 0x894c;
  m_trampoline[21] = 0xc4;
  /* ret */
  m_trampoline[22] = 0xc3;
  /* m16:32 call far destination */
  * (uint32_t *) &m_trampoline[23] =
          (uint32_t) (uint64_t) m_32bitCode;
  * (uint16_t *) &m_trampoline[27] = codeSelector;


# test single plugins
G_SLICE=always-malloc G_DEBUG=gc-friendly GLIBCPP_FORCE_NEW=1 GLIBCXX_FORCE_NEW=1 valgrind --tool=memcheck --leak-check=full --leak-resolution=high --trace-children=yes --num-callers=20 -v ../src/.libs/bmltest_info ~/buzztrax/lib/Gear-real/Effects/Static\ Duafilt\ II.dll
../src/.libs/bmltest_info ~/buzztrax/lib/Gear-real/Effects/Static\ Duafilt\ II.dll


# what windows dll to copy and which not
## don't (we need to emulate)
kernel32.dll ntdll.dll
## seems safe
msvcp50.dll  msvcp60.dll MFC42.DLL MSVCRT.DLL


# buzz emulation
## mdk machine
* we need to do ::AddInput() and ::DeleteInput to get input-mixing working
 (even though we would always just give one input).
* before we call WorkXXX() we need to call
  MachineInterfaceEx::Input(float *psamples, int numsamples, float amp)

other-sources/buzz-sources/__BUZZ1PLAYER__NOT_FOR_PUBLIC!/CMachine.cpp
* stereo output MIF_DOES_INPUT_MIXING machines will get stereo input
* bm->callbacks->mdkHelper->numChannels
## auxbus machines
* currently all auxbus using machines fail

## machine tests
In gst-buzztrax/src/bml/plugin.c we maintain a black list.

There seems to be several machines failing when calling GetInfo()
tail -n1 *.fail | grep -B1 "GetInfo"

## Generators
Geonik's PrimiFun.dll
  if we don't have MFC42.dll
    Called unk_MFC42.DLL:1176
    Called unk_MFC42.DLL:1243
    Called unk_MFC42.DLL:823
      - its library:ordinal
  if we have MFC42.DLL
    Called unk_GetConsoleMode
m4wii.dll
  if we don't have MFC42.dll
    Called unk_MFC42.DLL:1176
    Called unk_MFC42.DLL:1243
  if we have MFC42.DLL
    Called unk__CxxThrowException
    http://www.winehq.org/pipermail/wine-patches/2002-October/003917.html
    http://source.winehq.org/source/dlls/msvcrt/cppexcept.c
    http://source.winehq.org/source/dlls/msvcrt/cppexcept.h
    If we wrap this localy, the unk__ is gone (empty impl), but it segfaults.
    Maybe we need a full impl.
mimo's MidiGen.dll
  win32.c: TLS_COUNT is not enough
Jeskola Bass 2.dll
  buzzmachinecallbackspre12.cpp:105:BuzzMachineCallbacksPre12::MidiOut (dev=1,data=136987296)
Jeskola Tracker.dll
  if we don't have MFC42.dll
    Called unk_MFC42.DLL:823
  if we have MFC42.DLL
    Called unk__CxxThrowException
Ruff SPECCY II.dll
HD Percussive_FM.dll
  crash after ?

Aeguelles TB4004
  noisy distortion

## Effects
Arguelles Degrader
      4.6823: :0:: f:\buzzmachineloader.cpp:442:bm_init   no CMachineDataInput
  vex x86->IR: unhandled instruction bytes: 0xDD 0xFE 0x32 0x0
  ==14474== Invalid write of size 1
  ==14474==    at 0xFEDD6398: ???
  ==14474==  Address 0xf5c905d2 is not stack'd, malloc'd or (recently) free'd
cheapo amp
  seems to call some method on CMachine
     17.4767: :0:: f:\buzzmachinecallbacks.cpp:163:BuzzMachineCallbacks::GetThisMachine ()=0x04685A80
  ==14386== Jump to the invalid address stated on the next line
  ==14386==    at 0x0: ???
  ==14386==  Address 0x0 is not stack'd, malloc'd or (recently) free'd

Jeskola XDelay (causes distortions)
Jeskola Raverb (causes distortions)
  BuzzMachineCallbacks::GetEnvSize (wave=-96185375,env=0)
  buzzmachinecallbackspre12.cpp:112:BuzzMachineCallbacksPre12::GetOscillatorTable (waveform=-128840458)
Jeskola Stereo Reverb (sometimes silences the whole song, NAN?)
Jeskola Wave Shaper.dll
  if we don't have MFC42.DLL
    Called unk_MFC42.DLL:1176
    Called unk_MFC42.DLL:1243
  if we have MFC42.DLL
    Entering DllMain(DLL_PROCESS_ATTACH) for /home/ensonic/buzztrax/lib/win32/mfc42.dll
    Segmentation fault
Shaman Chorus.dll
  #3  0xb7fb48e8 in LookupExternalByName (library=0xb7fbce28 "kernel32.dll", name=0x37f7f9c2 <Address 0x37f7f9c2 out of bounds>) at win32.c:5650
  #4  0xb7fb4abe in expGetProcAddress (mod=288, name=0x37f7f9c2 <Address 0x37f7f9c2 out of bounds>) at win32.c:2374
  #5  0xb7f843ba in ?? ()
  -> mangled/corrupted exports name table
     winedump -x ~/projects/buzztrax/win32/Gear/Effects/Shaman\ Chorus.dll
     objconv -d Effects/Shaman\ Chorus.dll
     objconv -v2 -ew2035 -fgasm Effects/Shaman\ Chorus.dll
Sonic Verb (seems to work now)
  buzzmachinecallbacks.cpp:199:BuzzMachineCallbacks::GetEnvPoint (wave=-1,env=-1,i=-1076278224,&x,&y,&flags)
  buzzmachinecallbacks.cpp:276:BuzzMachineCallbacks::SetSequenceData (row=-1217504548,ppat=B7A5B3C4)
Dolby Prologic Echo
  Param Direction : val [1 ... d=4096/n=0 ... 255], the def-val for an enum, can't be outside the range

Ynzn's Remote Compressor.dll
Ynzn's Remote Gate.dll
  both crash (problem with name? - no we have others with "'")
PSI Corp's LFO Filter.dll
Repeater.dll
RnZnAnFnCnR VST Effect Adapter.dll
Rnznanf Vst Effect Adapter.dll
Rout VST Plugin Loader.dll
Shaman Chorus.dll
WhiteNoise AuxReturn.dll
  crash

Zu*.dll
  #0  0xb7fce4e2 in exp_EH_prolog () from /home/ensonic/buzztrax/lib/libbml.so.0
  #1  0xb7c9e38a in ?? ()
  #2  0x1001a898 in ?? ()
  #3  0xb7fafad4 in bmlw_open (bm_file_name=0xbfffeffe "/home/ensonic/buzztrax/lib/Gear-real/Effects/Zu Parametric EQ.dll") at bml.c:207
  #4  0x080490cb in bmlw_test_info (libpath=0xbfffeffe "/home/ensonic/buzztrax/lib/Gear-real/Effects/Zu Parametric EQ.dll") at bmltest_info.h:34
  #5  0x0804ab15 in main (argc=2, argv=0xbfffed64) at bmltest_info.c:96

