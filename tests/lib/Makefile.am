SUBDIRS = core ic

.PHONY: splint 

splint::
	for dir in $(SUBDIRS); do $(MAKE) -C $$dir splint ; done
