#!/bin/sh
# $Id: xmlvalid.sh,v 1.1 2005-01-05 07:19:01 ensonic Exp $
# test validity of xml files

xmllint --noout ./songs/*.xml
if [ $? -ne 0 ]; then exit 1; fi
