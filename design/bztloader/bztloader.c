/* $Id$ */

#include "bztloader.h"

/*
 * try to read the given input file, open it an read the main buzztard song xml
 * file. Currently we searching hard for a file named "song.xml".
 */
int readBuzztardXmlFile(int argc, char**argv) {
  GsfInput  *input;
  GsfInfile *infile;
  GError    *err = NULL;
  
	// open the file from the first argument
  input = gsf_input_stdio_new (argv[1], &err);
  
  if (input == NULL) {
		g_return_val_if_fail (err != NULL, 1);
		g_warning ("'%s' error: %s", argv[1], err->message);
		g_error_free (err);
		return -1;
  }

	// create an gsf input file
	infile = gsf_infile_zip_new (input, &err);
	if (infile == NULL) {
	  g_return_val_if_fail (err != NULL, 1);
	  g_warning ("'%s' Not a Zip file: %s", argv[1], err->message);
	  g_error_free (err);
	  return -1;
	}
	// print out file informations
	printf ("'%s' size: %" GSF_OFF_T_FORMAT "\n", gsf_input_name (GSF_INPUT (input)),
		gsf_input_size (GSF_INPUT (input)));
	
	// check if the input has content
	gboolean hasContent = (gsf_infile_num_children (GSF_INFILE (infile)) >= 0);
	if (hasContent) {
		printf("file has content ...\n");
		// get file from zip
		GsfInput *theFirstContent = gsf_infile_child_by_index (GSF_INFILE (infile), 0);
		// now print out informations about this content
		printf ("'%s' size: %" GSF_OFF_T_FORMAT "\n", 
						gsf_input_name (GSF_INPUT (theFirstContent)),
						gsf_input_size (GSF_INPUT (theFirstContent)));
	}
	
  g_object_unref (G_OBJECT (infile));
  g_object_unref (G_OBJECT (input));
  return 0;
}

/*
 * In this test implementation we would check, if we can use libgsf to work
 * with buzztard save files.
 *
 * The first example can be used to open a save file and to read the buzztard
 * xml file from.
 */

int main(int argc, char **argv) {
    int returnValue;
	gsf_init();
	returnValue = readBuzztardXmlFile(argc, argv);
	gsf_shutdown();
  	return returnValue;
}
