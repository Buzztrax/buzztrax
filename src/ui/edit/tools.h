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

#ifndef BT_EDIT_TOOLS_H
#define BT_EDIT_TOOLS_H

#include <gtk/gtk.h>

/* pixmap/buf helpers */
extern void add_pixmap_directory(const gchar *directory);
extern GtkWidget *gtk_image_new_from_filename(const gchar *filename);
extern GdkPixbuf *gdk_pixbuf_new_from_filename(const gchar *filename);
extern GdkPixbuf *gdk_pixbuf_new_from_theme(const gchar *name, gint size);

/* gtk toolbar helper */
extern GtkToolbarStyle gtk_toolbar_get_style_from_string(const gchar *style_name);

/* save focus grab */
extern void gtk_widget_grab_focus_savely(GtkWidget *widget);

/* gtk clipboard helper */
extern GtkTargetEntry *gtk_target_table_make(GdkAtom format_atom,gint *n_targets);

/* gtk help helper */
extern void gtk_show_uri_simple(GtkWidget *widget, const gchar *uri);

/* debug helper */

#if USE_DEBUG
gboolean bt_edit_ui_config(const gchar *str);
#define BT_EDIT_UI_CONFIG(str) bt_edit_ui_config(str)
#else
#define BT_EDIT_UI_CONFIG(str) FALSE
#endif

#endif // BT_EDIT_TOOLS_H
