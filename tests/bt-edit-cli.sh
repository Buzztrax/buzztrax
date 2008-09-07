#!/bin/sh
# $Id$
# run buzztard-edit commandline options

. ./bt-cfg.sh

# test the output a little
libtool --mode=execute $BUZZTARD_EDIT --help | grep >/dev/null -- "--help-bt-core"
if [ $? -ne 0 ]; then exit 1; fi

libtool --mode=execute $BUZZTARD_EDIT --version | grep >/dev/null -- "buzztard-edit from buzztard"
if [ $? -ne 0 ]; then exit 1; fi

# other tests would launch the UI :/
