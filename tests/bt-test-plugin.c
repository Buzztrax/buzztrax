/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@buzztrax.org>
 *
 * gstreamer test plugin for unit tests
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
 * SECTION::bttestplugin:
 * @short_description: test gstreamer element for unit tests
 *
 * Stub elements for unit-tests. The Polyphonic element use the monophonic
 * one for its voices. Thus prefix the parameter names with 'g-' for 'global'
 * (in the monophonic class) and 'v-' for 'voice' in the poly class.
 */
/* TODO(ensonic):
 * - implement the property-meta iface in the test machines
 * - add a trigger param to the machines
 *   - the trigger param counts how often it gets triggered
 *   - the count is exposed as a read-only param
 */

#include "bt-check.h"
#include "gst/childbin.h"
#include "gst/tempo.h"

//-- enums

#define BT_TYPE_TEST_SPARSE_ENUM (bt_test_sparse_enum_get_type())
static GType
bt_test_sparse_enum_get_type (void)
{
  static GType type = 0;
  static const GEnumValue enums[] = {
    {BT_TEST_SPARSE_ENUM_ZERO, "Zero", "zero"},
    {BT_TEST_SPARSE_ENUM_ONE, "One", "one"},
    {BT_TEST_SPARSE_ENUM_TEN, "Ten", "ten"},
    {BT_TEST_SPARSE_ENUM_TWENTY, "Twenty", "twenty"},
    {BT_TEST_SPARSE_ENUM_TWENTY_ONE, "TwentyOne", "twenty-one"},
    {BT_TEST_SPARSE_ENUM_TWO_HUNDRED, "TwoHundred", "two-hundred"},
    {0, NULL, NULL},
  };

  if (G_UNLIKELY (!type)) {
    type = g_enum_register_static ("BtTestSparseEnum", enums);
  }
  return type;
}

//-- property ids

enum
{
  PROP_VOICES = 1,              // child bin iface
  PROP_UINT,                    // controlable
  PROP_DOUBLE,
  PROP_SWITCH,
  PROP_NOTE,
  PROP_SPARSE_ENUM,
  PROP_WAVE,
  PROP_STATIC,                  // static
  PROP_HOST_CALLBACKS,
  PROP_WAVE_CALLBACKS,
  PROP_COUNT
};

//-- pad templates

/*
static GstStaticPadTemplate sink_pad_template =
GST_STATIC_PAD_TEMPLATE (
  "sink",
  GST_PAD_SINK,
  GST_PAD_ALWAYS,
  GST_STATIC_CAPS ("ANY")
);
*/

static GstStaticPadTemplate src_pad_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    );

//-- child proxy interface implementation

static GObject *
bt_test_child_proxy_get_child_by_index (GstChildProxy * child_proxy,
    guint index)
{
  BtTestPolySource *self = BT_TEST_POLY_SOURCE (child_proxy);
  GST_INFO_OBJECT (self, "getting child %u of %lu", index, self->num_voices);
  g_return_val_if_fail (index < self->num_voices, NULL);

  return g_object_ref (g_list_nth_data (self->voices, index));
}

static guint
bt_test_child_proxy_get_children_count (GstChildProxy * child_proxy)
{
  return BT_TEST_POLY_SOURCE (child_proxy)->num_voices;
}


static void
bt_test_child_proxy_interface_init (gpointer g_iface, gpointer iface_data)
{
  GstChildProxyInterface *iface = g_iface;

  iface->get_child_by_index = bt_test_child_proxy_get_child_by_index;
  iface->get_children_count = bt_test_child_proxy_get_children_count;
}

//-- test_no_arg_mono_source

G_DEFINE_TYPE (BtTestNoArgMonoSource, bt_test_no_arg_mono_source,
    GST_TYPE_ELEMENT);

static void
bt_test_no_arg_mono_source_init (BtTestNoArgMonoSource * self)
{
  GstElementClass *klass = GST_ELEMENT_GET_CLASS (self);
  GstPad *src_pad =
      gst_pad_new_from_template (gst_element_class_get_pad_template (klass,
          "src"), "src");
  gst_element_add_pad (GST_ELEMENT (self), src_pad);
}

static void
bt_test_no_arg_mono_source_class_init (BtTestNoArgMonoSourceClass * klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&src_pad_template));

  gst_element_class_set_static_metadata (element_class,
      "Monophonic source for unit tests with 0 properties", "Source/Audio",
      "Use in unit tests", "Stefan Sauer <ensonic@users.sf.net>");
}

//-- test_mono_source

G_DEFINE_TYPE (BtTestMonoSource, bt_test_mono_source, GST_TYPE_ELEMENT);

static GObjectClass *mono_parent_class = NULL;

static void
bt_test_mono_source_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  BtTestMonoSource *self = BT_TEST_MONO_SOURCE (object);

  switch (property_id) {
    case PROP_UINT:
      g_value_set_uint (value, self->uint_val);
      break;
    case PROP_DOUBLE:
      g_value_set_double (value, self->double_val);
      break;
    case PROP_SWITCH:
      g_value_set_boolean (value, self->switch_val);
      break;
    case PROP_SPARSE_ENUM:
      g_value_set_enum (value, self->sparse_enum_val);
      break;
    case PROP_WAVE:
      g_value_set_enum (value, self->wave_val);
      break;
    case PROP_STATIC:
      g_value_set_int (value, self->int_val);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_test_mono_source_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  BtTestMonoSource *self = BT_TEST_MONO_SOURCE (object);

  switch (property_id) {
    case PROP_UINT:
      self->uint_val = g_value_get_uint (value);
      break;
    case PROP_DOUBLE:
      self->double_val = g_value_get_double (value);
      break;
    case PROP_SWITCH:
      self->switch_val = g_value_get_boolean (value);
      break;
    case PROP_NOTE:
      self->note_val = g_value_get_enum (value);
      break;
    case PROP_SPARSE_ENUM:
      self->sparse_enum_val = g_value_get_enum (value);
      break;
    case PROP_WAVE:
      self->wave_val = g_value_get_enum (value);
      break;
    case PROP_STATIC:
      self->int_val = g_value_get_int (value);
      break;
    case PROP_HOST_CALLBACKS:
      self->host_callbacks = g_value_get_pointer (value);
      break;
    case PROP_WAVE_CALLBACKS:
      self->wave_callbacks = g_value_get_pointer (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_test_mono_source_set_context (GstElement * element, GstContext * context)
{
  BtTestMonoSource *self = BT_TEST_MONO_SOURCE (element);

  gstbt_audio_tempo_context_get_tempo (context, &self->bpm, &self->tpb,
      &self->stpb);

#if GST_CHECK_VERSION (1,8,0)
  GST_ELEMENT_CLASS (mono_parent_class)->set_context (element, context);
#else
  if (GST_ELEMENT_CLASS (mono_parent_class)->set_context) {
    GST_ELEMENT_CLASS (mono_parent_class)->set_context (element, context);
  }
#endif
}

static void
bt_test_mono_source_init (BtTestMonoSource * self)
{
  GstElementClass *klass = GST_ELEMENT_GET_CLASS (self);
  GstPad *src_pad =
      gst_pad_new_from_template (gst_element_class_get_pad_template (klass,
          "src"), "src");
  gst_element_add_pad (GST_ELEMENT (self), src_pad);
}

static void
bt_test_mono_source_class_init (BtTestMonoSourceClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  mono_parent_class = g_type_class_peek_parent (klass);

  gobject_class->set_property = bt_test_mono_source_set_property;
  gobject_class->get_property = bt_test_mono_source_get_property;

  element_class->set_context = bt_test_mono_source_set_context;

  g_object_class_install_property (gobject_class, PROP_UINT,
      g_param_spec_uint ("g-uint",
          "uint prop",
          "uint number parameter for the test_mono_source",
          0, G_MAXUINT, 0,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DOUBLE,
      g_param_spec_double ("g-double",
          "double prop",
          "double number parameter for the test_mono_source",
          -1000.0, 1000.0, 0.0,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_SWITCH,
      g_param_spec_boolean ("g-switch",
          "switch prop",
          "switch parameter for the test_mono_source",
          FALSE,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_NOTE,
      g_param_spec_enum ("g-note", "note prop",
          "note parameter for the test_mono_source",
          GSTBT_TYPE_NOTE, GSTBT_NOTE_NONE,
          G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_SPARSE_ENUM,
      g_param_spec_enum ("g-sparse-enum", "sparse enum prop",
          "sparse enum parameter for the test_mono_source",
          BT_TYPE_TEST_SPARSE_ENUM, BT_TEST_SPARSE_ENUM_ZERO,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_WAVE,
      g_param_spec_enum ("g-wave", "wave",
          "wave parameter for the test_mono_source", GSTBT_TYPE_WAVE_INDEX, 0,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_STATIC,
      g_param_spec_int ("g-static",
          "int prop",
          "int number parameter for the test_mono_source",
          0, G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_WAVE_CALLBACKS,
      g_param_spec_pointer ("wave-callbacks",
          "Wavetable Callbacks", "The wave-table access callbacks",
          G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_HOST_CALLBACKS,
      g_param_spec_pointer ("host-callbacks",
          "host-callbacks property",
          "Buzz host callback structure",
          G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&src_pad_template));

  gst_element_class_set_static_metadata (element_class,
      "Monophonic source for unit tests", "Source/Audio",
      "Use in unit tests", "Stefan Kost <ensonic@users.sf.net>");
}

//-- test_poly_source

G_DEFINE_TYPE_WITH_CODE (BtTestPolySource, bt_test_poly_source,
    GST_TYPE_ELEMENT,
    G_IMPLEMENT_INTERFACE (GST_TYPE_CHILD_PROXY,
        bt_test_child_proxy_interface_init)
    G_IMPLEMENT_INTERFACE (GSTBT_TYPE_CHILD_BIN, NULL));

static GObjectClass *poly_parent_class = NULL;

static void
bt_test_poly_source_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  BtTestPolySource *self = BT_TEST_POLY_SOURCE (object);

  switch (property_id) {
    case PROP_VOICES:
      g_value_set_ulong (value, self->num_voices);
      break;
    case PROP_UINT:
      g_value_set_uint (value, self->uint_val);
      break;
    case PROP_DOUBLE:
      g_value_set_double (value, self->double_val);
      break;
    case PROP_SWITCH:
      g_value_set_boolean (value, self->switch_val);
      break;
    case PROP_SPARSE_ENUM:
      g_value_set_enum (value, self->sparse_enum_val);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_test_poly_source_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  BtTestPolySource *self = BT_TEST_POLY_SOURCE (object);
  BtTestMonoSource *voice;
  gulong i, num_voices;

  switch (property_id) {
    case PROP_VOICES:
      num_voices = self->num_voices;
      self->num_voices = g_value_get_ulong (value);
      GST_INFO ("machine %p, changing voices from %lu to %lu", object,
          num_voices, self->num_voices);
      if (self->num_voices > num_voices) {
        for (i = num_voices; i < self->num_voices; i++) {
          voice = g_object_new (BT_TYPE_TEST_MONO_SOURCE, NULL);
          self->voices = g_list_append (self->voices, voice);
          GST_DEBUG (" adding voice %" G_OBJECT_REF_COUNT_FMT,
              G_OBJECT_LOG_REF_COUNT (voice));
          gst_object_set_parent (GST_OBJECT (voice), GST_OBJECT (self));
          GST_DEBUG (" parented voice %" G_OBJECT_REF_COUNT_FMT,
              G_OBJECT_LOG_REF_COUNT (voice));
        }
      } else if (self->num_voices < num_voices) {
        for (i = num_voices; i > self->num_voices; i--) {
          voice = g_list_nth_data (self->voices, (i - 1));
          self->voices = g_list_remove (self->voices, voice);
          gst_object_unparent (GST_OBJECT (voice));
        }
      }
      break;
    case PROP_UINT:
      self->uint_val = g_value_get_uint (value);
      break;
    case PROP_DOUBLE:
      self->double_val = g_value_get_double (value);
      break;
    case PROP_SWITCH:
      self->switch_val = g_value_get_boolean (value);
      break;
    case PROP_NOTE:
      self->note_val = g_value_get_enum (value);
      break;
    case PROP_SPARSE_ENUM:
      self->sparse_enum_val = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_test_poly_source_finalize (GObject * object)
{
  BtTestPolySource *self = BT_TEST_POLY_SOURCE (object);

  GST_DEBUG (" !!! self=%p", object);

  if (self->voices) {
    GList *node;
    for (node = self->voices; node; node = g_list_next (node)) {
      GST_DEBUG (" free voice %" G_OBJECT_REF_COUNT_FMT,
          G_OBJECT_LOG_REF_COUNT (node->data));
      gst_object_unparent (node->data);
      node->data = NULL;
    }
    g_list_free (self->voices);
    self->voices = NULL;
  }
  G_OBJECT_CLASS (poly_parent_class)->finalize (object);
}

static void
bt_test_poly_source_set_context (GstElement * element, GstContext * context)
{
  BtTestPolySource *self = BT_TEST_POLY_SOURCE (element);

  gstbt_audio_tempo_context_get_tempo (context, &self->bpm, &self->tpb,
      &self->stpb);

#if GST_CHECK_VERSION (1,8,0)
  GST_ELEMENT_CLASS (poly_parent_class)->set_context (element, context);
#else
  if (GST_ELEMENT_CLASS (poly_parent_class)->set_context) {
    GST_ELEMENT_CLASS (poly_parent_class)->set_context (element, context);
  }
#endif
}


static void
bt_test_poly_source_init (BtTestPolySource * self)
{
  GstElementClass *klass = GST_ELEMENT_GET_CLASS (self);
  GstPad *src_pad =
      gst_pad_new_from_template (gst_element_class_get_pad_template (klass,
          "src"), "src");
  gst_element_add_pad (GST_ELEMENT (self), src_pad);
}

static void
bt_test_poly_source_class_init (BtTestPolySourceClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  poly_parent_class = g_type_class_peek_parent (klass);

  gobject_class->set_property = bt_test_poly_source_set_property;
  gobject_class->get_property = bt_test_poly_source_get_property;
  //gobject_class->dispose      = bt_test_poly_source_dispose;
  gobject_class->finalize = bt_test_poly_source_finalize;

  element_class->set_context = bt_test_poly_source_set_context;

  g_object_class_override_property (gobject_class, PROP_VOICES, "children");

  g_object_class_install_property (gobject_class, PROP_UINT,
      g_param_spec_uint ("v-uint",
          "uint prop",
          "uint number parameter for the test_poly_source",
          0, G_MAXUINT, 0, G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));

  g_object_class_install_property (gobject_class, PROP_DOUBLE,
      g_param_spec_double ("v-double",
          "double prop",
          "double number parameter for the test_poly_source",
          -1000.0, 1000.0, 0, G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));

  g_object_class_install_property (gobject_class, PROP_SWITCH,
      g_param_spec_boolean ("v-switch",
          "switch prop",
          "switch parameter for the test_poly_source",
          FALSE, G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));

  g_object_class_install_property (gobject_class, PROP_NOTE,
      g_param_spec_enum ("v-note", "note prop",
          "note parameter for the test_poly_source",
          GSTBT_TYPE_NOTE, GSTBT_NOTE_NONE,
          G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_SPARSE_ENUM,
      g_param_spec_enum ("v-sparse-enum", "sparse enum prop",
          "sparse enum parameter for the test_poly_source",
          BT_TYPE_TEST_SPARSE_ENUM, BT_TEST_SPARSE_ENUM_ZERO,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&src_pad_template));

  gst_element_class_set_static_metadata (element_class,
      "Polyphonic source for unit tests", "Source/Audio",
      "Use in unit tests", "Stefan Kost <ensonic@users.sf.net>");
}

//-- test_mono_processor

//-- test_poly_processor

//-- plugin handling

gboolean
bt_test_plugin_init (GstPlugin * plugin)
{
  //GST_INFO("registering unit test plugin");

  gst_element_register (plugin, "buzztrax-test-no-arg-mono-source",
      GST_RANK_NONE, BT_TYPE_TEST_NO_ARG_MONO_SOURCE);
  gst_element_register (plugin, "buzztrax-test-mono-source", GST_RANK_NONE,
      BT_TYPE_TEST_MONO_SOURCE);
  gst_element_register (plugin, "buzztrax-test-poly-source", GST_RANK_NONE,
      BT_TYPE_TEST_POLY_SOURCE);
  /*
     gst_element_register (plugin, "buzztrax-test-mono-processor", GST_RANK_NONE,
     BT_TYPE_TEST_MONO_PROCESSOR);
     gst_element_register (plugin, "buzztrax-test-poly-processor", GST_RANK_NONE,
     BT_TYPE_TEST_POLY_PROCESSOR);
   */
  return TRUE;
}
