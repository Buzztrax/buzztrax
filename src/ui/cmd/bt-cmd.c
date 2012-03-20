/* Buzztard
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

  static GOptionEntry options[] = {
    {"version",     '\0', 0, G_OPTION_ARG_NONE,     NULL, N_("Print application version"),     NULL },
    {"quiet",       'q',  0, G_OPTION_ARG_NONE,     NULL, N_("Be quiet"),         NULL },
    {"command",     'c',  0, G_OPTION_ARG_STRING,   NULL, N_("Command name"),     "{info, play, convert, encode}" },
    {"input-file",  'i',  0, G_OPTION_ARG_FILENAME, NULL, N_("Input file name"),  N_("<songfile>") },
    {"output-file", 'o',  0, G_OPTION_ARG_FILENAME, NULL, N_("Output file name"), N_("<songfile>") },
    {NULL}
  };
  // setting this separately gets us from 76 to 10 instructions
  options[0].arg_data=&arg_version;
  options[1].arg_data=&arg_quiet;
  options[2].arg_data=&command;
  options[3].arg_data=&input_file_name;
  options[4].arg_data=&output_file_name;

#if !GLIB_CHECK_VERSION (2, 31, 0) 
  // initialize as soon as possible
  if(!g_thread_supported()) {
    g_thread_init(NULL);
  }
#endif

  // init libraries
  ctx = g_option_context_new(NULL);
  //g_option_context_add_main_entries(ctx, options, GETTEXT_PACKAGE);
  group=g_option_group_new("main", _("buzztard-cmd options"),_("Show buzztard-cmd options"), argv[0], NULL);
  g_option_group_add_entries(group, options);
  g_option_group_set_translation_domain(group, GETTEXT_PACKAGE);
  g_option_context_set_main_group(ctx, group);

  bt_init_add_option_groups(ctx);
  g_option_context_add_group(ctx, btic_init_get_option_group());

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
  
  GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "bt-cmd", 0, "music production environment / command ui");
  GST_INFO("starting: command=\"%s\" input=\"%s\" output=\"%s\"", command, input_file_name, output_file_name);

  // give some global context info
  g_set_application_name("Buzztard");
  g_setenv("PULSE_PROP_application.icon_name", "buzztard", TRUE);
  g_setenv("PULSE_PROP_media.role", "production", TRUE);

  app=bt_cmd_application_new(arg_quiet);

  // depending on the popt options call the correct method
  if(!strcmp(command,"p") || !strcmp(command,"play")) {
    if(!BT_IS_STRING(input_file_name)) usage(argc, argv, ctx);
    res=bt_cmd_application_play(app,input_file_name);
  }
  else if(!strcmp(command,"i") || !strcmp(command,"info")) {
    if(!BT_IS_STRING(input_file_name)) usage(argc, argv, ctx);
    res=bt_cmd_application_info(app,input_file_name, output_file_name);
  }
  else if(!strcmp(command,"c") || !strcmp(command,"convert")) {
    if(!BT_IS_STRING(input_file_name) || !BT_IS_STRING(output_file_name)) usage(argc, argv, ctx);
    res=bt_cmd_application_convert(app,input_file_name,output_file_name);
  }
  else if(!strcmp(command,"e") ||!strcmp(command,"encode")) {
    if(!BT_IS_STRING(input_file_name) || !BT_IS_STRING(output_file_name)) usage(argc, argv, ctx);
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
