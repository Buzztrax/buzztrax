/* $Id$
 *
 * Buzztard
 * Copyright (C) 2007 Buzztard team <buzztard-devel@lists.sf.net>
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
 * SECTION:libbtic
 * @short_description: interaction controller library of the buzztard
 *   application framework
 *
 * The library offers an abstraction of hardware controllers that can be bound
 * to ui controls.
 */

#define BTIC_CORE
#define BTIC_CORE_C

#include "ic_private.h"

/**
 * btic_major_version:
 *
 * buzztard version stamp, major part; determined from #BTIC_MAJOR_VERSION
 */
const unsigned int btic_major_version=BTIC_MAJOR_VERSION;
/**
 * btic_minor_version:
 *
 * buzztard version stamp, minor part; determined from #BTIC_MINOR_VERSION
 */
const unsigned int btic_minor_version=BTIC_MINOR_VERSION;
/**
 * btic_micro_version:
 *
 * buzztard version stamp, micro part; determined from #BTIC_MICRO_VERSION
 */
const unsigned int btic_micro_version=BTIC_MICRO_VERSION;

GST_DEBUG_CATEGORY(GST_CAT_DEFAULT);

static gboolean btic_initialized = FALSE;

//-- helper methods

/* we have no fail cases yet, but maybe in the future */
static gboolean btic_init_pre (void) {
#ifdef ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

  //-- initialize gobject
  g_type_init ();

  //g_log_set_always_fatal(G_LOG_LEVEL_WARNING);
  return TRUE;
}

static gboolean btic_init_post (void) {
  gboolean res=FALSE;

  GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "bt-ic", 0, "music production environment / interaction controller library");
  
  res=TRUE;
  
//Error:
  return(res);
}

static gboolean parse_goption_arg(const gchar *opt, const gchar * arg, gpointer data, GError ** err)
{
  gboolean ret=TRUE;
  
  if(!strcmp (opt, "--btic-version")) {
    g_printf("libbtic-%d.%d.%d from "PACKAGE_STRING"\n",BTIC_MAJOR_VERSION,BTIC_MINOR_VERSION,BTIC_MICRO_VERSION);
  }
  else ret=FALSE;
    
  return(ret);
}

//-- ic initialisation

/**
 * btic_init_get_option_group:
 *
 * Returns a #GOptionGroup with libbtic's argument specifications. The group
 * is set up to use standard GOption callbacks, so when using this group in
 * combination with GOption parsing methods, all argument parsing and
 * initialization is automated.
 *
 * This function is useful if you want to integrate libbtic with other
 * libraries that use GOption (see g_option_context_add_group() ).
 *
 * Returns: a pointer to a GOption group. Should be dereferenced after use.
 */
GOptionGroup *btic_init_get_option_group(void) {  
  GOptionGroup *group;
  static GOptionEntry btic_args[] = {
    {"btic-version", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, (gpointer)parse_goption_arg, N_("Print the buzztard interaction controller version"), NULL},
    {NULL}
  };
  
  group = g_option_group_new("btic-core", _("Buzztard interaction controller options"),_("Show buzztard interaction controller options"), NULL, NULL);
  g_option_group_set_parse_hooks(group, (GOptionParseFunc)btic_init_pre, (GOptionParseFunc)btic_init_post);

  g_option_group_add_entries(group, btic_args);
  g_option_group_set_translation_domain(group, PACKAGE_NAME);

  return group;
}

/**
 * btic_init_check:
 * @argc: pointer to application's argc
 * @argv: pointer to application's argv
 * @err: pointer to a #GError to which a message will be posted on error
 *
 * Initializes the Buzztard interaction controller library.
 *
 * This function will return %FALSE if Buzztard interaction controller could not
 * be initialized for some reason.  If you want your program to fail fatally,
 * use btic_init() instead.
 *
 * Returns: %TRUE if Buzztard interaction controller could be initialized.
 */
gboolean btic_init_check(int *argc, char **argv[], GError **err) {
  GOptionContext *ctx;
  gboolean res;

  if(btic_initialized) {
    //g_print("already initialized Buzztard interaction controller");
    return(TRUE);
  }

  ctx = g_option_context_new(NULL);
  g_option_context_add_group(ctx, btic_init_get_option_group());
  res = g_option_context_parse(ctx, argc, argv, err);
  g_option_context_free(ctx);
    
  btic_initialized=TRUE;

  return(res);
}


/**
 * btic_init:
 * @argc: pointer to application's argc
 * @argv: pointer to application's argv
 *
 * Initializes the Buzztard Interaction Controller library.
 *
 * <note><para>
 * This function will terminate your program if it was unable to initialize
 * the core for some reason.  If you want your program to fall back,
 * use btic_init_check() instead.
 * </para></note>
 *
 * WARNING: This function does not work in the same way as corresponding
 * functions in other glib-style libraries, such as gtk_init(). In
 * particular, unknown command line options cause this function to
 * abort program execution.
 */
void btic_init(int *argc, char **argv[]) {
  GError *err = NULL;

  if(!btic_init_check(argc, argv, &err)) {
    g_print("Could not initialized Buzztard interaction controller: %s\n", err ? err->message : "unknown error occurred");
    if(err) {
      g_error_free(err);
    }
    exit(1);
  }
}
