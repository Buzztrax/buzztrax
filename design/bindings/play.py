# http://live.gnome.org/GObjectIntrospection
# cd /home/ensonic/projects/pybank
# GI_TYPELIB_PATH=/home/ensonic/buzztard/lib/girepository PYTHONPATH=$PWD python ~/projects/buzztard/buzztard/design/bindings/play.py
#

import bank
import BuzztardCore

#BuzztardCore.init(0,None)

app=BuzztardCore.Application()
# bin=app.get("bin")
#song=BuzztardCore.Song(app=app)
song=BuzztardCore.Song(app)
songio=BuzztardCore.SongIO.make("/home/ensonic/buzztard/share/buzztard/songs/melo3.xml")
songio.load(song)

