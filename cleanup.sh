#!/bin/sh

#
# cleanup.sh
#
# Script to cleanup some files which making
# conflicts with svn (like *.po files
#
# Version 1.0
# 
# Author: Thomas Wabner
#

#####
#
# clean up all po file under the current directory
#
#####

find . -type f -name "*.po" -exec rm -f {} \;

