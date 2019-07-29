/* Buzztrax
 * Copyright (C) 20015 Buzztrax team <buzztrax-devel@buzztrax.org>
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
 * SECTION:btpianokeys
 * @short_description: a piano keyboard widget
 * @see_also: #BtMachinePropertiesDialog
 *
 * A musical piano keyboard widget.
 */

#include <string.h>
#include "piano-keys.h"

#define KEY_HEIGHT 35
#define KEY_WIDTH 14
#define WHITE_KEY_HEIGHT KEY_HEIGHT
#define BLACK_KEY_HEIGHT 24

//-- signal ids

enum
{
  KEY_PRESSED,
  KEY_RELEASED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

//-- the class

G_DEFINE_TYPE (BtPianoKeys, bt_piano_keys, GTK_TYPE_WIDGET);


static void
bt_piano_keys_realize (GtkWidget * widget)
{
  BtPianoKeys *self = BT_PIANO_KEYS (widget);
  GdkWindow *window;
  GtkAllocation allocation;
  GdkWindowAttr attributes;
  gint attributes_mask;

  gtk_widget_set_realized (widget, TRUE);

  window = gtk_widget_get_parent_window (widget);
  gtk_widget_set_window (widget, window);
  g_object_ref (window);

  gtk_widget_get_allocation (widget, &allocation);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.wclass = GDK_INPUT_ONLY;
  attributes.event_mask = gtk_widget_get_events (widget);
  attributes.event_mask |= (GDK_EXPOSURE_MASK |
      GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_ENTER_NOTIFY_MASK |
      GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK | GDK_LEAVE_NOTIFY_MASK);
  attributes_mask = GDK_WA_X | GDK_WA_Y;

  self->window = gdk_window_new (window, &attributes, attributes_mask);
#if GTK_CHECK_VERSION (3,8,0)
  gtk_widget_register_window (widget, self->window);
#else
  gdk_window_set_user_data (self->window, widget);
#endif
}

static void
bt_piano_keys_unrealize (GtkWidget * widget)
{
  BtPianoKeys *self = BT_PIANO_KEYS (widget);

#if GTK_CHECK_VERSION (3,8,0)
  gtk_widget_unregister_window (widget, self->window);
#else
  gdk_window_set_user_data (self->window, NULL);
#endif
  gdk_window_destroy (self->window);
  self->window = NULL;
  GTK_WIDGET_CLASS (bt_piano_keys_parent_class)->unrealize (widget);
}

static void
bt_piano_keys_map (GtkWidget * widget)
{
  BtPianoKeys *self = BT_PIANO_KEYS (widget);

  gdk_window_show (self->window);

  GTK_WIDGET_CLASS (bt_piano_keys_parent_class)->map (widget);
}

static void
bt_piano_keys_unmap (GtkWidget * widget)
{
  BtPianoKeys *self = BT_PIANO_KEYS (widget);

  gdk_window_hide (self->window);

  GTK_WIDGET_CLASS (bt_piano_keys_parent_class)->unmap (widget);
}

static gboolean
bt_piano_keys_draw (GtkWidget * widget, cairo_t * cr)
{
  BtPianoKeys *self = BT_PIANO_KEYS (widget);
  GtkStyleContext *style_ctx;
  gint width, height, left, right, top;
  gint x, k, y;
  gboolean sensitive = gtk_widget_is_sensitive (widget);
  gint pressed_key = -1, pressed_oct = -1;
  static const gint bwk[] = { 1, -1, 2, -2, 3, 4, -4, 5, -5, 6, -6, 7 };
  gchar oct[4];
  cairo_text_extents_t ext;

  width = gtk_widget_get_allocated_width (widget);
  height = gtk_widget_get_allocated_height (widget);
  style_ctx = gtk_widget_get_style_context (widget);

  /* draw border */
  gtk_render_background (style_ctx, cr, 0, 0, width, height);
  gtk_render_frame (style_ctx, cr, 0, 0, width, height);

  left = self->border.left;
  top = self->border.top;
  width -= self->border.left + self->border.right;
  height -= self->border.top + self->border.bottom;
  right = left + width;

  // selected key
  if (self->key != GSTBT_NOTE_NONE) {
    pressed_key = self->key - GSTBT_NOTE_C_0;
    pressed_oct = pressed_key / 16;
    pressed_key &= 0xf;
  }

  if (sensitive) {
    cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  } else {
    cairo_set_source_rgb (cr, 0.7, 0.7, 0.7);
  }
  cairo_rectangle (cr, left, top, width, height);
  cairo_fill (cr);

  if (sensitive) {
    cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
  } else {
    cairo_set_source_rgb (cr, 0.3, 0.3, 0.3);
  }
  // draw white keys
  for (x = left; x < right; x += KEY_WIDTH) {
    cairo_rectangle (cr, x, top, KEY_WIDTH, WHITE_KEY_HEIGHT);
    cairo_stroke (cr);
  }
  // render selected white key
  if (pressed_key != -1 && bwk[pressed_key] > 0) {
    x = left + (pressed_oct * 7 + (bwk[pressed_key] - 1)) * KEY_WIDTH;
    cairo_set_source_rgb (cr, 0.7, 0.7, 0.7);
    cairo_rectangle (cr, x, top, KEY_WIDTH, WHITE_KEY_HEIGHT);
    cairo_fill_preserve (cr);
    cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
    cairo_stroke (cr);
  }
  // draw black keys
  for (x = left + KEY_WIDTH / 2, k = 0; x < right; x += KEY_WIDTH, k++) {
    if (k == 7)
      k = 0;
    if (k == 2 || k == 6)
      continue;
    if (sensitive) {
      cairo_set_source_rgb (cr, 0.2, 0.2, 0.2);
    } else {
      cairo_set_source_rgb (cr, 0.4, 0.4, 0.4);
    }
    cairo_rectangle (cr, x + 1, top, KEY_WIDTH - 2, BLACK_KEY_HEIGHT - 3);
    cairo_fill (cr);
    if (sensitive) {
      cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
    } else {
      cairo_set_source_rgb (cr, 0.3, 0.3, 0.3);
    }
    cairo_rectangle (cr, x + 1, top + (BLACK_KEY_HEIGHT - 3), KEY_WIDTH - 2, 3);
    cairo_fill (cr);
  }
  // render selected black key
  if (pressed_key != -1 && bwk[pressed_key] < 0) {
    x = left + KEY_WIDTH / 2 +
        (pressed_oct * 7 + (-bwk[pressed_key] - 1)) * KEY_WIDTH;
    cairo_set_source_rgb (cr, 0.3, 0.3, 0.3);
    cairo_rectangle (cr, x + 1, top, KEY_WIDTH - 2, BLACK_KEY_HEIGHT);
    cairo_fill (cr);
  }
  // draw octave numbers
  if (sensitive) {
    cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
  } else {
    cairo_set_source_rgb (cr, 0.3, 0.3, 0.3);
  }
  y = top + (WHITE_KEY_HEIGHT - 2);
  for (x = left; x < right; x += (7 * KEY_WIDTH)) {
    snprintf (oct, sizeof(oct), "%u", (guint8) ((x - left) / (7 * KEY_WIDTH)));
    cairo_text_extents (cr, oct, &ext);
    k = x + ((KEY_WIDTH / 2) - (ext.width / 2));
    if (k + ext.width < right) {
      cairo_move_to (cr, k, y);
      cairo_show_text (cr, oct);
    }
  }

  // draw focus
  if (gtk_widget_has_visible_focus (widget)) {
    gtk_render_focus (style_ctx, cr, 0, 0, width, height);
  }

  return TRUE;
}


static void
bt_piano_keys_get_preferred_width (GtkWidget * widget,
    gint * minimal_width, gint * natural_width)
{
  BtPianoKeys *self = BT_PIANO_KEYS (widget);
  gint border_padding = self->border.left + self->border.right;

  *minimal_width = (KEY_WIDTH * 7) + border_padding;    // one octave
  *natural_width = (KEY_WIDTH * 70) + border_padding;   // 10 octaves
}

static void
bt_piano_keys_get_preferred_height (GtkWidget * widget,
    gint * minimal_height, gint * natural_height)
{
  BtPianoKeys *self = BT_PIANO_KEYS (widget);
  gint border_padding = self->border.top + self->border.bottom;

  *minimal_height = KEY_HEIGHT + border_padding;
  *natural_height = *minimal_height;
}

static void
bt_piano_keys_size_allocate (GtkWidget * widget, GtkAllocation * allocation)
{
  BtPianoKeys *self = BT_PIANO_KEYS (widget);
  GtkStyleContext *context;
  GtkStateFlags state;
  GtkBorder pad;
  gint minimal, natural, maximal;

  context = gtk_widget_get_style_context (widget);
  state = gtk_widget_get_state_flags (widget);
  gtk_style_context_get_border (context, state, &self->border);
  gtk_style_context_get_padding (context, state, &pad);
  // the default padding is a bit weird :/
  self->border.left += pad.left - 1;
  self->border.right += pad.right - 1;
  self->border.top += pad.top + 1;
  self->border.bottom += pad.bottom + 1;

  // no more than 10 octaves
  maximal = self->border.left + self->border.right + KEY_WIDTH * 70;
  bt_piano_keys_get_preferred_width (widget, &minimal, &natural);
  allocation->width = CLAMP (allocation->width, minimal, maximal);

  bt_piano_keys_get_preferred_height (widget, &minimal, &natural);
  allocation->height = CLAMP (allocation->height, minimal, natural);

  gtk_widget_set_allocation (widget, allocation);

  if (gtk_widget_get_realized (widget))
    gdk_window_move_resize (self->window,
        allocation->x, allocation->y, allocation->width, allocation->height);
}

static gboolean
bt_piano_keys_button_press (GtkWidget * widget, GdkEventButton * event)
{
  BtPianoKeys *self = BT_PIANO_KEYS (widget);
  static const gint wk[] = { 0, 2, 4, 5, 7, 9, 11 };
  static const gint bk[] = { -1, 1, 3, -1, 6, 8, 10 };

  if (!gtk_widget_is_sensitive (widget))
    return TRUE;

  if (event->button == GDK_BUTTON_PRIMARY) {
    gint k = -1, x = (gint) event->x;

    // check black keys
    if (event->y < BLACK_KEY_HEIGHT) {
      k = bk[((x + (KEY_WIDTH / 2)) / KEY_WIDTH) % 7];
    }
    // check white keys
    if (k == -1) {
      k = wk[(x / KEY_WIDTH) % 7];
    }
    // add octave offset and GSTBT_NOTE_C_0 offset
    k += GSTBT_NOTE_C_0 + (16 * (x / (7 * KEY_WIDTH)));

    if (k < GSTBT_NOTE_LAST) {
      self->key = k;
      gtk_widget_queue_draw (widget);
      g_signal_emit (self, signals[KEY_PRESSED], 0, k);
    }
  }
  return FALSE;
}

static gboolean
bt_piano_keys_button_release (GtkWidget * widget, GdkEventButton * event)
{
  BtPianoKeys *self = BT_PIANO_KEYS (widget);

  if (!gtk_widget_is_sensitive (widget))
    return TRUE;

  if (self->key != GSTBT_NOTE_NONE) {
    gtk_widget_queue_draw (widget);
    g_signal_emit (self, signals[KEY_RELEASED], 0, self->key);
    self->key = GSTBT_NOTE_NONE;
  }
  return FALSE;
}

static gboolean
bt_piano_keys_key_press (GtkWidget * widget, GdkEventKey * event)
{
  BtPianoKeys *self = BT_PIANO_KEYS (widget);
  //static const char notenames[] = "zsxdcvgbhnjm\t\t\t\tq2w3er5t6y7u\t\t\t\ti9o0p";
  static const gchar notenames[] =
      "\x34\x27\x35\x28\x36\x37\x2a\x38\x2b\x39\x2c\x3a\x3a\x3a\x3a\x3a\x18\x0b\x19\x0c\x1a\x1b\x0e\x1c\x0f\x1d\x10\x1e\x1e\x1e\x1e\x1e\x1f\x12\x20\x13\x21";
  const gchar *p;

  if (!gtk_widget_is_sensitive (widget))
    return TRUE;

  if ((p = strchr (notenames, (char) event->hardware_keycode))) {
    gint k = GSTBT_NOTE_C_0 + (p - notenames);
    if ((k & 15) <= 12) {
      self->key = k;
      gtk_widget_queue_draw (widget);
      g_signal_emit (self, signals[KEY_PRESSED], 0, k);
    }
  }
  return FALSE;
}

static gboolean
bt_piano_keys_key_release (GtkWidget * widget, GdkEventKey * event)
{
  BtPianoKeys *self = BT_PIANO_KEYS (widget);

  if (!gtk_widget_is_sensitive (widget))
    return TRUE;

  // check that we release that key that actually triggered this?
  if (self->key != GSTBT_NOTE_NONE) {
    gtk_widget_queue_draw (widget);
    g_signal_emit (self, signals[KEY_RELEASED], 0, self->key);
    self->key = GSTBT_NOTE_NONE;
  }
  return FALSE;
}

static void
bt_piano_keys_class_init (BtPianoKeysClass * klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->realize = bt_piano_keys_realize;
  widget_class->unrealize = bt_piano_keys_unrealize;
  widget_class->map = bt_piano_keys_map;
  widget_class->unmap = bt_piano_keys_unmap;
  widget_class->draw = bt_piano_keys_draw;
  widget_class->get_preferred_width = bt_piano_keys_get_preferred_width;
  widget_class->get_preferred_height = bt_piano_keys_get_preferred_height;
  widget_class->size_allocate = bt_piano_keys_size_allocate;
  widget_class->button_press_event = bt_piano_keys_button_press;
  widget_class->button_release_event = bt_piano_keys_button_release;
  widget_class->key_press_event = bt_piano_keys_key_press;
  widget_class->key_release_event = bt_piano_keys_key_release;

  /**
   * BtPianoKeys::key-pressed:
   * @self: the piano-keys object that emitted the signal
   * @key: the key note number
   *
   * Signals that a piano key was pressed.
   */
  signals[KEY_PRESSED] =
      g_signal_new ("key-pressed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 0, NULL,
      NULL, g_cclosure_marshal_VOID__ENUM, G_TYPE_NONE, 1, GSTBT_TYPE_NOTE);

  /**
   * BtPianoKeys::key-released:
   * @self: the piano-keys object that emitted the signal
   * @key: the key note number
   *
   * Signals that a piano key was released.
   */
  signals[KEY_RELEASED] =
      g_signal_new ("key-released", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 0, NULL,
      NULL, g_cclosure_marshal_VOID__ENUM, G_TYPE_NONE, 1, GSTBT_TYPE_NOTE);
}

static void
bt_piano_keys_init (BtPianoKeys * self)
{
  GtkStyleContext *context;

  context = gtk_widget_get_style_context (GTK_WIDGET (self));
  gtk_style_context_add_class (context, GTK_STYLE_CLASS_FRAME);
  gtk_style_context_add_class (context, GTK_STYLE_CLASS_BUTTON);

  gtk_widget_set_can_focus (GTK_WIDGET (self), TRUE);
  gtk_widget_set_has_window (GTK_WIDGET (self), FALSE);
}

/**
 * bt_piano_keys_new:
 *
 * Create a new piano keys widget.
 *
 * Returns: the widget
 */
GtkWidget *
bt_piano_keys_new (void)
{
  return GTK_WIDGET (g_object_new (BT_TYPE_PIANO_KEYS, NULL));
}
