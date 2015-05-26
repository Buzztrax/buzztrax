#!/bin/bash
# Buzztrax
# Copyright (C) 2005 Buzztrax team <buzztrax-devel@buzztrax.org>
#
# test validity of xml files
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

if [ -z $srcdir ]; then
  srcdir=.
fi

XML_OPTS="--noout --nonet"

E_SONGS="$srcdir/tests/songs/buzz*.xml $srcdir/tests/songs/combi*.xml $srcdir/tests/songs/melo*.xml $srcdir/tests/songs/simple*.xml"

# do wellformed checking
xmllint $XML_OPTS $E_SONGS
if [ $? -ne 0 ]; then exit 1; fi

# check the schema itself
xmllint $XML_OPTS $srcdir/docs/buzztrax.xsd
if [ $? -ne 0 ]; then exit 1; fi

# do schema validation
xmllint $XML_OPTS --schema $srcdir/docs/buzztrax.xsd $E_SONGS
if [ $? -ne 0 ]; then exit 1; fi

# test the docs
xmllint $XML_OPTS --xinclude --postvalid $srcdir/docs/help/bt-edit/C/buzztrax-edit.xml
if [ $? -ne 0 ]; then exit 1; fi

