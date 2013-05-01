// http://live.gnome.org/GObjectIntrospection
// GI_TYPELIB_PATH=/home/ensonic/buzztrax/lib/girepository gjs-console ~/projects/buzztrax/buzztrax/design/bindings/play.js
// GI_TYPELIB_PATH=/home/ensonic/buzztrax/lib/girepository LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/usr/lib/xulrunner-1.9.0.9" gjs-console ~/projects/buzztrax/buzztrax/design/bindings/play.js

const GLib = imports.gi.GLib;
const BuzztraxCore = imports.gi.BuzztraxCore;
const Mainloop = imports.mainloop;

GLib.thread_init(null);
BuzztraxCore.init(0,null);

let app = new BuzztraxCore.Application();
let song = new BuzztraxCore.Song({app:app});
let songio = BuzztraxCore.SongIO.make("/home/ensonic/buzztrax/share/buzztrax/songs/melo3.xml");
songio.load(song);
song.play();

song.connect("notify::is-playing", function(song) {
                              if (!song.is_playing) {
                                  log("stop playing");
                                  Mainloop.quit('playing');
                              }
                              else {
                                  log("start playing");
                              }
                          });

// this is a hack, we need to properly wait for eos
//Mainloop.timeout_add(10000,
//                       function() {
//                           Mainloop.quit('playing');
//                           return false;
//                       });
Mainloop.run('playing');
