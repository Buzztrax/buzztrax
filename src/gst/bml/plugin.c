/* GStreamer
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

#include "plugin.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#if HAVE_BMLW
#include <sys/wait.h>
#endif

#define GST_CAT_DEFAULT bml_debug
GST_DEBUG_CATEGORY (GST_CAT_DEFAULT);

GstStructure *bml_meta_all;

GstPlugin *bml_plugin;
GHashTable *bml_category_by_machine_name;

GQuark gst_bml_property_meta_quark_type;

// we need to do this as this is compiled globally without bml/BML macros defined
#if HAVE_BMLW
extern gboolean bmlw_gstbml_inspect (gchar * file_name);
extern gboolean bmlw_gstbml_register_element (GstPlugin * plugin,
    GstStructure * bml_meta);
#endif
extern gboolean bmln_gstbml_inspect (gchar * file_name);
extern gboolean bmln_gstbml_register_element (GstPlugin * plugin,
    GstStructure * bml_meta);


typedef int (*bsearchcomparefunc) (const void *, const void *);


#define LINE_LEN 500
#define CAT_LEN 1000
static gboolean
read_index (const gchar * dir_name)
{
  gchar *file_name;
  FILE *file;
  gboolean res = FALSE;

  /* we want a GHashTable bml_category_by_machine_name
   * with the plugin-name (BM_PROP_SHORT_NAME) as a key
   * and the categories as values.
   *
   * this can be used in utils.c: gstbml_class_set_details()
   *
   * maybe we can also filter some plugins based on categories
   */

  file_name = g_build_filename (dir_name, "index.txt", NULL);
  if ((file = fopen (file_name, "rt"))) {
    GST_INFO ("found buzz machine index at \"%s\"", file_name);
    gchar line[LINE_LEN + 1], *entry;
    gchar categories[CAT_LEN + 1] = "";
    gint cat_pos = 0, i, len;

    /* the format
     * there are:
     *   comments?  : ","
     *   level-lines: "1,-----"
     *   categories : "/Peer Control"
     *   end-of-cat.: "/.."
     *   separators : "-----" or "--- Dx ---"
     *   machines   : "Arguelles Pro3"
     *   mach.+alias: "Argüelles Pro2, Arguelles Pro2"
     */

    while (!feof (file)) {
      if (fgets (line, LINE_LEN, file)) {
        // strip leading and trailing spaces and convert
        entry =
            g_convert (g_strstrip (line), -1, "UTF-8", "WINDOWS-1252", NULL,
            NULL, NULL);
        if (entry[0] == '/') {
          if (entry[1] == '.' && entry[2] == '.') {
            // pop stack
            if (cat_pos > 0) {
              while (cat_pos > 0 && categories[cat_pos] != '/')
                cat_pos--;
              categories[cat_pos] = '\0';
            }
            GST_DEBUG ("- %4d %s", cat_pos, categories);
          } else {
            // push stack
            len = strlen (entry);
            if ((cat_pos + len) < CAT_LEN) {
              categories[cat_pos++] = '/';
              for (i = 1; i < len; i++) {
                categories[cat_pos++] = (entry[i] != '/') ? entry[i] : '+';
              }
              categories[cat_pos] = '\0';
            }
            GST_DEBUG ("+ %4d %s", cat_pos, categories);
          }
        } else {
          if (entry[0] == '-' || entry[0] == ',' || g_ascii_isdigit (entry[0])) {
            // skip for now
          } else if (g_ascii_isalpha (entry[0])) {
            // machines
            // check for "'" and cut the alias
            gchar **names = g_strsplit (entry, ",", -1);
            gchar *cat = g_strdup (categories), *beg, *end;
            gint a;

            // we need to filter 'Generators,Effects,Gear' from the categories
            if ((beg = strstr (cat, "/Generator"))) {
              end = &beg[strlen ("/Generator")];
              memmove (beg, end, strlen (end) + 1);
            }
            if ((beg = strstr (cat, "/Effect"))) {
              end = &beg[strlen ("/Effect")];
              memmove (beg, end, strlen (end) + 1);
            }
            if ((beg = strstr (cat, "/Gear"))) {
              end = &beg[strlen ("/Gear")];
              memmove (beg, end, strlen (end) + 1);
            }

            if (*cat) {
              for (a = 0; a < g_strv_length (names); a++) {
                if (names[a] && *names[a]) {
                  GST_DEBUG ("  %s -> %s", names[a], categories);
                  g_hash_table_insert (bml_category_by_machine_name,
                      g_strdup (names[a]), g_strdup (cat));
                }
              }
            }
            g_free (cat);
            g_strfreev (names);
          }
        }
        g_free (entry);
      }
    }

    res = TRUE;
    fclose (file);
  }
  g_free (file_name);
  return (res);
}

static int
blacklist_compare (const void *node1, const void *node2)
{
  //GST_DEBUG("comparing '%s' '%s'",*(gchar**)node1,*(gchar**)node2);
  return (strcasecmp (*(gchar **) node1, *(gchar **) node2));
}

static const gchar *
get_bml_path (void)
{
#if HAVE_BMLW
  return (LIBDIR G_DIR_SEPARATOR_S "Gear:" LIBDIR G_DIR_SEPARATOR_S "Gear"
      G_DIR_SEPARATOR_S "Generators:" LIBDIR G_DIR_SEPARATOR_S "Gear"
      G_DIR_SEPARATOR_S "Effects");
#else
  return (LIBDIR G_DIR_SEPARATOR_S "Gear");
#endif
}

#if HAVE_BMLW
#if 0
static struct sigaction oldaction;
static gboolean fault_handler_active = FALSE;

static void
fault_handler_restore (void)
{
  if (!fault_handler_active)
    return;

  fault_handler_active = FALSE;

  sigaction (SIGSEGV, &oldaction, NULL);
}

static void
fault_handler_sighandler (int signum)
{
  /* We need to restore the fault handler or we'll keep getting it */
  fault_handler_restore ();

  switch (signum) {
    case SIGSEGV:
      g_print ("Caught a segmentation fault while loading plugin file:\n");
      exit (-1);
      break;
    default:
      g_print ("Caught unhandled signal on plugin loading\n");
      break;
  }
}

static void
fault_handler_setup (void)
{
  struct sigaction action;

  if (fault_handler_active)
    return;

  fault_handler_active = TRUE;

  memset (&action, 0, sizeof (action));
  action.sa_handler = fault_handler_sighandler;
  sigaction (SIGSEGV, &action, &oldaction);
}
#endif
#endif

/*
 * dir_scan:
 *
 * Search the given directory for plugins. Supress some based on a built-in
 * blacklist.
 */
static gboolean
dir_scan (const gchar * dir_name)
{
  GDir *dir;
  gchar *file_name, *ext, *conv_entry_name = NULL, *cur_entry_name;
  const gchar *entry_name;
  gboolean res = FALSE;

  /* @TODO: find a way to sync this with bml's testmachine report
   * also turning this into an include would ease, e.g. sorting it
   */
  const gchar *blacklist[] = {
    "2NDPLOOPJUMPHACK.DLL",
    "2NDPROCESS NUMBIRD 1.4.DLL",
    "7900S PEARL DRUM.DLL",
    "ANEURYSM DISTGARB.DLL",
    "ARGUELLES FX2.DLL",
    /*"ARGUELLES PRO3.DLL",
     * is disguised as an effect (stereo)
     * I patched @2d28 : 02 00 00 00 0c 00 00 00 -> 01 00 ...
     * segfault still :/
     * bml ../gstbmlsrc.c:512:gst_bml_src_create_stereo:   calling work_m2s(5292)
     */
    "ARGUELLES ROVOX.DLL",
    "AUTOMATON COMPRESSOR MKII.DLL",
    /*"AUTOMATON PARAMETRIC EQ.DLL",
       - crashes on CMachineInterface::SetNumTracks(1)
     */
    "BUZZINAMOVIE.DLL",
    "CHIMP REPLAY.DLL",
    "CYANPHASE AUXRETURN.DLL",
    /* this is part of normal buzz installs these days, even its not a plugin */
    "CYANPHASE BUZZ OVERLOADER.DLL",
    "CYANPHASE DMO EFFECT ADAPTER.DLL",
    "CYANPHASE DX EFFECT ADAPTER.DLL",
    "CYANPHASE SEA CUCUMBER.DLL",
    "CYANPHASE SONGINFO.DLL",
    "CYANPHASE UNNATIVE EFFECTS.DLL",
    "CYANPHASE VIDIST 01.DLL",
    "DAVE'S SMOOTHERDRIVE.DLL",
    "DEX CROSSFADE.DLL",
    "DEX EFX.DLL",
    "DEX RINGMOD.DLL",
    "DT_BLOCKFX (STEREO).DLL",
    "DT_BLOCKFX.DLL",
    "EAX2OUTPUT.DLL",
    "FIRESLEDGE ANTIOPE-1.DLL",
    "FREQUENCY UNKNOWN FREQ IN.DLL",
    "FREQUENCY UNKNOWN FREQ OUT.DLL",
    "FUZZPILZ POTTWAL.DLL",
    "FUZZPILZ RO-BOT.DLL",
    "FUZZPILZ UNWIELDYDELAY3.DLL",
    "GAZBABY'S PROLOGIC ENCODER.DLL",
    "GEONIK'S DX INPUT.DLL",
    "GEONIK'S PRIMIFUN.DLL",
    "GEONIK'S VISUALIZATION.DLL",
    "HD AUXREPEATER.DLL",
    "HD F-FLANGER.DLL",
    "HD MOD-X.DLL",
    "J.R. BP V1 FILTER.DLL",
    "J.R. BP V2 FILTER.DLL",
    "J.R. HP FILTER.DLL",
    "J.R. LP FILTER.DLL",
    "J.R. NOTCH FILTER.DLL",
    "JESKOLA AUXSEND.DLL",
    "JESKOLA EQ-3 XP.DLL",
    "JESKOLA ES-9.DLL",
    "JESKOLA FREEVERB.DLL",
    "JESKOLA MULTIPLIER.DLL",
    "JESKOLA O1.DLL",
    /*"JESKOLA REVERB 2.DLL", */
    /*"JESKOLA REVERB.DLL", */
    /*"JESKOLA STEREO REVERB.DLL", */
    "JESKOLA TRACKER.DLL",
    "JESKOLA WAVE SHAPER.DLL",
    "JESKOLA WAVEIN INTERFACE.DLL",
    "JOASMURF VUMETER.DLL",
    "JOYPLUG 1.DLL",
    "LD AUXSEND.DLL",
    "LD FMFILTER.DLL",
    "LD GATE.DLL",
    "LD MIXER.DLL",
    "LD VOCODER XP.DLL",
    "LD VOCODER.DLL",
    "LIVE.DLL",
    "LOST_BIT IMOD.DLL",
    "LOST_BIT IPAN.DLL",
    "M4WII.DLL",
    "MARC MP3LOADER.DLL",
    "MIMO'S MIDIGEN.DLL",
    "MIMO'S MIDIOUT.DLL",
    "MIMO'S MIXO.DLL",
    "NINEREEDS 2P FILTER.DLL",
    "NINEREEDS DISCRETIZE.DLL",
    "NINEREEDS DISCRITIZE.DLL",
    "NINEREEDS FADE.DLL",
    "NINEREEDS LFO.DLL",
    "NINEREEDS LFO FADE.DLL",
    "NINEREEDS NRS04.DLL",
    "NINEREEDS NRS05.DLL",
    "P. DOOM'S HACK JUMP.DLL",
    "P. DOOM'S HACK MSYNC.DLL",
    "PITCHWIZARD.DLL",
    "POLAC MIDI IN.DLL",
    "POLAC VST 1.1.DLL",
    "PSI KRAFT.DLL",
    "PSI KRAFT2.DLL",
    "REBIRTH MIDI 2.DLL",
    "REBIRTH MIDI.DLL",
    "REPEATER.DLL",
    "RNZNANF VST EFFECT ADAPTER.DLL",
    "RNZNANF VST INSTRUMENT ADAPTER.DLL",
    "RNZNANFNCNR VST EFFECT ADAPTER.DLL",
    "RNZNANFNCNR VST INSTRUMENT ADAPTER.DLL",
    "RNZNANFNR VST INSTRUMENT ADAPTER.DLL",
    "ROUT VST PLUGIN LOADER.DLL",
    "SHAMAN CHORUS.DLL",
    "STATIC DUAFILT II.DLL",
    // *** glibc detected *** /home/ensonic/projects/buzztrax/bml/src/.libs/lt-bmltest_info: free(): invalid next size (normal): 0x0805cc18 ***
    "SYNTHROM SINUS 2.DLL",
    "TRACK ORGANIZER.DLL",
    "VGRAPHITY.DLL",
    "VMIDIOUT.DLL",
    "WAVEEDIT.DLL",
    "WHITENOISE AUXRETURN.DLL",
    "WHITENOISE'S DRUMMER 2.DLL",
    "XMIX.DLL",
    "YNZN'S AMPLITUDE MODULATOR.DLL",
    "YNZN'S CHIRPFILTER.DLL",
    "YNZN'S MULTIPLEXER.DLL",
    "YNZN'S REMOTE COMPRESSOR.DLL",
    "YNZN'S REMOTE GATE.DLL",
    "YNZN'S VOCODER.DLL",
    /* Zephod machines seem to need some functions from oleauth.dll
     * other die right after GetInfo() :/
     */
    "ZEPHOD BLUE FILTER.DLL",
    "ZEPHOD CSYNTH.DLL",
    "ZEPHOD GOLDFISH.DLL",
    "ZEPHOD GREEN FILTER.DLL",
    "ZEPHOD MIDITRACKER.DLL",
    "ZEPHOD PLATINUMFISH.DLL",
    "ZEPHOD_RESAW.DLL",
    "ZNT WAVEEDIT.DLL",
    "ZU ?TAPS.DLL",
    "ZU MORPHIN FINAL DOSE.DLL",
    "ZU MORPHIN.DLL",
    "ZU PARAMETRIC EQ.DLL",
    "ZU SLICER.DLL",
    "ZU µTAPS.DLL",
    "ZU �TAPS.DLL",
    "Zu æTaps.dll"
  };

  GST_INFO ("scanning directory \"%s\"", dir_name);

  dir = g_dir_open (dir_name, 0, NULL);
  if (!dir)
    return (res);

  while ((entry_name = g_dir_read_name (dir))) {
    cur_entry_name = (gchar *) entry_name;
    if (!g_utf8_validate (entry_name, -1, NULL)) {
      GST_WARNING ("file %s is not a valid file-name", entry_name);
      if ((conv_entry_name =
              g_convert (entry_name, -1, "UTF-8", "WINDOWS-1252", NULL, NULL,
                  NULL))) {
        cur_entry_name = conv_entry_name;
      } else {
        GST_WARNING ("can't convert encoding for %s", entry_name);
        continue;
      }
    }

    ext = strrchr (entry_name, '.');
    if (ext && (!strcasecmp (ext, ".dll") || !strcmp (ext, ".so"))) {
      /* test against blacklist */
      if (!bsearch (&cur_entry_name, blacklist, G_N_ELEMENTS (blacklist),
              sizeof (gchar *), blacklist_compare)) {
        file_name = g_build_filename (dir_name, cur_entry_name, NULL);
        GST_INFO ("trying plugin '%s','%s'", cur_entry_name, file_name);
        if (!strcasecmp (ext, ".dll")) {
#if HAVE_BMLW
#if 0
          pid_t pid;
          int status;

          /* Do a fork and try to open() the plugin there to avoid crashing on
           * bad ones. This is not perfect, plugins can still crash later on.
           * Also it seems that this causes troubles in the wine emulation
           * state as some plugins that otherwise work, now fail later on :/
           */
          fault_handler_setup ();
          if ((pid = fork ()) == 0) {
            // child
            gpointer bmh;
            if ((bmh = bmlw_open (file_name))) {
              bmlw_close (bmh);
              exit (0);
            }
            exit (1);
          }
          fault_handler_restore ();
          waitpid (pid, &status, 0);
          if (WIFEXITED (status)) {
            if (WEXITSTATUS (status) == 0) {
              GST_WARNING ("loading %s worked okay", file_name);
#endif
              res = bmlw_gstbml_inspect (file_name);
#if 0
            } else {
              GST_WARNING ("try loading %s failed with exit code %d", file_name,
                  WEXITSTATUS (status));
            }
          } else if (WIFSIGNALED (status)) {
            GST_WARNING ("try loading %s failed with signal %d", file_name,
                WTERMSIG (status));
          }
#endif
#else
          GST_WARNING ("no dll emulation on non x86 platforms");
#endif
        } else {
          res = bmln_gstbml_inspect (file_name);
        }
        g_free (file_name);
      } else {
        GST_WARNING ("machine %s is black-listed", entry_name);
      }
    }
    g_free (conv_entry_name);
    conv_entry_name = NULL;
  }
  g_dir_close (dir);

  GST_INFO ("after scanning dir \"%s\", res=%d", dir_name, res);
  return (res);
}

/*
 * bml_scan:
 *
 * Search through the $(BML_PATH) (or a default path) for any buzz plugins. Each
 * plugin library is tested using dlopen() and dlsym(,"bml_descriptor").
 * After loading each library, the callback function is called to process it.
 * This function leaves items passed to the callback function open.
 */
static gboolean
bml_scan (void)
{
  const gchar *bml_path;
  gchar **paths;
  gint i, path_entries;
  gboolean res = FALSE;

  bml_path = g_getenv ("BML_PATH");

  if (!bml_path || !*bml_path) {
    bml_path = get_bml_path ();
    GST_WARNING
        ("You do not have a BML_PATH environment variable set, using default: '%s'",
        bml_path);
  }

  paths = g_strsplit (bml_path, G_SEARCHPATH_SEPARATOR_S, 0);
  path_entries = g_strv_length (paths);
  GST_INFO ("%d dirs in search paths \"%s\"", path_entries, bml_path);

  bml_category_by_machine_name =
      g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

  // check of index.txt in any of the paths
  for (i = 0; i < path_entries; i++) {
    if (read_index (paths[i]))
      break;
  }

  for (i = 0; i < path_entries; i++) {
    res |= dir_scan (paths[i]);
  }
  g_strfreev (paths);

  g_hash_table_destroy (bml_category_by_machine_name);

  GST_INFO ("after scanning path \"%s\", res=%d", bml_path, res);
  return (res);
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  gboolean res = FALSE;
  gint n = 0;

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "bml",
      GST_DEBUG_FG_GREEN | GST_DEBUG_BG_BLACK | GST_DEBUG_BOLD, "BML");

  GST_INFO
      ("lets go ===========================================================");

  gst_plugin_add_dependency_simple (plugin,
      "BML_PATH",
      get_bml_path (),
      "so,dll",
      GST_PLUGIN_DEPENDENCY_FLAG_PATHS_ARE_DEFAULT_ONLY |
      GST_PLUGIN_DEPENDENCY_FLAG_FILE_NAME_IS_SUFFIX);

  // init bml library
  if (!bml_setup ()) {
    GST_WARNING ("failed to init bml library");
    return (FALSE);
  }
  // init global data
  bml_plugin = plugin;

  // TODO(ensonic): this is a hack
#if HAVE_BMLW
  bmlw_set_master_info (120, 4, 44100);
#endif
  bmln_set_master_info (120, 4, 44100);

  gst_bml_property_meta_quark_type =
      g_quark_from_string ("GstBMLPropertyMeta::type");

  GST_INFO ("scan for plugins");
  bml_meta_all = (GstStructure *) gst_plugin_get_cache_data (plugin);
  if (bml_meta_all) {
    n = gst_structure_n_fields (bml_meta_all);
  }
  GST_INFO ("%d entries in cache", n);
  if (!n) {
    bml_meta_all = gst_structure_new_empty ("bml");
    res = bml_scan ();
    if (res) {
      n = gst_structure_n_fields (bml_meta_all);
      GST_INFO ("%d entries after scanning", n);
      gst_plugin_set_cache_data (plugin, bml_meta_all);
    }
  } else {
    res = TRUE;
  }

  if (n) {
    gint i;
    const gchar *name;
    const GValue *value;
    GQuark bmln_type = g_quark_from_static_string ("bmln");

    GST_INFO ("register types");

    for (i = 0; i < n; i++) {
      name = gst_structure_nth_field_name (bml_meta_all, i);
      value = gst_structure_get_value (bml_meta_all, name);
      if (G_VALUE_TYPE (value) == GST_TYPE_STRUCTURE) {
        GstStructure *bml_meta = g_value_get_boxed (value);
        GQuark bml_type = gst_structure_get_name_id (bml_meta);

        if (bml_type == bmln_type) {
          res &= bmln_gstbml_register_element (plugin, bml_meta);
        }
#if HAVE_BMLW
        else {
          res &= bmlw_gstbml_register_element (plugin, bml_meta);
        }
#endif
      }
    }
  }

  if (!res) {
    GST_WARNING ("no buzzmachine plugins found, check BML_PATH");
  }

  /* we don't want to fail, even if there are no elements registered */
  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    bml,
    "buzz machine loader - all buzz machines",
    plugin_init, VERSION, "LGPL", GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
