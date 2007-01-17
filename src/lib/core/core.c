/* $Id: core.c,v 1.33 2007-01-17 21:51:51 ensonic Exp $
 *
 * Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/**
 * SECTION:libbtcore
 * @short_description: core library of the buzztard application framework
 *
 * The library offers base objects such as #BtApplication and #BtSong.
 */
 
/* @todo add check_version stuff like in gstreamer */

#define BT_CORE
#define BT_CORE_C
#include <libbtcore/core.h>

#ifdef HAVE_SCHED_SETSCHEDULER
#include <sched.h>
#if HAVE_MLOCKALL
#include <sys/mman.h>
#endif
#endif

/**
 * bt_major_version:
 *
 * buzztard version stamp, major part; determined from #BT_MAJOR_VERSION
 */
const unsigned int bt_major_version=BT_MAJOR_VERSION;
/**
 * bt_minor_version:
 *
 * buzztard version stamp, minor part; determined from #BT_MINOR_VERSION
 */
const unsigned int bt_minor_version=BT_MINOR_VERSION;
/**
 * bt_micro_version:
 *
 * buzztard version stamp, micro part; determined from #BT_MICRO_VERSION
 */
const unsigned int bt_micro_version=BT_MICRO_VERSION;

GST_DEBUG_CATEGORY(GST_CAT_DEFAULT);

static gboolean bt_initialized = FALSE;
static gboolean arg_version=FALSE;

//-- helper methods

/* we have no fail cases yet, but maybe in the future */
static gboolean bt_init_pre (void) {
  //-- initialize gobject
  g_type_init ();

#ifdef ENABLE_NLS
  setlocale (LC_ALL, "");
  bindtextdomain ("bt-core", LOCALEDIR);
#endif /* ENABLE_NLS */

  //g_log_set_always_fatal(G_LOG_LEVEL_WARNING);

  return TRUE;
}

static gboolean bt_init_post (void) {
  gboolean res=FALSE;
  
  //-- initialize gstreamer
  //gst_init(argc,argv);
  //-- initialize dynamic parameter control module
  //gst_controller_init(argc,argv);
  gst_controller_init(NULL,NULL);

  GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "bt-core", 0, "music production environment / core library");

  GST_DEBUG("init xml");
  //-- initialize libxml
  // set own error handler
  //xmlSetGenericErrorFunc("libxml-error: ",&gitk_libxmlxslt_error_func);
  // initialize the xml parser
  xmlInitParser();
  // xmlInitParser does that for us
  //xmlXPathInit();
  xmlSubstituteEntitiesDefault(1);
  xmlLoadExtDtdDefaultValue=TRUE;            // always load DTD default values (even when not validating)
  xmlDoValidityCheckingDefaultValue=FALSE;  // do not validate files (we load xsl files as well  
  
  GST_DEBUG("init gnome-vfs");
  //-- initialize gnome-vfs
  if (!gnome_vfs_init ()) {
    GST_WARNING("gnome vfs failed to initialize");
    goto Error;
  }

#if 0
// I just got
// switching scheduler failed: Die Operation ist nicht erlaubt
#ifdef HAVE_SCHED_SETSCHEDULER
  // @idea; only do this in non-debug builds
  //http://www.gnu.org/software/libc/manual/html_node/Basic-Scheduling-Functions.html
  {
    struct sched_param p={0,};

    p.sched_priority=sched_get_priority_min(SCHED_RR);
    //p.sched_priority=sched_get_priority_max(SCHED_RR);
    if(sched_setscheduler(0,SCHED_RR,&p)<0) {
      GST_WARNING("switching scheduler failed: %s",g_strerror(errno));
    }
    else {
      GST_INFO("switched scheduler");
#if HAVE_MLOCKALL
      if(mlockall(MCL_CURRENT)<0)
        GST_WARNING("locking memory pages failed: %s",g_strerror(errno));
#endif
    }
  }
#endif
#endif
  
  res=TRUE;
  
Error:
  return(res);
}
  
//-- core initialisation

/**
 * bt_init_get_option_group:
 *
 * Returns a #GOptionGroup with libbtcore's argument specifications. The group
 * is set up to use standard GOption callbacks, so when using this group in
 * combination with GOption parsing methods, all argument parsing and
 * initialization is automated.
 *
 * This function is useful if you want to integrate libbtcore with other
 * libraries that use GOption (see g_option_context_add_group() ).
 *
 * Returns: a pointer to a GOption group. Should be dereferenced after use.
 */
GOptionGroup *bt_init_get_option_group(void) {  
  GOptionGroup *group;
  static GOptionEntry bt_args[] = {
    {"bt-version", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_NONE, &arg_version, N_("Print the buzztard core version"), NULL},
    {NULL}
  };
  
  group = g_option_group_new("bt-core", _("Buzztard core options"),_("Show buzztard core options"), NULL, NULL);
  g_option_group_set_parse_hooks(group, (GOptionParseFunc)bt_init_pre, (GOptionParseFunc)bt_init_post);

  g_option_group_add_entries(group, bt_args);
  g_option_group_set_translation_domain(group, PACKAGE_NAME);

  return group;

}

/**
 * bt_init_add_option_groups:
 * @ctx: main option context
 *
 * Adds all option groups to the main context the core library will pull in.
 */
void bt_init_add_option_groups(GOptionContext * const ctx) {
  g_option_context_add_group(ctx, gst_init_get_option_group());
  g_option_context_add_group(ctx, bt_init_get_option_group());
}

/**
 * bt_init_check:
 * @argc: pointer to application's argc
 * @argv: pointer to application's argv
 * @err: pointer to a #GError to which a message will be posted on error
 *
 * Initializes the GStreamer library, setting up internal path lists,
 * registering built-in elements, and loading standard plugins.
 *
 * This function will return %FALSE if GStreamer could not be initialized
 * for some reason.  If you want your program to fail fatally,
 * use gst_init() instead.
 *
 * Returns: %TRUE if GStreamer could be initialized.
 */
gboolean bt_init_check(int *argc, char **argv[], GError **err) {
  GOptionContext *ctx;
  gboolean res;

  if(bt_initialized) {
    //g_print("already initialized Buzztard core");
    return(TRUE);
  }

  ctx = g_option_context_new(NULL);
  bt_init_add_option_groups(ctx);
  res = g_option_context_parse(ctx, argc, argv, err);
  g_option_context_free(ctx);

  if(res) {
    // check for missing core elements (borked gstreamer install)
    GList *missing;

    if((missing=bt_gst_check_core_elements())) {
      GList *node;
      for(node=missing;node;node=g_list_next(node)) {
        GST_WARNING("missing core element '%s'",(gchar *)node->data);
      }
      g_list_free(missing);
    }
    
    bt_initialized=TRUE;
  }

  return(res);
}

/**
 * bt_init:
 * @argc: pointer to application's argc
 * @argv: pointer to application's argv
 *
 * Initializes the Buzztard Core library.
 *
 * <note><para>
 * This function will terminate your program if it was unable to initialize
 * the core for some reason.  If you want your program to fall back,
 * use bt_init_check() instead.
 * </note></para>
 *
 * WARNING: This function does not work in the same way as corresponding
 * functions in other glib-style libraries, such as gtk_init(). In
 * particular, unknown command line options cause this function to
 * abort program execution.
 */
void bt_init(int *argc, char **argv[]) {
  GError *err = NULL;

  if(!bt_init_check(argc, argv, &err)) {
    g_print("Could not initialized Buzztard Core: %s\n", err ? err->message : "unknown error occurred");
    if(err) {
      g_error_free(err);
    }
    exit(1);
  }
}
