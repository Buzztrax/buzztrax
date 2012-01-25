## common makefile targets

.PHONY: todo stats help

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

# make help            -- this list
help::
	@grep -e "^# make .*" Makefile | cut -c3-

