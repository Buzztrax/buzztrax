/* $Id: core.c,v 1.14 2005-04-27 16:31:05 ensonic Exp $
 */

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

/**
 * bt_init:
 * @argc: pointer to command line argument count
 * @argv: pointer to command line arguments
 * @options: custom command line options from the application 
 *
 * initialize the libbtcore usage.
 * This function prepares gstreamer and libxml. 
 */
void bt_init(int *argc, char ***argv, struct poptOption *options) {
  
	//-- initialize gobject
	g_type_init();
  //g_log_set_always_fatal(G_LOG_LEVEL_WARNING);

	//-- initialize gstreamer with popt options
  if(options) {
    gst_init_with_popt_table(argc,argv,(GstPoptOption*)options);
  }
  else {
    gst_init(argc,argv);
  }
	//-- initialize dynamic parameter control module
#ifdef USE_GST_DPARAMS
  gst_control_init(argc,argv);
#endif
#ifdef USE_GST_CONTROLLER
	gst_library_load("gstcontroller");
#endif

	GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "bt-core", 0, "music production environment / core library");

	//-- initialize libxml
  // set own error handler
  //xmlSetGenericErrorFunc("libxml-error: ",&gitk_libxmlxslt_error_func);
  // initialize the xml parser
  xmlInitParser();
  // xmlInitParser does that for us
  //xmlXPathInit();
  xmlSubstituteEntitiesDefault(1);
  xmlLoadExtDtdDefaultValue=TRUE;						// always load DTD default values (even when not validating)
	xmlDoValidityCheckingDefaultValue=FALSE;	// do not validate files (we load xsl files as well
}
