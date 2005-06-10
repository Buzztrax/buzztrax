#!/bin/sh
# $Id: povalid.sh,v 1.1 2005-06-10 09:28:15 ensonic Exp $
# test validity of po i18n files

res=`cd po;intltool-update -m 2>&1 | wc -l;cd ..`

exit $res
