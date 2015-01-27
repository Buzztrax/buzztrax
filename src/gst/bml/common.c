/* GStreamer
 * Copyright (C) 2005 Stefan Kost <ensonic at users.sf.net>
 *
 * common.c: Functions shared among all native and wrapped elements
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

#include "plugin.h"
#include "gstbmlorc.h"

#define GST_CAT_DEFAULT bml_debug
GST_DEBUG_CATEGORY_EXTERN (GST_CAT_DEFAULT);

//-- preset iface

/* buzzmacine preset format
 * header:
 * 4 bytes: version
 * 4 bytes: machine name size
 * n bytes: machine name
 * 4 bytes: number of presets
 *
 * preset:
 * 4 bytes: preset name size
 * n bytes: preset name
 * 4 bytes: number of tracks
 * 4 bytes: number of parameters
 * n * 4 bytes for each parameter
 * 4 bytes: comment size
 * n bytes: comment
 *
 */
static void
gstbml_preset_parse_preset_file (GstBMLClass * klass, const gchar * preset_path)
{
  FILE *in;

  if ((in = fopen (preset_path, "rb"))) {
    guint32 i, version, size, count;
    guint32 tracks, params;
    guint32 *data;
    gchar *machine_name, *preset_name, *comment;

    // read header
    if (!(fread (&version, sizeof (version), 1, in)))
      goto eof_error;
    if (!(fread (&size, sizeof (size), 1, in)))
      goto eof_error;

    machine_name = g_malloc (size + 1);
    if (!(fread (machine_name, size, 1, in)))
      goto eof_error;
    machine_name[size] = '\0';
    // need to cut off path and '.dll'
    if (!strstr (klass->dll_name, machine_name)) {
      GST_WARNING ("Preset for wrong machine? '%s' <> '%s'",
          klass->dll_name, machine_name);
    }

    if (!(fread (&count, sizeof (count), 1, in)))
      goto eof_error;

    GST_INFO
        ("reading %u presets for machine '%s' (version %u, dllname '%s')",
        count, machine_name, version, klass->dll_name);

    // read presets
    for (i = 0; i < count; i++) {
      gboolean add;

      if (!(fread (&size, sizeof (size), 1, in)))
        goto eof_error;

      preset_name = g_malloc (size + 1);
      if (!(fread (preset_name, size, 1, in)))
        goto eof_error;
      preset_name[size] = '\0';
      GST_INFO ("  reading preset %d: %p '%s'", i, preset_name, preset_name);
      if (!(fread (&tracks, sizeof (tracks), 1, in)))
        goto eof_error;
      if (!(fread (&params, sizeof (params), 1, in)))
        goto eof_error;
      // read preset data
      GST_INFO ("  data size %u", (4 * (2 + params)));
      data = g_malloc (4 * (2 + params));
      data[0] = tracks;
      data[1] = params;
      if (!(fread (&data[2], 4 * params, 1, in)))
        goto eof_error;

      add = (g_hash_table_lookup (klass->preset_data, (gpointer) preset_name)
          == NULL);
      g_hash_table_insert (klass->preset_data, (gpointer) preset_name,
          (gpointer) data);

      if (!(fread (&size, sizeof (size), 1, in)))
        goto eof_error;

      if (size) {
        comment = g_malloc0 (size + 1);
        if (!(fread (comment, size, 1, in)))
          goto eof_error;
        g_hash_table_insert (klass->preset_comments, (gpointer) preset_name,
            (gpointer) comment);
      } else {
        comment = NULL;
      }

      GST_INFO ("    %u tracks, %u params, comment '%s'", tracks, params,
          comment);

      if (add) {
        klass->presets =
            g_list_insert_sorted (klass->presets, (gpointer) preset_name,
            (GCompareFunc) strcmp);
      } else {
        g_free (preset_name);
      }
    }
    g_free (machine_name);

  eof_error:
    fclose (in);
  } else {
    GST_INFO ("can't open preset file: '%s'", preset_path);
  }
}

static gchar *
gstbml_preset_make_preset_file_name (GstBMLClass * klass, gchar * preset_dir)
{
  gchar *preset_path, *base_name, *ext;

  if ((base_name = strrchr (klass->preset_path, '/'))) {
    base_name++;
  } else {
    base_name = klass->preset_path;
  }
  ext = g_strrstr (base_name, ".");
  *ext = '\0';                  // temporarily terminate
  preset_path = g_strdup_printf ("%s" G_DIR_SEPARATOR_S "%s.prs", preset_dir,
      base_name);
  *ext = '.';                   // restore

  return preset_path;
}

gchar **
gstbml_preset_get_preset_names (GstBML * bml, GstBMLClass * klass)
{
  if (!klass->presets) {
    gchar *preset_path, *preset_dir;

    // create data hash if its not there
    if (!klass->preset_data) {
      klass->preset_data = g_hash_table_new (g_str_hash, g_str_equal);
    }
    if (!klass->preset_comments) {
      klass->preset_comments = g_hash_table_new (g_str_hash, g_str_equal);
    }
    // load them from local path and then from system path 
    preset_dir = g_build_filename (g_get_user_data_dir (),
        "gstreamer-" GST_MAJORMINOR, "presets", NULL);
    preset_path = gstbml_preset_make_preset_file_name (klass, preset_dir);
    gstbml_preset_parse_preset_file (klass, preset_path);
    g_free (preset_dir);
    g_free (preset_path);
    if (klass->preset_path) {
      gstbml_preset_parse_preset_file (klass, klass->preset_path);
    }
  } else {
    GST_INFO ("return cached preset list");
  }
  if (klass->presets) {
    gchar **preset_names = g_new (gchar *, g_list_length (klass->presets) + 1);
    GList *node;
    guint i = 0;

    for (node = klass->presets; node; node = g_list_next (node)) {
      preset_names[i++] = g_strdup (node->data);
    }
    preset_names[i] = NULL;
    return (preset_names);
  }
  return (NULL);
}

// skip non-controlable, trigger params & voice params
static gboolean
skip_property (GParamSpec * prop, GObjectClass * voice_class)
{
  if (!(prop->flags & GST_PARAM_CONTROLLABLE))
    return TRUE;
  if (!(GPOINTER_TO_INT (g_param_spec_get_qdata (prop,
                  gstbt_property_meta_quark_flags)) &
          GSTBT_PROPERTY_META_STATE))
    return TRUE;
  if (voice_class && g_object_class_find_property (voice_class, prop->name))
    return TRUE;
  return FALSE;
}

static GObjectClass *
get_voice_class (GType voice_type)
{
  return voice_type ? G_OBJECT_CLASS (g_type_class_ref (voice_type)) : NULL;
}

static GList *
get_preset_key_node (GstBMLClass * klass, const gchar * name)
{
  return g_list_find_custom (klass->presets, name, (GCompareFunc) strcmp);
}

static gpointer
get_preset_key (GstBMLClass * klass, const gchar * name)
{
  GList *node = get_preset_key_node (klass, name);
  return node ? node->data : NULL;
}

gboolean
gstbml_preset_load_preset (GstObject * self, GstBML * bml, GstBMLClass * klass,
    const gchar * name)
{
  gpointer key;
  guint32 *data;

  if (!klass->presets) {
    if (!(gstbml_preset_get_preset_names (bml, klass))) {
      GST_INFO ("no presets for machine");
      return FALSE;
    }
  }
  if (!(key = get_preset_key (klass, name))) {
    GST_WARNING ("no preset for '%s'", name);
    return FALSE;
  }
  // get preset data record
  if ((data = g_hash_table_lookup (klass->preset_data, key))) {
    guint32 i, tracks, params;
    GObjectClass *global_class = G_OBJECT_CLASS (GST_ELEMENT_GET_CLASS (self));
    GObjectClass *voice_class = get_voice_class (klass->voice_type);
    GParamSpec **props;
    guint num_props;

    tracks = *data++;
    params = *data++;

    GST_INFO ("about to load preset '%s' with %d tracks and %d total params",
        name, tracks, params);
    GST_INFO ("machine has %d global and %d voice params",
        klass->numglobalparams, klass->numtrackparams);

    // set global parameters
    if ((props = g_object_class_list_properties (global_class, &num_props))) {
      for (i = 0; i < num_props; i++) {
        if (!skip_property (props[i], voice_class))
          g_object_set (self, props[i]->name, *data++, NULL);
        else {
          GST_DEBUG ("skipping preset loading for global param %s",
              props[i]->name);
        }
      }
      g_free (props);
    }
    // set track times voice parameters
    if (voice_class && bml->num_voices) {
      if ((props = g_object_class_list_properties (voice_class, &num_props))) {
        GstBMLV *voice;
        GList *node;
        guint32 j;

        for (j = 0, node = bml->voices; (j < tracks && node);
            j++, node = g_list_next (node)) {
          voice = node->data;
          for (i = 0; i < num_props; i++) {
            if (!skip_property (props[i], NULL))
              g_object_set (voice, props[i]->name, *data++, NULL);
            else {
              GST_DEBUG ("skipping preset loading for voide param %s",
                  props[i]->name);
            }
          }
        }
        g_free (props);
      }
    }

    if (voice_class)
      g_type_class_unref (voice_class);
    return TRUE;
  } else {
    GST_WARNING ("no preset data for '%s'", name);
  }
  return FALSE;
}

static gboolean
gstbml_preset_save_presets_file (GstBMLClass * klass)
{
  gboolean res = FALSE;

  if (klass->preset_path) {
    FILE *out;
    gchar *preset_path, *preset_dir, *bak_file_name;
    gboolean backup = TRUE;

    // always safe to a local path (like in the upstream impl
    preset_dir = g_build_filename (g_get_user_data_dir (),
        "gstreamer-" GST_MAJORMINOR, "presets", NULL);
    g_mkdir_with_parents (preset_dir, 0755);
    preset_path = gstbml_preset_make_preset_file_name (klass, preset_dir);
    g_free (preset_dir);

    // create backup if possible
    bak_file_name = g_strdup_printf ("%s.bak", preset_path);
    if (g_file_test (bak_file_name, G_FILE_TEST_EXISTS)) {
      if (g_unlink (bak_file_name)) {
        backup = FALSE;
        GST_INFO ("cannot remove old backup file : %s", bak_file_name);
      }
    }
    if (backup) {
      if (g_rename (preset_path, bak_file_name)) {
        GST_INFO ("cannot backup file : %s -> %s", preset_path, bak_file_name);
      }
    }
    g_free (bak_file_name);

    GST_INFO ("about to save presets '%s'", preset_path);

    if ((out = fopen (preset_path, "wb"))) {
      guint32 version, size, count;
      gchar *machine_name, *ext, *preset_name, *comment;
      guint32 *data;
      GList *node;

      // write header
      version = 1;
      if (!(fwrite (&version, sizeof (version), 1, out)))
        goto eof_error;

      machine_name = strrchr (klass->dll_name, '/');
      if (!(ext = strstr (machine_name, ".dll"))) {
        if (!(ext = strstr (machine_name, ".so"))) {
          ext = machine_name;
          GST_WARNING ("unknown machine name extension (expected dll/so): %s",
              machine_name);
        }
      }
      size = (gulong) ext - (gulong) machine_name;
      if (!(fwrite (&size, sizeof (size), 1, out)))
        goto eof_error;
      if (!(fwrite (machine_name, size, 1, out)))
        goto eof_error;

      count = g_list_length (klass->presets);
      if (!(fwrite (&count, sizeof (count), 1, out)))
        goto eof_error;

      // write presets
      for (node = klass->presets; node; node = g_list_next (node)) {
        preset_name = node->data;
        size = strlen (preset_name);
        if (!(fwrite (&size, sizeof (size), 1, out)))
          goto eof_error;
        if (!(fwrite (preset_name, size, 1, out)))
          goto eof_error;
        // write preset data (data[1]=params)
        data = g_hash_table_lookup (klass->preset_data, (gpointer) preset_name);
        GST_INFO ("  name %p,'%s'", preset_name, preset_name);
        //GST_INFO("  data size [%s]: %ld",preset_name,(4*(2+data[1])));
        if (!(fwrite (data, (4 * (2 + data[1])), 1, out)))
          goto eof_error;

        // handle comments
        comment =
            g_hash_table_lookup (klass->preset_comments,
            (gpointer) preset_name);
        size = comment ? strlen (comment) : 0;
        if (!(fwrite (&size, sizeof (size), 1, out)))
          goto eof_error;
        if (comment) {
          if (!(fwrite (comment, size, 1, out)))
            goto eof_error;
        }
      }
    eof_error:
      fclose (out);
      res = TRUE;
    } else {
      GST_WARNING ("can't open preset file %s for writing", preset_path);
    }
    g_free (preset_path);
  }
  return res;
}

gboolean
gstbml_preset_save_preset (GstObject * self, GstBML * bml, GstBMLClass * klass,
    const gchar * name)
{
  GObjectClass *global_class = G_OBJECT_CLASS (GST_ELEMENT_GET_CLASS (self));
  GObjectClass *voice_class = get_voice_class (klass->voice_type);
  GParamSpec **props;
  guint num_props;
  guint32 *data, *ptr;
  guint32 i, params, numglobalparams = 0, numtrackparams = 0;

  // count global preset params
  if ((props = g_object_class_list_properties (global_class, &num_props))) {
    for (i = 0; i < num_props; i++) {
      if (!skip_property (props[i], voice_class))
        numglobalparams++;
    }
    g_free (props);
  }
  // count voice preset params
  if (voice_class && bml->num_voices) {
    if ((props = g_object_class_list_properties (voice_class, &num_props))) {
      for (i = 0; i < num_props; i++) {
        if (!skip_property (props[i], NULL))
          numtrackparams++;
      }
      g_free (props);
    }
  }
  // create new preset
  params = numglobalparams + bml->num_voices * numtrackparams;
  GST_INFO ("  data size %u", (4 * (2 + params)));
  data = g_malloc (4 * (2 + params));
  data[0] = bml->num_voices;
  data[1] = params;
  ptr = &data[2];

  GST_INFO ("about to add new preset '%s' with %lu tracks and %u total params",
      name, bml->num_voices, params);

  // get global parameters
  if ((props = g_object_class_list_properties (global_class, &num_props))) {
    for (i = 0; i < num_props; i++) {
      if (!skip_property (props[i], voice_class))
        g_object_get (self, props[i]->name, ptr++, NULL);
    }
    g_free (props);
  }
  // get track times voice parameters
  if (voice_class && bml->num_voices) {
    if ((props = g_object_class_list_properties (voice_class, &num_props))) {
      GstBMLV *voice;
      GList *node;
      guint32 j;

      for (j = 0, node = bml->voices; (j < bml->num_voices && node);
          j++, node = g_list_next (node)) {
        voice = node->data;
        for (i = 0; i < num_props; i++) {
          if (!skip_property (props[i], NULL))
            g_object_get (voice, props[i]->name, ptr++, NULL);
        }
      }
      g_free (props);
    }
  }
  // add new entry
  g_hash_table_insert (klass->preset_data, (gpointer) name, (gpointer) data);
  klass->presets =
      g_list_insert_sorted (klass->presets, (gpointer) name,
      (GCompareFunc) strcmp);

  if (voice_class)
    g_type_class_unref (voice_class);

  return gstbml_preset_save_presets_file (klass);
}

gboolean
gstbml_preset_rename_preset (GstBMLClass * klass, const gchar * old_name,
    const gchar * new_name)
{
  GList *node;

  if ((node = get_preset_key_node (klass, old_name))) {
    gpointer data;

    // move preset data record
    if ((data = g_hash_table_lookup (klass->preset_data, node->data))) {
      g_hash_table_remove (klass->preset_data, node->data);
      g_hash_table_insert (klass->preset_data, (gpointer) new_name,
          (gpointer) data);
    }
    // move preset comment
    if ((data = g_hash_table_lookup (klass->preset_comments, node->data))) {
      g_hash_table_remove (klass->preset_comments, node->data);
      g_hash_table_insert (klass->preset_comments, (gpointer) new_name,
          (gpointer) data);
    }
    // readd under new name
    klass->presets = g_list_delete_link (klass->presets, node);
    klass->presets =
        g_list_insert_sorted (klass->presets, (gpointer) new_name,
        (GCompareFunc) strcmp);
    GST_INFO ("preset moved '%s' -> '%s'", old_name, new_name);
    return gstbml_preset_save_presets_file (klass);
  }
  return FALSE;
}

gboolean
gstbml_preset_delete_preset (GstBMLClass * klass, const gchar * name)
{
  GList *node;
  if ((node = get_preset_key_node (klass, name))) {
    gpointer data;

    // remove preset data record
    if ((data = g_hash_table_lookup (klass->preset_data, node->data))) {
      g_hash_table_remove (klass->preset_data, node->data);
      g_free (data);
    }
    // remove preset comment
    if ((data = g_hash_table_lookup (klass->preset_comments, node->data))) {
      g_hash_table_remove (klass->preset_comments, node->data);
      g_free (data);
    }
    // remove the found one
    klass->presets = g_list_delete_link (klass->presets, node);
    GST_INFO ("preset '%s' removed", name);
    g_free ((gpointer) name);
    return gstbml_preset_save_presets_file (klass);
  }
  return FALSE;
}

gboolean
gstbml_preset_set_meta (GstBMLClass * klass, const gchar * name,
    const gchar * tag, const gchar * value)
{
  if (!strncmp (tag, "comment\0", 9)) {
    gpointer key;
    if ((key = get_preset_key (klass, name))) {
      gchar *old_value;
      gboolean changed = FALSE;
      if ((old_value = g_hash_table_lookup (klass->preset_comments, key))) {
        g_free (old_value);
        changed = TRUE;
      }
      if (value) {
        g_hash_table_insert (klass->preset_comments, key, g_strdup (value));
        changed = TRUE;
      }
      if (changed) {
        return gstbml_preset_save_presets_file (klass);
      }
    }
  }
  return FALSE;
}

gboolean
gstbml_preset_get_meta (GstBMLClass * klass, const gchar * name,
    const gchar * tag, gchar ** value)
{
  if (!strncmp (tag, "comment\0", 9)) {
    gpointer key;
    if ((key = get_preset_key (klass, name))) {
      gchar *new_value;
      if ((new_value = g_hash_table_lookup (klass->preset_comments, key))) {
        *value = g_strdup (new_value);
        return TRUE;
      }
    }
  }
  *value = NULL;
  return FALSE;
}

void
gstbml_preset_finalize (GstBMLClass * klass)
{
  if (klass->presets) {
    GList *node;
    gchar *name;
    gpointer data;

    // free preset_data && preset_comments
    for (node = klass->presets; node; node = g_list_next (node)) {
      name = node->data;
      // remove preset data record
      if ((data = g_hash_table_lookup (klass->preset_data, name))) {
        g_hash_table_remove (klass->preset_data, name);
        g_free (data);
      }
      // remove preset comment
      if ((data = g_hash_table_lookup (klass->preset_comments, name))) {
        g_hash_table_remove (klass->preset_comments, name);
        g_free (data);
      }
      g_free ((gpointer) name);
    }
    g_hash_table_destroy (klass->preset_data);
    klass->preset_data = NULL;
    g_hash_table_destroy (klass->preset_comments);
    klass->preset_comments = NULL;
    g_list_free (klass->presets);
    klass->presets = NULL;
  }
}

//-- common class functions

/**
 * gstbml_convert_names:
 * @klass: the instance class
 * @tmp_name: name to build @name and @nick from
 * @tmp_desc: desc to build @desc from
 * @name: target for name
 * @nick: target for nick
 * @desc: target for description
 *
 * Convert charset encoding and make property-names unique.
 */
void
gstbml_convert_names (GObjectClass * klass, gchar * tmp_name, gchar * tmp_desc,
    gchar ** name, gchar ** nick, gchar ** desc)
{
  gchar *cname, *ptr1, *ptr2;

  GST_DEBUG ("        tmp_name='%s'", tmp_name);
  // TODO(ensonic): do we want different charsets for BML_WRAPPED/BML_NATIVE?
  cname = g_convert (tmp_name, -1, "ASCII", "WINDOWS-1252", NULL, NULL, NULL);
  if (!cname) {
    // weak fallback when conversion failed
    cname = g_strdup (tmp_name);
  }
  if (nick) {
    *nick = g_convert (tmp_name, -1, "UTF-8", "WINDOWS-1252", NULL, NULL, NULL);
  }
  if (desc && tmp_desc) {
    *desc = g_convert (tmp_desc, -1, "UTF-8", "WINDOWS-1252", NULL, NULL, NULL);
  }
  g_strcanon (cname, G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS "-_", '-');

  // remove leading '-'
  ptr1 = ptr2 = cname;
  while (*ptr2 == '-')
    ptr2++;
  // remove double '-'
  while (*ptr2) {
    if (*ptr2 == '-') {
      while (ptr2[1] == '-')
        ptr2++;
    }
    if (ptr1 != ptr2)
      *ptr1 = *ptr2;
    ptr1++;
    ptr2++;
  }
  if (ptr1 != ptr2)
    *ptr1 = '\0';
  // remove trailing '-'
  ptr1--;
  while (*ptr1 == '-')
    *ptr1++ = '\0';

  // name must begin with a char
  if (!g_ascii_isalpha (cname[0])) {
    gchar *old_name = cname;
    cname = g_strconcat ("Par_", old_name, NULL);
    g_free (old_name);
  }
  // check for already existing property names
  if (g_object_class_find_property (klass, cname)) {
    gchar *old_name = cname;
    gchar postfix[5];
    gint i = 0;

    // make name uniqe
    cname = NULL;
    do {
      if (cname)
        g_free (cname);
      snprintf (postfix, 5, "_%03d", i++);
      cname = g_strconcat (old_name, postfix, NULL);
    } while (g_object_class_find_property (klass, cname));
    g_free (old_name);
  }
  *name = cname;
}

/**
 * gstbml_register_param:
 * @klass: the instance class
 * @prop_id: the property number
 * @type: the parameter type
 * @enum_type: the enum type (or 0)
 * @name: the property name
 * @nick: the property nick
 * @desc: the property description
 * @flags: extra property flags
 * @min_val: minimum value
 * @max_val: maximum value
 * @no_val: unset value (neutral)
 * @def_val: default value
 *
 * Normalize data and create a paramspec.
 *
 * Returns: the param spec
 */
GParamSpec *
gstbml_register_param (GObjectClass * klass, gint prop_id,
    GstBMLParameterTypes type, GType enum_type, gchar * name, gchar * nick,
    gchar * desc, gint flags, gint min_val, gint max_val, gint no_val,
    gint def_val)
{
  GParamSpec *paramspec = NULL;
  gint saved_min_val = min_val, saved_max_val = max_val, saved_def_val =
      def_val;
  GParamFlags pspec_flags = G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE;

  GST_DEBUG
      ("        name='%s', nick='%s', type=%d, flags=0x%x : val [%d ... d=%d/n=%d ... %d]",
      name, nick, type, flags, min_val, def_val, no_val, max_val);
  //*def = defval;
  if (type != PT_SWITCH) {
    // for switch params usually min=max==-1
    if (def_val < min_val) {
      GST_WARNING ("par=%d:%s, def_val < min_val", type, name);
      saved_min_val = min_val;
      min_val = def_val;
    }
    if (def_val > max_val) {
      GST_WARNING ("par=%d:%s, def_val > max_val", type, name);
      saved_max_val = max_val;
      max_val = def_val;
    }
  } else {
    // avoid the min=max==-1 situation
    min_val = 0;
    max_val = 1;
  }
  if (!(flags & GSTBT_PROPERTY_META_STATE)) {
    // only trigger params need no_val handling
    if (no_val < min_val) {
      if (min_val != -1) {
        GST_WARNING ("par=%d:%s, no_val < min_val", type, name);
      }
      saved_min_val = min_val;
      min_val = no_val;
    }
    if (no_val > max_val) {
      if (max_val != -1) {
        GST_WARNING ("par=%d:%s, no_val > max_val", type, name);
      }
      saved_max_val = max_val;
      max_val = no_val;
    }
    def_val = no_val;
  } else {
    pspec_flags |= G_PARAM_READABLE;
  }
  GST_DEBUG ("        val [%d ... d=%d ... %d]", min_val, def_val, max_val);

  switch (type) {
    case PT_NOTE:
      paramspec = g_param_spec_enum (name, nick, desc,
          GSTBT_TYPE_NOTE, GSTBT_NOTE_NONE, pspec_flags);
      break;
    case PT_SWITCH:
      //if(!(flags&GSTBT_PROPERTY_META_STATE)) {
      /* TODO(ensonic): use better type for triggers
       * this is how its define for buzz
       * [ 0 ... n=255 ... 1]
       *
       * some machines have random stuff in here :(
       * [-1 ... d=  0/n=255 ... -1] -> [-1 ... d=255 ... 255] [-1 ...   0]
       * [ 1 ... d=255/n=255 ...  1] -> [ 1 ... d=255 ... 255] [ 1 ... 255]
       * [ 1 ... d=  0/n=255 ...  1] -> [ 0 ... d=255 ... 255]
       *
       * - we need to differenciate between PT_SWITCH as param or a trigger
       *   as trigger ones have a no-value
       * - using an uint with 0,1,255 is quite bad
       * - what about an enum OFF,ON,NO_VALUE?
       */
      /*
         if(min_val==-1) min_val=0;
         paramspec=g_param_spec_uint(name, nick, desc,
         min_val,max_val,def_val,
         pspec_flags);
       */
      type = PT_ENUM;
      paramspec = g_param_spec_enum (name, nick, desc,
          GSTBT_TYPE_TRIGGER_SWITCH, GSTBT_TRIGGER_SWITCH_EMPTY, pspec_flags);
      /*}
         else {
         // non-triggers use this
         //min_val=0;max_val=1;
         paramspec=g_param_spec_boolean(name, nick, desc,
         def_val,
         pspec_flags);
         } */
      break;
    case PT_BYTE:
      // TODO(ensonic): gstreamer has no support for CHAR/UCHAR
      paramspec = g_param_spec_uint (name, nick, desc,
          min_val, max_val, def_val, pspec_flags);
      break;
    case PT_WORD:
      paramspec = g_param_spec_uint (name, nick, desc,
          min_val, max_val, def_val, pspec_flags);
      break;
    case PT_ENUM:
      paramspec = g_param_spec_enum (name, nick, desc,
          enum_type, def_val, pspec_flags);
      break;
    case PT_ATTR:              // Attribute
      paramspec = g_param_spec_int (name, nick, desc,
          min_val, max_val, def_val, G_PARAM_READWRITE);
      break;
    default:
      GST_WARNING ("invalid type=%d", type);
      break;
  }
  if (paramspec) {
    // TODO(ensonic): can we skip the gstbt_property_meta qdata business for some parameters?
    g_param_spec_set_qdata (paramspec, gstbt_property_meta_quark,
        GINT_TO_POINTER (TRUE));
    g_param_spec_set_qdata (paramspec, gstbt_property_meta_quark_min_val,
        GINT_TO_POINTER (saved_min_val));
    g_param_spec_set_qdata (paramspec, gstbt_property_meta_quark_max_val,
        GINT_TO_POINTER (saved_max_val));
    g_param_spec_set_qdata (paramspec, gstbt_property_meta_quark_def_val,
        GINT_TO_POINTER (saved_def_val));
    g_param_spec_set_qdata (paramspec, gstbt_property_meta_quark_no_val,
        GINT_TO_POINTER (no_val));
    g_param_spec_set_qdata (paramspec, gstbt_property_meta_quark_flags,
        GINT_TO_POINTER (flags));
    g_param_spec_set_qdata (paramspec, gst_bml_property_meta_quark_type,
        GINT_TO_POINTER (type));
    g_object_class_install_property (klass, prop_id, paramspec);
    GST_DEBUG ("registered paramspec=%p", paramspec);
  } else {
    GST_WARNING ("failed to create paramspec");
  }
  return (paramspec);
}

//-- common element functions

/**
 * gstbml_get_param:
 * @type: parameter type
 * @value: the source
 *
 * Get a parameter according to @type from the @value.
 *
 * Return: the content value.
 */
gint
gstbml_get_param (GstBMLParameterTypes type, const GValue * value)
{
  gint ret = 0;

  switch (type) {
    case PT_NOTE:
      ret = (gint) g_value_get_enum (value);
      break;
    case PT_SWITCH:
      ret = (gint) g_value_get_boolean (value);
      break;
    case PT_BYTE:
      ret = (gint) g_value_get_uint (value);
      break;
    case PT_WORD:
      ret = (gint) g_value_get_uint (value);
      break;
    case PT_ENUM:
      ret = g_value_get_enum (value);
      break;
    default:
      GST_WARNING ("unhandled type : %d", type);
  }
  return ret;
}

/**
 * gstbml_set_param:
 * @type: parameter type
 * @val: the new content
 * @value: the target
 *
 * Store a parameter according to @type from @val into the @value.
 */
void
gstbml_set_param (GstBMLParameterTypes type, gint val, GValue * value)
{
  switch (type) {
    case PT_NOTE:
      //g_value_set_string(value, gstbt_tone_conversion_note_number_2_string (val));
      break;
    case PT_SWITCH:
      g_value_set_boolean (value, val);
      break;
    case PT_BYTE:
      g_value_set_uint (value, (guint) val);
      break;
    case PT_WORD:
      g_value_set_uint (value, (guint) val);
      break;
    case PT_ENUM:
      g_value_set_enum (value, val);
      break;
    default:
      GST_WARNING ("unhandled type : %d", type);
  }
}

/**
 * gstbml_calculate_buffer_size:
 * @bml: bml instance
 *
 * Calculate the buffer size in bytes.
 *
 * Returns: the buffer size in bytes
 */
guint
gstbml_calculate_buffer_size (GstBML * bml)
{
  // TODO(ensonic): need channels
  // bml_class->output_channels
  return 2 * bml->samples_per_buffer * sizeof (BMLData);
}

/**
 * gstbml_calculate_buffer_frames:
 * @bml: bml instance
 *
 * Update the buffersize for calculation (in samples)
 * buffer_frames = samples_per_minute/ticks_per_minute
 */
void
gstbml_calculate_buffer_frames (GstBML * bml)
{
  const gdouble ticks_per_minute =
      (gdouble) (bml->beats_per_minute * bml->ticks_per_beat);
  const gdouble div = 60.0 / bml->subticks_per_tick;
  const gdouble subticktime = ((GST_SECOND * div) / ticks_per_minute);
  GstClockTime ticktime =
      (GstClockTime) (0.5 + ((GST_SECOND * 60.0) / ticks_per_minute));

  bml->samples_per_buffer = ((bml->samplerate * div) / ticks_per_minute);
  bml->ticktime = (GstClockTime) (0.5 + subticktime);
  GST_DEBUG ("samples_per_buffer=%d", bml->samples_per_buffer);
  // the sequence is quantized to ticks and not subticks
  // we need to compensate for the rounding errors :/
  bml->ticktime_err =
      ((gdouble) ticktime -
      (gdouble) (bml->subticks_per_tick * bml->ticktime)) /
      (gdouble) bml->subticks_per_tick;
  GST_DEBUG ("ticktime err=%lf", bml->ticktime_err);
}

/**
 * gstbml_dispose:
 * @bml: bml instance
 *
 * Dispose common buzzmachine data.
 */
void
gstbml_dispose (GstBML * bml)
{
  GList *node;

  if (bml->dispose_has_run)
    return;
  bml->dispose_has_run = TRUE;

  GST_DEBUG_OBJECT (bml->self, "!!!! bml=%p", bml);

  // unref list of voices
  if (bml->voices) {
    for (node = bml->voices; node; node = g_list_next (node)) {
      GstObject *obj = node->data;
      GST_DEBUG ("  free voice : %p (%d)", obj, G_OBJECT (obj)->ref_count);
      gst_object_unparent (obj);
      // no need to unref, the unparent does that
      //g_object_unref(node->data);
      node->data = NULL;
    }
  }
}

/**
 * gstbml_fix_data:
 * @elem: the element instance
 * @info: the GstMapInfo for the data buffer
 * @has_data: indication wheter the buffer has values != 0.0
 *
 * Fixes elements that output denormalized values (sets them to 0.0).
 *
 * Returns: TRUE if the gap flag could be set
 */
gboolean
gstbml_fix_data (GstElement * elem, GstMapInfo * info, gboolean has_data)
{
  BMLData *data = (BMLData *) info->data;
  guint i, num_samples = info->size / sizeof (BMLData);

  if (has_data) {
    has_data = FALSE;

    // see also http://www.musicdsp.org/archive.php?classid=5#191
    for (i = 0; i < num_samples; i++) {
      if (data[i] != 0.0) {
        has_data = TRUE;
        break;
      }
/* FIXME(ensonic): we configure the FPU to DAZ|FZ in libbuzztrax-core
 * this would not apply to other clients
 */
#if 0
      /* isnormal checks for != zero */
      if (G_LIKELY (isnormal (data[i]))) {
        has_data = TRUE;
        break;
      } else {
        if (isnan (data[i])) {
          GST_WARNING_OBJECT (elem, "data contains NaN");
        } else if (isinf (data[i])) {
          GST_WARNING_OBJECT (elem, "data contains Inf");
        } else if (data[i] != 0.0) {    //fpclassify(data[i])==FP_SUBNORMAL
          GST_WARNING_OBJECT (bml_transform, "data contains Denormal");
        }
        data[i] = 0.0;
      }
#endif
    }
#if 0
    for (; i < num_samples; i++) {
      if (G_UNLIKELY (!isnormal (data[i]))) {
        data[i] = 0.0;
      }
    }
#endif
  }
  if (!has_data) {
    GST_LOG_OBJECT (elem, "silent buffer");
    return TRUE;
  } else {
    GST_LOG_OBJECT (elem, "signal buffer");
    // buzz generates relative loud output
    gfloat fc = 1.0 / 32768.0;
    orc_scalarmultiply_f32_ns (data, data, fc, num_samples);
    return FALSE;
  }
}
