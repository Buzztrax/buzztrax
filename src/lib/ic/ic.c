/* Buzztrax
 * Copyright (C) 2007 Buzztrax team <buzztrax-devel@buzztrax.org>
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
 * SECTION:libbtic
 * @short_description: interaction controller library of the buzztrax
 *   application framework
 *
 * The library offers an abstraction of hardware controllers that can be bound
 * to ui controls.
 */

#define BTIC_CORE
#define BTIC_CORE_C

#include "ic_private.h"
#include <glib/gprintf.h>
#include <gio/gio.h>

/**
 * btic_major_version:
 *
 * buzztrax version stamp, major part; determined from #BTIC_MAJOR_VERSION
 */
const guint btic_major_version = BTIC_MAJOR_VERSION;
/**
 * btic_minor_version:
 *
 * buzztrax version stamp, minor part; determined from #BTIC_MINOR_VERSION
 */
const guint btic_minor_version = BTIC_MINOR_VERSION;
/**
 * btic_micro_version:
 *
 * buzztrax version stamp, micro part; determined from #BTIC_MICRO_VERSION
 */
const guint btic_micro_version = BTIC_MICRO_VERSION;

GST_DEBUG_CATEGORY (GST_CAT_DEFAULT);

static gboolean arg_version = FALSE;

//-- helper methods

/* we have no fail cases yet, but maybe in the future */
static gboolean
btic_init_pre (void)
{
#ifdef ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

  //g_log_set_always_fatal(G_LOG_LEVEL_WARNING);
  return TRUE;
}

static gboolean
btic_init_post (void)
{
  if (arg_version) {
    g_printf ("libbuzztrax-ic-%d.%d.%d from " PACKAGE_STRING "\n",
        BTIC_MAJOR_VERSION, BTIC_MINOR_VERSION, BTIC_MICRO_VERSION);
  }

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "bt-ic", 0,
      "music production environment / interaction controller library");

  return TRUE;
}

//-- ic initialisation

/**
 * btic_init_get_option_group: (skip)
 *
 * Returns a #GOptionGroup with libbtic's argument specifications. The group
 * is set up to use standard GOption callbacks, so when using this group in
 * combination with GOption parsing methods, all argument parsing and
 * initialization is automated.
 *
 * This function is useful if you want to integrate libbtic with other
 * libraries that use GOption (see g_option_context_add_group() ).
 *
 * Returns: (transfer full): a pointer to a GOption group. Should be
 * dereferenced after use.
 */
GOptionGroup *
btic_init_get_option_group (void)
{
  GOptionGroup *group;
  static GOptionEntry options[] = {
    {"btic-version", 0, 0, G_OPTION_ARG_NONE, NULL,
        N_("Print the buzztrax interaction controller version"), NULL},
    {NULL}
  };
  options[0].arg_data = &arg_version;

  group =
      g_option_group_new ("bt-ic", _("Buzztrax interaction controller options"),
      _("Show buzztrax interaction controller options"), NULL, NULL);
  g_option_group_set_parse_hooks (group, (GOptionParseFunc) btic_init_pre,
      (GOptionParseFunc) btic_init_post);

  g_option_group_add_entries (group, options);
  g_option_group_set_translation_domain (group, PACKAGE_NAME);

  return group;
}

/**
 * btic_init_check:
 * @argc: pointer to application's argc
 * @argv: pointer to application's argv
 * @err: pointer to a #GError to which a message will be posted on error
 *
 * Initializes the Buzztrax interaction controller library.
 *
 * This function will return %FALSE if Buzztrax interaction controller could not
 * be initialized for some reason.  If you want your program to fail fatally,
 * use btic_init() instead.
 *
 * Returns: %TRUE if Buzztrax interaction controller could be initialized.
 */
gboolean
btic_init_check (gint * argc, gchar ** argv[], GError ** err)
{
  GOptionContext *ctx;
  gboolean res;

  ctx = g_option_context_new (NULL);
  g_option_context_add_group (ctx, btic_init_get_option_group ());
  res = g_option_context_parse (ctx, argc, argv, err);
  g_option_context_free (ctx);

  return res;
}


/**
 * btic_init:
 * @argc: pointer to application's argc
 * @argv: pointer to application's argv
 *
 * Initializes the Buzztrax Interaction Controller library.
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
void
btic_init (gint * argc, gchar ** argv[])
{
  GError *err = NULL;

  if (!btic_init_check (argc, argv, &err)) {
    g_print ("Could not initialized Buzztrax interaction controller: %s\n",
        err ? err->message : "unknown error occurred");
    if (err) {
      g_error_free (err);
    }
    exit (1);
  }
}
