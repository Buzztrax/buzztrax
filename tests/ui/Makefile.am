## $Id: Makefile.am,v 1.4 2005-04-15 17:08:28 ensonic Exp $

SUBDIRS = cmd edit

.PHONY: splint 

splint::
	for dir in $(SUBDIRS); do $(MAKE) -C $$dir splint ; done
