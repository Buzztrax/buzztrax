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
#include <glib/gprintf.h>

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
    return gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, size, size);
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
    return GTK_TOOLBAR_ICONS;
  } else if (!strcmp (style_name, "both")) {
    return GTK_TOOLBAR_BOTH;
  } else if (!strcmp (style_name, "both-horiz")) {
    return GTK_TOOLBAR_BOTH_HORIZ;
  } else if (!strcmp (style_name, "text")) {
    return GTK_TOOLBAR_TEXT;
  }
  return GTK_TOOLBAR_BOTH;
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

  return targets;
}


// gtk help helper

/**
 * gtk_show_uri_simple:
 * @widget: widget that triggered the action
 * @uri: the uri
 *
 * Show the given @uri. Uses same screen as @widget.
 */
void
gtk_show_uri_simple (GtkWidget * widget, const gchar * uri)
{
  GError *error = NULL;

  g_return_if_fail (widget);
  g_return_if_fail (uri);

#if GTK_CHECK_VERSION (3, 22, 0)
  GtkWidget *toplevel = gtk_widget_get_toplevel (widget);
  if (!gtk_widget_is_toplevel (toplevel)) {
    GST_WARNING ("Failed lookup widgets window\n");
  }
  GtkWindow *window = GTK_WINDOW (toplevel);

  if (!gtk_show_uri_on_window (window, uri, gtk_get_current_event_time (),
          &error)) {
    GST_WARNING ("Failed to display help: %s\n", error->message);
    g_error_free (error);
  }
#else
  GdkScreen *screen = gtk_widget_get_screen (widget);

  if (!gtk_show_uri (screen, uri, gtk_get_current_event_time (), &error)) {
    GST_WARNING ("Failed to display help: %s\n", error->message);
    g_error_free (error);
  }
#endif
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
    return TRUE;
  }
  return FALSE;
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

/**
 * bt_pointer_to_boolean:
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
bt_pointer_to_boolean (GBinding * binding, const GValue * from_value,
    GValue * to_value, gpointer user_data)
{
  g_value_set_boolean (to_value, (g_value_get_object (from_value) != NULL));
  return TRUE;
}

/* tool bar icon helper */

/**
 * gtk_tool_button_new_from_icon_name:
 * @icon_name: (allow-none): the name of the themed icon
 * @label: (allow-none): a string that will be used as label, or %NULL
 *
 * Creates a new #GtkToolButton containing @icon_name as contents and @label as
 * label.
 *
 * Returns: the new #GtkToolItem
 */
GtkToolItem *
gtk_tool_button_new_from_icon_name (const gchar * icon_name,
    const gchar * label)
{
  return g_object_new (GTK_TYPE_TOOL_BUTTON,
      "icon-name", icon_name, "label", label, NULL);
}

/**
 * gtk_toggle_tool_button_new_from_icon_name:
 * @icon_name: (allow-none): the name of the themed icon
 * @label: (allow-none): a string that will be used as label, or %NULL
 *
 * Creates a new #GtkToggleToolButton containing @icon_name as contents and
 * @label as label.
 *
 * Returns: the new #GtkToolItem
 */
GtkToolItem *
gtk_toggle_tool_button_new_from_icon_name (const gchar * icon_name,
    const gchar * label)
{
  return g_object_new (GTK_TYPE_TOGGLE_TOOL_BUTTON,
      "icon-name", icon_name, "label", label, NULL);
}

/* menu accel helper */

/**
 * gtk_menu_item_add_accel:
 * @mi: a valid GtkMenuItem
 * @path: accelerator path, corresponding to this menu item's functionality.
 * @accel_key: the accelerator key
 * @accel_mods: the accelerator modifiers
 *
 * Convenience wrapper around gtk_menu_item_set_accel_path() and
 * gtk_accel_map_add_entry().
 *
 * see_also: gtk_widget_add_accelerator()
 */
void
gtk_menu_item_add_accel (GtkMenuItem * mi, const gchar * path, guint accel_key,
    GdkModifierType accel_mods)
{
  g_intern_static_string (path);
  gtk_menu_item_set_accel_path (mi, path);
  gtk_accel_map_add_entry (path, accel_key, accel_mods);
}

/* notify main-loop dispatch helper */

typedef struct
{
  GObject *object;
  GParamSpec *pspec;
  GWeakRef user_data;
  BtNotifyFunc func;
} BtNotifyIdleData;

static gboolean
on_idle_notify (gpointer user_data)
{
  BtNotifyIdleData *data = (BtNotifyIdleData *) user_data;
  gpointer weak_data = g_weak_ref_get (&data->user_data);

  if (weak_data) {
    data->func (data->object, data->pspec, weak_data);
    g_object_unref (weak_data);
  }
  g_weak_ref_clear (&data->user_data);
  g_slice_free (BtNotifyIdleData, data);
  return FALSE;
}

/**
 * bt_notify_idle_dispatch:
 * @object: the object
 * @pspec: the arg
 * @user_data: the extra data
 * @func: the actual callback
 *
 * Save the parameters from a #GObject::notify callback, run it through
 * g_idle_add(), unpack the params and call @func.
 */
void
bt_notify_idle_dispatch (GObject * object, GParamSpec * pspec,
    gpointer user_data, BtNotifyFunc func)
{
  BtNotifyIdleData *data = g_slice_new (BtNotifyIdleData);

  data->object = object;
  data->pspec = pspec;
  data->func = func;
  g_weak_ref_init (&data->user_data, user_data);
  g_idle_add (on_idle_notify, data);
}

/* gtk compat helper */

/**
 * bt_gtk_workarea_size:
 * @max_width: destination for the width or %NULL
 * @max_height: destination for the heigth or %NULL
 *
 * Gets the potitial max size the window could occupy. This can be used to
 * hint the content size for #GtkScrelledWindow.
 */
void
bt_gtk_workarea_size (gint * max_width, gint * max_height)
{
#if GTK_CHECK_VERSION (3, 22, 0)
  GdkRectangle area;
  gdk_monitor_get_workarea (gdk_display_get_primary_monitor
      (gdk_display_get_default ()), &area);
  if (max_width)
    *max_width = area.width;
  if (max_height)
    *max_height = area.height;
#else
  GdkScreen *screen = gdk_screen_get_default ();
  /* TODO(ensonjc): these constances below are arbitrary
   * look at http://standards.freedesktop.org/wm-spec/1.3/ar01s05.html#id2523368
   * search for _NET_WM_STRUT_PARTIAL
   */
  if (max_width)
    *max_width = gdk_screen_get_width (screen) - 16;
  if (max_height)
    *max_height = gdk_screen_get_height (screen) - 80;
#endif
}

/**
 * bt_strjoin_list:
 * @list: a list of test nodes
 *
 * Concatenates the strings using a newline. There is no final newline.
 *
 * Returns: a new string, Free after use.
 */
gchar *
bt_strjoin_list (GList * list)
{
  GList *node;
  gchar *str, *ptr;
  gint length = 0;

  if (!list) {
    return NULL;
  }

  for (node = list; node; node = g_list_next (node)) {
    length += 2 + strlen ((gchar *) (node->data));
  }
  ptr = str = g_malloc (length);
  for (node = list; node; node = g_list_next (node)) {
    length = g_sprintf (ptr, "%s\n", (gchar *) (node->data));
    ptr = &ptr[length];
  }
  ptr[-1] = '\0';               // remove last '\n'

  return str;
}
