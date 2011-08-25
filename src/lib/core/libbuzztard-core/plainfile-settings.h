/* $Id$
 *
 * Buzztard
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

#ifndef BT_PLAINFILE_SETTINGS_H
#define BT_PLAINFILE_SETTINGS_H

#include <glib.h>
#include <glib-object.h>

#include "settings.h"

#define BT_TYPE_PLAINFILE_SETTINGS            (bt_plainfile_settings_get_type ())
#define BT_PLAINFILE_SETTINGS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_PLAINFILE_SETTINGS, BtPlainfileSettings))
#define BT_PLAINFILE_SETTINGS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_PLAINFILE_SETTINGS, BtPlainfileSettingsClass))
#define BT_IS_PLAINFILE_SETTINGS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_PLAINFILE_SETTINGS))
#define BT_IS_PLAINFILE_SETTINGS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_PLAINFILE_SETTINGS))
#define BT_PLAINFILE_SETTINGS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_PLAINFILE_SETTINGS, BtPlainfileSettingsClass))

/* type macros */

typedef struct _BtPlainfileSettings BtPlainfileSettings;
typedef struct _BtPlainfileSettingsClass BtPlainfileSettingsClass;
typedef struct _BtPlainfileSettingsPrivate BtPlainfileSettingsPrivate;

/**
 * BtPlainfileSettings:
 *
 * gconf based implementation object for a buzztard based settings
 */
struct _BtPlainfileSettings {
  const BtSettings parent;
  
  /*< private >*/
  BtPlainfileSettingsPrivate *priv;
};

struct _BtPlainfileSettingsClass {
  const BtSettingsClass parent;
  
};

GType bt_plainfile_settings_get_type(void) G_GNUC_CONST;

BtPlainfileSettings *bt_plainfile_settings_new(void);

#endif // BT_PLAINFILE_SETTINGS_H
