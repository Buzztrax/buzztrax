/* Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
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

#ifndef BT_SETTINGS_H
#define BT_SETTINGS_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_SETTINGS            (bt_settings_get_type ())
#define BT_SETTINGS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_SETTINGS, BtSettings))
#define BT_SETTINGS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_SETTINGS, BtSettingsClass))
#define BT_IS_SETTINGS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_SETTINGS))
#define BT_IS_SETTINGS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_SETTINGS))
#define BT_SETTINGS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_SETTINGS, BtSettingsClass))

/**
 * BtSettingsFactory:
 *
 * Factory method that creates a new settings instance.
 *
 * Returns: the setting implementation
 */
typedef gpointer (*BtSettingsFactory)(void);

/* type macros */

typedef struct _BtSettings BtSettings;
typedef struct _BtSettingsClass BtSettingsClass;

/**
 * BtSettings:
 *
 * base object for a buzztard based settings
 */
struct _BtSettings {
  const GObject parent;
};

struct _BtSettingsClass {
  const GObjectClass parent;

  /* class methods */
  //gboolean (*get)(const gpointer self);
  //gboolean (*set)(const gpointer self);
};

GType bt_settings_get_type(void) G_GNUC_CONST;

BtSettings *bt_settings_make(void);

void bt_settings_set_factory(BtSettingsFactory factory);
gboolean bt_settings_determine_audiosink_name(const BtSettings * const self,
    gchar **element_name, gchar **device_name);

#endif // BT_SETTINGS_H
