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
 * SECTION:btuiresources
 * @short_description: common shared ui resources like icons and colors
 *
 * This class serves as a central storage for colors and icons.
 * It is implemented as a singleton.
 */

#define BT_EDIT
#define BT_UI_RESOURCES_C

#include "bt-edit.h"

struct _BtUIResourcesPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* icons */
  GdkPixbuf *source_machine_pixbuf;
  GdkPixbuf *processor_machine_pixbuf;
  GdkPixbuf *sink_machine_pixbuf;

  /* the keyboard shortcut table for the window */
  GtkAccelGroup *accel_group;

  /* machine graphics */
  gdouble zoom;
  GdkPixbuf *source_machine_pixbufs[BT_MACHINE_STATE_COUNT];
  GdkPixbuf *processor_machine_pixbufs[BT_MACHINE_STATE_COUNT];
  GdkPixbuf *sink_machine_pixbufs[BT_MACHINE_STATE_COUNT];
  GdkPixbuf *wire_pixbuf[2];

  /* css provider */
  GtkStyleProvider *provider;
};

static BtUIResources *singleton = NULL;

//-- the class

G_DEFINE_TYPE_WITH_CODE (BtUIResources, bt_ui_resources, G_TYPE_OBJECT, 
    G_ADD_PRIVATE(BtUIResources));

//-- event handler

static void
on_theme_notify (BtSettings * const settings, GParamSpec * const arg,
    gconstpointer user_data)
{
  GError *err = NULL;
  gchar *style;
  gboolean use_dark, use_compact;

  g_object_get (settings, "dark-theme", &use_dark, "compact_theme",
      &use_compact, NULL);

  /* TODO(ensonic): compact:
   * - changes to the separators do not apply automatically right now
   */
  g_object_set (gtk_settings_get_default (),
      "gtk-application-prefer-dark-theme", use_dark, NULL);

  // also load from $srcdir (when uninstalled)
  // TODO: needs to be top_srcdir for out-of-srcdir builds
  style = g_strdup_printf ("src" G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S
      "edit" G_DIR_SEPARATOR_S "bt-edit.%s.%s.css",
      (use_dark ? "dark" : "light"), (use_compact ? "compact" : "normal"));
  if (!g_file_test (style, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR)) {
    g_free (style);
    style = g_strdup_printf (DATADIR ""
        G_DIR_SEPARATOR_S "" PACKAGE "" G_DIR_SEPARATOR_S "bt-edit.%s.%s.css",
        (use_dark ? "dark" : "light"), (use_compact ? "compact" : "normal"));
  }

  if (!gtk_css_provider_load_from_path (GTK_CSS_PROVIDER (singleton->
              priv->provider), style, &err)) {
    g_print ("Error loading css: %s\n", safe_string (err->message));
    g_error_free (err);
    err = NULL;
  }
  g_free (style);
}

//-- helper methods

static void
bt_ui_resources_init_icons (void)
{
  GtkSettings *settings;
  gchar *icon_theme_name, *fallback_icon_theme_name;
  gint w, h;

  settings = gtk_settings_get_default ();
  g_object_get (settings,
      "gtk-icon-theme-name", &icon_theme_name,
      "gtk-fallback-icon-theme", &fallback_icon_theme_name, NULL);
  GST_INFO ("Icon Theme: %s, Fallback Icon Theme: %s", icon_theme_name,
      fallback_icon_theme_name);

  if (strcasecmp (icon_theme_name, "gnome")) {
    if (fallback_icon_theme_name
        && strcasecmp (fallback_icon_theme_name, "gnome")) {
      g_object_set (settings, "gtk-fallback-icon-theme", "gnome", NULL);
    }
  }
  g_free (icon_theme_name);
  g_free (fallback_icon_theme_name);

#if 0
  // TODO: needs to be top_srcdir for out-of-srcdir builds
  // TODO: don't show any effect :/
  gchar *src_dir = g_get_current_dir ();
  gchar *icon_dir = g_build_filename (src_dir, "icons/gnome", NULL);
  gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (), icon_dir);
  g_free (src_dir);
  g_free (icon_dir);
#endif

  gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &w, &h);

  singleton->priv->source_machine_pixbuf =
      gdk_pixbuf_new_from_theme ("buzztrax_menu_source_machine", w);
  singleton->priv->processor_machine_pixbuf =
      gdk_pixbuf_new_from_theme ("buzztrax_menu_processor_machine", w);
  singleton->priv->sink_machine_pixbuf =
      gdk_pixbuf_new_from_theme ("buzztrax_menu_sink_machine", w);

  GST_INFO ("images created");
}

static void
bt_ui_resources_free_graphics (BtUIResources * self)
{
  guint state;

  for (state = 0; state < BT_MACHINE_STATE_COUNT; state++) {
    g_object_try_unref (self->priv->source_machine_pixbufs[state]);
    g_object_try_unref (self->priv->processor_machine_pixbufs[state]);
    g_object_try_unref (self->priv->sink_machine_pixbufs[state]);
  }
  g_object_try_unref (self->priv->wire_pixbuf[0]);
  g_object_try_unref (self->priv->wire_pixbuf[1]);
}

static void
bt_ui_resources_init_graphics (BtUIResources * self)
{
  // 12*6=72, 14*6=84
  const gint size =
      (gint) (self->priv->zoom * (gdouble) (GTK_ICON_SIZE_DIALOG * 14));
  //const gint size=(gint)(self->priv->zoom*(gdouble)(6*14));

  GST_INFO ("regenerating machine graphics at %d pixels", size);

  self->priv->source_machine_pixbufs[BT_MACHINE_STATE_NORMAL] =
      gdk_pixbuf_new_from_theme ("buzztrax_generator", size);
  self->priv->source_machine_pixbufs[BT_MACHINE_STATE_MUTE] =
      gdk_pixbuf_new_from_theme ("buzztrax_generator_mute", size);
  self->priv->source_machine_pixbufs[BT_MACHINE_STATE_SOLO] =
      gdk_pixbuf_new_from_theme ("buzztrax_generator_solo", size);

  self->priv->processor_machine_pixbufs[BT_MACHINE_STATE_NORMAL] =
      gdk_pixbuf_new_from_theme ("buzztrax_effect", size);
  self->priv->processor_machine_pixbufs[BT_MACHINE_STATE_MUTE] =
      gdk_pixbuf_new_from_theme ("buzztrax_effect_mute", size);
  self->priv->processor_machine_pixbufs[BT_MACHINE_STATE_BYPASS] =
      gdk_pixbuf_new_from_theme ("buzztrax_effect_bypass", size);

  self->priv->sink_machine_pixbufs[BT_MACHINE_STATE_NORMAL] =
      gdk_pixbuf_new_from_theme ("buzztrax_master", size);
  self->priv->sink_machine_pixbufs[BT_MACHINE_STATE_MUTE] =
      gdk_pixbuf_new_from_theme ("buzztrax_master_mute", size);

  self->priv->wire_pixbuf[0] =
      gdk_pixbuf_new_from_theme ("buzztrax_wire", size * 2.0);
  self->priv->wire_pixbuf[1] =
      gdk_pixbuf_new_from_theme ("buzztrax_wire_nopan", size * 2.0);

  /* DEBUG
     gint w,h;
     g_object_get(self->priv->source_machine_pixbufs[BT_MACHINE_STATE_NORMAL],"width",&w,"height",&h,NULL);
     GST_DEBUG("svg: w,h = %d, %d",w,h);
   */
}

//-- constructor methods

/**
 * bt_ui_resources_new:
 *
 * Create a new instance on first call and return a reference later on.
 *
 * Returns: the new signleton instance
 */
BtUIResources *
bt_ui_resources_new (void)
{
  return g_object_new (BT_TYPE_UI_RESOURCES, NULL);
}

//-- methods

/**
 * bt_ui_resources_get_icon_pixbuf_by_machine:
 * @machine: the machine to get the image for
 *
 * Gets a #GdkPixbuf image that matches the given machine type for use in menus.
 *
 * Returns: a #GdkPixbuf image
 */
GdkPixbuf *
bt_ui_resources_get_icon_pixbuf_by_machine (const BtMachine * machine)
{
  if (BT_IS_SOURCE_MACHINE (machine)) {
    return g_object_ref (singleton->priv->source_machine_pixbuf);
  } else if (BT_IS_PROCESSOR_MACHINE (machine)) {
    return g_object_ref (singleton->priv->processor_machine_pixbuf);
  } else if (BT_IS_SINK_MACHINE (machine)) {
    return g_object_ref (singleton->priv->sink_machine_pixbuf);
  }
  return NULL;
}

/**
 * bt_ui_resources_get_machine_graphics_pixbuf_by_machine:
 * @machine: the machine to get the image for
 * @zoom: scaling factor for the icons
 *
 * Gets a #GdkPixbuf image that matches the given machine type for use on the
 * canvas.
 *
 * Returns: a #GdkPixbuf image
 */
GdkPixbuf *
bt_ui_resources_get_machine_graphics_pixbuf_by_machine (const BtMachine *
    machine, gdouble zoom)
{
  BtMachineState state;

  if (zoom != singleton->priv->zoom) {
    GST_DEBUG ("change zoom %f -> %f", singleton->priv->zoom, zoom);
    bt_ui_resources_free_graphics (singleton);
    singleton->priv->zoom = zoom;
    bt_ui_resources_init_graphics (singleton);
  }

  g_object_get ((gpointer) machine, "state", &state, NULL);

  if (BT_IS_SOURCE_MACHINE (machine)) {
    return g_object_ref (singleton->priv->source_machine_pixbufs[state]);
  } else if (BT_IS_PROCESSOR_MACHINE (machine)) {
    return g_object_ref (singleton->priv->processor_machine_pixbufs[state]);
  } else if (BT_IS_SINK_MACHINE (machine)) {
    return g_object_ref (singleton->priv->sink_machine_pixbufs[state]);
  }
  return NULL;
}

/**
 * bt_ui_resources_get_icon_image_by_machine:
 * @machine: the machine to get the image for
 *
 * Gets a #GtkImage that matches the given machine type.
 *
 * Returns: a #GtkImage widget
 */
GtkWidget *
bt_ui_resources_get_icon_image_by_machine (const BtMachine * machine)
{
  if (BT_IS_SOURCE_MACHINE (machine)) {
    return gtk_image_new_from_pixbuf (singleton->priv->source_machine_pixbuf);
  } else if (BT_IS_PROCESSOR_MACHINE (machine)) {
    return gtk_image_new_from_pixbuf (singleton->
        priv->processor_machine_pixbuf);
  } else if (BT_IS_SINK_MACHINE (machine)) {
    return gtk_image_new_from_pixbuf (singleton->priv->sink_machine_pixbuf);
  }
  return NULL;
}

/**
 * bt_ui_resources_get_icon_image_by_machine_type:
 * @machine_type: the machine_type to get the image for
 *
 * Gets a #GtkImage that matches the given machine type.
 *
 * Returns: a #GtkImage widget
 */
GtkWidget *
bt_ui_resources_get_icon_image_by_machine_type (GType machine_type)
{
  if (machine_type == BT_TYPE_SOURCE_MACHINE) {
    return gtk_image_new_from_pixbuf (singleton->priv->source_machine_pixbuf);
  } else if (machine_type == BT_TYPE_PROCESSOR_MACHINE) {
    return gtk_image_new_from_pixbuf (singleton->priv->
        processor_machine_pixbuf);
  } else if (machine_type == BT_TYPE_SINK_MACHINE) {
    return gtk_image_new_from_pixbuf (singleton->priv->sink_machine_pixbuf);
  }
  return NULL;
}

/**
 * bt_ui_resources_get_wire_graphics_pixbuf_by_wire:
 * @wire: the wire to get the image for
 * @zoom: scaling factor for the icons
 *
 * Gets a #GdkPixbuf image for use on the canvas.
 *
 * Returns: a #GdkPixbuf image
 */
GdkPixbuf *
bt_ui_resources_get_wire_graphics_pixbuf_by_wire (const BtWire * wire,
    gdouble zoom)
{
  GstElement *wire_pan;
  guint state = 0;

  g_object_get ((GObject *) wire, "pan", &wire_pan, NULL);
  if (wire_pan) {
    g_object_unref (wire_pan);
  } else {
    state = 1;
  }

  /*
     if (zoom != singleton->priv->zoom) {
     GST_DEBUG ("change zoom %f -> %f", singleton->priv->zoom, zoom);
     bt_ui_resources_free_graphics (singleton);
     singleton->priv->zoom = zoom;
     bt_ui_resources_init_graphics (singleton);
     }
   */

  return g_object_ref (singleton->priv->wire_pixbuf[state]);
}

/**
 * bt_ui_resources_get_accel_group:
 *
 * All windows share one accelerator map.
 *
 * Returns: the shared keyboard accelerator map
 */
GtkAccelGroup *
bt_ui_resources_get_accel_group (void)
{
  return singleton->priv->accel_group;
}

//-- wrapper

//-- class internals

static void
bt_ui_resources_dispose (GObject * object)
{
  BtUIResources *self = BT_UI_RESOURCES (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  GST_DEBUG ("  pb->ref_cts: %d, %d, %d",
      G_OBJECT_REF_COUNT (self->priv->source_machine_pixbuf),
      G_OBJECT_REF_COUNT (self->priv->processor_machine_pixbuf),
      G_OBJECT_REF_COUNT (self->priv->sink_machine_pixbuf));
  g_object_try_unref (self->priv->source_machine_pixbuf);
  g_object_try_unref (self->priv->processor_machine_pixbuf);
  g_object_try_unref (self->priv->sink_machine_pixbuf);

  bt_ui_resources_free_graphics (self);

  g_object_try_unref (self->priv->accel_group);

  G_OBJECT_CLASS (bt_ui_resources_parent_class)->dispose (object);
}

static GObject *
bt_ui_resources_constructor (GType type, guint n_construct_params,
    GObjectConstructParam * construct_params)
{
  GObject *object;

  if (G_UNLIKELY (!singleton)) {
    BtEditApplication *app;
    BtSettings *settings;

    object =
        G_OBJECT_CLASS (bt_ui_resources_parent_class)->constructor (type,
        n_construct_params, construct_params);
    singleton = BT_UI_RESOURCES (object);
    g_object_add_weak_pointer (object, (gpointer *) (gpointer) & singleton);

    singleton->priv->provider = (GtkStyleProvider *) gtk_css_provider_new ();
    gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
        singleton->priv->provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    app = bt_edit_application_new ();
    g_object_get (app, "settings", &settings, NULL);

    // load our custom gtk-theming
    g_signal_connect (settings, "notify::dark-theme",
        G_CALLBACK (on_theme_notify), NULL);
    g_signal_connect (settings, "notify::compact-theme",
        G_CALLBACK (on_theme_notify), NULL);
    on_theme_notify (settings, NULL, NULL);
    // initialise ressources
    bt_ui_resources_init_icons ();
    singleton->priv->accel_group = gtk_accel_group_new ();

    g_object_unref (settings);
    g_object_unref (app);
  } else {
    object = g_object_ref ((gpointer) singleton);
  }
  return object;
}

static void
bt_ui_resources_init (BtUIResources * self)
{
  self->priv = bt_ui_resources_get_instance_private(self);
}

static void
bt_ui_resources_class_init (BtUIResourcesClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructor = bt_ui_resources_constructor;
  gobject_class->dispose = bt_ui_resources_dispose;
}
