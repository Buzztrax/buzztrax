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
  GdkPaintable *source_machine_texture;
  GdkPaintable *processor_machine_texture;
  GdkPaintable *sink_machine_texture;

  /* the keyboard shortcut table for the window */
  /// GTK4 GtkAccelGroup *accel_group;

  /* machine graphics */
  gdouble zoom;
  GdkPaintable *source_machine_textures[BT_MACHINE_STATE_COUNT];
  GdkPaintable *processor_machine_textures[BT_MACHINE_STATE_COUNT];
  GdkPaintable *sink_machine_textures[BT_MACHINE_STATE_COUNT];
  GdkPaintable *wire_texture[2];

  /* css provider */
  GtkStyleProvider *provider;

  GdkDisplay *display;
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
  gchar *style;
  gboolean use_dark, use_compact;

  g_object_get (settings, "dark-theme", &use_dark, "compact_theme",
      &use_compact, NULL);

  /* TODO(ensonic): compact:
   * - changes to the separators do not apply automatically right now
   */
  adw_style_manager_set_color_scheme(
      adw_style_manager_get_default (),
      use_dark ? ADW_COLOR_SCHEME_PREFER_DARK : ADW_COLOR_SCHEME_DEFAULT);

  /// GTK4 do authors still want to load files not compiled into resources?
  // also load from $srcdir (when uninstalled)
  // TODO: needs to be top_srcdir for out-of-srcdir builds
  style = g_strdup_printf ("/org/buzztrax/css/bt-edit.%s.%s.css",
      (use_dark ? "dark" : "light"), (use_compact ? "compact" : "normal"));

  gtk_css_provider_load_from_resource (
      GTK_CSS_PROVIDER (singleton->priv->provider), style);
  
  g_free (style);
}

//-- helper methods

static void
bt_ui_resources_init_icons (GdkDisplay* display)
{
  GtkSettings *settings;
  gchar *icon_theme_name;

  settings = gtk_settings_get_default ();
  g_object_get (settings,
      "gtk-icon-theme-name", &icon_theme_name, NULL);
  GST_INFO ("Icon Theme: %s", icon_theme_name);

  g_free (icon_theme_name);

  // https://developer.gnome.org/documentation/tutorials/themed-icons.html
  gtk_icon_theme_add_search_path (
      gtk_icon_theme_get_for_display (display),
      DATADIR "/org.buzztrax.Buzztrax/icons");
  
#if 0
  // TODO: needs to be top_srcdir for out-of-srcdir builds
  // TODO: don't show any effect :/
  gchar *src_dir = g_get_current_dir ();
  gchar *icon_dir = g_build_filename (src_dir, "icons/gnome", NULL);
  gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (), icon_dir);
  g_free (src_dir);
  g_free (icon_dir);
#endif

  singleton->priv->source_machine_texture =
      gdk_paintable_new_from_theme ("buzztrax_menu_source_machine", 16, display);
  singleton->priv->processor_machine_texture =
      gdk_paintable_new_from_theme ("buzztrax_menu_processor_machine", 16, display);
  singleton->priv->sink_machine_texture =
      gdk_paintable_new_from_theme ("buzztrax_menu_sink_machine", 16, display);

  GST_INFO ("images created");
}

static void
bt_ui_resources_free_graphics (BtUIResources * self)
{
  guint state;

  for (state = 0; state < BT_MACHINE_STATE_COUNT; state++) {
    g_object_try_unref (self->priv->source_machine_textures[state]);
    g_object_try_unref (self->priv->processor_machine_textures[state]);
    g_object_try_unref (self->priv->sink_machine_textures[state]);
  }
  g_object_try_unref (self->priv->wire_texture[0]);
  g_object_try_unref (self->priv->wire_texture[1]);
}

static void
bt_ui_resources_init_graphics (BtUIResources * self)
{
  // 12*6=72, 14*6=84
  const gint size =
      (gint) (self->priv->zoom * (gdouble) (48 * 14));
  //const gint size=(gint)(self->priv->zoom*(gdouble)(6*14));

  GST_INFO ("regenerating machine graphics at %d pixels", size);

  self->priv->source_machine_textures[BT_MACHINE_STATE_NORMAL] =
    gdk_paintable_new_from_theme ("buzztrax_generator", size, self->priv->display);
  self->priv->source_machine_textures[BT_MACHINE_STATE_MUTE] =
      gdk_paintable_new_from_theme ("buzztrax_generator_mute", size, self->priv->display);
  self->priv->source_machine_textures[BT_MACHINE_STATE_SOLO] =
      gdk_paintable_new_from_theme ("buzztrax_generator_solo", size, self->priv->display);

  self->priv->processor_machine_textures[BT_MACHINE_STATE_NORMAL] =
      gdk_paintable_new_from_theme ("buzztrax_effect", size, self->priv->display);
  self->priv->processor_machine_textures[BT_MACHINE_STATE_MUTE] =
      gdk_paintable_new_from_theme ("buzztrax_effect_mute", size, self->priv->display);
  self->priv->processor_machine_textures[BT_MACHINE_STATE_BYPASS] =
      gdk_paintable_new_from_theme ("buzztrax_effect_bypass", size, self->priv->display);

  self->priv->sink_machine_textures[BT_MACHINE_STATE_NORMAL] =
      gdk_paintable_new_from_theme ("buzztrax_master", size, self->priv->display);
  self->priv->sink_machine_textures[BT_MACHINE_STATE_MUTE] =
      gdk_paintable_new_from_theme ("buzztrax_master_mute", size, self->priv->display);

  self->priv->wire_texture[0] =
      gdk_paintable_new_from_theme ("buzztrax_wire", size * 2.0, self->priv->display);
  self->priv->wire_texture[1] =
      gdk_paintable_new_from_theme ("buzztrax_wire_nopan", size * 2.0, self->priv->display);

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
bt_ui_resources_new (GdkDisplay* display)
{
  BtUIResources *result = g_object_new (BT_TYPE_UI_RESOURCES, NULL);
  result->priv->display = display;

  // initialise ressources
  bt_ui_resources_init_icons (display);
  
  return result;
}

//-- methods

/**
 * bt_ui_resources_get_icon_texture_by_machine:
 * @machine: the machine to get the image for
 *
 * Gets a #GdkPaintable image that matches the given machine type for use in menus.
 *
 * Returns: a #GdkPaintable image
 */
GdkPaintable *
bt_ui_resources_get_icon_texture_by_machine (const BtMachine * machine)
{
  if (BT_IS_SOURCE_MACHINE (machine)) {
    return g_object_ref (singleton->priv->source_machine_texture);
  } else if (BT_IS_PROCESSOR_MACHINE (machine)) {
    return g_object_ref (singleton->priv->processor_machine_texture);
  } else if (BT_IS_SINK_MACHINE (machine)) {
    return g_object_ref (singleton->priv->sink_machine_texture);
  }
  return NULL;
}

/**
 * bt_ui_resources_get_machine_graphics_texture_by_machine:
 * @machine: the machine to get the image for
 * @zoom: scaling factor for the icons
 *
 * Returns: a a #GdkPaintable that matches the given machine type for use on the
 * canvas.
 */
GdkPaintable *
bt_ui_resources_get_machine_graphics_texture_by_machine (const BtMachine *
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
    return g_object_ref (singleton->priv->source_machine_textures[state]);
  } else if (BT_IS_PROCESSOR_MACHINE (machine)) {
    return g_object_ref (singleton->priv->processor_machine_textures[state]);
  } else if (BT_IS_SINK_MACHINE (machine)) {
    return g_object_ref (singleton->priv->sink_machine_textures[state]);
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
GdkPaintable *
bt_ui_resources_get_icon_paintable_by_machine (const BtMachine * machine)
{
  if (BT_IS_SOURCE_MACHINE (machine)) {
    return singleton->priv->source_machine_texture;
  } else if (BT_IS_PROCESSOR_MACHINE (machine)) {
    return singleton->priv->processor_machine_texture;
  } else if (BT_IS_SINK_MACHINE (machine)) {
    return singleton->priv->sink_machine_texture;
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
    return gtk_image_new_from_paintable (singleton->priv->source_machine_texture);
  } else if (machine_type == BT_TYPE_PROCESSOR_MACHINE) {
    return gtk_image_new_from_paintable (singleton->priv->
        processor_machine_texture);
  } else if (machine_type == BT_TYPE_SINK_MACHINE) {
    return gtk_image_new_from_paintable (singleton->priv->sink_machine_texture);
  }
  return NULL;
}

/**
 * bt_ui_resources_get_wire_graphics_texture_by_wire:
 * @wire: the wire to get the image for
 * @zoom: scaling factor for the icons
 *
 * Gets a #GdkPaintable image for use on the canvas.
 *
 * Returns: a #GdkPaintable image
 */
GdkPaintable *
bt_ui_resources_get_wire_graphics_paintable_by_wire (const BtWire * wire,
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

  return g_object_ref (singleton->priv->wire_texture[state]);
}

/**
 * bt_ui_resources_get_accel_group:
 *
 * All windows share one accelerator map.
 *
 * Returns: the shared keyboard accelerator map
 */
#if 0 /// GTK4
GtkAccelGroup *
bt_ui_resources_get_accel_group (void)
{
  return singleton->priv->accel_group;
}
#endif

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
      G_OBJECT_REF_COUNT (self->priv->source_machine_texture),
      G_OBJECT_REF_COUNT (self->priv->processor_machine_texture),
      G_OBJECT_REF_COUNT (self->priv->sink_machine_texture));
  g_object_try_unref (self->priv->source_machine_texture);
  g_object_try_unref (self->priv->processor_machine_texture);
  g_object_try_unref (self->priv->sink_machine_texture);

  bt_ui_resources_free_graphics (self);

  /// GTK4 g_object_try_unref (self->priv->accel_group);

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

    gtk_style_context_add_provider_for_display (gdk_display_get_default (),
        singleton->priv->provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    app = bt_edit_application_new ();
    g_object_get (app, "settings", &settings, NULL);

    // load our custom gtk-theming
    g_signal_connect (settings, "notify::dark-theme",
        G_CALLBACK (on_theme_notify), NULL);
    g_signal_connect (settings, "notify::compact-theme",
        G_CALLBACK (on_theme_notify), NULL);
    on_theme_notify (settings, NULL, NULL);
    /// GTK4 singleton->priv->accel_group = gtk_accel_group_new ();

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
