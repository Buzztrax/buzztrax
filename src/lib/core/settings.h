/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@lists.sf.net>
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

#ifndef BT_SETTINGS_H
#define BT_SETTINGS_H

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>


#define BT_TYPE_SETTINGS            (bt_settings_get_type ())
#define BT_SETTINGS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_SETTINGS, BtSettings))
#define BT_SETTINGS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_SETTINGS, BtSettingsClass))
#define BT_IS_SETTINGS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_SETTINGS))
#define BT_IS_SETTINGS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_SETTINGS))
#define BT_SETTINGS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_SETTINGS, BtSettingsClass))

/* type macros */

typedef struct _BtSettings BtSettings;
typedef struct _BtSettingsClass BtSettingsClass;
typedef struct _BtSettingsPrivate BtSettingsPrivate;

/**
 * BtSettings:
 *
 * base object for a buzztrax based settings
 */
struct _BtSettings {
  const GObject parent;
  
  /*< private >*/
  BtSettingsPrivate *priv;
};

struct _BtSettingsClass {
  const GObjectClass parent;
};

GType bt_settings_get_type(void) G_GNUC_CONST;

BtSettings *bt_settings_make(void);

void bt_settings_set_backend(gpointer backend);
gboolean bt_settings_determine_audiosink_name(const BtSettings * const self,
    gchar **element_name, gchar **device_name);

#endif // BT_SETTINGS_H
