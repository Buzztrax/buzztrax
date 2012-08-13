## common makefile targets

.PHONY: todo stats splint help

# make todo            -- generate a list of TODO items in the form of gcc error/warnings/notes"
todo::
	@find . \( -name "*.c" -o -name "*.h" \) -exec grep -Hno "FIXME([a-z]*): .*" {} \; | sed "s/FIXME([a-z]*)/error:/"
	@find . \( -name "*.c" -o -name "*.h" \) -exec grep -Hno "TODO([a-z]*): .*" {} \; | sed "s/TODO([a-z]*)/warning:/"
	@find . \( -name "*.c" -o -name "*.h" \) -exec grep -Hno "IDEA([a-z]*): .*" {} \; | sed "s/IDEA([a-z]*)/note:/"

# make stats           -- run source code stats
stats:: ctags
	@echo "files by byte size:"
	@find . \( -name "*.c" -o -name "*.h" \) -printf "%s %p\n" | sort -rn
	@echo
	@echo "files by line size:"
	@find . \( -name "*.c" -o -name "*.h" \) -exec wc -l {} \; | sort -rn
	@echo
	@echo "files by tag entries:"
	@for file in *.c; do size=`grep $${file} tags | wc -l`;echo $${size} $${file}; done | sort -rn

## need all -I -D flags
## ideally we use a gcc wrapper to dump the cpp flags for each file we compile
## and use that, that'd be also useful for clang-check
# make splint          -- check sources using splint
splint::
	@globs=`find src -name "*.c" -printf "%h/*.c %h/*.h\n" | sort | uniq | xargs echo`; \
	iflags=`grep -o -e '\-I[a-zA-Z0-9/.-]* ' Makefile | xargs echo`; \
	splint +posixlib -weak $$iflags $$globs;

# make help            -- this list
help::
	@grep -e "^# make .*" Makefile | cut -c3-

