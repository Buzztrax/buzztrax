/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@buzztrax.org>
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

#ifndef BT_SETTINGS_PAGE_PLAYBACK_CONTROLLER_H
#define BT_SETTINGS_PAGE_PLAYBACK_CONTROLLER_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_SETTINGS_PAGE_PLAYBACK_CONTROLLER            (bt_settings_page_playback_controller_get_type ())
#define BT_SETTINGS_PAGE_PLAYBACK_CONTROLLER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_SETTINGS_PAGE_PLAYBACK_CONTROLLER, BtSettingsPagePlaybackController))
#define BT_SETTINGS_PAGE_PLAYBACK_CONTROLLER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_SETTINGS_PAGE_PLAYBACK_CONTROLLER, BtSettingsPagePlaybackControllerClass))
#define BT_IS_SETTINGS_PAGE_PLAYBACK_CONTROLLER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_SETTINGS_PAGE_PLAYBACK_CONTROLLER))
#define BT_IS_SETTINGS_PAGE_PLAYBACK_CONTROLLER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_SETTINGS_PAGE_PLAYBACK_CONTROLLER))
#define BT_SETTINGS_PAGE_PLAYBACK_CONTROLLER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_SETTINGS_PAGE_PLAYBACK_CONTROLLER, BtSettingsPagePlaybackControllerClass))

/* type macros */

typedef struct _BtSettingsPagePlaybackController BtSettingsPagePlaybackController;
typedef struct _BtSettingsPagePlaybackControllerClass BtSettingsPagePlaybackControllerClass;
typedef struct _BtSettingsPagePlaybackControllerPrivate BtSettingsPagePlaybackControllerPrivate;

/**
 * BtSettingsPagePlaybackController:
 *
 * the root window for the editor application
 */
struct _BtSettingsPagePlaybackController {
  GtkGrid parent;

  /*< private >*/
  BtSettingsPagePlaybackControllerPrivate *priv;
};

struct _BtSettingsPagePlaybackControllerClass {
  GtkGridClass parent;

};

GType bt_settings_page_playback_controller_get_type(void) G_GNUC_CONST;

BtSettingsPagePlaybackController *bt_settings_page_playback_controller_new(GtkWidget *pages);

#endif // BT_SETTINGS_PAGE_PLAYBACK_CONTROLLER_H
