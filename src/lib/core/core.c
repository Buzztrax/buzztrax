/* $Id: core.c,v 1.9 2004-07-28 13:54:42 ensonic Exp $
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
 * @argc: pointer to commandline argument count
 * @argv: pointer to commandline arguments
 * @options: custom commandline options from the application 
 *
 * initialize the libbtcore usage.
 * This function prepares gstreamer and libxml. 
 */
void bt_init(int *argc, char ***argv, struct poptOption *options) {

	//-- init gobject
	g_type_init();

	//-- init gstreamer with popt options
	gst_init_with_popt_table(argc,argv,(GstPoptOption*)options);
  gst_control_init(argc,argv);
	GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "bt-core", 0, "music production environment / core library");

	//-- init libxml
  // set own error handler
  //xmlSetGenericErrorFunc("libxml-error: ",&gitk_libxmlxslt_error_func);
  // initialise the xml parser
  xmlInitParser();
  // xmlInitParser does that for us
  //xmlXPathInit();
  xmlSubstituteEntitiesDefault(1);
  xmlLoadExtDtdDefaultValue=TRUE;						// always load DTD default values (even when not validating)
	xmlDoValidityCheckingDefaultValue=FALSE;	// do not validate files (we load xsl files as well
}

