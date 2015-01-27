/* GStreamer
 * Copyright (C) 2005 Stefan Kost <ensonic at user.sf.net>
 *
 * gstbml.h: Header for BML plugin
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
/* < private_header > */

#ifndef __GST_BML_H__
#define __GST_BML_H__

#include <glib.h>
#include <gst/gst.h>

G_BEGIN_DECLS

typedef float BMLData;

typedef enum {
  ARG_BPM=1,  // tempo iface
  ARG_TPB,
  ARG_STPT,
  ARG_HOST_CALLBACKS, // buzztrax host callbacks
  //ARG_VOICES,  // child bin iface
  ARG_LAST
} GstBMLPropertyIDs;

typedef enum {
  PT_NOTE=0,
  PT_SWITCH,
  PT_BYTE,
  PT_WORD,
  PT_ENUM,
  PT_ATTR
} GstBMLParameterTypes;

typedef enum {
  MT_MASTER=0,
  MT_GENERATOR,
  MT_EFFECT
} GstBMLMachineTypes;

extern GQuark gst_bml_property_meta_quark_type;

#define GST_BML(obj)         ((GstBML *)(&obj->bml))
#define GST_BML_CLASS(klass) ((GstBMLClass *)(&klass->bml_class))

typedef struct _GstBML GstBML;
typedef struct _GstBMLClass GstBMLClass;

struct _GstBML {
  gboolean dispose_has_run;

  GstElement *self;

  // the buzz machine handle (to use with libbml API)
  gpointer bm;

  // the number of voices the plugin has
  gulong num_voices;
  GList *voices;

  // tempo handling
  gulong beats_per_minute;
  gulong ticks_per_beat;
  gulong subticks_per_tick;
  gulong subtick_count;
  gdouble ticktime_err,ticktime_err_accum;

  // pads
  GstPad **sinkpads,**srcpads;

  gint samplerate, samples_per_buffer;
  
  // array with an entry for each parameter
  // flags that a g_object_set has set a value for a trigger param
  gint * volatile triggers_changed;

  /* < private > */
  gboolean tags_pushed;			/* send tags just once ? */
  GstClockTime ticktime;
  GstClockTime running_time;            /* total running time */
  gint64 n_samples;                     /* total samples sent */
  gint64 n_samples_stop;
  gboolean check_eos;
  gboolean eos_reached;
  gboolean reverse;                  /* play backwards */
};

struct _GstBMLClass {
  // the buzz machine handle (to use with libbml API)
  gpointer bmh;

  gchar *dll_name;
  gchar *help_uri;
  gchar *preset_path;

  // the type for a voice parameter faccade
  GType voice_type;

  // preset names and data
  GList *presets;
  GHashTable *preset_data;
  GHashTable *preset_comments;

  // basic machine data
  gint numsinkpads,numsrcpads;
  gint numattributes,numglobalparams,numtrackparams;

  gint input_channels,output_channels;
  
  // param specs
  GParamSpec **global_property,**track_property;
};

//-- helper

extern gboolean bml(gstbml_inspect(gchar *file_name));
extern gboolean bml(gstbml_register_element(GstPlugin *plugin, GstStructure *bml_meta));

extern gboolean bml(gstbml_is_polyphonic(gpointer bmh));

//-- common iface functions

extern gchar *bml(gstbml_property_meta_describe_property(GstBMLClass *bml_class, GstBML *bml, guint prop_id, const GValue *event));
extern void bml(gstbml_tempo_change_tempo(GObject *gstbml, GstBML *bml, glong beats_per_minute, glong ticks_per_beat, glong subticks_per_tick));

//-- common class functions

extern gpointer bml(gstbml_class_base_init(GstBMLClass *klass, GType type, gint numsrcpads, gint numsinkpads));
extern void bml(gstbml_base_finalize(GstBMLClass *klass));
extern void bml(gstbml_class_set_details(GstElementClass *element_class, GstBMLClass *bml_class, gpointer bmh, const gchar *category));
extern void bml(gstbml_class_prepare_properties(GObjectClass *klass, GstBMLClass *bml_class));
extern GType bml(gstbml_register_track_enum_type(GObjectClass *klass, gpointer bm, gint i, gchar *name, gint min_val, gint max_val, gint no_val));

//-- common element functions

extern void bml(gstbml_init(GstBML *bml,GstBMLClass *klass,GstElement *element));
extern void bml(gstbml_init_pads(GstElement *element, GstBML *bml, GstPadLinkFunction *gst_bml_link));

extern void bml(gstbml_finalize(GstBML *bml));

extern void bml(gstbml_set_property(GstBML *bml, GstBMLClass *bml_class, guint prop_id, const GValue *value, GParamSpec * pspec));
extern void bml(gstbml_get_property(GstBML *bml, GstBMLClass *bml_class, guint prop_id, GValue *value, GParamSpec * pspec));

extern void bml(gstbml_sync_values(GstBML *bml, GstBMLClass *bml_class, GstClockTime ts));
extern void bml(gstbml_reset_triggers(GstBML *bml, GstBMLClass *bml_class));

G_END_DECLS

#endif /* __GST_BML_H__ */
