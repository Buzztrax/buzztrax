// $Id: bt-cmd.c,v 1.31 2005-10-20 10:07:43 ensonic Exp $
/**
 * SECTION:btcmd
 * @short_description: buzztard commandline tool
 *
 * Implements the body of the buzztard commandline tool.
 *
 * You can try to run the uninstalled program via
 * <informalexample><programlisting>
 *  libtool --mode=execute bt-cmd --command=info --input-file=&lt;filename&gt;
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
  // @todo need a replacement for this
  //poptPrintUsage(context,stdout,0);
  //poptFreeContext(context);
  g_option_context_free(ctx);
  exit(0);
}

int main(int argc, char **argv) {
  gboolean res=FALSE;
  gboolean arg_version=FALSE;
  gboolean arg_quiet=FALSE;
  gchar *command=NULL,*input_file_name=NULL,*output_file_name=NULL;
  BtCmdApplication *app;
  GOptionContext *ctx;
  GError *err=NULL;
  
  GOptionEntry options[] = {
    {"version",     '\0', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_NONE,     &arg_version,      N_("Show version"),     NULL },
    {"quiet",       'q',  G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_NONE,     &arg_quiet,        N_("Be quiet"),         NULL },
    {"command",     '\0', 0,                    G_OPTION_ARG_STRING,   &command,          N_("Command name"),     N_("{info, play, convert, encode}") },
    {"input-file",  '\0', 0,                    G_OPTION_ARG_FILENAME, &input_file_name,  N_("Input file name"),  N_("SONGFILE") },
    {"output-file", '\0', 0,                    G_OPTION_ARG_FILENAME, &output_file_name, N_("Output file name"), N_("SONGFILE") },
    {NULL}
  };

  // init libraries
  ctx = g_option_context_new(NULL);
  g_option_context_add_main_entries (ctx, options, PACKAGE_NAME);
  bt_init_add_option_groups(ctx);
  if(!g_option_context_parse(ctx, &argc, &argv, &err)) {
    g_print("Error initializing: %s\n", safe_string(err->message));
    exit(1);
  }
  
  g_log_set_always_fatal(G_LOG_LEVEL_WARNING);
  GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "bt-cmd", 0, "music production environment / command ui");

  if(arg_version) {
    g_printf("%s from "PACKAGE_STRING"\n",argv[0]);
    g_option_context_free(ctx);
    exit(0);
  }
  if(!command) usage(argc, argv, ctx);
  GST_DEBUG("command=\"%s\" input=\"%s\" output=\"%s\"\n",command, input_file_name, output_file_name);

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
    g_printf("sorry this is not yet implemented\n");
    res=bt_cmd_application_encode(app,input_file_name,output_file_name);
  }
  else usage(argc, argv, ctx);
  
  // free application
  g_option_context_free(ctx);
  g_object_unref(app);
  
  return(!res);
}
