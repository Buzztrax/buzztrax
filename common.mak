## common makefile targets

.PHONY: todo stats splint help

# make todo            -- generate a list of TODO items in the form of gcc error/warnings/notes"
todo::
	@find . \( -name "*.c" -o -name "*.h" \) -exec grep -Hno "FIXME([a-z]*): .*" {} \; | sed "s/FIXME([a-z]*)/error:/"
	@find . \( -name "*.c" -o -name "*.h" \) -exec grep -Hno "TODO([a-z]*): .*" {} \; | sed "s/TODO([a-z]*)/warning:/"
	@find . \( -name "*.c" -o -name "*.h" \) -exec grep -Hno "IDEA([a-z]*): .*" {} \; | sed "s/IDEA([a-z]*)/note:/"

# make stats           -- run source code stats
stats:: tags
	@echo "files by size:"
	@find . \( -name "*.c" -o -name "*.h" \) -printf "%s %p\n" | sort -rn
	@echo
	@echo "files by tags:"
	@for file in *.c; do size=`grep $${file} tags | wc -l`;echo $${size} $${file}; done | sort -rn

## need all -I -D flags
## ideally we use a gcc wrapper to dump the cpp flags for each file we compile
## and use that, that'd be also useful for clang
# make splint          -- check sources using splint
splint::
	@if test -n "*.c"; then \
	  iflags=`grep -o -e '\-I[a-zA-Z0-9/.-]* ' Makefile | xargs echo`; \
	  splint +posixlib -weak $$iflags *.c *.h; \
	fi
	@for dr in $(SUBDIRS); do $(MAKE) -C $$dir splint ; done

# make help            -- this list
help::
	@grep -e "^# make .*" Makefile | cut -c3-

