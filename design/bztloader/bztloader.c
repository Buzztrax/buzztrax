/* libgsf test */

#include <gsf/gsf-input-stdio.h>
#include <gsf/gsf-utils.h>
#include <gsf/gsf-infile.h>
#include <gsf/gsf-infile-zip.h>

/*
 * try to read the given input file, open it an read the main buzztrax song xml
 * file. Currently we searching hard for a file named "song.xml".
 */
int
readBuzztraxXmlFile (gchar * file_name)
{
  GsfInput *input;
  GsfInfile *infile;
  GError *err = NULL;

  // open the file from the first argument
  input = gsf_input_stdio_new (file_name, &err);
  if (!input) {
    g_warning ("'%s' error: %s", file_name, err->message);
    g_error_free (err);
    return -1;
  }
  // create an gsf input file
  infile = gsf_infile_zip_new (input, &err);
  if (!infile) {
    g_warning ("'%s' is not a zip file: %s", file_name, err->message);
    g_error_free (err);
    return -1;
  }
  // print out file informations
  printf ("'%s' size: %" GSF_OFF_T_FORMAT "\n",
      gsf_input_name (input), gsf_input_size (input));

  // check if the input has content
  if (gsf_infile_num_children (infile) >= 0) {
    printf ("file has content ...\n");
    // get file from zip
    GsfInput *data = gsf_infile_child_by_index (infile, 0);
    //GsfInput *data=gsf_infile_child_by_name(infile,"song.xml");

    if (data) {
      const guint8 *bytes;
      size_t len = (size_t) gsf_input_size (data);

      // now print out informations about this content
      printf ("'%s' size: %" G_GSIZE_FORMAT "\n", gsf_input_name (data), len);

      bytes = gsf_input_read (data, len, NULL);
      if (bytes) {
        fwrite (bytes, len, 1, stdout);
        puts ("");
      }
    }
    g_object_unref (G_OBJECT (data));
  }

  g_object_unref (G_OBJECT (infile));
  g_object_unref (G_OBJECT (input));
  return 0;
}

/*
 * check if we can use libgsf to work with buzztrax song files.
 *
 * The first example can be used to open a save file and to read the buzztrax
 * xml file from.
 */

int
main (int argc, char **argv)
{
  int returnValue = 1;

  if (argc > 1) {
    gsf_init ();
    returnValue = readBuzztraxXmlFile (argv[1]);
    gsf_shutdown ();
  } else {
    printf ("Usage: %s <filename>\n", argv[0]);
  }
  return returnValue;
}
