#!/bin/sh
# $Id: bt-edit-cli.sh,v 1.1 2007-07-11 20:41:31 ensonic Exp $
# run bt-edit commandline options

. ./bt-cfg.sh

BINARY=../src/ui/edit/bt-edit

# test the output a little
libtool --mode=execute $BINARY --help | grep >/dev/null -- "--help-bt-core"
if [ $? -ne 0 ]; then exit 1; fi

libtool --mode=execute $BINARY --version | grep >/dev/null -- "bt-edit from buzztard"
if [ $? -ne 0 ]; then exit 1; fi

# other tests would launch the UI :/
