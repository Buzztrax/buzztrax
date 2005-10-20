/* $Id: core.c,v 1.21 2005-10-20 10:07:39 ensonic Exp $
 */
/**
 * SECTION:btcore
 * @short_description: core library of the buzztard application framework
 *
 * The library offers base objects such as #BtApplication and #BtSong.
 *
 */
 
/* @todo add check_version stuff like in gstreamer */

#define BT_CORE
#define BT_CORE_C
#include <libbtcore/core.h>

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
  //-- initialize gstreamer
  //gst_init(argc,argv);
  //-- initialize dynamic parameter control module
  //gst_controller_init(argc,argv);
  gst_controller_init(NULL,NULL);

  GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "bt-core", 0, "music production environment / core library");

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
  
  return TRUE;
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
void bt_init_add_option_groups(GOptionContext *ctx) {
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
