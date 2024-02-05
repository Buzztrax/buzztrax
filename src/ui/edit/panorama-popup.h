/* GNOME Volume Applet
 * Copyright (C) 2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *               2006 Stefan Kost <ensonic@users.sf.net>
 *
 * gtkvolumepopup.h: floating window containing volume widgets
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BT_PANORAMA_POPUP_H
#define BT_PANORAMA_POPUP_H

#include <glib.h>
#include <gtk/gtk.h>
#include <gst/gst.h>

G_BEGIN_DECLS

/**
 * BtPanoramaPopup:
 *
 * a volume popup widget
 */
G_DECLARE_FINAL_TYPE(BtPanoramaPopup, bt_panorama_popup, BT, PANORAMA_POPUP, GtkPopover);

#define BT_TYPE_PANORAMA_POPUP          (bt_panorama_popup_get_type ())

GtkWidget *bt_panorama_popup_new(GtkAdjustment *adj);

void bt_panorama_popup_show(BtPanoramaPopup *self);
void bt_panorama_popup_hide(BtPanoramaPopup *self);

G_END_DECLS

#endif // BT_PANORAMA_POPUP_H
