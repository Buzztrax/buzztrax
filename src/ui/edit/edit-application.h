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

#ifndef BT_EDIT_APPLICATION_H
#define BT_EDIT_APPLICATION_H

#include <glib.h>
#include <glib-object.h>
#include <adwaita.h>
#include "core/application.h"

#define BT_TYPE_EDIT_APPLICATION            (bt_edit_application_get_type ())
#define BT_EDIT_APPLICATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_EDIT_APPLICATION, BtEditApplication))
#define BT_EDIT_APPLICATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_EDIT_APPLICATION, BtEditApplicationClass))
#define BT_IS_EDIT_APPLICATION(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_EDIT_APPLICATION))
#define BT_IS_EDIT_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_EDIT_APPLICATION))
#define BT_EDIT_APPLICATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_EDIT_APPLICATION, BtEditApplicationClass))

/* type macros */

typedef struct _BtEditApplication BtEditApplication;
typedef struct _BtEditApplicationClass BtEditApplicationClass;
typedef struct _BtEditApplicationPrivate BtEditApplicationPrivate;

/**
 * BtEditApplication:
 *
 * #BtApplication subclass for the gtk editor application
 */
struct _BtEditApplication {
  BtApplication parent;
  
  /*< private >*/
  BtEditApplicationPrivate *priv;
};

struct _BtEditApplicationClass {
  BtApplicationClass parent;

  void (*song_changed)(const BtEditApplication *app, gpointer user_data);
};

GType bt_edit_application_get_type(void) G_GNUC_CONST;

BtEditApplication *bt_edit_application_new(void);

gboolean bt_edit_application_new_song(const BtEditApplication *self);
gboolean bt_edit_application_load_song(const BtEditApplication *self,const char *file_name, GError **err);
gboolean bt_edit_application_save_song(const BtEditApplication *self,const char *file_name, GError **err);

gboolean bt_edit_application_run(const BtEditApplication *self);
gboolean bt_edit_application_load_and_run(const BtEditApplication *self, const gchar *input_file_name);
void bt_edit_application_quit(const BtEditApplication *self);

void bt_edit_application_show_about(BtEditApplication *self);
void bt_edit_application_show_tip(const BtEditApplication *self);

void bt_edit_application_crash_log_recover(const BtEditApplication *self);

void bt_edit_application_attach_child_window(const BtEditApplication *self, GtkWindow *window);

void bt_edit_application_ui_lock(const BtEditApplication *self);
void bt_edit_application_ui_unlock(const BtEditApplication *self);

gboolean bt_edit_application_is_song_unsaved(const BtEditApplication *self);
void bt_edit_application_set_song_unsaved(const BtEditApplication *self);


/**
 * BtAdwAppEdit:
 *
 * AdwApplication subclass for the gtk editor application, used to leverage the GTK4 way of application lifecycle
 * management.
 */
G_DECLARE_FINAL_TYPE(BtAdwAppEdit, bt_adw_app_edit, BT, ADW_APP_EDIT, AdwApplication);

BtAdwAppEdit* bt_adw_app_edit_new ();

#endif // BT_EDIT_APPLICATION_H
