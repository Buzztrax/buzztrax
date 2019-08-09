# Buzztrax

[![Gitter](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/Buzztrax/buzztrax?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

## quick info
Please turn you browser to http://www.buzztrax.org to learn what this project
is about. Buzztrax is free software and distributed under the LGPL.

## build status
We are running continuous integration on travis-ci, get coverage from codecov
and have the codebase checked by coverity:

[![Build Status](https://travis-ci.org/Buzztrax/buzztrax.svg?branch=master)](https://travis-ci.org/Buzztrax/buzztrax/builds)
[![Test Coverage](https://codecov.io/gh/Buzztrax/buzztrax/branch/master/graph/badge.svg)](https://codecov.io/gh/Buzztrax/buzztrax)
[![Coverity Defects](https://scan.coverity.com/projects/533/badge.svg)](https://scan.coverity.com/projects/buzztrax)

## intro
Buzztrax is a music composer similar to tracker applications. It is roughly
modelled after the windows only, closed source application called Buzz.

## requirements
* gstreamer >= 1.2 and its plugins (gstreamer, gst-plugins-base and gst-plugins-good).
* glib, gsf and libxml2 for the core libraries.
* clutter-gtk and gtk+ for the UI

## optional packages
* gst-plugins-ugly: for the use of mp3 recording
* gst-plugins-bad: extra audio effects
* gudev and libasound: for interaction controller support
* orc: for plugin acceleration
* fluidsynth: to build a relates gstreamer wrapper
* check: for unit tests

## building from git
To build use autogen.sh instead of configure. This accept the same options like
configure. Later one can use autoregen.sh to rerun the bootstrapping.
To build from git one needs to have gtk-doc and cvs (for autopoint from gettext)
installed.

## directories
* docs : design ideas and API reference
* po : gettext i18n catalogs
* src : the project sources
  * ui
    * cmd : buzztrax tool for the command line
    * edit : buzztrax editor (gtk based ui)
  * lib
    * core : logic, e.g. connections framework, file i/o, ...
    * ic : interaction controller
* tests : unit tests (same directory structure as src)

## installing locally
Use following options for ./autogen.sh or ./configure

    --prefix=$HOME/buzztrax
    --with-gconf-source=xml::/home/ensonic/.gconf/
    --with-desktop-dir=/home/ensonic/.gnome2/vfolders/

when installing the package to e.g. $HOME/buzztrax you ned to set a few
environment variables. To use the apps these variables are enough:

    # libraries:
    export LD_LIBRARY_PATH=$HOME/buzztrax/lib:$LD_LIBRARY_PATH
    # gstreamer
    export GST_PLUGIN_PATH="$HOME/buzztrax/lib/gstreamer-1.0"
    # mime-database & icon-themes:
    export XDG_DATA_DIRS="$XDG_DATA_DIRS:$HOME/buzztrax/share"
    update-mime-database $HOME/buzztrax/share/mime/

Likewise for the man-pages to be found:

    export MANPATH=\$MANPATH:$prefix/share/man

For developers:

    # see buzztrax help files in devhelp:
    export DEVHELP_SEARCH_PATH="$DEVHELP_SEARCH_PATH:$HOME/buzztrax/share/gtk-doc/html"
    # compile against buzztrax libs using pkg-config:
    export PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$HOME/buzztrax/lib/pkgconfig"
    #
    export GI_TYPELIB_PATH="\$GI_TYPELIB_PATH:$prefix/lib/girepository"


## installing in /usr/local
The default value for the --prefix configure option is /usr/local. Also in that
case the variables mentioned in the last example need to be exported.

## running the apps

    cd $HOME/buzztrax/bin
    ./buzztrax-cmd --command=info --input-file=../share/buzztrax/songs/melo1.xml
    ./buzztrax-cmd --command=play --input-file=../share/buzztrax/songs/melo1.xml
    ./buzztrax-cmd --command=encode --input-file=../share/buzztrax/songs/melo1.xml --output-file=./melo1.ogg
    ./buzztrax-edit --command=load --input-file=../share/buzztrax/songs/melo1.xml

## running unit tests
run all the tests:

    make check

select tests to run:

    BT_CHECKS="test_bt_edit_app*" make bt_edit.check


