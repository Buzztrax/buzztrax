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

#ifndef BT_VOLUME_POPUP_H
#define BT_VOLUME_POPUP_H

#include <glib.h>
#include <gtk/gtk.h>
#include <gst/gst.h>

G_BEGIN_DECLS

/**
 * BtVolumePopup:
 *
 * a volume popup widget
 */
G_DECLARE_FINAL_TYPE (BtVolumePopup, bt_volume_popup, BT, VOLUME_POPUP, GtkPopover);
#define BT_TYPE_VOLUME_POPUP          (bt_volume_popup_get_type ())

GtkWidget *bt_volume_popup_new(GtkAdjustment *adj);

void bt_volume_popup_show(BtVolumePopup *self, GtkWidget* parent);
void bt_volume_popup_hide(BtVolumePopup *self);

GType bt_volume_popup_get_type(void) G_GNUC_CONST;

G_END_DECLS

#endif // BT_VOLUME_POPUP_H
