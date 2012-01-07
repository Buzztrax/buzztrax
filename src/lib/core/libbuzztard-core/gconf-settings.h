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

#ifndef BT_GCONF_SETTINGS_H
#define BT_GCONF_SETTINGS_H

#include <glib.h>
#include <glib-object.h>

#include "settings.h"

#define BT_TYPE_GCONF_SETTINGS            (bt_gconf_settings_get_type ())
#define BT_GCONF_SETTINGS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_GCONF_SETTINGS, BtGConfSettings))
#define BT_GCONF_SETTINGS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_GCONF_SETTINGS, BtGConfSettingsClass))
#define BT_IS_GCONF_SETTINGS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_GCONF_SETTINGS))
#define BT_IS_GCONF_SETTINGS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_GCONF_SETTINGS))
#define BT_GCONF_SETTINGS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_GCONF_SETTINGS, BtGConfSettingsClass))

/* type macros */

typedef struct _BtGConfSettings BtGConfSettings;
typedef struct _BtGConfSettingsClass BtGConfSettingsClass;
typedef struct _BtGConfSettingsPrivate BtGConfSettingsPrivate;

/**
 * BtGConfSettings:
 *
 * gconf based implementation object for a buzztard based settings
 */
struct _BtGConfSettings {
  const BtSettings parent;
  
  /*< private >*/
  BtGConfSettingsPrivate *priv;
};

struct _BtGConfSettingsClass {
  const BtSettingsClass parent;
  
};

GType bt_gconf_settings_get_type(void) G_GNUC_CONST;

BtGConfSettings *bt_gconf_settings_new(void);

#endif // BT_GCONF_SETTINGS_H
