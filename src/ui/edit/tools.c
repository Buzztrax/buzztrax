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
/* TODO(ensonic):
 * - we load some icons several times, we could extend the stock icons
 * - some pointers:
 *   gtk_stock_add(...)
 *   gtk_icon_factory_add(GtkIconFactory *,const gchar *stock_id,GtkIconSet *);
 *   GtkIconSet *gtk_icon_set_new_from_pixbuf(...)
 * - examples:
 *   https://github.com/jcupitt/nip2/blob/master/src/main.c#L668
 */

#define BT_EDIT
#define BT_TOOLS_C

#include "bt-edit.h"

/**
 * gdk_pixbuf_new_from_theme:
 * @name: the icon name
 * @size: the pixel size
 *
 * Creates a new pixbuf image from the icon @name and @size.
 *
 * Returns: a new pixbuf, g_object_unref() when done.
 */
GdkPixbuf *
gdk_pixbuf_new_from_theme (const gchar * name, gint size)
{
  GdkPixbuf *pixbuf;
  GError *error = NULL;
  GtkIconTheme *it = gtk_icon_theme_get_default ();

  /* TODO(ensonic): docs recommend to listen to GtkWidget::style-set and update icon or
   * do gdk_pixbuf_copy() to avoid gtk keeping icon-theme loaded if it changes
   */
  if (!(pixbuf =
          gtk_icon_theme_load_icon (it, name, size,
              GTK_ICON_LOOKUP_FORCE_SVG | GTK_ICON_LOOKUP_FORCE_SIZE,
              &error))) {
    GST_WARNING ("Couldn't load %s %dx%d icon: %s", name, size, size,
        error->message);
    g_error_free (error);
    /* TODO(ensonic): machine icons are in 'gnome' theme, how can we use this as a
     * fallback
     * gtk_icon_theme_set_custom_theme(it,"gnome");
     * is a bit brutal 
     */
    return gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, size, size);
    //return NULL;
  } else {
    GdkPixbuf *result = gdk_pixbuf_copy (pixbuf);
    g_object_unref (pixbuf);
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
GtkToolbarStyle
gtk_toolbar_get_style_from_string (const gchar * style_name)
{

  g_return_val_if_fail (style_name, GTK_TOOLBAR_BOTH);

  if (!strcmp (style_name, "icons")) {
    return (GTK_TOOLBAR_ICONS);
  } else if (!strcmp (style_name, "both")) {
    return (GTK_TOOLBAR_BOTH);
  } else if (!strcmp (style_name, "both-horiz")) {
    return (GTK_TOOLBAR_BOTH_HORIZ);
  } else if (!strcmp (style_name, "text")) {
    return (GTK_TOOLBAR_TEXT);
  }
  return (GTK_TOOLBAR_BOTH);
}


// save focus grab

/**
 * gtk_widget_grab_focus_savely:
 * @widget: the widget
 *
 * Grab focus only if widget has been realized and is mapped.
 */
void
gtk_widget_grab_focus_savely (GtkWidget * widget)
{
  gboolean realized = gtk_widget_get_realized (widget);
  gboolean mapped = gtk_widget_get_mapped (widget);

  if (realized && mapped) {
    gtk_widget_grab_focus (widget);
  } else {
    GST_INFO ("not grabbing widget '%s', realized: %d, mapped %d",
        gtk_widget_get_name (widget), realized, mapped);
  }
}


// gtk clipboard helper

/**
 * gtk_target_table_make:
 * @format_atom: format atom for the target list
 * @n_targets: out variable for the size of the table
 *
 * Generate the target table for pasting to clipboard. Use
 * gtk_target_table_free (targets, n_targets);
 *
 * Returns: the table and the size in the out variable @n_targets 
 */
GtkTargetEntry *
gtk_target_table_make (GdkAtom format_atom, gint * n_targets)
{
  GtkTargetList *list;
  GtkTargetEntry *targets;

  list = gtk_target_list_new (NULL, 0);
  gtk_target_list_add (list, format_atom, 0, 0);
#if USE_DEBUG
  // this allows to paste into a text editor
  gtk_target_list_add (list, gdk_atom_intern_static_string ("UTF8_STRING"), 0,
      1);
  gtk_target_list_add (list, gdk_atom_intern_static_string ("TEXT"), 0, 2);
  gtk_target_list_add (list, gdk_atom_intern_static_string ("text/plain"), 0,
      3);
  gtk_target_list_add (list,
      gdk_atom_intern_static_string ("text/plain;charset=utf-8"), 0, 4);
#endif
  targets = gtk_target_table_new_from_list (list, n_targets);
  gtk_target_list_unref (list);

  return (targets);
}


// gtk help helper

/**
 * gtk_show_uri_simple:
 * @widget: widget that triggered the action
 * @uri: the uri
 * 
 * Show the given @uri. Uses same screen as @widget (default if @widget is NULL).
 */
void
gtk_show_uri_simple (GtkWidget * widget, const gchar * uri)
{
  GError *error = NULL;
  GdkScreen *screen = NULL;

  if (widget)
    screen = gtk_widget_get_screen (widget);

  if (!gtk_show_uri (screen, uri, gtk_get_current_event_time (), &error)) {
    GST_WARNING ("Failed to display help: %s\n", error->message);
    g_error_free (error);
  }
}

/* debug helper */

/*
 * bt_edit_ui_config:
 * @str: token to check
 *
 * Allows to adhoc re-config the UI via BT_EDIT_UI_CFG environment variable.
 *
 * Return: %TRUE if the token is found in the env-var.
 */
#if USE_DEBUG
gboolean
bt_edit_ui_config (const gchar * str)
{
  const gchar *env = g_getenv ("BT_EDIT_UI_CFG");
  if (env && strstr (env, str)) {
    return (TRUE);
  }
  return (FALSE);
}
#endif

/* gobject property binding transform functions */

/**
 * bt_toolbar_style_changed:
 * @binding: the binding
 * @from_value: the source value
 * @to_value: the target value
 * @user_data: unused
 *
 * Transform function to be used with g_object_bind_property_full()
 *
 * Returns: %TRUE for successful sync of the properties
 */
gboolean
bt_toolbar_style_changed (GBinding * binding, const GValue * from_value,
    GValue * to_value, gpointer user_data)
{
  const gchar *style = g_value_get_string (from_value);
  if (!BT_IS_STRING (style))
    return FALSE;

  g_value_set_enum (to_value, gtk_toolbar_get_style_from_string (style));
  return TRUE;
}

/**
 * bt_label_value_changed:
 * @binding: the binding
 * @from_value: the source value
 * @to_value: the target value
 * @user_data: the label formatstring
 *
 * Transform function to be used with g_object_bind_property_full()
 *
 * Returns: %TRUE for successful sync of the properties
 */
gboolean
bt_label_value_changed (GBinding * binding, const GValue * from_value,
    GValue * to_value, gpointer user_data)
{
  g_value_take_string (to_value, g_strdup_printf (user_data,
          g_value_get_string (from_value)));
  return TRUE;
}
