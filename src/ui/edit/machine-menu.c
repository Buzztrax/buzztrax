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

struct _BtMachineMenu
{
  GObject parent;
  
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* the machine page we belong to */
  BtMainPageMachines *main_page_machines;

  GMenu *menu;
};

//-- the class

G_DEFINE_TYPE (BtMachineMenu, bt_machine_menu, G_TYPE_OBJECT);

//-- event handler

//-- helper methods

static gint
bt_machine_menu_compare (GstPluginFeature * f1, GstPluginFeature * f2)
{
  return (strcasecmp (gst_plugin_feature_get_name (f1),
          gst_plugin_feature_get_name (f2)));
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

// If @label is found in @labelset, then allocate a new unique label and
// return it. Otherwise, return a duplicate of @label.
// Return: (transfer full)
gchar *
make_menu_label_unique (const gchar *label, GHashTable *labelset)
{
  if (g_hash_table_contains (labelset, label)) {
    // Just pop a space on the end so it's "different", even if it doesn't
    // look it.
    gchar *newlabel = g_strdup_printf ("%s ", label);
    gchar *result = make_menu_label_unique (newlabel, labelset);
    g_free (newlabel);
    return result;
  } else {
    g_hash_table_insert (labelset, (gpointer) g_strdup (label), NULL);
    return g_strdup (label);
  }
}

// DB: I think there's an annoying GTK bug where Popovers have trouble with
// navigation when two different submenus share the same name. It also
// raises a GTK warning...
// This can happen for the two categories Source/Audio/LADSPA/Generators and
// Filter/Audio/LADSPA/Generators, for instance.
//
// So, labelset is used to keep track of the labels used and disambiguate
// them.
static void
bt_machine_menu_init_submenu (const BtMachineMenu * self, GMenu * submenu,
    const gchar * root, gboolean is_source, GHashTable * labelset)
{
  GMenu *parentmenu;
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
      g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
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
    GST_LOG ("trying add of element : '%s' with classification: '%s'", factory_name,
        klass_name);

    // by default we would add the new element here
    parentmenu = submenu;
    have_submenu = FALSE;

    // add sub-menus for BML, LADSPA & Co.
    // remove prefix, e.g. 'Source/Audio'
    GST_LOG ("  try klass_name : '%s' (root '%s')", klass_name, root);
    
    klass_name = &klass_name[strlen (root)];
    
    GST_LOG ("  de-rooted klass_name : '%s'", klass_name);

    const gboolean klass_consists_only_of_root = *klass_name == 0;
    
    if (!klass_consists_only_of_root) {
      GMenu *cached_menu;
      gchar **names;
      gchar *menu_path;
      gint i, len = 0;

      GST_LOG ("  subclass : '%s'", klass_name);

      // created nested menus
      names = g_strsplit (klass_name, "/", 0);
      for (i = 0; i < g_strv_length (names); i++) {
        guint namelen = strlen (names[i]);
        len += namelen;
        if (namelen == 0) {
          // In this case, klass_name has started with a slash.
          // +1 for separator, which is not included in "names[i]".
          // Not doing this will cause machine names to be truncated.
          len += 1; 
          continue;
        }

        menu_path = g_strndup (klass_name, len);

        // +1 for separator, which is not included in "names[i]"
        // Not doing this will cause machine names to be truncated.
        len += 1; 

        //check in parent_menu_hash if we have a parent for this klass
        if (!(cached_menu =
                g_hash_table_lookup (parent_menu_hash, (gpointer) menu_path))) {
          gchar *label = make_menu_label_unique (names[i], labelset);
          GST_DEBUG ("    create new: '%s' '%s'", menu_path, label);
          GMenu *m = g_menu_new ();
          g_menu_append_submenu (parentmenu, label, G_MENU_MODEL (m));
          g_free (label);
          parentmenu = m;
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
        GMenu *cached_menu;

        GST_DEBUG ("  subclass without submenu : '%s'", menu_path);

        //check in parent_menu_hash if we have a parent for this klass
        if (!(cached_menu =
                g_hash_table_lookup (parent_menu_hash, (gpointer) menu_path))) {
          gchar *label = make_menu_label_unique (&menu_path[1], labelset);
          GST_DEBUG ("    create new without submenu : '%s'", label);
          GMenu* m = g_menu_new ();
          g_menu_append_submenu (parentmenu, label, G_MENU_MODEL (m));
          g_free (label);
          parentmenu = m;
          g_hash_table_insert (parent_menu_hash,
              (gpointer) g_strdup (menu_path), (gpointer) parentmenu);
        } else {
          parentmenu = cached_menu;
        }
        // uncomment if we add another filter
        have_submenu=TRUE;
      }
      gst_object_unref (loaded_feature);
    }

    if (!have_submenu) {
      GST_DEBUG ("  no submenu created for factory '%s'", factory_name);
      parentmenu = submenu;
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

      GST_LOG ("  cut prefix from wrapper plugin name %s:%s, %c", plugin_name, menu_name, menu_name[len]);

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

    gchar* label = make_menu_label_unique (menu_name, labelset);
    GST_LOG ("  adding item '%s'", label);
    GMenuItem* item = g_menu_item_new (label, NULL);
    g_free (label);

    /// GTK4 although it's nice to express this using actions, a way should be
    /// found so that it's not necessary to regenerate it when x and y changes.
    /// Store a "machine last requested pos" variable in app and make x/y
    /// optional?
    g_menu_item_set_action_and_target (
        item,
        "app.machine.add",
        "(sidd)",
        factory_name,
        is_source ? 0 : 1,
        0, // x
        0  // y
    );

    g_menu_append_item (parentmenu, item);
    
    g_object_unref (item);
  }
  g_hash_table_destroy (parent_menu_hash);
  gst_plugin_feature_list_free (element_factories);
}

static void
bt_machine_menu_init_ui (BtMachineMenu * self)
{
  GMenu* menu = g_menu_new ();
  
  GMenu *submenu;

  // See comment in bt_machine_menu_init_submenu
  GHashTable *labelset = g_hash_table_new_full (g_str_hash, g_str_equal,
      g_free, NULL);
  
  g_hash_table_insert (labelset, g_strdup (_("Generators")), NULL);
  g_hash_table_insert (labelset, g_strdup (_("Effect")), NULL);
  
  // generators
  submenu = g_menu_new ();
  g_menu_append_submenu (menu, _("Generators"), G_MENU_MODEL (submenu));
  
  bt_machine_menu_init_submenu (self, submenu, "Source/Audio", TRUE, labelset);
  g_object_unref (submenu);


  // effects
  submenu = g_menu_new ();
  g_menu_append_submenu (menu, _("Effects"), G_MENU_MODEL (submenu));

  bt_machine_menu_init_submenu (self, submenu, "Filter/Effect/Audio", FALSE,
      labelset);

  g_hash_table_unref (labelset);  
  g_object_unref (submenu);

  self->menu = menu;
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

GMenu*
bt_machine_menu_get_menu(BtMachineMenu* self)
{
  return self->menu;
}

//-- class internals

static void
bt_machine_menu_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  BtMachineMenu *self = BT_MACHINE_MENU (object);
  return_if_disposed_self ();
  switch (property_id) {
    case MACHINE_MENU_MACHINES_PAGE:
      g_object_try_weak_unref (self->main_page_machines);
      self->main_page_machines =
          BT_MAIN_PAGE_MACHINES (g_value_get_object (value));
      g_object_try_weak_ref (self->main_page_machines);
      //GST_DEBUG("set the main_page_machines for machine_menu: %p",self->main_page_machines);
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
  return_if_disposed_self ();
  self->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);
  g_object_try_weak_unref (self->main_page_machines);
  g_object_unref (self->menu);
  g_object_unref (self->app);

  G_OBJECT_CLASS (bt_machine_menu_parent_class)->dispose (object);
}

static void
bt_machine_menu_init (BtMachineMenu * self)
{
  self = bt_machine_menu_get_instance_private(self);
  GST_DEBUG ("!!!! self=%p", self);
  self->app = bt_edit_application_new ();
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
