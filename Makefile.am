## $Id: Makefile.am,v 1.6 2004-07-21 13:21:13 ensonic Exp $

SUBDIRS = src tests docs

if DEBUG
AM_CFLAGS = -pipe -O0 -Werror -g
else
AM_CFLAGS = -pipe -O3
endif

todo::
	find . -name "*.c" -exec grep -Hno "@todo .*" {} \; | sed "s/@todo/error:/"
