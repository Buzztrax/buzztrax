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

/* gtk+ compatibillity */

#if !GTK_CHECK_VERSION(2,14,0)

#define gtk_dialog_get_content_area(dialog) (dialog->vbox)
#define gtk_dialog_get_action_area(dialog) (dialog->action_area)

#define gtk_widget_get_window(widget) (widget->window)

#define gtk_selection_data_get_data(sdata) (sdata->data);
#define gtk_selection_data_get_target(sdata) (sdata->taget);

#endif

#if !GTK_CHECK_VERSION(2,18,0)

#define gtk_widget_get_allocation(widget, alloc) memcpy(alloc,&(widget->allocation),sizeof(GtkAllocation))
#define gtk_widget_is_toplevel(widget) GTK_WIDGET_TOPLEVEL(widget)
#define gtk_widget_set_has_window(widget, flag) \
  if (!flag) GTK_WIDGET_SET_FLAGS (widget, GTK_NO_WINDOW); \
  else GTK_WIDGET_UNSET_FLAGS (widget, GTK_NO_WINDOW)

#endif

#if !GTK_CHECK_VERSION(2,20,0)

#define gtk_widget_set_realized(widget, flag) \
  if (flag) GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED); \
  else GTK_WIDGET_UNSET_FLAGS (widget, GTK_REALIZED)
#define gtk_widget_set_can_focus(widget, flag) \
  if (flag) GTK_WIDGET_SET_FLAGS (widget, GTK_CAN_FOCUS); \
  else GTK_WIDGET_UNSET_FLAGS (widget, GTK_CAN_FOCUS)

#define gtk_widget_get_realized(widget) \
  ((GTK_WIDGET_FLAGS (widget) & GTK_REALIZED) != 0)
#define gtk_widget_get_mapped(widget) \
  ((GTK_WIDGET_FLAGS (widget) & GTK_MAPPED) != 0)

#endif

#endif // BT_EDIT_TOOLS_H
