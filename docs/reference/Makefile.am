SUBDIRS = . bt-core bt-ic bt-cmd bt-edit

# this is an ugly hack to help xsltproc finding the files
# it won't be needed with gtk-doc 1.11

all-local: version.entities

version.entities:
	cp ../version.entities ./

clean-local:
	-rm version.entities
