/* Buzztrax
 * Copyright (C) 2007 Buzztrax team <buzztrax-devel@buzztrax.org>
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

#ifndef BT_SETTINGS_PAGE_DIRECTORIES_H
#define BT_SETTINGS_PAGE_DIRECTORIES_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_SETTINGS_PAGE_DIRECTORIES            (bt_settings_page_directories_get_type ())
#define BT_SETTINGS_PAGE_DIRECTORIES(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_SETTINGS_PAGE_DIRECTORIES, BtSettingsPageDirectories))
#define BT_SETTINGS_PAGE_DIRECTORIES_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_SETTINGS_PAGE_DIRECTORIES, BtSettingsPageDirectoriesClass))
#define BT_IS_SETTINGS_PAGE_DIRECTORIES(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_SETTINGS_PAGE_DIRECTORIES))
#define BT_IS_SETTINGS_PAGE_DIRECTORIES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_SETTINGS_PAGE_DIRECTORIES))
#define BT_SETTINGS_PAGE_DIRECTORIES_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_SETTINGS_PAGE_DIRECTORIES, BtSettingsPageDirectoriesClass))

/* type macros */

typedef struct _BtSettingsPageDirectories BtSettingsPageDirectories;
typedef struct _BtSettingsPageDirectoriesClass BtSettingsPageDirectoriesClass;
typedef struct _BtSettingsPageDirectoriesPrivate BtSettingsPageDirectoriesPrivate;

/**
 * BtSettingsPageDirectories:
 *
 * the root window for the editor application
 */
struct _BtSettingsPageDirectories {
  GtkGrid parent;
  
  /*< private >*/
  BtSettingsPageDirectoriesPrivate *priv;
};

struct _BtSettingsPageDirectoriesClass {
  GtkGridClass parent;
  
};

GType bt_settings_page_directories_get_type(void) G_GNUC_CONST;

BtSettingsPageDirectories *bt_settings_page_directories_new(GtkWidget *pages);

#endif // BT_SETTINGS_PAGE_DIRECTORIES_H
