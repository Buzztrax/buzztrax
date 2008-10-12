# cd /home/ensonic/projects/pybank
# PYTHONPATH=$PWD python ~/projects/buzztard/buzztard/design/python/play.py
#

import bank
import BuzztardCore

app=BuzztardCore.Application()
app.new()
song=BuzztardCore.Song(app)
songio=BuzztardCore.SongIO("/home/ensonic/buzztard/share/buzztard/songs/melo3.xml")
songio.load(song)

