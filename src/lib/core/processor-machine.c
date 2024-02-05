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
 * SECTION:btprocessormachine
 * @short_description: class for signal processing machines with inputs and
 * outputs
 *
 * Processors are machines that alter incomming audio.
 */

#define BT_CORE
#define BT_PROCESSOR_MACHINE_C

#include "core_private.h"
#include "machine.h"
#include <gst/base/gstbasetransform.h>

//-- the class

static void bt_processor_machine_persistence_interface_init (gpointer const
    g_iface, gpointer const iface_data);

G_DEFINE_TYPE_WITH_CODE (BtProcessorMachine, bt_processor_machine,
    BT_TYPE_MACHINE, G_IMPLEMENT_INTERFACE (BT_TYPE_PERSISTENCE,
        bt_processor_machine_persistence_interface_init));


//-- pad templates
static GstStaticPadTemplate machine_src_template =
GST_STATIC_PAD_TEMPLATE ("src_%u",
    GST_PAD_SRC,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate machine_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink_%u",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS_ANY);

//-- constructor methods

/**
 * bt_processor_machine_new:
 * @plugin_name: the name of the gst-plugin the machine is using
 * @voices: the number of voices the machine should initially have
 * @err: inform about failed instance creation
 *
 * Create a new instance.
 * The machine is automaticly added to the setup of the given song. You don't
 * need to call <code>#bt_setup_add_machine(setup,BT_MACHINE(machine));</code>.
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtProcessorMachine *
bt_processor_machine_new (BtMachineConstructorParams* const params, 
    const gchar * const plugin_name, const glong voices, GError ** err)
{
  return BT_PROCESSOR_MACHINE (g_object_new (BT_TYPE_PROCESSOR_MACHINE,
          "construction-error", err, "song", params->song, "id", params->id,
          "plugin-name", plugin_name, "voices", voices, NULL));
}

//-- methods

//-- io interface

static xmlNodePtr
bt_processor_machine_persistence_save (const BtPersistence * const persistence,
    xmlNodePtr const parent_node, gpointer const userdata)
{
  const BtProcessorMachine *const self = BT_PROCESSOR_MACHINE (persistence);
  const BtPersistenceInterface *const parent_iface =
      g_type_interface_peek_parent (BT_PERSISTENCE_GET_INTERFACE (persistence));
  xmlNodePtr node = NULL;
  gchar *const plugin_name;
  gulong voices;

  GST_DEBUG ("PERSISTENCE::processor-machine");

  // save parent class stuff
  if ((node = parent_iface->save (persistence, parent_node, userdata))) {
    xmlNewProp (node, XML_CHAR_PTR ("type"), XML_CHAR_PTR ("processor"));

    g_object_get ((gpointer) self, "plugin-name", &plugin_name, "voices",
        &voices, NULL);
    xmlNewProp (node, XML_CHAR_PTR ("plugin-name"), XML_CHAR_PTR (plugin_name));
    xmlNewProp (node, XML_CHAR_PTR ("voices"),
        XML_CHAR_PTR (bt_str_format_ulong (voices)));
    g_free (plugin_name);
  }
  return node;
}

static BtPersistence *
bt_processor_machine_persistence_load (const GType type,
    BtPersistence * const persistence, xmlNodePtr node, GError ** err,
    va_list var_args)
{
  BtProcessorMachine *self;
  BtPersistence *result;
  BtPersistenceInterface *parent_iface;
  gulong voices;

  GST_DEBUG ("PERSISTENCE::processor_machine");
  g_assert (node);

  xmlChar *const id = xmlGetProp (node, XML_CHAR_PTR ("id"));
  xmlChar *const plugin_name = xmlGetProp (node, XML_CHAR_PTR ("plugin-name"));
  xmlChar *const voices_str = xmlGetProp (node, XML_CHAR_PTR ("voices"));
  voices = voices_str ? atol ((char *) voices_str) : 0;

  if (!persistence) {
    BtMachineConstructorParams cparams;
    bt_machine_varargs_to_constructor_params (var_args, (gchar *) id, &cparams);
    
    self =
        bt_processor_machine_new (&cparams, (gchar *) plugin_name,
        voices, err);
    result = BT_PERSISTENCE (self);
  } else {
    self = BT_PROCESSOR_MACHINE (persistence);
    result = BT_PERSISTENCE (persistence);

    // TODO(ensonic): isn't plugin-name construct-only
    g_object_set (self, "plugin-name", plugin_name, "voices", voices, NULL);
  }
  xmlFree (id);
  xmlFree (plugin_name);
  xmlFree (voices_str);

  // load parent class stuff
  parent_iface =
      g_type_interface_peek_parent (BT_PERSISTENCE_GET_INTERFACE (result));
  parent_iface->load (BT_TYPE_MACHINE, result, node, NULL, var_args);

  return result;
}

static void
bt_processor_machine_persistence_interface_init (gpointer const g_iface,
    gpointer const iface_data)
{
  BtPersistenceInterface *const iface = g_iface;

  iface->load = bt_processor_machine_persistence_load;
  iface->save = bt_processor_machine_persistence_save;
}

//-- wrapper

//-- bt_machine overrides

static gboolean
bt_processor_machine_check_type (const BtMachine * const self,
    const gulong pad_src_ct, const gulong pad_sink_ct)
{
  if (pad_src_ct == 0 || pad_sink_ct == 0) {
    GST_ERROR_OBJECT (self,
        "plugin has %lu src pads instead of >0 and %lu sink pads instead of >0",
        pad_src_ct, pad_sink_ct);
    return FALSE;
  }
  return TRUE;
}

//-- g_object overrides

static void
bt_processor_machine_constructed (GObject * object)
{
  BtProcessorMachine *const self = BT_PROCESSOR_MACHINE (object);
  GError **err;

  GST_INFO ("processor-machine constructed");

  G_OBJECT_CLASS (bt_processor_machine_parent_class)->constructed (object);

  g_object_get (self, "construction-error", &err, NULL);
  if (err == NULL || *err == NULL) {
    GstElement *const element;
    BtSong *const song;
    BtSetup *setup;
    BtMachine *machine = (BtMachine *) self;

    g_object_get (self, "machine", &element, "song", &song, NULL);
    if (element) {
      if (GST_IS_BASE_TRANSFORM (element)) {
        gst_base_transform_set_passthrough ((GstBaseTransform *) element,
            FALSE);
      }
      gst_object_unref (element);

      bt_machine_activate_adder (machine);
      bt_machine_activate_spreader (machine);
      bt_machine_enable_output_gain (machine);
    }
    g_object_unref (bt_cmd_pattern_new (song, machine, BT_PATTERN_CMD_BYPASS));

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

//-- class internals

static void
bt_processor_machine_init (BtProcessorMachine * self)
{
  //self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_PROCESSOR_MACHINE, BtProcessorMachinePrivate);
}

static void
bt_processor_machine_class_init (BtProcessorMachineClass * const klass)
{
  GObjectClass *const gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *const gstelement_klass = GST_ELEMENT_CLASS (klass);
  BtMachineClass *const machine_class = BT_MACHINE_CLASS (klass);

  gobject_class->constructed = bt_processor_machine_constructed;

  machine_class->check_type = bt_processor_machine_check_type;

  gst_element_class_add_pad_template (gstelement_klass,
      gst_static_pad_template_get (&machine_src_template));
  gst_element_class_add_pad_template (gstelement_klass,
      gst_static_pad_template_get (&machine_sink_template));
}
