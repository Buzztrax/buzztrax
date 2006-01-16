#!/bin/sh
# $Id: xmlvalid.sh,v 1.4 2006-01-16 21:39:26 ensonic Exp $
# test validity of xml files

# do wellformed checking
xmllint --noout $srcdir/songs/*.xml
if [ $? -ne 0 ]; then exit 1; fi

# check the schema itself
xmllint --noout $srcdir/../docs/buzztard.xsd
if [ $? -ne 0 ]; then exit 1; fi

# do schema validation
#xmllint --noout --schema ../docs/buzztard.xsd ./songs/*.xml
#if [ $? -ne 0 ]; then exit 1; fi

