/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */
/* Modified by the buzztrax developers due to:
 * BUG(726795) and BUG(730730), unlikely to be fixed quickly
 */

#include "config.h"

#include <math.h>

#include "gtkscrolledsyncwindow.h"
#include "marshal.h"

/**
 * SECTION:gtkscrolledsyncwindow
 * @Short_description: Adds scrolling to its child widget
 * @Title: GtkScrolledSyncWindow
 * @See_also: #GtkScrollable, #GtkViewport, #GtkAdjustment
 *
 * #GtkScrolledSyncWindow is a copy of GtkScrolledWindow that can be used as
 * a companion widget. For this one will share some or all of the adjustments
 * of the master widget with this widget. The #GtkScrolledSyncWindow will never
 * show scrollbars, instead it will follow the scrollbars of the master widget.
 */

#define TOUCH_BYPASS_CAPTURED_THRESHOLD 30

/* Kinetic scrolling */
#define FRAME_INTERVAL (1000 / 60)
#define MAX_OVERSHOOT_DISTANCE 50
#define FRICTION_DECELERATION 0.003
#define OVERSHOOT_INVERSE_ACCELERATION 0.003
#define RELEASE_EVENT_TIMEOUT 1000

struct _GtkScrolledSyncWindowPrivate
{
  GtkAdjustment *hadjustment;
  GtkAdjustment *vadjustment;

  guint focus_out:1;            /* Flag used by ::move-focus-out implementation */

  /* Kinetic scrolling */
  GdkEvent *button_press_event;
  GdkWindow *overshoot_window;
  GdkSeat *drag_seat;
  guint kinetic_scrolling:1;
  guint capture_button_press:1;
  guint in_drag:1;
  guint last_button_event_valid:1;

  guint release_timeout_id;
  guint deceleration_id;

  gdouble last_button_event_x_root;
  gdouble last_button_event_y_root;

  gdouble last_motion_event_x_root;
  gdouble last_motion_event_y_root;
  guint32 last_motion_event_time;

  gdouble x_velocity;
  gdouble y_velocity;

  gdouble unclamped_hadj_value;
  gdouble unclamped_vadj_value;
};

typedef struct
{
  GtkScrolledSyncWindow *scrolled_window;
  gint64 last_deceleration_time;

  gdouble x_velocity;
  gdouble y_velocity;
  gdouble vel_cosine;
  gdouble vel_sine;
} KineticScrollData;

enum
{
  PROP_0,
  PROP_HADJUSTMENT,
  PROP_VADJUSTMENT,
  PROP_KINETIC_SCROLLING
};

/* Signals */
enum
{
  SCROLL_CHILD,
  MOVE_FOCUS_OUT,
  LAST_SIGNAL
};

static void gtk_scrolled_sync_window_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gtk_scrolled_sync_window_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);

static void gtk_scrolled_sync_window_destroy (GtkWidget * widget);
static gboolean gtk_scrolled_sync_window_draw (GtkWidget * widget,
    cairo_t * cr);
static void gtk_scrolled_sync_window_size_allocate (GtkWidget * widget,
    GtkAllocation * allocation);
static gboolean gtk_scrolled_sync_window_scroll_event (GtkWidget * widget,
    GdkEventScroll * event);
static gboolean gtk_scrolled_sync_window_captured_event (GtkWidget * widget,
    GdkEvent * event);
static gboolean gtk_scrolled_sync_window_focus (GtkWidget * widget,
    GtkDirectionType direction);
static void gtk_scrolled_sync_window_add (GtkContainer * container,
    GtkWidget * widget);
static void gtk_scrolled_sync_window_remove (GtkContainer * container,
    GtkWidget * widget);
static gboolean gtk_scrolled_sync_window_scroll_child (GtkScrolledSyncWindow *
    scrolled_sync_window, GtkScrollType scroll, gboolean horizontal);
static void gtk_scrolled_sync_window_move_focus_out (GtkScrolledSyncWindow *
    scrolled_sync_window, GtkDirectionType direction_type);

static void gtk_scrolled_sync_window_adjustment_value_changed (GtkAdjustment *
    adjustment, gpointer data);

static void gtk_scrolled_sync_window_get_preferred_width (GtkWidget * widget,
    gint * minimum_size, gint * natural_size);
static void gtk_scrolled_sync_window_get_preferred_height (GtkWidget * widget,
    gint * minimum_size, gint * natural_size);
static void gtk_scrolled_sync_window_get_preferred_height_for_width (GtkWidget *
    layout, gint width, gint * minimum_height, gint * natural_height);
static void gtk_scrolled_sync_window_get_preferred_width_for_height (GtkWidget *
    layout, gint width, gint * minimum_height, gint * natural_height);

static void gtk_scrolled_sync_window_realize (GtkWidget * widget);
static void gtk_scrolled_sync_window_unrealize (GtkWidget * widget);
static void gtk_scrolled_sync_window_map (GtkWidget * widget);
static void gtk_scrolled_sync_window_unmap (GtkWidget * widget);

static void gtk_scrolled_sync_window_grab_notify (GtkWidget * widget,
    gboolean was_grabbed);

static gboolean
_gtk_scrolled_sync_window_set_adjustment_value (GtkScrolledSyncWindow * self,
    GtkAdjustment * adjustment, gdouble value, gboolean allow_overshooting,
    gboolean snap_to_border);

static void gtk_scrolled_sync_window_cancel_deceleration (GtkScrolledSyncWindow
    * self);

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (GtkScrolledSyncWindow, gtk_scrolled_sync_window, GTK_TYPE_BIN)

     static void
         add_scroll_binding (GtkBindingSet * binding_set,
    guint keyval,
    GdkModifierType mask, GtkScrollType scroll, gboolean horizontal)
{
  guint keypad_keyval = keyval - GDK_KEY_Left + GDK_KEY_KP_Left;

  gtk_binding_entry_add_signal (binding_set, keyval, mask,
      "scroll-child", 2,
      GTK_TYPE_SCROLL_TYPE, scroll, G_TYPE_BOOLEAN, horizontal);
  gtk_binding_entry_add_signal (binding_set, keypad_keyval, mask,
      "scroll-child", 2,
      GTK_TYPE_SCROLL_TYPE, scroll, G_TYPE_BOOLEAN, horizontal);
}

static void
add_tab_bindings (GtkBindingSet * binding_set,
    GdkModifierType modifiers, GtkDirectionType direction)
{
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_Tab, modifiers,
      "move-focus-out", 1, GTK_TYPE_DIRECTION_TYPE, direction);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Tab, modifiers,
      "move-focus-out", 1, GTK_TYPE_DIRECTION_TYPE, direction);
}

static void
gtk_scrolled_sync_window_class_init (GtkScrolledSyncWindowClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = (GtkWidgetClass *) klass;
  GtkContainerClass *container_class = (GtkContainerClass *) klass;
  GtkBindingSet *binding_set;

  g_type_class_add_private (klass, sizeof (GtkScrolledSyncWindowPrivate));

  gobject_class->set_property = gtk_scrolled_sync_window_set_property;
  gobject_class->get_property = gtk_scrolled_sync_window_get_property;

  widget_class->destroy = gtk_scrolled_sync_window_destroy;
  widget_class->draw = gtk_scrolled_sync_window_draw;
  widget_class->size_allocate = gtk_scrolled_sync_window_size_allocate;
  widget_class->scroll_event = gtk_scrolled_sync_window_scroll_event;
  widget_class->focus = gtk_scrolled_sync_window_focus;
  widget_class->get_preferred_width =
      gtk_scrolled_sync_window_get_preferred_width;
  widget_class->get_preferred_height =
      gtk_scrolled_sync_window_get_preferred_height;
  widget_class->get_preferred_height_for_width =
      gtk_scrolled_sync_window_get_preferred_height_for_width;
  widget_class->get_preferred_width_for_height =
      gtk_scrolled_sync_window_get_preferred_width_for_height;
  widget_class->realize = gtk_scrolled_sync_window_realize;
  widget_class->unrealize = gtk_scrolled_sync_window_unrealize;
  widget_class->map = gtk_scrolled_sync_window_map;
  widget_class->unmap = gtk_scrolled_sync_window_unmap;
  widget_class->grab_notify = gtk_scrolled_sync_window_grab_notify;

  container_class->add = gtk_scrolled_sync_window_add;
  container_class->remove = gtk_scrolled_sync_window_remove;
  gtk_container_class_handle_border_width (container_class);

  klass->scroll_child = gtk_scrolled_sync_window_scroll_child;
  klass->move_focus_out = gtk_scrolled_sync_window_move_focus_out;

  g_object_class_install_property (gobject_class,
      PROP_HADJUSTMENT,
      g_param_spec_object ("hadjustment",
          "Horizontal Adjustment",
          "The GtkAdjustment for the horizontal position",
          GTK_TYPE_ADJUSTMENT,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class,
      PROP_VADJUSTMENT,
      g_param_spec_object ("vadjustment",
          "Vertical Adjustment",
          "The GtkAdjustment for the vertical position",
          GTK_TYPE_ADJUSTMENT,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  /**
   * GtkScrolledSyncWindow:kinetic-scrolling:
   *
   * The kinetic scrolling behavior flags. Kinetic scrolling
   * only applies to devices with source %GDK_SOURCE_TOUCHSCREEN
   */
  g_object_class_install_property (gobject_class,
      PROP_KINETIC_SCROLLING,
      g_param_spec_boolean ("kinetic-scrolling",
          "Kinetic Scrolling",
          "Kinetic scrolling mode.",
          TRUE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  /**
   * GtkScrolledSyncWindow::scroll-child:
   * @scrolled_sync_window: a #GtkScrolledSyncWindow
   * @scroll: a #GtkScrollType describing how much to scroll
   * @horizontal: whether the keybinding scrolls the child
   *   horizontally or not
   *
   * The ::scroll-child signal is a
   * <link linkend="keybinding-signals">keybinding signal</link>
   * which gets emitted when a keybinding that scrolls is pressed.
   * The horizontal or vertical adjustment is updated which triggers a
   * signal that the scrolled windows child may listen to and scroll itself.
   */
  signals[SCROLL_CHILD] =
      g_signal_new ("scroll-child",
      G_TYPE_FROM_CLASS (gobject_class),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET (GtkScrolledSyncWindowClass, scroll_child),
      NULL, NULL,
      bt_marshal_BOOLEAN__ENUM_BOOLEAN,
      G_TYPE_BOOLEAN, 2, GTK_TYPE_SCROLL_TYPE, G_TYPE_BOOLEAN);

  /**
   * GtkScrolledSyncWindow::move-focus-out:
   * @scrolled_sync_window: a #GtkScrolledSyncWindow
   * @direction_type: either %GTK_DIR_TAB_FORWARD or
   *   %GTK_DIR_TAB_BACKWARD
   *
   * The ::move-focus-out signal is a
   * <link linkend="keybinding-signals">keybinding signal</link>
   * which gets emitted when focus is moved away from the scrolled
   * window by a keybinding.
   * The #GtkWidget::move-focus signal is emitted with @direction_type
   * on this scrolled windows toplevel parent in the container hierarchy.
   * The default bindings for this signal are
   * <keycombo><keycap>Tab</keycap><keycap>Ctrl</keycap></keycombo>
   * and
   * <keycombo><keycap>Tab</keycap><keycap>Ctrl</keycap><keycap>Shift</keycap></keycombo>.
   */
  signals[MOVE_FOCUS_OUT] =
      g_signal_new ("move-focus-out",
      G_TYPE_FROM_CLASS (gobject_class),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET (GtkScrolledSyncWindowClass, move_focus_out),
      NULL, NULL,
      g_cclosure_marshal_VOID__ENUM, G_TYPE_NONE, 1, GTK_TYPE_DIRECTION_TYPE);

  binding_set = gtk_binding_set_by_class (klass);

  add_scroll_binding (binding_set, GDK_KEY_Left, GDK_CONTROL_MASK,
      GTK_SCROLL_STEP_BACKWARD, TRUE);
  add_scroll_binding (binding_set, GDK_KEY_Right, GDK_CONTROL_MASK,
      GTK_SCROLL_STEP_FORWARD, TRUE);
  add_scroll_binding (binding_set, GDK_KEY_Up, GDK_CONTROL_MASK,
      GTK_SCROLL_STEP_BACKWARD, FALSE);
  add_scroll_binding (binding_set, GDK_KEY_Down, GDK_CONTROL_MASK,
      GTK_SCROLL_STEP_FORWARD, FALSE);

  add_scroll_binding (binding_set, GDK_KEY_Page_Up, GDK_CONTROL_MASK,
      GTK_SCROLL_PAGE_BACKWARD, TRUE);
  add_scroll_binding (binding_set, GDK_KEY_Page_Down, GDK_CONTROL_MASK,
      GTK_SCROLL_PAGE_FORWARD, TRUE);
  add_scroll_binding (binding_set, GDK_KEY_Page_Up, 0, GTK_SCROLL_PAGE_BACKWARD,
      FALSE);
  add_scroll_binding (binding_set, GDK_KEY_Page_Down, 0,
      GTK_SCROLL_PAGE_FORWARD, FALSE);

  add_scroll_binding (binding_set, GDK_KEY_Home, GDK_CONTROL_MASK,
      GTK_SCROLL_START, TRUE);
  add_scroll_binding (binding_set, GDK_KEY_End, GDK_CONTROL_MASK,
      GTK_SCROLL_END, TRUE);
  add_scroll_binding (binding_set, GDK_KEY_Home, 0, GTK_SCROLL_START, FALSE);
  add_scroll_binding (binding_set, GDK_KEY_End, 0, GTK_SCROLL_END, FALSE);

  add_tab_bindings (binding_set, GDK_CONTROL_MASK, GTK_DIR_TAB_FORWARD);
  add_tab_bindings (binding_set, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
      GTK_DIR_TAB_BACKWARD);
}

static void
gtk_scrolled_sync_window_init (GtkScrolledSyncWindow * self)
{
  GtkScrolledSyncWindowPrivate *priv;

  self->priv = priv =
      G_TYPE_INSTANCE_GET_PRIVATE (self,
      GTK_TYPE_SCROLLED_SYNC_WINDOW, GtkScrolledSyncWindowPrivate);

  gtk_widget_set_has_window (GTK_WIDGET (self), FALSE);
  gtk_widget_set_can_focus (GTK_WIDGET (self), TRUE);

  priv->hadjustment = NULL;
  priv->vadjustment = NULL;

  priv->focus_out = FALSE;

  gtk_scrolled_sync_window_set_kinetic_scrolling (self, TRUE);
  gtk_scrolled_sync_window_set_capture_button_press (self, TRUE);
}

/**
 * gtk_scrolled_sync_window_new:
 * @hadjustment: (allow-none): horizontal adjustment
 * @vadjustment: (allow-none): vertical adjustment
 *
 * Creates a new synced scrolled window.
 *
 * The two arguments are the scrolled window's adjustments; these will be
 * shared with the scrollbars and the child widget to keep the bars in sync
 * with the child. Usually you want to pass %NULL for the adjustments, which
 * will cause the scrolled window to create them for you.
 *
 * Returns: a new synced scrolled window
 */
GtkWidget *
gtk_scrolled_sync_window_new (GtkAdjustment * hadjustment,
    GtkAdjustment * vadjustment)
{
  if (hadjustment)
    g_return_val_if_fail (GTK_IS_ADJUSTMENT (hadjustment), NULL);

  if (vadjustment)
    g_return_val_if_fail (GTK_IS_ADJUSTMENT (vadjustment), NULL);

  return g_object_new (GTK_TYPE_SCROLLED_SYNC_WINDOW,
      "hadjustment", hadjustment, "vadjustment", vadjustment, NULL);
}

/**
 * gtk_scrolled_sync_window_set_hadjustment:
 * @self: a #GtkScrolledSyncWindow
 * @hadjustment: horizontal scroll adjustment
 *
 * Sets the #GtkAdjustment for the horizontal scrollbar.
 */
void
gtk_scrolled_sync_window_set_hadjustment (GtkScrolledSyncWindow * self,
    GtkAdjustment * hadjustment)
{
  GtkScrolledSyncWindowPrivate *priv;
  GtkBin *bin;
  GtkWidget *child;

  g_return_if_fail (GTK_IS_SCROLLED_SYNC_WINDOW (self));
  if (hadjustment)
    g_return_if_fail (GTK_IS_ADJUSTMENT (hadjustment));
  else
    hadjustment = (GtkAdjustment *) g_object_new (GTK_TYPE_ADJUSTMENT, NULL);

  bin = GTK_BIN (self);
  priv = self->priv;

  if (priv->hadjustment == hadjustment)
    return;

  self->priv->hadjustment = g_object_ref (hadjustment);
  g_signal_connect_object (hadjustment,
      "value-changed",
      G_CALLBACK (gtk_scrolled_sync_window_adjustment_value_changed), self, 0);
  gtk_scrolled_sync_window_adjustment_value_changed (hadjustment, self);

  child = gtk_bin_get_child (bin);
  if (GTK_IS_SCROLLABLE (child))
    gtk_scrollable_set_hadjustment (GTK_SCROLLABLE (child), hadjustment);

  g_object_notify (G_OBJECT (self), "hadjustment");
}

/**
 * gtk_scrolled_sync_window_set_vadjustment:
 * @self: a #GtkScrolledSyncWindow
 * @vadjustment: vertical scroll adjustment
 *
 * Sets the #GtkAdjustment for the vertical scrollbar.
 */
void
gtk_scrolled_sync_window_set_vadjustment (GtkScrolledSyncWindow * self,
    GtkAdjustment * vadjustment)
{
  GtkScrolledSyncWindowPrivate *priv;
  GtkBin *bin;
  GtkWidget *child;

  g_return_if_fail (GTK_IS_SCROLLED_SYNC_WINDOW (self));
  if (vadjustment)
    g_return_if_fail (GTK_IS_ADJUSTMENT (vadjustment));
  else
    vadjustment = (GtkAdjustment *) g_object_new (GTK_TYPE_ADJUSTMENT, NULL);

  bin = GTK_BIN (self);
  priv = self->priv;

  if (priv->vadjustment == vadjustment)
    return;

  self->priv->vadjustment = g_object_ref (vadjustment);
  g_signal_connect_object (vadjustment,
      "value-changed",
      G_CALLBACK (gtk_scrolled_sync_window_adjustment_value_changed), self, 0);
  gtk_scrolled_sync_window_adjustment_value_changed (vadjustment, self);

  child = gtk_bin_get_child (bin);
  if (GTK_IS_SCROLLABLE (child))
    gtk_scrollable_set_vadjustment (GTK_SCROLLABLE (child), vadjustment);

  g_object_notify (G_OBJECT (self), "vadjustment");
}

/**
 * gtk_scrolled_sync_window_get_hadjustment:
 * @self: a #GtkScrolledSyncWindow
 *
 * Returns the horizontal adjustment.
 *
 * Returns: (transfer none): the horizontal #GtkAdjustment
 */
GtkAdjustment *
gtk_scrolled_sync_window_get_hadjustment (GtkScrolledSyncWindow * self)
{
  g_return_val_if_fail (GTK_IS_SCROLLED_SYNC_WINDOW (self), NULL);

  return self->priv->hadjustment;
}

/**
 * gtk_scrolled_sync_window_get_vadjustment:
 * @self: a #GtkScrolledSyncWindow
 *
 * Returns the vertical adjustment.
 *
 * Returns: (transfer none): the vertical #GtkAdjustment
 */
GtkAdjustment *
gtk_scrolled_sync_window_get_vadjustment (GtkScrolledSyncWindow * self)
{
  g_return_val_if_fail (GTK_IS_SCROLLED_SYNC_WINDOW (self), NULL);

  return self->priv->vadjustment;
}

/**
 * gtk_scrolled_sync_window_set_kinetic_scrolling:
 * @self: a #GtkScrolledSyncWindow
 * @kinetic_scrolling: %TRUE to enable kinetic scrolling
 *
 * Turns kinetic scrolling on or off.
 * Kinetic scrolling only applies to devices with source
 * %GDK_SOURCE_TOUCHSCREEN.
 **/
void
gtk_scrolled_sync_window_set_kinetic_scrolling (GtkScrolledSyncWindow * self,
    gboolean kinetic_scrolling)
{
  GtkScrolledSyncWindowPrivate *priv;

  g_return_if_fail (GTK_IS_SCROLLED_SYNC_WINDOW (self));

  priv = self->priv;
  if (priv->kinetic_scrolling == kinetic_scrolling)
    return;

  priv->kinetic_scrolling = kinetic_scrolling;
  if (priv->kinetic_scrolling) {
    g_object_set_data (G_OBJECT (self), "captured-event-handler",
        gtk_scrolled_sync_window_captured_event);
  } else {
    g_object_set_data (G_OBJECT (self), "captured-event-handler", NULL);
    if (priv->release_timeout_id) {
      g_source_remove (priv->release_timeout_id);
      priv->release_timeout_id = 0;
    }
    if (priv->deceleration_id) {
      g_source_remove (priv->deceleration_id);
      priv->deceleration_id = 0;
    }
  }
  g_object_notify (G_OBJECT (self), "kinetic-scrolling");
}

/**
 * gtk_scrolled_sync_window_get_kinetic_scrolling:
 * @self: a #GtkScrolledSyncWindow
 *
 * Returns the specified kinetic scrolling behavior.
 *
 * Return value: the scrolling behavior flags.
 */
gboolean
gtk_scrolled_sync_window_get_kinetic_scrolling (GtkScrolledSyncWindow * self)
{
  g_return_val_if_fail (GTK_IS_SCROLLED_SYNC_WINDOW (self), FALSE);

  return self->priv->kinetic_scrolling;
}

/**
 * gtk_scrolled_sync_window_set_capture_button_press:
 * @self: a #GtkScrolledSyncWindow
 * @capture_button_press: %TRUE to capture button presses
 *
 * Changes the behaviour of @self wrt. to the initial
 * event that possibly starts kinetic scrolling. When @capture_button_press
 * is set to %TRUE, the event is captured by the scrolled window, and
 * then later replayed if it is meant to go to the child widget.
 *
 * This should be enabled if any child widgets perform non-reversible
 * actions on #GtkWidget::button-press-event. If they don't, and handle
 * additionally handle #GtkWidget::grab-broken-event, it might be better
 * to set @capture_button_press to %FALSE.
 *
 * This setting only has an effect if kinetic scrolling is enabled.
 */
void
gtk_scrolled_sync_window_set_capture_button_press (GtkScrolledSyncWindow * self,
    gboolean capture_button_press)
{
  g_return_if_fail (GTK_IS_SCROLLED_SYNC_WINDOW (self));

  self->priv->capture_button_press = capture_button_press;
}

/**
 * gtk_scrolled_sync_window_get_capture_button_press:
 * @self: a #GtkScrolledSyncWindow
 *
 * Return whether button presses are captured during kinetic
 * scrolling. See gtk_scrolled_sync_window_set_capture_button_press().
 *
 * Returns: %TRUE if button presses are captured during kinetic scrolling
 */
gboolean
gtk_scrolled_sync_window_get_capture_button_press (GtkScrolledSyncWindow * self)
{
  g_return_val_if_fail (GTK_IS_SCROLLED_SYNC_WINDOW (self), FALSE);

  return self->priv->capture_button_press;
}


static void
gtk_scrolled_sync_window_destroy (GtkWidget * widget)
{
  GtkScrolledSyncWindow *self = GTK_SCROLLED_SYNC_WINDOW (widget);
  GtkScrolledSyncWindowPrivate *priv = self->priv;

  if (priv->hadjustment) {
    g_object_unref (priv->hadjustment);
    priv->hadjustment = NULL;
  }
  if (priv->vadjustment) {
    g_object_unref (priv->vadjustment);
    priv->vadjustment = NULL;
  }

  if (priv->release_timeout_id) {
    g_source_remove (priv->release_timeout_id);
    priv->release_timeout_id = 0;
  }
  if (priv->deceleration_id) {
    g_source_remove (priv->deceleration_id);
    priv->deceleration_id = 0;
  }

  if (priv->button_press_event) {
    gdk_event_free (priv->button_press_event);
    priv->button_press_event = NULL;
  }

  GTK_WIDGET_CLASS (gtk_scrolled_sync_window_parent_class)->destroy (widget);
}

static void
gtk_scrolled_sync_window_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GtkScrolledSyncWindow *self = GTK_SCROLLED_SYNC_WINDOW (object);

  switch (prop_id) {
    case PROP_HADJUSTMENT:
      gtk_scrolled_sync_window_set_hadjustment (self,
          g_value_get_object (value));
      break;
    case PROP_VADJUSTMENT:
      gtk_scrolled_sync_window_set_vadjustment (self,
          g_value_get_object (value));
      break;
    case PROP_KINETIC_SCROLLING:
      gtk_scrolled_sync_window_set_kinetic_scrolling (self,
          g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gtk_scrolled_sync_window_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  GtkScrolledSyncWindow *self = GTK_SCROLLED_SYNC_WINDOW (object);
  GtkScrolledSyncWindowPrivate *priv = self->priv;

  switch (prop_id) {
    case PROP_HADJUSTMENT:
      g_value_set_object (value,
          G_OBJECT (gtk_scrolled_sync_window_get_hadjustment (self)));
      break;
    case PROP_VADJUSTMENT:
      g_value_set_object (value,
          G_OBJECT (gtk_scrolled_sync_window_get_vadjustment (self)));
      break;
    case PROP_KINETIC_SCROLLING:
      g_value_set_boolean (value, priv->kinetic_scrolling);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
gtk_scrolled_sync_window_draw (GtkWidget * widget, cairo_t * cr)
{
  GtkStyleContext *context = gtk_widget_get_style_context (widget);

  gtk_render_background (context, cr,
      0, 0,
      gtk_widget_get_allocated_width (widget),
      gtk_widget_get_allocated_height (widget));

  GTK_WIDGET_CLASS (gtk_scrolled_sync_window_parent_class)->draw (widget, cr);

  return FALSE;
}

static gboolean
gtk_scrolled_sync_window_scroll_child (GtkScrolledSyncWindow * self,
    GtkScrollType scroll, gboolean horizontal)
{
  GtkScrolledSyncWindowPrivate *priv = self->priv;
  GtkAdjustment *adjustment = NULL;

  switch (scroll) {
    case GTK_SCROLL_STEP_UP:
      scroll = GTK_SCROLL_STEP_BACKWARD;
      horizontal = FALSE;
      break;
    case GTK_SCROLL_STEP_DOWN:
      scroll = GTK_SCROLL_STEP_FORWARD;
      horizontal = FALSE;
      break;
    case GTK_SCROLL_STEP_LEFT:
      scroll = GTK_SCROLL_STEP_BACKWARD;
      horizontal = TRUE;
      break;
    case GTK_SCROLL_STEP_RIGHT:
      scroll = GTK_SCROLL_STEP_FORWARD;
      horizontal = TRUE;
      break;
    case GTK_SCROLL_PAGE_UP:
      scroll = GTK_SCROLL_PAGE_BACKWARD;
      horizontal = FALSE;
      break;
    case GTK_SCROLL_PAGE_DOWN:
      scroll = GTK_SCROLL_PAGE_FORWARD;
      horizontal = FALSE;
      break;
    case GTK_SCROLL_PAGE_LEFT:
      scroll = GTK_SCROLL_STEP_BACKWARD;
      horizontal = TRUE;
      break;
    case GTK_SCROLL_PAGE_RIGHT:
      scroll = GTK_SCROLL_STEP_FORWARD;
      horizontal = TRUE;
      break;
    case GTK_SCROLL_STEP_BACKWARD:
    case GTK_SCROLL_STEP_FORWARD:
    case GTK_SCROLL_PAGE_BACKWARD:
    case GTK_SCROLL_PAGE_FORWARD:
    case GTK_SCROLL_START:
    case GTK_SCROLL_END:
      break;
    default:
      g_warning
          ("Invalid scroll type %u for GtkScrolledSyncWindow::scroll-child",
          scroll);
      return FALSE;
  }

  adjustment = horizontal ? priv->hadjustment : priv->vadjustment;
  if (adjustment) {
    gdouble value = gtk_adjustment_get_value (adjustment);

    switch (scroll) {
      case GTK_SCROLL_STEP_FORWARD:
        value += gtk_adjustment_get_step_increment (adjustment);
        break;
      case GTK_SCROLL_STEP_BACKWARD:
        value -= gtk_adjustment_get_step_increment (adjustment);
        break;
      case GTK_SCROLL_PAGE_FORWARD:
        value += gtk_adjustment_get_page_increment (adjustment);
        break;
      case GTK_SCROLL_PAGE_BACKWARD:
        value -= gtk_adjustment_get_page_increment (adjustment);
        break;
      case GTK_SCROLL_START:
        value = gtk_adjustment_get_lower (adjustment);
        break;
      case GTK_SCROLL_END:
        value = gtk_adjustment_get_upper (adjustment);
        break;
      default:
        g_assert_not_reached ();
        break;
    }
    gtk_adjustment_set_value (adjustment, value);
    return TRUE;
  }
  return FALSE;
}

static void
gtk_scrolled_sync_window_move_focus_out (GtkScrolledSyncWindow * self,
    GtkDirectionType direction_type)
{
  GtkScrolledSyncWindowPrivate *priv = self->priv;
  GtkWidget *toplevel;

  /* Focus out of the scrolled window entirely. We do this by setting
   * a flag, then propagating the focus motion to the notebook.
   */
  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (self));
  if (!gtk_widget_is_toplevel (toplevel))
    return;

  g_object_ref (self);

  priv->focus_out = TRUE;
  g_signal_emit_by_name (toplevel, "move-focus", direction_type);
  priv->focus_out = FALSE;

  g_object_unref (self);
}

static gboolean
_gtk_scrolled_sync_window_get_overshoot (GtkScrolledSyncWindow * self,
    gint * overshoot_x, gint * overshoot_y)
{
  GtkScrolledSyncWindowPrivate *priv = self->priv;
  GtkAdjustment *adj;
  gdouble lower, upper, x = 0, y = 0;

  /* Vertical overshoot */
  if (priv->vadjustment) {
    adj = priv->vadjustment;
    lower = gtk_adjustment_get_lower (adj);
    upper = gtk_adjustment_get_upper (adj) - gtk_adjustment_get_page_size (adj);

    if (priv->unclamped_vadj_value < lower)
      y = priv->unclamped_vadj_value - lower;
    else if (priv->unclamped_vadj_value > upper)
      y = priv->unclamped_vadj_value - upper;
  }

  /* Horizontal overshoot */
  if (priv->hadjustment) {
    adj = priv->hadjustment;
    lower = gtk_adjustment_get_lower (adj);
    upper = gtk_adjustment_get_upper (adj) - gtk_adjustment_get_page_size (adj);

    if (priv->unclamped_hadj_value < lower)
      x = priv->unclamped_hadj_value - lower;
    else if (priv->unclamped_hadj_value > upper)
      x = priv->unclamped_hadj_value - upper;
  }

  if (overshoot_x)
    *overshoot_x = x;

  if (overshoot_y)
    *overshoot_y = y;

  return (x != 0 || y != 0);
}

static void
_gtk_scrolled_sync_window_allocate_overshoot_window (GtkScrolledSyncWindow *
    self)
{
  GtkAllocation window_allocation;
  GtkScrolledSyncWindowPrivate *priv = self->priv;
  GtkWidget *widget = GTK_WIDGET (self);
  gint overshoot_x, overshoot_y;

  if (!gtk_widget_get_realized (widget))
    return;

  gtk_widget_get_allocation (widget, &window_allocation);
  _gtk_scrolled_sync_window_get_overshoot (self, &overshoot_x, &overshoot_y);

  /* Handle displacement to the left/top by moving the overshoot
   * window, overshooting to the bottom/right is handled in
   * gtk_scrolled_sync_window_allocate_child()
   */
  if (overshoot_x < 0)
    window_allocation.x += -overshoot_x;

  if (overshoot_y < 0)
    window_allocation.y += -overshoot_y;

  window_allocation.width -= ABS (overshoot_x);
  window_allocation.height -= ABS (overshoot_y);

  gdk_window_move_resize (priv->overshoot_window,
      window_allocation.x, window_allocation.y,
      window_allocation.width, window_allocation.height);
}

static void
gtk_scrolled_sync_window_allocate_child (GtkScrolledSyncWindow * swindow)
{
  GtkWidget *widget = GTK_WIDGET (swindow), *child;
  GtkAllocation allocation;
  GtkAllocation child_allocation;
  gint overshoot_x, overshoot_y;

  child = gtk_bin_get_child (GTK_BIN (widget));

  gtk_widget_get_allocation (widget, &allocation);

  _gtk_scrolled_sync_window_get_overshoot (swindow, &overshoot_x, &overshoot_y);

  /* Handle overshooting to the right/bottom by relocating the
   * widget allocation to negative coordinates, so these edges
   * stick to the overshoot window border.
   */
  if (overshoot_x > 0)
    child_allocation.x = -overshoot_x;
  else
    child_allocation.x = 0;

  if (overshoot_y > 0)
    child_allocation.y = -overshoot_y;
  else
    child_allocation.y = 0;

  child_allocation.width = allocation.width;
  child_allocation.height = allocation.height;

  gtk_widget_size_allocate (child, &child_allocation);
}

static void
gtk_scrolled_sync_window_size_allocate (GtkWidget * widget,
    GtkAllocation * allocation)
{
  GtkScrolledSyncWindow *self;
  GtkStyleContext *context;
  GtkStateFlags state;
  GtkBorder padding, border;
  GtkBin *bin;
  GtkWidget *child;

  g_return_if_fail (GTK_IS_SCROLLED_SYNC_WINDOW (widget));
  g_return_if_fail (allocation != NULL);

  self = GTK_SCROLLED_SYNC_WINDOW (widget);
  bin = GTK_BIN (self);

  context = gtk_widget_get_style_context (widget);
  state = gtk_widget_get_state_flags (widget);

  gtk_style_context_save (context);
  gtk_style_context_add_class (context, GTK_STYLE_CLASS_FRAME);

  gtk_style_context_get_padding (context, state, &padding);
  gtk_style_context_get_border (context, state, &border);

  gtk_widget_set_allocation (widget, allocation);
  gtk_style_context_restore (context);

  child = gtk_bin_get_child (bin);
  if (child && gtk_widget_get_visible (child)) {
    gint child_scroll_width;
    gint child_scroll_height;
    GtkScrollablePolicy hscroll_policy;
    GtkScrollablePolicy vscroll_policy;

    hscroll_policy = GTK_IS_SCROLLABLE (child)
        ? gtk_scrollable_get_hscroll_policy (GTK_SCROLLABLE (child))
        : GTK_SCROLL_MINIMUM;
    vscroll_policy = GTK_IS_SCROLLABLE (child)
        ? gtk_scrollable_get_vscroll_policy (GTK_SCROLLABLE (child))
        : GTK_SCROLL_MINIMUM;

    /* Determine scrollbar visibility first via hfw apis */
    if (gtk_widget_get_request_mode (child) ==
        GTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH) {
      if (hscroll_policy == GTK_SCROLL_MINIMUM)
        gtk_widget_get_preferred_width (child, &child_scroll_width, NULL);
      else
        gtk_widget_get_preferred_width (child, NULL, &child_scroll_width);

      /* First try without a vertical scrollbar if the content will fit the height
       * given the extra width of the scrollbar */
      if (vscroll_policy == GTK_SCROLL_MINIMUM)
        gtk_widget_get_preferred_height_for_width (child,
            MAX (allocation->width, child_scroll_width),
            &child_scroll_height, NULL);
      else
        gtk_widget_get_preferred_height_for_width (child,
            MAX (allocation->width, child_scroll_width),
            NULL, &child_scroll_height);
    } else {                    /* GTK_SIZE_REQUEST_WIDTH_FOR_HEIGHT */
      if (vscroll_policy == GTK_SCROLL_MINIMUM)
        gtk_widget_get_preferred_height (child, &child_scroll_height, NULL);
      else
        gtk_widget_get_preferred_height (child, NULL, &child_scroll_height);

      /* First try without a horizontal scrollbar if the content will fit the width
       * given the extra height of the scrollbar */
      if (hscroll_policy == GTK_SCROLL_MINIMUM)
        gtk_widget_get_preferred_width_for_height (child,
            MAX (allocation->height, child_scroll_height),
            &child_scroll_width, NULL);
      else
        gtk_widget_get_preferred_width_for_height (child,
            MAX (allocation->height, child_scroll_height),
            NULL, &child_scroll_width);
    }

    gtk_scrolled_sync_window_allocate_child (self);
  }

  _gtk_scrolled_sync_window_allocate_overshoot_window (self);
}

static gboolean
gtk_scrolled_sync_window_scroll_event (GtkWidget * widget,
    GdkEventScroll * event)
{
  GtkScrolledSyncWindowPrivate *priv;
  GtkScrolledSyncWindow *self;
  gboolean handled = FALSE;
  gdouble delta_x;
  gdouble delta_y;
  gdouble delta;

  g_return_val_if_fail (GTK_IS_SCROLLED_SYNC_WINDOW (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  self = GTK_SCROLLED_SYNC_WINDOW (widget);
  priv = self->priv;

  if (gdk_event_get_scroll_deltas ((GdkEvent *) event, &delta_x, &delta_y)) {
    if (delta_x != 0.0 && priv->hadjustment) {
      GtkAdjustment *adj = priv->hadjustment;
      gdouble new_value;
      gdouble page_size;
      gdouble scroll_unit;

      page_size = gtk_adjustment_get_page_size (adj);
      scroll_unit = pow (page_size, 2.0 / 3.0);

      new_value = CLAMP (gtk_adjustment_get_value (adj) + delta_x * scroll_unit,
          gtk_adjustment_get_lower (adj),
          gtk_adjustment_get_upper (adj) - gtk_adjustment_get_page_size (adj));

      gtk_adjustment_set_value (adj, new_value);

      handled = TRUE;
    }

    if (delta_y != 0.0 && priv->vadjustment) {
      GtkAdjustment *adj = priv->vadjustment;
      gdouble new_value;
      gdouble page_size;
      gdouble scroll_unit;

      page_size = gtk_adjustment_get_page_size (adj);
      scroll_unit = pow (page_size, 2.0 / 3.0);

      new_value = CLAMP (gtk_adjustment_get_value (adj) + delta_y * scroll_unit,
          gtk_adjustment_get_lower (adj),
          gtk_adjustment_get_upper (adj) - gtk_adjustment_get_page_size (adj));

      gtk_adjustment_set_value (adj, new_value);

      handled = TRUE;
    }
  } else {
    GtkAdjustment *adj;

    if (event->direction == GDK_SCROLL_UP
        || event->direction == GDK_SCROLL_DOWN)
      adj = priv->vadjustment;
    else
      adj = priv->hadjustment;

    if (adj) {
      gdouble new_value, page_size;
      GdkScrollDirection direction = event->direction;

      page_size = gtk_adjustment_get_page_size (adj);
      delta = pow (page_size, 2.0 / 3.0);
      if (direction == GDK_SCROLL_UP || direction == GDK_SCROLL_LEFT)
        delta = -delta;

      new_value = CLAMP (gtk_adjustment_get_value (adj) + delta,
          gtk_adjustment_get_lower (adj),
          gtk_adjustment_get_upper (adj) - page_size);

      gtk_adjustment_set_value (adj, new_value);

      handled = TRUE;
    }
  }

  return handled;
}

static gboolean
_gtk_scrolled_sync_window_set_adjustment_value (GtkScrolledSyncWindow * self,
    GtkAdjustment * adjustment,
    gdouble value, gboolean allow_overshooting, gboolean snap_to_border)
{
  GtkScrolledSyncWindowPrivate *priv = self->priv;
  gdouble lower, upper, *prev_value;

  lower = gtk_adjustment_get_lower (adjustment);
  upper = gtk_adjustment_get_upper (adjustment) -
      gtk_adjustment_get_page_size (adjustment);

  if (adjustment == priv->hadjustment)
    prev_value = &priv->unclamped_hadj_value;
  else if (adjustment == priv->vadjustment)
    prev_value = &priv->unclamped_vadj_value;
  else
    return FALSE;

  if (snap_to_border) {
    if (*prev_value < 0 && value > 0)
      value = 0;
    else if (*prev_value > upper && value < upper)
      value = upper;
  }

  if (allow_overshooting) {
    lower -= MAX_OVERSHOOT_DISTANCE;
    upper += MAX_OVERSHOOT_DISTANCE;
  }

  *prev_value = CLAMP (value, lower, upper);
  gtk_adjustment_set_value (adjustment, value);

  return (*prev_value != value);
}

static gboolean
scrolled_sync_window_deceleration_cb (gpointer user_data)
{
  KineticScrollData *data = user_data;
  GtkScrolledSyncWindow *self = data->scrolled_window;
  GtkScrolledSyncWindowPrivate *priv = self->priv;
  gint old_overshoot_x, old_overshoot_y, overshoot_x, overshoot_y;
  gdouble value;
  gint64 current_time;
  guint elapsed;

  _gtk_scrolled_sync_window_get_overshoot (self,
      &old_overshoot_x, &old_overshoot_y);

  current_time = g_get_monotonic_time ();
  elapsed = (current_time - data->last_deceleration_time) / 1000;
  data->last_deceleration_time = current_time;

  if (priv->hadjustment) {
    value = priv->unclamped_hadj_value + (data->x_velocity * elapsed);

    if (_gtk_scrolled_sync_window_set_adjustment_value (self,
            priv->hadjustment, value, TRUE, TRUE))
      data->x_velocity = 0;
  } else
    data->x_velocity = 0;

  if (priv->vadjustment) {
    value = priv->unclamped_vadj_value + (data->y_velocity * elapsed);

    if (_gtk_scrolled_sync_window_set_adjustment_value (self,
            priv->vadjustment, value, TRUE, TRUE))
      data->y_velocity = 0;
  } else
    data->y_velocity = 0;

  _gtk_scrolled_sync_window_get_overshoot (self, &overshoot_x, &overshoot_y);

  if (overshoot_x == 0) {
    if (old_overshoot_x != 0) {
      /* Overshooting finished snapping back */
      data->x_velocity = 0;
    } else if (data->x_velocity > 0) {
      data->x_velocity -= FRICTION_DECELERATION * elapsed * data->vel_sine;
      data->x_velocity = MAX (0, data->x_velocity);
    } else if (data->x_velocity < 0) {
      data->x_velocity += FRICTION_DECELERATION * elapsed * data->vel_sine;
      data->x_velocity = MIN (0, data->x_velocity);
    }
  } else if (overshoot_x < 0)
    data->x_velocity += OVERSHOOT_INVERSE_ACCELERATION * elapsed;
  else if (overshoot_x > 0)
    data->x_velocity -= OVERSHOOT_INVERSE_ACCELERATION * elapsed;

  if (overshoot_y == 0) {
    if (old_overshoot_y != 0) {
      /* Overshooting finished snapping back */
      data->y_velocity = 0;
    } else if (data->y_velocity > 0) {
      data->y_velocity -= FRICTION_DECELERATION * elapsed * data->vel_cosine;
      data->y_velocity = MAX (0, data->y_velocity);
    } else if (data->y_velocity < 0) {
      data->y_velocity += FRICTION_DECELERATION * elapsed * data->vel_cosine;
      data->y_velocity = MIN (0, data->y_velocity);
    }
  } else if (overshoot_y < 0)
    data->y_velocity += OVERSHOOT_INVERSE_ACCELERATION * elapsed;
  else if (overshoot_y > 0)
    data->y_velocity -= OVERSHOOT_INVERSE_ACCELERATION * elapsed;

  if (old_overshoot_x != overshoot_x || old_overshoot_y != overshoot_y) {
    if (overshoot_x >= 0 || overshoot_y >= 0) {
      /* We need to reallocate the widget to have it at
       * negative offset, so there's a "gravity" on the
       * bottom/right corner
       */
      gtk_widget_queue_resize (GTK_WIDGET (self));
    } else if (overshoot_x < 0 || overshoot_y < 0)
      _gtk_scrolled_sync_window_allocate_overshoot_window (self);
  }

  if (overshoot_x == 0 && overshoot_y == 0 &&
      data->x_velocity == 0 && data->y_velocity == 0)
    gtk_scrolled_sync_window_cancel_deceleration (self);

  return G_SOURCE_CONTINUE;
}

static void
gtk_scrolled_sync_window_cancel_deceleration (GtkScrolledSyncWindow * self)
{
  GtkScrolledSyncWindowPrivate *priv = self->priv;

  if (priv->deceleration_id) {
    g_source_remove (priv->deceleration_id);
    priv->deceleration_id = 0;
  }
}

static void
gtk_scrolled_sync_window_start_deceleration (GtkScrolledSyncWindow * self)
{
  GtkScrolledSyncWindowPrivate *priv = self->priv;
  KineticScrollData *data;
  gdouble angle;

  data = g_new0 (KineticScrollData, 1);
  data->scrolled_window = self;
  data->last_deceleration_time = g_get_monotonic_time ();
  data->x_velocity = priv->x_velocity;
  data->y_velocity = priv->y_velocity;

  /* We use sine/cosine as a factor to deceleration x/y components
   * of the vector, so we care about the sign later.
   */
  angle = atan2 (ABS (data->x_velocity), ABS (data->y_velocity));
  data->vel_cosine = cos (angle);
  data->vel_sine = sin (angle);

  self->priv->deceleration_id =
      gdk_threads_add_timeout_full (G_PRIORITY_DEFAULT,
      FRAME_INTERVAL,
      scrolled_sync_window_deceleration_cb, data, (GDestroyNotify) g_free);
}

static gboolean
gtk_scrolled_sync_window_release_captured_event (GtkScrolledSyncWindow * self)
{
  GtkScrolledSyncWindowPrivate *priv = self->priv;

  /* Cancel the scrolling and send the button press
   * event to the child widget
   */
  if (!priv->button_press_event)
    return FALSE;

  if (priv->drag_seat) {
    gdk_seat_ungrab (priv->drag_seat);
    priv->drag_seat = NULL;
  }

  if (priv->capture_button_press) {
    GtkWidget *event_widget;

    event_widget = gtk_get_event_widget (priv->button_press_event);

    /* internal api :/
       if (!_gtk_propagate_captured_event (event_widget,
       priv->button_press_event,
       gtk_bin_get_child (GTK_BIN (self))))
     */
    gtk_propagate_event (event_widget, priv->button_press_event);

    gdk_event_free (priv->button_press_event);
    priv->button_press_event = NULL;
  }

  if (_gtk_scrolled_sync_window_get_overshoot (self, NULL, NULL))
    gtk_scrolled_sync_window_start_deceleration (self);

  return FALSE;
}

static gboolean
gtk_scrolled_sync_window_calculate_velocity (GtkScrolledSyncWindow * self,
    GdkEvent * event)
{
  GtkScrolledSyncWindowPrivate *priv;
  gdouble x_root, y_root;
  guint32 _time;

#define STILL_THRESHOLD 40

  if (!gdk_event_get_root_coords (event, &x_root, &y_root))
    return FALSE;

  priv = self->priv;
  _time = gdk_event_get_time (event);

  if (priv->last_motion_event_x_root != x_root ||
      priv->last_motion_event_y_root != y_root ||
      (_time - priv->last_motion_event_time) > STILL_THRESHOLD) {
    priv->x_velocity = (priv->last_motion_event_x_root - x_root) /
        (gdouble) (_time - priv->last_motion_event_time);
    priv->y_velocity = (priv->last_motion_event_y_root - y_root) /
        (gdouble) (_time - priv->last_motion_event_time);
  }

  priv->last_motion_event_x_root = x_root;
  priv->last_motion_event_y_root = y_root;
  priv->last_motion_event_time = _time;

#undef STILL_THRESHOLD

  return TRUE;
}

static gboolean
gtk_scrolled_sync_window_captured_button_release (GtkWidget * widget,
    GdkEvent * event)
{
  GtkScrolledSyncWindow *self = GTK_SCROLLED_SYNC_WINDOW (widget);
  GtkScrolledSyncWindowPrivate *priv = self->priv;
  GtkWidget *child;
  gboolean overshoot;
  guint button;
  gdouble x_root, y_root;

  if (gdk_event_get_button (event, &button) && button != 1)
    return FALSE;

  child = gtk_bin_get_child (GTK_BIN (widget));
  if (!child)
    return FALSE;

  gdk_seat_ungrab (priv->drag_seat);
  priv->drag_seat = NULL;

  if (priv->release_timeout_id) {
    g_source_remove (priv->release_timeout_id);
    priv->release_timeout_id = 0;
  }

  overshoot = _gtk_scrolled_sync_window_get_overshoot (self, NULL, NULL);

  if (priv->in_drag)
    gdk_seat_ungrab (gdk_event_get_seat (event));
  else {
    /* There hasn't been scrolling at all, so just let the
     * child widget handle the button press normally
     */
    gtk_scrolled_sync_window_release_captured_event (self);

    if (!overshoot)
      return FALSE;
  }
  priv->in_drag = FALSE;

  if (priv->button_press_event) {
    gdk_event_free (priv->button_press_event);
    priv->button_press_event = NULL;
  }

  gtk_scrolled_sync_window_calculate_velocity (self, event);

  if (priv->x_velocity != 0 || priv->y_velocity != 0 || overshoot) {
    gtk_scrolled_sync_window_start_deceleration (self);
    priv->x_velocity = priv->y_velocity = 0;
    priv->last_button_event_valid = FALSE;
  } else {
    gdk_event_get_root_coords (event, &x_root, &y_root);
    priv->last_button_event_x_root = x_root;
    priv->last_button_event_y_root = y_root;
    priv->last_button_event_valid = TRUE;
  }

  if (priv->capture_button_press)
    return TRUE;
  else
    return FALSE;
}

static gboolean
gtk_scrolled_sync_window_captured_motion_notify (GtkWidget * widget,
    GdkEvent * event)
{
  GtkScrolledSyncWindow *self = GTK_SCROLLED_SYNC_WINDOW (widget);
  GtkScrolledSyncWindowPrivate *priv = self->priv;
  gint old_overshoot_x, old_overshoot_y;
  gint new_overshoot_x, new_overshoot_y;
  GtkWidget *child;
  gdouble dx, dy;
  GdkModifierType state;
  gdouble x_root, y_root;

  gdk_event_get_state (event, &state);
  if (!(state & GDK_BUTTON1_MASK))
    return FALSE;

  child = gtk_bin_get_child (GTK_BIN (widget));
  if (!child)
    return FALSE;

  /* Check if we've passed the drag threshold */
  gdk_event_get_root_coords (event, &x_root, &y_root);
  if (!priv->in_drag) {
    if (gtk_drag_check_threshold (widget,
            priv->last_button_event_x_root,
            priv->last_button_event_y_root, x_root, y_root)) {
      if (priv->release_timeout_id) {
        g_source_remove (priv->release_timeout_id);
        priv->release_timeout_id = 0;
      }

      priv->last_button_event_valid = FALSE;
      priv->in_drag = TRUE;
    } else
      return TRUE;
  }

  gdk_seat_grab (priv->drag_seat,
				 gtk_widget_get_window (widget),
				 GDK_SEAT_CAPABILITY_ALL_POINTING,
				 TRUE,
				 NULL,			 
				 NULL,
				 NULL,
				 NULL);

  priv->last_button_event_valid = FALSE;

  if (priv->button_press_event) {
    gdk_event_free (priv->button_press_event);
    priv->button_press_event = NULL;
  }

  _gtk_scrolled_sync_window_get_overshoot (self,
      &old_overshoot_x, &old_overshoot_y);

  if (priv->hadjustment) {
    dx = (priv->last_motion_event_x_root - x_root) + priv->unclamped_hadj_value;
    _gtk_scrolled_sync_window_set_adjustment_value (self,
        priv->hadjustment, dx, TRUE, FALSE);
  }

  if (priv->vadjustment) {
    dy = (priv->last_motion_event_y_root - y_root) + priv->unclamped_vadj_value;
    _gtk_scrolled_sync_window_set_adjustment_value (self,
        priv->vadjustment, dy, TRUE, FALSE);
  }

  _gtk_scrolled_sync_window_get_overshoot (self,
      &new_overshoot_x, &new_overshoot_y);

  if (old_overshoot_x != new_overshoot_x || old_overshoot_y != new_overshoot_y) {
    if (new_overshoot_x >= 0 || new_overshoot_y >= 0) {
      /* We need to reallocate the widget to have it at
       * negative offset, so there's a "gravity" on the
       * bottom/right corner
       */
      gtk_widget_queue_resize (GTK_WIDGET (self));
    }
    if (new_overshoot_x < 0 || new_overshoot_y < 0)
      _gtk_scrolled_sync_window_allocate_overshoot_window (self);
  }

  gtk_scrolled_sync_window_calculate_velocity (self, event);

  return TRUE;
}

static gboolean
gtk_scrolled_sync_window_captured_button_press (GtkWidget * widget,
    GdkEvent * event)
{
  GtkScrolledSyncWindow *self = GTK_SCROLLED_SYNC_WINDOW (widget);
  GtkScrolledSyncWindowPrivate *priv = self->priv;
  GtkWidget *child;
  GtkWidget *event_widget;
  GdkDevice *source_device;
  GdkInputSource source;
  gdouble x_root, y_root;
  guint button;

  source_device = gdk_event_get_source_device (event);
  source = gdk_device_get_source (source_device);

  if (source != GDK_SOURCE_TOUCHSCREEN)
    return FALSE;

  event_widget = gtk_get_event_widget (event);

  /* If there's another scrolled window between the widget
   * receiving the event and this capturing scrolled window,
   * let it handle the events.
   */
  if (widget != gtk_widget_get_ancestor (event_widget,
          GTK_TYPE_SCROLLED_SYNC_WINDOW))
    return FALSE;

  /* Check whether the button press is close to the previous one,
   * take that as a shortcut to get the child widget handle events
   */
  gdk_event_get_root_coords (event, &x_root, &y_root);
  if (priv->last_button_event_valid &&
      ABS (x_root - priv->last_button_event_x_root) <
      TOUCH_BYPASS_CAPTURED_THRESHOLD
      && ABS (y_root - priv->last_button_event_y_root) <
      TOUCH_BYPASS_CAPTURED_THRESHOLD) {
    priv->last_button_event_valid = FALSE;
    return FALSE;
  }

  priv->last_button_event_x_root = priv->last_motion_event_x_root = x_root;
  priv->last_button_event_y_root = priv->last_motion_event_y_root = y_root;
  priv->last_motion_event_time = gdk_event_get_time (event);
  priv->last_button_event_valid = TRUE;

  if (gdk_event_get_button (event, &button) && button != 1)
    return FALSE;

  child = gtk_bin_get_child (GTK_BIN (widget));
  if (!child)
    return FALSE;

  priv->drag_seat = gdk_event_get_seat (event);
  // dbeswick: fix
  //  gdk_seat_grab(priv->drag_seat, widget, GDK_SEAT_CAPABILITY_ALL_POINTING, TRUE, NULL, event, NULL, NULL);

  gtk_scrolled_sync_window_cancel_deceleration (self);

  /* Only set the timeout if we're going to store an event */
  if (priv->capture_button_press)
    priv->release_timeout_id =
        gdk_threads_add_timeout (RELEASE_EVENT_TIMEOUT,
        (GSourceFunc) gtk_scrolled_sync_window_release_captured_event, self);

  priv->in_drag = FALSE;

  if (priv->capture_button_press) {
    /* Store the button press event in
     * case we need to propagate it later
     */
    priv->button_press_event = gdk_event_copy (event);
    return TRUE;
  } else
    return FALSE;
}

static gboolean
gtk_scrolled_sync_window_captured_event (GtkWidget * widget, GdkEvent * event)
{
  gboolean retval = FALSE;
  GtkScrolledSyncWindowPrivate *priv = GTK_SCROLLED_SYNC_WINDOW (widget)->priv;

  if (gdk_window_get_window_type (event->any.window) == GDK_WINDOW_TEMP)
    return FALSE;

  switch (event->type) {
    case GDK_TOUCH_BEGIN:
    case GDK_BUTTON_PRESS:
      retval = gtk_scrolled_sync_window_captured_button_press (widget, event);
      break;
    case GDK_TOUCH_END:
    case GDK_BUTTON_RELEASE:
      if (priv->drag_seat)
        retval =
            gtk_scrolled_sync_window_captured_button_release (widget, event);
      else
        priv->last_button_event_valid = FALSE;
      break;
    case GDK_TOUCH_UPDATE:
    case GDK_MOTION_NOTIFY:
      if (priv->drag_seat)
        retval =
            gtk_scrolled_sync_window_captured_motion_notify (widget, event);
      break;
    case GDK_LEAVE_NOTIFY:
    case GDK_ENTER_NOTIFY:
      if (priv->in_drag && event->crossing.mode != GDK_CROSSING_GRAB)
        retval = TRUE;
      break;
    default:
      break;
  }

  return retval;
}

static gboolean
gtk_scrolled_sync_window_focus (GtkWidget * widget, GtkDirectionType direction)
{
  GtkScrolledSyncWindow *self = GTK_SCROLLED_SYNC_WINDOW (widget);
  GtkScrolledSyncWindowPrivate *priv = self->priv;
  GtkWidget *child;
  gboolean had_focus_child =
      gtk_container_get_focus_child (GTK_CONTAINER (widget)) != NULL;

  if (priv->focus_out) {
    priv->focus_out = FALSE;    /* Clear this to catch the wrap-around case */
    return FALSE;
  }

  if (gtk_widget_is_focus (widget))
    return FALSE;

  /* We only put the scrolled window itself in the focus chain if it
   * isn't possible to focus any children.
   */
  if ((child = gtk_bin_get_child (GTK_BIN (widget)))) {
    if (gtk_widget_child_focus (child, direction))
      return TRUE;
  }

  if (!had_focus_child && gtk_widget_get_can_focus (widget)) {
    gtk_widget_grab_focus (widget);
    return TRUE;
  } else
    return FALSE;
}

static void
gtk_scrolled_sync_window_adjustment_value_changed (GtkAdjustment * adjustment,
    gpointer user_data)
{
  GtkScrolledSyncWindow *self = user_data;
  GtkScrolledSyncWindowPrivate *priv = self->priv;

  /* Allow overshooting for kinetic scrolling operations */
  if (priv->drag_seat || priv->deceleration_id)
    return;

  /* Ensure GtkAdjustment and unclamped values are in sync */
  if (priv->vadjustment == adjustment)
    priv->unclamped_vadj_value = gtk_adjustment_get_value (adjustment);
  else if (priv->hadjustment == adjustment)
    priv->unclamped_hadj_value = gtk_adjustment_get_value (adjustment);
}

static void
gtk_scrolled_sync_window_add (GtkContainer * container, GtkWidget * child)
{
  GtkScrolledSyncWindowPrivate *priv;
  GtkScrolledSyncWindow *self;
  GtkWidget *scrollable_child;

  g_return_if_fail (gtk_bin_get_child (GTK_BIN (container)) == NULL);

  self = GTK_SCROLLED_SYNC_WINDOW (container);
  priv = self->priv;

  if (GTK_IS_SCROLLABLE (child)) {
    scrollable_child = child;
  } else {
    scrollable_child = gtk_viewport_new (priv->hadjustment, priv->vadjustment);
    gtk_widget_show (scrollable_child);
    gtk_container_set_focus_hadjustment (GTK_CONTAINER (scrollable_child),
        gtk_scrolled_sync_window_get_hadjustment (GTK_SCROLLED_SYNC_WINDOW
            (self)));
    gtk_container_set_focus_vadjustment (GTK_CONTAINER (scrollable_child),
        gtk_scrolled_sync_window_get_vadjustment (GTK_SCROLLED_SYNC_WINDOW
            (self)));
    gtk_container_add (GTK_CONTAINER (scrollable_child), child);
  }

  if (gtk_widget_get_realized (GTK_WIDGET (container)))
    gtk_widget_set_parent_window (scrollable_child, priv->overshoot_window);

  GTK_CONTAINER_CLASS (gtk_scrolled_sync_window_parent_class)->add (container,
      scrollable_child);

  g_object_set (scrollable_child, "hadjustment", priv->hadjustment,
      "vadjustment", priv->vadjustment, NULL);
}

static void
gtk_scrolled_sync_window_remove (GtkContainer * container, GtkWidget * child)
{
  g_return_if_fail (GTK_IS_SCROLLED_SYNC_WINDOW (container));
  g_return_if_fail (child != NULL);
  g_return_if_fail (gtk_bin_get_child (GTK_BIN (container)) == child);

  g_object_set (child, "hadjustment", NULL, "vadjustment", NULL, NULL);

  /* chain parent class handler to remove child */
  GTK_CONTAINER_CLASS (gtk_scrolled_sync_window_parent_class)->remove
      (container, child);
}

/**
 * gtk_scrolled_sync_window_add_with_viewport:
 * @self: a #GtkScrolledSyncWindow
 * @child: the widget you want to scroll
 *
 * Used to add children without native scrolling capabilities. This
 * is simply a convenience function; it is equivalent to adding the
 * unscrollable child to a viewport, then adding the viewport to the
 * scrolled window. If a child has native scrolling, use
 * gtk_container_add() instead of this function.
 *
 * The viewport scrolls the child by moving its #GdkWindow, and takes
 * the size of the child to be the size of its toplevel #GdkWindow.
 * This will be very wrong for most widgets that support native scrolling;
 * for example, if you add a widget such as #GtkTreeView with a viewport,
 * the whole widget will scroll, including the column headings. Thus,
 * widgets with native scrolling support should not be used with the
 * #GtkViewport proxy.
 *
 * A widget supports scrolling natively if it implements the
 * #GtkScrollable interface.
 *
 * Deprecated: 3.8: gtk_container_add() will now automatically add
 * a #GtkViewport if the child doesn't implement #GtkScrollable.
 */
void
gtk_scrolled_sync_window_add_with_viewport (GtkScrolledSyncWindow * self,
    GtkWidget * child)
{
  GtkBin *bin;
  GtkWidget *viewport;
  GtkWidget *child_widget;

  g_return_if_fail (GTK_IS_SCROLLED_SYNC_WINDOW (self));
  g_return_if_fail (GTK_IS_WIDGET (child));
  g_return_if_fail (gtk_widget_get_parent (child) == NULL);

  bin = GTK_BIN (self);
  child_widget = gtk_bin_get_child (bin);

  if (child_widget) {
    g_return_if_fail (GTK_IS_VIEWPORT (child_widget));
    g_return_if_fail (gtk_bin_get_child (GTK_BIN (child_widget)) == NULL);

    viewport = child_widget;
  } else {
    viewport =
        gtk_viewport_new (gtk_scrolled_sync_window_get_hadjustment (self),
        gtk_scrolled_sync_window_get_vadjustment (self));
    gtk_container_set_focus_hadjustment (GTK_CONTAINER (viewport),
        gtk_scrolled_sync_window_get_hadjustment (GTK_SCROLLED_SYNC_WINDOW
            (self)));
    gtk_container_set_focus_vadjustment (GTK_CONTAINER (viewport),
        gtk_scrolled_sync_window_get_vadjustment (GTK_SCROLLED_SYNC_WINDOW
            (self)));
    gtk_container_add (GTK_CONTAINER (self), viewport);
  }

  gtk_widget_show (viewport);
  gtk_container_add (GTK_CONTAINER (viewport), child);
}

static void
gtk_scrolled_sync_window_get_preferred_size (GtkWidget * widget,
    GtkOrientation orientation, gint * minimum_size, gint * natural_size)
{
  GtkScrolledSyncWindow *self = GTK_SCROLLED_SYNC_WINDOW (widget);
  GtkBin *bin = GTK_BIN (self);
  GtkRequisition minimum_req, natural_req;
  GtkWidget *child;
  gint min_child_size, nat_child_size;

  minimum_req.width = 0;
  minimum_req.height = 0;
  natural_req.width = 0;
  natural_req.height = 0;

  child = gtk_bin_get_child (bin);
  if (child && gtk_widget_get_visible (child)) {
    if (orientation == GTK_ORIENTATION_HORIZONTAL) {
      gtk_widget_get_preferred_width (child, &min_child_size, &nat_child_size);
      natural_req.width += nat_child_size;
    } else {                    /* GTK_ORIENTATION_VERTICAL */
      gtk_widget_get_preferred_height (child, &min_child_size, &nat_child_size);
      natural_req.height += nat_child_size;
    }
  }

  if (orientation == GTK_ORIENTATION_HORIZONTAL) {
    if (minimum_size)
      *minimum_size = minimum_req.width;
    if (natural_size)
      *natural_size = natural_req.width;
  } else {
    if (minimum_size)
      *minimum_size = minimum_req.height;
    if (natural_size)
      *natural_size = natural_req.height;
  }
}

static void
gtk_scrolled_sync_window_get_preferred_width (GtkWidget * widget,
    gint * minimum_size, gint * natural_size)
{
  gtk_scrolled_sync_window_get_preferred_size (widget,
      GTK_ORIENTATION_HORIZONTAL, minimum_size, natural_size);
}

static void
gtk_scrolled_sync_window_get_preferred_height (GtkWidget * widget,
    gint * minimum_size, gint * natural_size)
{
  gtk_scrolled_sync_window_get_preferred_size (widget, GTK_ORIENTATION_VERTICAL,
      minimum_size, natural_size);
}

static void
gtk_scrolled_sync_window_get_preferred_height_for_width (GtkWidget * widget,
    gint width, gint * minimum_height, gint * natural_height)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  GTK_WIDGET_GET_CLASS (widget)->get_preferred_height (widget, minimum_height,
      natural_height);
}

static void
gtk_scrolled_sync_window_get_preferred_width_for_height (GtkWidget * widget,
    gint height, gint * minimum_width, gint * natural_width)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  GTK_WIDGET_GET_CLASS (widget)->get_preferred_width (widget, minimum_width,
      natural_width);
}

static void
gtk_scrolled_sync_window_realize (GtkWidget * widget)
{
  GtkScrolledSyncWindow *self = GTK_SCROLLED_SYNC_WINDOW (widget);
  GtkAllocation allocation;
  GdkWindowAttr attributes;
  GtkWidget *child_widget;
  gint attributes_mask;

  gtk_widget_set_realized (widget, TRUE);
  gtk_widget_get_allocation (widget, &allocation);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.event_mask = GDK_VISIBILITY_NOTIFY_MASK |
      GDK_BUTTON_MOTION_MASK | GDK_TOUCH_MASK | GDK_EXPOSURE_MASK;

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;

  self->priv->overshoot_window =
      gdk_window_new (gtk_widget_get_parent_window (widget),
      &attributes, attributes_mask);
#if GTK_CHECK_VERSION (3,8,0)
  gtk_widget_register_window (widget, self->priv->overshoot_window);
#else
  gdk_window_set_user_data (self->priv->overshoot_window, widget);
#endif

  child_widget = gtk_bin_get_child (GTK_BIN (widget));

  if (child_widget)
    gtk_widget_set_parent_window (child_widget, self->priv->overshoot_window);

  GTK_WIDGET_CLASS (gtk_scrolled_sync_window_parent_class)->realize (widget);
}

static void
gtk_scrolled_sync_window_unrealize (GtkWidget * widget)
{
  GtkScrolledSyncWindow *self = GTK_SCROLLED_SYNC_WINDOW (widget);

#if GTK_CHECK_VERSION (3,8,0)
  gtk_widget_unregister_window (widget, self->priv->overshoot_window);
#else
  gdk_window_set_user_data (self->priv->overshoot_window, NULL);
#endif
  gdk_window_destroy (self->priv->overshoot_window);
  self->priv->overshoot_window = NULL;

  gtk_widget_set_realized (widget, FALSE);

  GTK_WIDGET_CLASS (gtk_scrolled_sync_window_parent_class)->unrealize (widget);
}

static void
gtk_scrolled_sync_window_map (GtkWidget * widget)
{
  GtkScrolledSyncWindow *self = GTK_SCROLLED_SYNC_WINDOW (widget);

  gdk_window_show (self->priv->overshoot_window);

  GTK_WIDGET_CLASS (gtk_scrolled_sync_window_parent_class)->map (widget);
}

static void
gtk_scrolled_sync_window_unmap (GtkWidget * widget)
{
  GtkScrolledSyncWindow *self = GTK_SCROLLED_SYNC_WINDOW (widget);

  gdk_window_hide (self->priv->overshoot_window);

  GTK_WIDGET_CLASS (gtk_scrolled_sync_window_parent_class)->unmap (widget);
}

static void
gtk_scrolled_sync_window_grab_notify (GtkWidget * widget, gboolean was_grabbed)
{
  GtkScrolledSyncWindow *self = GTK_SCROLLED_SYNC_WINDOW (widget);
  GtkScrolledSyncWindowPrivate *priv = self->priv;

  if (priv->drag_seat) {
    gdk_seat_ungrab (priv->drag_seat);
    priv->drag_seat = NULL;
    priv->in_drag = FALSE;

    if (priv->release_timeout_id) {
      g_source_remove (priv->release_timeout_id);
      priv->release_timeout_id = 0;
    }

    if (_gtk_scrolled_sync_window_get_overshoot (self, NULL, NULL))
      gtk_scrolled_sync_window_start_deceleration (self);
    else
      gtk_scrolled_sync_window_cancel_deceleration (self);

    priv->last_button_event_valid = FALSE;
  }
}
