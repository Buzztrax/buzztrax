# http://live.gnome.org/GObjectIntrospection
# cd /home/ensonic/projects/pybank
# PYTHONPATH=$PWD python ~/projects/buzztard/buzztard/design/bindings/play.py
#

import bank
import BuzztardCore

#BuzztardCore.init(0,None)

app=BuzztardCore.Application()
# bin=app.get("bin")
song=BuzztardCore.Song(app)
songio=BuzztardCore.SongIO("/home/ensonic/buzztard/share/buzztard/songs/melo3.xml")
songio.load(song)

