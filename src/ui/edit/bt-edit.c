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
 * SECTION:btedit
 * @short_description: buzztrax graphical editor application
 * @see_also: #BtEditApplication
 *
 * Implements the body of the buzztrax GUI editor.
 *
 * You can try to run the uninstalled program via
 * <informalexample><programlisting>
 *   libtool --mode=execute buzztrax-edit
 * </programlisting></informalexample>
 * to enable debug output add:
 * <informalexample><programlisting>
 *  --gst-debug="*:2,bt-*:3" for not-so-much-logdata or
 *  --gst-debug="*:2,bt-*:4" for a-lot-logdata
 * </programlisting></informalexample>
 *
 * Example songs can be found in <filename>./test/songs/</filename>.
 */

#define BT_EDIT
#define BT_EDIT_C

#include "bt-edit.h"
#include <clutter-gtk/clutter-gtk.h>
#include <glib/gprintf.h>

#ifdef ENABLE_NLS
#ifdef HAVE_X11_XLOCALE_H
  /* defines a more portable setlocale for X11 (_Xsetlocale) */
#include <X11/Xlocale.h>
#else
#include <locale.h>
#endif
#endif

static void
usage (int argc, char **argv, GOptionContext * ctx)
{
  gchar *help = g_option_context_get_help (ctx, TRUE, NULL);
  puts (help);
  g_free (help);
}

// see if(arg_version) comment in main() below
static gboolean
parse_goption_arg (const gchar * opt, const gchar * arg, gpointer data,
    GError ** err)
{
  if (!strcmp (opt, "--version")) {
    g_printf ("%s from " PACKAGE_STRING "\n", (gchar *) data);
    exit (0);
  }
  return TRUE;
}

gint
main (gint argc, gchar ** argv)
{
  gboolean res = FALSE;
  gchar *command = NULL, *input_file_name = NULL;
  BtEditApplication *app;
  GOptionContext *ctx = NULL;
  GOptionGroup *group;
  GError *err = NULL;

#ifdef ENABLE_NLS
  setlocale (LC_ALL, "");
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
#endif /* ENABLE_NLS */

  bt_setup_for_local_install ();

  GOptionEntry options[] = {
    {"version", '\0', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
        (gpointer) parse_goption_arg, N_("Print application version"), NULL}
    ,
    {"command", 'c', 0, G_OPTION_ARG_STRING, &command, N_("Command name"),
        "{load}"}
    ,
    {"input-file", 'i', 0, G_OPTION_ARG_FILENAME, &input_file_name,
        N_("Input file name"), N_("<songfile>")}
    ,
    {NULL}
  };

  // init libraries
  ctx = g_option_context_new (NULL);
  //g_option_context_add_main_entries (ctx, options, GETTEXT_PACKAGE);
  group =
      g_option_group_new ("main", _("buzztrax-edit options"),
      _("Show buzztrax-edit options"), argv[0], NULL);
  g_option_group_add_entries (group, options);
  g_option_group_set_translation_domain (group, GETTEXT_PACKAGE);
  g_option_context_set_main_group (ctx, group);

  bt_init_add_option_groups (ctx);
  g_option_context_add_group (ctx, btic_init_get_option_group ());
  g_option_context_add_group (ctx, gtk_get_option_group (TRUE));
  g_option_context_add_group (ctx, clutter_get_option_group_without_init ());
  g_option_context_add_group (ctx, gtk_clutter_get_option_group ());

  if (!bt_init_check (ctx, &argc, &argv, &err)) {
    g_print ("Error initializing: %s\n", safe_string (err->message));
    g_error_free (err);
    goto Done;
  }

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "bt-edit", 0,
      "music production environment / editor ui");

  // give some global context info
  g_set_prgname ("buzztrax-edit");
  g_set_application_name ("Buzztrax");
  gtk_window_set_default_icon_name ("buzztrax");
  g_setenv ("PULSE_PROP_media.role", "production", TRUE);

  extern gboolean bt_memory_audio_src_plugin_init (GstPlugin * const plugin);
  gst_plugin_register_static (GST_VERSION_MAJOR,
      GST_VERSION_MINOR,
      "memoryaudiosrc",
      "Plays audio from memory",
      bt_memory_audio_src_plugin_init,
      VERSION, "LGPL", PACKAGE, PACKAGE_NAME, "http://www.buzztrax.org");

  GST_INFO ("starting: thread=%p", g_thread_self ());

  app = bt_edit_application_new ();

  // set a default command, if a file is given
  if (!command && BT_IS_STRING (input_file_name)) {
    command = g_strdup ("l");
  }

  if (command) {
    // depending on the options call the correct method
    if (!strcmp (command, "l") || !strcmp (command, "load")) {
      if (!BT_IS_STRING (input_file_name)) {
        usage (argc, argv, ctx);
        // if commandline options where wrong, just start
        res = bt_edit_application_run (app);
      } else {
        res = bt_edit_application_load_and_run (app, input_file_name);
      }
    } else {
      usage (argc, argv, ctx);
      // if commandline options where wrong, just start
      res = bt_edit_application_run (app);
    }
  } else {
    res = bt_edit_application_run (app);
  }

  // free application
  GST_INFO ("app %" G_OBJECT_REF_COUNT_FMT, G_OBJECT_LOG_REF_COUNT (app));
  g_object_unref (app);

Done:
  g_free (command);
  g_free (input_file_name);
  g_option_context_free (ctx);
  return !res;
}
