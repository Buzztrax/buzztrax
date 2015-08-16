/* GStreamer
 * Copyright (C) 2012 Stefan Sauer <ensonic@users.sf.net>
 *
 * sidsynv.cc: c64 sid synthesizer voice
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
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
 * SECTION:sidsynv
 * @title: GstBtSidSynV
 * @short_description: c64 sid synthesizer voice
 *
 * A single voice for #GstBtSidSyn.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gst/propertymeta.h"
#include "gst/tempo.h"

#include "sidsynv.h"

#define GST_CAT_DEFAULT sid_syn_debug
GST_DEBUG_CATEGORY_EXTERN (GST_CAT_DEFAULT);

enum
{
  PROP_NOTE = 1,
  PROP_SYNC,
  PROP_RING_MOD,
  PROP_TEST,
  PROP_WAVE,
  PROP_PULSE_WIDTH,
  PROP_FILTER_VOICE,
  PROP_ATTACK,
  PROP_DECAY,
  PROP_SUSTAIN,
  PROP_RELEASE,
  PROP_EFFECT_TYPE,
  PROP_EFFECT_VALUE
};
/* TODO(ensonic): add effect type and effect param props
type  param desc
00    xy    arpeggio, +x halftones, +y halftones
01    xx    slide pitch up, xx speed
02    xx    slide pitch down, xx speed
03    xx    tone portamento (portamento to note) (00 keep going), xx speed
04    xy    vibrato, x speed, y depth (if 0 keep previous)
E3    0x    glissando control, pitch effects would be quantised to semitones
E4    0x    vibrato type (continous = no phase reset)
            0 = Sine
            1 = Ramp down
            2 = Square
            4 = Continuous sine
            5 = Continuous ramp down
            6 = Continuous square
E5    xx    set finetune (00=-1/2 semitone, 80=0, FF=+1/2 semitone

- all the effects here would modulate the frequency
- on each of the subticks process the fx and update freq
*/

#define GSTBT_TYPE_SID_SYN_WAVE (gst_sid_syn_wave_get_type())
static GType
gst_sid_syn_wave_get_type (void)
{
  static GType type = 0;
  static const GEnumValue enums[] = {
    {GSTBT_SID_SYN_WAVE_TRIANGLE, "Triangle", "triangle"},
    {GSTBT_SID_SYN_WAVE_SAW, "Saw", "saw"},
    {GSTBT_SID_SYN_WAVE_SAW_TRIANGLE, "Saw+Triangle", "saw-triangle"},
    {GSTBT_SID_SYN_WAVE_PULSE, "Pulse", "pulse"},
    {GSTBT_SID_SYN_WAVE_PULSE_TRIANGLE, "Pulse+Triangle", "pulse-triangle"},
    {GSTBT_SID_SYN_WAVE_PULSE_SAW, "Pulse+Saw", "pulse-saw"},
    {GSTBT_SID_SYN_WAVE_PULSE_SAW_TRIANGLE, "Pulse+Saw+Triangle",
          "pulse-saw-triangle"},
    {GSTBT_SID_SYN_WAVE_NOISE, "Noise", "noise"},
    {0, NULL, NULL},
  };

  if (G_UNLIKELY (!type)) {
    type = g_enum_register_static ("GstBtSidSynWave", enums);
  }
  return type;
}

#define GSTBT_TYPE_SID_SYN_EFFECT (gst_sid_syn_effect_get_type())
static GType
gst_sid_syn_effect_get_type (void)
{
  static GType type = 0;
  static const GEnumValue enums[] = {
    {GSTBT_SID_SYN_EFFECT_ARPEGGIO, "Arpeggio", "arpeggio"},
    {GSTBT_SID_SYN_EFFECT_PORTAMENTO_UP, "Portamento up", "portamento-up"},
    {GSTBT_SID_SYN_EFFECT_PORTAMENTO_DOWN, "Portamento down", "portamento-down"},
    {GSTBT_SID_SYN_EFFECT_PORTAMENTO, "Portamento", "portamento"},
    {GSTBT_SID_SYN_EFFECT_VIBRATO, "Vibrato", "vibrato"},
    {GSTBT_SID_SYN_EFFECT_GLISSANDO_CONTROL, "Glissando control", "glissando-control"},
    {GSTBT_SID_SYN_EFFECT_VIBRATO_TYPE, "Vibrato type", "vibrato-type"},
    {GSTBT_SID_SYN_EFFECT_FINETUNE, "Finetune", "finetune"},
    {GSTBT_SID_SYN_EFFECT_NONE, "None", "none"},
    {0, NULL, NULL},
  };

  if (G_UNLIKELY (!type)) {
    type = g_enum_register_static ("GstBtSidSynEffect", enums);
  }
  return type;
}

static void gst_sid_synv_property_meta_interface_init (gpointer const g_iface,
    gpointer const iface_data);

G_DEFINE_TYPE_WITH_CODE (GstBtSidSynV, gstbt_sid_synv, GST_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (GSTBT_TYPE_PROPERTY_META,
        gst_sid_synv_property_meta_interface_init));

//-- property meta interface implementations

static gchar *
g_enum_get_value_name (GType type, const GValue * value)
{
  gchar *res = NULL;
  GEnumClass * enum_class = (GEnumClass *) g_type_class_ref (type);
  GEnumValue * enum_value = g_enum_get_value (enum_class, 
      g_value_get_enum (value));
  
  if (enum_value) {
    res = g_strdup (enum_value->value_name);
  } else {
    res = g_strdup ("None");
  }
  g_type_class_unref (enum_class);
  return res;
}

static gchar *
gst_sid_synv_property_meta_describe_property (GstBtPropertyMeta * property_meta,
    guint prop_id, const GValue * value)
{
  //GstBtSidSynV *src = GSTBT_SID_SYNV (property_meta);
  gchar *res = NULL;
  static const gchar *attack[] = {
    "2 ms", "8 ms", "16 ms", "24 ms", "38 ms", "56 ms", "68 ms", "80 ms",
    "100 ms", "240 ms", "500 ms", "800 ms", "1 s", "3 s", "5 s", "8 s"
  };
  static const gchar *decay_release[] = {
    "6 ms", "24 ms", "48 ms", "72 ms", "114 ms", "168 ms", "204 ms", "240 ms",
    "300 ms", "750 ms", "1.5 s", "2.4 s", "3 s", "9 s", "15 s", "24 s",
  };

  switch (prop_id) {
    case PROP_WAVE:
      res = g_enum_get_value_name (GSTBT_TYPE_SID_SYN_WAVE, value);
      break;
    case PROP_PULSE_WIDTH:
      res = g_strdup_printf ("%5.1lf %%",
          (gfloat) g_value_get_uint (value) / 40.95);
      break;
    case PROP_ATTACK:
      res = g_strdup (attack[g_value_get_uint (value)]);
      break;
    case PROP_DECAY:
      res = g_strdup (decay_release[g_value_get_uint (value)]);
      break;
    case PROP_RELEASE:
      res = g_strdup (decay_release[g_value_get_uint (value)]);
      break;
    case PROP_EFFECT_TYPE:
      res = g_enum_get_value_name (GSTBT_TYPE_SID_SYN_EFFECT, value);
      break;
    case PROP_EFFECT_VALUE:
#if 0      
      // FIXME(ensonic): just nagivating the pattern, won't update the type
      // we really need to know the type from the pattern row
      switch (src->effect_type) {
        case GSTBT_SID_SYN_EFFECT_ARPEGGIO:
          res = 
              g_strdup ("Semitones up for two notes (first and second column)");
          break;
        case GSTBT_SID_SYN_EFFECT_PORTAMENTO_UP:
        case GSTBT_SID_SYN_EFFECT_PORTAMENTO_DOWN:
          res = g_strdup ("Speed for pitch slide");
          break;
        case GSTBT_SID_SYN_EFFECT_PORTAMENTO:
          res = g_strdup ("Speed for pitch slide to new note, 00 keep going");
          break;
        case GSTBT_SID_SYN_EFFECT_VIBRATO:
          res = 
              g_strdup ("Speed + Depth for vibrato (first and second column)");
          break;
        case GSTBT_SID_SYN_EFFECT_GLISSANDO_CONTROL:
          res = g_strdup (src->quantize_freq ? "quantized" : "smooth");
          break;
        case GSTBT_SID_SYN_EFFECT_VIBRATO_TYPE: {
          static const gchar *waves[] = { "Sine", "Ramp down", "Square" };
          res = 
              g_strdup_printf ("Vibrato waveform: %s", waves[src->vib_type]);
        } break;
        case GSTBT_SID_SYN_EFFECT_FINETUNE:
          g_strdup ("Pitch finetuning in cents for voice.");
          break;
        default:
          break;
      }
#endif
      break;
    default:
      break;
  }
  return res;
}

static void
gst_sid_synv_property_meta_interface_init (gpointer const g_iface,
    gpointer const iface_data)
{
  GstBtPropertyMetaInterface *const iface =
      (GstBtPropertyMetaInterface * const) g_iface;

  GST_INFO ("initializing iface");

  iface->describe_property = gst_sid_synv_property_meta_describe_property;
}

//-- sid syn voice class implementation

static void
gst_sid_synv_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstBtSidSynV *src = GSTBT_SID_SYNV (object);

  switch (prop_id) {
    case PROP_NOTE: {
      guint note = g_value_get_enum (value);
      GST_INFO_OBJECT (src, "note: %d", note);
      if (note) {
        src->note = (GstBtNote) note;
        src->note_set = TRUE;
      }
      if (note != GSTBT_NOTE_OFF) {
        src->prev_note = src->note;
      }
    } break;
    case PROP_SYNC:
      src->sync = g_value_get_boolean (value);
      break;
    case PROP_RING_MOD:
      src->ringmod = g_value_get_boolean (value);
      break;
    case PROP_TEST:
      src->test = g_value_get_boolean (value);
      break;
    case PROP_WAVE:
      src->wave = (GstBtSidSynWave) g_value_get_enum (value);
      break;
    case PROP_PULSE_WIDTH:
      src->pulse_width = g_value_get_uint (value);
      break;
    case PROP_FILTER_VOICE:
      src->filter = g_value_get_boolean (value);
      break;
    case PROP_ATTACK:
      src->attack = g_value_get_uint (value);
      break;
    case PROP_DECAY:
      src->decay = g_value_get_uint (value);
      break;
    case PROP_SUSTAIN:
      src->sustain = g_value_get_uint (value);
      break;
    case PROP_RELEASE:
      src->release = g_value_get_uint (value);
      break;
    case PROP_EFFECT_TYPE: {
      guint effect_type = g_value_get_enum (value);
      if (effect_type != GSTBT_SID_SYN_EFFECT_NONE) {
        src->effect_type = (GstBtSidSynEffect) effect_type;
        src->effect_set = TRUE;
        GST_INFO_OBJECT (src, "set fx: %d",src->effect_type);
      }
    } break;
    case PROP_EFFECT_VALUE:
      src->effect_value = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_sid_synv_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstBtSidSynV *src = GSTBT_SID_SYNV (object);

  switch (prop_id) {
    case PROP_SYNC:
      g_value_set_boolean (value, src->sync);
      break;
    case PROP_RING_MOD:
      g_value_set_boolean (value, src->ringmod);
      break;
    case PROP_TEST:
      g_value_set_boolean (value, src->test);
      break;
    case PROP_WAVE:
      g_value_set_enum (value, src->wave);
      break;
    case PROP_PULSE_WIDTH:
      g_value_set_uint (value, src->pulse_width);
      break;
    case PROP_FILTER_VOICE:
      g_value_set_boolean (value, src->filter);
      break;
    case PROP_ATTACK:
      g_value_set_uint (value, src->attack);
      break;
    case PROP_DECAY:
      g_value_set_uint (value, src->decay);
      break;
    case PROP_SUSTAIN:
      g_value_set_uint (value, src->sustain);
      break;
    case PROP_RELEASE:
      g_value_set_uint (value, src->release);
      break;
    case PROP_EFFECT_TYPE:
      g_value_set_enum (value, src->effect_type);
      break;
    case PROP_EFFECT_VALUE:
      g_value_set_uint (value, src->effect_value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_sid_synv_init (GstBtSidSynV * self)
{
  self->wave = GSTBT_SID_SYN_WAVE_TRIANGLE;
  self->pulse_width = 2048;
  self->attack = 2;
  self->decay = 2;
  self->sustain = 10;
  self->release = 5;
  self->finetune = 1.0;
}

static void
gstbt_sid_synv_class_init (GstBtSidSynVClass * klass)
{
  GObjectClass *const gobject_class = G_OBJECT_CLASS (klass);
  const GParamFlags pflags1 = (GParamFlags)
      (G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS);
  const GParamFlags pflags2 = (GParamFlags)
      (G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS);

  gobject_class->set_property = gst_sid_synv_set_property;
  gobject_class->get_property = gst_sid_synv_get_property;

  g_object_class_install_property (gobject_class, PROP_NOTE,
      g_param_spec_enum ("note", "Musical note",
          "Musical note (e.g. 'c-3', 'd#4')", GSTBT_TYPE_NOTE, GSTBT_NOTE_NONE,
          pflags1));

  g_object_class_install_property (gobject_class, PROP_SYNC,
      g_param_spec_boolean ("sync", "Sync", "Sync with voice 3", FALSE,
          pflags2));

  g_object_class_install_property (gobject_class, PROP_RING_MOD,
      g_param_spec_boolean ("ringmod", "Ringmod", "Ringmod with voice 3", FALSE,
          pflags2));

  g_object_class_install_property (gobject_class, PROP_TEST,
      g_param_spec_boolean ("test", "Test", "Control test bit", FALSE,
          pflags2));

  g_object_class_install_property (gobject_class, PROP_WAVE,
      g_param_spec_enum ("wave", "Waveform", "Oscillator waveform",
          GSTBT_TYPE_SID_SYN_WAVE, GSTBT_SID_SYN_WAVE_TRIANGLE, pflags2));

  g_object_class_install_property (gobject_class, PROP_PULSE_WIDTH,
      g_param_spec_uint ("pulse-width", "Pulse Width", "Pulse Width", 0, 4095,
          2048, pflags2));

  g_object_class_install_property (gobject_class, PROP_FILTER_VOICE,
      g_param_spec_boolean ("fiter-voice", "Filter Voice", "Filter Voice",
          FALSE, pflags2));

  g_object_class_install_property (gobject_class, PROP_ATTACK,
      g_param_spec_uint ("attack", "Attack", "Attack", 0, 15, 2, pflags2));

  g_object_class_install_property (gobject_class, PROP_DECAY,
      g_param_spec_uint ("decay", "Decay", "Decay", 0, 15, 2, pflags2));

  g_object_class_install_property (gobject_class, PROP_SUSTAIN,
      g_param_spec_uint ("sustain", "Sustain", "Sustain", 0, 15, 10, pflags2));

  g_object_class_install_property (gobject_class, PROP_RELEASE,
      g_param_spec_uint ("release", "Release", "Release", 0, 15, 5, pflags2));

  g_object_class_install_property (gobject_class, PROP_EFFECT_TYPE,
      g_param_spec_enum ("effect-type", "Effect type", "Effect Type",
          GSTBT_TYPE_SID_SYN_EFFECT, GSTBT_SID_SYN_EFFECT_NONE, pflags1));

  g_object_class_install_property (gobject_class, PROP_EFFECT_VALUE,   
      g_param_spec_uint ("effect-value", "Effect value", "Effect parameter(s)",
          0, 255, 0, pflags1));
}
