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

/* TODO(ensonic): manage GdkColor entries and guint colors (via gdk_color.pixel)
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

  /* colors */
  GdkColor colors[BT_UI_RES_COLOR_COUNT];

  /* the keyboard shortcut table for the window */
  GtkAccelGroup *accel_group;

  /* machine graphics */
  gdouble zoom;
  GdkPixbuf *source_machine_pixbufs[BT_MACHINE_STATE_COUNT];
  GdkPixbuf *processor_machine_pixbufs[BT_MACHINE_STATE_COUNT];
  GdkPixbuf *sink_machine_pixbufs[BT_MACHINE_STATE_COUNT];
  GdkPixbuf *wire_pixbuf[2];
};

static BtUIResources *singleton = NULL;

//-- the class

G_DEFINE_TYPE (BtUIResources, bt_ui_resources, G_TYPE_OBJECT);

//-- event handler

//-- helper methods

#define MAKE_COLOR_FROM_FLOATS(ix,r,g,b) \
  self->priv->colors[ix].red=  (guint16)(r*65535); \
  self->priv->colors[ix].green=(guint16)(g*65535); \
  self->priv->colors[ix].blue= (guint16)(b*65535)

#define MAKE_COLOR_FROM_HEX(ix,r,g,b) \
  self->priv->colors[ix].red=  (guint16)((r*256)); \
  self->priv->colors[ix].green=(guint16)((g*256)); \
  self->priv->colors[ix].blue= (guint16)((b*256))

#define MAKE_COLOR_FROM_HEX_MIX(ix,r1,g1,b1,r2,g2,b2) \
  self->priv->colors[ix].red=  (guint16)((r2+((r1-r2)>>1))*256); \
  self->priv->colors[ix].green=(guint16)((g2+((g1-g2)>>1))*256); \
  self->priv->colors[ix].blue= (guint16)((b2+((b1-b2)>>1))*256)

static void
bt_ui_resources_init_colors (BtUIResources * self)
{
  GtkSettings *settings;
  gchar *icon_theme_name;
  gboolean use_tango_colors = FALSE;

  settings = gtk_settings_get_default ();
  /* get the theme name - we need different machine colors for tango
   * http://tango.freedesktop.org/Tango_Icon_Theme_Guidelines
   * http://tango.freedesktop.org/static/cvs/tango-art-tools/palettes/Tango-Palette.gpl
   */
  g_object_get (settings, "gtk-icon-theme-name", &icon_theme_name, NULL);
  GST_INFO ("Icon Theme: %s", icon_theme_name);

  /* TODO(ensonic): can we get some colors from the theme ?
   * gtk_widget_style_get(widget,
   *   "cursor-color",&self->priv->colors[ix],
   *   "secondary-cursor-color",&self->priv->colors[ix],
   *   NULL);
   */

  if (!strcasecmp (icon_theme_name, "tango")
      || !strcasecmp (icon_theme_name, "gnome"))
    use_tango_colors = TRUE;

  // cursor
  MAKE_COLOR_FROM_FLOATS (BT_UI_RES_COLOR_CURSOR, 0.85, 0.85, 0.20);

  // selection background
  MAKE_COLOR_FROM_FLOATS (BT_UI_RES_COLOR_SELECTION1, 1.00, 1.00, 0.60);
  MAKE_COLOR_FROM_FLOATS (BT_UI_RES_COLOR_SELECTION2, 0.95, 0.95, 0.55);

  // tree view lines
  MAKE_COLOR_FROM_FLOATS (BT_UI_RES_COLOR_PLAYLINE, 0.00, 0.00, 1.00);
  MAKE_COLOR_FROM_FLOATS (BT_UI_RES_COLOR_LOOPLINE, 1.00, 0.75, 0.00);
  MAKE_COLOR_FROM_FLOATS (BT_UI_RES_COLOR_ENDLINE, 1.00, 0.30, 0.20);

  // source machine
  if (use_tango_colors) {
    /* (#ffd699) #fcaf3e / #f57900 / #ce5c00
     * 252 175  62      Orange 1
     * 245 121   0      Orange 2
     * 206  92   0      Orange 3
     */
    MAKE_COLOR_FROM_HEX (BT_UI_RES_COLOR_SOURCE_MACHINE_BASE, 0xf5, 0x79, 0x00);
    //MAKE_COLOR_FROM_HEX(BT_UI_RES_COLOR_SOURCE_MACHINE_BRIGHT1,       0xfc,0xaf,0x3e);
    //MAKE_COLOR_FROM_HEX_MIX(BT_UI_RES_COLOR_SOURCE_MACHINE_BRIGHT2,   0xfc,0xaf,0x3e,0xf5,0x79,0x00);
    MAKE_COLOR_FROM_HEX (BT_UI_RES_COLOR_SOURCE_MACHINE_BRIGHT1, 0xff, 0xd6,
        0x99);
    MAKE_COLOR_FROM_HEX_MIX (BT_UI_RES_COLOR_SOURCE_MACHINE_BRIGHT2, 0xff, 0xd6,
        0x99, 0xf5, 0x79, 0x00);
    MAKE_COLOR_FROM_HEX_MIX (BT_UI_RES_COLOR_SOURCE_MACHINE_DARK1, 0xce, 0x5c,
        0x00, 0xf5, 0x79, 0x00);
    MAKE_COLOR_FROM_HEX (BT_UI_RES_COLOR_SOURCE_MACHINE_DARK2, 0xce, 0x5c,
        0x00);
  } else {
    MAKE_COLOR_FROM_FLOATS (BT_UI_RES_COLOR_SOURCE_MACHINE_BASE, 1.00, 0.60,
        0.60);
    MAKE_COLOR_FROM_FLOATS (BT_UI_RES_COLOR_SOURCE_MACHINE_BRIGHT1, 1.00, 0.90,
        0.90);
    MAKE_COLOR_FROM_FLOATS (BT_UI_RES_COLOR_SOURCE_MACHINE_BRIGHT2, 1.00, 0.80,
        0.80);
    MAKE_COLOR_FROM_FLOATS (BT_UI_RES_COLOR_SOURCE_MACHINE_DARK1, 0.60, 0.40,
        0.40);
    MAKE_COLOR_FROM_FLOATS (BT_UI_RES_COLOR_SOURCE_MACHINE_DARK2, 0.50, 0.20,
        0.20);
  }

  // processor machine
  if (use_tango_colors) {
    /* (#cbff99) #8ae234 / #73d216 / #4e9a06
     * 138 226  52      Chameleon 1
     * 115 210  22      Chameleon 2
     *  78 154   6      Chameleon 3
     */
    MAKE_COLOR_FROM_HEX (BT_UI_RES_COLOR_PROCESSOR_MACHINE_BASE, 0x73, 0xd2,
        0x16);
    //MAKE_COLOR_FROM_HEX(BT_UI_RES_COLOR_PROCESSOR_MACHINE_BRIGHT1,    0x8a,0xe2,0x34);
    //MAKE_COLOR_FROM_HEX_MIX(BT_UI_RES_COLOR_PROCESSOR_MACHINE_BRIGHT2,0x8a,0xe2,0x34,0x73,0xd2,0x16);
    MAKE_COLOR_FROM_HEX (BT_UI_RES_COLOR_PROCESSOR_MACHINE_BRIGHT1, 0xcb, 0xff,
        0x99);
    MAKE_COLOR_FROM_HEX_MIX (BT_UI_RES_COLOR_PROCESSOR_MACHINE_BRIGHT2, 0xcb,
        0xff, 0x99, 0x73, 0xd2, 0x16);
    MAKE_COLOR_FROM_HEX_MIX (BT_UI_RES_COLOR_PROCESSOR_MACHINE_DARK1, 0x4e,
        0x9a, 0x06, 0x73, 0xd2, 0x16);
    MAKE_COLOR_FROM_HEX (BT_UI_RES_COLOR_PROCESSOR_MACHINE_DARK2, 0x4e, 0x9a,
        0x06);
  } else {
    MAKE_COLOR_FROM_FLOATS (BT_UI_RES_COLOR_PROCESSOR_MACHINE_BASE, 0.60, 1.00,
        0.60);
    MAKE_COLOR_FROM_FLOATS (BT_UI_RES_COLOR_PROCESSOR_MACHINE_BRIGHT1, 0.90,
        1.00, 0.90);
    MAKE_COLOR_FROM_FLOATS (BT_UI_RES_COLOR_PROCESSOR_MACHINE_BRIGHT2, 0.80,
        1.00, 0.80);
    MAKE_COLOR_FROM_FLOATS (BT_UI_RES_COLOR_PROCESSOR_MACHINE_DARK1, 0.40, 0.60,
        0.40);
    MAKE_COLOR_FROM_FLOATS (BT_UI_RES_COLOR_PROCESSOR_MACHINE_DARK2, 0.20, 0.50,
        0.20);
  }

  // sink machine
  if (use_tango_colors) {
    /* (#99caff) #729fcf / #3465a4 / #204a87
     * 114 159 207      Sky Blue 1 [62 58 43]
     *  52 101 164      Sky Blue 2
     *  32  74 135      Sky Blue 3 [20 27 29]
     */
    MAKE_COLOR_FROM_HEX (BT_UI_RES_COLOR_SINK_MACHINE_BASE, 0x34, 0x65, 0xa4);
    //MAKE_COLOR_FROM_HEX(BT_UI_RES_COLOR_SINK_MACHINE_BRIGHT1,         0x72,0x9f,0xcf);
    //MAKE_COLOR_FROM_HEX_MIX(BT_UI_RES_COLOR_SINK_MACHINE_BRIGHT2,     0x72,0x9f,0xcf,0x34,0x65,0xa4);
    MAKE_COLOR_FROM_HEX (BT_UI_RES_COLOR_SINK_MACHINE_BRIGHT1, 0x99, 0xca,
        0xff);
    MAKE_COLOR_FROM_HEX_MIX (BT_UI_RES_COLOR_SINK_MACHINE_BRIGHT2, 0x99, 0xca,
        0xff, 0x34, 0x65, 0xa4);
    MAKE_COLOR_FROM_HEX_MIX (BT_UI_RES_COLOR_SINK_MACHINE_DARK1, 0x20, 0x4a,
        0x87, 0x34, 0x65, 0xa4);
    MAKE_COLOR_FROM_HEX (BT_UI_RES_COLOR_SINK_MACHINE_DARK2, 0x20, 0x4a, 0x87);
  } else {
    MAKE_COLOR_FROM_FLOATS (BT_UI_RES_COLOR_SINK_MACHINE_BASE, 0.60, 0.60,
        1.00);
    MAKE_COLOR_FROM_FLOATS (BT_UI_RES_COLOR_SINK_MACHINE_BRIGHT1, 0.90, 0.90,
        1.00);
    MAKE_COLOR_FROM_FLOATS (BT_UI_RES_COLOR_SINK_MACHINE_BRIGHT2, 0.80, 0.80,
        1.00);
    MAKE_COLOR_FROM_FLOATS (BT_UI_RES_COLOR_SINK_MACHINE_DARK1, 0.40, 0.40,
        0.60);
    MAKE_COLOR_FROM_FLOATS (BT_UI_RES_COLOR_SINK_MACHINE_DARK2, 0.20, 0.20,
        0.50);
  }

  // analyzer window
  MAKE_COLOR_FROM_FLOATS (BT_UI_RES_COLOR_ANALYZER_DECAY, 1.00, 0.75, 0.00);
  MAKE_COLOR_FROM_FLOATS (BT_UI_RES_COLOR_GRID_LINES, 0.5, 0.5, 0.5);

  g_free (icon_theme_name);
  GST_INFO ("colors created");
}


static void
bt_ui_resources_init_icons (BtUIResources * self)
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


  gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &w, &h);

  /*
     self->priv->source_machine_pixbuf=gdk_pixbuf_new_from_filename("buzztrax_menu_source_machine.png");
     self->priv->processor_machine_pixbuf=gdk_pixbuf_new_from_filename("buzztrax_menu_processor_machine.png");
     self->priv->sink_machine_pixbuf=gdk_pixbuf_new_from_filename("buzztrax_menu_sink_machine.png");
   */
  self->priv->source_machine_pixbuf =
      gdk_pixbuf_new_from_theme ("buzztrax_menu_source_machine", w);
  self->priv->processor_machine_pixbuf =
      gdk_pixbuf_new_from_theme ("buzztrax_menu_processor_machine", w);
  self->priv->sink_machine_pixbuf =
      gdk_pixbuf_new_from_theme ("buzztrax_menu_sink_machine", w);

  GST_INFO ("images created");
}

/*
static GdkPixbuf *bt_ui_resources_load_svg(const gchar *file_name) {
  GError *error = NULL;
  GdkPixbuf *pixbuf;

  pixbuf = rsvg_pixbuf_from_file_at_size (file_name, 96, 96, &error);
  if(error) {
    GST_WARNING ("loading svg %s failed : %s", file_name, error->message);
    g_error_free(error);
  }
  return(pixbuf);
}
*/

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

  //self->priv->source_machine_pixbufs[BT_MACHINE_STATE_NORMAL] = bt_ui_resources_load_svg ("generator.svg");

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
  return (g_object_new (BT_TYPE_UI_RESOURCES, NULL));
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
    return (g_object_ref (singleton->priv->source_machine_pixbuf));
  } else if (BT_IS_PROCESSOR_MACHINE (machine)) {
    return (g_object_ref (singleton->priv->processor_machine_pixbuf));
  } else if (BT_IS_SINK_MACHINE (machine)) {
    return (g_object_ref (singleton->priv->sink_machine_pixbuf));
  }
  return (NULL);
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
    return (g_object_ref (singleton->priv->source_machine_pixbufs[state]));
  } else if (BT_IS_PROCESSOR_MACHINE (machine)) {
    return (g_object_ref (singleton->priv->processor_machine_pixbufs[state]));
  } else if (BT_IS_SINK_MACHINE (machine)) {
    return (g_object_ref (singleton->priv->sink_machine_pixbufs[state]));
  }
  return (NULL);
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
    return (gtk_image_new_from_pixbuf (singleton->priv->source_machine_pixbuf));
  } else if (BT_IS_PROCESSOR_MACHINE (machine)) {
    return (gtk_image_new_from_pixbuf (singleton->priv->
            processor_machine_pixbuf));
  } else if (BT_IS_SINK_MACHINE (machine)) {
    return (gtk_image_new_from_pixbuf (singleton->priv->sink_machine_pixbuf));
  }
  return (NULL);
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
    return (gtk_image_new_from_pixbuf (singleton->priv->source_machine_pixbuf));
  } else if (machine_type == BT_TYPE_PROCESSOR_MACHINE) {
    return (gtk_image_new_from_pixbuf (singleton->priv->
            processor_machine_pixbuf));
  } else if (machine_type == BT_TYPE_SINK_MACHINE) {
    return (gtk_image_new_from_pixbuf (singleton->priv->sink_machine_pixbuf));
  }
  return (NULL);
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
 * bt_ui_resources_get_gdk_color:
 * @color_type: the color id
 *
 * Gets a prealocated color by id.
 *
 * Returns: the requested #GdkColor.
 */
GdkColor *
bt_ui_resources_get_gdk_color (BtUIResourcesColors color_type)
{
  return (&singleton->priv->colors[color_type]);
}

/**
 * bt_ui_resources_get_rgb_color:
 * @color_type: the color id
 * @color: target for color values
 *
 * Gets a prealocated color by id.
 */
void
bt_ui_resources_get_rgb_color (BtUIResourcesColors color_type, GdkRGBA * color)
{
  GdkColor *c = &singleton->priv->colors[color_type];

  color->red = c->red / 65535.0;
  color->green = c->green / 65535.0;
  color->blue = c->blue / 65535.0;
  color->alpha = 1.0;
}

/**
 * bt_ui_resources_get_color_by_machine:
 * @machine: the machine to get the color for
 * @color_type: a color shade
 *
 * Gets a colors shade depending on machine type in rgba format.
 *
 * Returns: a color depending on machine class and color_type
 */
guint32
bt_ui_resources_get_color_by_machine (const BtMachine * machine,
    BtUIResourcesMachineColors color_type)
{
  gulong ix = 0;
  guint32 color = 0;
  GdkColor *c;

  if (BT_IS_SOURCE_MACHINE (machine)) {
    ix = BT_UI_RES_COLOR_SOURCE_MACHINE_BASE + color_type;
  } else if (BT_IS_PROCESSOR_MACHINE (machine)) {
    ix = BT_UI_RES_COLOR_PROCESSOR_MACHINE_BASE + color_type;
  } else if (BT_IS_SINK_MACHINE (machine)) {
    ix = BT_UI_RES_COLOR_SINK_MACHINE_BASE + color_type;
  }
  c = &singleton->priv->colors[ix];
  color = (((guint32) (c->red & 0xFF00)) << 16) |
      (((guint32) (c->green & 0xFF00)) << 8) |
      ((guint32) (c->blue & 0xFF00)) | 0x000000FF;
  //GST_INFO("color[%2d/%1d] : 0x%08lx : %04x,%04x,%04x",ix,color_type,color,
  //  ui_resources->priv->colors[ix].red,ui_resources->priv->colors[ix].green,ui_resources->priv->colors[ix].blue);
  return (color);
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
  return (singleton->priv->accel_group);
}

void
bt_ui_resources_init_theme (void)
{
  GtkStyleProvider *provider;
  GError *err = NULL;

//#define USE_DARK_THEME
#ifdef USE_DARK_THEME
  g_object_set (gtk_settings_get_default (),
      "gtk-application-prefer-dark-theme", TRUE, NULL);
#endif

  provider = (GtkStyleProvider *) gtk_css_provider_new ();
  if (!gtk_css_provider_load_from_path (GTK_CSS_PROVIDER (provider), DATADIR
          "" G_DIR_SEPARATOR_S "" PACKAGE "" G_DIR_SEPARATOR_S
#ifndef USE_COMPACT_UI
#ifdef USE_DARK_THEME
          "bt-edit.dark.css",
#else
          "bt-edit.light.css",
#endif
#else
          "bt-edit.compact.css"
#endif
          & err)) {
    g_print ("Error loading css: %s\n", safe_string (err->message));
    g_error_free (err);
    err = NULL;
  } else {
    gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
        provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  }

  add_pixmap_directory (DATADIR "" G_DIR_SEPARATOR_S "" PACKAGE ""
      G_DIR_SEPARATOR_S "pixmaps" G_DIR_SEPARATOR_S);

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
    object =
        G_OBJECT_CLASS (bt_ui_resources_parent_class)->constructor (type,
        n_construct_params, construct_params);
    singleton = BT_UI_RESOURCES (object);
    g_object_add_weak_pointer (object, (gpointer *) (gpointer) & singleton);

    // initialise ressources
    bt_ui_resources_init_colors (singleton);
    bt_ui_resources_init_icons (singleton);
    singleton->priv->accel_group = gtk_accel_group_new ();
  } else {
    object = g_object_ref (singleton);
  }
  return object;
}

static void
bt_ui_resources_init (BtUIResources * self)
{
  self->priv =
      G_TYPE_INSTANCE_GET_PRIVATE (self, BT_TYPE_UI_RESOURCES,
      BtUIResourcesPrivate);
}

static void
bt_ui_resources_class_init (BtUIResourcesClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (BtUIResourcesPrivate));

  gobject_class->constructor = bt_ui_resources_constructor;
  gobject_class->dispose = bt_ui_resources_dispose;
}
