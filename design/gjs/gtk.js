const Gtk = imports.gi.Gtk;

print('Hello GObject');

let window = bt_get_window();
//print(window.prototype.getName());

let box = new Gtk.VBox({spacing: 12, border_width: 24});
box.add(new Gtk.Label({label: '---- Hello ----'}))
box.add(new Gtk.Label({label:'<big><b>Seed</b></big>', use_markup: true }))
window.add(box);
