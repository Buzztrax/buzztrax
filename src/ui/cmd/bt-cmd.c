/* $Id$
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
 * SECTION:btcmd
 * @short_description: buzztard commandline tool
 * @see_also: #BtCmdApplication
 *
 * Implements the body of the buzztard commandline tool.
 *
 * You can try to run the uninstalled program via
 * <informalexample><programlisting>
 *  libtool --mode=execute buzztard-cmd --command=info --input-file=&lt;filename&gt;
 * </programlisting></informalexample>
 * to enable debug output add:
 * <informalexample><programlisting>
 *  --gst-debug="*:2,bt-*:3" for not-so-much-logdata or
 *  --gst-debug="*:2,bt-*:4" for a-lot-of-log-data
 * </programlisting></informalexample>
 *
 * Example songs can be found in <filename>./test/songs/</filename>.
 */

#define BT_CMD
#define BT_CMD_C

#include "bt-cmd.h"

static void usage(int argc, char **argv, GOptionContext *ctx) {
#if GLIB_CHECK_VERSION(2,14,0)
  gchar *help=g_option_context_get_help(ctx, TRUE, NULL);
  puts(help);
  g_free(help);
#endif
}

int main(int argc, char **argv) {
  gboolean res=FALSE;
  gboolean arg_version=FALSE;
  gboolean arg_quiet=FALSE;
  gchar *command=NULL,*input_file_name=NULL,*output_file_name=NULL;
  BtCmdApplication *app;
  GOptionContext *ctx;
  GOptionGroup *group;
  GError *err=NULL;

#ifdef ENABLE_NLS
  setlocale(LC_ALL, "");
  bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
  textdomain(GETTEXT_PACKAGE);
#endif /* ENABLE_NLS */

  GOptionEntry options[] = {
    {"version",     '\0', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_NONE,     &arg_version,      N_("Print application version"),     NULL },
    {"quiet",       'q',  G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_NONE,     &arg_quiet,        N_("Be quiet"),         NULL },
    {"command",     '\0', 0,                    G_OPTION_ARG_STRING,   &command,          N_("Command name"),     "{info, play, convert, encode}" },
    {"input-file",  '\0', 0,                    G_OPTION_ARG_FILENAME, &input_file_name,  N_("Input file name"),  N_("<songfile>") },
    {"output-file", '\0', 0,                    G_OPTION_ARG_FILENAME, &output_file_name, N_("Output file name"), N_("<songfile>") },
    {NULL}
  };

  // initialize as soon as possible
  if(!g_thread_supported()) {
    g_thread_init(NULL);
  }

  // init libraries
  ctx = g_option_context_new(NULL);
  //g_option_context_add_main_entries(ctx, options, GETTEXT_PACKAGE);
  group=g_option_group_new("main", _("buzztard-cmd options"),_("Show buzztard-cmd options"), argv[0], NULL);
  g_option_group_add_entries(group, options);
  g_option_group_set_translation_domain(group, GETTEXT_PACKAGE);
  g_option_context_set_main_group(ctx, group);

  bt_init_add_option_groups(ctx);
  if(!g_option_context_parse(ctx, &argc, &argv, &err)) {
    g_print("Error initializing: %s\n", safe_string(err->message));
    g_option_context_free(ctx);
    exit(1);
  }
  if(arg_version) {
    g_printf("%s from "PACKAGE_STRING"\n",argv[0]);
    res=TRUE;
    goto Done;
  }
  if(!command) {
    usage(argc, argv, ctx);
    goto Done;
  }
  
  g_log_set_always_fatal(G_LOG_LEVEL_WARNING);
  GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "bt-cmd", 0, "music production environment / command ui");

  GST_INFO("starting: thread=%p, command=\"%s\" input=\"%s\" output=\"%s\"",
    g_thread_self(),command, input_file_name, output_file_name);

  // give some global context info
  g_set_application_name("Buzztard");
  g_setenv("PULSE_PROP_application.icon_name", "buzztard", TRUE);
  g_setenv("PULSE_PROP_media.role", "production", TRUE);

  app=bt_cmd_application_new(arg_quiet);

  // depending on the popt options call the correct method
  if(!strncmp(command,"play",4)) {
    if(!input_file_name) usage(argc, argv, ctx);
    res=bt_cmd_application_play(app,input_file_name);
  }
  else if(!strncmp(command,"info",4)) {
    if(!input_file_name) usage(argc, argv, ctx);
    res=bt_cmd_application_info(app,input_file_name, output_file_name);
  }
  else if(!strncmp(command,"convert",7)) {
    if(!input_file_name || !output_file_name) usage(argc, argv, ctx);
    res=bt_cmd_application_convert(app,input_file_name,output_file_name);
  }
  else if(!strncmp(command,"encode",6)) {
    if(!input_file_name || !output_file_name) usage(argc, argv, ctx);
    res=bt_cmd_application_encode(app,input_file_name,output_file_name);
  }
  else usage(argc, argv, ctx);
  
  // free application
  g_object_unref(app);

Done:
  g_free(command);
  g_free(input_file_name);
  g_free(output_file_name);
  g_option_context_free(ctx);
  
  return(!res);
}
