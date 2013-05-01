# http://live.gnome.org/GObjectIntrospection
# cd /home/ensonic/projects/pybank
# GI_TYPELIB_PATH=/home/ensonic/buzztrax/lib/girepository PYTHONPATH=$PWD python ~/projects/buzztrax/buzztrax/design/bindings/play.py
#

import bank
import GLib
import BuzztraxCore

GLib.thread_init(None);
BuzztraxCore.init(0,None)

app=BuzztraxCore.Application()
# bin=app.get("bin")
#song=BuzztraxCore.Song(app=app)
song=BuzztraxCore.Song(app)
songio=BuzztraxCore.SongIO.make("/home/ensonic/buzztrax/share/buzztrax/songs/melo3.xml")
songio.load(song)

