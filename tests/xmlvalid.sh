#!/bin/sh
# $Id: xmlvalid.sh,v 1.2 2005-01-05 11:24:57 ensonic Exp $
# test validity of xml files

# do wellformed checking
xmllint --noout ./songs/*.xml
if [ $? -ne 0 ]; then exit 1; fi

# check the schema itself
xmllint --noout ../docs/buzztard.xsd

# do schema validation
#xmllint --noout --schema ../docs/buzztard.xsd ./songs/*.xml
#if [ $? -ne 0 ]; then exit 1; fi
