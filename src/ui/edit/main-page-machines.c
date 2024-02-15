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
 * 1.) on the toolbar we could have a button that toggles between "move" and
 *     "link". In "move" mode one can move machines with the mouse and in "link"
 *     mode one can link. Would need to have a keyboard shortcut for toggling.
 * 2.) click-zones on the machine-icons. Link the machine when click+drag the
 *     title bar. Make the 'leds' clickable and move the machines outside of
 *     title and led. Eventualy change the mouse-cursor when hovering over the
 *     zones.
 * Option '2' looks nice and would also help on touch-screens.
 */
/* TODO(ensonic): add a machine list to the right side
 * - this would be like the menu, but as a tree view and therefore more
 *   discoverable
 * - one can drag machines out of the list
 * - we could even show presets as children of machines
 *   - dragging the preset would create the machine with the preset selected
 * - we could also allow to preview machines and/or presets
 * - write a machine-model.{c,h}
 *   - a GtkTreeModel provides row-{inserted,deleted,changed} signals, which we
 *     can use to update the machine-menu
 *   - this lets us extract the code that is in bt_machine_menu_init_submenu()
 *     and thus simply the machine-menu.c (~180 lines)
 */

#define BT_EDIT
#define BT_MAIN_PAGE_MACHINES_C

#include "bt-edit.h"
#include "main-page-sequence.h"

#include <math.h>

enum
{
  MAIN_PAGE_MACHINES_CANVAS = 1,
  MAIN_PAGE_MACHINES_GRID_DENSITY
};

// machine view area
// TODO(ensonic): should we check screen dpi?
#define MACHINE_VIEW_W 1000.0
#define MACHINE_VIEW_H 750.0
// zoom range
#define ZOOM_MIN 0.45
#define ZOOM_MAX 2.5

struct _BtMainPageMachines
{
  GtkWidget parent;
  
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* the toolbar widget */
  GtkWidget *toolbar;

  /* the setup of machines and wires */
  BtSetup *setup;

  /* the zoomration in pixels/per unit */
  gdouble zoom;
  /* zomm in/out widgets */
  GtkWidget *zoom_in, *zoom_out;

  /* canvas context_menu */
  GMenu *context_menu;

  /* grid density */
  BtMainPageMachinesGridDensity grid_density;

  /* we probably need a list of canvas items that we have drawn, so that we can
   * easily clear them later
   */
  GHashTable *machines;         // each entry points to BtMachineCanvasItem
  GHashTable *wires;            // each entry points to BtWireCanvasItem

  /* interaction state */
  gboolean connecting, moved, dragging;

  /* used when interactivly adding a new wire */
  GtkDrawingArea *new_wire;
  BtMachineCanvasItem *new_wire_src, *new_wire_dst;
  GdkRGBA wire_good_color, wire_bad_color;

  /* cached setup properties */
  GHashTable *properties;

  /* mouse coodinates on context menu invokation (used for placing new machines) */
  gfloat mouse_x, mouse_y;
  /* relative machine coordinates before/after draging (needed for undo) */
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

  GtkFixed *canvas;
  GtkDrawingArea *grid_canvas;
  GtkMenuButton *machine_menu_button;
  GtkScrolledWindow *scrolled_window;

  BtMachineMenu *machine_menu;
};

//-- the class

static void bt_main_page_machines_change_logger_interface_init (gpointer const
    g_iface, gconstpointer const iface_data);

G_DEFINE_TYPE_WITH_CODE (BtMainPageMachines, bt_main_page_machines, GTK_TYPE_WIDGET,
    G_IMPLEMENT_INTERFACE (BT_TYPE_CHANGE_LOGGER,
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
static gboolean bt_main_page_machines_check_wire (BtMainPageMachines *
    self);

static void on_machine_item_start_connect (BtMachineCanvasItem * machine_item,
    gpointer user_data);

static void on_machine_item_position_changed (BtMachineCanvasItem *
    machine_item, guint ev_type, gpointer user_data);

//-- enums

G_DEFINE_ENUM_TYPE (BtMainPageMachinesGridDensity,
    bt_main_page_machines_grid_density,
    G_DEFINE_ENUM_VALUE (BT_MAIN_PAGE_MACHINES_GRID_DENSITY_OFF, "off"),
    G_DEFINE_ENUM_VALUE (BT_MAIN_PAGE_MACHINES_GRID_DENSITY_LOW, "low"),
    G_DEFINE_ENUM_VALUE (BT_MAIN_PAGE_MACHINES_GRID_DENSITY_MEDIUM, "medium"),
    G_DEFINE_ENUM_VALUE (BT_MAIN_PAGE_MACHINES_GRID_DENSITY_HIGH, "high")
);


//-- converters for coordinate systems

static void
widget_to_canvas_pos (BtMainPageMachines * self, gfloat wx, gfloat wy,
    gfloat * cx, gfloat * cy)
{
#if 0 /// GTK4 no longer relevant? Use GTK coord translate functions?
  gdouble ox = gtk_adjustment_get_value (self->hadjustment);
  gdouble oy = gtk_adjustment_get_value (self->vadjustment);
  *cx = (wx + ox) / self->zoom;
  *cy = (wy + oy) / self->zoom;
#else
  *cx = wx;
  *cy = wy;
#endif
}

static void
canvas_to_widget_pos (BtMainPageMachines * self, gfloat cx, gfloat cy,
    gfloat * wx, gfloat * wy)
{
#if 0 /// GTK4 no longer relevant? Use GTK coord translate functions?
  gdouble ox = gtk_adjustment_get_value (self->hadjustment);
  gdouble oy = gtk_adjustment_get_value (self->vadjustment);
  *wx = (cx * self->zoom) - ox;
  *wy = (cy * self->zoom) - oy;
#else
  *wx = cx;
  *wy = cy;
#endif
}

//-- drawing handlers

static void
on_wire_draw (GtkDrawingArea* drawing_area, cairo_t* cr, int width, int height,
    gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);
  gfloat mx, my, x1, x2, y1, y2;

  mx = 0; my = 0; /// GTK4 clutter_actor_get_position ((ClutterActor *) self->new_wire_src, &mx, &my);
  if (mx < self->mouse_x) {
    x1 = 0.0;
    x2 = width;
  } else {
    x1 = width;
    x2 = 0.0;
  }
  if (my < self->mouse_y) {
    y1 = 0.0;
    y2 = height;
  } else {
    y1 = height;
    y2 = 0.0;
  }

  /* clear the contents of the canvas, to not paint over the previous frame */
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

  cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_width (cr, 1.0);
  if (self->new_wire_dst && bt_main_page_machines_check_wire (self)) {
    gdk_cairo_set_source_rgba (cr, &self->wire_good_color);
  } else {
    gdk_cairo_set_source_rgba (cr, &self->wire_bad_color);
  }

  cairo_move_to (cr, x1, y1);
  cairo_line_to (cr, x2, y2);
  cairo_stroke (cr);
}

static void
on_grid_draw (GtkDrawingArea* drawing_area, cairo_t* cr, int width, int height,
    gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);
  gdouble i, step, c, r1, r2, rx, ry;
  gfloat gray[4] = { 0.5, 0.85, 0.7, 0.85 };

  GST_INFO ("redrawing grid: density=%d  canvas: %4d,%4d",
      self->grid_density, width, height);

  /* clear the contents of the canvas, to not paint over the previous frame */
  gtk_render_background (gtk_widget_get_style_context (
      GTK_WIDGET (self->canvas)), cr, 0, 0, width, height);

  if (!self->grid_density)
    return;

  /* center the grid */
  rx = width / 2.0;
  ry = height / 2.0;
  GST_DEBUG ("size: %d,%d, center: %4.1f,%4.1f", width, height, rx, ry);
  cairo_translate (cr, rx, ry);

  cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_width (cr, 1.0 / self->zoom);     // make relative to zoom

#if 0
  {                             // DEBUG
    cairo_set_source_rgba (cr, 1.0, 0.0, 0.0, 1.0);
    cairo_move_to (cr, -rx, -ry);
    cairo_line_to (cr, rx, -ry);
    cairo_line_to (cr, rx, ry);
    cairo_line_to (cr, -rx, ry);
    cairo_line_to (cr, -rx, -ry);
    cairo_stroke (cr);

    cairo_move_to (cr, -rx, -ry);
    cairo_line_to (cr, rx, ry);
    cairo_stroke (cr);
    cairo_move_to (cr, -rx, ry);
    cairo_line_to (cr, rx, -ry);
    cairo_stroke (cr);
  }                             // DEBUG
#endif

  r1 = MAX (rx, ry);
  r2 = MAX (MACHINE_VIEW_W, MACHINE_VIEW_H);
  step = r2 / (gdouble) (1 << self->grid_density);
  for (i = 0.0; i <= r1; i += step) {
    c = gray[((gint) fabs ((i / r2) * 8.0)) & 0x3];
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
}

//-- linking signal handler & helper

static void
update_connect (BtMainPageMachines * self)
{
  gfloat mm_x, mm_y, w, h, x, y;

  graphene_rect_t rect;
  if (gtk_widget_compute_bounds (GTK_WIDGET (self->new_wire_src), GTK_WIDGET (self), &rect)) {
    mm_x = self->mouse_x;
    mm_y = self->mouse_y;
    w = 1.0 + fabs(mm_x - rect.origin.x);
    h = 1.0 + fabs(mm_y - rect.origin.y);
    x = MIN(mm_x, rect.origin.x);
    y = MIN(mm_y, rect.origin.y);

    gtk_drawing_area_set_content_width (self->new_wire, w);
    gtk_drawing_area_set_content_height (self->new_wire, h);
    
    gtk_fixed_move (self->canvas, GTK_WIDGET (self->new_wire), x, y);

    self->moved = TRUE;
  } else {
    GST_WARNING ("Error trying to get new wire widget bounds");
  }
}

static void
start_connect (BtMainPageMachines * self)
{
  // handle drawing a new wire
  g_assert (!self->new_wire);
  self->new_wire = GTK_DRAWING_AREA (gtk_drawing_area_new ());
  gtk_drawing_area_set_draw_func (self->new_wire, on_wire_draw, self, NULL);
  gtk_widget_set_parent (GTK_WIDGET (self->new_wire), GTK_WIDGET (self->canvas));
  update_connect (self);

  self->connecting = TRUE;
  self->moved = FALSE;
}

//-- event handler helper

static void
clutter_event_get_cursor_pos (BtMainPageMachines * self, gfloat x, gfloat y)
{
  widget_to_canvas_pos (self, x, y, &self->mouse_x, &self->mouse_y);
}

static void
machine_item_moved (BtMainPageMachines * self, const gchar * mid)
{
  gchar *undo_str, *redo_str;
  gchar str[G_ASCII_DTOSTR_BUF_SIZE];

  bt_change_log_start_group (self->change_log);

  undo_str =
      g_strdup_printf ("set_machine_property \"%s\",\"xpos\",\"%s\"", mid,
      g_ascii_dtostr (str, G_ASCII_DTOSTR_BUF_SIZE, self->machine_xo));
  redo_str =
      g_strdup_printf ("set_machine_property \"%s\",\"xpos\",\"%s\"", mid,
      g_ascii_dtostr (str, G_ASCII_DTOSTR_BUF_SIZE, self->machine_xn));
  bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self), undo_str,
      redo_str);
  undo_str =
      g_strdup_printf ("set_machine_property \"%s\",\"ypos\",\"%s\"", mid,
      g_ascii_dtostr (str, G_ASCII_DTOSTR_BUF_SIZE, self->machine_yo));
  redo_str =
      g_strdup_printf ("set_machine_property \"%s\",\"ypos\",\"%s\"", mid,
      g_ascii_dtostr (str, G_ASCII_DTOSTR_BUF_SIZE, self->machine_yn));
  bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self), undo_str,
      redo_str);

  bt_change_log_end_group (self->change_log);
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
      *pos_x = g_ascii_strtod (prop, NULL);
      // do not g_free(prop);
      res = TRUE;
    } else
      GST_INFO ("no xpos property found");
    prop = (gchar *) g_hash_table_lookup (properties, "ypos");
    if (prop) {
      *pos_y = g_ascii_strtod (prop, NULL);
      // do not g_free(prop);
      res &= TRUE;
    } else
      GST_INFO ("no ypos property found");
  } else
    GST_WARNING ("no properties supplied");
  return res;
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
update_machines_zoom (BtMainPageMachines * self)
{
  gchar str[G_ASCII_DTOSTR_BUF_SIZE];

  g_hash_table_insert (self->properties, g_strdup ("zoom"),
      g_strdup (g_ascii_dtostr (str, G_ASCII_DTOSTR_BUF_SIZE,
              self->zoom)));
  bt_edit_application_set_song_unsaved (self->app);

  g_hash_table_foreach (self->machines, update_zoom, &self->zoom);
  g_hash_table_foreach (self->wires, update_zoom, &self->zoom);

  gtk_widget_set_sensitive (self->zoom_out,
      (self->zoom > ZOOM_MIN));
  gtk_widget_set_sensitive (self->zoom_in, (self->zoom < ZOOM_MAX));
}

static void
machine_item_new (BtMainPageMachines * self, BtMachine * machine,
    gdouble xpos, gdouble ypos)
{
  BtMachineCanvasItem *item;

  item =
      bt_machine_canvas_item_new (self, machine, self->zoom);
  g_hash_table_insert (self->machines, machine, item);
  g_signal_connect_object (item, "start-connect",
      G_CALLBACK (on_machine_item_start_connect), (gpointer) self, 0);

  gtk_fixed_put (self->canvas, GTK_WIDGET (item), xpos, ypos);
  
  GST_DEBUG ("machine canvas item added, %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (self));

  g_signal_connect (item, "position-changed",
      G_CALLBACK (on_machine_item_position_changed), (gpointer) self);
  on_machine_item_position_changed (item, 1, (gpointer) self);
}

static void
wire_item_new (BtMainPageMachines * self, BtWire * wire,
    BtMachineCanvasItem * src_machine_item,
    BtMachineCanvasItem * dst_machine_item)
{
  BtWireCanvasItem *item;

  item =
      bt_wire_canvas_item_new (self, wire, src_machine_item, dst_machine_item);

  gtk_widget_set_parent (GTK_WIDGET (self), GTK_WIDGET (self->canvas));
  
  g_hash_table_insert (self->wires, wire, item);
}

/* TODO(ensonic):
 * - we need a bounding box (bb) for all the machines
 *   - from that we can set page-size, upper, lower and pos of the scrollbars
 *   - upper and lower are the min/max of the bounding box
 *   - page-size = MAX (canvas-size, bb-size) / zoom
 * - we need scrollbars when the bb is partially outside the view
 *   - due to zoom or panning
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
update_adjustment (GtkAdjustment * adj, gdouble bbmi, gdouble bbma,
    gdouble page)
{
  gdouble bbd = bbma - bbmi;
  gdouble m, adjva, adjmi, adjma, adjd, adjps;

  g_object_get (adj, "value", &adjva, "lower", &adjmi, "upper", &adjma,
      "page-size", &adjps, NULL);
  adjd = adjma - adjmi;

  GST_DEBUG ("adj: %4d .. %4d .. %4d (%4d) = %4d", (gint) adjmi, (gint) adjva,
      (gint) adjma, (gint) adjps, (gint) adjd);
  GST_DEBUG ("bb : %4d .. %4d (%4d) = %4d", (gint) bbmi, (gint) bbma,
      (gint) page, (gint) bbd);

  // cur value -> relative scrollpos -> new value
  if (adjd > adjps) {
    m = ((adjva + (adjps / 2.0)) - adjmi) / adjd;
    GST_DEBUG ("old-middle: %4.2lf", m);
  } else {
    m = 0.5;
    GST_DEBUG ("old-middle: static=0.5");
  }
  if (bbd > page) {
    adjva = (bbmi + (bbd * m)) - (page / 2.0);
    GST_DEBUG ("new-middle: %4.2lf => %4d", m, (gint) adjva);
    gtk_adjustment_configure (adj, adjva, bbmi, bbma, page / 10.0, page, page);
  } else {
    GST_DEBUG ("new-middle: 0.5 => 0");
    gtk_adjustment_configure (adj, 0.0, bbmi, bbma, bbd / 10.0, bbd, bbd);
  }
}

static void
update_scrolled_window (BtMainPageMachines * self)
{
#if 0
  // dynamic sizing :/
  GST_DEBUG ("adj.x");
  update_adjustment (self->hadjustment, self->mi_x, self->ma_x, self->view_w);
  GST_DEBUG ("adj.y");
  update_adjustment (self->vadjustment, self->mi_y, self->ma_y, self->view_h);
#else
  GST_DEBUG ("adj.x");
  update_adjustment (
      gtk_scrolled_window_get_hadjustment (self->scrolled_window),
      0.0, self->canvas_w * self->zoom, self->view_w);
  GST_DEBUG ("adj.y");
  update_adjustment (
      gtk_scrolled_window_get_vadjustment (self->scrolled_window),
      0.0, self->canvas_h * self->zoom, self->view_h);
#endif
}

static void
machine_actor_move (gpointer key, gpointer value, gpointer user_data)
{
  gfloat *delta = (gfloat *) user_data;
  GtkWidget *widget = GTK_WIDGET (value);

  /// GTK4 tbd: remove need to access parent
  GtkFixed *canvas = GTK_FIXED (gtk_widget_get_parent (widget));
  
  gtk_fixed_move (canvas, widget, delta[0], delta[1]);
  g_signal_emit_by_name (value, "position-changed", 0, 1);
}

static void
update_scrolled_window_zoom (BtMainPageMachines * self)
{
  gfloat cw, ch, delta[2];

  // need to make stage+canvas large enough to show grid when scrolling
  cw = MACHINE_VIEW_W;
  if ((cw * self->zoom) < self->view_w) {
    GST_DEBUG ("canvas: enlarge x to fit view");
    cw = self->view_w / self->zoom;
  }
  ch = MACHINE_VIEW_H;
  if ((ch * self->zoom) < self->view_h) {
    GST_DEBUG ("canvas: enlarge y to fit view");
    ch = self->view_h / self->zoom;
  }
  delta[0] = (cw - self->canvas_w) / 2.0;
  delta[1] = (ch - self->canvas_h) / 2.0;

  GST_DEBUG ("zoom: %4.1f", self->zoom);
  GST_DEBUG ("canvas: %4.1f,%4.1f -> %4.1f,%4.1f", self->canvas_w, self->canvas_h,
      cw, ch);
  GST_DEBUG ("delta: %4.1f,%4.1f", delta[0], delta[1]);

  self->canvas_w = cw;
  self->canvas_h = ch;

  // apply zoom here as we don't scale the stage
  gtk_widget_set_size_request (GTK_WIDGET (self->canvas), cw, ch);

  // keep machines centered
  g_hash_table_foreach (self->machines, machine_actor_move, delta);

  update_scrolled_window (self);
}

static gboolean
machine_view_remove_item (gpointer key, gpointer value, gpointer user_data)
{
  gtk_widget_unparent (GTK_WIDGET (value));
  return TRUE;
}

static void
machine_view_clear (BtMainPageMachines * self)
{
  // clear the canvas
  GST_DEBUG ("before destroying machine canvas items");
  g_hash_table_foreach_remove (self->machines,
      machine_view_remove_item, NULL);
  GST_DEBUG ("before destoying wire canvas items");
  g_hash_table_foreach_remove (self->wires,
      machine_view_remove_item, NULL);
  GST_DEBUG ("done");
}

static void
machine_view_refresh (BtMainPageMachines * self)
{
  GHashTable *properties;
  BtMachineCanvasItem *src_machine_item, *dst_machine_item;
  BtMachine *machine, *src_machine, *dst_machine;
  BtWire *wire;
  gdouble xr, yr, xc, yc;
  GList *node, *list;
  gchar *prop;

  GtkAdjustment *hadjustment =
    gtk_scrolled_window_get_hadjustment (self->scrolled_window);
  GtkAdjustment *vadjustment =
    gtk_scrolled_window_get_vadjustment (self->scrolled_window);

  machine_view_clear (self);

  // update view
  if ((prop = (gchar *) g_hash_table_lookup (self->properties, "zoom"))) {
    self->zoom = g_ascii_strtod (prop, NULL);
#if 0 /// GTK 4
    clutter_actor_set_scale (self->canvas, self->zoom,
        self->zoom);
#endif
    update_scrolled_window (self);
    GST_INFO ("set zoom to %6.4lf", self->zoom);
  }
  if ((prop = (gchar *) g_hash_table_lookup (self->properties, "xpos"))) {
    gtk_adjustment_set_value (hadjustment, g_ascii_strtod (prop,
            NULL));
    GST_INFO ("set xpos to %s", prop);
  } else {
    gdouble xs, xe, xp;
    // center
    g_object_get (hadjustment, "lower", &xs, "upper", &xe,
        "page-size", &xp, NULL);
    gtk_adjustment_set_value (hadjustment,
        xs + ((xe - xs - xp) * 0.5));
  }
  if ((prop = (gchar *) g_hash_table_lookup (self->properties, "ypos"))) {
    gtk_adjustment_set_value (
        gtk_scrolled_window_get_vadjustment (self->scrolled_window),
        g_ascii_strtod (prop, NULL));
    GST_INFO ("set ypos to %s", prop);
  } else {
    gdouble ys, ye, yp;
    // center
    g_object_get (vadjustment,
        "lower", &ys, "upper", &ye, "page-size", &yp, NULL);
    gtk_adjustment_set_value (
        vadjustment,
        ys + ((ye - ys - yp) * 0.5));
  }

  GST_INFO ("creating machine canvas items");

  // draw all machines
  g_object_get (self->setup, "machines", &list, NULL);
  for (node = list; node; node = g_list_next (node)) {
    machine = BT_MACHINE (node->data);
    // get position
    g_object_get (machine, "properties", &properties, NULL);
    machine_view_get_machine_position (properties, &xr, &yr);
    bt_main_page_machines_relative_coords_to_canvas (self, xr, yr, &xc, &yc);
    // draw machine
    machine_item_new (self, machine, xc, yc);
  }
  g_list_free (list);

  // draw all wires
  g_object_get (self->setup, "wires", &list, NULL);
  for (node = list; node; node = g_list_next (node)) {
    wire = BT_WIRE (node->data);
    // get positions of source and dest
    g_object_get (wire, "src", &src_machine, "dst", &dst_machine, NULL);
    src_machine_item = g_hash_table_lookup (self->machines, src_machine);
    dst_machine_item = g_hash_table_lookup (self->machines, dst_machine);
    // draw wire
    wire_item_new (self, wire, src_machine_item, dst_machine_item);
    g_object_unref (src_machine);
    g_object_unref (dst_machine);
    // TODO(ensonic): get "analyzer-window-state" and if set,
    // get xpos, ypos and open window
  }
  g_list_free (list);
  GST_DEBUG ("drawing done");
}

static void
bt_main_page_machines_add_wire (BtMainPageMachines * self)
{
  BtSong *song;
  BtWire *wire;
  GError *err = NULL;
  BtMachine *src_machine, *dst_machine;

  g_assert (self->new_wire_src);
  g_assert (self->new_wire_dst);

  g_object_get (self->app, "song", &song, NULL);
  g_object_get (self->new_wire_src, "machine", &src_machine, NULL);
  g_object_get (self->new_wire_dst, "machine", &dst_machine, NULL);

  // try to establish a new connection
  wire = bt_wire_new (song, src_machine, dst_machine, &err);
  if (err == NULL) {
    gchar *undo_str, *redo_str;
    gchar *smid, *dmid;

    // draw wire
    wire_item_new (self, wire, self->new_wire_src,
        self->new_wire_dst);

    g_object_get (src_machine, "id", &smid, NULL);
    g_object_get (dst_machine, "id", &dmid, NULL);

    undo_str = g_strdup_printf ("rem_wire \"%s\",\"%s\"", smid, dmid);
    redo_str = g_strdup_printf ("add_wire \"%s\",\"%s\"", smid, dmid);
    bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self),
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

/// GTK4 remove this
static BtMachineCanvasItem *
get_machine_canvas_item_under_cursor (BtMainPageMachines * self)
{
#if 0
  ClutterActor *ci;
  gfloat x, y;

  canvas_to_widget_pos (self, self->mouse_x, self->mouse_y, &x, &y);

  //GST_DEBUG ("is there a machine at pos?");
  if ((ci = clutter_stage_get_actor_at_pos ((ClutterStage *) self->stage,
              CLUTTER_PICK_REACTIVE, x, y))) {
    if (BT_IS_MACHINE_CANVAS_ITEM (ci)) {
      //GST_DEBUG("  yes!");
      return BT_MACHINE_CANVAS_ITEM (g_object_ref (ci));
    }
  }
#endif
  
  return NULL;
}

static gboolean
bt_main_page_machines_check_wire (BtMainPageMachines * self)
{
  gboolean ret = FALSE;
  BtMachine *src_machine, *dst_machine;

  GST_INFO ("can we link to it?");

  g_assert (self->new_wire_src);
  g_assert (self->new_wire_dst);

  g_object_get (self->new_wire_src, "machine", &src_machine, NULL);
  g_object_get (self->new_wire_dst, "machine", &dst_machine, NULL);

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
  return ret;
}

//-- event handler

static void
on_machine_item_start_connect (BtMachineCanvasItem * machine_item,
    gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);

  self->new_wire_src = g_object_ref (machine_item);
  g_object_get (machine_item, "x", &self->mouse_x, "y",
      &self->mouse_y, NULL);
#if 0 /// GTK 4
  start_connect (self);
#endif
}

static void
machine_actor_update_bb (gpointer key, gpointer value, gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);
  gfloat px, py;

#if 0 /// GTK 4
  clutter_actor_get_position ((ClutterActor *) value, &px, &py);
#endif
  
  if (px < self->mi_x)
    self->mi_x = px;
  else if (px > self->ma_x)
    self->ma_x = px;
  if (py < self->mi_y)
    self->mi_y = py;
  else if (py > self->ma_y)
    self->ma_y = py;
}

static void
machine_actor_update_pos_and_bb (BtMainPageMachines * self, GtkWidget * ci,
    gdouble * x, gdouble * y)
{
  gdouble px, py;

  gtk_fixed_get_child_position (self->canvas, ci, &px, &py);
  
  bt_main_page_machines_canvas_coords_to_relative (self, px, py, x, y);

  self->mi_x = self->ma_x = px;
  self->mi_y = self->ma_y = py;
  g_hash_table_foreach (self->machines, machine_actor_update_bb, self);
  GST_DEBUG ("bb: %4.2f,%4.2f .. %4.2f,%4.2f",
      self->mi_x, self->mi_y, self->ma_x, self->ma_y);
  update_scrolled_window (self);
}

/// GTK4 ev_type actually represents a 'phase' of the event.
/// Starting, dragging, and releasing the machine.
/// These could be implemented as separate signals, too.
/// TBD: At the least, use an enum for ev_type instead.
static void
on_machine_item_position_changed (BtMachineCanvasItem * machine_item,
    guint ev_type, gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);
  gchar *mid;

  switch (ev_type) {
    case 0: // CLUTTER_BUTTON_PRESS:
      machine_actor_update_pos_and_bb (self, GTK_WIDGET (machine_item),
          &self->machine_xo, &self->machine_yo);
      break;
    case 1: // CLUTTER_MOTION:
      machine_actor_update_pos_and_bb (self, GTK_WIDGET (machine_item),
          NULL, NULL);
      break;
    case 2: // CLUTTER_BUTTON_RELEASE:
      machine_actor_update_pos_and_bb (self, GTK_WIDGET (machine_item),
          &self->machine_xn, &self->machine_yn);
      bt_child_proxy_get (machine_item, "machine::id", &mid, NULL);
      machine_item_moved (self, mid);
      g_free (mid);
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
  gdouble xr, yr, xc, yc;

  GST_INFO_OBJECT (machine, "new machine added: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine));

  g_object_get (machine, "properties", &properties, NULL);
  if (properties && machine_view_get_machine_position (properties, &xr, &yr)) {
    bt_main_page_machines_relative_coords_to_canvas (self, xr, yr, &xc, &yc);
  } else {
    xc = self->mouse_x;
    yc = self->mouse_y;
  }
  self->machine_xn = xc;
  self->machine_yn = yc;

  GST_DEBUG_OBJECT (machine,
      "adding machine at %lf x %lf, mouse is at %lf x %lf", xc, yc,
      self->mouse_x, self->mouse_y);

  // draw machine
  machine_item_new (self, machine, xc, yc);

  GST_INFO_OBJECT (machine, "... machine added: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine));
}

static void
on_machine_removed (BtSetup * setup, BtMachine * machine, gpointer user_data)
{
  /// GTK4 BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);
  /// GTK4 ClutterActor *item;

  if (!machine)
    return;

  GST_INFO_OBJECT (machine, "machine removed: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine));

#if 0 /// GTK 4
  if ((item = g_hash_table_lookup (self->machines, machine))) {
    GST_INFO ("now removing machine-item : %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (item));
    g_hash_table_remove (self->machines, machine);
    clutter_actor_destroy (item);
  }
#endif

  GST_INFO_OBJECT (machine, "... machine removed: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine));
}

static void
on_wire_removed (BtSetup * setup, BtWire * wire, gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);
  ///GTK4 ClutterActor *item;

  if (!wire) {
    return;
  }

  GST_INFO_OBJECT (wire, "wire removed: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (wire));

  // add undo/redo details
  if (bt_change_log_is_active (self->change_log)) {
    gchar *undo_str, *redo_str;
    gchar *prop;
    BtMachine *src_machine, *dst_machine;
    GHashTable *properties;
    gchar *smid, *dmid;

    g_object_get (wire, "src", &src_machine, "dst", &dst_machine, "properties",
        &properties, NULL);
    g_object_get (src_machine, "id", &smid, NULL);
    g_object_get (dst_machine, "id", &dmid, NULL);

    bt_change_log_start_group (self->change_log);

    undo_str = g_strdup_printf ("add_wire \"%s\",\"%s\"", smid, dmid);
    redo_str = g_strdup_printf ("rem_wire \"%s\",\"%s\"", smid, dmid);
    bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self),
        undo_str, redo_str);

    prop = (gchar *) g_hash_table_lookup (properties, "analyzer-shown");
    if (prop && *prop == '1') {
      undo_str =
          g_strdup_printf
          ("set_wire_property \"%s\",\"%s\",\"analyzer-shown\",\"1\"", smid,
          dmid);
      bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self),
          undo_str, g_strdup (undo_str));
    }
    // TODO(ensonic): volume and panorama
    g_free (smid);
    g_free (dmid);

    bt_change_log_end_group (self->change_log);

    g_object_unref (dst_machine);
    g_object_unref (src_machine);
  }

#if 0 /// GTK 4
  if ((item = g_hash_table_lookup (self->wires, wire))) {
    GST_INFO ("now removing wire-item : %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (item));
    g_hash_table_remove (self->wires, wire);
    clutter_actor_destroy (item);
  }
#endif

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
  g_object_try_unref (self->setup);

  // get song from app
  g_object_get (self->app, "song", &song, NULL);
  if (!song) {
    self->setup = NULL;
    self->properties = NULL;
    machine_view_clear (self);
    GST_INFO ("song (null) has changed done");
    return;
  }
  GST_INFO ("song: %" G_OBJECT_REF_COUNT_FMT, G_OBJECT_LOG_REF_COUNT (song));

  g_object_get (song, "setup", &self->setup, NULL);
  g_object_get (self->setup, "properties", &self->properties, NULL);
  // update page
  machine_view_refresh (self);
  g_signal_connect_object (self->setup, "machine-added",
      G_CALLBACK (on_machine_added), (gpointer) self, 0);
  g_signal_connect_object (self->setup, "machine-removed",
      G_CALLBACK (on_machine_removed), (gpointer) self, 0);
  g_signal_connect_object (self->setup, "wire-removed",
      G_CALLBACK (on_wire_removed), (gpointer) self, 0);
  // release the reference
  g_object_unref (song);
  GST_INFO ("song has changed done");
}

static void
on_toolbar_zoom_fit_clicked (GtkWidget* widget, const char* action_name, GVariant* parameter)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (widget);
  gdouble old_zoom = self->zoom;
  gdouble fc_x, fc_y, c_x, c_y;
  gdouble ma_xs, ma_xe, ma_xd, ma_ys, ma_ye, ma_yd;

  GST_INFO ("bb: x:%+6.4lf...%+6.4lf and y:%+6.4lf...%+6.4lf",
      self->mi_x, self->ma_x, self->mi_y, self->ma_y);

  // need to add machine-icon size + some space
  ma_xs = self->mi_x - MACHINE_W;
  ma_xe = self->ma_x + MACHINE_W;
  ma_xd = (ma_xe - ma_xs);
  ma_ys = self->mi_y - MACHINE_H;
  ma_ye = self->ma_y + MACHINE_H;
  ma_yd = (ma_ye - ma_ys);

  // zoom
  fc_x = self->view_w / ma_xd;
  fc_x = CLAMP (fc_x, ZOOM_MIN, ZOOM_MAX);
  fc_y = self->view_h / ma_yd;
  fc_y = CLAMP (fc_y, ZOOM_MIN, ZOOM_MAX);
  self->zoom = MIN (fc_x, fc_y);
  GST_INFO ("x:%+6.4lf / %+6.4lf = %+6.4lf", self->view_w, ma_xd, fc_x);
  GST_INFO ("y:%+6.4lf / %+6.4lf = %+6.4lf", self->view_h, ma_yd, fc_y);
  GST_INFO ("zoom: %6.4lf -> %6.4lf", old_zoom, self->zoom);

  // center region
  /* pos can be between: lower ... upper-page_size) */
  c_x = (ma_xs + (ma_xd / 2.0)) * self->zoom - (self->view_w / 2.0);
  c_y = (ma_ys + (ma_yd / 2.0)) * self->zoom - (self->view_h / 2.0);
  GST_INFO ("center: x/y = %+6.4lf,%+6.4lf", c_x, c_y);

  if (self->zoom > old_zoom) {
#if 0 /// GTK 4
    clutter_actor_set_scale (self->canvas, self->zoom, self->zoom);
#endif
    update_scrolled_window_zoom (self);
    update_machines_zoom (self);
  } else {
    update_machines_zoom (self);
#if 0 /// GTK 4
    clutter_actor_set_scale (self->canvas, self->zoom, self->zoom);
#endif
    update_scrolled_window_zoom (self);
  }
  
  gtk_adjustment_set_value (
      gtk_scrolled_window_get_hadjustment (self->scrolled_window), c_x);
  
  gtk_adjustment_set_value (
      gtk_scrolled_window_get_vadjustment (self->scrolled_window), c_y);

  gtk_widget_grab_focus_savely (GTK_WIDGET (self->canvas));
}

static void
on_toolbar_zoom_in_clicked (GtkWidget* widget, const char* action_name, GVariant* parameter)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (widget);

  self->zoom *= 1.2;
  GST_INFO ("toolbar zoom_in event occurred : %lf", self->zoom);

#if 0 /// GTK 4
  clutter_actor_set_scale (self->canvas, self->zoom, self->zoom);
#endif
  update_scrolled_window_zoom (self);
  update_machines_zoom (self);

  gtk_widget_grab_focus_savely (GTK_WIDGET (self->canvas));
}

static void
on_toolbar_zoom_out_clicked (GtkWidget* widget, const char* action_name, GVariant* parameter)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (widget);

  self->zoom /= 1.2;
  GST_INFO ("toolbar zoom_out event occurred : %lf", self->zoom);

  update_machines_zoom (self);
#if 0 /// GTK 4
  clutter_actor_set_scale (self->canvas, self->zoom, self->zoom);
#endif
  update_scrolled_window_zoom (self);

  gtk_widget_grab_focus_savely (GTK_WIDGET (self->canvas));
}

static void
on_toolbar_grid_clicked (GtkWidget* widget, const char* action_name, GVariant* parameter)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (widget);
  
  GtkPopover *popover = GTK_POPOVER (gtk_popover_menu_new_from_model (
      G_MENU_MODEL (self->context_menu)));
  gtk_widget_set_parent (GTK_WIDGET (popover), GTK_WIDGET (self));
  gtk_popover_popup (popover);
}

static void
on_toolbar_menu_show_clicked (GtkWidget* widget, const char* action_name, GVariant* parameter)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (widget);
  
  GtkPopover *popover = GTK_POPOVER (gtk_popover_menu_new_from_model (
      G_MENU_MODEL (self->context_menu)));
  gtk_widget_set_parent (GTK_WIDGET (popover), GTK_WIDGET (self));
  gtk_popover_popup (popover);
}

#if 0 /// GTK 4
static void
on_toolbar_grid_densitoff_y_activated (GtkMenuItem * menuitem,
    gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);

  if (!gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)))
    return;

  self->grid_density = 0;
  bt_child_proxy_set (self->app, "settings::grid-density", "off", NULL);
  clutter_content_invalidate (self->grid_canvas);
}
#endif

#if 0 /// GTK 4
static void
on_toolbar_grid_density_low_activated (GtkMenuItem * menuitem,
    gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);

  if (!gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)))
    return;

  self->grid_density = 1;
  bt_child_proxy_set (self->app, "settings::grid-density", "low", NULL);
  clutter_content_invalidate (self->grid_canvas);
}
#endif

#if 0 /// GTK 4
static void
on_toolbar_grid_density_mid_activated (GtkMenuItem * menuitem,
    gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);

  if (!gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)))
    return;

  self->grid_density = 2;
  bt_child_proxy_set (self->app, "settings::grid-density", "medium",
      NULL);
  clutter_content_invalidate (self->grid_canvas);
}
#endif

#if 0 /// GTK 4
static void
on_toolbar_grid_density_high_activated (GtkMenuItem * menuitem,
    gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);

  if (!gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)))
    return;

  self->grid_density = 3;
  bt_child_proxy_set (self->app, "settings::grid-density", "high", NULL);
  clutter_content_invalidate (self->grid_canvas);
}
#endif

#if 0 /// GTK4
static void
on_context_menu_unmute_all (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);
  GList *node, *list;
  BtMachine *machine;

  g_object_get (self->setup, "machines", &list, NULL);
  for (node = list; node; node = g_list_next (node)) {
    machine = BT_MACHINE (node->data);
    // TODO(ensonic): this also un-solos and un-bypasses
    g_object_set (machine, "state", BT_MACHINE_STATE_NORMAL, NULL);
  }
  g_list_free (list);
}
#endif

static gboolean
on_canvas_query_tooltip (GtkWidget * widget, gint x, gint y,
    gboolean keyboard_mode, GtkTooltip * tooltip, gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);
  BtMachineCanvasItem *ci;
  gchar *name;

  widget_to_canvas_pos (self, x, y, &self->mouse_x, &self->mouse_y);
  if (!(ci = get_machine_canvas_item_under_cursor (self)))
    return FALSE;

  bt_child_proxy_get (ci, "machine::pretty_name", &name, NULL);
  gtk_tooltip_set_text (tooltip, name);
  g_free (name);
  g_object_unref (ci);
  return TRUE;
}

static void
on_canvas_size_changed (GtkDrawingArea* widget, gint width, gint height,
    gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);
  gfloat vw = width, vh = height;

  if ((vw == self->view_w) && (vh == self->view_h))
    return;

  GST_DEBUG ("view: %4.1f,%4.1f -> %4.1f,%4.1f", self->view_w, self->view_h, vw, vh);

  // size of the view
  self->view_w = vw;
  self->view_h = vh;

  update_scrolled_window_zoom (self);
}

static void
store_scroll_pos (BtMainPageMachines * self, gchar * name, gdouble val)
{
  GST_LOG ("%s: %lf", name, val);
  if (self->properties) {
    gchar str[G_ASCII_DTOSTR_BUF_SIZE];
    gchar *prop;
    gdouble oval = 0.0;
    gboolean have_val = FALSE;

    if ((prop = (gchar *) g_hash_table_lookup (self->properties, name))) {
      oval = g_ascii_strtod (prop, NULL);
      have_val = TRUE;
    }
    if ((!have_val) || (oval != val)) {
      g_hash_table_insert (self->properties, g_strdup (name),
          g_strdup (g_ascii_dtostr (str, G_ASCII_DTOSTR_BUF_SIZE, val)));
      if (have_val)
        bt_edit_application_set_song_unsaved (self->app);
    }
  }
}

static gboolean
idle_redraw (gpointer user_data)
{
#if 0 /// GTK 4
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);
  clutter_content_invalidate (self->grid_canvas);
#endif
  return FALSE;
}

static void
on_page_switched (GtkNotebook * notebook, GParamSpec * arg, gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);
  guint page_num;
  static gint prev_page_num = -1;

  g_object_get (notebook, "page", &page_num, NULL);

  if (page_num != BT_MAIN_PAGES_MACHINES_PAGE) {
    // only do this if the page was BT_MAIN_PAGES_MACHINES_PAGE
    if (prev_page_num == BT_MAIN_PAGES_MACHINES_PAGE) {
      GST_DEBUG ("leave machine page");
      bt_child_proxy_set (self->app,
          "main-window::statusbar::status", NULL, NULL);
    }
  }
  prev_page_num = page_num;
}

static void
on_canvas_button_press (GtkGestureClick* click, gint n_press, gdouble x,
    gdouble y, gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);
  BtMachineCanvasItem *ci;
  guint button = gtk_gesture_single_get_current_button (GTK_GESTURE_SINGLE (click));
  GdkModifierType state = gtk_event_controller_get_current_event_state (
      GTK_EVENT_CONTROLLER (click));
 
  GST_DEBUG ("button-press: %d", button);
  // store mouse coordinates, so that we can later place a newly added machine there
  clutter_event_get_cursor_pos (self, x, y);

  if (self->connecting) {
    return;
  }

  if (!(ci = get_machine_canvas_item_under_cursor (self))) {
    if (button == GDK_BUTTON_PRIMARY) {
      // start dragging the canvas
      self->dragging = TRUE;
    } else if (button == GDK_BUTTON_SECONDARY) {
      GtkPopover *popover = GTK_POPOVER (gtk_popover_menu_new_from_model (
          G_MENU_MODEL (self->context_menu)));
      gtk_widget_set_parent (GTK_WIDGET (popover), GTK_WIDGET (self));
      GdkRectangle rect = { x, y, 0, 0 };
      gtk_popover_set_pointing_to (popover, &rect);
      gtk_popover_popup (popover);
    }
  } else {
    if (button == GDK_BUTTON_PRIMARY) {
      if (state & GDK_SHIFT_MASK) {
        BtMachine *machine;
        self->new_wire_src = BT_MACHINE_CANVAS_ITEM (g_object_ref (ci));
        g_object_get (ci, "machine", &machine, NULL);
        // if the citem->machine is a source/processor-machine
        if (BT_IS_SOURCE_MACHINE (machine)
            || BT_IS_PROCESSOR_MACHINE (machine)) {
          start_connect (self);
        }
        g_object_unref (machine);
      }
    }
    g_object_unref (ci);
  }
}

static void
on_canvas_button_release (GtkGestureClick* click, gint n_press, gdouble x,
    gdouble y, gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);
  guint button = gtk_gesture_single_get_current_button (
      GTK_GESTURE_SINGLE (click));

  GST_DEBUG ("button-release: %d", button);
  if (self->connecting) {
    clutter_event_get_cursor_pos (self, x, y);
    g_object_try_unref (self->new_wire_dst);
    self->new_wire_dst = get_machine_canvas_item_under_cursor (self);
    if (self->new_wire_dst) {
      if (bt_main_page_machines_check_wire (self)) {
        bt_main_page_machines_add_wire (self);
      }
      g_object_unref (self->new_wire_dst);
      self->new_wire_dst = NULL;
    }
    g_object_unref (self->new_wire_src);
    self->new_wire_src = NULL;
    
    gtk_widget_set_parent (GTK_WIDGET (self->new_wire), NULL);

    g_clear_object (&self->new_wire);
    self->connecting = FALSE;
  } else if (self->dragging) {
    self->dragging = FALSE;
  }
}

static void
on_canvas_motion (GtkEventControllerMotion* motion, gdouble x, gdouble y,
    gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);

  //GST_DEBUG("motion: %f,%f",event->button.x,event->button.y);
  if (self->connecting) {
    // update the connection line
    clutter_event_get_cursor_pos (self, x, y);
    g_object_try_unref (self->new_wire_dst);
    self->new_wire_dst = get_machine_canvas_item_under_cursor (self);
    update_connect (self);
  } else if (self->dragging) {
    gfloat x = self->mouse_x, y = self->mouse_y;
#if 0 /// GTK4 set scrolled window pos by normal means
    gdouble ox = gtk_adjustment_get_value (self->hadjustment);
    gdouble oy = gtk_adjustment_get_value (self->vadjustment);
    // snapshot current mousepos and calculate delta
    clutter_event_get_cursor_pos (self, x, y);
    gfloat dx = x - self->mouse_x;
    gfloat dy = y - self->mouse_y;
    // scroll canvas
    gtk_adjustment_set_value (self->hadjustment, ox + dx);
    gtk_adjustment_set_value (self->vadjustment, oy + dy);
    self->mouse_x += dx;
    self->mouse_y += dy;
#endif
  }
}

static void
on_canvas_key_release (GtkEventControllerKey* key, guint keyval,
    guint keycode, GdkModifierType state, gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);

  /// GTK4 clutter_event_get_cursor_pos (self, event);
  GST_DEBUG ("GDK_KEY_RELEASE: %d", keyval);
  switch (keyval) {
    case GDK_KEY_Menu:
      // show context menu
      gtk_popover_popup (GTK_POPOVER (self->machine_menu_button));
      break;
    default:
      break;
  }
}

static void
on_volume_popup_changed (GtkAdjustment * adj, gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);
  // FIXME(ensonic): workaround for https://bugzilla.gnome.org/show_bug.cgi?id=667598
  gdouble gain = 4.0 - (gtk_adjustment_get_value (adj) / 100.0);
  g_object_set (self->wire_gain, "volume", gain, NULL);
}

static void
on_panorama_popup_changed (GtkAdjustment * adj, gpointer user_data)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);
  gfloat pan = (gfloat) gtk_adjustment_get_value (adj) / 100.0;
  g_object_set (self->wire_pan, "panorama", pan, NULL);
}

static void
on_canvas_style_updated (GtkStyleContext * style_ctx, gconstpointer user_data)
{
#if 0 /// GTK4
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (user_data);

  gtk_style_context_lookup_color (style_ctx, "new_wire_good",
      &self->wire_good_color);
  gtk_style_context_lookup_color (style_ctx, "new_wire_bad",
      &self->wire_bad_color);

  if (self->grid_canvas) {
    clutter_content_invalidate (self->grid_canvas);
  }
#endif
}

//-- helper methods

static void
bt_main_page_machines_init_main_context_menu (BtMainPageMachines * self)
{
  GtkBuilder* builder = gtk_builder_new_from_resource (
      "/org/buzztrax/ui/machine-context-menu.ui");
  
  self->context_menu = G_MENU (gtk_builder_get_object (builder, "menu"));
  g_object_ref (self->context_menu);

  // GTK4 BtMachineMenu remains an object so that it can listen for machine
  // list updates.
  self->machine_menu = bt_machine_menu_new (self);
  g_menu_prepend_submenu (self->context_menu, _("Add machine"),
      G_MENU_MODEL (bt_machine_menu_get_menu (self->machine_menu)));
  
  g_object_unref (builder);
}

void
bt_main_page_machines_set_pages_parent (BtMainPageMachines * self, BtMainPages* pages)
{
  // It's assumed this will never be called more than once.
  /// GTK4 is it still needed at all?
  static gboolean executed = FALSE;
  g_assert (!executed);
  executed = TRUE;
  
  g_signal_connect ((gpointer) pages, "notify::page",
      G_CALLBACK (on_page_switched), (gpointer) self);
}

static void
bt_main_page_machines_init_ui (BtMainPageMachines * self)
{
  BtSettings *settings;
  /// GTK4 GtkStyleContext *style;
  gchar *density;

  GST_DEBUG ("!!!! self=%p", self);
  gtk_widget_set_name (GTK_WIDGET (self), "machine view");
  g_object_get (self->app, "settings", &settings, NULL);
  g_object_get (settings, "grid-density", &density, NULL);

  GEnumClass *enum_class = g_type_class_ref (
      bt_main_page_machines_grid_density_get_type ());
  
  GEnumValue *value = g_enum_get_value_by_nick (enum_class,  density);
  self->grid_density =
    value ? value->value : BT_MAIN_PAGE_MACHINES_GRID_DENSITY_LOW;
  
  g_free (density);

  // create the context menu
  bt_main_page_machines_init_main_context_menu (self);

  gtk_widget_set_sensitive (self->zoom_in, (self->zoom < ZOOM_MAX));
  gtk_widget_set_sensitive (self->zoom_out,
      (self->zoom > ZOOM_MIN));

  // popup menu button
  g_assert (self->context_menu);

  /*
    /// GTK4 this could be used in future to make the popups more like
    /// traditional ones.
  gtk_popover_menu_set_flags (
      gtk_menu_button_get_popover (self->machine_menu_button),
      GTK_POPOVER_MENU_NESTED);
  */
  
  gtk_menu_button_set_menu_model (self->machine_menu_button,
      G_MENU_MODEL (self->context_menu));

  gtk_drawing_area_set_draw_func (self->grid_canvas, on_grid_draw, self, NULL);
  
  // create volume popup
  self->vol_popup_adj =
      gtk_adjustment_new (100.0, 0.0, 400.0, 1.0, 10.0, 1.0);
  self->vol_popup =
      BT_VOLUME_POPUP (bt_volume_popup_new (self->vol_popup_adj));
  g_signal_connect (self->vol_popup_adj, "value-changed",
      G_CALLBACK (on_volume_popup_changed), (gpointer) self);

  // create panorama popup
  self->pan_popup_adj =
      gtk_adjustment_new (0.0, -100.0, 100.0, 1.0, 10.0, 1.0);
  self->pan_popup =
      BT_PANORAMA_POPUP (bt_panorama_popup_new (self->pan_popup_adj));
  g_signal_connect (self->pan_popup_adj, "value-changed",
      G_CALLBACK (on_panorama_popup_changed), (gpointer) self);

  // register event handlers
  g_signal_connect_object (self->app, "notify::song",
      G_CALLBACK (on_song_changed), (gpointer) self, 0);
  
  /// GTK4 replace with drag gesture?
  GtkGesture *gesture = gtk_gesture_click_new ();
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (gesture), 0);
  gtk_widget_add_controller (GTK_WIDGET (self->canvas), GTK_EVENT_CONTROLLER (gesture));
  g_signal_connect_object (gesture, "pressed",
      G_CALLBACK (on_canvas_button_press), (gpointer) self, 0);
  g_signal_connect_object (gesture, "released",
      G_CALLBACK (on_canvas_button_release), (gpointer) self, 0);

  GtkEventController *ctrl;
  ctrl = gtk_event_controller_motion_new ();
  gtk_widget_add_controller (GTK_WIDGET (self->canvas), ctrl);
  g_signal_connect_object (ctrl, "motion",
      G_CALLBACK (on_canvas_motion), (gpointer) self, 0);
  
  ctrl = gtk_event_controller_key_new ();
  gtk_widget_add_controller (GTK_WIDGET (self->canvas), ctrl);
  g_signal_connect_object (ctrl, "key-released",
      G_CALLBACK (on_canvas_key_release), (gpointer) self, 0);

#if 0 /// GTK4
  // let settings control toolbar style
  g_object_bind_property_full (settings, "toolbar-style", self->toolbar,
      "toolbar-style", G_BINDING_SYNC_CREATE, bt_toolbar_style_changed, NULL,
      NULL, NULL);
#endif
  
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
bt_main_page_machines_new ()
{
  return BT_MAIN_PAGE_MACHINES (g_object_new (BT_TYPE_MAIN_PAGE_MACHINES, NULL));
}

//-- methods

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
bt_main_page_machines_wire_volume_popup (BtMainPageMachines * self,
    BtWire * wire, gint xpos, gint ypos)
{
  gdouble gain;

  g_object_try_unref (self->wire_gain);
  g_object_get (wire, "gain", &self->wire_gain, NULL);
  /* set initial value */
  g_object_get (self->wire_gain, "volume", &gain, NULL);
  // FIXME(ensonic): workaround for https://bugzilla.gnome.org/show_bug.cgi?id=667598
  gtk_adjustment_set_value (self->vol_popup_adj, (4.0 - gain) * 100.0);

  bt_volume_popup_show (self->vol_popup, GTK_WIDGET (self));
  return TRUE;
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
bt_main_page_machines_wire_panorama_popup (BtMainPageMachines * self,
    BtWire * wire, gint xpos, gint ypos)
{
  g_object_try_unref (self->wire_pan);
  g_object_get (wire, "pan", &self->wire_pan, NULL);
  if (self->wire_pan) {
    gfloat pan;
    /* set initial value */
    g_object_get (self->wire_pan, "panorama", &pan, NULL);
    gtk_adjustment_set_value (self->pan_popup_adj, (gdouble) pan * 100.0);

    bt_panorama_popup_show (self->pan_popup);
  }
  return TRUE;
}

static gboolean
bt_main_page_machines_add_machine (BtMainPageMachines * self,
    guint type, const gchar * id, const gchar * plugin_name,
    gdouble x, gdouble y)
{
  BtMachine *machine = NULL;
  gchar *uid;
  GError *err = NULL;
  
  BtSong *song;
  g_object_get (self->app, "song", &song, NULL);
  g_assert (song);

  BtSetup *setup;
  g_object_get (song, "setup", &setup, NULL);
  
  uid = bt_setup_get_unique_machine_id (setup, id);
  
  BtMachineConstructorParams cparams;
  cparams.id = uid;
  cparams.song = song;
  
  // try with 1 voice, if monophonic, voices will be reset to 0 in
  // bt_machine_init_voice_params()
  switch (type) {
    case 0:
      machine = BT_MACHINE (bt_source_machine_new (&cparams, plugin_name,
              /*voices= */ 1, &err));
      break;
    case 1:
      machine = BT_MACHINE (bt_processor_machine_new (&cparams, plugin_name,
              /*voices= */ 1, &err));
      break;
  }
  if (err == NULL) {
    gchar *undo_str, *redo_str;
    GHashTable *properties;

    GST_INFO_OBJECT (machine, "created machine %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (machine));
    bt_change_log_start_group (self->change_log);

    g_object_get (machine, "properties", &properties, NULL);
    if (properties) {
      gchar str[G_ASCII_DTOSTR_BUF_SIZE];
      g_hash_table_insert (properties, g_strdup ("xpos"),
          g_strdup (g_ascii_dtostr (str, G_ASCII_DTOSTR_BUF_SIZE, x)));
      g_hash_table_insert (properties, g_strdup ("ypos"),
          g_strdup (g_ascii_dtostr (str, G_ASCII_DTOSTR_BUF_SIZE, y)));
    }

    undo_str = g_strdup_printf ("rem_machine \"%s\"", uid);
    redo_str =
        g_strdup_printf ("add_machine %u,\"%s\",\"%s\"", type, uid,
        plugin_name);

    bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self),
        undo_str, redo_str);
    bt_change_log_end_group (self->change_log);
  } else {
    GST_WARNING ("Can't create machine %s: %s", plugin_name, err->message);
    g_error_free (err);
    gst_object_unref (machine);
  }
  g_free (uid);
  g_object_unref (setup);
  g_object_unref (song);
  return (err == NULL);
}

static void
bt_main_page_machines_on_machine_add (GtkWidget *widget,
    const char *action_name, GVariant* param)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (widget);
  
  gchar *factory_name;
  gint machine_type;
  gdouble x, y;
  g_variant_get (param, "(&sidd)", &factory_name, &machine_type, &x, &y);

  bt_main_page_machines_add_machine (self, machine_type, factory_name,
      factory_name, x, y);
}

/**
 * bt_main_page_machines_delete_machine:
 * @self: the machines page
 * @machine: the machine to remove
 *
 * Remove a machine from the machine-page.
 */
void
bt_main_page_machines_delete_machine (BtMainPageMachines * self,
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
  bt_change_log_start_group (self->change_log);
  /* TODO(ensonic): by doing undo/redo from setup:machine-removed it happens
   * completely unconstrained, we need to enforce a certain order. Right now the
   * log will reverse the actions. Thus we need to write out the object we
   * remove and then, within, the things that get removed by that.
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
  bt_change_log_start_group (self->change_log);
  undo_str = g_strdup_printf ("add_machine %u,\"%s\",\"%s\"", type, mid, pname);
  redo_str = g_strdup_printf ("rem_machine \"%s\"", mid);
  bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self),
      undo_str, redo_str);
  undo_str =
      g_strdup_printf ("set_machine_property \"%s\",\"voices\",\"%lu\"", mid,
      voices);
  bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self),
      undo_str, g_strdup (undo_str));
  prop = (gchar *) g_hash_table_lookup (properties, "properties-shown");
  if (prop && *prop == '1') {
    undo_str =
        g_strdup_printf
        ("set_machine_property \"%s\",\"properties-shown\",\"1\"", mid);
    bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self),
        undo_str, g_strdup (undo_str));
  }

  if (machine_state != BT_MACHINE_STATE_NORMAL) {
    GEnumClass *enum_class = g_type_class_peek_static (BT_TYPE_MACHINE_STATE);
    GEnumValue *enum_value = g_enum_get_value (enum_class, machine_state);
    undo_str =
        g_strdup_printf ("set_machine_property \"%s\",\"state\",\"%s\"", mid,
        enum_value->value_nick);
    bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self),
        undo_str, g_strdup (undo_str));
  }
  // TODO(ensonic): machine parameters
  g_free (mid);
  g_free (pname);
  // remove patterns for machine, trigger setup::machine-removed
  GST_INFO ("removing the machine : %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine));
  // TODO(ensonic): block machine:pattern-removed signal emission for sequence
  // page to not clobber the sequence. In theory we we don't need to block it,
  // as we are removing the machine anyway and thus disconnecting, but if we
  // disconnect here, disconnecting them on the sequence page would fail
  pattern_removed_id = g_signal_lookup ("pattern-removed", BT_TYPE_MACHINE);
  bt_child_proxy_get (self->app, "main-window::pages::sequence-page",
      &sequence_page, NULL);
  g_signal_handlers_block_matched (machine,
      G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_DATA, pattern_removed_id, 0, NULL,
      NULL, (gpointer) sequence_page);
  bt_setup_remove_machine (self->setup, machine);
  g_object_unref (sequence_page);
  bt_change_log_end_group (self->change_log);
  bt_change_log_end_group (self->change_log);
}

/**
 * bt_main_page_machines_delete_wire:
 * @self: the machines page
 * @wire: the wire to remove
 *
 * Remove a wire from the machine-page (unlink the machines).
 */
void
bt_main_page_machines_delete_wire (BtMainPageMachines * self,
    BtWire * wire)
{
  GST_INFO ("now removing wire: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (wire));
  bt_change_log_start_group (self->change_log);
  bt_setup_remove_wire (self->setup, wire);
  bt_change_log_end_group (self->change_log);
}

/**
 * bt_main_page_machines_rename_machine:
 * @self: the machines page
 * @machine: the machine to rename
 *
 * Run the machine #BtMachineRenameDialog.
 */
void
bt_main_page_machines_rename_machine (BtMainPageMachines * self,
    BtMachine * machine)
{
  bt_machine_rename_dialog_show (machine, self->change_log);
}

/**
 * bt_main_page_machines_canvas_coords_to_relative:
 * @self: the machines page
 * @xc: the x pixel position
 * @yc: the y pixel position
 * @xr: pointer to store the relative x position into
 * @yr: pointer to store the relative y position into
 *
 * Convert the given canvas pixel coordinates into relative coordinates (with a
 * range of -1.0 .. 1.0).
 */
void
bt_main_page_machines_canvas_coords_to_relative (BtMainPageMachines *
    self, const gdouble xc, const gdouble yc, gdouble * xr, gdouble * yr)
{
  if (xr) {
    gdouble cw = self->canvas_w / 2.0;
    *xr = (xc - cw) / (MACHINE_VIEW_W / 2.0);
  }
  if (yr) {
    gdouble ch = self->canvas_h / 2.0;
    *yr = (yc - ch) / (MACHINE_VIEW_H / 2.0);
  }
}

/**
 * bt_main_page_machines_relative_coords_to_canvas:
 * @self: the machines page
 * @xr: the relative x position
 * @yr: the relative y position
 * @xc: pointer to store the canvas x position into
 * @yc: pointer to store the canvas y position into
 *
 * Convert the given relative coordinates (with a range of from -1.0 .. 1.0)
 * into canvas pixel coordinates.
 */
void
bt_main_page_machines_relative_coords_to_canvas (BtMainPageMachines *
    self, const gdouble xr, const gdouble yr, gdouble * xc, gdouble * yc)
{
  if (xc) {
    gdouble cw = self->canvas_w / 2.0;
    *xc = cw + (MACHINE_VIEW_W / 2.0) * xr;
  }
  if (yc) {
    gdouble ch = self->canvas_h / 2.0;
    *yc = ch + (MACHINE_VIEW_H / 2.0) * yr;
  }
}

//-- change logger interface

static gboolean
bt_main_page_machines_change_logger_change (BtChangeLogger * owner,
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
      g_object_get (self->app, "song", &song, NULL);
      
      BtMachineConstructorParams cparams;
      cparams.id = mid;
      cparams.song = song;
      
      switch (type) {
        case 0:
          machine = BT_MACHINE (bt_source_machine_new (&cparams, pname,        /*voices= */
                  1, &err));
          break;
        case 1:
          machine = BT_MACHINE (bt_processor_machine_new (&cparams, pname,
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
      if ((machine = bt_setup_get_machine_by_id (self->setup, mid))) {
        bt_setup_remove_machine (self->setup, machine);
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
      if ((machine = bt_setup_get_machine_by_id (self->setup, mid))) {
        BtMachineCanvasItem *item;
        gboolean is_prop = FALSE;
        g_object_get (machine, "properties", &properties, NULL);
        res = TRUE;
        if (!strcmp (key, "xpos")) {
          if ((item = g_hash_table_lookup (self->machines, machine))) {
            gdouble pos = g_ascii_strtod (val, NULL);
            bt_main_page_machines_relative_coords_to_canvas (self, pos, 0.0,
                &pos, NULL);
            GST_WARNING ("xpos : %s -> %f", val, pos);
            g_object_set (item, "x", (gfloat) pos, NULL);
            /// GTK4 g_signal_emit_by_name (item, "position-changed", 0, CLUTTER_MOTION);
          } else {
            GST_WARNING ("machine '%s' not found", mid);
          }
          is_prop = TRUE;
        } else if (!strcmp (key, "ypos")) {
          if ((item = g_hash_table_lookup (self->machines, machine))) {
            gdouble pos = g_ascii_strtod (val, NULL);
            bt_main_page_machines_relative_coords_to_canvas (self, 0.0, pos,
                NULL, &pos);
            GST_DEBUG ("ypos : %s -> %f", val, pos);
            g_object_set (item, "y", (gfloat) pos, NULL);
            /// GTK4 g_signal_emit_by_name (item, "position-changed", 0, CLUTTER_MOTION);
          } else {
            GST_WARNING ("machine '%s' not found", mid);
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
      g_object_get (self->app, "song", &song, NULL);
      smachine = bt_setup_get_machine_by_id (self->setup, smid);
      dmachine = bt_setup_get_machine_by_id (self->setup, dmid);
      if (smachine && dmachine) {
        BtMachineCanvasItem *src_machine_item, *dst_machine_item;
        wire = bt_wire_new (song, smachine, dmachine, NULL);
        // draw wire
        src_machine_item = g_hash_table_lookup (self->machines, smachine);
        dst_machine_item = g_hash_table_lookup (self->machines, dmachine);
        wire_item_new (self, wire, src_machine_item, dst_machine_item);
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
      smachine = bt_setup_get_machine_by_id (self->setup, smid);
      dmachine = bt_setup_get_machine_by_id (self->setup, dmid);
      if (smachine && dmachine) {
        if ((wire =
                bt_setup_get_wire_by_machines (self->setup, smachine,
                    dmachine))) {
          bt_setup_remove_wire (self->setup, wire);
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
      smachine = bt_setup_get_machine_by_id (self->setup, smid);
      dmachine = bt_setup_get_machine_by_id (self->setup, dmid);
      if (smachine && dmachine) {
        if ((wire =
                bt_setup_get_wire_by_machines (self->setup, smachine,
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
  gtk_widget_grab_focus_savely (GTK_WIDGET (self->canvas));
  /* update status bar */
  // FIXME: window might be NULL
  // in this case we can't continue the lookup and take the property off the
  // va_arg stack
  bt_child_proxy_set (self->app, "main-window::statusbar::status",
      _("Add new machines from right click context menu. "
          "Connect machines with shift+drag from source to target."), NULL);
  return FALSE;
}

static void
bt_main_page_machines_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (object);
  return_if_disposed_self ();
  switch (property_id) {
    case MAIN_PAGE_MACHINES_CANVAS:
      /// GTK4 g_value_set_object (value, self->canvas);
      break;
    case MAIN_PAGE_MACHINES_GRID_DENSITY:
      g_value_set_enum (value, self->grid_density);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_main_page_machines_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (object);
  return_if_disposed_self ();
  switch (property_id) {
    case MAIN_PAGE_MACHINES_GRID_DENSITY:
      self->grid_density = g_value_get_enum (value);
      gtk_widget_queue_draw (GTK_WIDGET (self->grid_canvas));
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
  return_if_disposed_self ();
  self->dispose_has_run = TRUE;
  GST_DEBUG ("!!!! self=%p", self);
  GST_DEBUG ("  unrefing popups");


  gtk_widget_dispose_template (GTK_WIDGET (self), BT_TYPE_MAIN_PAGE_MACHINES);
  
  g_object_try_unref (self->wire_gain);
  if (self->vol_popup) {
    bt_volume_popup_hide (self->vol_popup);
    g_object_unref (self->vol_popup);
  }
  g_object_try_unref (self->wire_pan);
  if (self->pan_popup) {
    bt_panorama_popup_hide (self->pan_popup);
    g_object_unref (self->pan_popup);
  }
  g_object_try_unref (self->setup);
  g_object_unref (self->change_log);
  g_object_unref (self->app);
  g_object_unref (self->context_menu);
#if 0 /// GTK4
  gtk_widget_destroy (GTK_WIDGET (self->grid_density_menu));
  g_object_unref (self->grid_density_menu);
#endif

  g_clear_object (&self->machine_menu);
  
  GST_DEBUG ("  chaining up");
  G_OBJECT_CLASS (bt_main_page_machines_parent_class)->dispose (object);
}

static void
bt_main_page_machines_finalize (GObject * object)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES (object);
  g_hash_table_destroy (self->machines);
  g_hash_table_destroy (self->wires);
  G_OBJECT_CLASS (bt_main_page_machines_parent_class)->finalize (object);
}

static void
bt_main_page_machines_init (BtMainPageMachines * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
  
  self = bt_main_page_machines_get_instance_private(self);
  GST_DEBUG ("!!!! self=%p", self);
  self->app = bt_edit_application_new ();
  self->zoom = 1.0;
  self->grid_density = 1;
  self->machines = g_hash_table_new (NULL, NULL);
  self->wires = g_hash_table_new (NULL, NULL);
  // initial size
  self->view_w = MACHINE_VIEW_W;
  self->view_h = MACHINE_VIEW_H;
  // the undo/redo changelogger
  self->change_log = bt_change_log_new ();
  bt_change_log_register (self->change_log, BT_CHANGE_LOGGER (self));

  bt_main_page_machines_init_ui (self);
}

static void
bt_main_page_machines_class_init (BtMainPageMachinesClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  gobject_class->get_property = bt_main_page_machines_get_property;
  gobject_class->set_property = bt_main_page_machines_set_property;
  gobject_class->dispose = bt_main_page_machines_dispose;
  gobject_class->finalize = bt_main_page_machines_finalize;
  widget_class->focus = bt_main_page_machines_focus;

#if 0 /// GTK4
  g_object_class_install_property (gobject_class,
      MAIN_PAGE_MACHINES_CANVAS, g_param_spec_object ("canvas",
          "canvas prop", "Get the machine canvas", CLUTTER_TYPE_ACTOR,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
#endif
  g_object_class_install_property (gobject_class,
      MAIN_PAGE_MACHINES_GRID_DENSITY, g_param_spec_enum ("grid-density",
          "grid density", "Modify the grid density view setting",
          bt_main_page_machines_grid_density_get_type (),
          BT_MAIN_PAGE_MACHINES_GRID_DENSITY_LOW,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  
  gtk_widget_class_set_template_from_resource (widget_class,
      "/org/buzztrax/ui/main-page-machines.ui");

  gtk_widget_class_bind_template_child (widget_class, BtMainPageMachines,
      canvas);
  gtk_widget_class_bind_template_child (widget_class, BtMainPageMachines,
      grid_canvas);
  gtk_widget_class_bind_template_child (widget_class, BtMainPageMachines,
      machine_menu_button);
  gtk_widget_class_bind_template_child (widget_class, BtMainPageMachines,
      scrolled_window);
  gtk_widget_class_bind_template_child (widget_class, BtMainPageMachines,
      zoom_in);
  gtk_widget_class_bind_template_child (widget_class, BtMainPageMachines,
      zoom_out);

  gtk_widget_class_bind_template_callback (widget_class,
      on_canvas_size_changed);
  gtk_widget_class_bind_template_callback (widget_class,
      on_canvas_query_tooltip);
  
  gtk_widget_class_install_action (widget_class, "main-page-machines.best-fit",
      NULL, on_toolbar_zoom_fit_clicked);
  gtk_widget_class_install_action (widget_class, "main-page-machines.zoom-in",
      NULL, on_toolbar_zoom_in_clicked);
  gtk_widget_class_install_action (widget_class, "main-page-machines.zoom-out",
      NULL, on_toolbar_zoom_out_clicked);
  gtk_widget_class_install_action (widget_class, "main-page-machines.menu-show",
      NULL, on_toolbar_menu_show_clicked);
  
  gtk_widget_class_install_property_action (widget_class,
      "main-page-machines.grid-density", "grid-density");

  gtk_widget_class_install_action (widget_class,  
      "machine.add", "(sidd)", bt_main_page_machines_on_machine_add);
//    { "machine.clone", NULL, "(sdd)", NULL, NULL },
}
