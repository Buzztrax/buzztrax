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
 * SECTION:btsinkmachine
 * @short_description: class for signal processing machines with inputs only
 *
 * Sinks are machines that do playback or recording of the song. The
 * sink-machine utilizes the #BtSinkBin to transparently switch elements between
 * record (encoding) and playback.
 */

#define BT_CORE
#define BT_SINK_MACHINE_C

#include "core_private.h"
#include "sink-machine.h"

//-- property ids

enum
{
  MACHINE_PRETTY_NAME = 1
};

//-- the class

static void bt_sink_machine_persistence_interface_init (gpointer const g_iface,
    gpointer const iface_data);

G_DEFINE_TYPE_WITH_CODE (BtSinkMachine, bt_sink_machine, BT_TYPE_MACHINE,
    G_IMPLEMENT_INTERFACE (BT_TYPE_PERSISTENCE,
        bt_sink_machine_persistence_interface_init));


//-- pad templates
static GstStaticPadTemplate machine_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink_%u",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS_ANY);

//-- constructor methods

/**
 * bt_sink_machine_new:
 * @song: the song the new instance belongs to
 * @id: the id, we can use to lookup the machine
 * @err: inform about failed instance creation
 *
 * Create a new instance.
 * The machine is automatically added to the setup from the given song object.
 * You don't need to add the machine with
 * <code>#bt_setup_add_machine(setup,BT_MACHINE(machine));</code>.
 * The element used for this machine is #BtSinkBin which is configured according
 * to the use-case (playback, recording). The playback device is taken from the
 * #BtSettings.
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtSinkMachine *
bt_sink_machine_new (const BtSong * const song, const gchar * const id,
    GError ** err)
{
  return BT_SINK_MACHINE (g_object_new (BT_TYPE_SINK_MACHINE,
          "construction-error", err, "song", song, "id", id, "plugin-name",
          "bt-sink-bin", NULL));
}

//-- methods

//-- io interface

static xmlNodePtr
bt_sink_machine_persistence_save (const BtPersistence * const persistence,
    xmlNodePtr const parent_node)
{
  //BtSinkMachine *self = BT_SINK_MACHINE(persistence);
  BtPersistenceInterface *const parent_iface =
      g_type_interface_peek_parent (BT_PERSISTENCE_GET_INTERFACE (persistence));
  xmlNodePtr node = NULL;

  GST_DEBUG ("PERSISTENCE::sink-machine");

  // save parent class stuff
  if ((node = parent_iface->save (persistence, parent_node))) {
    xmlNewProp (node, XML_CHAR_PTR ("type"), XML_CHAR_PTR ("sink"));
  }
  return node;
}

static BtPersistence *
bt_sink_machine_persistence_load (const GType type,
    const BtPersistence * const persistence, xmlNodePtr node, GError ** err,
    va_list var_args)
{
  BtSinkMachine *self;
  BtPersistence *result;
  BtPersistenceInterface *parent_iface;

  GST_DEBUG ("PERSISTENCE::sink_machine");
  g_assert (node);

  xmlChar *const id = xmlGetProp (node, XML_CHAR_PTR ("id"));

  if (!persistence) {
    BtSong *song = NULL;
    gchar *param_name;
    va_list va;

    G_VA_COPY (va, var_args);
    // we need to get parameters from var_args
    // TODO(ensonic): this is duplicated code among the subclasses
    param_name = va_arg (va, gchar *);
    while (param_name) {
      if (!strcmp (param_name, "song")) {
        song = va_arg (va, gpointer);
      } else {
        GST_WARNING ("unhandled argument: %s", param_name);
        break;
      }
      param_name = va_arg (va, gchar *);
    }

    self = bt_sink_machine_new (song, (gchar *) id, err);
    result = BT_PERSISTENCE (self);
    va_end (va);
  } else {
    self = BT_SINK_MACHINE (persistence);
    result = BT_PERSISTENCE (persistence);

    g_object_set (self, "plugin-name", "bt-sink-bin", NULL);
  }
  xmlFree (id);

  // load parent class stuff
  parent_iface =
      g_type_interface_peek_parent (BT_PERSISTENCE_GET_INTERFACE (result));
  parent_iface->load (BT_TYPE_MACHINE, result, node, NULL, var_args);

  return result;
}

static void
bt_sink_machine_persistence_interface_init (gpointer const g_iface,
    gpointer const iface_data)
{
  BtPersistenceInterface *const iface = g_iface;

  iface->load = bt_sink_machine_persistence_load;
  iface->save = bt_sink_machine_persistence_save;
}

//-- wrapper

//-- bt_machine overrides

static gboolean
bt_sink_machine_check_type (const BtMachine * const self,
    const gulong pad_src_ct, const gulong pad_sink_ct)
{
  if (pad_src_ct > 0 || pad_sink_ct == 0) {
    GST_ERROR_OBJECT (self,
        "plugin has %lu src pads instead of 0 and %lu sink pads instead of >0",
        pad_src_ct, pad_sink_ct);
    return FALSE;
  }
  return TRUE;
}

static gboolean
bt_sink_machine_is_cloneable (const BtMachine * const self)
{
  return FALSE;
}

//-- g_object overrides

static void
bt_sink_machine_constructed (GObject * object)
{
  BtSinkMachine *const self = BT_SINK_MACHINE (object);
  GError **err;

  GST_INFO ("sink-machine constructed");

  G_OBJECT_CLASS (bt_sink_machine_parent_class)->constructed (object);

  g_object_get (self, "construction-error", &err, NULL);
  if (err == NULL || *err == NULL) {
    BtSong *const song;
    GstElement *const element;
    BtSetup *setup;
    BtMachine *machine = (BtMachine *) self;

    g_object_get (self, "machine", &element, "song", &song, NULL);
    if (element) {
      GstElement *const gain;

      bt_machine_activate_adder (machine);
      bt_machine_enable_input_gain (machine);

      g_object_get (self, "input-gain", &gain, NULL);
      g_object_set (element, "input-gain", gain, NULL);
      gst_object_unref (gain);
      gst_object_unref (element);
    }
    g_object_set (song, "master", self, NULL);

    GST_INFO_OBJECT (self, "construction done: %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (self));

    // add the machine to the setup of the song
    g_object_get (song, "setup", &setup, NULL);
    bt_setup_add_machine (setup, machine);
    g_object_unref (setup);

    g_object_unref (song);
    GST_INFO_OBJECT (self, "machine added: %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (self));
  }
}

static void
bt_sink_machine_get_property (GObject * const object, const guint property_id,
    GValue * const value, GParamSpec * const pspec)
{
  switch (property_id) {
    case MACHINE_PRETTY_NAME:
      g_object_get_property (object, "id", value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

//-- class internals

static void
bt_sink_machine_init (BtSinkMachine * self)
{
  GST_OBJECT_FLAG_SET (self, GST_ELEMENT_FLAG_SINK);
}

static void
bt_sink_machine_class_init (BtSinkMachineClass * klass)
{
  GObjectClass *const gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *const gstelement_klass = GST_ELEMENT_CLASS (klass);
  BtMachineClass *const machine_class = BT_MACHINE_CLASS (klass);

  gobject_class->constructed = bt_sink_machine_constructed;
  gobject_class->get_property = bt_sink_machine_get_property;

  machine_class->check_type = bt_sink_machine_check_type;
  machine_class->is_cloneable = bt_sink_machine_is_cloneable;

  gst_element_class_add_pad_template (gstelement_klass,
      gst_static_pad_template_get (&machine_sink_template));

  // we don't want to expose the factory name here
  g_object_class_override_property (gobject_class, MACHINE_PRETTY_NAME,
      "pretty-name");
}
