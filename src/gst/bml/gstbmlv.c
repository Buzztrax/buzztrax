/* Buzztrax
 * Copyright (C) 2004 Stefan Kost <ensonic at users.sf.net>
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
 * SECTION:gstbmlv
 * @title: GstBmlV
 * @short_description: helper object for machine voices
 *
 * Helper for polyphonic buzzmachines.
 */

#include "plugin.h"

#define GST_CAT_DEFAULT bml_debug
GST_DEBUG_CATEGORY_EXTERN (GST_CAT_DEFAULT);

static GstElementClass *parent_class = NULL;

/* set by utils.c */
extern gpointer bml (voice_class_bmh);

//-- property meta interface implementations

static gchar *
bml (v_property_meta_describe_property (gpointer bmh, guint prop_id,
        const GValue * value))
{
  const gchar *str = NULL;
  gchar *res;
  gchar def[20];
  GType base, type = G_VALUE_TYPE (value);

  // property ids have an offset of 1
  prop_id--;

  while ((base = g_type_parent (type)))
    type = base;

  switch (type) {
    case G_TYPE_INT:
      if (!(str =
              bml (describe_track_value (bmh, prop_id,
                      g_value_get_int (value)))) || !*str) {
        snprintf (def, sizeof (def), "%d", g_value_get_int (value));
        str = def;
      }
      break;
    case G_TYPE_UINT:
      if (!(str =
              bml (describe_track_value (bmh, prop_id,
                      (gint) g_value_get_uint (value)))) || !*str) {
        snprintf (def, sizeof (def), "%u", g_value_get_uint (value));
        str = def;
      }
      break;
    case G_TYPE_ENUM:
      if (!(str =
              bml (describe_track_value (bmh, prop_id,
                      g_value_get_enum (value)))) || !*str) {
        // TODO(ensonic): get blurb for enum value
        snprintf (def, sizeof (def), "%d", g_value_get_enum (value));
        str = def;
      }
      break;
    case G_TYPE_STRING:
      return g_strdup_value_contents (value);
    default:
      GST_ERROR ("unsupported GType='%s'", G_VALUE_TYPE_NAME (value));
      return g_strdup_value_contents (value);
  }
  if (str == def) {
    res = g_strdup (str);
  } else {
    res = g_convert (str, -1, "UTF-8", "WINDOWS-1252", NULL, NULL, NULL);
  }
  GST_INFO ("formatted track parameter : '%s'", res);
  return res;
}

static gchar *
gst_bmlv_property_meta_describe_property (GstBtPropertyMeta * property_meta,
    guint prop_id, const GValue * value)
{
  GstBMLV *bmlv = GST_BMLV (property_meta);
  GstBMLVClass *klass = GST_BMLV_GET_CLASS (bmlv);

  return bml (v_property_meta_describe_property (klass->bmh, prop_id, value));
}

static void
gst_bmlv_property_meta_interface_init (gpointer g_iface, gpointer iface_data)
{
  GstBtPropertyMetaInterface *iface = g_iface;

  GST_INFO ("initializing iface");

  iface->describe_property = gst_bmlv_property_meta_describe_property;
}


//-- gstbmlvoice class implementation

static void
gst_bmlv_set_property (GObject * object, guint prop_id, const GValue * value,
    GParamSpec * pspec)
{
  GstBMLV *bmlv = GST_BMLV (object);
  gpointer bm = bmlv->bm;
  gint val;
  gint type;

  // property ids have an offset of 1
  prop_id--;
  GST_DEBUG ("id: %d, bm=0x%p", prop_id, bm);

  type =
      GPOINTER_TO_INT (g_param_spec_get_qdata (pspec,
          gst_bml_property_meta_quark_type));

  if (!(pspec->flags & G_PARAM_READABLE)
      && !g_param_value_defaults (pspec, (GValue *) value)) {
    // flag triggered triggers
    g_atomic_int_set (&bmlv->triggers_changed[prop_id], 1);
  }
  val = gstbml_get_param (type, value);
  bml (set_track_parameter_value (bm, bmlv->voice, prop_id, val));
  /*{ DEBUG
     gchar *valstr=g_strdup_value_contents(value);
     GST_DEBUG("set track param %d:%d to %s", prop_id, bmlv->voice, valstr);
     g_free(valstr);
     } DEBUG */
}

static void
gst_bmlv_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstBMLV *bmlv = GST_BMLV (object);
  gpointer bm = bmlv->bm;
  gint val;
  gint type;

  // property ids have an offset of 1
  prop_id--;
  GST_DEBUG ("id: %d, bm=0x%p", prop_id, bm);

  type =
      GPOINTER_TO_INT (g_param_spec_get_qdata (pspec,
          gst_bml_property_meta_quark_type));
  val = bml (get_track_parameter_value (bm, bmlv->voice, prop_id));
  gstbml_set_param (type, val, value);
  /*{ DEBUG
     gchar *valstr=g_strdup_value_contents(value);
     GST_DEBUG("got track param %d:%d as %s", prop_id, bmlv->voice, valstr);
     g_free(valstr);
     } DEBUG */
}

static void
gst_bmlv_dispose (GObject * object)
{
  GstBMLV *bmlv = GST_BMLV (object);

  if (bmlv->dispose_has_run)
    return;
  bmlv->dispose_has_run = TRUE;

  GST_DEBUG_OBJECT (bmlv, "!!!! bmlv=%p", bmlv);

  if (G_OBJECT_CLASS (parent_class)->dispose) {
    (G_OBJECT_CLASS (parent_class)->dispose) (object);
  }
}

static void
gst_bmlv_finalize (GObject * object)
{
  GstBMLV *bmlv = GST_BMLV (object);

  GST_DEBUG_OBJECT (bmlv, "!!!! bmlv=%p", bmlv);

  g_free (bmlv->triggers_changed);

  if (G_OBJECT_CLASS (parent_class)->finalize) {
    (G_OBJECT_CLASS (parent_class)->finalize) (object);
  }
}

static void
gst_bmlv_init (GstBMLV * bmlv)
{
  GstBMLVClass *klass = GST_BMLV_GET_CLASS (bmlv);
  GST_INFO ("initializing instance");

  bmlv->triggers_changed = g_new0 (gint, klass->numtrackparams);
}

static void
gst_bmlv_class_init (GstBMLVClass * klass)
{
  gpointer bmh;
  GType enum_type = 0;
  GObjectClass *gobject_class;
  gint num;

  GST_INFO ("initializing class");
  bmh = bml (voice_class_bmh);
  g_assert (bmh);

  GST_INFO ("  bmh=0x%p", bmh);
  klass->bmh = bmh;

  parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = gst_bmlv_set_property;
  gobject_class->get_property = gst_bmlv_get_property;
  gobject_class->dispose = gst_bmlv_dispose;
  gobject_class->finalize = gst_bmlv_finalize;

  if (bml (get_machine_info (bmh, BM_PROP_NUM_TRACK_PARAMS, (void *) &num))) {
    gint min_val, max_val, def_val, no_val;
    gint type, flags;
    gchar *tmp_name, *tmp_desc;
    gchar *name = NULL, *nick = NULL, *desc = NULL;
    gint i, prop_id = 1;

    //klass->numtrackparams=num;
    GST_INFO ("  machine has %d track params ", num);
    klass->track_property = g_new (GParamSpec *, num);
    for (i = 0; i < num; i++, prop_id++) {
      GST_DEBUG ("      track_param=%02i", i);
      if (bml (get_track_parameter_info (bmh, i, BM_PARA_TYPE, (void *) &type))
          && bml (get_track_parameter_info (bmh, i, BM_PARA_NAME,
                  (void *) &tmp_name))
          && bml (get_track_parameter_info (bmh, i, BM_PARA_DESCRIPTION,
                  (void *) &tmp_desc))
          && bml (get_track_parameter_info (bmh, i, BM_PARA_FLAGS,
                  (void *) &flags))
          && bml (get_track_parameter_info (bmh, i, BM_PARA_MIN_VALUE,
                  (void *) &min_val))
          && bml (get_track_parameter_info (bmh, i, BM_PARA_MAX_VALUE,
                  (void *) &max_val))
          && bml (get_track_parameter_info (bmh, i, BM_PARA_NO_VALUE,
                  (void *) &no_val))
          && bml (get_track_parameter_info (bmh, i, BM_PARA_DEF_VALUE,
                  (void *) &def_val))
          ) {
        // FIXME(ensonic): need to ensure that we use the same names asm in the
        // parent class as we use the name for the deduplication
        gstbml_convert_names (gobject_class, tmp_name, tmp_desc, &name, &nick,
            &desc);
        // create an enum on the fly
        if (type == PT_BYTE) {
          if ((enum_type =
                  bml (gstbml_register_track_enum_type (gobject_class, bmh, i,
                          name, min_val, max_val, no_val)))) {
            type = PT_ENUM;
          }
        }

        if ((klass->track_property[klass->numtrackparams] =
                gstbml_register_param (gobject_class, prop_id, type, enum_type,
                    name, nick, desc, flags, min_val, max_val, no_val,
                    def_val))) {
          klass->numtrackparams++;
        } else {
          GST_WARNING ("registering voice_param failed!");
        }
        g_free (name);
        name = NULL;
        g_free (nick);
        nick = NULL;
        g_free (desc);
        desc = NULL;
      }
    }
  }
  GST_INFO ("  %d track params installed", klass->numtrackparams);
}


GType
bml (v_get_type (const gchar * name))
{
  GType type;
  const GTypeInfo voice_type_info = {
    sizeof (GstBMLVClass),
    NULL,
    NULL,
    (GClassInitFunc) gst_bmlv_class_init,
    NULL,
    NULL,
    sizeof (GstBMLV),
    0,
    (GInstanceInitFunc) gst_bmlv_init,
  };
  const GInterfaceInfo property_meta_interface_info = {
    (GInterfaceInitFunc) gst_bmlv_property_meta_interface_init, /* interface_init */
    NULL,                       /* interface_finalize */
    NULL                        /* interface_data */
  };

  type = g_type_register_static (GST_TYPE_OBJECT, name, &voice_type_info, 0);
  g_type_add_interface_static (type, GSTBT_TYPE_PROPERTY_META,
      &property_meta_interface_info);
  return type;
}
