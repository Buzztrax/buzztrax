#!/bin/sh

eval `head -n5 $1 | grep gcc | cut -c4-`
