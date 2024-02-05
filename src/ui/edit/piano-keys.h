/* Buzztrax
 * Copyright (C) 2015 Buzztrax team <buzztrax-devel@buzztrax.org>
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

#ifndef BT_PIANO_KEYS_H
#define BT_PIANO_KEYS_H

#include <gtk/gtk.h>
#include "gst/musicenums.h"

G_BEGIN_DECLS

#define BT_TYPE_PIANO_KEYS          (bt_piano_keys_get_type ())
#define BT_PIANO_KEYS(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_PIANO_KEYS, BtPianoKeys))
#define BT_IS_PIANO_KEYS(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_PIANO_KEYS))
#define BT_PIANO_KEYS_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass),  BT_TYPE_PIANO_KEYS, BtPianoKeysClass))
#define BT_IS_PIANO_KEYS_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass),  BT_TYPE_PIANO_KEYS))

typedef struct _BtPianoKeys      BtPianoKeys;
typedef struct _BtPianoKeysClass BtPianoKeysClass;

/**
 * BtPianoKeys:
 *
 * A musical piano keyboard widget.
 */
struct _BtPianoKeys {
  GtkWidget parent;

  /* state */
  GstBtNote key;
  GdkSurface *window;
  GtkBorder border;
};

struct _BtPianoKeysClass {
  GtkWidgetClass klass;
};

GtkWidget *bt_piano_keys_new(void);

GType bt_piano_keys_get_type(void) G_GNUC_CONST;

G_END_DECLS

#endif // BT_PIANO_KEYS_H
