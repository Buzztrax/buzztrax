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

#define BT_EDIT
#define BT_TOOLS_C

#include "bt-edit.h"

static GList *pixmaps_directories = NULL;

/**
 * add_pixmap_directory:
 * @directory: register another directory to search for pixmaps
 *
 * Use this function to set the directory containing installed pixmaps.
 */
void add_pixmap_directory(const gchar *directory) {
  pixmaps_directories = g_list_prepend(pixmaps_directories, g_strdup(directory));
}

/*
 * find_pixmap_file:
 * @filename: name of the file to look for
 *
 * This is an internally used function to find pixmap files.
 * Use add_pixmap_directory() to extend the serarch path.
 *
 * Returns: the path that has this file. Free returned path after use.
 */
static gchar *find_pixmap_file(const gchar *filename) {
  GList *elem;

  /* We step through each of the pixmaps directory to find it. */
  elem = pixmaps_directories;
  while(elem) {
    gchar *pathname = g_strconcat((gchar*)elem->data, filename, NULL);
    if (g_file_test(pathname, G_FILE_TEST_EXISTS)) return pathname;
    g_free(pathname);
    elem = elem->next;
  }
  return NULL;
}

/**
 * gtk_image_new_from_filename:
 * @filename: the filename of the image file
 *
 * Creates a new pixmap image widget for the image file.
 *
 * Returns: a new pixmap widget
 */
GtkWidget *gtk_image_new_from_filename(const gchar *filename) {
  gchar *pathname = NULL;
  GtkWidget *pixmap;

  if(!filename || !filename[0]) return gtk_image_new();

  pathname = find_pixmap_file(filename);

  if(!pathname) {
    g_warning (_("Couldn't find pixmap file: %s"), filename);
    return gtk_image_new ();
  }

  pixmap = gtk_image_new_from_file(pathname);
  g_free(pathname);
  return pixmap;
}

/**
 * gdk_pixbuf_new_from_filename:
 * @filename: the filename of the image file
 *
 * Creates a new pixbuf image for the image file.
 *
 * Returns: a new pixbuf, g_object_unref() when done.
 */
GdkPixbuf *gdk_pixbuf_new_from_filename(const gchar *filename) {
  gchar *pathname = NULL;
  GdkPixbuf *pixbuf;
  GError *error = NULL;

  if(!filename || !filename[0]) return NULL;

  pathname = find_pixmap_file(filename);

  if(!pathname) {
    g_warning (_("Couldn't find pixmap file: %s"), filename);
    return NULL;
  }

  pixbuf = gdk_pixbuf_new_from_file(pathname, &error);
  if(!pixbuf) {
    fprintf(stderr, "Failed to load pixbuf file: %s: %s\n",pathname, error->message);
    g_error_free(error);
  }
  g_free(pathname);
  return pixbuf;
}

/**
 * gdk_pixbuf_new_from_theme:
 * @name: the icon name
 * @size: the pixel size
 *
 * Creates a new pixbuf image from the icon @name and @size.
 *
 * Returns: a new pixbuf, g_object_unref() when done.
 */
GdkPixbuf *gdk_pixbuf_new_from_theme(const gchar *name, gint size) {
  GdkPixbuf *pixbuf;
  GError *error = NULL;
  GtkIconTheme *it=gtk_icon_theme_get_default();

  /* @todo: docs recommend to listen to GtkWidget::style-set and update icon or
   * do gdk_pixbuf_copy() to avoid gtk keeping icon-theme loaded if it changes
  */
  if(!(pixbuf=gtk_icon_theme_load_icon(it,name,size,0,&error))) {
    GST_WARNING("Couldn't load %s %dx%d icon: %s",name,size,size,error->message);
    g_error_free(error);
    /* @todo: machine icons are in 'gnome' theme, how can we use this as a
     * fallback
     * gtk_icon_theme_set_custom_theme(it,"gnome");
     * is a bit brutal 
     */
    return gdk_pixbuf_new(GDK_COLORSPACE_RGB,TRUE,8,size,size);
    //return NULL;
  }
  else {
    GdkPixbuf *result = gdk_pixbuf_copy(pixbuf);
    g_object_unref(pixbuf);
    return result;
  }
}


// gtk toolbar helper

/**
 * gtk_toolbar_get_style_from_string:
 * @style_name: the style name returned from gconf settings
 *
 * toolbar gconf to style conversion
 *
 * Returns: the style id
 */
GtkToolbarStyle gtk_toolbar_get_style_from_string(const gchar *style_name) {

  g_return_val_if_fail(style_name,GTK_TOOLBAR_BOTH);

  if (!strcmp(style_name,"icons")) {
    return(GTK_TOOLBAR_ICONS);
  }
  else if (!strcmp(style_name,"both")) {
    return(GTK_TOOLBAR_BOTH);
  }
  else if (!strcmp(style_name,"both-horiz")) {
    return(GTK_TOOLBAR_BOTH_HORIZ);
  }
  else if (!strcmp(style_name,"text")) {
    return(GTK_TOOLBAR_TEXT);
  }
  return(GTK_TOOLBAR_BOTH);
}


// save focus grab

/**
 * gtk_widget_grab_focus_savely:
 * @widget: the widget
 *
 * Grab focus only if widget has been realized and is mapped.
 */
void gtk_widget_grab_focus_savely(GtkWidget *widget) {
  if(GTK_WIDGET_REALIZED(widget) && GTK_WIDGET_MAPPED(widget)) {
    gtk_widget_grab_focus(widget);
  }
}

