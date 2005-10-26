/* $Id: bztloader.c,v 1.8 2005-10-26 14:41:43 waffel Exp $ */

#include "bztloader.h"

static int
print_error (GnomeVFSResult result, const char *uri_string)
{
  const char *error_string;
  /* get the string corresponding to this GnomeVFSResult value */
  error_string = gnome_vfs_result_to_string (result);
  printf ("Error %s occured opening location %s\n", error_string, uri_string);
  return 1;
}

int main(int argc, char **argv) {
  GnomeVFSHandle *read_handle;
  const char *input_uri_string = argv[1];
  char *absolute_uri_string;
  GnomeVFSFileSize bytes_read;
  guint buffer[BYTES_TO_PROCESS];
  GnomeVFSResult result;
  GnomeVFSURI *input_uri, *internal_uri;
  GnomeVFSFileInfo *file_info;
  gboolean is_xml_mime=FALSE;
  
  /* remember to initialize GNOME VFS */
  if (!gnome_vfs_init ()) {
    printf ("Could not initialize GnomeVFS\n");
    return 1;
  }
  
  /* check if an argument was given */
  if (input_uri_string == NULL) {
    printf ("Error: \"%s\" was a wrong gnome vfs uri\n",input_uri_string);
    return 1;
  }

  /* creating a absolute uri string from the given input string. Works also if 
     the given string was a absolute uri. */
  absolute_uri_string = gnome_vfs_make_uri_from_input_with_dirs (input_uri_string, GNOME_VFS_MAKE_URI_DIR_CURRENT);
  printf("absolute uri: %s\n",absolute_uri_string);
  /* creating the gnome-vfs uri from the absolute path string */
  input_uri = gnome_vfs_uri_new(absolute_uri_string);
  
  /* check if the input is ok */
  if (input_uri == NULL) {
    printf ("Error: wrong gnome vfs uri\n");
    return 1;
  }  
  
  /* check if the given uri exists */
  if (!gnome_vfs_uri_exists(input_uri)) {
    printf("file doe's not exists ... stopping\n");
    return -1;
  }  
  
  printf("uri seems to be ok\n");
  
  file_info = gnome_vfs_file_info_new ();
  /* now we check the mime type */
  result = gnome_vfs_get_file_info_uri(input_uri,file_info,GNOME_VFS_FILE_INFO_GET_MIME_TYPE);
  /* if the operation was not successful, print the error and abort */
  if (result != GNOME_VFS_OK) {
   return print_error (result, gnome_vfs_uri_to_string(input_uri, GNOME_VFS_URI_HIDE_NONE));
  }
  
  printf("mime type: %s\n", gnome_vfs_file_info_get_mime_type(file_info));
  
  if (g_ascii_strcasecmp("text/xml",gnome_vfs_file_info_get_mime_type(file_info)) == 0) {
    is_xml_mime=TRUE;
  }
  gnome_vfs_file_info_unref (file_info);
  /* now we can free the input uri ... was just used to check if the given file
     exists */
  g_free(input_uri);
  
  // the the mime type of the file is not an text/xml mimetype we think it is an
  // bzt file and expanding the uri
  if (!is_xml_mime) {
    /* add suffix #gzip:#tar:/song.xml to input string */
    input_uri_string = g_strconcat(absolute_uri_string,"#gzip:#tar:/song.xml",NULL);
  } else {
    input_uri_string = absolute_uri_string;
  }
  
  printf("new input string: \"%s\"\n", input_uri_string);
  // create URI from string to check validity
  internal_uri = gnome_vfs_uri_new(input_uri_string);
  
  if (internal_uri == NULL) {
    printf("Error: wrong uri\n");
    return 1;
  }
  /* check if the new internal uri have a song.xml file */
  if (!gnome_vfs_uri_exists(internal_uri)) {
    printf("input uri doe's not exists\n");
    return 1;
  }
  
  printf("used uri in gnome_vfs: \"%s\"\n", gnome_vfs_uri_to_string(internal_uri, GNOME_VFS_URI_HIDE_NONE));
  /* open the input file for read access */
  result = gnome_vfs_open_uri (&read_handle, internal_uri, GNOME_VFS_OPEN_READ);
  /* if the operation was not successful, print the error and abort */
  if (result != GNOME_VFS_OK) {
    return print_error (result, gnome_vfs_uri_to_string(internal_uri, GNOME_VFS_URI_HIDE_NONE));
  }
  
  while (result == GNOME_VFS_OK) {
    /* read data from the input uri */
    result = gnome_vfs_read (read_handle, buffer, BYTES_TO_PROCESS-1, &bytes_read);
    buffer[bytes_read] = 0;
    /* write out the file entry */
    write(1, buffer, bytes_read);
    if (bytes_read == 0) {
      break;
    }
  }
  
  if (result != GNOME_VFS_OK) {
    return print_error (result, gnome_vfs_uri_to_string(internal_uri, GNOME_VFS_URI_HIDE_NONE));
  }
  gnome_vfs_shutdown();
  printf("wow! it seems to work\n");
  return 0;
}
