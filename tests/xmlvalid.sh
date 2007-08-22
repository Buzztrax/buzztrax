#!/bin/sh
# $Id: xmlvalid.sh,v 1.5 2007-08-22 06:22:50 ensonic Exp $
# test validity of xml files

if [ -z $srcdir ]; then
  srcdir=.
fi

XML_OPTS="--noout --nonet"

# do wellformed checking
xmllint $XML_OPTS $srcdir/songs/*.xml
if [ $? -ne 0 ]; then exit 1; fi

# check the schema itself
xmllint $XML_OPTS $srcdir/../docs/buzztard.xsd
if [ $? -ne 0 ]; then exit 1; fi

# do schema validation
#xmllint $XML_OPTS --schema ../docs/buzztard.xsd ./songs/*.xml
#if [ $? -ne 0 ]; then exit 1; fi

# test the docs
xmllint $XML_OPTS --xinclude --postvalid $srcdir/../docs/help/bt-edit/C/bt-edit.xml
if [ $? -ne 0 ]; then exit 1; fi

