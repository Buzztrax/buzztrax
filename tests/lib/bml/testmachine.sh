#!/bin/bash
# Buzztrax
# Copyright (C) 2007 Buzztrax team <buzztrax-devel@buzztrax.org>
#
# iterates over the given directory and tries all buzz machines
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public
# License along with this library; if not, see <http://www.gnu.org/licenses/>.

# Test all machines matching the glob. If machines hang, they can be skipped by
# pressing ctrl-c. Run as e.g.
#
# ./testmachine.sh "machines/*.dll"
# ./testmachine.sh "/home/ensonic/buzztrax/lib/Gear-real/Effects/*.dll"
# ./testmachine.sh "/home/ensonic/buzztrax/lib/Gear-real/Generators/*.dll"
# ./testmachine.sh "/home/ensonic/buzztrax/lib/Gear-real/[GE]*/*.dll"
# ./testmachine.sh "/home/ensonic/buzztrax/lib/Gear/*.so"
#
# analyze results
#
# search for unk_XXX -> unknown symbols
#  grep -Hn "unk_" testmachine/*.fail | sort | uniq
#  grep -ho "unk_.*" testmachine/*.fail | sort | uniq -c
# search for FIXME -> unimplemented buzz callback entries
#  grep -Hn "FIXME" testmachine/*.fail
# search for "wine/module: Win32 LoadLibrary failed to load:"
#  grep -ho "wine/module: Win32 LoadLibrary failed to load:.*" testmachine/*.fail | sort -f | uniq -ic
# search for same last line in fail log
#  tail -n1 testmachine/*.fail | grep -B1 "GetInfo()" | grep "==>"
#  tail -n1 testmachine/*.fail | grep -B1 "Entering DllMain(DLL_PROCESS_ATTACH) for /home/ensonic/buzztrax/lib/Gear-real/"  | grep "==>"
#  for file in testmachine/*.txt.fail; do echo $file; grep ":0::" "$file" | tail -n2; done
#
# stats
#  ls -1 testmachine/*.okay | wc -l
#  ls -1 testmachine/*.fail | wc -l
#
# TODO:
# - allow running under valgrind
# - if a machine is m2s (fieldFlags), then channels=2
#

if [ -e ../../bt-cfg.sh ]; then
  . ../../bt-cfg.sh
else
  BUZZTRAX_CMD=buzztrax-cmd
  BUZZTRAX_EDIT=buzztrax-edit
fi

if [ -z "$1" ]; then
  echo "Usage: $0 <directory>";
  exit
fi
if [ -z "$(ls 2>/dev/null $1)" ]; then
    echo "no machines found"
    exit
fi

# initialize

machine_glob="$1";
mkdir -p testmachine
m_okay=0;
m_info=0;
m_fail=0;
trap "sig_segv=1" SIGSEGV
trap "sig_int=1" SIGINT
touch testmachine.failtmp
rm -f testmachine.body.html
touch testmachine.body.html

# create input sound for effects
gst-launch-1.0 >/dev/null 2>&1 audiotestsrc wave="ticks" num-buffers=100 ! audio/x-raw,format=S16LE,channels=1 ! filesink location=input.raw
#dd count=200 if=/dev/zero of=input.raw

# run test loop

for machine in $machine_glob ; do
  #name=`basename "$machine" "$machine_glob"`
  name=`basename "$machine"`
  ext=${name#${name%.*}}
  log_name="./testmachine/$name.txt"
  rm -f "$log_name" "$log_name".okay "$log_name".fail "$log_name".info
  # try to run it
  sig_segv=0
  sig_int=0
  echo >bmltest_info.log "BML_DEBUG=255 $BMLTEST_INFO \"$machine\""
  #env >>bmltest_info.log 2>&1 LD_LIBRARY_PATH="../src/" ../src/bmltest_info "$machine"
  #res=$?
  # this suppresses the output of e.g. "Sementation fault"
  res=`env >>bmltest_info.log 2>&1 BML_DEBUG=255 $BMLTEST_INFO "$machine"; echo $?`
  cat bmltest_info.log | grep >"$log_name" -v "Warning: the specified"
  if [ $sig_int -eq "1" ] ; then res=1; fi
  cat bmltest_info.log | iconv >bmltest_info.tmp -fWINDOWS-1250 -tUTF-8 -c
  fieldCreateTime=`egrep -o "machine created in .*$" bmltest_info.tmp | sed -e 's/machine created in \(.*\) sec$/\1/'`
  fieldInitTime=`egrep -o "machine initialized in .*$" bmltest_info.tmp | sed -e 's/machine initialized in \(.*\) sec$/\1/'`
  fieldShortName=`egrep -o "Short Name: .*$" bmltest_info.tmp | sed -e 's/Short Name: "\(.*\)"$/\1/'`
  fieldAuthor=`egrep -o "Author: .*$" bmltest_info.tmp | sed -e 's/Author: "\(.*\)"$/\1/'`
  fieldType=`egrep -o "^    Type: . -> \"MT_.*$" bmltest_info.tmp | sed -e 's/^\ *Type: . -> "\(.*\)"$/\1/'`
  fieldVersion=`egrep -o "Version: .*$" bmltest_info.tmp | sed -e 's/Version: \(.*\)$/\1/'`
  fieldCommands=`egrep -o "Commands: .*$" bmltest_info.tmp | sed -e 's/Commands: "\(.*\)"$/\1/'`
  fieldFlags=`egrep -o "^    Flags: .*$" bmltest_info.tmp | sed -e 's/^\ *Flags: \(.*\)$/\1/'`
  fieldMinTracks=`egrep -o "MinTracks: .*$" bmltest_info.tmp | sed -e 's/MinTracks: \(.*\)$/\1/'`
  fieldMaxTracks=`egrep -o "MaxTracks: .*$" bmltest_info.tmp | sed -e 's/MaxTracks: \(.*\)$/\1/'`
  fieldInputChannels=`egrep -o "InputChannels: .*$" bmltest_info.tmp | sed -e 's/InputChannels: \(.*\)$/\1/'`
  fieldOutputChannels=`egrep -o "OutputChannels: .*$" bmltest_info.tmp | sed -e 's/OutputChannels: \(.*\)$/\1/'`
  fieldNumGlobalParams=`egrep -o "NumGlobalParams: .*$" bmltest_info.tmp | sed -e 's/NumGlobalParams: \(.*\)$/\1/'`
  fieldNumTrackParams=`egrep -o "NumTrackParams: .*$" bmltest_info.tmp | sed -e 's/NumTrackParams: \(.*\)$/\1/'`
  fieldNumAttributes=`egrep -o "NumAttributes: .*$" bmltest_info.tmp | sed -e 's/NumAttributes: \(.*\)$/\1/'`
  if [ $res -eq "0" ] ; then
    # try to run it again
    sig_segv=0
    sig_int=0
    echo >bmltest_process.log "BML_DEBUG=255 $BMLTEST_PROCESS \"$machine\" input.raw output.raw"
    # this suppresses the output of e.g. "Sementation fault"
    res=`env >>bmltest_process.log 2>&1 BML_DEBUG=255 $BMLTEST_PROCESS "$machine" input.raw output.raw; echo $?`
    cat bmltest_process.log | grep >>"$log_name" -v "Warning: the specified"
    if [ $sig_int -eq "1" ] ; then res=1; fi
    if [ $res -eq "0" ] ; then
      echo "okay : $machine";
      m_okay=$((m_okay+1))
      mv "$log_name" "$log_name".okay
      # convert raw audio to wav
      gst-launch-1.0 >/dev/null 2>&1 filesrc location=output.raw ! audio/x-raw,format=S16LE,channels=$fieldOutputChannels,rate=44100 ! wavenc ! filesink location="testmachine/$name.wav"
      tablecolor="#E0FFE0"
      tableresult="okay"
    else
      echo "info : $machine";
      m_info=$((m_info+1))
      mv "$log_name" "$log_name".info
      tablecolor="#FFF7E0"
      tableresult="info"
    fi
  else
    echo "fail : $machine";
    m_fail=$((m_fail+1))
    mv "$log_name" "$log_name".fail
    tablecolor="#FFE0E0"
    tableresult="fail"
    reason=`tail -n1 "$log_name".fail | strings`;
    echo "$reason :: $name" >>testmachine.failtmp
    touch bmltest_process.log
  fi
  cat bmltest_process.log | iconv >bmltest_process.tmp -fWINDOWS-1250 -tUTF-8 -c
  fieldMaxAmp=`egrep -o "MaxAmp: .*$" bmltest_process.tmp | sed -e 's/MaxAmp: \(.*\)$/\1/'`
  fieldClipped=`egrep -o "Clipped: .*$" bmltest_process.tmp | sed -e 's/Clipped: \(.*\)$/\1/'`
  fieldMathNaN=`egrep -o "some values are nan" bmltest_process.tmp | sed -e 's/some values are \(.*\)$/\1/'`
  fieldMathInf=`egrep -o "some values are inf" bmltest_process.tmp | sed -e 's/some values are \(.*\)$/\1/'`
  fieldMathDen=`egrep -o "some values are denormal" bmltest_process.tmp | sed -e 's/some values are \(.*\)$/\1/'`

  # collect used dlls
  #fieldLibs=`strings "$machine" | grep -i -F "$ext" | grep -vi "$name" | tr "A-Z" "a-z" | sort | uniq`
  fieldLibs=`grep "Loading Microsoft style imports for" "$log_name.$tableresult" | cut -d' ' -f12 | tr "A-Z" "a-z" | sort | uniq`

  cat >>testmachine.body.html <<END_OF_HTML
      <tr bgcolor="$tablecolor">
        <td><a href="./$name.txt.$tableresult">$tableresult</a></td>
        <td>$fieldCreateTime</td>
        <td>$fieldInitTime</td>
        <td>$name</td>
        <td>$fieldShortName</td>
        <td>$fieldAuthor</td>
        <td>$fieldType</td>
        <td>$fieldVersion</td>
        <td>$fieldFlags</td>
        <td>$fieldCommands</td>
        <td>$fieldMinTracks</td>
        <td>$fieldMaxTracks</td>
        <td>$fieldInputChannels</td>
        <td>$fieldOutputChannels</td>
        <td>$fieldNumGlobalParams</td>
        <td>$fieldNumTrackParams</td>
        <td>$fieldNumAttributes</td>
        <td>$fieldLibs</td>
        <td>$fieldMaxAmp</td>
        <td>$fieldClipped</td>
        <td>$fieldMathNaN $fieldMathInf $fieldMathDen</td>
      </tr>
END_OF_HTML
  rm -f bmltest_info.log bmltest_process.log
done

# cleanup and report

rm -f bmltest_info.log bmltest_info.tmp
rm -f bmltest_process.log bmltest_process.tmp
rm -f input.raw output.raw
sort testmachine.failtmp >testmachine/_.fails
rm testmachine.failtmp

cat >testmachine/_.html <<END_OF_HTML
<html>
  <head>
    <script src="../sorttable.js"></script>
    <meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
  </head>
  <body>
    <table class="sortable" border="1" cellspacing="0">
      <tr>
        <th>Res.</th>
        <th>Create sec.</th>
        <th>Init sec.</th>
        <th>Plugin Lib.</th>
        <th>Name</th>
        <th>Author</th>
        <th>Type</th>
        <th>API Ver.</th>
        <th>Flags</th>
        <th>Commands</th>
        <th>Min Trk.</th>
        <th>Max Trk.</th>
        <th>Input Ch.</th>
        <th>Output Ch.</th>
        <th>Global Par.</th>
        <th>Track Par.</th>
        <th>Attr.</th>
        <th>Libs</th>
        <th>Max Amp.</th>
        <th>Clipped</th>
        <th>NaN/Inf/Den</th>
      </tr>
END_OF_HTML
cat >>testmachine/_.html testmachine.body.html
cat >>testmachine/_.html <<END_OF_HTML
    </table>
  </body>
</html>
END_OF_HTML
rm testmachine.body.html

m_all=$((m_fail+m_info+m_okay))
echo "Of $m_all machine(s) $m_okay worked, $m_info did not processed data and $m_fail failed to load."
echo "See testmachine/_.fails and testmachine/_.html for details"

