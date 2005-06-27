#!/bin/sh

if [ -e docs/reference/bt-core/bt-core-undocumented.txt ]; then
  for file in ../docs/reference/*/*-undocumented.txt; do
    name=`basename $file ../docs/reference/`
    echo "$name :" `head -n1 $file | cut -d' ' -f1`
    if [ `head -n1 $file | cut -d% -f1` -ne "100" ]; then
      exit 1
    fi
  done
fi

exit 0
