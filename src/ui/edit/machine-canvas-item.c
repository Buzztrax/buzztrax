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
 * SECTION:btmachinecanvasitem
 * @short_description: class for the editor machine views machine canvas item
 *
 * Provides UI to manipulate the machine state.
 *
 * The canvas object emits #BtMachineCanvasItem::position-changed signal after
 * it has been moved.
 */

/* TODO(ensonic): more graphics:
 * - use svg gfx (design/gui/svgcanvas.c )
 *   - need to have prerenderend images for current zoom level
 *     - idealy have them in ui-resources, in order to have them shared
 *     - currently there is a ::zoom property to update the font-size
 *       its set in update_machines_zoom(), there we would need to regenerate
 *       the pixmaps too
 * - state graphics
 *   - have some gfx in the middle
 *     mute: x over o
 *     bypass: -/\- around o
 *     solo:
 *       ! over x
 *       one filled o on top of 4 hollow o's
 *   - use transparency for mute/bypass, solo would switch all other sources to
 *     muted, can't differenciate mute from bypass on an fx
 *
 * TODO(ensonic): play machines live (midi keyboard for playing notes and triggers)
 *   - for source-machine context menu add
 *     - one item "play with > [list of keyboard]"
 *     - one more item to disconnect
 *   - playing multiple machines with a split keyboard would be nice too
 *
 * TODO(ensonic): add insert before/after to context menu (see wire-canvas item)
 *   - below the connect item in the context menu add "Add machine and connect"
 *     for src-machines, "Add machine (before|after) and connect" for effects"
 *     and "Add machine and connect" for master.
 *     - the submenu would be the machine menu
 *     - need to figure a nice way to place the new machine
 *
 * TODO(ensonic): "remove and relink" is difficult if there are non empty wire patterns
 *   - those would need to be copied to new target machine(s) and we would need
 *     to add more tracks for playing them.
 *   - we could ask the user if that is what he wants:
 *     - "don't remove"
 *     - "drop wire patterns"
 *     - "copy wire patterns"
 *
 * TODO(ensonic): make the properties dialog a readable gobject property
 * - then we can go over all of them from machine page and show/hide them
 */

#define BT_EDIT
#define BT_MACHINE_CANVAS_ITEM_C

#include "bt-edit.h"
#include <clutter/gdk/clutter-gdk.h>

#define LOW_VUMETER_VAL -60.0

//-- signal ids

enum
{
  POSITION_CHANGED,
  START_CONNECT,
  LAST_SIGNAL
};

//-- property ids

enum
{
  MACHINE_CANVAS_ITEM_MACHINES_PAGE = 1,
  MACHINE_CANVAS_ITEM_MACHINE,
  MACHINE_CANVAS_ITEM_ZOOM,
  MACHINE_CANVAS_ITEM_PROPERTIES_DIALOG,
  MACHINE_CANVAS_ITEM_PREFERENCES_DIALOG,
  MACHINE_CANVAS_ITEM_ANALYSIS_DIALOG
};


struct _BtMachineCanvasItemPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* the machine page we are on */
  BtMainPageMachines *main_page_machines;

  /* the underlying machine */
  BtMachine *machine;
  GHashTable *properties;
  const gchar *help_uri;

  /* machine context_menu */
  GtkMenu *context_menu;
  GtkWidget *menu_item_mute, *menu_item_solo, *menu_item_bypass;
  gulong id_mute, id_solo, id_bypass;

  /* the properties and preferences dialogs */
  GtkWidget *properties_dialog;
  GtkWidget *preferences_dialog;
  /* the analysis dialog */
  GtkWidget *analysis_dialog;

  /* the graphical components */
  ClutterContent *image;
  ClutterActor *label;
  ClutterActor *output_meter, *input_meter;
  GstElement *output_level;
  GstElement *input_level;
  guint skip_input_level;
  guint skip_output_level;

  GstClock *clock;

  /* cursor for moving */
  GdkCursor *drag_cursor;

  /* the zoom-ratio in pixels/per unit */
  gdouble zoom;

  /* interaction state */
  gboolean dragging, moved /*,switching */ ;
  gfloat offx, offy, dragx, dragy;
  gulong capture_id;

  /* playback state */
  gboolean is_playing;

  /* lock for multithreaded access */
  GMutex lock;
};

static guint signals[LAST_SIGNAL] = { 0, };

static GQuark bus_msg_level_quark = 0;
static GQuark machine_canvas_item_quark = 0;


//-- the class

G_DEFINE_TYPE (BtMachineCanvasItem, bt_machine_canvas_item, CLUTTER_TYPE_ACTOR);


//-- prototypes

static void on_machine_properties_dialog_destroy (GtkWidget * widget,
    gpointer user_data);
static void on_machine_preferences_dialog_destroy (GtkWidget * widget,
    gpointer user_data);
static void on_signal_analysis_dialog_destroy (GtkWidget * widget,
    gpointer user_data);

//-- helper methods

static void
update_machine_graphics (BtMachineCanvasItem * self)
{
  GdkPixbuf *pixbuf =
      bt_ui_resources_get_machine_graphics_pixbuf_by_machine (self->
      priv->machine, self->priv->zoom);

  clutter_image_set_data (CLUTTER_IMAGE (self->priv->image),
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
show_machine_properties_dialog (BtMachineCanvasItem * self)
{
  if (!self->priv->properties_dialog) {
    self->priv->properties_dialog =
        GTK_WIDGET (bt_machine_properties_dialog_new (self->priv->machine));
    bt_edit_application_attach_child_window (self->priv->app,
        GTK_WINDOW (self->priv->properties_dialog));
    gtk_widget_show_all (self->priv->properties_dialog);
    GST_INFO ("machine properties dialog opened");
    // remember open/closed state
    g_hash_table_insert (self->priv->properties, g_strdup ("properties-shown"),
        g_strdup ("1"));
    g_signal_connect (self->priv->properties_dialog, "destroy",
        G_CALLBACK (on_machine_properties_dialog_destroy), (gpointer) self);
    g_object_notify ((GObject *) self, "properties-dialog");
  }
  gtk_window_present (GTK_WINDOW (self->priv->properties_dialog));
}

static void
show_machine_preferences_dialog (BtMachineCanvasItem * self)
{
  if (!self->priv->preferences_dialog) {
    self->priv->preferences_dialog =
        GTK_WIDGET (bt_machine_preferences_dialog_new (self->priv->machine));
    bt_edit_application_attach_child_window (self->priv->app,
        GTK_WINDOW (self->priv->preferences_dialog));
    gtk_widget_show_all (self->priv->preferences_dialog);
    g_signal_connect (self->priv->preferences_dialog, "destroy",
        G_CALLBACK (on_machine_preferences_dialog_destroy), (gpointer) self);
    g_object_notify ((GObject *) self, "preferences-dialog");
  }
  gtk_window_present (GTK_WINDOW (self->priv->preferences_dialog));
}

static void
show_machine_analyzer_dialog (BtMachineCanvasItem * self)
{
  if (!self->priv->analysis_dialog) {
    self->priv->analysis_dialog =
        GTK_WIDGET (bt_signal_analysis_dialog_new (GST_BIN (self->
                priv->machine)));
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
on_signal_analysis_dialog_destroy (GtkWidget * widget, gpointer user_data)
{
  BtMachineCanvasItem *self = BT_MACHINE_CANVAS_ITEM (user_data);

  GST_INFO ("signal analysis dialog destroy occurred");
  self->priv->analysis_dialog = NULL;
  g_object_notify ((GObject *) self, "analysis-dialog");
  // remember open/closed state
  g_hash_table_remove (self->priv->properties, "analyzer-shown");
}


static void
on_song_is_playing_notify (const BtSong * song, GParamSpec * arg,
    gpointer user_data)
{
  BtMachineCanvasItem *self = BT_MACHINE_CANVAS_ITEM (user_data);

  g_object_get ((gpointer) song, "is-playing", &self->priv->is_playing, NULL);
  if (!self->priv->is_playing) {
    self->priv->skip_output_level = FALSE;
    g_object_set (self->priv->output_meter, "y", MACHINE_METER_BASE,
        "height", 0.0, NULL);
    self->priv->skip_input_level = FALSE;
    g_object_set (self->priv->input_meter, "y", MACHINE_METER_BASE,
        "height", 0.0, NULL);
  }
}

typedef struct
{
  BtMachineCanvasItem *self;
  ClutterActor *meter;
  gdouble peak;
} BtUpdateIdleData;

#define MAKE_UPDATE_IDLE_DATA(data,self,meter,peak) G_STMT_START { \
  data=g_slice_new(BtUpdateIdleData); \
  data->self=self; \
  data->meter=meter; \
  data->peak=peak; \
  g_mutex_lock(&self->priv->lock); \
  g_object_add_weak_pointer((GObject *)self,(gpointer *)(&data->self)); \
  g_mutex_unlock(&self->priv->lock); \
} G_STMT_END

#define FREE_UPDATE_IDLE_DATA(data) G_STMT_START { \
  if(data->self) { \
    g_mutex_lock(&data->self->priv->lock); \
    g_object_remove_weak_pointer((gpointer)data->self,(gpointer *)(&data->self)); \
    g_mutex_unlock(&data->self->priv->lock); \
  } \
  g_slice_free(BtUpdateIdleData,data); \
} G_STMT_END


static gboolean
on_delayed_idle_machine_level_change (gpointer user_data)
{
  BtUpdateIdleData *data = (BtUpdateIdleData *) user_data;
  BtMachineCanvasItem *self = data->self;

  if (self && self->priv->is_playing) {
    const gdouble h = (MACHINE_METER_HEIGHT * data->peak);

    g_object_set (data->meter, "y", MACHINE_METER_BASE - h, "height", h, NULL);
  }
  FREE_UPDATE_IDLE_DATA (data);
  return FALSE;
}

static gboolean
on_delayed_machine_level_change (GstClock * clock, GstClockTime time,
    GstClockID id, gpointer user_data)
{
  // the callback is called from a clock thread
  if (GST_CLOCK_TIME_IS_VALID (time))
    g_idle_add_full (G_PRIORITY_HIGH, on_delayed_idle_machine_level_change,
        user_data, NULL);
  else {
    BtUpdateIdleData *data = (BtUpdateIdleData *) user_data;
    FREE_UPDATE_IDLE_DATA (data);
  }
  return TRUE;
}

static void
on_machine_level_change (GstBus * bus, GstMessage * message, gpointer user_data)
{
  const GstStructure *s = gst_message_get_structure (message);
  const GQuark name_id = gst_structure_get_name_id (s);

  if (name_id == bus_msg_level_quark) {
    BtMachineCanvasItem *self = BT_MACHINE_CANVAS_ITEM (user_data);
    GstElement *level = GST_ELEMENT (GST_MESSAGE_SRC (message));

    // check if its our level-meter
    if ((level == self->priv->output_level) ||
        (level == self->priv->input_level)) {
      GstClockTime waittime = bt_gst_analyzer_get_waittime (level, s, TRUE);
      if (GST_CLOCK_TIME_IS_VALID (waittime)) {
        ClutterActor *meter = NULL;
        gdouble peak;
        gint new_skip = 0, old_skip = 0;

        // check the value and calculate the average for the channels
        peak =
            bt_gst_level_message_get_aggregated_field (s, "peak",
            LOW_VUMETER_VAL);
        // check if we are very loud
        if (peak > 0.0) {
          new_skip = 2;         // beyond max level
          peak = 0.0;
        } else
          peak = peak / LOW_VUMETER_VAL;
        // check if we a silent
        if (peak >= 1.0) {
          new_skip = 1;         // below min level
          peak = 1.0;
        }
        peak = 1.0 - peak;      // invert since it was -db
        // skip *updates* if we are still below LOW_VUMETER_VAL or beyond 0.0
        if (level == self->priv->output_level) {
          meter = self->priv->output_meter;
          old_skip = self->priv->skip_output_level;
          self->priv->skip_output_level = new_skip;
        } else if (level == self->priv->input_level) {
          meter = self->priv->input_meter;
          old_skip = self->priv->skip_input_level;
          self->priv->skip_input_level = new_skip;
        }
        if (!old_skip || !new_skip || old_skip != new_skip) {
          BtUpdateIdleData *data;
          GstClockID clock_id;
          GstClockReturn clk_ret;

          waittime += gst_element_get_base_time (level);
          clock_id = gst_clock_new_single_shot_id (self->priv->clock, waittime);
          MAKE_UPDATE_IDLE_DATA (data, self, meter, peak);
          if ((clk_ret = gst_clock_id_wait_async (clock_id,
                      on_delayed_machine_level_change,
                      (gpointer) data, NULL)) != GST_CLOCK_OK) {
            GST_WARNING_OBJECT (level, "clock wait failed: %d", clk_ret);
            FREE_UPDATE_IDLE_DATA (data);
          }
          gst_clock_id_unref (clock_id);
        }
        // just for counting
        //else GST_WARNING_OBJECT(level,"skipping level update");
      }
    }
  }
}

static void
on_machine_parent_changed (GstObject * object, GParamSpec * arg,
    gpointer user_data)
{
  ClutterActor *self = CLUTTER_ACTOR (user_data);

  if (GST_OBJECT_PARENT (object)) {
    clutter_actor_clear_effects (self);
  } else {
    clutter_actor_add_effect (self, clutter_desaturate_effect_new (0.8));
  }
}

static void
on_machine_state_changed_idle (BtMachine * machine, GParamSpec * arg,
    gpointer user_data)
{
  BtMachineCanvasItem *self = BT_MACHINE_CANVAS_ITEM (user_data);
  BtMachineState state;

  g_object_get (machine, "state", &state, NULL);
  GST_INFO_OBJECT (machine, "new state is %d", state);

  update_machine_graphics (self);

  switch (state) {
    case BT_MACHINE_STATE_NORMAL:
      if (self->priv->menu_item_mute
          && gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (self->
                  priv->menu_item_mute))) {
        g_signal_handler_block (self->priv->menu_item_mute,
            self->priv->id_mute);
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (self->
                priv->menu_item_mute), FALSE);
        g_signal_handler_unblock (self->priv->menu_item_mute,
            self->priv->id_mute);
      }
      if (self->priv->menu_item_solo
          && gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (self->
                  priv->menu_item_solo))) {
        g_signal_handler_block (self->priv->menu_item_solo,
            self->priv->id_solo);
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (self->
                priv->menu_item_solo), FALSE);
        g_signal_handler_unblock (self->priv->menu_item_solo,
            self->priv->id_solo);
      }
      if (self->priv->menu_item_bypass
          && gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (self->
                  priv->menu_item_bypass))) {
        g_signal_handler_block (self->priv->menu_item_bypass,
            self->priv->id_bypass);
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (self->
                priv->menu_item_bypass), FALSE);
        g_signal_handler_unblock (self->priv->menu_item_bypass,
            self->priv->id_bypass);
      }
      break;
    case BT_MACHINE_STATE_MUTE:
      if (self->priv->menu_item_mute
          && !gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (self->
                  priv->menu_item_mute))) {
        g_signal_handler_block (self->priv->menu_item_mute,
            self->priv->id_mute);
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (self->
                priv->menu_item_mute), TRUE);
        g_signal_handler_unblock (self->priv->menu_item_mute,
            self->priv->id_mute);
      }
      if (self->priv->menu_item_solo
          && gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (self->
                  priv->menu_item_solo))) {
        g_signal_handler_block (self->priv->menu_item_solo,
            self->priv->id_solo);
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (self->
                priv->menu_item_solo), FALSE);
        g_signal_handler_unblock (self->priv->menu_item_solo,
            self->priv->id_solo);
      }
      if (self->priv->menu_item_bypass
          && gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (self->
                  priv->menu_item_bypass))) {
        g_signal_handler_block (self->priv->menu_item_bypass,
            self->priv->id_bypass);
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (self->
                priv->menu_item_bypass), FALSE);
        g_signal_handler_unblock (self->priv->menu_item_bypass,
            self->priv->id_bypass);
      }
      break;
    case BT_MACHINE_STATE_SOLO:
      if (self->priv->menu_item_mute
          && gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (self->
                  priv->menu_item_mute))) {
        g_signal_handler_block (self->priv->menu_item_mute,
            self->priv->id_mute);
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (self->
                priv->menu_item_mute), FALSE);
        g_signal_handler_unblock (self->priv->menu_item_mute,
            self->priv->id_mute);
      }
      if (self->priv->menu_item_solo
          && !gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (self->
                  priv->menu_item_solo))) {
        g_signal_handler_block (self->priv->menu_item_solo,
            self->priv->id_solo);
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (self->
                priv->menu_item_solo), TRUE);
        g_signal_handler_unblock (self->priv->menu_item_solo,
            self->priv->id_solo);
      }
      if (self->priv->menu_item_bypass
          && gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (self->
                  priv->menu_item_bypass))) {
        g_signal_handler_block (self->priv->menu_item_bypass,
            self->priv->id_bypass);
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (self->
                priv->menu_item_bypass), FALSE);
        g_signal_handler_unblock (self->priv->menu_item_bypass,
            self->priv->id_bypass);
      }
      break;
    case BT_MACHINE_STATE_BYPASS:
      if (self->priv->menu_item_mute
          && gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (self->
                  priv->menu_item_mute))) {
        g_signal_handler_block (self->priv->menu_item_mute,
            self->priv->id_mute);
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (self->
                priv->menu_item_mute), FALSE);
        g_signal_handler_unblock (self->priv->menu_item_mute,
            self->priv->id_mute);
      }
      if (self->priv->menu_item_solo
          && gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (self->
                  priv->menu_item_solo))) {
        g_signal_handler_block (self->priv->menu_item_solo,
            self->priv->id_solo);
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (self->
                priv->menu_item_solo), FALSE);
        g_signal_handler_unblock (self->priv->menu_item_solo,
            self->priv->id_solo);
      }
      if (self->priv->menu_item_bypass
          && !gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (self->
                  priv->menu_item_bypass))) {
        g_signal_handler_block (self->priv->menu_item_bypass,
            self->priv->id_bypass);
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (self->
                priv->menu_item_bypass), TRUE);
        g_signal_handler_unblock (self->priv->menu_item_bypass,
            self->priv->id_bypass);
      }
      break;
    default:
      GST_WARNING ("invalid machine state: %d", state);
      break;
  }
}

static void
on_machine_state_changed (GObject * obj, GParamSpec * arg, gpointer user_data)
{
  bt_notify_idle_dispatch (obj, arg, user_data,
      (BtNotifyFunc) on_machine_state_changed_idle);
}

static void
on_machine_properties_dialog_destroy (GtkWidget * widget, gpointer user_data)
{
  BtMachineCanvasItem *self = BT_MACHINE_CANVAS_ITEM (user_data);

  GST_INFO ("machine properties dialog destroy occurred");
  self->priv->properties_dialog = NULL;
  g_object_notify ((GObject *) self, "properties-dialog");
  // remember open/closed state
  g_hash_table_remove (self->priv->properties, "properties-shown");
}

static void
on_machine_preferences_dialog_destroy (GtkWidget * widget, gpointer user_data)
{
  BtMachineCanvasItem *self = BT_MACHINE_CANVAS_ITEM (user_data);

  GST_INFO ("machine preferences dialog destroy occurred");
  self->priv->preferences_dialog = NULL;
  g_object_notify ((GObject *) self, "preferences-dialog");
}

static void
on_context_menu_mute_toggled (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMachineCanvasItem *self = BT_MACHINE_CANVAS_ITEM (user_data);

  GST_INFO ("context_menu mute toggled");

  if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem))) {
    g_object_set (self->priv->machine, "state", BT_MACHINE_STATE_MUTE, NULL);
  } else {
    g_object_set (self->priv->machine, "state", BT_MACHINE_STATE_NORMAL, NULL);
  }
  bt_machine_update_default_state_value (self->priv->machine);
  bt_edit_application_set_song_unsaved (self->priv->app);
}

static void
on_context_menu_solo_toggled (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMachineCanvasItem *self = BT_MACHINE_CANVAS_ITEM (user_data);

  GST_INFO ("context_menu solo toggled");

  if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem))) {
    g_object_set (self->priv->machine, "state", BT_MACHINE_STATE_SOLO, NULL);
  } else {
    g_object_set (self->priv->machine, "state", BT_MACHINE_STATE_NORMAL, NULL);
  }
  bt_machine_update_default_state_value (self->priv->machine);
  bt_edit_application_set_song_unsaved (self->priv->app);
}

static void
on_context_menu_bypass_toggled (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMachineCanvasItem *self = BT_MACHINE_CANVAS_ITEM (user_data);

  GST_INFO ("context_menu bypass toggled");

  if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem))) {
    g_object_set (self->priv->machine, "state", BT_MACHINE_STATE_BYPASS, NULL);
  } else {
    g_object_set (self->priv->machine, "state", BT_MACHINE_STATE_NORMAL, NULL);
  }
  bt_machine_update_default_state_value (self->priv->machine);
  bt_edit_application_set_song_unsaved (self->priv->app);
}

static void
on_context_menu_properties_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMachineCanvasItem *self = BT_MACHINE_CANVAS_ITEM (user_data);

  show_machine_properties_dialog (self);
}

static void
on_context_menu_preferences_activate (GtkMenuItem * menuitem,
    gpointer user_data)
{
  BtMachineCanvasItem *self = BT_MACHINE_CANVAS_ITEM (user_data);

  show_machine_preferences_dialog (self);
}

static void
on_context_menu_rename_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMachineCanvasItem *self = BT_MACHINE_CANVAS_ITEM (user_data);

  GST_INFO ("context_menu rename event occurred");
  bt_main_page_machines_rename_machine (self->priv->main_page_machines,
      self->priv->machine);
}

static void
on_context_menu_delete_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMachineCanvasItem *self = BT_MACHINE_CANVAS_ITEM (user_data);

  GST_INFO ("context_menu delete event occurred for machine : %p",
      self->priv->machine);
  bt_main_page_machines_delete_machine (self->priv->main_page_machines,
      self->priv->machine);
}

static void
on_context_menu_connect_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMachineCanvasItem *self = BT_MACHINE_CANVAS_ITEM (user_data);

  g_signal_emit (self, signals[START_CONNECT], 0);
}

static void
on_context_menu_analysis_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMachineCanvasItem *self = BT_MACHINE_CANVAS_ITEM (user_data);

  show_machine_analyzer_dialog (self);
}

static void
on_context_menu_help_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMachineCanvasItem *self = BT_MACHINE_CANVAS_ITEM (user_data);

  // show help for machine
  gtk_show_uri_simple (GTK_WIDGET (self->priv->main_page_machines),
      self->priv->help_uri);
}

static void
on_context_menu_about_activate (GtkMenuItem * menuitem, gpointer user_data)
{
  BtMachineCanvasItem *self = BT_MACHINE_CANVAS_ITEM (user_data);
  BtMainWindow *main_window;

  // show info about machine
  g_object_get (self->priv->app, "main-window", &main_window, NULL);
  bt_machine_action_about (self->priv->machine, main_window);
  g_object_unref (main_window);
}

//-- helper methods

#if 0
static gboolean
bt_machine_canvas_item_is_over_state_switch (const BtMachineCanvasItem * self,
    GdkEvent * event)
{
  GnomeCanvas *canvas;
  ClutterActor *ci, *pci;
  gboolean res = FALSE;

  g_object_get (self->priv->main_page_machines, "canvas", &canvas, NULL);
  if ((ci =
          gnome_canvas_get_item_at (canvas, event->button.x,
              event->button.y))) {
    g_object_get (ci, "parent", &pci, NULL);
    //GST_DEBUG("ci=%p : self=%p, self->box=%p, self->state_switch=%p",ci,self,self->priv->box,self->priv->state_switch);
    if ((ci == self->priv->state_switch)
        || (ci == self->priv->state_mute) || (pci == self->priv->state_mute)
        || (ci == self->priv->state_solo)
        || (ci == self->priv->state_bypass)
        || (pci == self->priv->state_bypass)) {
      res = TRUE;
    }
    g_object_unref (pci);
  }
  g_object_unref (canvas);
  return res;
}
#endif

// interaction control helper

static gboolean
bt_machine_canvas_item_init_context_menu (const BtMachineCanvasItem * self)
{
  GtkWidget *menu_item, *label;
  BtMachine *machine = self->priv->machine;
  BtMachineState state;
  gulong num_property_params;

  g_object_get (machine, "state", &state, "prefs-params", &num_property_params,
      NULL);

  self->priv->menu_item_mute = menu_item =
      gtk_check_menu_item_new_with_label (_("Mute"));
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item),
      (state == BT_MACHINE_STATE_MUTE));
  gtk_menu_shell_append (GTK_MENU_SHELL (self->priv->context_menu), menu_item);
  gtk_widget_show (menu_item);
  self->priv->id_mute =
      g_signal_connect (menu_item, "toggled",
      G_CALLBACK (on_context_menu_mute_toggled), (gpointer) self);
  if (BT_IS_SOURCE_MACHINE (machine)) {
    self->priv->menu_item_solo = menu_item =
        gtk_check_menu_item_new_with_label (_("Solo"));
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item),
        (state == BT_MACHINE_STATE_SOLO));
    gtk_menu_shell_append (GTK_MENU_SHELL (self->priv->context_menu),
        menu_item);
    gtk_widget_show (menu_item);
    self->priv->id_solo =
        g_signal_connect (menu_item, "toggled",
        G_CALLBACK (on_context_menu_solo_toggled), (gpointer) self);
  }
  if (BT_IS_PROCESSOR_MACHINE (machine)) {
    self->priv->menu_item_bypass = menu_item =
        gtk_check_menu_item_new_with_label (_("Bypass"));
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item),
        (state == BT_MACHINE_STATE_BYPASS));
    gtk_menu_shell_append (GTK_MENU_SHELL (self->priv->context_menu),
        menu_item);
    gtk_widget_show (menu_item);
    self->priv->id_bypass =
        g_signal_connect (menu_item, "toggled",
        G_CALLBACK (on_context_menu_bypass_toggled), (gpointer) self);
  }

  menu_item = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (GTK_MENU_SHELL (self->priv->context_menu), menu_item);
  gtk_widget_show (menu_item);

  // dynamic part
  menu_item = gtk_menu_item_new_with_mnemonic (_("_Properties"));
  gtk_menu_shell_append (GTK_MENU_SHELL (self->priv->context_menu), menu_item);
  // make this menu item bold (default)
  label = gtk_bin_get_child (GTK_BIN (menu_item));
  if (GTK_IS_LABEL (label)) {
    gchar *str =
        g_strconcat ("<b>", gtk_label_get_label (GTK_LABEL (label)), "</b>",
        NULL);
    if (gtk_label_get_use_underline (GTK_LABEL (label))) {
      gtk_label_set_markup_with_mnemonic (GTK_LABEL (label), str);
    } else {
      gtk_label_set_markup (GTK_LABEL (label), str);
    }
    g_free (str);
  } else {
    GST_WARNING ("expecting a GtkLabel as a first child");
  }
  gtk_widget_show (menu_item);
  g_signal_connect (menu_item, "activate",
      G_CALLBACK (on_context_menu_properties_activate), (gpointer) self);

  // static part
  menu_item = gtk_menu_item_new_with_mnemonic (_("_Preferences"));
  gtk_menu_shell_append (GTK_MENU_SHELL (self->priv->context_menu), menu_item);
  gtk_widget_show (menu_item);
  g_signal_connect (menu_item, "activate",
      G_CALLBACK (on_context_menu_preferences_activate), (gpointer) self);
  gtk_widget_set_sensitive (menu_item, (num_property_params != 0));

  menu_item = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (GTK_MENU_SHELL (self->priv->context_menu), menu_item);
  gtk_widget_show (menu_item);

  menu_item = gtk_menu_item_new_with_label (_("Rename…"));
  gtk_menu_shell_append (GTK_MENU_SHELL (self->priv->context_menu), menu_item);
  gtk_widget_show (menu_item);
  g_signal_connect (menu_item, "activate",
      G_CALLBACK (on_context_menu_rename_activate), (gpointer) self);
  if (!BT_IS_SINK_MACHINE (machine)) {
    menu_item = gtk_menu_item_new_with_mnemonic (_("_Delete"));
    gtk_menu_shell_append (GTK_MENU_SHELL (self->priv->context_menu),
        menu_item);
    gtk_widget_show (menu_item);
    g_signal_connect (menu_item, "activate",
        G_CALLBACK (on_context_menu_delete_activate), (gpointer) self);

    menu_item = gtk_separator_menu_item_new ();
    gtk_menu_shell_append (GTK_MENU_SHELL (self->priv->context_menu),
        menu_item);
    gtk_widget_show (menu_item);

    menu_item = gtk_menu_item_new_with_label (_("Connect machines"));
    gtk_menu_shell_append (GTK_MENU_SHELL (self->priv->context_menu),
        menu_item);
    gtk_widget_show (menu_item);
    g_signal_connect (menu_item, "activate",
        G_CALLBACK (on_context_menu_connect_activate), (gpointer) self);

  } else {
    menu_item = gtk_menu_item_new_with_label (_("Signal Analysis…"));
    gtk_menu_shell_append (GTK_MENU_SHELL (self->priv->context_menu),
        menu_item);
    gtk_widget_show (menu_item);
    g_signal_connect (menu_item, "activate",
        G_CALLBACK (on_context_menu_analysis_activate), (gpointer) self);
  }

  if (BT_IS_SOURCE_MACHINE (machine)) {
    glong pi = -1;
    BtParameterGroup *pg;

    GST_INFO_OBJECT (machine,
        "check if source machine has a trigger parameter");

    // find trigger property in global or first voice params
    if ((pg = bt_machine_get_global_param_group (machine))) {
      if ((pi = bt_parameter_group_get_trigger_param_index (pg)) == -1) {
        if (bt_machine_is_polyphonic (machine)) {
          if ((pg = bt_machine_get_voice_param_group (machine, 0))) {
            pi = bt_parameter_group_get_trigger_param_index (pg);
          }
        }
      }
    }
    if (pi != -1) {
      const gchar *property_name = bt_parameter_group_get_param_name (pg, pi);
      GObject *param_parent = bt_parameter_group_get_param_parent (pg, pi);

      GST_INFO_OBJECT (machine, "attaching controller menu for %s",
          property_name);

      menu_item = gtk_separator_menu_item_new ();
      gtk_menu_shell_append (GTK_MENU_SHELL (self->priv->context_menu),
          menu_item);
      gtk_widget_show (menu_item);

      // this will create the following menu layout:
      //   play with
      //     bind controller
      //       device1
      //     unbind controller
      //     unbind all
      // it would be nice to just have:
      //   bind controller
      //     device1
      //   unbind controller
      // but maybe that is okay, as this way we can also unbind-all from here.
      GtkWidget *menu = (GtkWidget *) bt_interaction_controller_menu_new
          (BT_INTERACTION_CONTROLLER_RANGE_MENU, self->priv->machine);

      menu_item = gtk_menu_item_new_with_label (_("Play with"));
      gtk_menu_shell_append (GTK_MENU_SHELL (self->priv->context_menu),
          menu_item);
      gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), menu);
      gtk_widget_show (menu_item);

      g_object_set (menu, "selected-object", param_parent,
          "selected-parameter-group", pg, "selected-property-name",
          property_name, NULL);
    }
  }

  menu_item = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (GTK_MENU_SHELL (self->priv->context_menu), menu_item);
  gtk_widget_show (menu_item);

  menu_item = gtk_menu_item_new_with_mnemonic (_("_Help"));
  gtk_menu_shell_append (GTK_MENU_SHELL (self->priv->context_menu), menu_item);
  if (!self->priv->help_uri) {
    gtk_widget_set_sensitive (menu_item, FALSE);
  } else {
    g_signal_connect (menu_item, "activate",
        G_CALLBACK (on_context_menu_help_activate), (gpointer) self);
  }
  gtk_widget_show (menu_item);

  menu_item = gtk_menu_item_new_with_mnemonic (_("_About"));
  gtk_menu_shell_append (GTK_MENU_SHELL (self->priv->context_menu), menu_item);
  gtk_widget_show (menu_item);
  g_signal_connect (menu_item, "activate",
      G_CALLBACK (on_context_menu_about_activate), (gpointer) self);

  return TRUE;
}

//-- constructor methods

/**
 * bt_machine_canvas_item_new:
 * @main_page_machines: the machine page the new item belongs to
 * @machine: the machine for which a canvas item should be created
 * @xpos: the horizontal location
 * @ypos: the vertical location
 * @zoom: the zoom ratio
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtMachineCanvasItem *
bt_machine_canvas_item_new (const BtMainPageMachines * main_page_machines,
    BtMachine * machine, gdouble xpos, gdouble ypos, gdouble zoom)
{
  BtMachineCanvasItem *self;
  ClutterActor *canvas;

  g_object_get ((gpointer) main_page_machines, "canvas", &canvas, NULL);

  self = BT_MACHINE_CANVAS_ITEM (g_object_new (BT_TYPE_MACHINE_CANVAS_ITEM,
          "machines-page", main_page_machines, "machine", machine, "x", xpos,
          "y", ypos, "zoom", zoom, "reactive", TRUE, NULL));

  GST_DEBUG ("machine canvas item created, %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (self));

  clutter_actor_add_child (canvas, (ClutterActor *) self);

  GST_DEBUG ("machine canvas item added, %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (self));

  g_object_unref (canvas);
  return self;
}

//-- methods

/**
 * bt_machine_show_properties_dialog:
 * @machine: machine to show the dialog for
 *
 * Shows the machine properties dialog.
 *
 * Since: 0.6
 */
void
bt_machine_show_properties_dialog (BtMachine * machine)
{
  BtMachineCanvasItem *self =
      BT_MACHINE_CANVAS_ITEM (g_object_get_qdata ((GObject *) machine,
          machine_canvas_item_quark));

  show_machine_properties_dialog (self);
}

/**
 * bt_machine_show_preferences_dialog:
 * @machine: machine to show the dialog for
 *
 * Shows the machine preferences dialog.
 *
 * Since: 0.6
 */
void
bt_machine_show_preferences_dialog (BtMachine * machine)
{
  BtMachineCanvasItem *self =
      BT_MACHINE_CANVAS_ITEM (g_object_get_qdata ((GObject *) machine,
          machine_canvas_item_quark));

  show_machine_preferences_dialog (self);
}

/**
 * bt_machine_show_analyzer_dialog:
 * @machine: machine to show the dialog for
 *
 * Shows the machine signal analysis dialog.
 *
 * Since: 0.6
 */
void
bt_machine_show_analyzer_dialog (BtMachine * machine)
{
  BtMachineCanvasItem *self =
      BT_MACHINE_CANVAS_ITEM (g_object_get_qdata ((GObject *) machine,
          machine_canvas_item_quark));

  show_machine_analyzer_dialog (self);
}

//-- wrapper

//-- class internals

static void
bt_machine_canvas_item_constructed (GObject * object)
{
  BtMachineCanvasItem *self = BT_MACHINE_CANVAS_ITEM (object);
  gchar *id, *prop;
  PangoAttrList *pango_attrs;
  ClutterColor *meter_bg;

  if (G_OBJECT_CLASS (bt_machine_canvas_item_parent_class)->constructed)
    G_OBJECT_CLASS (bt_machine_canvas_item_parent_class)->constructed (object);

  g_object_get (self->priv->machine, "id", &id, NULL);

  // add machine components
  // the body
  self->priv->image = clutter_image_new ();
  update_machine_graphics (self);
  clutter_actor_set_content_scaling_filters ((ClutterActor *) self,
      CLUTTER_SCALING_FILTER_TRILINEAR, CLUTTER_SCALING_FILTER_LINEAR);
  clutter_actor_set_size ((ClutterActor *) self, MACHINE_W, MACHINE_H);
  clutter_actor_set_pivot_point ((ClutterActor *) self, 0.5, 0.5);
  clutter_actor_set_translation ((ClutterActor *) self, (MACHINE_W / -2.0),
      (MACHINE_H / -2.0), 0.0);
  clutter_actor_set_content ((ClutterActor *) self, self->priv->image);

  // the name label
  // TODO(ensonic): use MACHINE_LABEL_HEIGHT (7)
  // TODO(ensonic): when zooming, the font gets blurry :/
  self->priv->label = clutter_text_new_with_text ("Sans 8px", id);
  pango_attrs = pango_attr_list_new ();
  // PANGO_STRETCH_{,EXTRA,ULTRA}_CONDENSED
  pango_attr_list_insert (pango_attrs,
      pango_attr_stretch_new (PANGO_STRETCH_EXTRA_CONDENSED));
  g_object_set (self->priv->label,
      "activatable", FALSE,
      "attributes", pango_attrs,
      "ellipsize", PANGO_ELLIPSIZE_END,
      "line-alignment", PANGO_ALIGN_CENTER, "selectable", FALSE, NULL);
  pango_attr_list_unref (pango_attrs);
  // machine is set via CONSTRUCT_ONLY prop
  g_assert (self->priv->machine);
  g_object_bind_property (self->priv->machine, "id", self->priv->label,
      "text", G_BINDING_SYNC_CREATE);

  // we want that as a max-width for clipping :/
  clutter_actor_set_width (self->priv->label, MACHINE_LABEL_WIDTH);
  clutter_actor_set_pivot_point (self->priv->label, 0.5, 0.5);
  clutter_actor_add_child ((ClutterActor *) self, self->priv->label);
  clutter_actor_set_position (self->priv->label,
      (MACHINE_W - MACHINE_LABEL_WIDTH) / 2.0,
      MACHINE_LABEL_BASE - (MACHINE_LABEL_HEIGHT / 2.0));

  // the meters
  meter_bg = clutter_color_new (0x5f, 0x5f, 0x5f, 0xff);
  //if(!BT_IS_SOURCE_MACHINE(self->priv->machine)) {
  self->priv->input_meter = g_object_new (CLUTTER_TYPE_ACTOR,
      "background-color", meter_bg,
      "x", MACHINE_METER_LEFT, "y", MACHINE_METER_BASE,
      "width", MACHINE_METER_WIDTH, "height", 0.0, NULL);
  clutter_actor_add_child ((ClutterActor *) self, self->priv->input_meter);
  //}
  //if(!BT_IS_SINK_MACHINE(self->priv->machine)) {
  self->priv->output_meter = g_object_new (CLUTTER_TYPE_ACTOR,
      "background-color", meter_bg,
      "x", MACHINE_METER_RIGHT, "y", MACHINE_METER_BASE,
      "width", MACHINE_METER_WIDTH, "height", 0.0, NULL);
  clutter_actor_add_child ((ClutterActor *) self, self->priv->output_meter);
  //}
  clutter_color_free (meter_bg);

  g_free (id);
  if (!GST_OBJECT_PARENT ((GstObject *) self->priv->machine)) {
    on_machine_parent_changed ((GstObject *) self->priv->machine, NULL, self);
  }

  prop =
      (gchar *) g_hash_table_lookup (self->priv->properties,
      "properties-shown");
  if (prop && prop[0] == '1' && prop[1] == '\0') {
    self->priv->properties_dialog =
        GTK_WIDGET (bt_machine_properties_dialog_new (self->priv->machine));
    bt_edit_application_attach_child_window (self->priv->app,
        GTK_WINDOW (self->priv->properties_dialog));
    gtk_widget_show_all (self->priv->properties_dialog);
    g_signal_connect (self->priv->properties_dialog, "destroy",
        G_CALLBACK (on_machine_properties_dialog_destroy), (gpointer) self);
  }
  prop =
      (gchar *) g_hash_table_lookup (self->priv->properties, "analyzer-shown");
  if (prop && prop[0] == '1' && prop[1] == '\0') {
    if ((self->priv->analysis_dialog =
            GTK_WIDGET (bt_signal_analysis_dialog_new (GST_BIN (self->
                        priv->machine))))) {
      bt_edit_application_attach_child_window (self->priv->app,
          GTK_WINDOW (self->priv->analysis_dialog));
      gtk_widget_show_all (self->priv->analysis_dialog);
      g_signal_connect (self->priv->analysis_dialog, "destroy",
          G_CALLBACK (on_signal_analysis_dialog_destroy), (gpointer) self);
    }
  }
}

static void
bt_machine_canvas_item_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  BtMachineCanvasItem *self = BT_MACHINE_CANVAS_ITEM (object);
  return_if_disposed ();
  switch (property_id) {
    case MACHINE_CANVAS_ITEM_MACHINES_PAGE:
      g_value_set_object (value, self->priv->main_page_machines);
      break;
    case MACHINE_CANVAS_ITEM_MACHINE:
      GST_INFO ("getting machine : %" G_OBJECT_REF_COUNT_FMT,
          G_OBJECT_LOG_REF_COUNT (self->priv->machine));
      g_value_set_object (value, self->priv->machine);
      break;
    case MACHINE_CANVAS_ITEM_ZOOM:
      g_value_set_double (value, self->priv->zoom);
      break;
    case MACHINE_CANVAS_ITEM_PROPERTIES_DIALOG:
      g_value_set_object (value, self->priv->properties_dialog);
      break;
    case MACHINE_CANVAS_ITEM_PREFERENCES_DIALOG:
      g_value_set_object (value, self->priv->preferences_dialog);
      break;
    case MACHINE_CANVAS_ITEM_ANALYSIS_DIALOG:
      g_value_set_object (value, self->priv->analysis_dialog);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_machine_canvas_item_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  BtMachineCanvasItem *self = BT_MACHINE_CANVAS_ITEM (object);
  return_if_disposed ();
  switch (property_id) {
    case MACHINE_CANVAS_ITEM_MACHINES_PAGE:
      g_object_try_weak_unref (self->priv->main_page_machines);
      self->priv->main_page_machines =
          BT_MAIN_PAGE_MACHINES (g_value_get_object (value));
      g_object_try_weak_ref (self->priv->main_page_machines);
      break;
    case MACHINE_CANVAS_ITEM_MACHINE:{
      BtMachine *new_machine = (BtMachine *) g_value_get_object (value);
      if (new_machine != self->priv->machine) {
        GstElement *element;

        g_object_try_unref (self->priv->machine);
        self->priv->machine = g_object_ref (new_machine);

        GST_INFO ("set the machine %" G_OBJECT_REF_COUNT_FMT,
            G_OBJECT_LOG_REF_COUNT (self->priv->machine));
        g_object_set_qdata ((GObject *) self->priv->machine,
            machine_canvas_item_quark, (gpointer) self);
        g_object_get (self->priv->machine, "properties",
            &self->priv->properties, "machine", &element, NULL);

        self->priv->help_uri =
            gst_element_factory_get_metadata (gst_element_get_factory (element),
            GST_ELEMENT_METADATA_DOC_URI);
        gst_object_unref (element);

        bt_machine_canvas_item_init_context_menu (self);
        g_signal_connect_object (self->priv->machine, "notify::state",
            G_CALLBACK (on_machine_state_changed), (gpointer) self, 0);
        g_signal_connect_object (self->priv->machine, "notify::parent",
            G_CALLBACK (on_machine_parent_changed), (gpointer) self, 0);

        if (!BT_IS_SINK_MACHINE (self->priv->machine)) {
          if (bt_machine_enable_output_post_level (self->priv->machine)) {
            g_object_try_weak_unref (self->priv->output_level);
            g_object_get (self->priv->machine, "output-post-level",
                &self->priv->output_level, NULL);
            g_object_try_weak_ref (self->priv->output_level);
            gst_object_unref (self->priv->output_level);
          } else {
            GST_INFO ("enabling output level for machine failed");
          }
        }
        if (!BT_IS_SOURCE_MACHINE (self->priv->machine)) {
          if (bt_machine_enable_input_pre_level (self->priv->machine)) {
            g_object_try_weak_unref (self->priv->input_level);
            g_object_get (self->priv->machine, "input-pre-level",
                &self->priv->input_level, NULL);
            g_object_try_weak_ref (self->priv->input_level);
            gst_object_unref (self->priv->input_level);
          } else {
            GST_INFO ("enabling input level for machine failed");
          }
        }
      }
      break;
    }
    case MACHINE_CANVAS_ITEM_ZOOM:
      self->priv->zoom = g_value_get_double (value);
      GST_DEBUG ("set the zoom for machine_canvas_item: %f", self->priv->zoom);
      /* reload the svg icons, we do this to keep them sharp */
      if (self->priv->image) {
        update_machine_graphics (self);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_machine_canvas_item_dispose (GObject * object)
{
  BtMachineCanvasItem *self = BT_MACHINE_CANVAS_ITEM (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  GST_DEBUG ("machine: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (self->priv->machine));

  g_object_unref (self->priv->image);

  GST_INFO ("release the machine %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (self->priv->machine));

  g_object_try_weak_unref (self->priv->output_level);
  g_object_try_weak_unref (self->priv->input_level);
  g_object_try_unref (self->priv->machine);
  g_object_try_weak_unref (self->priv->main_page_machines);
  if (self->priv->clock)
    gst_object_unref (self->priv->clock);
  g_object_unref (self->priv->app);

  GST_DEBUG ("  unrefing done");

  if (self->priv->properties_dialog) {
    gtk_widget_destroy (self->priv->properties_dialog);
  }
  if (self->priv->preferences_dialog) {
    gtk_widget_destroy (self->priv->preferences_dialog);
  }
  if (self->priv->analysis_dialog) {
    gtk_widget_destroy (self->priv->analysis_dialog);
  }
  GST_DEBUG ("  destroying dialogs done");

  g_object_unref (self->priv->drag_cursor);

  gtk_widget_destroy (GTK_WIDGET (self->priv->context_menu));
  g_object_try_unref (self->priv->context_menu);

  GST_DEBUG ("  chaining up");
  G_OBJECT_CLASS (bt_machine_canvas_item_parent_class)->dispose (object);
  GST_DEBUG ("  done");
}

static void
bt_machine_canvas_item_finalize (GObject * object)
{
  BtMachineCanvasItem *self = BT_MACHINE_CANVAS_ITEM (object);

  GST_DEBUG ("!!!! self=%p", self);
  g_mutex_clear (&self->priv->lock);

  G_OBJECT_CLASS (bt_machine_canvas_item_parent_class)->finalize (object);
  GST_DEBUG ("  done");
}

typedef struct
{
  BtMachineCanvasItem *self;
  guint32 activate_time;
  gfloat mouse_x;
  gfloat mouse_y;
} BtEventIdleData;

static BtEventIdleData*
new_event_idle_data(BtMachineCanvasItem * self, ClutterEvent * event) {
  BtEventIdleData *data = g_slice_new(BtEventIdleData);
  data->self = self;
  data->activate_time = clutter_event_get_time (event);
  clutter_event_get_coords(event, &data->mouse_x, &data->mouse_y);
  return data;
}

void
free_event_idle_data(BtEventIdleData * data) {
  g_slice_free(BtEventIdleData,data);
}

static gboolean
popup_helper (gpointer user_data)
{
  BtEventIdleData *data = (BtEventIdleData *) user_data;
  BtMachineCanvasItem *self = data->self;

  GdkRectangle rect;
  rect.x = (gint)data->mouse_x;
  rect.y = (gint)data->mouse_y;
  rect.width = 0;
  rect.height = 0;

  free_event_idle_data(data);
								   
  GdkWindow* window = gtk_widget_get_window(bt_main_page_machines_get_canvas_widget(self->priv->main_page_machines));
  gtk_menu_popup_at_rect(self->priv->context_menu, window,
						 &rect, GDK_GRAVITY_SOUTH_EAST, GDK_GRAVITY_NORTH_WEST, NULL);
  return FALSE;
}

static gboolean on_captured_event (ClutterActor * citem, ClutterEvent * event,
    gpointer user_data);

static gboolean
bt_machine_canvas_item_event (ClutterActor * citem, ClutterEvent * event)
{
  BtMachineCanvasItem *self = BT_MACHINE_CANVAS_ITEM (citem);
  gboolean res = FALSE;

  //GST_INFO("event for machine occurred");

  switch (clutter_event_type (event)) {
    case CLUTTER_BUTTON_PRESS:{
      ClutterButtonEvent *button_event = (ClutterButtonEvent *) event;
      GST_DEBUG ("CLUTTER_BUTTON_PRESS: button=%d, clicks=%d, mod=0x%x",
          button_event->button, button_event->click_count,
          button_event->modifier_state);
      switch (button_event->button) {
        case 1:
          switch (button_event->click_count) {
            case 1:
              // we're connecting when shift is pressed
              if (!(button_event->modifier_state & CLUTTER_SHIFT_MASK)) {
                // dragx/y coords are world coords of button press
                self->priv->dragx = button_event->x;
                self->priv->dragy = button_event->y;
                // set some flags
                self->priv->dragging = TRUE;
                self->priv->moved = FALSE;
                self->priv->capture_id =
                    g_signal_connect_after (clutter_actor_get_stage (citem),
                    "captured-event", G_CALLBACK (on_captured_event), self);
                res = TRUE;
              }
              break;
            case 2:
              show_machine_properties_dialog (self);
              res = TRUE;
              break;
            default:
              break;
          }
          break;
        case 3:{
          g_idle_add (popup_helper, new_event_idle_data (self, event));
          res = TRUE;
          break;
        }
        default:
          break;
      }
      break;
    }
    case CLUTTER_MOTION:
      //GST_DEBUG("CLUTTER_MOTION: %f,%f",event->button.x,event->button.y);
      if (self->priv->dragging) {
        ClutterMotionEvent *motion_event = (ClutterMotionEvent *) event;
        gfloat dx, dy;
        if (!self->priv->moved) {
          ClutterActor *canvas;

          g_object_get (self->priv->main_page_machines, "canvas", &canvas,
              NULL);
          clutter_actor_set_opacity (citem, 160);
          clutter_actor_set_child_above_sibling (canvas, citem, NULL);
          g_object_unref (canvas);

          g_signal_emit (citem, signals[POSITION_CHANGED], 0,
              CLUTTER_BUTTON_PRESS);
        }
        dx = (motion_event->x - self->priv->dragx) / self->priv->zoom;
        dy = (motion_event->y - self->priv->dragy) / self->priv->zoom;
        clutter_actor_move_by (citem, dx, dy);
        g_signal_emit (citem, signals[POSITION_CHANGED], 0, CLUTTER_MOTION);
        self->priv->dragx = motion_event->x;
        self->priv->dragy = motion_event->y;
        self->priv->moved = TRUE;
        res = TRUE;
      }
      break;
    case CLUTTER_BUTTON_RELEASE:
      GST_DEBUG ("CLUTTER_BUTTON_RELEASE: %d", event->button.button);
      if (self->priv->dragging) {
        self->priv->dragging = FALSE;
        if (self->priv->capture_id) {
          g_signal_handler_disconnect (clutter_actor_get_stage (citem),
              self->priv->capture_id);
          self->priv->capture_id = 0;
        }
        if (self->priv->moved) {
          gfloat xc, yc;
          gdouble xr, yr;
          gchar str[G_ASCII_DTOSTR_BUF_SIZE];

          // change position properties of the machines
          g_object_get (citem, "x", &xc, "y", &yc, NULL);
          bt_main_page_machines_canvas_coords_to_relative (self->priv->
              main_page_machines, xc, yc, &xr, &yr);
          g_hash_table_insert (self->priv->properties, g_strdup ("xpos"),
              g_strdup (g_ascii_dtostr (str, G_ASCII_DTOSTR_BUF_SIZE, xr)));
          g_hash_table_insert (self->priv->properties, g_strdup ("ypos"),
              g_strdup (g_ascii_dtostr (str, G_ASCII_DTOSTR_BUF_SIZE, yr)));
          g_signal_emit (citem, signals[POSITION_CHANGED], 0,
              CLUTTER_BUTTON_RELEASE);
          clutter_actor_set_opacity (citem, 255);
          res = TRUE;
        }
        // if not moved, let event fall through to make
        // [context menu > connect] work
      } else {
#if 0
        if (self->priv->switching) {
          self->priv->switching = FALSE;
          // still over mode switch
          if (bt_machine_canvas_item_is_over_state_switch (self, event)) {
            guint modifier =
                (gulong) event->
                button.state & gtk_accelerator_get_default_mod_mask ();
            //gulong modifier=(gulong)event->button.state&(GDK_CONTROL_MASK|GDK_MOD4_MASK);
            GST_DEBUG
                ("  mode quad state switch, key_modifier is: 0x%x + mask: 0x%x -> 0x%x",
                event->button.state, (GDK_CONTROL_MASK | GDK_MOD4_MASK),
                modifier);
            switch (modifier) {
              case 0:
                g_object_set (self->priv->machine, "state",
                    BT_MACHINE_STATE_NORMAL, NULL);
                break;
              case GDK_CONTROL_MASK:
                g_object_set (self->priv->machine, "state",
                    BT_MACHINE_STATE_MUTE, NULL);
                break;
              case GDK_MOD4_MASK:
                g_object_set (self->priv->machine, "state",
                    BT_MACHINE_STATE_SOLO, NULL);
                break;
              case GDK_CONTROL_MASK | GDK_MOD4_MASK:
                g_object_set (self->priv->machine, "state",
                    BT_MACHINE_STATE_BYPASS, NULL);
                break;
            }
          }
        }
#endif
      }
      break;
    case CLUTTER_KEY_RELEASE:{
      ClutterKeyEvent *key_event = (ClutterKeyEvent *) event;
      GST_DEBUG ("CLUTTER_KEY_RELEASE: %d", key_event->keyval);
      switch (key_event->keyval) {
        case GDK_KEY_Menu:{
          g_idle_add (popup_helper, new_event_idle_data (self, event));
          res = TRUE;
          break;
        }
        default:
          break;
      }
      break;
    }
    default:
      break;
  }
  /* we don't want the click falling through to the parent canvas item, if we have handled it */
  if (!res) {
    if (CLUTTER_ACTOR_CLASS (bt_machine_canvas_item_parent_class)->event) {
      res = CLUTTER_ACTOR_CLASS (bt_machine_canvas_item_parent_class)->event
          (citem, event);
    }
  }
  //GST_INFO("event for machine occurred : %d",res);
  return res;
}

static gboolean
on_captured_event (ClutterActor * citem, ClutterEvent * event,
    gpointer user_data)
{
  return bt_machine_canvas_item_event ((ClutterActor *) user_data, event);
}


static void
bt_machine_canvas_item_init (BtMachineCanvasItem * self)
{
  BtSong *song;
  GstBin *bin;
  GstBus *bus;

  self->priv =
      G_TYPE_INSTANCE_GET_PRIVATE (self, BT_TYPE_MACHINE_CANVAS_ITEM,
      BtMachineCanvasItemPrivate);
  GST_DEBUG ("!!!! self=%p", self);
  self->priv->app = bt_edit_application_new ();

  g_object_get (self->priv->app, "song", &song, "bin", &bin, NULL);
  g_signal_connect_object (song, "notify::is-playing",
      G_CALLBACK (on_song_is_playing_notify), (gpointer) self, 0);
  bus = gst_element_get_bus (GST_ELEMENT (bin));
  g_signal_connect_object (bus, "sync-message::element",
      G_CALLBACK (on_machine_level_change), (gpointer) self, 0);
  gst_object_unref (bus);
  self->priv->clock = gst_pipeline_get_clock (GST_PIPELINE (bin));
  gst_object_unref (bin);
  g_object_unref (song);

  // generate the context menu
  self->priv->context_menu = GTK_MENU (g_object_ref_sink (gtk_menu_new ()));
  // the menu-items are generated in bt_machine_canvas_item_init_context_menu()

  // the cursor for dragging
  self->priv->drag_cursor =
      gdk_cursor_new_for_display (gdk_display_get_default (), GDK_FLEUR);

  self->priv->zoom = 1.0;

  g_mutex_init (&self->priv->lock);
}

static void
bt_machine_canvas_item_class_init (BtMachineCanvasItemClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *citem_class = CLUTTER_ACTOR_CLASS (klass);

  bus_msg_level_quark = g_quark_from_static_string ("level");
  machine_canvas_item_quark =
      g_quark_from_static_string ("machine-canvas-item");

  g_type_class_add_private (klass, sizeof (BtMachineCanvasItemPrivate));

  gobject_class->constructed = bt_machine_canvas_item_constructed;
  gobject_class->set_property = bt_machine_canvas_item_set_property;
  gobject_class->get_property = bt_machine_canvas_item_get_property;
  gobject_class->dispose = bt_machine_canvas_item_dispose;
  gobject_class->finalize = bt_machine_canvas_item_finalize;

  citem_class->event = bt_machine_canvas_item_event;

  /**
   * BtMachineCanvasItem::position-changed:
   * @self: the machine-canvas-item object that emitted the signal
   * @event: the clutter event type, e.g. %CLUTTER_BUTTON_PRESS
   *
   * Signals that item has been moved around. The new position can be read from
   * the canvas item.
   */
  signals[POSITION_CHANGED] =
      g_signal_new ("position-changed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 0, NULL,
      NULL, g_cclosure_marshal_VOID__ENUM, G_TYPE_NONE, 1,
      CLUTTER_TYPE_EVENT_TYPE);
  /**
   * BtMachineCanvasItem::start-connect:
   * @self: the machine-canvas-item object that emitted the signal
   *
   * Signals that a connect should be made starting from this machine.
   */
  signals[START_CONNECT] =
      g_signal_new ("start-connect", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 0, NULL,
      NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

  g_object_class_install_property (gobject_class,
      MACHINE_CANVAS_ITEM_MACHINES_PAGE, g_param_spec_object ("machines-page",
          "machines-page contruct prop",
          "Set application object, the window belongs to",
          BT_TYPE_MAIN_PAGE_MACHINES,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, MACHINE_CANVAS_ITEM_MACHINE,
      g_param_spec_object ("machine", "machine contruct prop",
          "Set machine object, the item belongs to", BT_TYPE_MACHINE,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, MACHINE_CANVAS_ITEM_ZOOM,
      g_param_spec_double ("zoom",
          "zoom prop",
          "Set zoom ratio for the machine item",
          0.0, 100.0, 1.0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      MACHINE_CANVAS_ITEM_PROPERTIES_DIALOG,
      g_param_spec_object ("properties-dialog", "properties dialog prop",
          "Get the the properties dialog if shown",
          BT_TYPE_MACHINE_PROPERTIES_DIALOG,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      MACHINE_CANVAS_ITEM_PREFERENCES_DIALOG,
      g_param_spec_object ("preferences-dialog", "preferences dialog prop",
          "Get the the preferences dialog if shown",
          BT_TYPE_MACHINE_PREFERENCES_DIALOG,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      MACHINE_CANVAS_ITEM_ANALYSIS_DIALOG,
      g_param_spec_object ("analysis-dialog", "analysis dialog prop",
          "Get the the analysis dialog if shown",
          BT_TYPE_SIGNAL_ANALYSIS_DIALOG,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}
