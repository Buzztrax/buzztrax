#!/bin/sh
# $Id: xmlvalid.sh,v 1.3 2005-04-08 13:35:39 ensonic Exp $
# test validity of xml files

# do wellformed checking
xmllint --noout ./songs/*.xml
if [ $? -ne 0 ]; then exit 1; fi

# check the schema itself
xmllint --noout ../docs/buzztard.xsd
if [ $? -ne 0 ]; then exit 1; fi

# do schema validation
#xmllint --noout --schema ../docs/buzztard.xsd ./songs/*.xml
#if [ $? -ne 0 ]; then exit 1; fi
