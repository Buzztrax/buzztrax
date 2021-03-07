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
 * SECTION:btsettingspageshortcuts
 * @short_description: keyboard shortcut settings page
 *
 * Edit and manage keyboard shortcut schemes.
 */
/* TODO(ensonic): we need a shortcut scheme management (buzz,fasttracker,..)
 * - select current scheme
 * - save edited scheme under new/current name
 *   - g_build_filename(g_get_user_data_dir(),PACKAGE,"shortcut-maps",NULL);
 *   - check the midi-profiles in lib/ic/learn.c for the gkeyfile handling
 *   - or use css:
 *     https://git.gnome.org/browse/gnome-builder/tree/data/keybindings/vim.css
 * - in the settings we just store the map name
 */
/* TODO(ensonic): we need a way to edit the current scheme
 * - we can look at the shortcut editor code from the gnome-control-center
 *   http://git.gnome.org/browse/gnome-control-center/tree/capplets/keybindings?h=gnome-2-32
 * - we need to ensure that keys are unique and not otherwise bound
 */
/* TODO(ensonic): allow to assign bt-ic trigger events as short cuts
 * - the list will have 3 columns : short-cut-name, key-command, controller-command
 * - when clicking the key-column, one can press a short cut or ESC
 *   - if a key is already bound, we bring up a dialog, where the user can confirm
 *     (and unassign the old function) or cancel
 * - when clicking the controller-command, we could offer a context menu, or
 *   just wait for the command as well.
 */

#define BT_EDIT
#define BT_SETTINGS_PAGE_SHORTCUTS_C

#include "bt-edit.h"

struct _BtSettingsPageShortcutsPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;
};

//-- the class

G_DEFINE_TYPE_WITH_CODE (BtSettingsPageShortcuts, bt_settings_page_shortcuts,
    GTK_TYPE_GRID, 
    G_ADD_PRIVATE(BtSettingsPageShortcuts));



//-- event handler

//-- helper methods

static void
bt_settings_page_shortcuts_init_ui (const BtSettingsPageShortcuts * self,
    GtkWidget * pages)
{
  BtSettings *settings;
  GtkWidget *label;
  gchar *str;

  gtk_widget_set_name (GTK_WIDGET (self), "keyboard shortcut settings");

  // get settings
  g_object_get (self->priv->app, "settings", &settings, NULL);
  //g_object_get(settings,NULL);

  // add setting widgets
  label = gtk_label_new (NULL);
  str = g_strdup_printf ("<big><b>%s</b></big>", _("Shortcuts"));
  gtk_label_set_markup (GTK_LABEL (label), str);
  g_free (str);
  g_object_set (label, "xalign", 0.0, NULL);
  gtk_grid_attach (GTK_GRID (self), label, 0, 0, 3, 1);
  gtk_grid_attach (GTK_GRID (self), gtk_label_new ("    "), 0, 1, 1, 2);

  /* FIXME(ensonic): add the UI */
  label = gtk_label_new ("no keyboard settings yet");
  g_object_set (label, "hexpand", TRUE, NULL);
  gtk_grid_attach (GTK_GRID (self), label, 1, 1, 2, 1);

  g_object_unref (settings);
}

//-- constructor methods

/**
 * bt_settings_page_shortcuts_new:
 * @pages: the page collection
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtSettingsPageShortcuts *
bt_settings_page_shortcuts_new (GtkWidget * pages)
{
  BtSettingsPageShortcuts *self;

  self =
      BT_SETTINGS_PAGE_SHORTCUTS (g_object_new (BT_TYPE_SETTINGS_PAGE_SHORTCUTS,
          NULL));
  bt_settings_page_shortcuts_init_ui (self, pages);
  gtk_widget_show_all (GTK_WIDGET (self));
  return self;
}

//-- methods

//-- wrapper

//-- class internals

static void
bt_settings_page_shortcuts_dispose (GObject * object)
{
  BtSettingsPageShortcuts *self = BT_SETTINGS_PAGE_SHORTCUTS (object);
  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);
  g_object_unref (self->priv->app);

  G_OBJECT_CLASS (bt_settings_page_shortcuts_parent_class)->dispose (object);
}

static void
bt_settings_page_shortcuts_finalize (GObject * object)
{
  BtSettingsPageShortcuts *self = BT_SETTINGS_PAGE_SHORTCUTS (object);

  GST_DEBUG ("!!!! self=%p", self);

  G_OBJECT_CLASS (bt_settings_page_shortcuts_parent_class)->finalize (object);
}

static void
bt_settings_page_shortcuts_init (BtSettingsPageShortcuts * self)
{
  self->priv = bt_settings_page_shortcuts_get_instance_private(self);
  GST_DEBUG ("!!!! self=%p", self);
  self->priv->app = bt_edit_application_new ();
}

static void
bt_settings_page_shortcuts_class_init (BtSettingsPageShortcutsClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = bt_settings_page_shortcuts_dispose;
  gobject_class->finalize = bt_settings_page_shortcuts_finalize;
}
