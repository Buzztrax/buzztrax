/* $Id: bt-cmd.c,v 1.19 2004-08-17 14:38:12 waffel Exp $
 * You can try to run the uninstalled program via
 *   libtool --mode=execute bt-cmd --command=info --input-file=<filename>
 * to enable debug output add:
 *  --gst-debug="*:2,bt-*:3" for not-so-much-logdata or
 *  --gst-debug="*:2,bt-*:4" for a-lot-logdata
 *
 * example songs can be found in ./test/songs/
 */

#define BT_CMD
#define BT_CMD_C

#include "bt-cmd.h"

static void usage(int argc, char **argv, const struct poptOption *options) {
  const poptContext context=poptGetContext(argv[0], argc, (const char **)argv, options, 0);
  poptPrintUsage(context,stdout,0);
  poptFreeContext(context);
  exit(0);
}

int main(int argc, char **argv) {
	gboolean res=FALSE;
  gboolean arg_version=FALSE;
  gchar *command=NULL,*input_file_name=NULL,*output_file_name=NULL;
	BtCmdApplication *app;

	struct poptOption options[] = {
		{"version",     '\0', POPT_ARG_NONE   | POPT_ARGFLAG_STRIP | POPT_ARGFLAG_OPTIONAL, &arg_version, 0, "version", NULL },
		{"command",     '\0', POPT_ARG_STRING | POPT_ARGFLAG_STRIP, &command,     0, "command name", "{info, play, convert, encode}" },
		{"input-file",  '\0', POPT_ARG_STRING | POPT_ARGFLAG_STRIP, &input_file_name, 	0, "input file name", "SONGFILE" },
		{"output-file", '\0', POPT_ARG_STRING | POPT_ARGFLAG_STRIP | POPT_ARGFLAG_OPTIONAL, &output_file_name,	0, "output file name", "SONGFILE" },
		POPT_TABLEEND
	};
  
	// init buzztard core with own popt options
	bt_init(&argc,&argv,options);

	GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "bt-cmd", 0, "music production environment / command ui");

  if(arg_version) {
    g_printf("%s from "PACKAGE_STRING"\n",argv[0]);
    exit(0);
  }
  if(!command) usage(argc, argv, options);
  g_printf("command=\"%s\" input=\"%s\" output=\"%s\"\n",command, input_file_name, output_file_name);

	app=bt_cmd_application_new();
  // depending on the popt options call the correct method
  if(!strncmp(command,"play",4)) {
    if(!input_file_name) usage(argc, argv, options);
    res=bt_cmd_application_play(app,input_file_name);
  }
  else if(!strncmp(command,"info",4)) {
    if(!input_file_name) usage(argc, argv, options);
    res=bt_cmd_application_info(app,input_file_name, output_file_name);
  }
  else if(!strncmp(command,"convert",7)) {
    if(!input_file_name || !output_file_name) usage(argc, argv, options);
    g_printf("sorry this is not yet implemented\n");
    // @todo implement bt_cmd_application_convert()
    //res=bt_cmd_application_convert(app,input_file_name,output_file_name);
  }
  else if(!strncmp(command,"encode",6)) {
    if(!input_file_name || !output_file_name) usage(argc, argv, options);
    g_printf("sorry this is not yet implemented\n");
    // @todo implement bt_cmd_application_encode() like play, with a different sink
    //res=bt_cmd_application_encode(app,input_file_name,output_file_name);
  }
  else usage(argc, argv, options);
	
	// free application
	g_object_unref(G_OBJECT(app));
	
	return(!res);
}

