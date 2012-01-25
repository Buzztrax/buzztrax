## common makefile targets

.PHONY: help todo stats

todo::
	@find . \( -name "*.c" -o -name "*.h" \) -exec grep -Hno "FIXME([a-z]*): .*" {} \; | sed "s/FIXME([a-z]*)/error:/"
	@find . \( -name "*.c" -o -name "*.h" \) -exec grep -Hno "TODO([a-z]*): .*" {} \; | sed "s/TODO([a-z]*)/warning:/"
	@find . \( -name "*.c" -o -name "*.h" \) -exec grep -Hno "IDEA([a-z]*): .*" {} \; | sed "s/IDEA([a-z]*)/note:/"

stats:: tags
	@echo "files by size:"
	@find . \( -name "*.c" -o -name "*.h" \) -printf "%s %p\n" | sort -rn
	@echo
	@echo "files by tags:"
	@for file in *.c; do size=`grep $${file} tags | wc -l`;echo $${size} $${file}; done | sort -rn

help::
	@echo "make todo        -- generate a list of TODO items in the form of gcc error/warnings/notes"
	@echo "make stats       -- run source code stats"

