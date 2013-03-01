const Gtk = imports.gi.Gtk;

print('Hello GObject');

print('bt_get_five() = ' + bt_get_five());

let window = bt_get_window();
print(window);
for (property in window) {
  print(' ' + property + ': ' + window[property]);
}
print(window.__proto__);

let box = new Gtk.VBox({spacing: 12, border_width: 24});
box.add(new Gtk.Label({label: '---- Hello ----'}))
box.add(new Gtk.Label({label:'<big><b>Gjs</b></big>', use_markup: true }))
window.add(box);
