// http://live.gnome.org/GObjectIntrospection
// GI_TYPELIB_PATH=/home/ensonic/buzztard/lib/girepository gjs-console ~/projects/buzztard/buzztard/design/bindings/play.js

const GLib = imports.gi.GLib;
const BuzztardCore = imports.gi.BuzztardCore;

GLib.thread_init(null);
BuzztardCore.init(0,null);

let app = new BuzztardCore.Application();
let song = new BuzztardCore.Song(app);
let songio = new BuzztardCore.SongIO.make({file_name:"/home/ensonic/buzztard/share/buzztard/songs/melo3.xml"});
songio.load(song);
