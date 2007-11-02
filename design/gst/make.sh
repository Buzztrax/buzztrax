#!/bin/sh

eval `head -n8 $1 | grep gcc | cut -c4-`
