// $Id: volume-popup.h,v 1.1 2006-05-18 21:20:33 ensonic Exp $
/*
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef BT_VOLUME_POPUP_H
#define BT_VOLUME_POPUP_H

#include <glib.h>

#include <gtk/gtkbutton.h>
#include <gtk/gtkrange.h>
#include <gtk/gtkwindow.h>
#include <gst/gst.h>

G_BEGIN_DECLS

#define BT_TYPE_VOLUME_POPUP          (bt_volume_popup_get_type ())
#define BT_VOLUME_POPUP(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_VOLUME_POPUP, BtVolumePopup))
#define BT_IS_VOLUME_POPUP(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_VOLUME_POPUP))
#define BT_VOLUME_POPUP_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass),  BT_TYPE_VOLUME_POPUP, BtVolumePopupClass))
#define BT_IS_VOLUME_POPUP_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass),  BT_TYPE_VOLUME_POPUP))

typedef struct _BtVolumePopup {
  GtkWindow parent;

  /* us */
  GtkRange *scale;
  GtkButton *plus, *minus;

  /* timeout for buttons */
  guint timeout;
  /* for +/- buttons */
  gint direction;

} BtVolumePopup;

typedef struct _BtVolumePopupClass {
  GtkWindowClass klass;
} BtVolumePopupClass;

GtkWidget *bt_volume_popup_new(GtkAdjustment *adj);

void bt_volume_popup_show(BtVolumePopup *self);
void bt_volume_popup_hide(BtVolumePopup *self);

GType bt_volume_popup_get_type(void) G_GNUC_CONST;

G_END_DECLS

#endif // BT_VOLUME_POPUP_H
