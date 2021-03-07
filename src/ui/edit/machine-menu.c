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
/**
 * SECTION:btmachinemenu
 * @short_description: class for the machine selection popup menu
 *
 * Builds a hierachical menu with usable machines from the GStreamer registry.
 */

#define BT_EDIT
#define BT_MACHINE_MENU_C

#include "bt-edit.h"

#include <gst/base/gstpushsrc.h>

//-- property ids

enum
{
  MACHINE_MENU_MACHINES_PAGE = 1
};

struct _BtMachineMenuPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* the machine page we belong to */
  BtMainPageMachines *main_page_machines;
};

//-- the class

G_DEFINE_TYPE_WITH_CODE (BtMachineMenu, bt_machine_menu, GTK_TYPE_MENU, 
    G_ADD_PRIVATE(BtMachineMenu));

//-- event handler

static void
on_source_machine_add_activated (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMachineMenu *self = BT_MACHINE_MENU (user_data);
  gchar *name, *id;

  name = (gchar *) gtk_widget_get_name (GTK_WIDGET (menuitem));
  id = (gchar *)
      gtk_label_get_text (GTK_LABEL (gtk_bin_get_child (GTK_BIN (menuitem))));

  GST_DEBUG ("adding source machine \"%s\" : \"%s\"", name, id);
  bt_main_page_machines_add_source_machine (self->priv->main_page_machines, id,
      name);
}

static void
on_processor_machine_add_activated (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMachineMenu *self = BT_MACHINE_MENU (user_data);
  gchar *name, *id;

  name = (gchar *) gtk_widget_get_name (GTK_WIDGET (menuitem));
  id = (gchar *)
      gtk_label_get_text (GTK_LABEL (gtk_bin_get_child (GTK_BIN (menuitem))));

  GST_DEBUG ("adding processor machine \"%s\"", name);
  bt_main_page_machines_add_processor_machine (self->priv->main_page_machines,
      id, name);
}

//-- helper methods

static gint
bt_machine_menu_compare (GstPluginFeature * f1, GstPluginFeature * f2)
{
#if 0
  // TODO(ensonic): this is fragmenting memory :/
  gchar *str1c = g_utf8_casefold (gst_plugin_feature_get_name (f1), -1);
  gchar *str2c = g_utf8_casefold (gst_plugin_feature_get_name (f2), -1);
  gint res = g_utf8_collate (str1c, str2c);

  g_free (str1c);
  g_free (str2c);
  return res;
#else
  return (strcasecmp (gst_plugin_feature_get_name (f1),
          gst_plugin_feature_get_name (f2)));
#endif
}

static gboolean
bt_machine_menu_check_pads (const GList * pads)
{
  const GList *node;
  gint pad_dir_ct[4] = { 0, };

  for (node = pads; node; node = g_list_next (node)) {
    pad_dir_ct[((GstStaticPadTemplate *) (node->data))->direction]++;
  }
  // skip everything with more that one src or sink pad
  if ((pad_dir_ct[GST_PAD_SRC] > 1) || (pad_dir_ct[GST_PAD_SINK] > 1)) {
    GST_DEBUG ("%d src pads, %d sink pads", pad_dir_ct[GST_PAD_SRC],
        pad_dir_ct[GST_PAD_SINK]);
    return FALSE;
  }
  return TRUE;
}

static int
blacklist_compare (const void *node1, const void *node2)
{
  //GST_DEBUG("comparing '%s' '%s'",*(gchar**)node1,*(gchar**)node2);
  return strcasecmp (*(gchar **) node1, *(gchar **) node2);
}

static void
bt_machine_menu_init_submenu (const BtMachineMenu * self, GtkWidget * submenu,
    const gchar * root, GCallback handler)
{
  GtkWidget *menu_item, *parentmenu;
  GList *node, *element_factories;
  GstElementFactory *factory;
  GstPluginFeature *loaded_feature;
  GHashTable *parent_menu_hash;
  const gchar *klass_name, *menu_name, *plugin_name, *factory_name;
  GType type;
  gboolean have_submenu;

  /* list of known gstreamer elements that are not useful under buzztrax,
   * but we can't detect otherwise */
  const gchar *blacklist[] = {
    /* - 'delay' and 'max-delay' are guint64 which is hard to handle in the UI
     *   also having it as ns instead of ms or sec (double) is impractical
     * - 'max-delay' is not controlable and therefore hard to discover in the UI
     * => we can make max-delay controlable to make this usable again
     */
    "audioecho",
    /* - 'amplification' is gdouble range, even a tick of a slider turn every
     *   signal into noise
     */
    "audioamplify",
    "audiorate",
    "dtmfsrc",
    "interaudiosrc",
    "memoryaudiosrc",
    "rglimiter",
    "rgvolume",
    "sfsrc",
    "spanplc"                   // comfort noise generator ?
  };

  // scan registered sources
  element_factories =
      bt_gst_registry_get_element_factories_matching_all_categories (root);
  parent_menu_hash =
      g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  // sort list by name
  // eventually first filter and sort remaining factories into a new list
  element_factories =
      g_list_sort (element_factories, (GCompareFunc) bt_machine_menu_compare);
  for (node = element_factories; node; node = g_list_next (node)) {
    factory = node->data;
    factory_name = gst_plugin_feature_get_name ((GstPluginFeature *) factory);

    // TODO(ensonic): we could also hide elements without controlable parameters,
    // that derive from basesrc, but not from pushsrc
    // this could help to hide "dtmfsrc" and "sfsrc"
    // for this it would be helpful to get a src/sink hint from the previous function
    // lets simply blacklist for now
    if (bsearch (&factory_name, blacklist, G_N_ELEMENTS (blacklist),
            sizeof (gchar *), blacklist_compare)) {
      GST_INFO ("skipping backlisted element : '%s'", factory_name);
      continue;
    }
    // skip elements with too many pads
    if (!(bt_machine_menu_check_pads
            (gst_element_factory_get_static_pad_templates (factory)))) {
      GST_INFO ("skipping uncompatible element : '%s'", factory_name);
      continue;
    }

    klass_name =
        gst_element_factory_get_metadata (factory, GST_ELEMENT_METADATA_KLASS);
    GST_LOG ("adding element : '%s' with classification: '%s'", factory_name,
        klass_name);

    // by default we would add the new element here
    parentmenu = submenu;
    have_submenu = FALSE;

    // add sub-menus for BML, LADSPA & Co.
    // remove prefix, e.g. 'Source/Audio'
    klass_name = &klass_name[strlen (root)];
    if (*klass_name) {
      GtkWidget *cached_menu;
      gchar **names;
      gchar *menu_path;
      gint i, len = 1;

      GST_LOG ("  subclass : '%s'", klass_name);

      // created nested menues
      names = g_strsplit (&klass_name[1], "/", 0);
      for (i = 0; i < g_strv_length (names); i++) {
        len += strlen (names[i]);
        menu_path = g_strndup (klass_name, len);
        //check in parent_menu_hash if we have a parent for this klass
        if (!(cached_menu =
                g_hash_table_lookup (parent_menu_hash, (gpointer) menu_path))) {
          GST_DEBUG ("    create new: '%s'", names[i]);
          menu_item = gtk_menu_item_new_with_label (names[i]);
          gtk_menu_shell_append (GTK_MENU_SHELL (parentmenu), menu_item);
          gtk_widget_show (menu_item);
          parentmenu = gtk_menu_new ();
          gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), parentmenu);
          g_hash_table_insert (parent_menu_hash, (gpointer) menu_path,
              (gpointer) parentmenu);
        } else {
          parentmenu = cached_menu;
          g_free (menu_path);
        }
      }
      g_strfreev (names);
      have_submenu = TRUE;
    }

    if (!have_submenu) {
      gchar *menu_path = NULL;
      // can we do something about bins (autoaudiosrc,gconfaudiosrc,halaudiosrc)
      // - having autoaudiosrc might be nice to have
      // - extra category?

      // add sub-menu for all audio inputs
      // get element type for filtering, this slows things down :/
      if (!(loaded_feature =
              gst_plugin_feature_load (GST_PLUGIN_FEATURE (factory)))) {
        GST_INFO ("skipping unloadable element : '%s'", factory_name);
        continue;
      }
      // presumably, we're no longer interested in the potentially-unloaded feature
      type = gst_element_factory_get_element_type ((GstElementFactory *)
          loaded_feature);
      // check class hierarchy
      if (g_type_is_a (type, GST_TYPE_PUSH_SRC)) {
        menu_path = "/Direct Input";
      } else if (g_type_is_a (type, GST_TYPE_BIN)) {
        menu_path = "/Abstract Input";
      }
      if (menu_path) {
        GtkWidget *cached_menu;

        GST_DEBUG ("  subclass : '%s'", &menu_path[1]);

        //check in parent_menu_hash if we have a parent for this klass
        if (!(cached_menu =
                g_hash_table_lookup (parent_menu_hash, (gpointer) menu_path))) {
          GST_DEBUG ("    create new: '%s'", &menu_path[1]);
          menu_item = gtk_menu_item_new_with_label (&menu_path[1]);
          gtk_menu_shell_append (GTK_MENU_SHELL (parentmenu), menu_item);
          gtk_widget_show (menu_item);
          parentmenu = gtk_menu_new ();
          gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), parentmenu);
          g_hash_table_insert (parent_menu_hash,
              (gpointer) g_strdup (menu_path), (gpointer) parentmenu);
        } else {
          parentmenu = cached_menu;
        }
        // uncomment if we add another filter
        //have_submenu=TRUE;
      }
      gst_object_unref (loaded_feature);
    }

    menu_name = factory_name;
    // cut plugin name from elemnt names for wrapper plugins
    // so, how can we detect wrapper plugins? -> we only filter, if plugin_name
    // is also in klass_name (for now we just check if klass_name is !empty)
    if (BT_IS_STRING (klass_name) &&
        (plugin_name =
            gst_plugin_feature_get_plugin_name (GST_PLUGIN_FEATURE (factory))))
    {
      gint len = strlen (plugin_name);

      GST_LOG ("%s:%s, %c", plugin_name, menu_name, menu_name[len]);

      // remove prefix "<plugin-name>-"
      if (!strncasecmp (menu_name, plugin_name, len)) {
        if (menu_name[len] == '-') {
          menu_name = &menu_name[len + 1];
        } else if (!strncasecmp (&menu_name[len], "src-", 4)) {
          menu_name = &menu_name[len + 4];
        } else if (!strncasecmp (&menu_name[len], "sink-", 5)) {
          menu_name = &menu_name[len + 5];
        }
      }
    }
    menu_item = gtk_menu_item_new_with_label (menu_name);
    gtk_widget_set_name (menu_item, factory_name);
    gtk_menu_shell_append (GTK_MENU_SHELL (parentmenu), menu_item);
    gtk_widget_show (menu_item);
    g_signal_connect (menu_item, "activate", G_CALLBACK (handler),
        (gpointer) self);
  }
  g_hash_table_destroy (parent_menu_hash);
  gst_plugin_feature_list_free (element_factories);
}

static void
bt_machine_menu_init_ui (const BtMachineMenu * self)
{
  GtkWidget *menu_item, *submenu;

  gtk_widget_set_name (GTK_WIDGET (self), "add machine menu");

  // generators
  menu_item = gtk_menu_item_new_with_label (_("Generators"));
  gtk_menu_shell_append (GTK_MENU_SHELL (self), menu_item);
  gtk_widget_show (menu_item);
  // add another submenu
  submenu = gtk_menu_new ();
  gtk_widget_set_name (submenu, "generators menu");
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), submenu);

  bt_machine_menu_init_submenu (self, submenu, "Source/Audio",
      G_CALLBACK (on_source_machine_add_activated));

  // effects
  menu_item = gtk_menu_item_new_with_label (_("Effects"));
  gtk_menu_shell_append (GTK_MENU_SHELL (self), menu_item);
  gtk_widget_show (menu_item);
  // add another submenu
  submenu = gtk_menu_new ();
  gtk_widget_set_name (submenu, "effects menu");
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), submenu);

  bt_machine_menu_init_submenu (self, submenu, "Filter/Effect/Audio",
      G_CALLBACK (on_processor_machine_add_activated));
}

//-- constructor methods

/**
 * bt_machine_menu_new:
 * @main_page_machines: the machine page for the menu
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtMachineMenu *
bt_machine_menu_new (const BtMainPageMachines * main_page_machines)
{
  BtMachineMenu *self;

  self =
      BT_MACHINE_MENU (g_object_new (BT_TYPE_MACHINE_MENU, "machines-page",
          main_page_machines, NULL));
  bt_machine_menu_init_ui (self);
  return self;
}

//-- methods


//-- class internals

static void
bt_machine_menu_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  BtMachineMenu *self = BT_MACHINE_MENU (object);
  return_if_disposed ();
  switch (property_id) {
    case MACHINE_MENU_MACHINES_PAGE:
      g_object_try_weak_unref (self->priv->main_page_machines);
      self->priv->main_page_machines =
          BT_MAIN_PAGE_MACHINES (g_value_get_object (value));
      g_object_try_weak_ref (self->priv->main_page_machines);
      //GST_DEBUG("set the main_page_machines for machine_menu: %p",self->priv->main_page_machines);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_machine_menu_dispose (GObject * object)
{
  BtMachineMenu *self = BT_MACHINE_MENU (object);
  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);
  g_object_try_weak_unref (self->priv->main_page_machines);
  g_object_unref (self->priv->app);

  G_OBJECT_CLASS (bt_machine_menu_parent_class)->dispose (object);
}

static void
bt_machine_menu_init (BtMachineMenu * self)
{
  self->priv = bt_machine_menu_get_instance_private(self);
  GST_DEBUG ("!!!! self=%p", self);
  self->priv->app = bt_edit_application_new ();
}

static void
bt_machine_menu_class_init (BtMachineMenuClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = bt_machine_menu_set_property;
  gobject_class->dispose = bt_machine_menu_dispose;

  g_object_class_install_property (gobject_class, MACHINE_MENU_MACHINES_PAGE,
      g_param_spec_object ("machines-page", "machines-page contruct prop",
          "Set application object, the window belongs to",
          BT_TYPE_MAIN_PAGE_MACHINES,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));
}
