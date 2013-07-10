/*
 * test to use clutter for a gnome-canvas replacement
 *
 * gcc -Wall -g cluttercanvas.c -o cluttercanvas `pkg-config clutter-1.0 clutter-gtk-1.0 gtk+-3.0 --cflags  --libs`
 *
 * DONE:
 * - scrollable canvas
 *   - adjusts scrollbars on resize
 *   - center canvas when scrollarea > canvas
*      - or make canvas infinite? - otherwise zoom-out is less useful
 * - background grid
 *   - background color from theme
 * - machine icon
 *   - can be dragged around
 *   - becomes transparent while dragging
 * - draw wires
 *   - wire get updated with movement
 * - zooming
 *   - adjusts scrollbars on zoom
 * - panning by dragging the background
 *
 * TODO:
 * - zooming
 *   - pick better resolution for icon images
 * - draw wires
 *   - need directional marker
 * - context menu on items/stage
 *
 * BUGS:
 * - 702510 - ClutterDragAction does not work for nested objects
 */

#include <gtk/gtk.h>
#include <clutter/clutter.h>
#include <clutter-gtk/clutter-gtk.h>
#include <math.h>
#include <stdlib.h>

static GtkIconTheme *it = NULL;
static GtkAdjustment *h_adjustment = NULL, *v_adjustment = NULL;
static ClutterActor *stage = NULL;
static ClutterActor *canvas = NULL;
static ClutterActor *grid = NULL;
static ClutterActor *icon_s = NULL, *icon_m = NULL;
static ClutterActor *wire = NULL;

#define WIDTH 640.0
#define HEIGHT 480.0

/* toolbar actions */

static gdouble zoom = 1.0;
static gfloat x_off = 0.0, y_off = 0.0;

static void
update_scrolled_window (void)
{
  gdouble rw = WIDTH * zoom, rh = HEIGHT * zoom;
  gdouble w = gtk_adjustment_get_page_size (h_adjustment);
  gdouble h = gtk_adjustment_get_page_size (v_adjustment);

  // if visible area > canvas size, center canvas
  clutter_actor_set_width (stage, MAX (w, rw));
  clutter_actor_set_height (stage, MAX (h, rh));
  x_off = w > rw ? (w - rw) / 2.0 : 0.0;
  y_off = h > rh ? (h - rh) / 2.0 : 0.0;

  gtk_adjustment_set_upper (h_adjustment, rw);
  gtk_adjustment_changed (h_adjustment);
  gtk_adjustment_value_changed (h_adjustment);
  gtk_adjustment_set_upper (v_adjustment, rh);
  gtk_adjustment_changed (v_adjustment);
  gtk_adjustment_value_changed (v_adjustment);
}

static void
update_wire (ClutterActor * wire)
{
  gfloat ms_x, ms_y, mm_x, mm_y, w, h, x, y;
  clutter_actor_get_position (icon_s, &ms_x, &ms_y);
  clutter_actor_get_position (icon_m, &mm_x, &mm_y);
  w = 1.0 + fabs (mm_x - ms_x);
  h = 1.0 + fabs (mm_y - ms_y);
  x = 32.0 + MIN (mm_x, ms_x);
  y = 32.0 + MIN (mm_y, ms_y);

  ClutterContent *canvas = clutter_actor_get_content (wire);
  clutter_canvas_set_size (CLUTTER_CANVAS (canvas), w, h);

  clutter_actor_set_size (wire, w, h);
  clutter_actor_set_position (wire, x, y);
}

static gboolean
on_zoom_in_clicked (GtkButton * button, gpointer user_data)
{
  zoom *= 1.5;
  clutter_actor_set_scale (canvas, zoom, zoom);
  update_scrolled_window ();
  return TRUE;
}

static gboolean
on_zoom_out_clicked (GtkButton * button, gpointer user_data)
{
  zoom /= 1.5;
  clutter_actor_set_scale (canvas, zoom, zoom);
  update_scrolled_window ();
  return TRUE;
}

/* gtk events */

static void
on_view_size_changed (GtkWidget * widget, GdkRectangle * allocation,
    gpointer user_data)
{
  // page size is how much of the scrollable part is visible
  gtk_adjustment_set_page_size (h_adjustment, allocation->width);
  gtk_adjustment_set_page_size (v_adjustment, allocation->height);
  update_scrolled_window ();
}

/* actor actions */

static gfloat ptr_x = 0.0, ptr_y = 0.0;
static gfloat obj_x = 0.0, obj_y = 0.0;
static gboolean icon_drag = FALSE;
static ClutterActor *current_icon = NULL;

static gboolean
on_icon_button_press (ClutterActor * actor, ClutterEvent * event,
    gpointer user_data)
{
  clutter_actor_set_child_above_sibling (canvas, actor, NULL);
  clutter_actor_set_opacity (actor, 200);
  icon_drag = TRUE;
  clutter_event_get_coords (event, &ptr_x, &ptr_y);
  clutter_actor_get_position (actor, &obj_x, &obj_y);
  current_icon = actor;
  return TRUE;
}

static gboolean
on_icon_button_release (ClutterActor * actor, ClutterEvent * event,
    gpointer user_data)
{
  clutter_actor_set_opacity (actor, 255);
  icon_drag = FALSE;
  current_icon = NULL;
  return TRUE;
}

static gboolean
on_icon_motion (ClutterActor * actor, ClutterEvent * event, gpointer user_data)
{
  if (icon_drag) {
    ClutterModifierType mod = ((ClutterMotionEvent *) event)->modifier_state;
    if (!(mod & CLUTTER_BUTTON1_MASK)) {
      return on_icon_button_release (actor, event, user_data);
    }

    gfloat x, y, pos_x, pos_y;
    clutter_event_get_coords (event, &x, &y);
    pos_x = obj_x + ((x - ptr_x) / zoom);
    pos_y = obj_y + ((y - ptr_y) / zoom);
    pos_x = CLAMP (pos_x, 0.0, WIDTH - 64.0);
    pos_y = CLAMP (pos_y, 0.0, HEIGHT - 64.0);
    clutter_actor_set_position (actor, pos_x, pos_y);
    update_wire (wire);
  }
  return TRUE;
}

static gboolean canvas_drag = FALSE;

static gboolean
on_canvas_button_press (ClutterActor * actor, ClutterEvent * event,
    gpointer user_data)
{
  canvas_drag = TRUE;
  clutter_event_get_coords (event, &ptr_x, &ptr_y);
  clutter_actor_get_position (actor, &obj_x, &obj_y);
  return TRUE;
}

static gboolean
on_canvas_button_release (ClutterActor * actor, ClutterEvent * event,
    gpointer user_data)
{
  canvas_drag = FALSE;
  return TRUE;
}

static gboolean
on_canvas_motion (ClutterActor * actor, ClutterEvent * event,
    gpointer user_data)
{
  if (canvas_drag) {
    ClutterModifierType mod = ((ClutterMotionEvent *) event)->modifier_state;
    if (!(mod & CLUTTER_BUTTON1_MASK)) {
      return on_canvas_button_release (actor, event, user_data);
    }

    gfloat x, y, pos_x, pos_y;
    clutter_event_get_coords (event, &x, &y);
    pos_x = obj_x + (x - ptr_x);
    pos_y = obj_y + (y - ptr_y);
    if (x_off > 0.0) {
      pos_x = x_off;
    } else {
      gfloat m = (WIDTH * zoom) - gtk_adjustment_get_page_size (h_adjustment);

      pos_x = CLAMP (pos_x, -m, 0.0);
      gtk_adjustment_set_value (h_adjustment, -pos_x);
      gtk_adjustment_value_changed (h_adjustment);
    }
    if (y_off > 0.0) {
      pos_y = y_off;
    } else {
      gfloat m = (HEIGHT * zoom) - gtk_adjustment_get_page_size (v_adjustment);

      pos_y = CLAMP (pos_y, -m, 0.0);
      gtk_adjustment_set_value (v_adjustment, -pos_y);
      gtk_adjustment_value_changed (v_adjustment);
    }

    clutter_actor_set_position (actor, pos_x, pos_y);
  }
  if (icon_drag) {
    return on_icon_motion (current_icon, event, user_data);
  }
  return TRUE;
}


static void
on_v_scroll (GtkAdjustment * adjustment, gpointer user_data)
{
  clutter_actor_set_y (canvas, y_off - gtk_adjustment_get_value (adjustment));
}

static void
on_h_scroll (GtkAdjustment * adjustment, gpointer user_data)
{
  clutter_actor_set_x (canvas, x_off - gtk_adjustment_get_value (adjustment));
}

static void
cairo_clear (cairo_t * cr)
{
  /* clear the contents of the canvas, to not paint over the previous frame */
  cairo_save (cr);
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);
  cairo_restore (cr);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
}

static gboolean
on_grid_draw (ClutterCanvas * canvas, cairo_t * cr, gint width, gint height)
{
  gfloat i;

  cairo_clear (cr);
  /* scale the modelview to the size of the surface */
  cairo_scale (cr, width, height);
  cairo_translate (cr, 0.5, 0.5);

  cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_width (cr, 0.002 / zoom);      // make relative to zoom
  cairo_set_source_rgba (cr, 0.5, 0.5, 0.5, 1.0);

  for (i = -0.5; i <= 0.5; i += 0.25) {
    cairo_move_to (cr, -0.5, i);
    cairo_line_to (cr, 0.5, i);
    cairo_stroke (cr);
    cairo_move_to (cr, i, -0.5);
    cairo_line_to (cr, i, 0.5);
    cairo_stroke (cr);
  }

  return TRUE;
}

static gboolean
on_wire_draw (ClutterCanvas * canvas, cairo_t * cr, gint width, gint height)
{
  gfloat ms_x, ms_y, mm_x, mm_y;
  gfloat x1, x2, y1, y2;

  clutter_actor_get_position (icon_s, &ms_x, &ms_y);
  clutter_actor_get_position (icon_m, &mm_x, &mm_y);
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


  cairo_clear (cr);

  cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_width (cr, 1.0);
  cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0);

  cairo_move_to (cr, x1, y1);
  cairo_line_to (cr, x2, y2);
  cairo_stroke (cr);

  return TRUE;
}

/* helper */

static ClutterActor *
make_machine (gchar * icon_name, gfloat x, gfloat y)
{
  GdkPixbuf *pixbuf = gtk_icon_theme_load_icon (it, icon_name, 64,
      GTK_ICON_LOOKUP_FORCE_SVG | GTK_ICON_LOOKUP_FORCE_SIZE, NULL);
  ClutterContent *image = clutter_image_new ();
  if (pixbuf) {
    clutter_image_set_data (CLUTTER_IMAGE (image),
        gdk_pixbuf_get_pixels (pixbuf), gdk_pixbuf_get_has_alpha (pixbuf)
        ? COGL_PIXEL_FORMAT_RGBA_8888
        : COGL_PIXEL_FORMAT_RGB_888,
        gdk_pixbuf_get_width (pixbuf),
        gdk_pixbuf_get_height (pixbuf), gdk_pixbuf_get_rowstride (pixbuf),
        NULL);
    g_object_unref (pixbuf);
  } else {
    guchar dummy[3] = { 0xff, 0x7f, 0x3f };
    clutter_image_set_data (CLUTTER_IMAGE (image), dummy,
        COGL_PIXEL_FORMAT_RGB_888, 1, 1, 1, NULL);
  }

  ClutterActor *icon = clutter_actor_new ();
  clutter_actor_set_size (icon, 64.0, 64.0);
  clutter_actor_set_content_scaling_filters (icon,
      CLUTTER_SCALING_FILTER_TRILINEAR, CLUTTER_SCALING_FILTER_LINEAR);
  clutter_actor_set_reactive (icon, TRUE);
  clutter_actor_set_position (icon, x, y);
  clutter_actor_set_content (icon, image);
  g_object_unref (image);

  g_signal_connect (icon, "button-press-event",
      G_CALLBACK (on_icon_button_press), NULL);
  g_signal_connect (icon, "button-release-event",
      G_CALLBACK (on_icon_button_release), NULL);
  g_signal_connect (icon, "motion-event", G_CALLBACK (on_icon_motion), NULL);

  return icon;
}

static ClutterActor *
make_background_grid (void)
{
  ClutterContent *canvas = clutter_canvas_new ();
  clutter_canvas_set_size (CLUTTER_CANVAS (canvas), WIDTH, HEIGHT);

  ClutterActor *grid = clutter_actor_new ();
  clutter_actor_set_size (grid, WIDTH, HEIGHT);
  clutter_actor_set_content (grid, canvas);
  clutter_actor_set_content_scaling_filters (grid,
      CLUTTER_SCALING_FILTER_TRILINEAR, CLUTTER_SCALING_FILTER_LINEAR);

  g_signal_connect (canvas, "draw", G_CALLBACK (on_grid_draw), NULL);
  /* invalidate the canvas, so that we can draw before the main loop starts */
  clutter_content_invalidate (canvas);
  return grid;
}

static ClutterActor *
make_wire (void)
{
  ClutterContent *canvas = clutter_canvas_new ();

  ClutterActor *wire = clutter_actor_new ();
  clutter_actor_set_content (wire, canvas);
  clutter_actor_set_content_scaling_filters (wire,
      CLUTTER_SCALING_FILTER_TRILINEAR, CLUTTER_SCALING_FILTER_LINEAR);

  update_wire (wire);

  g_signal_connect (canvas, "draw", G_CALLBACK (on_wire_draw), NULL);
  /* invalidate the canvas, so that we can draw before the main loop starts */
  clutter_content_invalidate (canvas);
  return wire;
}

/* main */

gint
main (gint argc, gchar * argv[])
{
  GtkWidget *button;

  if (gtk_clutter_init (&argc, &argv) != CLUTTER_INIT_SUCCESS)
    return 1;

  it = gtk_icon_theme_get_default ();

  /* Create the window and some child widgets: */
  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_size_request (window, 200, 200);
  /* Stop the application when the window is closed: */
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

  GtkWidget *vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  /* tool bar */
  GtkWidget *hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  button = gtk_button_new_with_label ("Zoom In");
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked", G_CALLBACK (on_zoom_in_clicked), NULL);

  button = gtk_button_new_with_label ("Zoom Out");
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked", G_CALLBACK (on_zoom_out_clicked), NULL);

  /* Create a table to hold the scrollbars and the ClutterEmbed widget: */
  GtkWidget *table = gtk_table_new (2, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  /* Create the clutter widget: */
  GtkWidget *clutter_widget = gtk_clutter_embed_new ();
  gtk_table_attach (GTK_TABLE (table), clutter_widget,
      0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (clutter_widget);
  g_signal_connect (clutter_widget, "size-allocate",
      G_CALLBACK (on_view_size_changed), NULL);

  /* Get the stage and set its color: */
  stage = gtk_clutter_embed_get_stage (GTK_CLUTTER_EMBED (clutter_widget));
  GtkStyle *style = gtk_widget_get_style (clutter_widget);
  GdkColor *c = &style->bg[GTK_STATE_NORMAL];
  ClutterColor stage_color = { c->red >> 8, c->green >> 8, c->blue >> 8, 0xff };
  clutter_stage_set_color (CLUTTER_STAGE (stage), &stage_color);
  clutter_actor_set_size (stage, WIDTH, HEIGHT);

  /* Add canvas (root group) 
   * we're using this for scrolling
   * in clutter 1.12 we shoud use ClutterScrollActor
   */
  canvas = clutter_actor_new ();
  clutter_actor_set_reactive (canvas, TRUE);
  clutter_actor_set_size (canvas, WIDTH, HEIGHT);
  clutter_actor_set_position (canvas, 0.0, 0.0);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), canvas);
  g_signal_connect (canvas, "button-press-event",
      G_CALLBACK (on_canvas_button_press), NULL);
  g_signal_connect (canvas, "button-release-event",
      G_CALLBACK (on_canvas_button_release), NULL);
  g_signal_connect (canvas, "motion-event", G_CALLBACK (on_canvas_motion),
      NULL);

  /* Add a background grid */
  grid = make_background_grid ();
  clutter_container_add_actor (CLUTTER_CONTAINER (canvas), grid);

  /* Add two machines */
  icon_s = make_machine ("buzztrax_generator", 0.0, 0.0);
  clutter_container_add_actor (CLUTTER_CONTAINER (canvas), icon_s);
  icon_m = make_machine ("buzztrax_master", 100.0, 100.0);
  clutter_container_add_actor (CLUTTER_CONTAINER (canvas), icon_m);

  /* Add the wire */
  wire = make_wire ();
  clutter_container_add_actor (CLUTTER_CONTAINER (canvas), wire);
  clutter_actor_set_child_below_sibling (canvas, wire, NULL);

  /* Show all content */
  clutter_actor_show_all (stage);

  /* Create scrollbars and connect them */
  GtkWidget *scrollbar;

  v_adjustment = gtk_adjustment_new (0.0, 0.0, HEIGHT, 1.0, 10.0, 10.0);
  g_signal_connect (v_adjustment, "value-changed", G_CALLBACK (on_v_scroll),
      NULL);
  scrollbar = gtk_scrollbar_new (GTK_ORIENTATION_VERTICAL, v_adjustment);
  gtk_table_attach (GTK_TABLE (table), scrollbar, 1, 2, 0, 1, 0, 0, 0, 0);

  h_adjustment = gtk_adjustment_new (0.0, 0.0, WIDTH, 1.0, 10.0, 10.0);
  g_signal_connect (h_adjustment, "value-changed", G_CALLBACK (on_h_scroll),
      NULL);
  scrollbar = gtk_scrollbar_new (GTK_ORIENTATION_HORIZONTAL, h_adjustment);
  gtk_table_attach (GTK_TABLE (table), scrollbar, 0, 1, 1, 2, 0L, 0, 0, 0);

  /* Show the window and start the main-loop */
  gtk_widget_show_all (GTK_WIDGET (window));
  gtk_main ();

  return EXIT_SUCCESS;
}
