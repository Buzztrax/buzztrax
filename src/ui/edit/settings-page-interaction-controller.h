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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BT_SETTINGS_PAGE_INTERACTION_CONTROLLER_H
#define BT_SETTINGS_PAGE_INTERACTION_CONTROLLER_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_SETTINGS_PAGE_INTERACTION_CONTROLLER            (bt_settings_page_interaction_controller_get_type ())
#define BT_SETTINGS_PAGE_INTERACTION_CONTROLLER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_SETTINGS_PAGE_INTERACTION_CONTROLLER, BtSettingsPageInteractionController))
#define BT_SETTINGS_PAGE_INTERACTION_CONTROLLER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_SETTINGS_PAGE_INTERACTION_CONTROLLER, BtSettingsPageInteractionControllerClass))
#define BT_IS_SETTINGS_PAGE_INTERACTION_CONTROLLER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_SETTINGS_PAGE_INTERACTION_CONTROLLER))
#define BT_IS_SETTINGS_PAGE_INTERACTION_CONTROLLER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_SETTINGS_PAGE_INTERACTION_CONTROLLER))
#define BT_SETTINGS_PAGE_INTERACTION_CONTROLLER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_SETTINGS_PAGE_INTERACTION_CONTROLLER, BtSettingsPageInteractionControllerClass))

/* type macros */

typedef struct _BtSettingsPageInteractionController BtSettingsPageInteractionController;
typedef struct _BtSettingsPageInteractionControllerClass BtSettingsPageInteractionControllerClass;
typedef struct _BtSettingsPageInteractionControllerPrivate BtSettingsPageInteractionControllerPrivate;

/**
 * BtSettingsPageInteractionController:
 *
 * the root window for the editor application
 */
struct _BtSettingsPageInteractionController {
  GtkTable parent;

  /*< private >*/
  BtSettingsPageInteractionControllerPrivate *priv;
};

struct _BtSettingsPageInteractionControllerClass {
  GtkTableClass parent;

};

GType bt_settings_page_interaction_controller_get_type(void) G_GNUC_CONST;

BtSettingsPageInteractionController *bt_settings_page_interaction_controller_new(void);

#endif // BT_SETTINGS_PAGE_INTERACTION_CONTROLLER_H
