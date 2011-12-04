/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

/*
 * NOTE this widget is considered too specialized/little-used for
 * GTK+, and will in the future be moved to some other package.  If
 * your application needs this widget, feel free to use it, as the
 * widget does work and is useful in some applications; it's just not
 * of general interest. However, we are not accepting new features for
 * the widget, and it will eventually move out of the GTK+
 * distribution.
 */

#ifndef __BT_VRULER_H__
#define __BT_VRULER_H__

#include <gtk/gtk.h>
#include "ruler.h"

G_BEGIN_DECLS

#define BT_TYPE_VRULER            (bt_vruler_get_type ())
#define BT_VRULER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_VRULER, BtVRuler))
#define BT_VRULER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_VRULER, BtVRulerClass))
#define BT_IS_VRULER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_VRULER))
#define BT_IS_VRULER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_VRULER))
#define BT_VRULER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_VRULER, BtVRulerClass))

typedef struct _BtVRuler       BtVRuler;
typedef struct _BtVRulerClass  BtVRulerClass;

struct _BtVRuler
{
  BtRuler ruler;
};

struct _BtVRulerClass
{
  BtRulerClass parent_class;
};

GType      bt_vruler_get_type (void) G_GNUC_CONST;
GtkWidget* bt_vruler_new      (void);

G_END_DECLS

#endif /* __BT_VRULER_H__ */

