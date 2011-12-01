#!/bin/sh
# $Id$
# test validity of xml files

if [ -z $srcdir ]; then
  srcdir=.
fi

XML_OPTS="--noout --nonet"

E_SONGS="$srcdir/songs/buzz*.xml $srcdir/songs/combi*.xml $srcdir/songs/melo*.xml $srcdir/songs/simple*.xml"

# do wellformed checking
xmllint $XML_OPTS $E_SONGS
if [ $? -ne 0 ]; then exit 1; fi

# check the schema itself
xmllint $XML_OPTS $srcdir/../docs/buzztard.xsd
if [ $? -ne 0 ]; then exit 1; fi

# do schema validation
xmllint $XML_OPTS --schema $srcdir/../docs/buzztard.xsd $E_SONGS
if [ $? -ne 0 ]; then exit 1; fi

# test the docs
xmllint $XML_OPTS --xinclude --postvalid $srcdir/../docs/help/bt-edit/C/buzztard-edit.xml
if [ $? -ne 0 ]; then exit 1; fi

