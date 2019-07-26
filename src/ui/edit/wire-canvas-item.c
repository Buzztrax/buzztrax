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
 * SECTION:btwirecanvasitem
 * @short_description: class for the editor wire views wire canvas item
 * @see_also: #BtSignalAnalysisDialog
 *
 * Provides volume control on the wires, as well as a menu to disconnect wires
 * and to launch the analyzer screen.
 */
/* TODO(ensonic): mixer strip
 *   - right now a click on the triangle pops up the volume or panorama slider
 *   - it could popup a whole mixer strip (with level meters)
 *
 * TODO(ensonic): add "insert effect" to context menu
 *   - could use the machine menu and work simillar to connect menu on canvas
 *   - would turn existing wire into 2nd wire to preserve wirepatterns
 */
#define BT_EDIT
#define BT_WIRE_CANVAS_ITEM_C

#include <math.h>

#include "bt-edit.h"

//-- property ids

enum
{
  WIRE_CANVAS_ITEM_MACHINES_PAGE = 1,
  WIRE_CANVAS_ITEM_WIRE,
  WIRE_CANVAS_ITEM_SRC,
  WIRE_CANVAS_ITEM_DST,
  WIRE_CANVAS_ITEM_ZOOM,
  WIRE_CANVAS_ITEM_ANALYSIS_DIALOG
};

struct _BtWireCanvasItemPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* the machine page we are on */
  BtMainPageMachines *main_page_machines;

  /* the underlying wire */
  BtWire *wire;
  /* and its properties */
  GHashTable *properties;
  /* gain and pan elements for monitoring */
  GstElement *wire_gain, *wire_pan;

  /* source and dst machine canvas item */
  BtMachineCanvasItem *src, *dst;

  /* the graphical components */
  ClutterContent *canvas;
  ClutterActor *pad;
  ClutterContent *pad_image;
  ClutterActor *vol_level, *pan_pos;
  GdkRGBA wire_color;

  /* wire context_menu */
  GtkMenu *context_menu;

  /* the analysis dialog */
  GtkWidget *analysis_dialog;

  /* the zoom-ratio in pixels/per unit */
  gdouble zoom;

  /* interaction state */
  gboolean dragging, moved;
  gdouble offx, offy, dragx, dragy;
};

static GQuark wire_canvas_item_quark = 0;

#define WIRE_UPSAMPLING 2.0

//-- the class

G_DEFINE_TYPE (BtWireCanvasItem, bt_wire_canvas_item, CLUTTER_TYPE_ACTOR);

//-- prototypes

static void on_signal_analysis_dialog_destroy (GtkWidget * widget,
    gpointer user_data);

//-- helper

static void
update_wire_graphics (BtWireCanvasItem * self)
{
  GdkPixbuf *pixbuf =
      bt_ui_resources_get_wire_graphics_pixbuf_by_wire (self->priv->wire,
      self->priv->zoom);

  clutter_image_set_data (CLUTTER_IMAGE (self->priv->pad_image),
      gdk_pixbuf_get_pixels (pixbuf), gdk_pixbuf_get_has_alpha (pixbuf)
      ? COGL_PIXEL_FORMAT_RGBA_8888
      : COGL_PIXEL_FORMAT_RGB_888,
      gdk_pixbuf_get_width (pixbuf),
      gdk_pixbuf_get_height (pixbuf), gdk_pixbuf_get_rowstride (pixbuf), NULL);

  GST_INFO ("pixbuf: %dx%d", gdk_pixbuf_get_width (pixbuf),
      gdk_pixbuf_get_height (pixbuf));

  g_object_unref (pixbuf);
}

static void
show_wire_analyzer_dialog (BtWireCanvasItem * self)
{
  if (!self->priv->analysis_dialog) {
    self->priv->analysis_dialog =
        GTK_WIDGET (bt_signal_analysis_dialog_new (GST_BIN (self->priv->wire)));
    bt_edit_application_attach_child_window (self->priv->app,
        GTK_WINDOW (self->priv->analysis_dialog));
    gtk_widget_show_all (self->priv->analysis_dialog);
    GST_INFO ("analyzer dialog opened");
    // remember open/closed state
    g_hash_table_insert (self->priv->properties, g_strdup ("analyzer-shown"),
        g_strdup ("1"));
    g_signal_connect (self->priv->analysis_dialog, "destroy",
        G_CALLBACK (on_signal_analysis_dialog_destroy), (gpointer) self);
    g_object_notify ((GObject *) self, "analysis-dialog");
  }
  gtk_window_present (GTK_WINDOW (self->priv->analysis_dialog));
}

//-- event handler

static void
on_wire_size_changed (const BtWireCanvasItem * self, GParamSpec * arg,
    gpointer user_data)
{
  gfloat w, h;

  clutter_actor_get_size ((ClutterActor *) self, &w, &h);
  if (w > 0.0 && h > 0.0) {
    clutter_canvas_set_size (CLUTTER_CANVAS (self->priv->canvas),
        (gint) (w * WIRE_UPSAMPLING), (gint) (h * WIRE_UPSAMPLING));
  }
}

/* draw the connecting wire */
static gboolean
on_wire_draw (ClutterCanvas * canvas, cairo_t * cr, gint width,
    gint height, gpointer user_data)
{
  BtWireCanvasItem *self = BT_WIRE_CANVAS_ITEM (user_data);
  //const gfloat xm = (gfloat) width / 2.0;
  const gfloat ym = (gfloat) height / 2.0;

  /* clear the contents of the canvas, to not paint over the previous frame */
  cairo_save (cr);
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);
  cairo_restore (cr);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

  cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_width (cr, 2.0);
  gdk_cairo_set_source_rgba (cr, &self->priv->wire_color);

  cairo_move_to (cr, 0.0, ym);
  cairo_line_to (cr, width, ym);
  cairo_stroke (cr);

  return TRUE;
}

static void
on_signal_analysis_dialog_destroy (GtkWidget * widget, gpointer user_data)
{
  BtWireCanvasItem *self = BT_WIRE_CANVAS_ITEM (user_data);

  GST_INFO ("signal analysis dialog destroy occurred");
  self->priv->analysis_dialog = NULL;
  g_object_notify ((GObject *) self, "analysis-dialog");
  // remember open/closed state
  g_hash_table_remove (self->priv->properties, "analyzer-shown");
}

static void
on_machine_removed (BtSetup * setup, BtMachine * machine, gpointer user_data)
{
  BtWireCanvasItem *self = BT_WIRE_CANVAS_ITEM (user_data);
  BtMachine *src, *dst;

  g_return_if_fail (BT_IS_MACHINE (machine));

  GST_INFO_OBJECT (self->priv->wire,
      "machine %" G_OBJECT_REF_COUNT_FMT " has been removed",
      G_OBJECT_LOG_REF_COUNT (machine));

  g_object_get (self->priv->src, "machine", &src, NULL);
  g_object_get (self->priv->dst, "machine", &dst, NULL);

  GST_INFO_OBJECT (self->priv->wire, "checking wire %p->%p", src, dst);
  if ((src == machine) || (dst == machine) || (src == NULL) || (dst == NULL)) {
    GST_INFO ("the machine, this wire %" G_OBJECT_REF_COUNT_FMT
        " is connected to, has been removed",
        G_OBJECT_LOG_REF_COUNT (self->priv->wire));
    // this will trigger the removal of the wire-canvas-item, see
    // main-page-machines:on_wire_removed()
    bt_setup_remove_wire (setup, self->priv->wire);
    GST_INFO_OBJECT (machine, "... machine %" G_OBJECT_REF_COUNT_FMT ", src %"
        G_OBJECT_REF_COUNT_FMT ", dst %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (machine), G_OBJECT_LOG_REF_COUNT (src),
        G_OBJECT_LOG_REF_COUNT (dst));
  }
  g_object_try_unref (src);
  g_object_try_unref (dst);

  // the wire is gone now
  GST_INFO ("checked wire for machine %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine));
}

static void
update_geometry (BtMachineCanvasItem * src, BtMachineCanvasItem * dst,
    gfloat * x, gfloat * y, gfloat * w, gfloat * h, gfloat * a)
{
  gfloat xs, ys, xe, ye, xd, yd;

  g_object_get (src, "x", &xs, "y", &ys, NULL);
  g_object_get (dst, "x", &xe, "y", &ye, NULL);
  xd = xe - xs;
  yd = ye - ys;

  *w = sqrt (xd * xd + yd * yd);
  *h = WIRE_PAD_H;
  *x = xs + (xd / 2.0);
  *y = ys + (yd / 2.0);
  *a = atan2 (yd, xd) * 180.0 / M_PI;
  GST_DEBUG ("src: %f, %f, dst: %f, %f, size: %f, %f, angle: %f",
      xs, ys, xe, ye, *w, *h, *a);
}

static void
on_wire_src_position_changed (BtMachineCanvasItem * src,
    ClutterEventType ev_type, gpointer user_data)
{
  BtWireCanvasItem *self = BT_WIRE_CANVAS_ITEM (user_data);
  gfloat x, y, w, h, a;

  update_geometry (src, self->priv->dst, &x, &y, &w, &h, &a);
  g_object_set (self, "x", x, "y", y, "width", w, "height", h,
      "rotation-angle-z", a, NULL);
  clutter_actor_set_position (self->priv->pad, w / 2.0, h / 2.0);
}

static void
on_wire_dst_position_changed (BtMachineCanvasItem * dst,
    ClutterEventType ev_type, gpointer user_data)
{
  BtWireCanvasItem *self = BT_WIRE_CANVAS_ITEM (user_data);
  gfloat x, y, w, h, a;

  update_geometry (self->priv->src, dst, &x, &y, &w, &h, &a);
  g_object_set (self, "x", x, "y", y, "width", w, "height", h,
      "rotation-angle-z", a, NULL);
  clutter_actor_set_position (self->priv->pad, w / 2.0, h / 2.0);
}

static void
on_context_menu_disconnect_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtWireCanvasItem *self = BT_WIRE_CANVAS_ITEM (user_data);

  bt_main_page_machines_delete_wire (self->priv->main_page_machines,
      self->priv->wire);
}

static void
on_context_menu_analysis_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtWireCanvasItem *self = BT_WIRE_CANVAS_ITEM (user_data);

  show_wire_analyzer_dialog (self);
}

static void
on_gain_changed (GstElement * element, GParamSpec * arg, gpointer user_data)
{
  BtWireCanvasItem *self = BT_WIRE_CANVAS_ITEM (user_data);
  gdouble gain;

  g_object_get (self->priv->wire_gain, "volume", &gain, NULL);
  // do some sensible clamping
  if (gain > 4.0) {
    g_object_set (self->priv->vol_level, "width", WIRE_PAD_METER_WIDTH, NULL);
  } else {
    g_object_set (self->priv->vol_level, "width",
        (gain / 4.0) * WIRE_PAD_METER_WIDTH, NULL);
  }
}

static void
on_pan_changed (GstElement * element, GParamSpec * arg, gpointer user_data)
{
  BtWireCanvasItem *self = BT_WIRE_CANVAS_ITEM (user_data);
  const gfloat w2 = (WIRE_PAD_METER_WIDTH / 2.0);
  gfloat pan;

  g_object_get (self->priv->wire_pan, "panorama", &pan, NULL);
  if (pan < 0.0) {
    g_object_set (self->priv->pan_pos,
        "x", WIRE_PAD_METER_BASE + ((1.0 + pan) * w2),
        "width", (-pan * w2), NULL);
  } else {
    g_object_set (self->priv->pan_pos,
        "x", WIRE_PAD_METER_BASE + w2, "width", (pan * w2), NULL);
  }
}

static void
on_wire_pan_changed (GstElement * element, GParamSpec * arg, gpointer user_data)
{
  BtWireCanvasItem *self = BT_WIRE_CANVAS_ITEM (user_data);

  g_object_get (self->priv->wire, "pan", &self->priv->wire_pan, NULL);
  g_signal_connect (self->priv->wire_pan, "notify::panorama",
      G_CALLBACK (on_pan_changed), (gpointer) self);
  // TODO(ensonic): need to change colors of the pan-icon
}

typedef struct
{
  BtWireCanvasItem *self;
  guint32 activate_time;
  gfloat mouse_x;
  gfloat mouse_y;
} BtEventIdleData;

static BtEventIdleData*
new_event_idle_data(BtWireCanvasItem * self, ClutterEvent * event) {
  BtEventIdleData *data = g_slice_new(BtEventIdleData);
  data->self = self;
  data->activate_time = clutter_event_get_time (event);
  clutter_event_get_coords(event, &data->mouse_x, &data->mouse_y);
  return data;
}

static void
free_event_idle_data(BtEventIdleData * data) {
  g_slice_free(BtEventIdleData,data);
}

static gboolean
volume_popup_helper (gpointer user_data)
{
  BtEventIdleData *data = (BtEventIdleData *) user_data;
  BtWireCanvasItem *self = data->self;
  guint x = data->mouse_x, y = data->mouse_y;
  free_event_idle_data (data);

  bt_main_page_machines_wire_volume_popup (self->priv->main_page_machines,
      self->priv->wire, x, y);
  return FALSE;
}

static gboolean
wire_popup_helper (gpointer user_data)
{
  BtEventIdleData *data = (BtEventIdleData *) user_data;
  BtWireCanvasItem *self = data->self;
  guint x = data->mouse_x, y = data->mouse_y;
  free_event_idle_data (data);

  bt_main_page_machines_wire_panorama_popup (self->priv->main_page_machines,
      self->priv->wire, x, y);
  return FALSE;
}

static gboolean
popup_helper (gpointer user_data)
{
  BtEventIdleData *data = (BtEventIdleData *) user_data;
  BtWireCanvasItem *self = data->self;

  GdkRectangle rect;
  rect.x = (gint)data->mouse_x;
  rect.y = (gint)data->mouse_y;
  rect.width = 0;
  rect.height = 0;

  free_event_idle_data (data);
								   
  GdkWindow* window = gtk_widget_get_window(bt_main_page_machines_get_canvas_widget(self->priv->main_page_machines));
  gtk_menu_popup_at_rect(self->priv->context_menu, window,
						 &rect, GDK_GRAVITY_SOUTH_EAST, GDK_GRAVITY_NORTH_WEST, NULL);
  return FALSE;
}

static gboolean
on_wire_pad_button_press (ClutterActor * citem, ClutterEvent * event,
    gpointer user_data)
{
  BtWireCanvasItem *self = BT_WIRE_CANVAS_ITEM (user_data);
  ClutterButtonEvent *button_event = (ClutterButtonEvent *) event;
  gboolean res = FALSE;

  GST_DEBUG ("CLUTTER_BUTTON_PRESS: %d", button_event->button);
  switch (button_event->button) {
    case 1:{
      BtEventIdleData *data = new_event_idle_data (self, event);
      GST_WARNING ("showing popup at %f,%f", button_event->x, button_event->y);
      if (!(button_event->modifier_state & CLUTTER_SHIFT_MASK)) {
        g_idle_add (volume_popup_helper, data);
      } else {
        g_idle_add (wire_popup_helper, data);
      }
      res = TRUE;
      break;
    }
    case 3:{
      g_idle_add (popup_helper, new_event_idle_data (self, event));
      res = TRUE;
      break;
    }
    default:
      break;
  }
  return res;
}

static gboolean
on_wire_pad_key_release (ClutterActor * citem, ClutterEvent * event,
    gpointer user_data)
{
  BtWireCanvasItem *self = BT_WIRE_CANVAS_ITEM (user_data);
  ClutterKeyEvent *key_event = (ClutterKeyEvent *) event;
  gboolean res = FALSE;

  GST_DEBUG ("CLUTTER_KEY_RELEASE: %d", key_event->keyval);
  switch (key_event->keyval) {
    case GDK_KEY_Menu:
      // show context menu
      gtk_menu_popup_at_pointer (self->priv->context_menu, NULL);
      res = TRUE;
      break;
    default:
      break;
  }
  return res;
}

static void
on_canvas_style_updated (GtkWidget * widget, gconstpointer user_data)
{
  BtWireCanvasItem *self = BT_WIRE_CANVAS_ITEM (user_data);
  GtkStyleContext *style = gtk_widget_get_style_context (widget);

  gtk_style_context_lookup_color (style, "wire_line", &self->priv->wire_color);
  if (self->priv->canvas) {
    // a directy invalidate crashes
    g_idle_add_full (G_PRIORITY_HIGH, (GSourceFunc) clutter_content_invalidate,
        self->priv->canvas, NULL);
  }
}

//-- helper methods

//-- constructor methods

/**
 * bt_wire_canvas_item_new:
 * @main_page_machines: the machine page the new item belongs to
 * @wire: the wire for which a canvas item should be created
 * @src_machine_item: the machine item at start
 * @dst_machine_item: the machine item at end
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in c#if 0
ase of an error
 */
BtWireCanvasItem *
bt_wire_canvas_item_new (const BtMainPageMachines * main_page_machines,
    BtWire * wire, BtMachineCanvasItem * src_machine_item,
    BtMachineCanvasItem * dst_machine_item)
{
  BtWireCanvasItem *self;
  ClutterActor *canvas;
  BtSetup *setup;
  gfloat x, y, w, h, a;

  g_object_get ((gpointer) main_page_machines, "canvas", &canvas, NULL);

  update_geometry (src_machine_item, dst_machine_item, &x, &y, &w, &h, &a);

  self = BT_WIRE_CANVAS_ITEM (g_object_new (BT_TYPE_WIRE_CANVAS_ITEM,
          "machines-page", main_page_machines, "wire", wire,
          "x", x, "y", y, "width", w, "height", h,
          "anchor-gravity", CLUTTER_GRAVITY_CENTER,
          "rotation-angle-z", a,
          "src", src_machine_item, "dst", dst_machine_item,
          "reactive", FALSE, NULL));

  clutter_actor_set_position (self->priv->pad, w / 2.0, h / 2.0);

  clutter_actor_add_child (canvas, (ClutterActor *) self);
  clutter_actor_set_child_below_sibling ((ClutterActor *) canvas,
      (ClutterActor *) self, NULL);

  bt_child_proxy_get (self->priv->app, "song::setup", &setup, NULL);
  g_signal_connect_object (setup, "machine-removed",
      G_CALLBACK (on_machine_removed), (gpointer) self, 0);

  //GST_INFO("wire canvas item added");

  g_object_unref (setup);
  g_object_unref (canvas);
  return self;
}

//-- methods

/**
 * bt_wire_show_analyzer_dialog:
 * @wire: wire to show the dialog for
 *
 * Shows the wire analyzer dialog.
 *
 * Since: 0.6
 */
void
bt_wire_show_analyzer_dialog (BtWire * wire)
{
  BtWireCanvasItem *self =
      BT_WIRE_CANVAS_ITEM (g_object_get_qdata ((GObject *) wire,
          wire_canvas_item_quark));

  show_wire_analyzer_dialog (self);
}

//-- wrapper

//-- class internals

static void
bt_wire_canvas_item_constructed (GObject * object)
{
  BtWireCanvasItem *self = BT_WIRE_CANVAS_ITEM (object);
  gchar *prop;
  ClutterColor *meter_bg;

  if (G_OBJECT_CLASS (bt_wire_canvas_item_parent_class)->constructed)
    G_OBJECT_CLASS (bt_wire_canvas_item_parent_class)->constructed (object);

  // volume and panorama handling
  g_object_get (self->priv->wire, "gain", &self->priv->wire_gain, "pan",
      &self->priv->wire_pan, NULL);
  g_signal_connect_object (self->priv->wire_gain, "notify::volume",
      G_CALLBACK (on_gain_changed), (gpointer) self, 0);
  if (self->priv->wire_pan) {
    g_signal_connect_object (self->priv->wire_pan, "notify::panorama",
        G_CALLBACK (on_pan_changed), (gpointer) self, 0);
  } else {
    g_signal_connect_object (self->priv->wire, "notify::pan",
        G_CALLBACK (on_wire_pan_changed), (gpointer) self, 0);
  }


  GST_INFO ("add sub actors");

  // the wire pad
  self->priv->pad = clutter_actor_new ();
  self->priv->pad_image = clutter_image_new ();
  update_wire_graphics (self);
  clutter_actor_set_content_scaling_filters (self->priv->pad,
      CLUTTER_SCALING_FILTER_TRILINEAR, CLUTTER_SCALING_FILTER_LINEAR);
  clutter_actor_set_size (self->priv->pad, WIRE_PAD_W, WIRE_PAD_H);
  clutter_actor_set_pivot_point (self->priv->pad, 0.5, 0.5);
  clutter_actor_set_translation ((ClutterActor *) self->priv->pad,
      (WIRE_PAD_W / -2.0), (WIRE_PAD_H / -2.0), 0.0);
  clutter_actor_set_reactive (self->priv->pad, TRUE);
  clutter_actor_add_child ((ClutterActor *) self, self->priv->pad);
  clutter_actor_set_content (self->priv->pad, self->priv->pad_image);
  clutter_actor_set_child_above_sibling ((ClutterActor *) self, self->priv->pad,
      NULL);
  g_signal_connect (self->priv->pad, "button-press-event",
      G_CALLBACK (on_wire_pad_button_press), (gpointer) self);
  g_signal_connect (self->priv->pad, "key-release-event",
      G_CALLBACK (on_wire_pad_key_release), (gpointer) self);


  // the meters
  meter_bg = clutter_color_new (0x5f, 0x5f, 0x5f, 0xff);
  self->priv->vol_level = g_object_new (CLUTTER_TYPE_ACTOR,
      "background-color", meter_bg,
      "x", WIRE_PAD_METER_BASE, "y", WIRE_PAD_METER_VOL,
      "width", 0.0, "height", WIRE_PAD_METER_HEIGHT, NULL);
  clutter_actor_add_child (self->priv->pad, self->priv->vol_level);
  on_gain_changed (self->priv->wire_gain, NULL, (gpointer) self);

  if (self->priv->wire_pan) {
    self->priv->pan_pos = g_object_new (CLUTTER_TYPE_ACTOR,
        "background-color", meter_bg,
        "x", WIRE_PAD_METER_BASE, "y", WIRE_PAD_METER_PAN,
        "width", 0.0, "height", WIRE_PAD_METER_HEIGHT, NULL);
    clutter_actor_add_child (self->priv->pad, self->priv->pan_pos);
    on_pan_changed (self->priv->wire_pan, NULL, (gpointer) self);
  }
  clutter_color_free (meter_bg);
  // the wire line
  self->priv->canvas = clutter_canvas_new ();
  clutter_actor_set_content ((ClutterActor *) self, self->priv->canvas);
  clutter_actor_set_content_scaling_filters ((ClutterActor *) self,
      CLUTTER_SCALING_FILTER_TRILINEAR, CLUTTER_SCALING_FILTER_LINEAR);
  g_signal_connect_object (self->priv->canvas, "draw",
      G_CALLBACK (on_wire_draw), self, 0);
  g_signal_connect (self, "notify::width", G_CALLBACK (on_wire_size_changed),
      self);
  g_signal_connect (self, "notify::height", G_CALLBACK (on_wire_size_changed),
      self);
  on_wire_size_changed (self, NULL, self);
  clutter_content_invalidate (self->priv->canvas);

  GST_INFO ("done and all shown");

  prop =
      (gchar *) g_hash_table_lookup (self->priv->properties, "analyzer-shown");
  if (prop && prop[0] == '1' && prop[1] == '\0') {
    if ((self->priv->analysis_dialog =
            GTK_WIDGET (bt_signal_analysis_dialog_new (GST_BIN (self->
                        priv->wire))))) {
      bt_edit_application_attach_child_window (self->priv->app,
          GTK_WINDOW (self->priv->analysis_dialog));
      gtk_widget_show_all (self->priv->analysis_dialog);
      g_signal_connect (self->priv->analysis_dialog, "destroy",
          G_CALLBACK (on_signal_analysis_dialog_destroy), (gpointer) self);
    }
  }
}

static void
bt_wire_canvas_item_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  BtWireCanvasItem *self = BT_WIRE_CANVAS_ITEM (object);
  return_if_disposed ();
  switch (property_id) {
    case WIRE_CANVAS_ITEM_MACHINES_PAGE:
      g_value_set_object (value, self->priv->main_page_machines);
      break;
    case WIRE_CANVAS_ITEM_WIRE:
      g_value_set_object (value, self->priv->wire);
      break;
    case WIRE_CANVAS_ITEM_SRC:
      g_value_set_object (value, self->priv->src);
      break;
    case WIRE_CANVAS_ITEM_DST:
      g_value_set_object (value, self->priv->dst);
      break;
    case WIRE_CANVAS_ITEM_ZOOM:
      g_value_set_double (value, self->priv->zoom);
      break;
    case WIRE_CANVAS_ITEM_ANALYSIS_DIALOG:
      g_value_set_object (value, self->priv->analysis_dialog);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_wire_canvas_item_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  BtWireCanvasItem *self = BT_WIRE_CANVAS_ITEM (object);
  return_if_disposed ();
  switch (property_id) {
    case WIRE_CANVAS_ITEM_MACHINES_PAGE:
      g_object_try_weak_unref (self->priv->main_page_machines);
      self->priv->main_page_machines =
          BT_MAIN_PAGE_MACHINES (g_value_get_object (value));
      g_object_try_weak_ref (self->priv->main_page_machines);
      on_canvas_style_updated ((GtkWidget *) self->priv->main_page_machines,
          (gpointer) self);
      g_signal_connect_object (self->priv->main_page_machines, "style-updated",
          G_CALLBACK (on_canvas_style_updated), (gpointer) self,
          G_CONNECT_AFTER);
      //GST_DEBUG("set the main_page_machines for wire_canvas_item: %p",self->priv->main_page_machines);
      break;
    case WIRE_CANVAS_ITEM_WIRE:
      g_object_try_unref (self->priv->wire);
      self->priv->wire = BT_WIRE (g_value_dup_object (value));
      if (self->priv->wire) {
        //GST_DEBUG("set the wire for wire_canvas_item: %p",self->priv->wire);
        g_object_set_qdata ((GObject *) self->priv->wire,
            wire_canvas_item_quark, (gpointer) self);
        g_object_get (self->priv->wire, "properties", &(self->priv->properties),
            NULL);
      }
      break;
    case WIRE_CANVAS_ITEM_SRC:
      self->priv->src = BT_MACHINE_CANVAS_ITEM (g_value_dup_object (value));
      if (self->priv->src) {
        g_signal_connect_object (self->priv->src, "position-changed",
            G_CALLBACK (on_wire_src_position_changed), (gpointer) self, 0);
        GST_DEBUG ("set the src for wire_canvas_item: %p", self->priv->src);
      }
      break;
    case WIRE_CANVAS_ITEM_DST:
      self->priv->dst = BT_MACHINE_CANVAS_ITEM (g_value_dup_object (value));
      if (self->priv->dst) {
        g_signal_connect_object (self->priv->dst, "position-changed",
            G_CALLBACK (on_wire_dst_position_changed), (gpointer) self, 0);
        GST_DEBUG ("set the dst for wire_canvas_item: %p", self->priv->dst);
      }
      break;
    case WIRE_CANVAS_ITEM_ZOOM:
      self->priv->zoom = g_value_get_double (value);
      GST_DEBUG ("set the zoom for wire_canvas_item: %f", self->priv->zoom);
      /* reload the svg icons, we do this to keep them sharp */
      if (self->priv->pad_image) {
        update_wire_graphics (self);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_wire_canvas_item_dispose (GObject * object)
{
  BtWireCanvasItem *self = BT_WIRE_CANVAS_ITEM (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  g_object_try_unref (self->priv->wire_gain);
  g_object_try_unref (self->priv->wire_pan);
  g_object_unref (self->priv->pad_image);
  g_object_try_unref (self->priv->wire);
  g_object_try_unref (self->priv->src);
  g_object_try_unref (self->priv->dst);
  g_object_try_weak_unref (self->priv->main_page_machines);
  g_object_unref (self->priv->app);

  if (self->priv->analysis_dialog) {
    gtk_widget_destroy (self->priv->analysis_dialog);
  }

  gtk_widget_destroy (GTK_WIDGET (self->priv->context_menu));
  g_object_unref (self->priv->context_menu);

  GST_DEBUG ("  chaining up");
  G_OBJECT_CLASS (bt_wire_canvas_item_parent_class)->dispose (object);
  GST_DEBUG ("  done");
}

static void
bt_wire_canvas_item_init (BtWireCanvasItem * self)
{
  GtkWidget *menu_item;

  self->priv =
      G_TYPE_INSTANCE_GET_PRIVATE (self, BT_TYPE_WIRE_CANVAS_ITEM,
      BtWireCanvasItemPrivate);
  GST_DEBUG ("!!!! self=%p", self);
  self->priv->app = bt_edit_application_new ();

  // generate the context menu
  self->priv->context_menu = GTK_MENU (g_object_ref_sink (gtk_menu_new ()));

  menu_item = gtk_menu_item_new_with_label (_("Disconnect"));
  gtk_menu_shell_append (GTK_MENU_SHELL (self->priv->context_menu), menu_item);
  gtk_widget_show (menu_item);
  g_signal_connect (menu_item, "activate",
      G_CALLBACK (on_context_menu_disconnect_activate), (gpointer) self);

  menu_item = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (GTK_MENU_SHELL (self->priv->context_menu), menu_item);
  gtk_widget_show (menu_item);

  menu_item = gtk_menu_item_new_with_label (_("Signal Analysis…"));
  gtk_menu_shell_append (GTK_MENU_SHELL (self->priv->context_menu), menu_item);
  gtk_widget_show (menu_item);
  g_signal_connect (menu_item, "activate",
      G_CALLBACK (on_context_menu_analysis_activate), (gpointer) self);

  self->priv->zoom = 1.0;
}

static void
bt_wire_canvas_item_class_init (BtWireCanvasItemClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  wire_canvas_item_quark = g_quark_from_static_string ("wire-canvas-item");

  g_type_class_add_private (klass, sizeof (BtWireCanvasItemPrivate));

  gobject_class->constructed = bt_wire_canvas_item_constructed;
  gobject_class->set_property = bt_wire_canvas_item_set_property;
  gobject_class->get_property = bt_wire_canvas_item_get_property;
  gobject_class->dispose = bt_wire_canvas_item_dispose;

  g_object_class_install_property (gobject_class,
      WIRE_CANVAS_ITEM_MACHINES_PAGE, g_param_spec_object ("machines-page",
          "machines-page contruct prop",
          "Set application object, the window belongs to",
          BT_TYPE_MAIN_PAGE_MACHINES,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, WIRE_CANVAS_ITEM_WIRE,
      g_param_spec_object ("wire", "wire contruct prop",
          "Set wire object, the item belongs to", BT_TYPE_WIRE,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, WIRE_CANVAS_ITEM_SRC,
      g_param_spec_object ("src", "src contruct prop",
          "Set wire src machine canvas item", BT_TYPE_MACHINE_CANVAS_ITEM,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, WIRE_CANVAS_ITEM_DST,
      g_param_spec_object ("dst", "dst contruct prop",
          "Set wire dst machine canvas item", BT_TYPE_MACHINE_CANVAS_ITEM,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, WIRE_CANVAS_ITEM_ZOOM,
      g_param_spec_double ("zoom",
          "zoom prop",
          "Set zoom ratio for the wire item",
          0.0, 100.0, 1.0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      WIRE_CANVAS_ITEM_ANALYSIS_DIALOG, g_param_spec_object ("analysis-dialog",
          "analysis dialog prop", "Get the the analysis dialog if shown",
          BT_TYPE_SIGNAL_ANALYSIS_DIALOG,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}
