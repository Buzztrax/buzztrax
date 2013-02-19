local lgi = require 'lgi'
local Gtk = lgi.require('Gtk')

print "Hello GObject"

local window = Gtk.Window(bt_get_window())
local box = Gtk.VBox { spacing = 12, border_width = 24 }
box:add(Gtk.Label { label = '---- Hello ----' })
box:add(Gtk.Label { label = '<big><b>Lua</b></big>', use_markup = true })
window:add(box)
