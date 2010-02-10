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

#include "main-window.h"

/* pixmap/buf helpers */
extern void add_pixmap_directory(const gchar *directory);
extern GtkWidget *gtk_image_new_from_filename(const gchar *filename);
extern GdkPixbuf *gdk_pixbuf_new_from_filename(const gchar *filename);
extern GdkPixbuf *gdk_pixbuf_new_from_theme(const gchar *name, gint size);

/* gtk toolbar helper */
extern GtkToolbarStyle gtk_toolbar_get_style_from_string(const gchar *style_name);

/* save focus grab */
extern void gtk_widget_grab_focus_savely(GtkWidget *widget);

/* gtk+ compatibillity */

#if !GTK_CHECK_VERSION(2,12,0)

#define gtk_widget_set_tooltip_text(widget,text) \
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips),widget,text,NULL)

#define gtk_tool_item_set_tooltip_text(widget,text) \
    gtk_tool_item_set_tooltip(widget,GTK_TOOLTIPS(tips),text,NULL)

#define gtk_menu_tool_button_set_arrow_tooltip_text(widget,text) \
    gtk_menu_tool_button_set_arrow_tooltip(widget,GTK_TOOLTIPS(tips),text,NULL)
    
#endif

#if !GTK_CHECK_VERSION(2,14,0)

#define gtk_dialog_get_content_area(dialog) (dialog->vbox)
#define gtk_dialog_get_action_area(dialog) (dialog->action_area)

#define gtk_widget_get_window(widget) (widget->window)

#endif

#if !GTK_CHECK_VERSION(2,18,0)

gtk_widget_get_allocation(widget, alloc) memcpy(alloc,&(widget->allocation),sizeof(GtkAllocation))

#endif

#endif // BT_EDIT_TOOLS_H
