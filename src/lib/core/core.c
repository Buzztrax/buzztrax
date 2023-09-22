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
 * SECTION:libbtcore
 * @short_description: core library of the buzztrax application framework
 *
 * The library offers base objects such as #BtApplication and #BtSong.
 */

#define BT_CORE
#define BT_CORE_C

#include "core_private.h"
#include <glib/gprintf.h>
#include <gst/audio/audio.h>
#include <gst/pbutils/pbutils.h>
#include <libxml/tree.h>

#ifdef HAVE_SCHED_SETSCHEDULER
#include <sched.h>
#ifdef HAVE_MLOCKALL
#include <sys/mman.h>
#endif
#endif

#ifdef USE_X86_SSE
#ifdef HAVE_XMMINTRIN_H
#include <xmmintrin.h>
#endif
#endif

/**
 * bt_major_version:
 *
 * buzztrax version stamp, major part
 */
const guint bt_major_version = BT_MAJOR_VERSION;
/**
 * bt_minor_version:
 *
 * buzztrax version stamp, minor part
 */
const guint bt_minor_version = BT_MINOR_VERSION;
/**
 * bt_micro_version:
 *
 * buzztrax version stamp, micro part
 */
const guint bt_micro_version = BT_MICRO_VERSION;

GST_DEBUG_CATEGORY (GST_CAT_DEFAULT);
GST_DEBUG_CATEGORY_STATIC (libxml_category);

static gboolean arg_version = FALSE;
static gchar **arg_experiments = NULL;

GstCaps *bt_default_caps = NULL;

//-- helper methods

/* we have no fail cases yet, but maybe in the future */
static gboolean
bt_init_pre (void)
{
#ifdef ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

  return TRUE;
}

/* xml error log handler */
static void
bt_libxml_error_func (void *user_data, xmlErrorPtr error)
{
  GstDebugLevel level;
  switch (error->level) {
    case XML_ERR_FATAL:
    case XML_ERR_ERROR:
      level = GST_LEVEL_ERROR;
      break;
    case XML_ERR_WARNING:
      level = GST_LEVEL_WARNING;
      break;
    case XML_ERR_NONE:
    default:
      level = GST_LEVEL_INFO;
      break;
  }

  gst_debug_log (libxml_category, level, error->file, "", error->line, NULL,
      "%d:%d:%s", error->domain, error->code, error->message);
}

static gboolean
bt_init_post (void)
{
  gst_pb_utils_init ();

  if (arg_version) {
    g_printf ("libbuzztrax-core-%d.%d.%d from " PACKAGE_STRING "\n",
        BT_MAJOR_VERSION, BT_MINOR_VERSION, BT_MICRO_VERSION);
  }

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "bt-core", 0,
      "music production environment / core library");
  GST_DEBUG_CATEGORY_INIT (libxml_category, "libxml", 0, "xml library");

  if (arg_experiments) {
    bt_experiments_init (arg_experiments);
    g_strfreev (arg_experiments);
    arg_experiments = NULL;
  }

  // dbeswick: During tests, a single process destroys and re-initializes an application
  // repeatedly. When the plugin was being repeatedly re-registered, I found that
  // segfaults were occurring as factories returned from elements seemed to have
  // been corrupted and weren't recognized as GObjects. It's probably not good to
  // register static GST plugins multiple times over the life of a single process.
  static gboolean plugin_inited = FALSE;
  if (plugin_inited) {
    GST_INFO ("bt-sink-bin plugin already registered, so doing nothing");
  } else {
    GST_INFO ("Registering bt-sink-bin plugin");
    extern gboolean bt_sink_bin_plugin_init (GstPlugin * const plugin);
    gst_plugin_register_static (GST_VERSION_MAJOR,
        GST_VERSION_MINOR,
        "bt-sink-bin",
        "buzztrax sink bin - encapsulates play and record functionality",
        bt_sink_bin_plugin_init,
        VERSION, "LGPL", PACKAGE, PACKAGE_NAME, "http://www.buzztrax.org");

    bt_default_caps = gst_caps_new_simple ("audio/x-raw",
        "format", G_TYPE_STRING, GST_AUDIO_NE (F32),
        "layout", G_TYPE_STRING, "interleaved",
        "rate", GST_TYPE_INT_RANGE, 1, G_MAXINT,
        "channels", GST_TYPE_INT_RANGE, 1, 2, NULL);

    plugin_inited = TRUE;
  }
  GST_DEBUG ("init xml");
  //-- initialize libxml
  // set own error handler
  xmlSetStructuredErrorFunc (NULL, &bt_libxml_error_func);
  // initialize the xml parser
  xmlInitParser ();
  // xmlInitParser does that for us
  //xmlXPathInit();
  // we don't use entities
  xmlSubstituteEntitiesDefault (0);
  xmlLoadExtDtdDefaultValue = FALSE;    // do not always load DTD default values
  xmlDoValidityCheckingDefaultValue = FALSE;    // do not validate files

#if 0
// I just got
// switching scheduler failed: operation is not permitted
// see /etc/security/limits.conf
#ifdef HAVE_SCHED_SETSCHEDULER
  // @idea; only do this in non-debug builds
  //http://www.gnu.org/software/libc/manual/html_node/Basic-Scheduling-Functions.html
  {
    struct sched_param p = { 0, };

    p.sched_priority = sched_get_priority_min (SCHED_RR);
    //p.sched_priority=sched_get_priority_max(SCHED_RR);
    if (sched_setscheduler (0, SCHED_RR, &p) < 0) {
      GST_WARNING ("switching scheduler failed: %s", g_strerror (errno));
    } else {
      GST_INFO ("switched scheduler");
#if HAVE_MLOCKALL
      if (mlockall (MCL_CURRENT) < 0)
        GST_WARNING ("locking memory pages failed: %s", g_strerror (errno));
#endif
    }
  }
#endif
#endif

#if USE_X86_SSE
  // TODO(ensonic): we need to probe the CPU capabilities
  // see http://www.mail-archive.com/linux-audio-dev@music.columbia.edu/msg19520.html
  //   [linux-audio-dev] Channels and best practice
  // _MM_FLUSH_ZERO_ON = FZ
  // TODO(ensonic): wikipedia says we must do this for each thread:
  // https://en.wikipedia.org/wiki/Denormal_number#Disabling_denormal_floats_at_the_code_level
  _mm_setcsr (_mm_getcsr () | 0x8040);  // set DAZ and FZ bits
#endif

  return TRUE;
}

//-- core initialisation

/**
 * bt_init_get_option_group: (skip)
 *
 * Returns a #GOptionGroup with libbtcore's argument specifications. The group
 * is set up to use standard GOption callbacks, so when using this group in
 * combination with GOption parsing methods, all argument parsing and
 * initialization is automated.
 *
 * This function is useful if you want to integrate libbtcore with other
 * libraries that use GOption (see g_option_context_add_group() ).
 *
 * Returns: (transfer full): a pointer to a GOption group. Should be
 * dereferenced after use.
 */
GOptionGroup *
bt_init_get_option_group (void)
{
  GOptionGroup *group;
  static GOptionEntry options[] = {
    {"bt-version", 0, 0, G_OPTION_ARG_NONE, NULL,
        N_("Print the buzztrax core version"), NULL},
    {"bt-core-experiment", 0, 0,
        G_OPTION_ARG_STRING_ARRAY, NULL, N_("Experiments"), "{audiomixer}"},
    {NULL}
  };
  options[0].arg_data = &arg_version;
  options[1].arg_data = &arg_experiments;

  group =
      g_option_group_new ("bt-core", _("Buzztrax core options"),
      _("Show buzztrax core options"), NULL, NULL);
  g_option_group_set_parse_hooks (group, (GOptionParseFunc) bt_init_pre,
      (GOptionParseFunc) bt_init_post);

  g_option_group_add_entries (group, options);
  g_option_group_set_translation_domain (group, PACKAGE_NAME);

  return group;

}

/**
 * bt_init_add_option_groups:
 * @ctx: main option context
 *
 * Adds all option groups to the main context the core library will pull in.
 */
void
bt_init_add_option_groups (GOptionContext * const ctx)
{
  g_option_context_add_group (ctx, gst_init_get_option_group ());
  g_option_context_add_group (ctx, bt_init_get_option_group ());
}

/**
 * bt_init_check:
 * @argc: (inout) (allow-none): pointer to application's argc
 * @argv: (inout) (array length=argc) (allow-none): pointer to application's argv
 * @err: pointer to a #GError to which a message will be posted on error
 *
 * Initializes the Buzztrax core library.
 *
 * This function will return %FALSE if Buzztrax core could not be initialized
 * for some reason.  If you want your program to fail fatally,
 * use bt_init() instead.
 *
 * Returns: %TRUE if Buzztrax core could be initialized.
 */
gboolean
bt_init_check (gint * argc, gchar ** argv[], GError ** err)
{
  GOptionContext *ctx;
  gboolean res;

  ctx = g_option_context_new (NULL);
  bt_init_add_option_groups (ctx);
  res = g_option_context_parse (ctx, argc, argv, err);
  g_option_context_free (ctx);
  return res;
}

/**
 * bt_init:
 * @argc: (inout) (allow-none): pointer to application's argc
 * @argv: (inout) (array length=argc) (allow-none): pointer to application's argv
 *
 * Initializes the Buzztrax Core library.
 *
 * <note><para>
 * This function will terminate your program if it was unable to initialize
 * the core for some reason.  If you want your program to fall back,
 * use bt_init_check() instead.
 * </para></note>
 *
 * WARNING: This function does not work in the same way as corresponding
 * functions in other glib-style libraries, such as gtk_init(). In
 * particular, unknown command line options cause this function to
 * abort program execution.
 */
void
bt_init (gint * argc, gchar ** argv[])
{
  GError *err = NULL;

  if (!bt_init_check (argc, argv, &err)) {
    g_print ("Could not initialized Buzztrax core: %s\n",
        err ? err->message : "unknown error occurred");
    if (err) {
      g_error_free (err);
    }
    exit (1);
  }
}

/**
 * bt_deinit:
 *
 * It is normally not needed to call this function in a normal application
 * as the resources will automatically be freed when the program terminates.
 * This function is therefore mostly used by testsuites and other memory
 * profiling tools.
 */
void
bt_deinit (void)
{
  // release some static ressources
  gst_caps_replace (&bt_default_caps, NULL);
  // deinit libraries
  gst_deinit ();
}


static gboolean
ensure_path (const gchar * env, const gchar * segment)
{
  const gchar *cur_var = g_getenv (env);
  gboolean modified = FALSE;
  gchar **path = NULL;

  if (cur_var) {
    path = g_strsplit (cur_var, ":", -1);
    if (!g_strv_contains ((const gchar * const *) path, segment)) {
      gchar *new_var = g_strconcat (segment, ":", cur_var, NULL);
      g_setenv (env, new_var, TRUE);
      g_free (new_var);
      modified = TRUE;
    }
    g_strfreev (path);
  } else {
    g_setenv (env, segment, TRUE);
    modified = TRUE;
  }
  return modified;
}


/**
 * bt_setup_for_local_install:
 *
 * Checks if for all env-vars are set for the location we run from. If not makes
 * the neccesary adjustments.
 *
 * Note: This must be called before initializing any other library.
 */
void
bt_setup_for_local_install (void)
{
  // NOTE: don't call any logging helpers!
  // printf ("install-prefix is: " PREFIX "\n");

  // probe and update the environment
  ensure_path ("LD_LIBRARY_PATH", LIBDIR);
  ensure_path ("XDG_DATA_DIRS", DATADIR);

  gchar *plugin_path =
      g_build_filename (LIBDIR, "gstreamer-" GST_MAJORMINOR, NULL);
  if (ensure_path ("GST_PLUGIN_PATH", plugin_path)) {
    gchar *registry_file = g_build_filename (g_get_user_cache_dir (),
        PACKAGE_NAME,
        "gstreamer-" GST_MAJORMINOR "-registry." TARGET_CPU ".bin",
        NULL);
    g_setenv ("GST_REGISTRY", registry_file, TRUE);
    g_free (registry_file);
  }
  g_free (plugin_path);
}
