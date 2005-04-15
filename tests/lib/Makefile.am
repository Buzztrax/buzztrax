## $Id: Makefile.am,v 1.3 2005-04-15 17:08:16 ensonic Exp $

SUBDIRS = core

.PHONY: splint 

splint::
	for dir in $(SUBDIRS); do $(MAKE) -C $$dir splint ; done
