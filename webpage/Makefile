# $Id: Makefile,v 1.3 2004-04-30 13:27:01 ensonic Exp $

SUFFIXES=.html .m4

DEPS=$(shell ( find . -maxdepth 1 -name "*.m4" -printf "%f "))
USER=$(shell cat CVS/Root | cut -d: -f3 | cut -d@ -f1)

html:
	for file in $(DEPS); do \
		outfile=`echo "$$file" | sed -e 's/.m4/.html/g'`; \
		echo "m4 -P -I_m4 $$file >$$outfile"; \
		m4 -P -I_m4 "$$file" >"$$outfile"; \
	done

all: html

ftp:
	@scp -r *.html *.css $(USER)@shell.sourceforge.net:/home/groups/b/bu/buzztard/htdocs/
#	@scp -r *.html *.css buzztard.gif favicon.ico gfx $(USER)@shell.sourceforge.net:/home/groups/b/bu/buzztard/htdocs/

