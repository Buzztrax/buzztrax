/* $Id: bt-edit.c,v 1.8 2004-09-15 16:57:59 ensonic Exp $
 * You can try to run the uninstalled program via
 *   libtool --mode=execute bt-edit
 * to enable debug output add:
 *  --gst-debug="*:2,bt-*:3" for not-so-much-logdata or
 *  --gst-debug="*:2,bt-*:4" for a-lot-logdata
 */

#define BT_EDIT
#define BT_EDIT_C

#include "bt-edit.h"

static void usage(int argc, char **argv, const struct poptOption *options) {
  const poptContext context=poptGetContext(argv[0], argc, (const char **)argv, options, 0);
  poptPrintUsage(context,stdout,0);
  poptFreeContext(context);
  exit(0);
}

int main(int argc, char **argv) {
	gboolean res=FALSE;
  gboolean arg_version=FALSE;
  gchar *command=NULL,*input_file_name=NULL;
	BtEditApplication *app;

	struct poptOption options[] = {
		{"version",     '\0', POPT_ARG_NONE   | POPT_ARGFLAG_STRIP | POPT_ARGFLAG_OPTIONAL, &arg_version, 0, "version", NULL },
		{"command",     '\0', POPT_ARG_STRING | POPT_ARGFLAG_STRIP | POPT_ARGFLAG_OPTIONAL, &command,     0, "command name", "{load}" },
		{"input-file",  '\0', POPT_ARG_STRING | POPT_ARGFLAG_STRIP | POPT_ARGFLAG_OPTIONAL, &input_file_name, 	0, "input file name", "SONGFILE" },
		POPT_TABLEEND
	};
  
  // init gtk (before or after bt_init ?)
  gtk_init(&argc,&argv);
	// init buzztard core with own popt options
	bt_init(&argc,&argv,options);

	GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "bt-edit", 0, "music production environment / gtk ui");
  
  add_pixmap_directory(DATADIR""G_DIR_SEPARATOR_S"pixmaps"G_DIR_SEPARATOR_S);
  
  if(!g_thread_supported()) g_thread_init(NULL);

  if(arg_version) {
    g_printf("%s from "PACKAGE_STRING"\n",argv[0]);
    exit(0);
  }

	app=bt_edit_application_new();
  if(command) {
    // depending on the popt options call the correct method
    if(!strncmp(command,"load",4)) {
      if(!input_file_name) usage(argc, argv, options);
      res=bt_edit_application_load_and_run(app,input_file_name);
    }
    else usage(argc, argv, options);
  }
  else {
    res=bt_edit_application_run(app);
  }
	
  GST_DEBUG("exiting ...");
	// free application
  {
    GObject *object=G_OBJECT(app);
    GST_INFO("app->ref_ct=%d",object->ref_count);
  }
	g_object_unref(app);
	return(!res);
}

