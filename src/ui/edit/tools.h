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

#ifndef BT_EDIT_TOOLS_H
#define BT_EDIT_TOOLS_H

#include <gtk/gtk.h>

/* pixbuf helpers */
GdkPixbuf *gdk_pixbuf_new_from_theme(const gchar *name, gint size);

/* gtk toolbar helper */
GtkToolbarStyle gtk_toolbar_get_style_from_string(const gchar *style_name);

/* save focus grab */
void gtk_widget_grab_focus_savely(GtkWidget *widget);

/* gtk clipboard helper */
GtkTargetEntry *gtk_target_table_make(GdkAtom format_atom,gint *n_targets);

/* gtk help helper */
void gtk_show_uri_simple(GtkWidget *widget, const gchar *uri);

/* debug helper */

#if USE_DEBUG
gboolean bt_edit_ui_config(const gchar *str);
#define BT_EDIT_UI_CONFIG(str) bt_edit_ui_config(str)
#else
#define BT_EDIT_UI_CONFIG(str) FALSE
#endif

/* gobject property binding transform functions */
gboolean bt_toolbar_style_changed(GBinding * binding, const GValue * from_value, GValue * to_value, gpointer user_data);
gboolean bt_label_value_changed(GBinding * binding, const GValue * from_value, GValue * to_value, gpointer user_data);
gboolean bt_pointer_to_boolean(GBinding * binding, const GValue * from_value, GValue * to_value, gpointer user_data);

/* tool bar icon helper */
GtkToolItem *gtk_tool_button_new_from_icon_name(const gchar *icon_name, const gchar *label);
GtkToolItem *gtk_toggle_tool_button_new_from_icon_name(const gchar *icon_name, const gchar *label);

/* menu accel helper */
void gtk_menu_item_add_accel(GtkMenuItem *mi, const gchar *path, guint accel_key, GdkModifierType accel_mods);

/* notify main-loop dispatch helper */

/**
 * BtNotifyFunc:
 * @object: the object
 * @pspec: the arg
 * @user_data: the extra data
 *
 * Callback function for #GObject::notify.
 */
typedef void (*BtNotifyFunc)(GObject *object, GParamSpec *pspec, gpointer user_data);
void bt_notify_idle_dispatch (GObject *object, GParamSpec *pspec, gpointer user_data, BtNotifyFunc func);

/* gtk compat helper */
void bt_gtk_workarea_size (gint * max_width, gint * max_height);


char *bt_strjoin_list (GList *list);

#endif // BT_EDIT_TOOLS_H
