/* $Id: bt-edit.c,v 1.1 2004-07-28 14:59:10 ensonic Exp $
 */

#define BT_EDIT_C

#include "bt-edit.h"

static void usage(int argc, char **argv, const struct poptOption *options) {
  const poptContext context=poptGetContext(argv[0], argc, argv, options, 0);
  poptPrintUsage(context,stdout,0);
  poptFreeContext(context);
  exit(0);
}

int main(int argc, char **argv) {
	gboolean res=FALSE;
  gboolean arg_version=FALSE;
  gchar *command=NULL,*input_file_name=NULL;
	/*BtEditApplication *app;*/

	struct poptOption options[] = {
		{"version",     '\0', POPT_ARG_NONE   | POPT_ARGFLAG_STRIP, &arg_version, 0, "version", NULL },
		{"command",     '\0', POPT_ARG_STRING | POPT_ARGFLAG_STRIP, &command,     0, "command name", "{load}" },
		{"input-file",  '\0', POPT_ARG_STRING | POPT_ARGFLAG_STRIP, &input_file_name, 	0, "input file name", "SONGFILE" },
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
  g_printf("command=\"%s\" input=\"%s\"\n",command, input_file_name);

  /*
	app=(BtEditApplication *)g_object_new(BT_TYPE_EDIT_APPLICATION,NULL);
  // depending on the popt options call the correct method
  if(!strncmp(command,"load",4)) {
    if(!input_file_name) usage(argc, argv, options);
    res=bt_edit_application_load(app,input_file_name);
  }
  else {
    res=bt_edit_application_run(app);
  }
	
	// free application
	g_object_unref(G_OBJECT(app));
	*/
	return(!res);
}

