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
#include "cairo.h"
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
bt_piano_keys_snapshot (GtkWidget *widget, GtkSnapshot *snapshot)
{
  BtPianoKeys *self = BT_PIANO_KEYS (widget);
  graphene_rect_t rect = GRAPHENE_RECT_INIT(0, 0, gtk_widget_get_width (widget), gtk_widget_get_height (widget));
  
  cairo_t* cr = gtk_snapshot_append_cairo (snapshot, &rect);
  
  gint width, height, left, right, top;
  gint x, k, y;
  gboolean sensitive = gtk_widget_is_sensitive (widget);
  gint pressed_key = -1, pressed_oct = -1;
  static const gint bwk[] = { 1, -1, 2, -2, 3, 4, -4, 5, -5, 6, -6, 7 };
  gchar oct[4];
  cairo_text_extents_t ext;

  width = rect.size.width;
  height = rect.size.height;

  /* draw border */
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

#if 0 /// GTK4
  // draw focus
  if (gtk_widget_has_visible_focus (widget)) {
    gtk_render_focus (style_ctx, cr, 0, 0, width, height);
  }
#endif

  cairo_destroy (cr);
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
bt_piano_keys_measure (GtkWidget* widget, GtkOrientation orientation, int for_size,
    int* minimum, int* natural, int* minimum_baseline, int* natural_baseline)
{
  BtPianoKeys *self = BT_PIANO_KEYS (widget);
  GtkStateFlags state;
  GtkBorder pad;

  state = gtk_widget_get_state_flags (widget);
  // the default padding is a bit weird :/
  self->border.left += pad.left - 1;
  self->border.right += pad.right - 1;
  self->border.top += pad.top + 1;
  self->border.bottom += pad.bottom + 1;

  if (orientation == GTK_ORIENTATION_HORIZONTAL) {
    gint border_padding = self->border.left + self->border.right;

    *minimum = (KEY_WIDTH * 7) + border_padding;    // one octave
    *natural = (KEY_WIDTH * 70) + border_padding;   // 10 octaves
  } else {
    gint border_padding = self->border.top + self->border.bottom;

    *minimum = KEY_HEIGHT + border_padding;
    *natural = *minimum;
  }
}

static void
bt_piano_keys_button_press (GtkGestureClick* click, gint n_press, gdouble x, gdouble y, gpointer user_data)
{
  BtPianoKeys *self = BT_PIANO_KEYS (user_data);
  static const gint wk[] = { 0, 2, 4, 5, 7, 9, 11 };
  static const gint bk[] = { -1, 1, 3, -1, 6, 8, 10 };

  if (!gtk_widget_is_sensitive (GTK_WIDGET (self)))
    return;

  guint button = gtk_gesture_single_get_current_button (GTK_GESTURE_SINGLE (click));
  
  if (button == GDK_BUTTON_PRIMARY) {
    gint k = -1, x = (gint) x;

    // check black keys
    if (y < BLACK_KEY_HEIGHT) {
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
      gtk_widget_queue_draw (GTK_WIDGET (self));
      g_signal_emit (self, signals[KEY_PRESSED], 0, k);
    }
  }
}

static void
bt_piano_keys_button_release (GtkGestureClick* click, gint n_press, gdouble x, gdouble y, gpointer user_data)
{
  BtPianoKeys *self = BT_PIANO_KEYS (user_data);

  if (!gtk_widget_is_sensitive (GTK_WIDGET (self)))
    return;

  if (self->key != GSTBT_NOTE_NONE) {
    gtk_widget_queue_draw (GTK_WIDGET (self));
    g_signal_emit (self, signals[KEY_RELEASED], 0, self->key);
    self->key = GSTBT_NOTE_NONE;
  }
}

static void
bt_piano_keys_key_press (GtkEventControllerKey* ctrl, guint keyval,
    guint keycode, GdkModifierType state, gpointer user_data)
{
  BtPianoKeys *self = BT_PIANO_KEYS (user_data);
  //static const char notenames[] = "zsxdcvgbhnjm\t\t\t\tq2w3er5t6y7u\t\t\t\ti9o0p";
  static const gchar notenames[] =
      "\x34\x27\x35\x28\x36\x37\x2a\x38\x2b\x39\x2c\x3a\x3a\x3a\x3a\x3a\x18\x0b\x19\x0c\x1a\x1b\x0e\x1c\x0f\x1d\x10\x1e\x1e\x1e\x1e\x1e\x1f\x12\x20\x13\x21";
  const gchar *p;

  if (!gtk_widget_is_sensitive (GTK_WIDGET (self)))
    return;

  if ((p = strchr (notenames, (char) keycode))) {
    gint k = GSTBT_NOTE_C_0 + (p - notenames);
    if ((k & 15) <= 12) {
      self->key = k;
      gtk_widget_queue_draw (GTK_WIDGET (self));
      g_signal_emit (self, signals[KEY_PRESSED], 0, k);
    }
  }
}

static void
bt_piano_keys_key_release (GtkEventControllerKey* ctrl, guint keyval,
    guint keycode, GdkModifierType state, gpointer user_data)
{
  BtPianoKeys *self = BT_PIANO_KEYS (user_data);

  if (!gtk_widget_is_sensitive (GTK_WIDGET (self)))
    return;

  // check that we release that key that actually triggered this?
  if (self->key != GSTBT_NOTE_NONE) {
    gtk_widget_queue_draw (GTK_WIDGET (self));
    g_signal_emit (self, signals[KEY_RELEASED], 0, self->key);
    self->key = GSTBT_NOTE_NONE;
  }
}

static void
bt_piano_keys_class_init (BtPianoKeysClass * klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->snapshot = bt_piano_keys_snapshot;
  
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
  gtk_widget_set_can_focus (GTK_WIDGET (self), TRUE);

  GtkEventController* ctrl;
  ctrl = GTK_EVENT_CONTROLLER (gtk_gesture_click_new ());
  gtk_widget_add_controller (GTK_WIDGET (self), ctrl);
  g_signal_connect_object (ctrl, "pressed", G_CALLBACK (bt_piano_keys_button_press), (gpointer) self, 0);
  g_signal_connect_object (ctrl, "released", G_CALLBACK (bt_piano_keys_button_release), (gpointer) self, 0);
  
  ctrl = gtk_event_controller_key_new ();
  gtk_widget_add_controller (GTK_WIDGET (self), ctrl);
  g_signal_connect_object (ctrl, "key-pressed", G_CALLBACK (bt_piano_keys_key_press), (gpointer) self, 0);
  g_signal_connect_object (ctrl, "key-released", G_CALLBACK (bt_piano_keys_key_press), (gpointer) self, 0);
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
