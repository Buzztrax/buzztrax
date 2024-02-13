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

struct _BtWireCanvasItem
{
  GtkWidget parent;
  
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
  GtkDrawingArea *canvas;
  GtkWidget *pad; // The "pad" is the arrow-shaped control in the middle of the wire.
  GtkImage *pad_image;
  GtkLevelBar *vol_level, *pan_pos;
  GdkRGBA wire_color;

  /* wire context_menu */
  GMenu *context_menu;

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

G_DEFINE_TYPE (BtWireCanvasItem, bt_wire_canvas_item, GTK_TYPE_WIDGET);

//-- prototypes

static void on_signal_analysis_dialog_destroy (GtkWidget * widget,
    gpointer user_data);

//-- helper

static void
update_wire_graphics (BtWireCanvasItem * self)
{
  GdkPaintable *paintable =
      bt_ui_resources_get_wire_graphics_paintable_by_wire (self->wire,
      self->zoom);

  gtk_image_set_from_paintable (self->pad_image, paintable);

  g_object_unref (paintable);
}

static void
show_wire_analyzer_dialog (BtWireCanvasItem * self)
{
  if (!self->analysis_dialog) {
    self->analysis_dialog =
        GTK_WIDGET (bt_signal_analysis_dialog_new (GST_BIN (self->wire)));
    bt_edit_application_attach_child_window (self->app,
        GTK_WINDOW (self->analysis_dialog));
    GST_INFO ("analyzer dialog opened");
    // remember open/closed state
    g_hash_table_insert (self->properties, g_strdup ("analyzer-shown"),
        g_strdup ("1"));
    g_signal_connect (self->analysis_dialog, "destroy",
        G_CALLBACK (on_signal_analysis_dialog_destroy), (gpointer) self);
    g_object_notify ((GObject *) self, "analysis-dialog");
  }
  gtk_window_present (GTK_WINDOW (self->analysis_dialog));
}

//-- event handler

static void
on_wire_size_changed (BtWireCanvasItem * self, GParamSpec * arg,
    gpointer user_data)
{
  /// GTK4 Revise to make use of 'WIRE_UPSAMPLING'
  gtk_widget_set_size_request (GTK_WIDGET (self->canvas),
      gtk_widget_get_width (GTK_WIDGET (self)),
      gtk_widget_get_height (GTK_WIDGET (self)));
}

/* draw the connecting wire */
static void
on_wire_draw (GtkDrawingArea *area, cairo_t *cr, int width, int height,
    gpointer user_data)
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
  gdk_cairo_set_source_rgba (cr, &self->wire_color);

  cairo_move_to (cr, 0.0, ym);
  cairo_line_to (cr, width, ym);
  cairo_stroke (cr);
}

static void
on_signal_analysis_dialog_destroy (GtkWidget * widget, gpointer user_data)
{
  BtWireCanvasItem *self = BT_WIRE_CANVAS_ITEM (user_data);

  GST_INFO ("signal analysis dialog destroy occurred");
  self->analysis_dialog = NULL;
  g_object_notify ((GObject *) self, "analysis-dialog");
  // remember open/closed state
  g_hash_table_remove (self->properties, "analyzer-shown");
}

static void
on_machine_removed (BtSetup * setup, BtMachine * machine, gpointer user_data)
{
  BtWireCanvasItem *self = BT_WIRE_CANVAS_ITEM (user_data);
  BtMachine *src, *dst;

  g_return_if_fail (BT_IS_MACHINE (machine));

  GST_INFO_OBJECT (self->wire,
      "machine %" G_OBJECT_REF_COUNT_FMT " has been removed",
      G_OBJECT_LOG_REF_COUNT (machine));

  g_object_get (self->src, "machine", &src, NULL);
  g_object_get (self->dst, "machine", &dst, NULL);

  GST_INFO_OBJECT (self->wire, "checking wire %p->%p", src, dst);
  if ((src == machine) || (dst == machine) || (src == NULL) || (dst == NULL)) {
    GST_INFO ("the machine, this wire %" G_OBJECT_REF_COUNT_FMT
        " is connected to, has been removed",
        G_OBJECT_LOG_REF_COUNT (self->wire));
    // this will trigger the removal of the wire-canvas-item, see
    // main-page-machines:on_wire_removed()
    bt_setup_remove_wire (setup, self->wire);
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
bt_wire_canvas_item_update_xform (BtWireCanvasItem *self, gdouble x,
    gdouble y, gdouble w, gdouble h, gdouble a)
{
  /// GTK4 TBD: remove assumption parent is GtkFixed
  GtkFixed *fixed = GTK_FIXED (gtk_widget_get_parent (GTK_WIDGET (self)));

  GskTransform *xform2;
  GskTransform *xform = gsk_transform_new ();
  
  xform2 = gsk_transform_rotate (xform, a);
  gsk_transform_unref (xform); xform = xform2;
  xform2 = gsk_transform_translate (xform, &GRAPHENE_POINT_INIT (x, y));
  gsk_transform_unref (xform); xform = xform2;
  
  gtk_fixed_set_child_transform (fixed, GTK_WIDGET (self), xform);

  gsk_transform_unref (xform);
  
  gtk_widget_set_size_request (GTK_WIDGET (self), w, h);
  
  gtk_fixed_move (fixed, GTK_WIDGET (self->pad), w / 2.0, h / 2.0);
}

static void
on_wire_src_position_changed (BtMachineCanvasItem * src,
    guint ev_type, gpointer user_data)
{
  BtWireCanvasItem *self = BT_WIRE_CANVAS_ITEM (user_data);
  gfloat x, y, w, h, a;

  update_geometry (src, self->dst, &x, &y, &w, &h, &a);
  bt_wire_canvas_item_update_xform (self, x, y, w, h, a);
}

static void
on_wire_dst_position_changed (BtMachineCanvasItem * dst,
    guint ev_type, gpointer user_data)
{
  BtWireCanvasItem *self = BT_WIRE_CANVAS_ITEM (user_data);
  gfloat x, y, w, h, a;

  update_geometry (self->src, dst, &x, &y, &w, &h, &a);
  bt_wire_canvas_item_update_xform (self, x, y, w, h, a);
}

static void
on_context_menu_disconnect_activate (GSimpleAction* action, GVariant* parameter,
    gpointer user_data)
{
  BtWireCanvasItem *self = BT_WIRE_CANVAS_ITEM (user_data);

  bt_main_page_machines_delete_wire (self->main_page_machines,
      self->wire);
}

static void
on_context_menu_analysis_activate (GSimpleAction* action, GVariant* parameter,
    gpointer user_data)
{
  BtWireCanvasItem *self = BT_WIRE_CANVAS_ITEM (user_data);

  show_wire_analyzer_dialog (self);
}

static void
on_gain_changed (GstElement * element, GParamSpec * arg, gpointer user_data)
{
  BtWireCanvasItem *self = BT_WIRE_CANVAS_ITEM (user_data);
  gdouble gain;

  g_object_get (self->wire_gain, "volume", &gain, NULL);
  // do some sensible clamping
  if (gain > 4.0) {
    g_object_set (self->vol_level, "width", WIRE_PAD_METER_WIDTH, NULL);
  } else {
    g_object_set (self->vol_level, "width",
        (gain / 4.0) * WIRE_PAD_METER_WIDTH, NULL);
  }
}

static void
on_pan_changed (GstElement * element, GParamSpec * arg, gpointer user_data)
{
  BtWireCanvasItem *self = BT_WIRE_CANVAS_ITEM (user_data);
  const gfloat w2 = (WIRE_PAD_METER_WIDTH / 2.0);
  gfloat pan;

  g_object_get (self->wire_pan, "panorama", &pan, NULL);
  if (pan < 0.0) {
    g_object_set (self->pan_pos,
        "x", WIRE_PAD_METER_BASE + ((1.0 + pan) * w2),
        "width", (-pan * w2), NULL);
  } else {
    g_object_set (self->pan_pos,
        "x", WIRE_PAD_METER_BASE + w2, "width", (pan * w2), NULL);
  }
}

static void
on_wire_pan_changed (GstElement * element, GParamSpec * arg, gpointer user_data)
{
  BtWireCanvasItem *self = BT_WIRE_CANVAS_ITEM (user_data);

  g_object_get (self->wire, "pan", &self->wire_pan, NULL);
  g_signal_connect (self->wire_pan, "notify::panorama",
      G_CALLBACK (on_pan_changed), (gpointer) self);
  // TODO(ensonic): need to change colors of the pan-icon
}

static void
on_wire_pad_button_press (GtkGestureClick* click, gint n_press, gdouble x,
    gdouble y, gpointer user_data)
{
  BtWireCanvasItem *self = BT_WIRE_CANVAS_ITEM (user_data);
  guint button = gtk_gesture_single_get_button (GTK_GESTURE_SINGLE (click));
  GdkModifierType state = gtk_event_controller_get_current_event_state (
      GTK_EVENT_CONTROLLER (click));

  GST_DEBUG ("BUTTON_PRESS: %d", button);
  switch (button) {
    case 1:{
      GST_WARNING ("showing popup at %f,%f", x, y);
      if (!(state & GDK_SHIFT_MASK)) {
        bt_main_page_machines_wire_volume_popup (self->main_page_machines,
            self->wire, x, y);
      } else {
        bt_main_page_machines_wire_panorama_popup (self->main_page_machines,
            self->wire, x, y);
      }
      break;
    }
    case 3:{
      GtkPopover* popover = GTK_POPOVER (
          gtk_popover_menu_new_from_model (G_MENU_MODEL (self->context_menu)));
      
      GdkRectangle rect = { x, y, 0, 0 };
      
      gtk_widget_set_parent (GTK_WIDGET (popover), GTK_WIDGET (self));
      gtk_popover_set_pointing_to (popover, &rect);
      gtk_popover_popup (popover);
      break;
    }
  }
}

static void
on_wire_pad_key_release (GtkEventControllerKey* key, guint keyval,
    guint keycode, GdkModifierType state, gpointer user_data)
{
  BtWireCanvasItem *self = BT_WIRE_CANVAS_ITEM (user_data);

  GST_DEBUG ("KEY_RELEASE: %d", keyval);
  switch (keyval) {
    case GDK_KEY_Menu: {
      // show context menu
      GtkPopover* popover = GTK_POPOVER (
          gtk_popover_menu_new_from_model (G_MENU_MODEL (self->context_menu)));
      
      gtk_widget_set_parent (GTK_WIDGET (popover), GTK_WIDGET (self));
      gtk_popover_popup (popover);
      break;
    }
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
  BtSetup *setup;
  gfloat x, y, w, h, a;

  self = BT_WIRE_CANVAS_ITEM (g_object_new (BT_TYPE_WIRE_CANVAS_ITEM,
          "machines-page", main_page_machines, "wire", wire,
          "src", src_machine_item, "dst", dst_machine_item,
          NULL));

  update_geometry (src_machine_item, dst_machine_item, &x, &y, &w, &h, &a);
  bt_wire_canvas_item_update_xform (self, x, y, w, h, a);

  bt_child_proxy_get (self->app, "song::setup", &setup, NULL);
  g_signal_connect_object (setup, "machine-removed",
      G_CALLBACK (on_machine_removed), (gpointer) self, 0);

  g_object_unref (setup);
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

  if (G_OBJECT_CLASS (bt_wire_canvas_item_parent_class)->constructed)
    G_OBJECT_CLASS (bt_wire_canvas_item_parent_class)->constructed (object);

  gtk_widget_set_layout_manager (GTK_WIDGET (self), gtk_fixed_layout_new ());
  
  // volume and panorama handling
  g_object_get (self->wire, "gain", &self->wire_gain, "pan",
      &self->wire_pan, NULL);
  g_signal_connect_object (self->wire_gain, "notify::volume",
      G_CALLBACK (on_gain_changed), (gpointer) self, 0);
  if (self->wire_pan) {
    g_signal_connect_object (self->wire_pan, "notify::panorama",
        G_CALLBACK (on_pan_changed), (gpointer) self, 0);
  } else {
    g_signal_connect_object (self->wire, "notify::pan",
        G_CALLBACK (on_wire_pan_changed), (gpointer) self, 0);
  }


  GST_INFO ("add sub actors");

  // the wire line
  self->canvas = GTK_DRAWING_AREA (gtk_drawing_area_new ());
  gtk_drawing_area_set_draw_func (self->canvas, on_wire_draw, self, g_object_unref);
  gtk_widget_set_parent (GTK_WIDGET (self->canvas), GTK_WIDGET (self));
  
  g_signal_connect (self, "notify::width-request", G_CALLBACK (on_wire_size_changed),
      self);
  g_signal_connect (self, "notify::height-request", G_CALLBACK (on_wire_size_changed),
      self);
  on_wire_size_changed (self, NULL, self);

  // the wire pad
  self->pad = gtk_frame_new (NULL);
  gtk_widget_set_layout_manager (self->pad, gtk_fixed_layout_new ());
  gtk_widget_set_size_request (self->pad, WIRE_PAD_W, WIRE_PAD_H);
  gtk_widget_set_parent (self->pad, GTK_WIDGET (self));
  
  self->pad_image = GTK_IMAGE (gtk_image_new ());
  gtk_widget_set_parent (GTK_WIDGET (self->pad_image), self->pad);
  update_wire_graphics (self);

  GtkGesture *click = gtk_gesture_click_new ();
  gtk_widget_add_controller (self->pad, GTK_EVENT_CONTROLLER (click));
  g_signal_connect (click, "pressed",
      G_CALLBACK (on_wire_pad_button_press), (gpointer) self);

  GtkEventController *key = gtk_event_controller_key_new ();
  gtk_widget_add_controller (self->pad, key);
  g_signal_connect (self->pad, "key-released",
      G_CALLBACK (on_wire_pad_key_release), (gpointer) self);


  // the meters
  /// GTK4: set color via CSS
  self->vol_level = GTK_LEVEL_BAR (gtk_level_bar_new ());
  gtk_widget_set_size_request (GTK_WIDGET (self->vol_level), WIRE_PAD_METER_WIDTH,
      WIRE_PAD_METER_HEIGHT);
  gtk_widget_set_parent (GTK_WIDGET (self->vol_level), self->pad);
  
  on_gain_changed (self->wire_gain, NULL, (gpointer) self);

  if (self->wire_pan) {
    self->pan_pos = GTK_LEVEL_BAR (gtk_level_bar_new ());
    gtk_widget_set_size_request (GTK_WIDGET (self->vol_level), WIRE_PAD_METER_WIDTH,
        WIRE_PAD_METER_HEIGHT);
    gtk_widget_set_parent (GTK_WIDGET (self->pan_pos), self->pad);
    on_pan_changed (self->wire_pan, NULL, (gpointer) self);
  }
  
  GST_INFO ("done and all shown");

  prop =
      (gchar *) g_hash_table_lookup (self->properties, "analyzer-shown");
  if (prop && prop[0] == '1' && prop[1] == '\0') {
    if ((self->analysis_dialog =
            GTK_WIDGET (bt_signal_analysis_dialog_new (GST_BIN (self->
                        wire))))) {
      bt_edit_application_attach_child_window (self->app,
          GTK_WINDOW (self->analysis_dialog));
      g_signal_connect (self->analysis_dialog, "destroy",
          G_CALLBACK (on_signal_analysis_dialog_destroy), (gpointer) self);
    }
  }
}

static void
bt_wire_canvas_item_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  BtWireCanvasItem *self = BT_WIRE_CANVAS_ITEM (object);
  return_if_disposed_self ();
  switch (property_id) {
    case WIRE_CANVAS_ITEM_MACHINES_PAGE:
      g_value_set_object (value, self->main_page_machines);
      break;
    case WIRE_CANVAS_ITEM_WIRE:
      g_value_set_object (value, self->wire);
      break;
    case WIRE_CANVAS_ITEM_SRC:
      g_value_set_object (value, self->src);
      break;
    case WIRE_CANVAS_ITEM_DST:
      g_value_set_object (value, self->dst);
      break;
    case WIRE_CANVAS_ITEM_ZOOM:
      g_value_set_double (value, self->zoom);
      break;
    case WIRE_CANVAS_ITEM_ANALYSIS_DIALOG:
      g_value_set_object (value, self->analysis_dialog);
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
  return_if_disposed_self ();
  switch (property_id) {
    case WIRE_CANVAS_ITEM_MACHINES_PAGE:
      g_object_try_weak_unref (self->main_page_machines);
      self->main_page_machines =
          BT_MAIN_PAGE_MACHINES (g_value_get_object (value));
      g_object_try_weak_ref (self->main_page_machines);
      //GST_DEBUG("set the main_page_machines for wire_canvas_item: %p",self->main_page_machines);
      break;
    case WIRE_CANVAS_ITEM_WIRE:
      g_object_try_unref (self->wire);
      self->wire = BT_WIRE (g_value_dup_object (value));
      if (self->wire) {
        //GST_DEBUG("set the wire for wire_canvas_item: %p",self->wire);
        g_object_set_qdata ((GObject *) self->wire,
            wire_canvas_item_quark, (gpointer) self);
        g_object_get (self->wire, "properties", &(self->properties),
            NULL);
      }
      break;
    case WIRE_CANVAS_ITEM_SRC:
      self->src = BT_MACHINE_CANVAS_ITEM (g_value_dup_object (value));
      if (self->src) {
        g_signal_connect_object (self->src, "position-changed",
            G_CALLBACK (on_wire_src_position_changed), (gpointer) self, 0);
        GST_DEBUG ("set the src for wire_canvas_item: %p", self->src);
      }
      break;
    case WIRE_CANVAS_ITEM_DST:
      self->dst = BT_MACHINE_CANVAS_ITEM (g_value_dup_object (value));
      if (self->dst) {
        g_signal_connect_object (self->dst, "position-changed",
            G_CALLBACK (on_wire_dst_position_changed), (gpointer) self, 0);
        GST_DEBUG ("set the dst for wire_canvas_item: %p", self->dst);
      }
      break;
    case WIRE_CANVAS_ITEM_ZOOM:
      self->zoom = g_value_get_double (value);
      GST_DEBUG ("set the zoom for wire_canvas_item: %f", self->zoom);
      /* reload the svg icons, we do this to keep them sharp */
      if (self->pad_image) {
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

  return_if_disposed_self ();
  self->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  g_clear_pointer ((GtkWidget**)&self->vol_level, gtk_widget_unparent);
  g_clear_pointer ((GtkWidget**)&self->pad, gtk_widget_unparent);
  g_clear_pointer ((GtkWidget**)&self->pad_image, gtk_widget_unparent);
  g_clear_pointer ((GtkWidget**)&self->pan_pos, gtk_widget_unparent);
  g_clear_pointer ((GtkWidget**)&self->canvas, gtk_widget_unparent);

  g_object_try_unref (self->wire_gain);
  g_object_try_unref (self->wire_pan);
  g_object_unref (self->pad_image);
  g_object_try_unref (self->wire);
  g_object_try_unref (self->src);
  g_object_try_unref (self->dst);
  g_object_try_weak_unref (self->main_page_machines);
  g_object_unref (self->app);

  g_clear_object (&self->analysis_dialog);
  g_clear_object (&self->context_menu);
  
  GST_DEBUG ("  chaining up");
  G_OBJECT_CLASS (bt_wire_canvas_item_parent_class)->dispose (object);
  GST_DEBUG ("  done");
}

static void
bt_wire_canvas_item_init (BtWireCanvasItem * self)
{
  self = bt_wire_canvas_item_get_instance_private(self);
  GST_DEBUG ("!!!! self=%p", self);
  self->app = bt_edit_application_new ();

  /// GTK4 complete these action definitions
  GSimpleActionGroup* ag = g_simple_action_group_new ();
  const GActionEntry entries[] = {
    { "wire.disconnect", on_context_menu_disconnect_activate, "s" },
    { "wire.signal-analysis", on_context_menu_analysis_activate, "s" }
  };
  g_action_map_add_action_entries (G_ACTION_MAP (ag), entries, G_N_ELEMENTS (entries), NULL);
  
  // generate the context menu
  self->context_menu = g_menu_new ();

  GMenu* section;
  section = g_menu_new ();
  g_menu_append_section (self->context_menu, NULL, G_MENU_MODEL (section));
  g_menu_append (section, _("Disconnect"), "wire.disconnect");

  section = g_menu_new ();
  g_menu_append_section (self->context_menu, NULL, G_MENU_MODEL (section));
  g_menu_append (section, _("Signal Analysis…"), "wire.signal-analysis");

  self->zoom = 1.0;
}

static void
bt_wire_canvas_item_class_init (BtWireCanvasItemClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  wire_canvas_item_quark = g_quark_from_static_string ("wire-canvas-item");

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


  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  
  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_FIXED_LAYOUT);
}
