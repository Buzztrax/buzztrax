/* Buzztrax
 * Copyright (C) 2011 Buzztrax team <buzztrax-devel@buzztrax.org>
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
/**
 * SECTION:btsettingspageui
 * @short_description: theme and color settings page
 *
 * Edit and manage themes and colors.
 */
/* TODO(ensonic): add settings for
 * - appearance:
 *   - ui theme name + variant, icon theme
 * - fonts (add to css theme?)
 *   - font + size for machine view canvas
 *   - font sizes for table-headings (as pango markup sizes)
 * - extra colors
 */
#define BT_EDIT
#define BT_SETTINGS_PAGE_UI_C

#include "bt-edit.h"

struct _BtSettingsPageUIPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;
};

//-- the class

G_DEFINE_TYPE_WITH_CODE (BtSettingsPageUI, bt_settings_page_ui, GTK_TYPE_GRID, 
    G_ADD_PRIVATE(BtSettingsPageUI));

//-- event handler

static void
on_theme_style_toggled (GtkToggleButton * button, gpointer user_data)
{
  BtSettingsPageUI *self = BT_SETTINGS_PAGE_UI (user_data);
  BtSettings *settings;

  g_object_get (self->priv->app, "settings", &settings, NULL);
  g_object_set (settings, gtk_widget_get_name (GTK_WIDGET (button)),
      gtk_toggle_button_get_active (button), NULL);
  g_object_unref (settings);
}

//-- helper methods

static void
bt_settings_page_ui_init_ui (const BtSettingsPageUI * self, GtkWidget * pages)
{
  BtSettings *settings;
  GtkWidget *label, *widget;
  gchar *str;
  gboolean use_dark_theme, use_compact_theme;

  gtk_widget_set_name (GTK_WIDGET (self), "ui shortcut settings");

  // get settings
  g_object_get (self->priv->app, "settings", &settings, NULL);
  g_object_get (settings, "dark-theme", &use_dark_theme,
      "compact-theme", &use_compact_theme, NULL);

  // add setting widgets
  label = gtk_label_new (NULL);
  str = g_strdup_printf ("<big><b>%s</b></big>", _("User Interface"));
  gtk_label_set_markup (GTK_LABEL (label), str);
  g_free (str);
  g_object_set (label, "xalign", 0.0, NULL);
  gtk_grid_attach (GTK_GRID (self), label, 0, 0, 3, 1);
  gtk_grid_attach (GTK_GRID (self), gtk_label_new ("    "), 0, 1, 1, 2);

  label = gtk_label_new (_("Dark theme variant"));
  g_object_set (label, "xalign", 1.0, NULL);
  gtk_grid_attach (GTK_GRID (self), label, 1, 1, 1, 1);

  widget = gtk_check_button_new ();
  gtk_widget_set_name (widget, "dark-theme");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), use_dark_theme);
  g_signal_connect (widget, "toggled", G_CALLBACK (on_theme_style_toggled),
      (gpointer) self);
  g_object_set (widget, "margin-left", LABEL_PADDING, NULL);
  gtk_grid_attach (GTK_GRID (self), widget, 2, 1, 1, 1);

  label = gtk_label_new (_("Compact theme variant"));
  g_object_set (label, "xalign", 1.0, NULL);
  gtk_grid_attach (GTK_GRID (self), label, 1, 2, 1, 1);

  widget = gtk_check_button_new ();
  gtk_widget_set_name (widget, "compact-theme");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), use_compact_theme);
  g_signal_connect (widget, "toggled", G_CALLBACK (on_theme_style_toggled),
      (gpointer) self);
  g_object_set (widget, "margin-left", LABEL_PADDING, NULL);
  gtk_grid_attach (GTK_GRID (self), widget, 2, 2, 1, 1);

  g_object_unref (settings);
}

//-- constructor methods

/**
 * bt_settings_page_ui_new:
 * @pages: the page collection
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtSettingsPageUI *
bt_settings_page_ui_new (GtkWidget * pages)
{
  BtSettingsPageUI *self;

  self = BT_SETTINGS_PAGE_UI (g_object_new (BT_TYPE_SETTINGS_PAGE_UI, NULL));
  bt_settings_page_ui_init_ui (self, pages);
  gtk_widget_show_all (GTK_WIDGET (self));
  return self;
}

//-- methods

//-- wrapper

//-- class internals

static void
bt_settings_page_ui_dispose (GObject * object)
{
  BtSettingsPageUI *self = BT_SETTINGS_PAGE_UI (object);
  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);
  g_object_unref (self->priv->app);

  G_OBJECT_CLASS (bt_settings_page_ui_parent_class)->dispose (object);
}

static void
bt_settings_page_ui_finalize (GObject * object)
{
  BtSettingsPageUI *self = BT_SETTINGS_PAGE_UI (object);

  GST_DEBUG ("!!!! self=%p", self);

  G_OBJECT_CLASS (bt_settings_page_ui_parent_class)->finalize (object);
}

static void
bt_settings_page_ui_init (BtSettingsPageUI * self)
{
  self->priv = bt_settings_page_ui_get_instance_private(self);
  GST_DEBUG ("!!!! self=%p", self);
  self->priv->app = bt_edit_application_new ();
}

static void
bt_settings_page_ui_class_init (BtSettingsPageUIClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = bt_settings_page_ui_dispose;
  gobject_class->finalize = bt_settings_page_ui_finalize;
}
