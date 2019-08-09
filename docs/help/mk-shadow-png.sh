#!/bin/bash

infile="$1"
outfile="$2"
dir=$(dirname $0)

if [ -e "$infile" ]; then \
	  pngtopnm "$infile" | "$dir/mk-shadow.sh" | pnmtopng >"$outfile";
fi;
