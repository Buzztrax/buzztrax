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
 * SECTION:btmainpagemachines
 * @short_description: the editor main machines page
 * @see_also: #BtSetup
 *
 * Displays the machine setup and wires on a canvas. The area is an infinite
 * canvas that can be moved and zoomed freely.
 */
/* TODO(ensonic): multiselect
 * - when clicking on the background
 *   - remove old selection if any
 *   - start new selection (semi-transparent rect)
 * - have context menu on the selection
 *   - delete
 *   - mute/solo/bypass ?
 * - move machines when moving the selection
 * - selected machines have a glowing border (would need another set of images)
 */
/* TODO(ensonic): move functions in context menu
 * - machines
 *   - clone machine (no patterns)
 *   - remove & relink (remove machine and relink wires)
 *     does not work in all scenarios (we might need to create more wires)
 *     A --\         /-- D
 *          +-- C --+
 *     B --/         \-- E
 *
 * - wires
 *   - insert machine (like menu on canvas)
 *     - what to do with wire-patterns?
 */
/* TODO(ensonic): easier machine manipulation
 * linking the machines with "shift" pressed comes from buzz and is hard to
 * discover. We now have a "connect machines" item in the connect menu, but
 * what about these:
 * 1.) on the toolbar I could have a button that toggles between "move" and
 *     "link". In "move" mode one can move machines with the mouse and in "link"
 *     mode one can link. Would need to have a keyboard shortcut for toggling.
 * 2.) click-zones on the machine-icons. Link the machine when click+drag the
 *     title bar. Make the 'leds' clickable and move the machines outside of
 *     title and led. Eventualy change the mouse-cursor when hovering over the
 *     zones.
 * Option '2' looks nice and would also help on touch-screens.
 */

#define BT_EDIT
#define BT_MAIN_PAGE_MACHINES_C

#include "bt-edit.h"

#include <math.h>

enum
{
  MAIN_PAGE_MACHINES_CANVAS = 1
};

struct _BtMainPageMachinesPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* the toolbar widget */
  GtkWidget *toolbar;

  /* the setup of machines and wires */
  BtSetup *setup;

  /* canvas for machine view, owner of the clutter stage */
  GtkWidget *canvas_widget;
  ClutterActor *stage;
  /* child actor, gets moved by adjustemnts */
  ClutterActor *canvas;
  GtkAdjustment *hadjustment, *vadjustment;
  /* canvas background grid, child of canvas */
  ClutterActor *grid;
  ClutterContent *grid_canvas;

  /* the zoomration in pixels/per unit */
  gdouble zoom;
  /* zomm in/out widgets */
  GtkWidget *zoom_in, *zoom_out;

  /* canvas context_menu */
  GtkMenu *context_menu;

  /* grid density menu */
  GtkMenu *grid_density_menu;
  GSList *grid_density_group;
  /* grid density */
  gulong grid_density;

  /* we probably need a list of canvas items that we have drawn, so that we can
   * easily clear them later
   */
  GHashTable *machines;         // each entry points to BtMachineCanvasItem
  GHashTable *wires;            // each entry points to BtWireCanvasItem

  /* interaction state */
  gboolean connecting, moved, dragging;

  /* cursor for moving */
  GdkCursor *drag_cursor;

  /* used when interactivly adding a new wire */
  ClutterActor *new_wire;
  BtMachineCanvasItem *new_wire_src, *new_wire_dst;

  /* cached setup properties */
  GHashTable *properties;

  /* mouse coodinates on context menu invokation (used for placing new machines) */
  gfloat mouse_x, mouse_y;
  /* machine coordinates before/after draging (needed for undo) */
  gdouble machine_xo, machine_yo;
  gdouble machine_xn, machine_yn;

  /* volume/panorama popup slider */
  BtVolumePopup *vol_popup;
  BtPanoramaPopup *pan_popup;
  GtkAdjustment *vol_popup_adj, *pan_popup_adj;
  GstElement *wire_gain, *wire_pan;

  /* window size */
  gfloat view_w, view_h;
  /* canvas size */
  gfloat canvas_w, canvas_h;
  /* machine boubding box */
  gfloat mi_x, ma_x, mi_y, ma_y;

  /* editor change log */
  BtChangeLog *change_log;
};

//-- the class

static void bt_main_page_machines_change_logger_interface_init (gpointer const
    g_iface, gconstpointer const iface_data);

G_DEFINE_TYPE_WITH_CODE (BtMainPageMachines, bt_main_page_machines,
    GTK_TYPE_BOX, G_IMPLEMENT_INTERFACE (BT_TYPE_CHANGE_LOGGER,
        bt_main_page_machines_change_logger_interface_init));

enum
{
  METHOD_ADD_MACHINE = 0,
  METHOD_REM_MACHINE,
  METHOD_SET_MACHINE_PROPERTY,
  METHOD_ADD_WIRE,
  METHOD_REM_WIRE,
  METHOD_SET_WIRE_PROPERTY,
};

static BtChangeLoggerMethods change_logger_methods[] = {
  BT_CHANGE_LOGGER_METHOD ("add_machine", 12,
      "([0-9]),\"([-_a-zA-Z0-9 ]+)\",\"([-_a-zA-Z0-9 ]+)\"$"),
  BT_CHANGE_LOGGER_METHOD ("rem_machine", 12, "\"([-_a-zA-Z0-9 ]+)\"$"),
  BT_CHANGE_LOGGER_METHOD ("set_machine_property", 21,
      "\"([-_a-zA-Z0-9 ]+)\",\"([-_a-zA-Z0-9]+)\",\"(.+)\"$"),
  BT_CHANGE_LOGGER_METHOD ("add_wire", 9,
      "\"([-_a-zA-Z0-9 ]+)\",\"([-_a-zA-Z0-9 ]+)\"$"),
  BT_CHANGE_LOGGER_METHOD ("rem_wire", 9,
      "\"([-_a-zA-Z0-9 ]+)\",\"([-_a-zA-Z0-9 ]+)\"$"),
  BT_CHANGE_LOGGER_METHOD ("set_wire_property", 18,
      "\"([-_a-zA-Z0-9 ]+)\",\"([-_a-zA-Z0-9 ]+)\",\"([-_a-zA-Z0-9]+)\",\"(.+)\"$"),
  {NULL,}
};

//-- prototypes
static gboolean bt_main_page_machines_check_wire (const BtMainPageMachines *
    self);

static void on_machine_item_start_connect (BtMachineCanvasItem * machine_item,
    gpointer user_data);
static void on_machine_item_position_changed (BtMachineCanvasItem *
    machine_item, ClutterEventType ev_type, gpointer user_data);

//-- converters for coordinate systems

static void
widget_to_canvas_pos (const BtMainPageMachines * self, gfloat wx, gfloat wy,
    gfloat * cx, gfloat * cy)
{
  BtMainPageMachinesPrivate *p = self->priv;
  gdouble ox = gtk_adjustment_get_value (p->hadjustment);
  gdouble oy = gtk_adjustment_get_value (p->vadjustment);
  *cx = (wx + ox) / p->zoom;
  *cy = (wy + oy) / p->zoom;
}

static void
canvas_to_widget_pos (const BtMainPageMachines * self, gfloat cx, gfloat cy,
    gfloat * wx, gfloat * wy)
{
  BtMainPageMachinesPrivate *p = self->priv;
  gdouble ox = gtk_adjustment_get_value (p->hadjustment);
  gdouble oy = gtk_adjustment_get_value (p->vadjustment);
  *wx = (cx * p->zoom) - ox;
  *wy = (cy * p->zoom) - oy;
}

//-- drawing handlers

static gboolean
on_wire_draw (ClutterCanvas * canvas, cairo_t * cr, gint width, gint height,
    gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);
  gfloat ms_x, ms_y, mm_x, mm_y;
  gfloat x1, x2, y1, y2;

  clutter_actor_get_position ((ClutterActor *) self->priv->new_wire_src,
      &ms_x, &ms_y);
  mm_x = self->priv->mouse_x;
  mm_y = self->priv->mouse_y;
  if (ms_x < mm_x) {
    x1 = 0.0;
    x2 = width;
  } else {
    x1 = width;
    x2 = 0.0;
  }
  if (ms_y < mm_y) {
    y1 = 0.0;
    y2 = height;
  } else {
    y1 = height;
    y2 = 0.0;
  }

  /* clear the contents of the canvas, to not paint over the previous frame */
  cairo_save (cr);
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);
  cairo_restore (cr);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

  cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_width (cr, 1.0);
  // IDEA(ensonic): the green is a bit bright, use ui_resources?
  if (self->priv->new_wire_dst && bt_main_page_machines_check_wire (self)) {
    cairo_set_source_rgba (cr, 0.0, 1.0, 0.0, 1.0);
  } else {
    cairo_set_source_rgba (cr, 1.0, 0.0, 0.0, 1.0);
  }

  cairo_move_to (cr, x1, y1);
  cairo_line_to (cr, x2, y2);
  cairo_stroke (cr);

  return TRUE;
}

static gboolean
on_grid_draw (ClutterCanvas * canvas, cairo_t * cr, gint width, gint height,
    gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);
  gdouble i, step, c, rx, ry;
  gfloat gray[4] = { 0.5, 0.85, 0.7, 0.85 };

  GST_INFO ("redrawing grid : density=%lu  canvas=%p, %4d,%4d",
      self->priv->grid_density, self->priv->canvas, width, height);

  if (!self->priv->grid_density)
    return TRUE;

  /* clear the contents of the canvas, to not paint over the previous frame */
  cairo_save (cr);
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);
  cairo_restore (cr);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

  /* scale the modelview to the size of the surface */
  rx = (width / (MACHINE_VIEW_SIZE_X * 2.0));
  ry = (height / (MACHINE_VIEW_SIZE_Y * 2.0));
  cairo_scale (cr, MACHINE_VIEW_SIZE_X, MACHINE_VIEW_SIZE_Y);
  cairo_translate (cr, rx, ry);

  cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_width (cr, 0.001 / self->priv->zoom);  // make relative to zoom

  step = 1.0 / (gdouble) (1 << self->priv->grid_density);
  for (i = 0.0; i <= MAX (rx, ry); i += step) {
    c = gray[((gint) fabs (i * 8.0)) & 0x3];
    cairo_set_source_rgba (cr, c, c, c, 1.0);

    cairo_move_to (cr, -rx, i);
    cairo_line_to (cr, rx, i);
    cairo_stroke (cr);
    cairo_move_to (cr, i, -ry);
    cairo_line_to (cr, i, ry);
    cairo_stroke (cr);

    cairo_move_to (cr, -rx, -i);
    cairo_line_to (cr, rx, -i);
    cairo_stroke (cr);
    cairo_move_to (cr, -i, -ry);
    cairo_line_to (cr, -i, ry);
    cairo_stroke (cr);
  }
  return TRUE;
}

//-- linking signal handler & helper

static void
update_connect (BtMainPageMachines * self)
{
  gfloat ms_x, ms_y, mm_x, mm_y, w, h, x, y;

  clutter_actor_get_position ((ClutterActor *) self->priv->new_wire_src,
      &ms_x, &ms_y);
  mm_x = self->priv->mouse_x;
  mm_y = self->priv->mouse_y;
  w = 1.0 + fabs (mm_x - ms_x);
  h = 1.0 + fabs (mm_y - ms_y);
  x = MIN (mm_x, ms_x);
  y = MIN (mm_y, ms_y);

  clutter_actor_set_size (self->priv->new_wire, w, h);
  clutter_actor_set_position (self->priv->new_wire, x, y);

  ClutterContent *canvas = clutter_actor_get_content (self->priv->new_wire);
  clutter_canvas_set_size (CLUTTER_CANVAS (canvas), w, h);

  self->priv->moved = TRUE;
}

static void
start_connect (BtMainPageMachines * self)
{
  // handle drawing a new wire
  ClutterContent *canvas = clutter_canvas_new ();

  self->priv->new_wire = clutter_actor_new ();
  clutter_actor_set_content (self->priv->new_wire, canvas);
  clutter_actor_set_content_scaling_filters (self->priv->new_wire,
      CLUTTER_SCALING_FILTER_TRILINEAR, CLUTTER_SCALING_FILTER_LINEAR);
  update_connect (self);
  clutter_actor_add_child (self->priv->canvas, self->priv->new_wire);

  g_signal_connect (canvas, "draw", G_CALLBACK (on_wire_draw), self);
  /* invalidate the canvas, so that we can draw before the main loop starts */
  clutter_content_invalidate (canvas);

  clutter_actor_set_child_below_sibling (self->priv->canvas,
      self->priv->new_wire, NULL);
  clutter_actor_set_child_below_sibling (self->priv->canvas,
      self->priv->grid, NULL);
  self->priv->connecting = TRUE;
  self->priv->moved = FALSE;
}

//-- event handler helper

static void
clutter_event_get_cursor_pos (BtMainPageMachines * self, ClutterEvent * event)
{
  gfloat x, y;

  clutter_event_get_coords (event, &x, &y);
  widget_to_canvas_pos (self, x, y, &self->priv->mouse_x, &self->priv->mouse_y);
}

static void
machine_item_moved (const BtMainPageMachines * self,
    BtMachineCanvasItem * machine_item)
{
  gchar *undo_str, *redo_str;
  gchar *mid;
  gchar str[G_ASCII_DTOSTR_BUF_SIZE];

  bt_child_proxy_get (machine_item, "machine::id", &mid, NULL);

  bt_change_log_start_group (self->priv->change_log);

  undo_str =
      g_strdup_printf ("set_machine_property \"%s\",\"xpos\",\"%s\"", mid,
      g_ascii_dtostr (str, G_ASCII_DTOSTR_BUF_SIZE, self->priv->machine_xo));
  redo_str =
      g_strdup_printf ("set_machine_property \"%s\",\"xpos\",\"%s\"", mid,
      g_ascii_dtostr (str, G_ASCII_DTOSTR_BUF_SIZE, self->priv->machine_xn));
  bt_change_log_add (self->priv->change_log, BT_CHANGE_LOGGER (self), undo_str,
      redo_str);
  undo_str =
      g_strdup_printf ("set_machine_property \"%s\",\"ypos\",\"%s\"", mid,
      g_ascii_dtostr (str, G_ASCII_DTOSTR_BUF_SIZE, self->priv->machine_yo));
  redo_str =
      g_strdup_printf ("set_machine_property \"%s\",\"ypos\",\"%s\"", mid,
      g_ascii_dtostr (str, G_ASCII_DTOSTR_BUF_SIZE, self->priv->machine_yn));
  bt_change_log_add (self->priv->change_log, BT_CHANGE_LOGGER (self), undo_str,
      redo_str);

  bt_change_log_end_group (self->priv->change_log);

  g_free (mid);
}

// TODO(ensonic): this method probably should go to BtMachine, but on the other
// hand it is GUI related
static gboolean
machine_view_get_machine_position (GHashTable * properties, gdouble * pos_x,
    gdouble * pos_y)
{
  gboolean res = FALSE;
  gchar *prop;

  if (properties) {
    prop = (gchar *) g_hash_table_lookup (properties, "xpos");
    if (prop) {
      *pos_x += MACHINE_VIEW_SIZE_X * g_ascii_strtod (prop, NULL);
      // do not g_free(prop);
      //GST_DEBUG("  xpos: %+5.1f  %p=\"%s\"",*pos_x,prop,prop);
      res = TRUE;
    } else
      GST_WARNING ("no xpos property found");
    prop = (gchar *) g_hash_table_lookup (properties, "ypos");
    if (prop) {
      *pos_y += MACHINE_VIEW_SIZE_Y * g_ascii_strtod (prop, NULL);
      // do not g_free(prop);
      //GST_DEBUG("  ypos: %+5.1f  %p=\"%s\"",*pos_y,prop,prop);
      res &= TRUE;
    } else
      GST_WARNING ("no ypos property found");
  } else
    GST_WARNING ("no properties supplied");
  return (res);
}

/*
 * update_zoom:
 *
 * this allows canvas items to reload their gfx
 */
static void
update_zoom (gpointer key, gpointer value, gpointer user_data)
{
  g_object_set (value, "zoom", (*(gdouble *) user_data), NULL);
}

static void
update_machines_zoom (const BtMainPageMachines * self)
{
  gchar str[G_ASCII_DTOSTR_BUF_SIZE];

  g_hash_table_insert (self->priv->properties, g_strdup ("zoom"),
      g_strdup (g_ascii_dtostr (str, G_ASCII_DTOSTR_BUF_SIZE,
              self->priv->zoom)));
  bt_edit_application_set_song_unsaved (self->priv->app);

  g_hash_table_foreach (self->priv->machines, update_zoom, &self->priv->zoom);
  g_hash_table_foreach (self->priv->wires, update_zoom, &self->priv->zoom);

  gtk_widget_set_sensitive (self->priv->zoom_out, (self->priv->zoom > 0.4));
  gtk_widget_set_sensitive (self->priv->zoom_in, (self->priv->zoom < 3.0));
}

static void
machine_item_new (const BtMainPageMachines * self, BtMachine * machine,
    gdouble xpos, gdouble ypos)
{
  BtMachineCanvasItem *item;

  item =
      bt_machine_canvas_item_new (self, machine, xpos, ypos, self->priv->zoom);
  g_hash_table_insert (self->priv->machines, machine, item);
  g_signal_connect (item, "start-connect",
      G_CALLBACK (on_machine_item_start_connect), (gpointer) self);
  g_signal_connect (item, "position-changed",
      G_CALLBACK (on_machine_item_position_changed), (gpointer) self);
  on_machine_item_position_changed (item, CLUTTER_MOTION, (gpointer) self);
}

static void
wire_item_new (const BtMainPageMachines * self, BtWire * wire,
    BtMachineCanvasItem * src_machine_item,
    BtMachineCanvasItem * dst_machine_item)
{
  BtWireCanvasItem *item;

  item =
      bt_wire_canvas_item_new (self, wire, src_machine_item, dst_machine_item);
  g_hash_table_insert (self->priv->wires, wire, item);
}

/* TODO(ensonic):
 * - we start with a canvas size of MACHINE_VIEW_SIZE_X, MACHINE_VIEW_SIZE_Y
 *   - unit size for a [-1,-1] .. [+1,+1] rect
 * - self->priv->view_{w,h} is the visible rect
 * - self->priv->canvas_{w,h} is the larger canvas
     - it is large enough to scroll the left content to the right border and
       still see the grid 
 * - we need a bounding box (bb) for all the machines
 *   - from that we can set page-size, upper, lower and pos of the scrollbars
 *   - upper and lower are the min/max of the bounding box
 *   - page-size = MAX (canvas-size, bb-size) / zoom
 * - we need scrollbars when the bb is partially outside the view
 *   - due to zoom or panning
 *   - zoom is limited (0.4 ... 3.0)
 *   - panning needs to be limitted to not allow to move more that 50 off view
 *     bb.size * zoom * 2 would be the scrollbar.size
 * - when adding/removing machines and re-calc the bb, we need to adjust the
 *   scroll-value to keep things on screen where they are
 * - when zooming we need to adjust both value and pagesize
 *
 * - we call this right now:
 *   - after the machines bb has changed
 *     - song loaded, machiens added/removed
 *   - zoom-{in,out,fit}
 *   - when the window has been resized
 * ----------------------------------------------------------------------------
 * we have these cases:
 * - single machine
 *   - initially center
 *   - scroll so that machine center can be on the borders:
 *     h.pagesize=window.width
 *     v.pagesize=window.height
 *       0 ... 100 ... 200 : window.width is 200
 *     -10      90     190 : v.min = bb.x1 - window.width/2 = 90 - 100
 *                           v.max = bb.x1 + window.width/2 = 90 + 100
 * - multiple machines
 *   - initially cernter the middle of the bb
 *   - scroll so that bounding-box can be on the borders:
 *     0 ... 100 ... 200 : window.width is 200
 *        90 ... 150     : bb.width = 150-90 = 60
 *    20 ... 120 ... 220 : bb.cx = 90 + 60/2
 *     
 */

static void
update_adjustment (GtkAdjustment * adj, gdouble bbmi, gdouble bbma, gdouble vw)
{
  gdouble bbd = bbma - bbmi;
  gdouble bbc = bbmi + (bbd / 2.0);
  gdouble adjva, adjmi, adjma, adjd;
  gdouble m, vw2 = vw / 2.0;

  g_object_get (adj, "value", &adjva, "lower", &adjmi, "upper", &adjma, NULL);
  adjd = adjma - adjmi;

  GST_DEBUG ("adj: %3d .. %3d .. %3d = %3d", (gint) adjmi, (gint) adjva,
      (gint) adjma, (gint) adjd);
  GST_DEBUG ("bb : %3d .. %3d .. %3d = %3d", (gint) bbmi, (gint) bbc,
      (gint) bbma, (gint) bbd);

  // relative scrollpos
  //m = (adjd >= bbd) ? ((adjva - adjmi) / adjd) : 0.5;
  m = (adjva - adjmi) / adjd;
  adjmi = bbc - vw2;
  adjma = bbc + vw2;
  adjva = adjmi + (vw * m);

  GST_DEBUG ("adj: %3d .. %3d (%4.2lf) .. %3d = %3d", (gint) adjmi,
      (gint) adjva, m, (gint) adjma, (gint) vw);

  gtk_adjustment_configure (adj, adjva, adjmi, adjma, 1.0, 1.0, 0.0);
}

static void
update_scrolled_window (const BtMainPageMachines * self)
{
  BtMainPageMachinesPrivate *p = self->priv;

#if 0
  // dynamic sizing :/
  update_adjustment (p->hadjustment, p->mi_x, p->ma_x, p->view_w);
  update_adjustment (p->vadjustment, p->mi_y, p->ma_y, p->view_h);
#else
  update_adjustment (p->hadjustment, 0.0, p->view_w, p->view_w);
  update_adjustment (p->vadjustment, 0.0, p->view_h, p->view_h);
#endif
}

static void
machine_actor_move (gpointer key, gpointer value, gpointer user_data)
{
  gfloat *delta = (gfloat *) user_data;

  clutter_actor_move_by ((ClutterActor *) value, delta[0], delta[1]);
  g_signal_emit_by_name (value, "position-changed", 0, CLUTTER_MOTION);
}

static void
update_scrolled_window_zoom (const BtMainPageMachines * self)
{
  BtMainPageMachinesPrivate *p = self->priv;
  // need to make stage+grid large enough to show grid when scrolling
  gfloat cw = 2.0 * (p->view_w / p->zoom);
  gfloat ch = 2.0 * (p->view_h / p->zoom);
  gfloat delta[2] = { (cw - p->canvas_w) / 2.0, (ch - p->canvas_h) / 2.0 };

  GST_DEBUG ("canvas: %4.1f,%4.1f -> %4.1f,%4.1f", p->canvas_w, p->canvas_h, cw,
      ch);
  GST_DEBUG ("delta: %4.1f,%4.1f", delta[0], delta[1]);

  p->canvas_w = cw;
  p->canvas_h = ch;
  clutter_actor_set_size (p->stage, cw, ch);
  clutter_actor_set_size (p->canvas, cw, ch);
  // size of grid 
  clutter_actor_set_size (p->grid, cw, ch);
  clutter_actor_set_position (p->grid, cw / 2.0, ch / 2.0);
  clutter_canvas_set_size (CLUTTER_CANVAS (p->grid_canvas), cw, ch);

  // keep machines centered
  g_hash_table_foreach (p->machines, machine_actor_move, delta);

  update_scrolled_window (self);
}

static gboolean
machine_view_remove_item (gpointer key, gpointer value, gpointer user_data)
{
  clutter_actor_destroy (value);
  return TRUE;
}

static void
machine_view_clear (const BtMainPageMachines * self)
{
  // clear the canvas
  GST_DEBUG ("before destroying machine canvas items");
  g_hash_table_foreach_remove (self->priv->machines,
      machine_view_remove_item, NULL);
  GST_DEBUG ("before destoying wire canvas items");
  g_hash_table_foreach_remove (self->priv->wires,
      machine_view_remove_item, NULL);
  GST_DEBUG ("done");
}

static void
machine_view_refresh (const BtMainPageMachines * self)
{
  GHashTable *properties;
  BtMachineCanvasItem *src_machine_item, *dst_machine_item;
  BtMachine *machine, *src_machine, *dst_machine;
  BtWire *wire;
  gdouble pos_x, pos_y;
  GList *node, *list;
  gchar *prop;

  machine_view_clear (self);

  // update view
  if ((prop = (gchar *) g_hash_table_lookup (self->priv->properties, "zoom"))) {
    self->priv->zoom = g_ascii_strtod (prop, NULL);
    clutter_actor_set_scale (self->priv->canvas, self->priv->zoom,
        self->priv->zoom);
    update_scrolled_window (self);
    GST_INFO ("set zoom to %6.4lf", self->priv->zoom);
  }
  if ((prop = (gchar *) g_hash_table_lookup (self->priv->properties, "xpos"))) {
    gtk_adjustment_set_value (self->priv->hadjustment, g_ascii_strtod (prop,
            NULL));
  } else {
    gdouble xs, xe, xp;
    // center
    g_object_get (self->priv->hadjustment, "lower", &xs, "upper", &xe,
        "page-size", &xp, NULL);
    gtk_adjustment_set_value (self->priv->hadjustment,
        xs + ((xe - xs - xp) * 0.5));
  }
  if ((prop = (gchar *) g_hash_table_lookup (self->priv->properties, "ypos"))) {
    gtk_adjustment_set_value (self->priv->vadjustment, g_ascii_strtod (prop,
            NULL));
  } else {
    gdouble ys, ye, yp;
    // center
    g_object_get (self->priv->vadjustment, "lower", &ys, "upper", &ye,
        "page-size", &yp, NULL);
    gtk_adjustment_set_value (self->priv->vadjustment,
        ys + ((ye - ys - yp) * 0.5));
  }

  GST_INFO ("creating machine canvas items");

  // draw all machines
  g_object_get (self->priv->setup, "machines", &list, NULL);
  for (node = list; node; node = g_list_next (node)) {
    machine = BT_MACHINE (node->data);
    // get position
    g_object_get (machine, "properties", &properties, NULL);
    pos_x = self->priv->canvas_w / 2.0;
    pos_y = self->priv->canvas_h / 2.0;
    machine_view_get_machine_position (properties, &pos_x, &pos_y);
    // draw machine
    machine_item_new (self, machine, pos_x, pos_y);
  }
  g_list_free (list);

  // draw all wires
  g_object_get (self->priv->setup, "wires", &list, NULL);
  for (node = list; node; node = g_list_next (node)) {
    wire = BT_WIRE (node->data);
    // get positions of source and dest
    g_object_get (wire, "src", &src_machine, "dst", &dst_machine, NULL);
    src_machine_item = g_hash_table_lookup (self->priv->machines, src_machine);
    dst_machine_item = g_hash_table_lookup (self->priv->machines, dst_machine);
    // draw wire
    wire_item_new (self, wire, src_machine_item, dst_machine_item);
    g_object_unref (src_machine);
    g_object_unref (dst_machine);
    // TODO(ensonic): get "analyzer-window-state" and if set,
    // get xpos, ypos and open window
  }
  g_list_free (list);
  clutter_actor_set_child_below_sibling (self->priv->canvas, self->priv->grid,
      NULL);
  GST_DEBUG ("drawing done");
}

static void
bt_main_page_machines_add_wire (const BtMainPageMachines * self)
{
  BtSong *song;
  BtWire *wire;
  GError *err = NULL;
  BtMachine *src_machine, *dst_machine;

  g_assert (self->priv->new_wire_src);
  g_assert (self->priv->new_wire_dst);

  g_object_get (self->priv->app, "song", &song, NULL);
  g_object_get (self->priv->new_wire_src, "machine", &src_machine, NULL);
  g_object_get (self->priv->new_wire_dst, "machine", &dst_machine, NULL);

  // try to establish a new connection
  wire = bt_wire_new (song, src_machine, dst_machine, &err);
  if (err == NULL) {
    gchar *undo_str, *redo_str;
    gchar *smid, *dmid;

    // draw wire
    wire_item_new (self, wire, self->priv->new_wire_src,
        self->priv->new_wire_dst);
    clutter_actor_set_child_below_sibling (self->priv->canvas,
        self->priv->grid, NULL);

    g_object_get (src_machine, "id", &smid, NULL);
    g_object_get (dst_machine, "id", &dmid, NULL);

    undo_str = g_strdup_printf ("rem_wire \"%s\",\"%s\"", smid, dmid);
    redo_str = g_strdup_printf ("add_wire \"%s\",\"%s\"", smid, dmid);
    bt_change_log_add (self->priv->change_log, BT_CHANGE_LOGGER (self),
        undo_str, redo_str);
    g_free (smid);
    g_free (dmid);
  } else {
    GST_WARNING ("failed to make wire: %s", err->message);
    g_error_free (err);
    gst_object_unref (wire);
  }
  g_object_unref (dst_machine);
  g_object_unref (src_machine);
  g_object_unref (song);
}

static BtMachineCanvasItem *
get_machine_canvas_item_under_cursor (const BtMainPageMachines * self)
{
  ClutterActor *ci;
  gfloat x, y;

  canvas_to_widget_pos (self, self->priv->mouse_x, self->priv->mouse_y, &x, &y);

  //GST_DEBUG ("is there a machine at pos?");
  if ((ci = clutter_stage_get_actor_at_pos ((ClutterStage *) self->priv->stage,
              CLUTTER_PICK_REACTIVE, x, y))) {
    if (BT_IS_MACHINE_CANVAS_ITEM (ci)) {
      //GST_DEBUG("  yes!");
      return BT_MACHINE_CANVAS_ITEM (g_object_ref (ci));
    }
  }
  return NULL;
}

static gboolean
bt_main_page_machines_check_wire (const BtMainPageMachines * self)
{
  gboolean ret = FALSE;
  BtMachine *src_machine, *dst_machine;

  GST_INFO ("can we link to it?");

  g_assert (self->priv->new_wire_src);
  g_assert (self->priv->new_wire_dst);

  g_object_get (self->priv->new_wire_src, "machine", &src_machine, NULL);
  g_object_get (self->priv->new_wire_dst, "machine", &dst_machine, NULL);

  // if the citem->machine is a sink/processor-machine
  if (BT_IS_SINK_MACHINE (dst_machine) || BT_IS_PROCESSOR_MACHINE (dst_machine)) {
    // check if these machines are not yet connected
    if (bt_wire_can_link (src_machine, dst_machine)) {
      ret = TRUE;
      GST_INFO ("  yes!");
    }
  }
  g_object_unref (dst_machine);
  g_object_unref (src_machine);
  return (ret);
}

//-- event handler

static void
on_machine_item_start_connect (BtMachineCanvasItem * machine_item,
    gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);

  self->priv->new_wire_src = g_object_ref (machine_item);
  g_object_get (machine_item, "x", &self->priv->mouse_x, "y",
      &self->priv->mouse_y, NULL);
  start_connect (self);
}

static void
machine_actor_update_bb (gpointer key, gpointer value, gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);
  gfloat px, py;

  clutter_actor_get_position ((ClutterActor *) value, &px, &py);
  if (px < self->priv->mi_x)
    self->priv->mi_x = px;
  else if (px > self->priv->ma_x)
    self->priv->ma_x = px;
  if (py < self->priv->mi_y)
    self->priv->mi_y = py;
  else if (py > self->priv->ma_y)
    self->priv->ma_y = py;
}

static void
machine_actor_update_pos_and_bb (BtMainPageMachines * self, ClutterActor * ci,
    gfloat cw, gfloat ch, gdouble * x, gdouble * y)
{
  gfloat px, py;

  clutter_actor_get_position (ci, &px, &py);
  if (x && y) {
    *x = (px - (cw / 2.0)) / cw;
    *y = (py - (ch / 2.0)) / ch;
  }

  self->priv->mi_x = self->priv->ma_x = px;
  self->priv->mi_y = self->priv->ma_y = py;
  g_hash_table_foreach (self->priv->machines, machine_actor_update_bb, self);
  GST_DEBUG ("bb: %4.2f,%4.2f .. %4.2f,%4.2f",
      self->priv->mi_x, self->priv->mi_y, self->priv->ma_x, self->priv->ma_y);
  update_scrolled_window (self);
}

static void
on_machine_item_position_changed (BtMachineCanvasItem * machine_item,
    ClutterEventType ev_type, gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);

  switch (ev_type) {
    case CLUTTER_BUTTON_PRESS:
      machine_actor_update_pos_and_bb (self, (ClutterActor *) machine_item,
          self->priv->view_w, self->priv->view_h,
          &self->priv->machine_xo, &self->priv->machine_yo);
      break;
    case CLUTTER_MOTION:
      machine_actor_update_pos_and_bb (self, (ClutterActor *) machine_item,
          0.0, 0.0, NULL, NULL);
      break;
    case CLUTTER_BUTTON_RELEASE:
      machine_actor_update_pos_and_bb (self, (ClutterActor *) machine_item,
          self->priv->view_w, self->priv->view_h,
          &self->priv->machine_xn, &self->priv->machine_yn);
      machine_item_moved (self, machine_item);
      break;
    default:
      break;
  }
}

static void
on_machine_added (BtSetup * setup, BtMachine * machine, gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);
  GHashTable *properties;
  gdouble pos_x, pos_y;

  GST_INFO_OBJECT (machine, "new machine added: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine));

  g_object_get (machine, "properties", &properties, NULL);
  if (properties) {
    pos_x = self->priv->canvas_w / 2.0;
    pos_y = self->priv->canvas_h / 2.0;
    if (!machine_view_get_machine_position (properties, &pos_x, &pos_y)) {
      gchar str[G_ASCII_DTOSTR_BUF_SIZE];
      pos_x = self->priv->mouse_x;
      pos_y = self->priv->mouse_y;
      g_hash_table_insert (properties, g_strdup ("xpos"),
          g_strdup (g_ascii_dtostr (str, G_ASCII_DTOSTR_BUF_SIZE,
                  (pos_x / MACHINE_VIEW_SIZE_X))));
      g_hash_table_insert (properties, g_strdup ("ypos"),
          g_strdup (g_ascii_dtostr (str, G_ASCII_DTOSTR_BUF_SIZE,
                  (pos_y / MACHINE_VIEW_SIZE_Y))));
    }
  } else {
    pos_x = self->priv->mouse_x;
    pos_y = self->priv->mouse_y;
  }
  self->priv->machine_xn = pos_x / MACHINE_VIEW_SIZE_X;
  self->priv->machine_yn = pos_y / MACHINE_VIEW_SIZE_Y;

  GST_DEBUG_OBJECT (machine,
      "adding machine at %lf x %lf, mouse is at %lf x %lf", pos_x, pos_y,
      self->priv->mouse_x, self->priv->mouse_y);

  // draw machine
  machine_item_new (self, machine, pos_x, pos_y);

  GST_INFO_OBJECT (machine, "... machine added: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine));
}

static void
on_machine_removed (BtSetup * setup, BtMachine * machine, gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);
  BtMachineCanvasItem *item;

  if (!machine)
    return;

  GST_INFO_OBJECT (machine, "machine removed: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine));

  if ((item = g_hash_table_lookup (self->priv->machines, machine))) {
    GST_INFO ("now removing machine-item : %p", item);
    g_hash_table_remove (self->priv->machines, machine);
    g_object_unref (item);
  }

  GST_INFO_OBJECT (machine, "... machine removed: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine));
}

static void
on_wire_removed (BtSetup * setup, BtWire * wire, gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);
  BtWireCanvasItem *item;

  if (!wire) {
    return;
  }

  GST_INFO_OBJECT (wire, "wire removed: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (wire));

  // add undo/redo details
  if (bt_change_log_is_active (self->priv->change_log)) {
    gchar *undo_str, *redo_str;
    gchar *prop;
    BtMachine *src_machine, *dst_machine;
    GHashTable *properties;
    gchar *smid, *dmid;

    g_object_get (wire, "src", &src_machine, "dst", &dst_machine, "properties",
        &properties, NULL);
    g_object_get (src_machine, "id", &smid, NULL);
    g_object_get (dst_machine, "id", &dmid, NULL);

    bt_change_log_start_group (self->priv->change_log);

    undo_str = g_strdup_printf ("add_wire \"%s\",\"%s\"", smid, dmid);
    redo_str = g_strdup_printf ("rem_wire \"%s\",\"%s\"", smid, dmid);
    bt_change_log_add (self->priv->change_log, BT_CHANGE_LOGGER (self),
        undo_str, redo_str);

    prop = (gchar *) g_hash_table_lookup (properties, "analyzer-shown");
    if (prop && *prop == '1') {
      undo_str =
          g_strdup_printf
          ("set_wire_property \"%s\",\"%s\",\"analyzer-shown\",\"1\"", smid,
          dmid);
      bt_change_log_add (self->priv->change_log, BT_CHANGE_LOGGER (self),
          undo_str, g_strdup (undo_str));
    }
    // TODO(ensonic): volume and panorama
    g_free (smid);
    g_free (dmid);

    bt_change_log_end_group (self->priv->change_log);

    g_object_unref (dst_machine);
    g_object_unref (src_machine);
  }

  if ((item = g_hash_table_lookup (self->priv->wires, wire))) {
    GST_INFO ("now removing wire-item : %p", item);
    g_hash_table_remove (self->priv->wires, wire);
    g_object_unref (item);
  }

  GST_INFO_OBJECT (wire, "... wire removed: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (wire));
}

static void
on_song_changed (const BtEditApplication * app, GParamSpec * arg,
    gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);
  BtSong *song;

  GST_INFO ("song has changed : app=%p, self=%p", app, self);
  g_object_try_unref (self->priv->setup);

  // get song from app
  g_object_get (self->priv->app, "song", &song, NULL);
  if (!song) {
    self->priv->setup = NULL;
    self->priv->properties = NULL;
    machine_view_clear (self);
    GST_INFO ("song (null) has changed done");
    return;
  }
  GST_INFO ("song: %" G_OBJECT_REF_COUNT_FMT, G_OBJECT_LOG_REF_COUNT (song));

  g_object_get (song, "setup", &self->priv->setup, NULL);
  g_object_get (self->priv->setup, "properties", &self->priv->properties, NULL);
  // update page
  machine_view_refresh (self);
  g_signal_connect (self->priv->setup, "machine-added",
      G_CALLBACK (on_machine_added), (gpointer) self);
  g_signal_connect (self->priv->setup, "machine-removed",
      G_CALLBACK (on_machine_removed), (gpointer) self);
  g_signal_connect (self->priv->setup, "wire-removed",
      G_CALLBACK (on_wire_removed), (gpointer) self);
  // release the reference
  g_object_unref (song);
  GST_INFO ("song has changed done");
}

static void
on_toolbar_zoom_fit_clicked (GtkButton * button, gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);
  BtMainPageMachinesPrivate *p = self->priv;
  BtMachine *machine;
  GHashTable *properties;
  GList *node, *list;
  gdouble fc_x, fc_y, c_x, c_y, ms;
  // machine area
  gdouble ma_xs = MACHINE_VIEW_SIZE_X, ma_x, ma_xe =
      -MACHINE_VIEW_SIZE_X, ma_xd;
  gdouble ma_ys = MACHINE_VIEW_SIZE_Y, ma_y, ma_ye =
      -MACHINE_VIEW_SIZE_Y, ma_yd;
  // page area
  gdouble /*pg_xs,pg_x,pg_xe,pg_xd, */ pg_xl;
  gdouble /*pg_ys,pg_y,pg_ye,pg_yd, */ pg_yl;
  gdouble old_zoom = self->priv->zoom;

  //calculate bounds
  g_object_get (p->setup, "machines", &list, NULL);
  for (node = list; node; node = g_list_next (node)) {
    machine = BT_MACHINE (node->data);
    // get position
    g_object_get (machine, "properties", &properties, NULL);
    ma_x = p->canvas_w / 2.0;
    ma_y = p->canvas_h / 2.0;
    machine_view_get_machine_position (properties, &ma_x, &ma_y);
    if (ma_x < ma_xs)
      ma_xs = ma_x;
    if (ma_x > ma_xe)
      ma_xe = ma_x;
    if (ma_y < ma_ys)
      ma_ys = ma_y;
    if (ma_y > ma_ye)
      ma_ye = ma_y;
  }
  g_list_free (list);
  GST_WARNING
      ("machines ranging from x:%+6.4lf...%+6.4lf and y:%+6.4lf...%+6.4lf",
      ma_xs, ma_xe, ma_ys, ma_ye);
  GST_WARNING
      ("bb       ranging from x:%+6.4lf...%+6.4lf and y:%+6.4lf...%+6.4lf",
      p->mi_x, p->ma_x, p->mi_y, p->ma_y);

  /* need to add machine extends + some space */
  ms = MACHINE_SIZE_X;
#if 0
  ma_xs -= ms;
  ma_xe += ms;
#else
  ma_xs = p->mi_x - ms;
  ma_xe = p->ma_x + ms;
#endif
  ma_xd = (ma_xe - ma_xs);
  ms = MACHINE_SIZE_Y;
#if 0
  ma_ys -= ms;
  ma_ye += ms;
#else
  ma_ys = p->mi_y - ms;
  ma_ye = p->ma_y + ms;
#endif
  ma_yd = (ma_ye - ma_ys);

#if 0
  g_object_get (self->priv->hadjustment,
      /*"lower",&pg_xs,"value",&pg_x,"upper",&pg_xe, */ "page-size", &pg_xl,
      NULL);
  g_object_get (self->priv->vadjustment,
      /*"lower",&pg_ys,"value",&pg_y,"upper",&pg_ye, */ "page-size", &pg_yl,
      NULL);
  /*
     pg_xd=(pg_xe-pg_xs)/MACHINE_VIEW_SIZE_X;
     pg_yd=(pg_ye-pg_ys)/MACHINE_VIEW_SIZE_Y;
     GST_INFO("page: pos x/y:%+6.4lf %+6.4lf size x/y: %+6.4lf %+6.4lf -> ranging from x:%+6.4lf...%+6.4lf and y:%+6.4lf...%+6.4lf",
     pg_x,pg_y,pg_xl,pg_yl, pg_xs,pg_xe,pg_ys,pg_ye);
   */
#else
  pg_xl = p->view_w;
  pg_yl = p->view_h;
#endif

  // zoom
  fc_x = pg_xl / ma_xd;
  fc_y = pg_yl / ma_yd;
  GST_WARNING
      ("zoom old=%6.4lf, x:%+6.4lf / %+6.4lf = %+6.4lf and y:%+6.4lf / %+6.4lf = %+6.4lf",
      self->priv->zoom, pg_xl, ma_xd, fc_x, pg_yl, ma_yd, fc_y);
  self->priv->zoom = MIN (fc_x, fc_y);

  // center region
  /* pos can be between: lower ... upper-page_size) */
  GST_INFO ("x: (%+6.4lf-%+6.4lf)/2=%+6.4lf", pg_xl, (ma_xd * p->zoom),
      ((pg_xl - (ma_xd * p->zoom)) / 2.0));
  GST_INFO ("y: (%+6.4lf-%+6.4lf)/2=%+6.4lf", pg_yl, (ma_yd * p->zoom),
      ((pg_yl - (ma_yd * p->zoom)) / 2.0));
  c_x = (MACHINE_VIEW_SIZE_X + ma_xs) * p->zoom -
      ((pg_xl - (ma_xd * p->zoom)) / 2.0);
  c_y = (MACHINE_VIEW_SIZE_Y + ma_ys) * p->zoom -
      ((pg_yl - (ma_yd * p->zoom)) / 2.0);
  gtk_adjustment_set_value (p->hadjustment, c_x);
  gtk_adjustment_set_value (p->vadjustment, c_y);
  GST_WARNING ("toolbar zoom_fit: zoom = %lf, center x/y = %+6.4lf,%+6.4lf",
      p->zoom, c_x, c_y);

  if (p->zoom > old_zoom) {
    clutter_actor_set_scale (p->canvas, p->zoom, p->zoom);
    update_scrolled_window_zoom (self);
    update_machines_zoom (self);
  } else {
    update_machines_zoom (self);
    clutter_actor_set_scale (p->canvas, p->zoom, p->zoom);
    update_scrolled_window_zoom (self);
  }

  gtk_widget_grab_focus_savely (GTK_WIDGET (p->canvas_widget));

}

static void
on_toolbar_zoom_in_clicked (GtkButton * button, gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);

  self->priv->zoom *= 1.2;
  GST_INFO ("toolbar zoom_in event occurred : %lf", self->priv->zoom);

  clutter_actor_set_scale (self->priv->canvas, self->priv->zoom,
      self->priv->zoom);
  update_scrolled_window_zoom (self);
  update_machines_zoom (self);

  gtk_widget_grab_focus_savely (GTK_WIDGET (self->priv->canvas_widget));
}

static void
on_toolbar_zoom_out_clicked (GtkButton * button, gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);

  self->priv->zoom /= 1.2;
  GST_INFO ("toolbar zoom_out event occurred : %lf", self->priv->zoom);

  update_machines_zoom (self);
  clutter_actor_set_scale (self->priv->canvas, self->priv->zoom,
      self->priv->zoom);
  update_scrolled_window_zoom (self);

  gtk_widget_grab_focus_savely (GTK_WIDGET (self->priv->canvas_widget));
}

#ifndef GRID_USES_MENU_TOOL_ITEM
static void
on_toolbar_grid_clicked (GtkButton * button, gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);

  gtk_menu_popup (self->priv->grid_density_menu, NULL, NULL, NULL, NULL, 1,
      gtk_get_current_event_time ());
}
#endif

static void
on_toolbar_grid_densitoff_y_activated (GtkMenuItem * menuitem,
    gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);

  if (!gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)))
    return;

  self->priv->grid_density = 0;
  bt_child_proxy_set (self->priv->app, "settings::grid-density", "off", NULL);
  clutter_content_invalidate (self->priv->grid_canvas);
}

static void
on_toolbar_grid_density_low_activated (GtkMenuItem * menuitem,
    gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);

  if (!gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)))
    return;

  self->priv->grid_density = 1;
  bt_child_proxy_set (self->priv->app, "settings::grid-density", "low", NULL);
  clutter_content_invalidate (self->priv->grid_canvas);
}

static void
on_toolbar_grid_density_mid_activated (GtkMenuItem * menuitem,
    gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);

  if (!gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)))
    return;

  self->priv->grid_density = 2;
  bt_child_proxy_set (self->priv->app, "settings::grid-density", "medium",
      NULL);
  clutter_content_invalidate (self->priv->grid_canvas);
}

static void
on_toolbar_grid_density_high_activated (GtkMenuItem * menuitem,
    gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);

  if (!gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)))
    return;

  self->priv->grid_density = 3;
  bt_child_proxy_set (self->priv->app, "settings::grid-density", "high", NULL);
  clutter_content_invalidate (self->priv->grid_canvas);
}

static void
on_toolbar_menu_clicked (GtkButton * button, gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);

  gtk_menu_popup (self->priv->context_menu, NULL, NULL, NULL, NULL, 1,
      gtk_get_current_event_time ());
}

static void
on_context_menu_unmute_all (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);
  GList *node, *list;
  BtMachine *machine;

  g_object_get (self->priv->setup, "machines", &list, NULL);
  for (node = list; node; node = g_list_next (node)) {
    machine = BT_MACHINE (node->data);
    // TODO(ensonic): this also un-solos and un-bypasses
    g_object_set (machine, "state", BT_MACHINE_STATE_NORMAL, NULL);
  }
  g_list_free (list);
}

static void
on_canvas_size_changed (GtkWidget * widget, GdkRectangle * allocation,
    gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);
  BtMainPageMachinesPrivate *p = self->priv;
  gfloat vw = allocation->width, vh = allocation->height;

  if ((vw == p->view_w) && (vh == p->view_h))
    return;

  GST_DEBUG ("view: %4.1f,%4.1f -> %4.1f,%4.1f", p->view_w, p->view_h, vw, vh);

  // size of the canvas (view)
  p->view_w = vw;
  p->view_h = vh;

  update_scrolled_window_zoom (self);
}

static void
store_scroll_pos (BtMainPageMachines * self, gchar * name, gdouble val)
{
  GST_LOG ("%s: %lf", name, val);
  if (self->priv->properties) {
    gchar str[G_ASCII_DTOSTR_BUF_SIZE];
    gchar *prop;
    gdouble oval = 0.0;
    gboolean have_val = FALSE;

    if ((prop = (gchar *) g_hash_table_lookup (self->priv->properties, name))) {
      oval = g_ascii_strtod (prop, NULL);
      have_val = TRUE;
    }
    if ((!have_val) || (oval != val)) {
      g_hash_table_insert (self->priv->properties, g_strdup ("ypos"),
          g_strdup (g_ascii_dtostr (str, G_ASCII_DTOSTR_BUF_SIZE, val)));
      if (have_val)
        bt_edit_application_set_song_unsaved (self->priv->app);
    }
  }
}

static void
on_vadjustment_changed (GtkAdjustment * adjustment, gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);
  gdouble val = gtk_adjustment_get_value (adjustment);

  clutter_actor_set_y (self->priv->canvas, -val);
  store_scroll_pos (self, "ypos", val);
}

static void
on_hadjustment_changed (GtkAdjustment * adjustment, gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);
  gdouble val = gtk_adjustment_get_value (adjustment);

  clutter_actor_set_x (self->priv->canvas, -val);
  store_scroll_pos (self, "xpos", val);
}

static void
on_page_mapped (GtkWidget * widget, gpointer user_data)
{
  GTK_WIDGET_GET_CLASS (widget)->focus (widget, GTK_DIR_TAB_FORWARD);
}

static void
on_page_switched (GtkNotebook * notebook, GParamSpec * arg, gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);
  guint page_num;
  static gint prev_page_num = -1;

  g_object_get (notebook, "page", &page_num, NULL);

  if (page_num == BT_MAIN_PAGES_MACHINES_PAGE) {
    // only do this if the page really has changed
    if (prev_page_num != BT_MAIN_PAGES_MACHINES_PAGE) {
      GST_DEBUG ("enter machine page");
    }
  } else {
    // only do this if the page was BT_MAIN_PAGES_MACHINES_PAGE
    if (prev_page_num == BT_MAIN_PAGES_MACHINES_PAGE) {
      GST_DEBUG ("leave machine page");
      bt_child_proxy_set (self->priv->app,
          "main-window::statusbar::status", NULL, NULL);
    }
  }
  prev_page_num = page_num;
}

static gboolean
on_canvas_button_press (ClutterActor * actor, ClutterEvent * event,
    gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);
  gboolean res = FALSE;
  BtMachineCanvasItem *ci;
  guint32 button = clutter_event_get_button (event);

  GST_DEBUG ("button-press: %d", event->button.button);
  // store mouse coordinates, so that we can later place a newly added machine there
  clutter_event_get_cursor_pos (self, event);

  if (self->priv->connecting) {
    return TRUE;
  }

  if (!(ci = get_machine_canvas_item_under_cursor (self))) {
    if (button == 1) {
      // start dragging the canvas
      self->priv->dragging = TRUE;
    } else if (button == 3) {
      // show context menu
      gtk_menu_popup (self->priv->context_menu, NULL, NULL, NULL, NULL, 3,
          clutter_event_get_time (event));
      res = TRUE;
    }
  } else {
    if (button == 1) {
      if (clutter_event_get_state (event) & CLUTTER_SHIFT_MASK) {
        BtMachine *machine;
        self->priv->new_wire_src = BT_MACHINE_CANVAS_ITEM (g_object_ref (ci));
        g_object_get (ci, "machine", &machine, NULL);
        // if the citem->machine is a source/processor-machine
        if (BT_IS_SOURCE_MACHINE (machine)
            || BT_IS_PROCESSOR_MACHINE (machine)) {
          start_connect (self);
          res = TRUE;
        }
        g_object_unref (machine);
      }
    }
    g_object_unref (ci);
  }
  return res;
}

static gboolean
on_canvas_button_release (ClutterActor * actor, ClutterEvent * event,
    gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);
  gboolean res = FALSE;

  GST_DEBUG ("button-release: %d", event->button.button);
  if (self->priv->connecting) {
    clutter_event_get_cursor_pos (self, event);
    g_object_try_unref (self->priv->new_wire_dst);
    self->priv->new_wire_dst = get_machine_canvas_item_under_cursor (self);
    if (self->priv->new_wire_dst) {
      if (bt_main_page_machines_check_wire (self)) {
        bt_main_page_machines_add_wire (self);
      }
      g_object_unref (self->priv->new_wire_dst);
      self->priv->new_wire_dst = NULL;
    }
    g_object_unref (self->priv->new_wire_src);
    self->priv->new_wire_src = NULL;
    clutter_actor_remove_child (self->priv->canvas, self->priv->new_wire);
    self->priv->new_wire = NULL;
    self->priv->connecting = FALSE;
  } else if (self->priv->dragging) {
    self->priv->dragging = FALSE;
  }
  return res;
}

static gboolean
on_canvas_motion (ClutterActor * actor, ClutterEvent * event,
    gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);
  BtMainPageMachinesPrivate *p = self->priv;
  gboolean res = FALSE;

  //GST_DEBUG("motion: %f,%f",event->button.x,event->button.y);
  if (p->connecting) {
    // update the connection line
    clutter_event_get_cursor_pos (self, event);
    g_object_try_unref (p->new_wire_dst);
    p->new_wire_dst = get_machine_canvas_item_under_cursor (self);
    update_connect (self);
    res = TRUE;
  } else if (self->priv->dragging) {
    gfloat x = p->mouse_x, y = p->mouse_y;
    gdouble ox = gtk_adjustment_get_value (p->hadjustment);
    gdouble oy = gtk_adjustment_get_value (p->vadjustment);
    // snapshot current mousepos and calculate delta
    clutter_event_get_cursor_pos (self, event);
    gfloat dx = x - p->mouse_x;
    gfloat dy = y - p->mouse_y;
    // scroll canvas
    gtk_adjustment_set_value (p->hadjustment, ox + dx);
    gtk_adjustment_set_value (p->vadjustment, oy + dy);
    p->mouse_x += dx;
    p->mouse_y += dy;
    res = TRUE;
  }
  return res;
}

static gboolean
on_canvas_key_release (ClutterActor * actor, ClutterEvent * event,
    gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);
  ClutterKeyEvent *key_event = (ClutterKeyEvent *) event;
  gboolean res = FALSE;

  clutter_event_get_cursor_pos (self, event);
  GST_DEBUG ("GDK_KEY_RELEASE: %d", key_event->keyval);
  switch (key_event->keyval) {
    case GDK_KEY_Menu:
      // show context menu
      gtk_menu_popup (self->priv->context_menu, NULL, NULL, NULL, NULL, 3,
          gtk_get_current_event_time ());
      res = TRUE;
      break;
    default:
      break;
  }
  return res;
}


static void
on_toolbar_style_changed (const BtSettings * settings, GParamSpec * arg,
    gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);
  gchar *toolbar_style;
  g_object_get ((gpointer) settings, "toolbar-style", &toolbar_style, NULL);
  if (!BT_IS_STRING (toolbar_style))
    return;
  GST_INFO ("!!!  toolbar style has changed '%s'", toolbar_style);
  gtk_toolbar_set_style (GTK_TOOLBAR (self->priv->toolbar),
      gtk_toolbar_get_style_from_string (toolbar_style));
  g_free (toolbar_style);
}

static void
on_volume_popup_changed (GtkAdjustment * adj, gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);
  // FIXME(ensonic): workaround for https://bugzilla.gnome.org/show_bug.cgi?id=667598
  gdouble gain = 4.0 - (gtk_adjustment_get_value (adj) / 100.0);
  g_object_set (self->priv->wire_gain, "volume", gain, NULL);
}

static void
on_panorama_popup_changed (GtkAdjustment * adj, gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);
  gfloat pan = (gfloat) gtk_adjustment_get_value (adj) / 100.0;
  g_object_set (self->priv->wire_pan, "panorama", pan, NULL);
}

//-- helper methods

static void
bt_main_page_machines_init_main_context_menu (const BtMainPageMachines * self)
{
  GtkWidget *menu_item, *menu, *image;
  self->priv->context_menu = GTK_MENU (g_object_ref_sink (gtk_menu_new ()));
  //menu_item=gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD,NULL);
  menu_item = gtk_image_menu_item_new_with_label (_("Add machine"));
  image = gtk_image_new_from_stock (GTK_STOCK_ADD, GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), image);
  gtk_menu_shell_append (GTK_MENU_SHELL (self->priv->context_menu), menu_item);
  gtk_widget_show (menu_item);
  // add machine selection sub-menu
  menu = GTK_WIDGET (bt_machine_menu_new (self));
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), menu);
  menu_item = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (GTK_MENU_SHELL (self->priv->context_menu), menu_item);
  gtk_widget_show (menu_item);
  menu_item = gtk_menu_item_new_with_label (_("Unmute all machines"));
  gtk_menu_shell_append (GTK_MENU_SHELL (self->priv->context_menu), menu_item);
  g_signal_connect (menu_item, "activate",
      G_CALLBACK (on_context_menu_unmute_all), (gpointer) self);
  gtk_widget_show (menu_item);
}

static void
bt_main_page_machines_init_grid_density_menu (const BtMainPageMachines * self)
{
  GtkWidget *menu_item;
  // create grid-density menu with grid-density={off,low,mid,high}
  self->priv->grid_density_menu =
      GTK_MENU (g_object_ref_sink (gtk_menu_new ()));
  // background grid density
  menu_item =
      gtk_radio_menu_item_new_with_label (self->priv->grid_density_group,
      _("Off"));
  self->priv->grid_density_group =
      gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (menu_item));
  if (self->priv->grid_density == 0)
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item), TRUE);
  gtk_menu_shell_append (GTK_MENU_SHELL (self->priv->grid_density_menu),
      menu_item);
  gtk_widget_show (menu_item);
  g_signal_connect (menu_item, "activate",
      G_CALLBACK (on_toolbar_grid_densitoff_y_activated), (gpointer) self);
  menu_item =
      gtk_radio_menu_item_new_with_label (self->priv->grid_density_group,
      _("Low"));
  self->priv->grid_density_group =
      gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (menu_item));
  if (self->priv->grid_density == 1)
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item), TRUE);
  gtk_menu_shell_append (GTK_MENU_SHELL (self->priv->grid_density_menu),
      menu_item);
  gtk_widget_show (menu_item);
  g_signal_connect (menu_item, "activate",
      G_CALLBACK (on_toolbar_grid_density_low_activated), (gpointer) self);
  menu_item =
      gtk_radio_menu_item_new_with_label (self->priv->grid_density_group,
      _("Medium"));
  self->priv->grid_density_group =
      gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (menu_item));
  if (self->priv->grid_density == 2)
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item), TRUE);
  gtk_menu_shell_append (GTK_MENU_SHELL (self->priv->grid_density_menu),
      menu_item);
  gtk_widget_show (menu_item);
  g_signal_connect (menu_item, "activate",
      G_CALLBACK (on_toolbar_grid_density_mid_activated), (gpointer) self);
  menu_item =
      gtk_radio_menu_item_new_with_label (self->priv->grid_density_group,
      _("High"));
  self->priv->grid_density_group =
      gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (menu_item));
  if (self->priv->grid_density == 3)
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item), TRUE);
  gtk_menu_shell_append (GTK_MENU_SHELL (self->priv->grid_density_menu),
      menu_item);
  gtk_widget_show (menu_item);
  g_signal_connect (menu_item, "activate",
      G_CALLBACK (on_toolbar_grid_density_high_activated), (gpointer) self);
}

static void
bt_main_page_machines_init_ui (const BtMainPageMachines * self,
    const BtMainPages * pages)
{
  BtSettings *settings;
  GtkWidget *image, *table, *scrollbar;
  GtkToolItem *tool_item;
  gchar *density;

  GST_DEBUG ("!!!! self=%p", self);
  gtk_widget_set_name (GTK_WIDGET (self), "machine view");
  g_object_get (self->priv->app, "settings", &settings, NULL);
  g_object_get (settings, "grid-density", &density, NULL);
  if (!strcmp (density, "off"))
    self->priv->grid_density = 0;
  else if (!strcmp (density, "low"))
    self->priv->grid_density = 1;
  else if (!strcmp (density, "medium"))
    self->priv->grid_density = 2;
  else if (!strcmp (density, "high"))
    self->priv->grid_density = 3;
  g_free (density);

  // create grid-density menu
  bt_main_page_machines_init_grid_density_menu (self);

  // create the context menu
  bt_main_page_machines_init_main_context_menu (self);

  // add toolbar
  self->priv->toolbar = gtk_toolbar_new ();
  gtk_widget_set_name (self->priv->toolbar, "machine view toolbar");
  tool_item = gtk_tool_button_new_from_stock (GTK_STOCK_ZOOM_FIT);
  gtk_widget_set_name (GTK_WIDGET (tool_item), "Zoom Fit");
  gtk_tool_item_set_tooltip_text (tool_item,
      _("Zoom in/out so that everything is visible"));
  gtk_toolbar_insert (GTK_TOOLBAR (self->priv->toolbar), tool_item, -1);
  g_signal_connect (tool_item, "clicked",
      G_CALLBACK (on_toolbar_zoom_fit_clicked), (gpointer) self);
  self->priv->zoom_in =
      GTK_WIDGET (gtk_tool_button_new_from_stock (GTK_STOCK_ZOOM_IN));
  gtk_widget_set_name (self->priv->zoom_in, "Zoom In");
  gtk_widget_set_sensitive (self->priv->zoom_in, (self->priv->zoom < 3.0));
  gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM (self->priv->zoom_in),
      _("Zoom in for more details"));
  gtk_toolbar_insert (GTK_TOOLBAR (self->priv->toolbar),
      GTK_TOOL_ITEM (self->priv->zoom_in), -1);
  g_signal_connect (self->priv->zoom_in, "clicked",
      G_CALLBACK (on_toolbar_zoom_in_clicked), (gpointer) self);
  self->priv->zoom_out =
      GTK_WIDGET (gtk_tool_button_new_from_stock (GTK_STOCK_ZOOM_OUT));
  gtk_widget_set_name (self->priv->zoom_out, "Zoom Out");
  gtk_widget_set_sensitive (self->priv->zoom_out, (self->priv->zoom > 0.4));
  gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM (self->priv->zoom_out),
      _("Zoom out for better overview"));
  gtk_toolbar_insert (GTK_TOOLBAR (self->priv->toolbar),
      GTK_TOOL_ITEM (self->priv->zoom_out), -1);
  g_signal_connect (self->priv->zoom_out, "clicked",
      G_CALLBACK (on_toolbar_zoom_out_clicked), (gpointer) self);
  gtk_toolbar_insert (GTK_TOOLBAR (self->priv->toolbar),
      gtk_separator_tool_item_new (), -1);

  // grid density toolbar icon
#ifdef GRID_USES_MENU_TOOL_ITEM
  // this is weird, we and up with a button and a menu, instead of a joint thing
  // so this is probably meant for e.g. undo, where the button undos and the
  // menu allows to undo up to a certain step
  image =
      gtk_image_new_from_icon_name ("buzztrax_menu_grid", GTK_ICON_SIZE_MENU);
  tool_item = gtk_menu_tool_button_new (image, _("Grid"));
  gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (tool_item),
      GTK_WIDGET (self->priv->grid_density_menu));
  gtk_menu_tool_button_set_arrow_tooltip_text (GTK_MENU_TOOL_BUTTON (tool_item),
      _("Show background grid"));
  gtk_tool_item_set_tooltip_text (tool_item, _("Show background grid"));
  gtk_toolbar_insert (GTK_TOOLBAR (self->priv->toolbar), tool_item, -1);
  //g_signal_connect(tool_item,"clicked",G_CALLBACK(on_toolbar_grid_clicked),(gpointer)self);
#else
  image =
      gtk_image_new_from_icon_name ("buzztrax_menu_grid", GTK_ICON_SIZE_MENU);
  tool_item = gtk_tool_button_new (image, _("Grid"));
  gtk_tool_item_set_tooltip_text (tool_item, _("Show background grid"));
  gtk_toolbar_insert (GTK_TOOLBAR (self->priv->toolbar), tool_item, -1);
  g_signal_connect (tool_item, "clicked", G_CALLBACK (on_toolbar_grid_clicked),
      (gpointer) self);
#endif

#ifndef USE_COMPACT_UI
  gtk_toolbar_insert (GTK_TOOLBAR (self->priv->toolbar),
      gtk_separator_tool_item_new (), -1);
#endif

  // popup menu button
  image = gtk_image_new_from_filename ("popup-menu.png");
  tool_item = gtk_tool_button_new (image, _("Machine view menu"));
  gtk_tool_item_set_tooltip_text (tool_item,
      _("Menu actions for machine view below"));
  gtk_toolbar_insert (GTK_TOOLBAR (self->priv->toolbar), tool_item, -1);
  g_signal_connect (tool_item, "clicked", G_CALLBACK (on_toolbar_menu_clicked),
      (gpointer) self);

  gtk_box_pack_start (GTK_BOX (self), self->priv->toolbar, FALSE, FALSE, 0);

  // add canvas
  table = gtk_table_new (2, 2, FALSE);
  // adjustments are not configured, need window-size for it
  self->priv->vadjustment = gtk_adjustment_new (5.0, 0.0, 10.0, 1.0, 1.0, 0.0);
  g_signal_connect (self->priv->vadjustment, "value-changed",
      G_CALLBACK (on_vadjustment_changed), (gpointer) self);
  scrollbar = gtk_scrollbar_new (GTK_ORIENTATION_VERTICAL,
      self->priv->vadjustment);
  gtk_table_attach (GTK_TABLE (table), scrollbar,
      1, 2, 0, 1, 0, GTK_EXPAND | GTK_FILL, 0, 0);
  self->priv->hadjustment = gtk_adjustment_new (5.0, 0.0, 10.0, 1.0, 1.0, 0.0);
  g_signal_connect (self->priv->hadjustment, "value-changed",
      G_CALLBACK (on_hadjustment_changed), (gpointer) self);
  scrollbar = gtk_scrollbar_new (GTK_ORIENTATION_HORIZONTAL,
      self->priv->hadjustment);
  gtk_table_attach (GTK_TABLE (table), scrollbar,
      0, 1, 1, 2, GTK_EXPAND | GTK_FILL, 0, 0, 0);

  self->priv->canvas_widget = gtk_clutter_embed_new ();
  gtk_widget_set_name (GTK_WIDGET (self->priv->canvas_widget),
      "machine-and-wire-editor");
  gtk_widget_set_can_focus (GTK_WIDGET (self->priv->canvas_widget), TRUE);
  g_signal_connect (self->priv->canvas_widget, "size-allocate",
      G_CALLBACK (on_canvas_size_changed), (gpointer) self);
  self->priv->stage =
      gtk_clutter_embed_get_stage (GTK_CLUTTER_EMBED (self->priv->
          canvas_widget));
  GtkStyle *style = gtk_widget_get_style (self->priv->canvas_widget);
  GdkColor *c = &style->bg[GTK_STATE_NORMAL];
  ClutterColor stage_color = {
    c->red >> 8, c->green >> 8, c->blue >> 8, 0xff
  };
  clutter_stage_set_color (CLUTTER_STAGE (self->priv->stage), &stage_color);
  clutter_actor_set_size (self->priv->stage, MACHINE_VIEW_SIZE_X,
      MACHINE_VIEW_SIZE_Y);

  self->priv->canvas = clutter_actor_new ();
  clutter_actor_set_reactive (self->priv->canvas, TRUE);
  clutter_actor_set_size (self->priv->canvas, MACHINE_VIEW_SIZE_X,
      MACHINE_VIEW_SIZE_Y);
  clutter_actor_add_child (self->priv->stage, self->priv->canvas);

  // generate a grid one (pushing it to bottom)
  self->priv->grid = clutter_actor_new ();
  clutter_actor_set_size (self->priv->grid, MACHINE_VIEW_SIZE_X,
      MACHINE_VIEW_SIZE_Y);
  clutter_actor_set_anchor_point_from_gravity (self->priv->grid,
      CLUTTER_GRAVITY_CENTER);
  clutter_actor_set_position (self->priv->grid, MACHINE_VIEW_SIZE_X / 2.0,
      MACHINE_VIEW_SIZE_Y / 2.0);
  clutter_actor_add_child (self->priv->canvas, self->priv->grid);
  clutter_actor_set_child_below_sibling (self->priv->canvas, self->priv->grid,
      NULL);

  self->priv->grid_canvas = clutter_canvas_new ();
  clutter_canvas_set_size (CLUTTER_CANVAS (self->priv->grid_canvas),
      MACHINE_VIEW_SIZE_X, MACHINE_VIEW_SIZE_Y);
  clutter_actor_set_content (self->priv->grid, self->priv->grid_canvas);
  clutter_actor_set_content_scaling_filters (self->priv->grid,
      CLUTTER_SCALING_FILTER_TRILINEAR, CLUTTER_SCALING_FILTER_LINEAR);

  g_signal_connect (self->priv->grid_canvas, "draw", G_CALLBACK (on_grid_draw),
      (gpointer) self);
  /* invalidate the canvas, so that we can draw before the main loop starts */
  clutter_content_invalidate (self->priv->grid_canvas);

  gtk_table_attach (GTK_TABLE (table), self->priv->canvas_widget,
      0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_box_pack_start (GTK_BOX (self), table, TRUE, TRUE, 0);

  // create volume popup
  self->priv->vol_popup_adj =
      gtk_adjustment_new (100.0, 0.0, 400.0, 1.0, 10.0, 1.0);
  self->priv->vol_popup =
      BT_VOLUME_POPUP (bt_volume_popup_new (GTK_ADJUSTMENT (self->
              priv->vol_popup_adj)));
  g_signal_connect (self->priv->vol_popup_adj, "value-changed",
      G_CALLBACK (on_volume_popup_changed), (gpointer) self);

  // create panorama popup
  self->priv->pan_popup_adj =
      gtk_adjustment_new (0.0, -100.0, 100.0, 1.0, 10.0, 1.0);
  self->priv->pan_popup =
      BT_PANORAMA_POPUP (bt_panorama_popup_new (GTK_ADJUSTMENT (self->
              priv->pan_popup_adj)));
  g_signal_connect (self->priv->pan_popup_adj, "value-changed",
      G_CALLBACK (on_panorama_popup_changed), (gpointer) self);

  // register event handlers
  g_signal_connect (self->priv->app, "notify::song",
      G_CALLBACK (on_song_changed), (gpointer) self);
  g_signal_connect (self->priv->canvas, "button-press-event",
      G_CALLBACK (on_canvas_button_press), (gpointer) self);
  g_signal_connect (self->priv->canvas, "button-release-event",
      G_CALLBACK (on_canvas_button_release), (gpointer) self);
  g_signal_connect (self->priv->canvas, "motion-event",
      G_CALLBACK (on_canvas_motion), (gpointer) self);
  g_signal_connect (self->priv->canvas, "key-release-event",
      G_CALLBACK (on_canvas_key_release), (gpointer) self);

  // listen to page changes
  g_signal_connect ((gpointer) pages, "notify::page",
      G_CALLBACK (on_page_switched), (gpointer) self);
  g_signal_connect ((gpointer) self, "map",
      G_CALLBACK (on_page_mapped), (gpointer) self);

  // let settings control toolbar style
  on_toolbar_style_changed (settings, NULL, (gpointer) self);
  g_signal_connect (settings, "notify::toolbar-style",
      G_CALLBACK (on_toolbar_style_changed), (gpointer) self);
  g_object_unref (settings);
  GST_DEBUG ("  done");
}

//-- constructor methods

/**
 * bt_main_page_machines_new:
 * @pages: the page collection
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtMainPageMachines *
bt_main_page_machines_new (const BtMainPages * pages)
{
  BtMainPageMachines *self;
  self =
      BT_MAIN_PAGE_MACHINES (g_object_new (BT_TYPE_MAIN_PAGE_MACHINES, NULL));
  bt_main_page_machines_init_ui (self, pages);
  return (self);
}

//-- methods

static void
place_popup (const BtMainPageMachines * self, GtkWindow * popup, gint xpos,
    gint ypos)
{
  BtMainWindow *main_window;
  GtkAllocation alloc;

  g_object_get (self->priv->app, "main-window", &main_window, NULL);
  gtk_window_set_transient_for (popup, GTK_WINDOW (main_window));
  gtk_window_get_position (GTK_WINDOW (main_window), &alloc.x, &alloc.y);
  xpos += alloc.x;
  ypos += alloc.y;
  g_object_unref (main_window);

  gtk_widget_get_allocation (self->priv->canvas_widget, &alloc);
  xpos += alloc.x;
  ypos += alloc.y;

  GST_INFO ("showing popup at %d,%d", xpos, ypos);

  /* show directly at mouse-pos */
  /* TODO(ensonic): move it so that the knob is under the mouse */
  // need to defer to map() to get allocation
  //gint pos = (allocation.height * ((gain  / 4.0) * 100.0)) / 100.0;
  //gtk_window_move (popup, xpos, ypos-mid);
  gtk_window_set_gravity (popup, GDK_GRAVITY_CENTER);   // is ignored :/
  gtk_window_move (popup, xpos, ypos);
}

/**
 * bt_main_page_machines_wire_volume_popup:
 * @self: the machines page
 * @wire: the wire to popup the volume control for
 * @xpos: the x-position for the popup
 * @ypos: the y-position for the popup
 *
 * Activates the volume-popup for the given wire.
 *
 * Returns: %TRUE for succes.
 */
gboolean
bt_main_page_machines_wire_volume_popup (const BtMainPageMachines * self,
    BtWire * wire, gint xpos, gint ypos)
{
  gdouble gain;

  g_object_try_unref (self->priv->wire_gain);
  g_object_get (wire, "gain", &self->priv->wire_gain, NULL);
  /* set initial value */
  g_object_get (self->priv->wire_gain, "volume", &gain, NULL);
  // FIXME(ensonic): workaround for https://bugzilla.gnome.org/show_bug.cgi?id=667598
  gtk_adjustment_set_value (GTK_ADJUSTMENT (self->priv->vol_popup_adj),
      (4.0 - gain) * 100.0);

  place_popup (self, GTK_WINDOW (self->priv->vol_popup), xpos, ypos);
  bt_volume_popup_show (self->priv->vol_popup);
  return (TRUE);
}

/**
 * bt_main_page_machines_wire_panorama_popup:
 * @self: the machines page
 * @wire: the wire to popup the panorama control for
 * @xpos: the x-position for the popup
 * @ypos: the y-position for the popup
 *
 * Activates the panorama-popup for the given wire.
 *
 * Returns: %TRUE for succes.
 */
gboolean
bt_main_page_machines_wire_panorama_popup (const BtMainPageMachines * self,
    BtWire * wire, gint xpos, gint ypos)
{
  g_object_try_unref (self->priv->wire_pan);
  g_object_get (wire, "pan", &self->priv->wire_pan, NULL);
  if (self->priv->wire_pan) {
    gfloat pan;
    /* set initial value */
    g_object_get (self->priv->wire_pan, "panorama", &pan, NULL);
    gtk_adjustment_set_value (GTK_ADJUSTMENT (self->priv->pan_popup_adj),
        (gdouble) pan * 100.0);

    place_popup (self, GTK_WINDOW (self->priv->pan_popup), xpos, ypos);
    bt_panorama_popup_show (self->priv->pan_popup);
  }
  return (TRUE);
}

static gboolean
bt_main_page_machines_add_machine (const BtMainPageMachines * self,
    guint type, const gchar * id, const gchar * plugin_name)
{
  BtSong *song;
  BtMachine *machine = NULL;
  gchar *uid;
  GError *err = NULL;
  g_object_get (self->priv->app, "song", &song, NULL);
  uid = bt_setup_get_unique_machine_id (self->priv->setup, id);
  // try with 1 voice, if monophonic, voices will be reset to 0 in
  // bt_machine_init_voice_params()
  switch (type) {
    case 0:
      machine = BT_MACHINE (bt_source_machine_new (song, uid, plugin_name,
              /*voices= */ 1, &err));
      break;
    case 1:
      machine = BT_MACHINE (bt_processor_machine_new (song, uid, plugin_name,
              /*voices= */ 1, &err));
      break;
  }
  if (err == NULL) {
    BtMachineCanvasItem *mi;
    gchar *undo_str, *redo_str;
    GST_INFO_OBJECT (machine, "created machine %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (machine));
    bt_change_log_start_group (self->priv->change_log);
    if ((mi = g_hash_table_lookup (self->priv->machines, machine))) {
      machine_item_moved (self, mi);
    }

    undo_str = g_strdup_printf ("rem_machine \"%s\"", uid);
    redo_str =
        g_strdup_printf ("add_machine %u,\"%s\",\"%s\"", type, uid,
        plugin_name);
    bt_change_log_add (self->priv->change_log, BT_CHANGE_LOGGER (self),
        undo_str, redo_str);
    bt_change_log_end_group (self->priv->change_log);
  } else {
    GST_WARNING ("Can't create machine %s: %s", plugin_name, err->message);
    g_error_free (err);
    gst_object_unref (machine);
  }
  g_free (uid);
  g_object_unref (song);
  return (err == NULL);
}

/**
 * bt_main_page_machines_add_source_machine:
 * @self: the machines page
 * @id: the id for the new machine
 * @plugin_name: the plugin-name for the new machine
 *
 * Add a new machine to the machine-page.
 */
gboolean
bt_main_page_machines_add_source_machine (const BtMainPageMachines * self,
    const gchar * id, const gchar * plugin_name)
{
  return (bt_main_page_machines_add_machine (self, 0, id, plugin_name));
}

/**
 * bt_main_page_machines_add_processor_machine:
 * @self: the machines page
 * @id: the id for the new machine
 * @plugin_name: the plugin-name for the new machine
 *
 * Add a new machine to the machine-page.
 */
gboolean
bt_main_page_machines_add_processor_machine (const BtMainPageMachines *
    self, const gchar * id, const gchar * plugin_name)
{
  return (bt_main_page_machines_add_machine (self, 1, id, plugin_name));
}

/**
 * bt_main_page_machines_delete_machine:
 * @self: the machines page
 * @machine: the machine to remove
 *
 * Remove a machine from the machine-page.
 */
void
bt_main_page_machines_delete_machine (const BtMainPageMachines * self,
    BtMachine * machine)
{
  BtMainPageSequence *sequence_page;
  GHashTable *properties;
  gchar *undo_str, *redo_str;
  gchar *mid, *pname, *prop;
  guint type = 0;
  BtMachineState machine_state;
  gulong voices;
  gulong pattern_removed_id;
  GST_INFO ("before removing machine : %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine));
  bt_change_log_start_group (self->priv->change_log);
  /* by doing undo/redo from setup:machine-removed it happens completely
   * unconstrained, we need to enforce a certain order. Right now the log will
   * reverse the actions. Thus we need to write out the object we remove and
   * then, within, the things that get removed by that.
   * Thus when removing X -
   * 1) we need to log it directly and not in on_x_removed
   * 2) trigger on_x_removed
   * Still this won't let us control the order or serialization of the children
   *
   * We can't control signal emission. Like emitting the signal with details or
   * similar approaches won't fix it nicely either.
   * Somehow the handlers would need to describe their dependencies and then
   * defer their execution. Which they can't as they don't know the context they
   * are called from. A pre-delete signal would not solve it either.
   *
   * BtMainPageMachines:bt_main_page_machines_delete_machine -> BtSetup:machine-removed { 
   *   BtMainPagePatterns:on_machine_removed -> BtMachine::pattern-removed { 
   *     BtMainPageSequence:on_pattern_removed
   *   }
   *   BtMainPageSequence:on_machine_removed
   *   BtWireCanvasItem:on_machine_removed -> BtSetup::wire-removed
   * }
   * Here we have the problem that we have two callbacks changing the sequence
   * data. When removing a machine, we just want handler for machine-removed to
   * do undo/redo, not e.g. pattern-removed.
   *
   * Should we store the signal in change log and check in signal handler if
   * we should be logging?
   *
   * when handling the ui interaction:
   * signal_id=g_signal_lookup("machine-removed",setup)
   * bt_change_log_set_hint(change_log,signal_id);
   *
   * inside the signal handler:
   * hint = g_signal_get_invocation_hint(object);
   * if (bt_change_log_is_hint(change_log,hint->signal_id) {
   *   // do undo/redo serialization
   * }
   *
   * For now we block conflicting handlers.
   */
  GST_INFO ("logging redo the machine : %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine));
  if (BT_IS_SOURCE_MACHINE (machine))
    type = 0;
  else if (BT_IS_PROCESSOR_MACHINE (machine))
    type = 1;
  g_object_get (machine, "id", &mid, "plugin-name", &pname, "properties",
      &properties, "state", &machine_state, "voices", &voices, NULL);
  bt_change_log_start_group (self->priv->change_log);
  undo_str = g_strdup_printf ("add_machine %u,\"%s\",\"%s\"", type, mid, pname);
  redo_str = g_strdup_printf ("rem_machine \"%s\"", mid);
  bt_change_log_add (self->priv->change_log, BT_CHANGE_LOGGER (self),
      undo_str, redo_str);
  undo_str =
      g_strdup_printf ("set_machine_property \"%s\",\"voices\",\"%lu\"", mid,
      voices);
  bt_change_log_add (self->priv->change_log, BT_CHANGE_LOGGER (self),
      undo_str, g_strdup (undo_str));
  prop = (gchar *) g_hash_table_lookup (properties, "properties-shown");
  if (prop && *prop == '1') {
    undo_str =
        g_strdup_printf
        ("set_machine_property \"%s\",\"properties-shown\",\"1\"", mid);
    bt_change_log_add (self->priv->change_log, BT_CHANGE_LOGGER (self),
        undo_str, g_strdup (undo_str));
  }

  if (machine_state != BT_MACHINE_STATE_NORMAL) {
    GEnumClass *enum_class = g_type_class_peek_static (BT_TYPE_MACHINE_STATE);
    GEnumValue *enum_value = g_enum_get_value (enum_class, machine_state);
    undo_str =
        g_strdup_printf ("set_machine_property \"%s\",\"state\",\"%s\"", mid,
        enum_value->value_nick);
    bt_change_log_add (self->priv->change_log, BT_CHANGE_LOGGER (self),
        undo_str, g_strdup (undo_str));
  }
  // TODO(ensonic): machine parameters
  g_free (mid);
  g_free (pname);
  // remove patterns for machine, trigger setup::machine-removed
  GST_INFO ("removing the machine : %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine));
  // block machine:pattern-removed signal emission for sequence page to not clobber the sequence
  // in theory we we don't need to block it, as we are removing the machine anyway and thus disconnecting
  // but if we disconnect here, disconnecting them on the sequence page would fail
  pattern_removed_id = g_signal_lookup ("pattern-removed", BT_TYPE_MACHINE);
  bt_child_proxy_get (self->priv->app, "main-window::pages::sequence-page",
      &sequence_page, NULL);
  g_signal_handlers_block_matched (machine,
      G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_DATA, pattern_removed_id, 0, NULL,
      NULL, (gpointer) sequence_page);
  bt_setup_remove_machine (self->priv->setup, machine);
  g_object_unref (sequence_page);
  bt_change_log_end_group (self->priv->change_log);
  bt_change_log_end_group (self->priv->change_log);
}

/**
 * bt_main_page_machines_delete_wire:
 * @self: the machines page
 * @wire: the wire to remove
 *
 * Remove a wire from the machine-page (unlink the machines).
 */
void
bt_main_page_machines_delete_wire (const BtMainPageMachines * self,
    BtWire * wire)
{
  GST_INFO ("now removing wire: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (wire));
  bt_change_log_start_group (self->priv->change_log);
  bt_setup_remove_wire (self->priv->setup, wire);
  bt_change_log_end_group (self->priv->change_log);
}

/**
 * bt_main_page_machines_rename_machine:
 * @self: the machines page
 * @machine: the machine to renam
 *
 * Run the machine #BtMachineRenameDialog.
 */
void
bt_main_page_machines_rename_machine (const BtMainPageMachines * self,
    BtMachine * machine)
{
  GtkWidget *dialog = GTK_WIDGET (bt_machine_rename_dialog_new (machine));
  bt_edit_application_attach_child_window (self->priv->app,
      GTK_WINDOW (dialog));
  gtk_widget_show_all (dialog);
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
    gchar *new_name;
    gchar *undo_str, *redo_str;
    gchar *mid;
    g_object_get (machine, "id", &mid, NULL);
    g_object_get (dialog, "name", &new_name, NULL);
    if (strcmp (mid, new_name)) {
      bt_change_log_start_group (self->priv->change_log);
      undo_str =
          g_strdup_printf ("set_machine_property \"%s\",\"name\",\"%s\"",
          new_name, mid);
      redo_str =
          g_strdup_printf ("set_machine_property \"%s\",\"name\",\"%s\"", mid,
          new_name);
      bt_change_log_add (self->priv->change_log, BT_CHANGE_LOGGER (self),
          undo_str, redo_str);
      bt_change_log_end_group (self->priv->change_log);
    }
    bt_machine_rename_dialog_apply (BT_MACHINE_RENAME_DIALOG (dialog));
    g_free (mid);
    g_free (new_name);
  }
  gtk_widget_destroy (dialog);
}

//-- change logger interface

static gboolean
bt_main_page_machines_change_logger_change (const BtChangeLogger * owner,
    const gchar * data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (owner);
  BtSong *song;
  gboolean res = FALSE;
  BtMachine *machine, *smachine, *dmachine;
  BtWire *wire;
  GMatchInfo *match_info;
  gchar *s;
  GST_INFO ("undo/redo: [%s]", data);
  // parse data and apply action
  switch (bt_change_logger_match_method (change_logger_methods, data,
          &match_info)) {
    case METHOD_ADD_MACHINE:{
      GError *err = NULL;
      gchar *mid, *pname;
      guint type;
      s = g_match_info_fetch (match_info, 1);
      type = atoi (s);
      g_free (s);
      mid = g_match_info_fetch (match_info, 2);
      pname = g_match_info_fetch (match_info, 3);
      g_match_info_free (match_info);
      GST_DEBUG ("-> [%d|%s|%s]", type, mid, pname);
      g_object_get (self->priv->app, "song", &song, NULL);
      switch (type) {
        case 0:
          machine = BT_MACHINE (bt_source_machine_new (song, mid, pname,        /*voices= */
                  1, &err));
          break;
        case 1:
          machine = BT_MACHINE (bt_processor_machine_new (song, mid, pname,
                  /*voices= */ 1, &err));
          break;
        default:
          machine = NULL;
          GST_WARNING ("unhandled machine type: %d", type);
          break;
      }
      if (machine) {
        if (err) {
          GST_WARNING ("failed to create machine: %s : %s : %s", mid, pname,
              err->message);
          g_error_free (err);
          err = NULL;
          gst_object_unref (machine);
        } else {
          res = TRUE;
        }
      }
      g_object_unref (song);
      g_free (mid);
      g_free (pname);
      break;
    }
    case METHOD_REM_MACHINE:{
      gchar *mid;
      mid = g_match_info_fetch (match_info, 1);
      g_match_info_free (match_info);
      GST_DEBUG ("-> [%s]", mid);
      if ((machine = bt_setup_get_machine_by_id (self->priv->setup, mid))) {
        bt_setup_remove_machine (self->priv->setup, machine);
        g_object_unref (machine);
        res = TRUE;
      }
      g_free (mid);
      break;
    }
    case METHOD_SET_MACHINE_PROPERTY:{
      gchar *mid, *key, *val;
      GHashTable *properties;
      mid = g_match_info_fetch (match_info, 1);
      key = g_match_info_fetch (match_info, 2);
      val = g_match_info_fetch (match_info, 3);
      g_match_info_free (match_info);
      GST_DEBUG ("-> [%s|%s|%s]", mid, key, val);
      if ((machine = bt_setup_get_machine_by_id (self->priv->setup, mid))) {
        BtMachineCanvasItem *item;
        gfloat pos;
        gboolean is_prop = FALSE;
        g_object_get (machine, "properties", &properties, NULL);
        res = TRUE;
        if (!strcmp (key, "xpos")) {
          if ((item = g_hash_table_lookup (self->priv->machines, machine))) {
            pos = (MACHINE_VIEW_SIZE_X / 2.0) +
                (MACHINE_VIEW_SIZE_X * g_ascii_strtod (val, NULL));
            GST_DEBUG ("xpos : %s -> %f", val, pos);
            g_object_set (item, "x", pos, NULL);
            g_signal_emit_by_name (item, "position-changed", 0, CLUTTER_MOTION);
          }
          is_prop = TRUE;
        } else if (!strcmp (key, "ypos")) {
          if ((item = g_hash_table_lookup (self->priv->machines, machine))) {
            pos = (MACHINE_VIEW_SIZE_Y / 2.0) +
                (MACHINE_VIEW_SIZE_Y * g_ascii_strtod (val, NULL));
            GST_DEBUG ("ypos : %s -> %f", val, pos);
            g_object_set (item, "y", pos, NULL);
            g_signal_emit_by_name (item, "position-changed", 0, CLUTTER_MOTION);
          }
          is_prop = TRUE;
        } else if (!strcmp (key, "properties-shown")) {
          if (*val == '1') {
            bt_machine_show_properties_dialog (machine);
          }
          is_prop = TRUE;
        } else if (!strcmp (key, "state")) {
          GEnumClass *enum_class =
              g_type_class_peek_static (BT_TYPE_MACHINE_STATE);
          GEnumValue *enum_value = g_enum_get_value_by_nick (enum_class, val);
          if (enum_value) {
            g_object_set (machine, "state", enum_value->value, NULL);
          }
        } else if (!strcmp (key, "voices")) {
          g_object_set (machine, "voices", atol (val), NULL);
        } else if (!strcmp (key, "name")) {
          g_object_set (machine, "id", val, NULL);
        } else {
          GST_WARNING ("unhandled property '%s'", key);
          res = FALSE;
        }
        if (is_prop && properties) {
          // take ownership of the strings
          g_hash_table_replace (properties, key, val);
          key = val = NULL;
        }
        g_object_unref (machine);
      }
      g_free (mid);
      g_free (key);
      g_free (val);
      break;
    }
    case METHOD_ADD_WIRE:{
      gchar *smid, *dmid;
      smid = g_match_info_fetch (match_info, 1);
      dmid = g_match_info_fetch (match_info, 2);
      g_match_info_free (match_info);
      GST_DEBUG ("-> [%s|%s]", smid, dmid);
      g_object_get (self->priv->app, "song", &song, NULL);
      smachine = bt_setup_get_machine_by_id (self->priv->setup, smid);
      dmachine = bt_setup_get_machine_by_id (self->priv->setup, dmid);
      if (smachine && dmachine) {
        BtMachineCanvasItem *src_machine_item, *dst_machine_item;
        wire = bt_wire_new (song, smachine, dmachine, NULL);
        // draw wire
        src_machine_item = g_hash_table_lookup (self->priv->machines, smachine);
        dst_machine_item = g_hash_table_lookup (self->priv->machines, dmachine);
        wire_item_new (self, wire, src_machine_item, dst_machine_item);
        clutter_actor_set_child_below_sibling (self->priv->canvas,
            self->priv->grid, NULL);
        res = TRUE;
      }
      g_object_try_unref (smachine);
      g_object_try_unref (dmachine);
      g_object_unref (song);
      g_free (smid);
      g_free (dmid);
      break;
    }
    case METHOD_REM_WIRE:{
      gchar *smid, *dmid;
      smid = g_match_info_fetch (match_info, 1);
      dmid = g_match_info_fetch (match_info, 2);
      g_match_info_free (match_info);
      GST_DEBUG ("-> [%s|%s]", smid, dmid);
      smachine = bt_setup_get_machine_by_id (self->priv->setup, smid);
      dmachine = bt_setup_get_machine_by_id (self->priv->setup, dmid);
      if (smachine && dmachine) {
        if ((wire =
                bt_setup_get_wire_by_machines (self->priv->setup, smachine,
                    dmachine))) {
          bt_setup_remove_wire (self->priv->setup, wire);
          g_object_unref (wire);
          res = TRUE;
        }
      }
      g_object_try_unref (smachine);
      g_object_try_unref (dmachine);
      g_free (smid);
      g_free (dmid);
      break;
    }
    case METHOD_SET_WIRE_PROPERTY:{
      gchar *smid, *dmid, *key, *val;
      GHashTable *properties;
      smid = g_match_info_fetch (match_info, 1);
      dmid = g_match_info_fetch (match_info, 2);
      key = g_match_info_fetch (match_info, 3);
      val = g_match_info_fetch (match_info, 4);
      g_match_info_free (match_info);
      GST_DEBUG ("-> [%s|%s|%s|%s]", smid, dmid, key, val);
      smachine = bt_setup_get_machine_by_id (self->priv->setup, smid);
      dmachine = bt_setup_get_machine_by_id (self->priv->setup, dmid);
      if (smachine && dmachine) {
        if ((wire =
                bt_setup_get_wire_by_machines (self->priv->setup, smachine,
                    dmachine))) {
          gboolean is_prop = FALSE;
          g_object_get (wire, "properties", &properties, NULL);
          if (!strcmp (key, "analyzer-shown")) {
            if (*val == '1') {
              bt_wire_show_analyzer_dialog (wire);
            }
            is_prop = TRUE;
          } else {
            GST_WARNING ("unhandled property '%s'", key);
            res = FALSE;
          }
          if (is_prop && properties) {
            // take ownership of the strings
            g_hash_table_replace (properties, key, val);
            key = val = NULL;
          }
          g_object_unref (wire);
        }
      }
      g_object_try_unref (smachine);
      g_object_try_unref (dmachine);
      g_free (smid);
      g_free (dmid);
      g_free (key);
      g_free (val);
      break;
    }
      /* TODO(ensonic):
         - machine parameters, wire parameters (volume/panorama)
         - only write them out on release to not blast to much crap into the log
         - open/close machine/wire windows
         - we do it on remove right, but we don't know about intermediate changes
         as thats still done on canvas-item classes
         - the canvas-items now expose their dialogs as a read-only (widget)
         property, we can listen to the notify
       */
    default:
      GST_WARNING ("unhandled undo/redo method: [%s]", data);
  }

  GST_INFO ("undo/redo: %s : [%s]", (res ? "okay" : "failed"), data);
  return res;
}

static void
bt_main_page_machines_change_logger_interface_init (gpointer const g_iface,
    gconstpointer const iface_data)
{
  BtChangeLoggerInterface *const iface = g_iface;
  iface->change = bt_main_page_machines_change_logger_change;
}

//-- wrapper

//-- class internals

static gboolean
bt_main_page_machines_focus (GtkWidget * widget, GtkDirectionType direction)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (widget);
  GST_DEBUG ("focusing default widget");
  gtk_widget_grab_focus_savely (GTK_WIDGET (self->priv->canvas_widget));
  /* update status bar */
  // FIXME: window might be NULL
  // in this case we can't continue the lookup and take the property off the
  // va_arg stack
  bt_child_proxy_set (self->priv->app, "main-window::statusbar::status",
      _("Add new machines from right click context menu. "
          "Connect machines with shift+drag from source to target."), NULL);
  return FALSE;
}

static void
bt_main_page_machines_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (object);
  return_if_disposed ();
  switch (property_id) {
    case MAIN_PAGE_MACHINES_CANVAS:
      g_value_set_object (value, self->priv->canvas);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_main_page_machines_dispose (GObject * object)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (object);
  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;
  GST_DEBUG ("!!!! self=%p", self);
  GST_DEBUG ("  unrefing popups");
  g_object_try_unref (self->priv->wire_gain);
  if (self->priv->vol_popup) {
    bt_volume_popup_hide (self->priv->vol_popup);
    gtk_widget_destroy (GTK_WIDGET (self->priv->vol_popup));
  }
  g_object_try_unref (self->priv->wire_pan);
  if (self->priv->pan_popup) {
    bt_panorama_popup_hide (self->priv->pan_popup);
    gtk_widget_destroy (GTK_WIDGET (self->priv->pan_popup));
  }
  g_signal_handlers_disconnect_by_func (self->priv->app, on_song_changed, self);
  g_object_try_unref (self->priv->setup);
  g_object_unref (self->priv->change_log);
  g_object_unref (self->priv->app);
  gtk_widget_destroy (GTK_WIDGET (self->priv->context_menu));
  g_object_unref (self->priv->context_menu);
  gtk_widget_destroy (GTK_WIDGET (self->priv->grid_density_menu));
  g_object_unref (self->priv->grid_density_menu);
  g_object_unref (self->priv->drag_cursor);
  GST_DEBUG ("  chaining up");
  G_OBJECT_CLASS (bt_main_page_machines_parent_class)->dispose (object);
}

static void
bt_main_page_machines_finalize (GObject * object)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (object);
  g_hash_table_destroy (self->priv->machines);
  g_hash_table_destroy (self->priv->wires);
  G_OBJECT_CLASS (bt_main_page_machines_parent_class)->finalize (object);
}

static void
bt_main_page_machines_init (BtMainPageMachines * self)
{
  self->priv =
      G_TYPE_INSTANCE_GET_PRIVATE (self, BT_TYPE_MAIN_PAGE_MACHINES,
      BtMainPageMachinesPrivate);
  GST_DEBUG ("!!!! self=%p", self);
  self->priv->app = bt_edit_application_new ();
  self->priv->zoom = 1.0;
  self->priv->grid_density = 1;
  self->priv->machines = g_hash_table_new (NULL, NULL);
  self->priv->wires = g_hash_table_new (NULL, NULL);
  // the cursor for dragging
  self->priv->drag_cursor = gdk_cursor_new (GDK_FLEUR);
  // initial size
  self->priv->view_w = MACHINE_VIEW_SIZE_X;
  self->priv->view_h = MACHINE_VIEW_SIZE_Y;
  // the undo/redo changelogger
  self->priv->change_log = bt_change_log_new ();
  bt_change_log_register (self->priv->change_log, BT_CHANGE_LOGGER (self));

  gtk_orientable_set_orientation (GTK_ORIENTABLE (self),
      GTK_ORIENTATION_VERTICAL);
}

static void
bt_main_page_machines_class_init (BtMainPageMachinesClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *gtkwidget_class = GTK_WIDGET_CLASS (klass);
  g_type_class_add_private (klass, sizeof (BtMainPageMachinesPrivate));
  gobject_class->get_property = bt_main_page_machines_get_property;
  gobject_class->dispose = bt_main_page_machines_dispose;
  gobject_class->finalize = bt_main_page_machines_finalize;
  gtkwidget_class->focus = bt_main_page_machines_focus;
  g_object_class_install_property (gobject_class,
      MAIN_PAGE_MACHINES_CANVAS, g_param_spec_object ("canvas",
          "canvas prop", "Get the machine canvas", CLUTTER_TYPE_ACTOR,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}
