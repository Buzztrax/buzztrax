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
 * SECTION:bml
 * @title: GstBml
 * @short_description: buzzmachine wrapper
 *
 * Wrapper for buzzmachine sound generators and effects.
 */

#include "plugin.h"

#define GST_CAT_DEFAULT bml_debug
GST_DEBUG_CATEGORY_EXTERN (GST_CAT_DEFAULT);

extern GstStructure *bml_meta_all;
extern GstPlugin *bml_plugin;
extern GHashTable *bml_category_by_machine_name;

/* needed in gstbmlv.c */
gpointer bml (voice_class_bmh);

//-- helper

static void
remove_double_def_chars (gchar * name)
{
  gchar *ptr1, *ptr2;

  ptr1 = ptr2 = name;
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
}

static gboolean
bml (describe_plugin (gchar * pathname, gpointer bmh))
{
  gchar *dllname;
  gint type, flags;
  gboolean res = FALSE;

  GST_INFO ("describing %p : %s", bmh, pathname);

  // use dllname to register
  // 'M3.dll' and 'M3 Pro.dll' both use the name M3 :(
  // BM_PROP_DLL_NAME instead of BM_PROP_SHORT_NAME

  g_assert (bmh);
  if (bml (get_machine_info (bmh, BM_PROP_DLL_NAME, (void *) &dllname)) &&
      bml (get_machine_info (bmh, BM_PROP_TYPE, (void *) &type)) &&
      bml (get_machine_info (bmh, BM_PROP_FLAGS, (void *) &flags))
      ) {
    gchar *name, *filename, *ext;
    gchar *element_type_name, *voice_type_name = NULL;
    gchar *help_filename, *preset_filename;
    gchar *data_pathname = NULL;
    gboolean type_names_ok = TRUE;
    GstStructure *bml_meta;
    GError *error = NULL;
    if ((filename = strrchr (dllname, '/'))) {
      filename++;
    } else {
      filename = dllname;
    }
    GST_INFO ("  dll-name: '%s', flags are: %x", filename, flags);
    if (flags & 0xFC) {
      // we only support, MONO_TO_STEREO(1), PLAYS_WAVES(2) yet
      GST_WARNING ("  machine is not yet fully supported, flags: %x", flags);
    }
    // get basename (filenames should have an extension, but lets be defensive)
    if ((ext = g_strrstr (filename, "."))) {
      *ext = '\0';              // temporarily terminate
    }
    if (!(name =
            g_convert_with_fallback (filename, -1, "ASCII", "WINDOWS-1252", "-",
                NULL, NULL, &error))) {
      GST_WARNING ("trouble converting filename: '%s': %s", filename,
          error->message);
      g_error_free (error);
      error = NULL;
    }
    if (name) {
      g_strstrip (name);
    }
    if (ext) {
      *ext = '.';               // restore
    }
    GST_INFO ("  name is '%s'", name);

    // construct the type name
    element_type_name = g_strdup_printf ("bml-%s", name);
    if (bml (gstbml_is_polyphonic (bmh))) {
      voice_type_name = g_strdup_printf ("bmlv-%s", name);
    }
    g_free (name);
    // if it's already registered, skip (mean e.g. native element has been registered)
    g_strcanon (element_type_name, G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS "-+",
        '-');
    remove_double_def_chars (element_type_name);
    if (g_type_from_name (element_type_name)) {
      GST_WARNING ("already registered type : \"%s\"", element_type_name);
      g_free (element_type_name);
      element_type_name = NULL;
      type_names_ok = FALSE;
    }
    if (voice_type_name) {
      g_strcanon (voice_type_name, G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS "-+",
          '-');
      remove_double_def_chars (voice_type_name);
      if (g_type_from_name (voice_type_name)) {
        GST_WARNING ("already registered type : \"%s\"", voice_type_name);
        g_free (voice_type_name);
        voice_type_name = NULL;
        type_names_ok = FALSE;
      }
    }
    if (!type_names_ok) {
      g_free (element_type_name);
      g_free (voice_type_name);
      return TRUE;
    }
    // create the metadata store
#ifdef BML_WRAPPED
    bml_meta = gst_structure_new_empty ("bmlw");
#else
    bml_meta = gst_structure_new_empty ("bmln");
#endif

    // use right path for emulated and native machines
#ifdef BML_WRAPPED
    data_pathname = g_strdup (pathname);
#else
    {
      gchar *pos;
      // replace lib in path-name wth share (should we use DATADIR/???)
      pos = strstr (pathname, "/lib/");
      if (pos) {
        *pos = '\0';
        data_pathname = g_strdup_printf ("%s/share/%s", pathname, &pos[5]);
        *pos = '/';
      } else {
        GST_WARNING ("failed to map plugin lib path '%s' to a datadir",
            pathname);
        data_pathname = g_strdup (pathname);
      }
    }
#endif
    // data files have same basename as plugin, but different extension
    ext = strrchr (data_pathname, '.');
    *ext = '\0';
    // try various help uris
    help_filename = g_strdup_printf ("%s.html", data_pathname);
    if (!g_file_test (help_filename, G_FILE_TEST_EXISTS)) {
      GST_INFO ("no user docs at '%s'", help_filename);
      g_free (help_filename);
      help_filename = g_strdup_printf ("%s.htm", data_pathname);
      if (!g_file_test (help_filename, G_FILE_TEST_EXISTS)) {
        GST_INFO ("no user docs at '%s'", help_filename);
        g_free (help_filename);
        help_filename = g_strdup_printf ("%s.txt", data_pathname);
        if (!g_file_test (help_filename, G_FILE_TEST_EXISTS)) {
          GST_INFO ("no user docs at '%s'", help_filename);
          g_free (help_filename);
          help_filename = NULL;
        }
      }
    }
    // generate preset name
    // we need a fallback to g_get_user_config_dir()/??? in any case
    preset_filename = g_strdup_printf ("%s.prs", data_pathname);
    g_free (data_pathname);

    // store help uri
    if (help_filename) {
      gchar *help_uri;
      help_uri = g_strdup_printf ("file://%s", help_filename);
      GST_INFO ("machine %p has user docs at '%s'", bmh, help_uri);
      g_free (help_filename);
      gst_structure_set (bml_meta, "help-filename", G_TYPE_STRING, help_uri,
          NULL);
    }
    // store preset path
    if (preset_filename) {
      GST_INFO ("machine %p preset path '%s'", bmh, preset_filename);
      gst_structure_set (bml_meta, "preset-filename", G_TYPE_STRING,
          preset_filename, NULL);
    }
    if (voice_type_name) {
      gst_structure_set (bml_meta, "voice-type-name", G_TYPE_STRING,
          voice_type_name, NULL);
    }

    gst_structure_set (bml_meta,
        "plugin-filename", G_TYPE_STRING, pathname,
        "machine-type", G_TYPE_INT, type,
        "element-type-name", G_TYPE_STRING, element_type_name, NULL);

    {
      gchar *str, *extra_categories;

      /* this does not yet match all machines, e.g. Elak SVF
       * we could try ("%s %s",author,longname) in addition? */
      bml (get_machine_info (bmh, BM_PROP_NAME, (void *) &str));
      str = g_convert (str, -1, "UTF-8", "WINDOWS-1252", NULL, NULL, NULL);
      extra_categories =
          g_hash_table_lookup (bml_category_by_machine_name, str);
      g_free (str);
      if (extra_categories) {
        gst_structure_set (bml_meta, "categories", G_TYPE_STRING,
            extra_categories, NULL);
      }
    }

    // store metadata cache
    {
      GValue value = { 0, };

      GST_INFO ("caching data: type_name=%s, file_name=%s", element_type_name,
          pathname);
      g_value_init (&value, GST_TYPE_STRUCTURE);
      g_value_set_boxed (&value, bml_meta);
      gst_structure_set_value (bml_meta_all, element_type_name, &value);
      g_value_unset (&value);
    }
    res = TRUE;
  }
  return res;
}

gboolean
bml (gstbml_inspect (gchar * file_name))
{
  gpointer bmh;
  gboolean res = FALSE;

  if ((bmh = bml (open (file_name)))) {
    if (bml (describe_plugin (file_name, bmh))) {
      res = TRUE;
    }
    bml (close (bmh));
  } else {
    GST_WARNING ("machine %s could not be loaded", file_name);
  }
  return res;
}

gboolean
bml (gstbml_register_element (GstPlugin * plugin, GstStructure * bml_meta))
{
  const gchar *element_type_name =
      gst_structure_get_string (bml_meta, "element-type-name");
  const gchar *voice_type_name =
      gst_structure_get_string (bml_meta, "voice-type-name");
  gint type;
  GType element_type = G_TYPE_INVALID, voice_type = G_TYPE_INVALID;
  gboolean res = FALSE;

  gst_structure_get_int (bml_meta, "machine-type", &type);

  // create the voice type, if needed
  if (voice_type_name) {
    // create the voice type now
    voice_type = bml (v_get_type (voice_type_name));
    GST_INFO ("  voice type \"%s\" is 0x%lu", voice_type_name,
        (gulong) voice_type);
  }
  // create the element type
  switch (type) {
    case MT_MASTER:            // (Sink)
      //element_type = bml(sink_get_type(element_type_name));
      GST_WARNING ("  unimplemented plugin type %d for '%s'", type,
          element_type_name);
      break;
    case MT_GENERATOR:         // (Source)
      element_type =
          bml (src_get_type (element_type_name, (voice_type_name != NULL)));
      break;
    case MT_EFFECT:            // (Processor)
      // base transform only supports elements with one source and one sink pad
      element_type =
          bml (transform_get_type (element_type_name,
              (voice_type_name != NULL)));
      break;
    default:
      GST_WARNING ("  invalid plugin type %d for '%s'", type,
          element_type_name);
  }
  if (element_type) {
    if (!gst_element_register (plugin, element_type_name, GST_RANK_NONE,
            element_type)) {
      GST_ERROR ("error registering new type : \"%s\"", element_type_name);
    } else {
      GST_INFO ("succefully registered new plugin : \"%s\"", element_type_name);
      res = TRUE;
    }
  }
  return res;
}

/*
 * bml_is_polyphonic:
 *
 * Test if a buzzmachine supports voices.
 *
 * Returns: %TRUE if the machine is polyphonic
 */
gboolean
bml (gstbml_is_polyphonic (gpointer bmh))
{
  int track_params = 0;

  if (bml (get_machine_info (bmh, BM_PROP_NUM_TRACK_PARAMS,
              (void *) &track_params))) {
    if (track_params > 0)
      return TRUE;
  }
  return FALSE;
}


//-- common iface functions

gchar *
bml (gstbml_property_meta_describe_property (GstBMLClass * bml_class,
        GstBML * bml, guint prop_id, const GValue * value))
{
  const gchar *str = NULL;
  gpointer bmh = bml_class->bmh;
  gpointer bm = bml->bm;
  gchar *res;
  gchar def[20];
  GType base, type = G_VALUE_TYPE (value);
  guint props_skip = ARG_LAST - 1;

  if (bml (gstbml_is_polyphonic (bm))) {
    props_skip++;
  }
  // property ids have an offset of 1
  prop_id -= (props_skip + bml_class->numattributes + 1);

  while ((base = g_type_parent (type)))
    type = base;

  switch (type) {
    case G_TYPE_INT:
      if (!(str =
              bml (describe_global_value (bmh, prop_id,
                      g_value_get_int (value)))) || !*str) {
        snprintf (def, sizeof(def), "%d", g_value_get_int (value));
        str = def;
      }
      break;
    case G_TYPE_UINT:
      if (!(str =
              bml (describe_global_value (bmh, prop_id,
                      (gint) g_value_get_uint (value)))) || !*str) {
        snprintf (def, sizeof(def), "%u", g_value_get_uint (value));
        str = def;
      }
      break;
    case G_TYPE_ENUM:
      if (!(str =
              bml (describe_global_value (bmh, prop_id,
                      g_value_get_enum (value)))) || !*str) {
        // TODO(ensonic): get blurb for enum value
        snprintf (def, sizeof(def), "%d", g_value_get_enum (value));
        str = def;
      }
      break;
    case G_TYPE_STRING:
      return g_strdup_value_contents (value);
      break;
    default:
      GST_ERROR ("unsupported GType='%s'", G_VALUE_TYPE_NAME (value));
      return g_strdup_value_contents (value);
  }
  if (str == def) {
    res = g_strdup (str);
  } else {
    res = g_convert (str, -1, "UTF-8", "WINDOWS-1252", NULL, NULL, NULL);
  }
  GST_INFO ("formatted global parameter : '%s'", res);
  return res;
}

void
bml (gstbml_tempo_change_tempo (GObject * gstbml, GstBML * bml,
        guint beats_per_minute, guint ticks_per_beat, guint subticks_per_tick))
{
  if (bml->beats_per_minute != beats_per_minute ||
      bml->ticks_per_beat != ticks_per_beat ||
      bml->subticks_per_tick != subticks_per_tick) {
    bml->beats_per_minute = (gulong) beats_per_minute;
    bml->ticks_per_beat = (gulong) ticks_per_beat;
    bml->subticks_per_tick = (gulong) subticks_per_tick;

    GST_INFO ("changing tempo to %u BPM  %u TPB  %u STPT",
        beats_per_minute, ticks_per_beat, subticks_per_tick);
    gstbml_calculate_buffer_frames (bml);
    if (GST_IS_BASE_SRC (gstbml)) {
      gst_base_src_set_blocksize (GST_BASE_SRC (gstbml),
          gstbml_calculate_buffer_size (bml));
    }
    // update timevalues in buzzmachine
    bml (set_master_info (bml->beats_per_minute, bml->ticks_per_beat,
            bml->samplerate));
    // if we call init here, it resets all parameter to deaults
    // including the voices to zero
    // TODO(ensonic): we need to do a test in EnsonicBCT to see what buzz calls when a machine
    //        exists and one changes the tempo
    //bml(init(bml->bm,0,NULL));
  }
}

//-- common class functions

/*
 * gstbml_class_base_init:
 *
 * store common class params
 */
gpointer
bml (gstbml_class_base_init (GstBMLClass * klass, GType type, gint numsrcpads,
        gint numsinkpads))
{
  gpointer bmh;
  GType voice_type = G_TYPE_INVALID;
  const GValue *value =
      gst_structure_get_value (bml_meta_all, g_type_name (type));
  GstStructure *bml_meta = g_value_get_boxed (value);
  const gchar *voice_type_name =
      gst_structure_get_string (bml_meta, "voice-type-name");
  const gchar *dll_name;

  GST_INFO ("initializing base: type=0x%lu", (gulong) type);

  dll_name = (gchar *) gst_structure_get_string (bml_meta, "plugin-filename");

  klass->dll_name = g_filename_from_utf8 (dll_name, -1, NULL, NULL, NULL);
  klass->help_uri =
      (gchar *) gst_structure_get_string (bml_meta, "help-filename");
  klass->preset_path =
      (gchar *) gst_structure_get_string (bml_meta, "preset-filename");
  GST_INFO ("initializing base: type_name=%s, file_name=%s, preset_path=%s",
      g_type_name (type), klass->dll_name, klass->preset_path);

  bmh = bml (open (klass->dll_name));
  g_assert (bmh);

  GST_INFO ("  bmh=0x%p", bmh);

  /* we now need to ensure that gst_bmlv_class_init() get bmh */
  if (voice_type_name) {
    GST_INFO ("prepare voice-type %s", voice_type_name);

    voice_type = g_type_from_name (voice_type_name);
    bml (voice_class_bmh) = bmh;
    //g_hash_table_insert(bml_descriptors_by_voice_type,GINT_TO_POINTER(voice_type),(gpointer)bmh);
    g_type_class_ref (voice_type);
  }

  GST_INFO ("initializing base: bmh=0x%p, dll_name=%s, voice_type=0x%lu", bmh,
      ((klass->dll_name) ? klass->dll_name : "?"), (gulong) voice_type);

  klass->bmh = bmh;
  klass->voice_type = voice_type;
  klass->numsrcpads = numsrcpads;
  klass->numsinkpads = numsinkpads;

  GST_INFO ("initializing base: docs='%s', presets='%s'", klass->help_uri,
      klass->preset_path);

  if (!bml (get_machine_info (bmh, BM_PROP_NUM_INPUT_CHANNELS,
              (void *) &klass->input_channels))
      || !bml (get_machine_info (bmh, BM_PROP_NUM_OUTPUT_CHANNELS,
              (void *) &klass->output_channels))) {

    gint flags;

    bml (get_machine_info (bmh, BM_PROP_FLAGS, (void *) &flags));
    klass->input_channels = klass->output_channels = 1;
    // MIF_MONO_TO_STEREO
    if (flags & 1)
      klass->output_channels = 2;
  }

  return bmh;
}

/* it seems that this is never called :/
 * due to that we 'leak' one instance of every buzzmachine we loaded
 *
 * FIXME(ensonic): count instances in _init() and call this on last finalize?
 * - might not work as base_init won't be called again, we'd need to init the
 *   klass variables lazily from _init
 */
void
bml (gstbml_base_finalize (GstBMLClass * klass))
{
  GST_INFO ("!!!! klass=%p", klass);
  bml (close (klass->bmh));
  g_free (klass->dll_name);
}

/*
 * gstbml_class_set_details:
 *
 * Get metadata from buzz-machine and set as element details
 */
void
bml (gstbml_class_set_details (GstElementClass * klass, GstBMLClass * bml_class,
        gpointer bmh, const gchar * category))
{
  gchar *str;
  GType type = G_TYPE_FROM_CLASS (klass);
  const GValue *value =
      gst_structure_get_value (bml_meta_all, g_type_name (type));
  GstStructure *bml_meta = g_value_get_boxed (value);
  const gchar *extra_categories =
      gst_structure_get_string (bml_meta, "categories");
  gchar *longname, *categories, *desc, *author;

  /* construct the element details struct */
  // TODO(ensonic): do we want different charsets for BML_WRAPPED/BML_NATIVE?
  bml (get_machine_info (bmh, BM_PROP_SHORT_NAME, (void *) &str));
  longname = g_convert (str, -1, "UTF-8", "WINDOWS-1252", NULL, NULL, NULL);
  bml (get_machine_info (bmh, BM_PROP_NAME, (void *) &str));
  desc = g_convert (str, -1, "UTF-8", "WINDOWS-1252", NULL, NULL, NULL);
  bml (get_machine_info (bmh, BM_PROP_AUTHOR, (void *) &str));
  author = g_convert (str, -1, "UTF-8", "WINDOWS-1252", NULL, NULL, NULL);
  if (extra_categories) {
    GST_DEBUG (" -> %s", extra_categories);
    categories = g_strconcat (category, extra_categories, NULL);
  } else {
    categories = g_strdup ((gchar *) category);
  }
  gst_element_class_set_metadata (klass, longname, categories, desc, author);
  g_free (longname);
  g_free (desc);
  g_free (author);
  g_free (categories);
  if (bml_class->help_uri) {
    gst_element_class_add_metadata (klass, GST_ELEMENT_METADATA_DOC_URI,
        bml_class->help_uri);
  }
  GST_DEBUG ("  element_class details have been set");
}

static GType
gst_bml_register_global_enum_type (GObjectClass * klass, gpointer bmh, gint i,
    gchar * name, gint min_val, gint max_val, gint no_val, gint def_val)
{
  GType enum_type = G_TYPE_INVALID;
  const gchar *desc;
  gchar *type_name;
  gint extra = 1;

  GST_INFO ("check enum, (entries=(%d-%d)=%d), no_val=%d, def_val=%d",
      max_val, min_val, ((max_val + 1) - min_val), no_val, def_val);

  if ((def_val < min_val) || (def_val > max_val)) {
    extra++;
  }
  if ((no_val < min_val) || (no_val > max_val)) {
    extra++;
  }
  // build type name
  type_name =
      g_strdup_printf ("%s%s", g_type_name (G_TYPE_FROM_CLASS (klass)), name);
  if (!(enum_type = g_type_from_name (type_name))) {
    gint j, total = (max_val + 1) - min_val, vcount = 0, tcount = 0;
    GEnumValue *enums;

    // count entries that contain mostly characters
    for (j = 0; j < total; j++) {
      desc = bml (describe_global_value (bmh, i, min_val + j));
      if (desc) {
        gint k = 0, nc = 0, cc = 0;
        vcount++;
        // count numbers and characters
        while (desc[k]) {
          if (g_ascii_isalpha (desc[k++]))
            cc++;
          else
            nc++;
        }
        // if there are more/eq characters than numbers assume it is text
        if (cc >= nc) {
          tcount++;
          GST_DEBUG ("check enum, description[%2d]='%s'", j, desc);
        }
      }
    }

    // DEBUG
    //if(count>50) {
    //  GST_WARNING("lots of entries (%d-%d)=%d)?",max_val,min_val,(max_val-min_val));
    //  count=50;
    //}
    // DEBUG

    // some plugins just have text for only val=min/max
    // don't make an enum for those
    //if(total-tcount<=2) {
    if (tcount >= (total >> 1)) {
      gint k = 0;
      // this we can never free :(
      enums = g_new (GEnumValue, vcount + extra);
      if (def_val < min_val) {
        enums[k].value = def_val;
        enums[k].value_name = enums[k].value_nick = "";
        k++;
      }
      if (no_val < min_val) {
        enums[k].value = no_val;
        enums[k].value_name = enums[k].value_nick = "";
        k++;
      }
      // create an enum type
      for (j = 0; j < total; j++) {
        desc = bml (describe_global_value (bmh, i, min_val + j));
        //if((j==no_val) && !desc) desc=" ";
        //if(desc && g_ascii_isalpha(desc[0])) {
        if (desc) {
          enums[k].value = min_val + j;
          // we have to copy these as buzzmachines can reuse the memory we get from describe()
          enums[k].value_nick = enums[k].value_name =
              g_convert ((gchar *) desc, -1, "UTF-8", "WINDOWS-1252", NULL,
              NULL, NULL);
          k++;
        }
      }
      if (def_val > max_val) {
        enums[k].value = def_val;
        enums[k].value_name = enums[k].value_nick = "";
        k++;
      }
      if (no_val > max_val) {
        enums[k].value = no_val;
        enums[k].value_name = enums[k].value_nick = "";
        k++;
      }
      // terminator
      enums[k].value = 0;
      enums[k].value_name = enums[k].value_nick = NULL;

      enum_type = g_enum_register_static (type_name, enums);
      GST_INFO ("register enum '%s' with %d values", type_name, vcount);
    } else {
      GST_INFO ("not making enum '%s' with %d text of %d total values",
          type_name, tcount, total);
    }
  } else {
    GST_INFO ("existing enum '%s'", type_name);
  }
  g_free (type_name);
  return enum_type;
}

GType
bml (gstbml_register_track_enum_type (GObjectClass * klass, gpointer bmh,
        gint i, gchar * name, gint min_val, gint max_val, gint no_val))
{
  GType enum_type = G_TYPE_INVALID;
  const gchar *desc;

  desc = bml (describe_track_value (bmh, i, min_val));
  GST_INFO ("check enum, description= '%s', (entries=(%d-%d)=%d), no_val=%d",
      desc, max_val, min_val, ((max_val + 1) - min_val), no_val);

  //if(desc && g_ascii_isalpha(desc[0])) {
  gchar *type_name;
  const gchar *class_type_name;

  // build type name
  // we need to avoid creating this for GstBML and GstBMLV
  // TODO(ensonic): if we have done it for GstBML, can't we look it up and return it ?
  class_type_name = g_type_name (G_TYPE_FROM_CLASS (klass));
  if (strncmp (class_type_name, "bmlv-", 5)) {
    type_name = g_strdup_printf ("%s%s", class_type_name, name);
  } else {
    type_name = g_strdup_printf ("bmlv-%s%s", &class_type_name[5], name);
  }
  if (!(enum_type = g_type_from_name (type_name))) {
    gint j, k, total = (max_val + 1) - min_val, vcount = 0, tcount = 0;
    GEnumValue *enums;

    // count entries that start with a character
    for (j = 0; j < total; j++) {
      desc = bml (describe_track_value (bmh, i, min_val + j));
      if (desc) {
        vcount++;
        if (g_ascii_isalpha (desc[0])) {
          tcount++;
          GST_DEBUG ("check enum, description[%2d]='%s'", j, desc);
        }
      }
    }

    // some plugins just have text for only val=min/max
    // don't make an enum for those
    if (tcount >= (total >> 1)) {
      // this we can never free :(
      enums = g_new (GEnumValue, vcount + 2);
      // create an enum type
      for (j = k = 0; j < total; j++) {
        desc = bml (describe_track_value (bmh, i, min_val + j));
        //if(desc && g_ascii_isalpha(desc[0])) {
        if (desc) {
          enums[k].value = min_val + j;
          // we have to copy these as buzzmachines can reuse the memory we get from describe()
          enums[k].value_nick = enums[k].value_name =
              g_convert ((gchar *) desc, -1, "UTF-8", "WINDOWS-1252", NULL,
              NULL, NULL);
          k++;
        }
      }
      enums[k].value = no_val;
      enums[k].value_name = "";
      enums[k].value_nick = "";
      k++;
      // terminator
      enums[k].value = 0;
      enums[k].value_name = NULL;
      enums[k].value_nick = NULL;

      enum_type = g_enum_register_static (type_name, enums);
      GST_INFO ("register enum '%s' with %d values", type_name, vcount);
    } else {
      GST_INFO ("not making enum '%s' with %d text of %d total values",
          type_name, tcount, total);
    }
  } else {
    GST_INFO ("existing enum '%s'", type_name);
  }
  g_free (type_name);
  //}
  return enum_type;
}

/*
 * gstbml_class_prepare_properties:
 *
 * override interface properties and register class properties
 */
void
bml (gstbml_class_prepare_properties (GObjectClass * klass,
        GstBMLClass * bml_class))
{
  gpointer bmh = bml_class->bmh;
  GType enum_type = 0;
  gint i, flags, type;
  gint num, min_val, max_val, def_val, no_val;
  gchar *tmp_name, *tmp_desc;
  gchar *name = NULL, *nick = NULL, *desc = NULL;
  gint prop_id = ARG_LAST;

  g_object_class_install_property (klass, ARG_HOST_CALLBACKS,
      g_param_spec_pointer ("host-callbacks",
          "host-callbacks property",
          "Buzz host callback structure",
          G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  if (bml (gstbml_is_polyphonic (bmh))) {
    gboolean res = TRUE;
    gint minv = 0, maxv = 0;

    res &= bml (get_machine_info (bmh, BM_PROP_MIN_TRACKS, (void *) &minv));
    res &= bml (get_machine_info (bmh, BM_PROP_MAX_TRACKS, (void *) &maxv));
    if (res) {
      g_object_class_install_property (klass, prop_id,
          g_param_spec_ulong ("children", "children count property",
              "the number of children this element uses", minv, maxv, minv,
              G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
    } else {
      g_object_class_override_property (klass, prop_id, "children");
    }
    prop_id++;
  }

  /* TODO(ensonic): CMachineDataInput/Output, see FSM/Infector for usage
   * - try using a properties (blob-data, blob-size) to emulate the
   *   CMachineDataInput/Output
   * - machines can read from the stream on Init() and write extradata on Save()
   * - see also: song-io-buzz.c:read_mach_section()
   */

  // register attributes as gobject properties
  if (bml (get_machine_info (bmh, BM_PROP_NUM_ATTRIBUTES, (void *) &num))) {
    GST_INFO ("  machine has %d attributes ", num);
    for (i = 0; i < num; i++, prop_id++) {
      GST_DEBUG ("      attribute=%02i", i);
      bml (get_attribute_info (bmh, i, BM_ATTR_NAME, (void *) &tmp_name));
      bml (get_attribute_info (bmh, i, BM_ATTR_MIN_VALUE, (void *) &min_val));
      bml (get_attribute_info (bmh, i, BM_ATTR_MAX_VALUE, (void *) &max_val));
      bml (get_attribute_info (bmh, i, BM_ATTR_DEF_VALUE, (void *) &def_val));
      gstbml_convert_names (klass, tmp_name, tmp_name, &name, &nick, &desc);
      if (gstbml_register_param (klass, prop_id, PT_ATTR, 0, name, nick, desc,
              0, min_val, max_val, 0, def_val)) {
        bml_class->numattributes++;
      } else {
        GST_WARNING ("registering attribute failed!");
      }
      g_free (name);
      name = NULL;
      g_free (nick);
      nick = NULL;
      g_free (desc);
      desc = NULL;
    }
  }
  GST_INFO ("  %d attribute installed", bml_class->numattributes);

  // register global params as gobject properties
  if (bml (get_machine_info (bmh, BM_PROP_NUM_GLOBAL_PARAMS, (void *) &num))) {
    GST_INFO ("  machine has %d global params ", num);
    bml_class->global_property = g_new (GParamSpec *, num);
    for (i = 0; i < num; i++, prop_id++) {
      GST_DEBUG ("      global_param=%02i", i);
      if (bml (get_global_parameter_info (bmh, i, BM_PARA_TYPE, (void *) &type))
          && bml (get_global_parameter_info (bmh, i, BM_PARA_NAME,
                  (void *) &tmp_name))
          && bml (get_global_parameter_info (bmh, i, BM_PARA_DESCRIPTION,
                  (void *) &tmp_desc))
          && bml (get_global_parameter_info (bmh, i, BM_PARA_FLAGS,
                  (void *) &flags))
          && bml (get_global_parameter_info (bmh, i, BM_PARA_MIN_VALUE,
                  (void *) &min_val))
          && bml (get_global_parameter_info (bmh, i, BM_PARA_MAX_VALUE,
                  (void *) &max_val))
          && bml (get_global_parameter_info (bmh, i, BM_PARA_NO_VALUE,
                  (void *) &no_val))
          && bml (get_global_parameter_info (bmh, i, BM_PARA_DEF_VALUE,
                  (void *) &def_val))
          ) {
        gstbml_convert_names (klass, tmp_name, tmp_desc, &name, &nick, &desc);
        // create an enum on the fly
        if (type == PT_BYTE) {
          if ((enum_type =
                  gst_bml_register_global_enum_type (klass, bmh, i, name,
                      min_val, max_val, no_val, def_val))) {
            type = PT_ENUM;
          }
        }
        if ((bml_class->global_property[bml_class->numglobalparams] =
                gstbml_register_param (klass, prop_id, type, enum_type, name,
                    nick, desc, flags, min_val, max_val, no_val, def_val))) {
          bml_class->numglobalparams++;
        } else {
          GST_WARNING ("registering global_param failed!");
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
  GST_INFO ("  %d global params installed", bml_class->numglobalparams);

  // register track params as gobject properties
  if (bml (get_machine_info (bmh, BM_PROP_NUM_TRACK_PARAMS, (void *) &num))) {
    GST_INFO ("  machine has %d track params ", num);
    bml_class->track_property = g_new (GParamSpec *, num);
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
        gstbml_convert_names (klass, tmp_name, tmp_desc, &name, &nick, &desc);
        // create an enum on the fly
        if (type == PT_BYTE) {
          if ((enum_type =
                  bml (gstbml_register_track_enum_type (klass, bmh, i, name,
                          min_val, max_val, no_val)))) {
            type = PT_ENUM;
          }
        }
        if ((bml_class->track_property[bml_class->numtrackparams] =
                gstbml_register_param (klass, prop_id, type, enum_type, name,
                    nick, desc, flags, min_val, max_val, no_val, def_val))) {
          bml_class->numtrackparams++;
        } else {
          GST_WARNING ("registering global_param (pseudo track param) failed!");
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
  GST_INFO ("  %d track params installed", bml_class->numtrackparams);
}

//-- common element functions

// not wrapper/native specific
/*
 * gst_bml_add_voice:
 *
 * create a new voice and add it to the list of voices
 */
static GstBMLV *
gst_bml_add_voice (GstBML * bml, GType voice_type)
{
  GstBMLV *bmlv;
  gchar *name;

  GST_DEBUG_OBJECT (bml->self,
      "adding a new voice to %p, current nr of voices is %lu", bml->self,
      bml->num_voices);

  bmlv = g_object_new (voice_type, NULL);
  //bmlv->parent=bml;
  bmlv->bm = bml->bm;
  bmlv->voice = bml->num_voices;

  name = g_strdup_printf ("voice%02d", bmlv->voice);
  // set name based on track number
  gst_object_set_name (GST_OBJECT (bmlv), name);
  g_free (name);
  // this refs the bmlv instance
  gst_object_set_parent (GST_OBJECT (bmlv), GST_OBJECT (bml->self));

  bml->voices = g_list_append (bml->voices, bmlv);
  bml->num_voices++;

  GST_DEBUG_OBJECT (bml->self, "added a new voice");
  return bmlv;
}

// not wrapper/native specific
/*
 * gst_bml_del_voice:
 *
 * delete last voice and remove it from the list of voices
 */
static void
gst_bml_del_voice (GstBML * bml, GType voice_type)
{
  GList *node;
  GstObject *obj;

  GST_DEBUG_OBJECT (bml->self,
      "removing last voice to %p, current nr of voices is %lu", bml->self,
      bml->num_voices);

  node = g_list_last (bml->voices);
  obj = node->data;
  GST_DEBUG_OBJECT (bml->self, "  free voice : %p (%d)", obj,
      G_OBJECT (obj)->ref_count);
  gst_object_unparent (obj);
  // no need to unref, the unparent does that
  //g_object_unref(node->data);

  bml->voices = g_list_delete_link (bml->voices, node);
  bml->num_voices--;

  GST_DEBUG_OBJECT (bml->self, "removed last voice");
}

/*
 * gst_bml_init_voices:
 *
 * initialize voice of a new instance
 */
static void
gst_bml_init_voices (GstBML * bml, GstBMLClass * klass)
{
  gpointer bmh = klass->bmh;

  GST_INFO_OBJECT (bml->self, "initializing voices: bml=%p, bml_class=%p", bml,
      klass);

  bml->num_voices = 0;
  bml->voices = NULL;
  if (bml (gstbml_is_polyphonic (bmh))) {
    gint i, min_voices;

    GST_DEBUG_OBJECT (bml->self, "instantiating default voices");

    // add voice instances
    if (bml (get_machine_info (bmh, BM_PROP_MIN_TRACKS, (void *) &min_voices))) {
      GST_DEBUG_OBJECT (bml->self, "adding %d voices", min_voices);
      for (i = 0; i < min_voices; i++) {
        gst_bml_add_voice (bml, klass->voice_type);
      }
    } else {
      GST_WARNING_OBJECT (bml->self, "failed to get min voices");
    }
  }
}

/*
 * gstbml_init:
 *
 * initialize the new instance
 */
void
bml (gstbml_init (GstBML * bml, GstBMLClass * klass, GstElement * element))
{
  GST_DEBUG_OBJECT (element, "init: element=%p, bml=%p, bml_class=%p", element,
      bml, klass);

  bml->self = element;

  bml->bm = bml (new (klass->bmh));
  g_assert (bml->bm);
  bml (init (bml->bm, 0, NULL));

  gst_bml_init_voices (bml, klass);

  // allocate the various arrays
  bml->srcpads = g_new0 (GstPad *, klass->numsrcpads);
  bml->sinkpads = g_new0 (GstPad *, klass->numsinkpads);

  bml->triggers_changed =
      g_new0 (gint, klass->numglobalparams + klass->numtrackparams);

  // nonzero default needed to instantiate() some plugins
  bml->samplerate = GST_AUDIO_DEF_RATE;
  bml->beats_per_minute = 120;
  bml->ticks_per_beat = 4;
  bml->subticks_per_tick = 1;
  gstbml_calculate_buffer_frames (bml);
  if (GST_IS_BASE_SRC (element)) {
    gst_base_src_set_blocksize (GST_BASE_SRC (element),
        gstbml_calculate_buffer_size (bml));
  }
  bml (set_master_info (bml->beats_per_minute, bml->ticks_per_beat,
          bml->samplerate));
  GST_DEBUG_OBJECT (element, "activating %lu voice(s)", bml->num_voices);
  //bml(set_num_tracks(bml,bml->num_voices));
}

void
bml (gstbml_finalize (GstBML * bml))
{
  GST_DEBUG_OBJECT (bml->self, "!!!! bml=%p", bml);

  // free list of voices
  if (bml->voices) {
    g_list_free (bml->voices);
    bml->voices = NULL;
  }

  g_free (bml->srcpads);
  g_free (bml->sinkpads);

  g_free (bml->triggers_changed);

  bml (free (bml->bm));
  bml->bm = NULL;
}

void
bml (gstbml_set_property (GstBML * bml, GstBMLClass * bml_class, guint prop_id,
        const GValue * value, GParamSpec * pspec))
{
  gboolean handled = FALSE;
  gpointer bm = bml->bm;
  guint props_skip = ARG_LAST - 1;

  GST_DEBUG_OBJECT (bml->self, "prop-id %d", prop_id);

  switch (prop_id) {
      // handle properties <ARG_LAST first
    case ARG_HOST_CALLBACKS:
      // supply the callbacks to bml
      GST_DEBUG_OBJECT (bml->self, "passing callbacks to bml");
      bml (set_callbacks (bm, g_value_get_pointer (value)));
      handled = TRUE;
      break;
    default:
      if (bml (gstbml_is_polyphonic (bm))) {
        if (prop_id == (props_skip + 1)) {
          gulong i;
          gulong old_num_voices = bml->num_voices;
          gulong new_num_voices = g_value_get_ulong (value);
          GST_DEBUG_OBJECT (bml->self,
              "change number of voices from %lu to %lu", old_num_voices,
              new_num_voices);
          // add or free voices
          if (old_num_voices < new_num_voices) {
            for (i = old_num_voices; i < new_num_voices; i++) {
              // this increments bml->num_voices
              gst_bml_add_voice (bml, bml_class->voice_type);
            }
          } else {
            for (i = new_num_voices; i < old_num_voices; i++) {
              // this decrements bml->num_voices
              gst_bml_del_voice (bml, bml_class->voice_type);
            }
          }
          if (old_num_voices != new_num_voices) {
            bml (set_num_tracks (bm, bml->num_voices));
          }
          handled = TRUE;
        }
        props_skip++;
      }
      break;
  }
  prop_id -= props_skip;

  // pass remaining props to wrapped plugin
  if (!handled) {
    gint val;
    gint type;
    // DEBUG
    //gchar *valstr;
    // DEBUG

    g_assert (prop_id > 0);

    type =
        GPOINTER_TO_INT (g_param_spec_get_qdata (pspec,
            gst_bml_property_meta_quark_type));

    prop_id--;
    GST_LOG_OBJECT (bml->self, "id: %d, attr: %d, global: %d, voice: %d",
        prop_id, bml_class->numattributes, bml_class->numglobalparams,
        bml_class->numtrackparams);
    // is it an attribute ?
    if (prop_id < bml_class->numattributes) {
      bml (set_attribute_value (bm, prop_id, g_value_get_int (value)));
      //GST_DEBUG("set attribute %d to %d", prop_id, g_value_get_int(value));
    } else {
      prop_id -= bml_class->numattributes;

      if (!(pspec->flags & G_PARAM_READABLE)
          && !g_param_value_defaults (pspec, (GValue *) value)) {
        // flag triggered triggers
        g_atomic_int_set (&bml->triggers_changed[prop_id], 1);
      }
      // is it a global param
      if (prop_id < bml_class->numglobalparams) {
        val = gstbml_get_param (type, value);
        bml (set_global_parameter_value (bm, prop_id, val));
        // DEBUG
        //valstr=g_strdup_value_contents(value);
        //GST_DEBUG("set global param %d to %s (%p)", prop_id, valstr,addr);
        //g_free(valstr);
        // DEBUG
      } else {
        prop_id -= bml_class->numglobalparams;
        // is it a voice00 param
        if (prop_id < bml_class->numtrackparams) {
          val = gstbml_get_param (type, value);
          bml (set_track_parameter_value (bm, 0, prop_id, val));
          // DEBUG
          //valstr=g_strdup_value_contents(value);
          //GST_DEBUG("set track param %d:0 to %s (%p)", prop_id, valstr,addr);
          //g_free(valstr);
        }
      }
    }
  }
}

void
bml (gstbml_get_property (GstBML * bml, GstBMLClass * bml_class, guint prop_id,
        GValue * value, GParamSpec * pspec))
{
  gboolean handled = FALSE;
  gpointer bm = bml->bm;
  guint props_skip = ARG_LAST - 1;

  GST_DEBUG_OBJECT (bml->self, "prop-id %d", prop_id);

  switch (prop_id) {
      // handle properties <ARG_LAST first
      /*case ARG_HOST_CALLBACKS:
         GST_WARNING ("callbacks property is write only");
         handled=TRUE;
         break; */
    default:
      if (bml (gstbml_is_polyphonic (bm))) {
        if (prop_id == (props_skip + 1)) {
          g_value_set_ulong (value, bml->num_voices);
          GST_DEBUG_OBJECT (bml->self, "requested number of voices = %lu",
              bml->num_voices);
          handled = TRUE;
        }
        props_skip++;
      }
      break;
  }
  prop_id -= props_skip;

  // pass remaining props to wrapped plugin
  if (!handled) {
    gint val;
    gint type;
    // DEBUG
    //gchar *valstr;
    // DEBUG

    g_assert (prop_id > 0);

    type =
        GPOINTER_TO_INT (g_param_spec_get_qdata (pspec,
            gst_bml_property_meta_quark_type));
    prop_id--;
    GST_DEBUG_OBJECT (bml->self, "id: %d, attr: %d, global: %d, voice: %d",
        prop_id, bml_class->numattributes, bml_class->numglobalparams,
        bml_class->numtrackparams);
    // is it an attribute ?
    if (prop_id < bml_class->numattributes) {
      g_value_set_int (value, bml (get_attribute_value (bm, prop_id)));
      //GST_DEBUG("got attribute as %d", g_value_get_int(value));
    } else {
      prop_id -= bml_class->numattributes;
      // is it a global param
      if (prop_id < bml_class->numglobalparams) {
        val = bml (get_global_parameter_value (bm, prop_id));
        gstbml_set_param (type, val, value);
        // DEBUG
        //valstr=g_strdup_value_contents(value);
        //GST_DEBUG ("got global param as %s (%p)", valstr,addr);
        //g_free(valstr);
        // DEBUG
      } else {
        prop_id -= bml_class->numglobalparams;
        // is it a voice00 param
        if (prop_id < bml_class->numtrackparams) {
          val = bml (get_track_parameter_value (bm, 0, prop_id));
          gstbml_set_param (type, val, value);
          // DEBUG
          //valstr=g_strdup_value_contents(value);
          //GST_DEBUG ("got track param as %s (%p)", valstr,addr);
          //g_free(valstr);
          // DEBUG
        }
      }
    }
  }
}


/*
 * gstbml_sync_values:
 *
 * updates the global and voice params
 */
void
bml (gstbml_sync_values (GstBML * bml, GstBMLClass * bml_class,
        GstClockTime ts))
{
  GList *node;
  gulong i;
  GstBMLV *bmlv;
  GstBMLVClass *bmlv_class;
  gint *volatile tc = bml->triggers_changed;
  //gboolean res;

  GST_DEBUG_OBJECT (bml->self, "  sync_values(%p), voices=%lu,%p", bml->self,
      bml->num_voices, bml->voices);

  for (i = 0; i < bml_class->numglobalparams + bml_class->numtrackparams; i++) {
    (void) g_atomic_int_compare_and_exchange (&tc[i], 1, 2);
  }
  /*res= */ gst_object_sync_values (GST_OBJECT (bml->self), ts);
  for (i = 0; i < bml_class->numglobalparams + bml_class->numtrackparams; i++) {
    (void) g_atomic_int_compare_and_exchange (&tc[i], 1, 0);
  }
  //if(G_UNLIKELY(!res)) { GST_WARNING("global sync failed"); }
  for (node = bml->voices; node; node = g_list_next (node)) {
    bmlv = node->data;
    bmlv_class = GST_BMLV_GET_CLASS (bmlv);
    tc = bmlv->triggers_changed;

    for (i = 0; i < bmlv_class->numtrackparams; i++) {
      (void) g_atomic_int_compare_and_exchange (&tc[i], 1, 2);
    }
    /*res= */ gst_object_sync_values (GST_OBJECT (bmlv), ts);
    //if(G_UNLIKELY(!res)) { GST_WARNING("voice sync failed"); }
    for (i = 0; i < bmlv_class->numtrackparams; i++) {
      (void) g_atomic_int_compare_and_exchange (&tc[i], 1, 0);
    }
  }
}

/*
 * gstbml_reset_triggers:
 *
 * set trigger parameter back to no-value
 */
void
bml (gstbml_reset_triggers (GstBML * bml, GstBMLClass * bml_class))
{
  GList *node;
  gpointer bm = bml->bm;
  GstBMLV *bmlv;
  GstBMLVClass *bmlv_class;
  GParamSpec *pspec;
  gulong i, v;
  gint val;

  for (i = 0; i < bml_class->numglobalparams; i++) {
    if (g_atomic_int_compare_and_exchange (&bml->triggers_changed[i], 2, 0)) {
      pspec = bml_class->global_property[i];
      val =
          GPOINTER_TO_INT (g_param_spec_get_qdata (pspec,
              gstbt_property_meta_quark_no_val));
      bml (set_global_parameter_value (bm, i, val));
    }
  }
  for (i = 0; i < bml_class->numtrackparams; i++) {
    if (g_atomic_int_compare_and_exchange (&bml->triggers_changed[bml_class->
                numglobalparams + i], 2, 0)) {
      pspec = bml_class->track_property[i];
      val =
          GPOINTER_TO_INT (g_param_spec_get_qdata (pspec,
              gstbt_property_meta_quark_no_val));
      bml (set_track_parameter_value (bm, 0, i, val));
    }
  }
  for (v = 0, node = bml->voices; node; node = g_list_next (node), v++) {
    bmlv = node->data;
    bmlv_class = GST_BMLV_GET_CLASS (bmlv);
    for (i = 0; i < bmlv_class->numtrackparams; i++) {
      if (g_atomic_int_compare_and_exchange (&bmlv->triggers_changed[i], 2, 0)) {
        pspec = bmlv_class->track_property[i];
        val =
            GPOINTER_TO_INT (g_param_spec_get_qdata (pspec,
                gstbt_property_meta_quark_no_val));
        bml (set_track_parameter_value (bm, v, i, val));
      }
    }
  }
}
