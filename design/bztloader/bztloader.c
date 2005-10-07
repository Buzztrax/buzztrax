/* $Id: bztloader.c,v 1.2 2005-10-07 15:04:28 waffel Exp $ */

#include "bztloader.h"

int main(int argc, char **argv) {
  GnomeVFSHandle *read_handle;
  const char *input_uri_string = argv[1];
  GnomeVFSFileSize bytes_read;
  guint buffer[BYTES_TO_PROCESS];
  GnomeVFSResult result;
  GnomeVFSURI *input_uri, *internal_uri;
  
  /* remember to initialize GnomeVFS! */
  if (!gnome_vfs_init ()) {
    printf ("Could not initialize GnomeVFS\n");
    return 1;
  }
  
  // create URI from string to check validity
  input_uri = gnome_vfs_uri_new(input_uri_string);
  if (input_uri == NULL) {
    printf ("Error: \"%s\" was a wrong gnome vfs uri\n",input_uri_string);
  }
  g_free(input_uri);
  // add #extfs:song.xml to uri and recreate the input uri 
  internal_uri= gnome_vfs_uri_new(g_strconcat(input_uri_string,"#gzip:song.xml"));
  printf ("expanded uri: \"%s\"\n", gnome_vfs_uri_to_string(internal_uri, GNOME_VFS_URI_HIDE_NONE));
  
  if (!gnome_vfs_uri_exists(internal_uri)) {
    printf("input uri doe's not exists\n");
    return 1;
  }
  /* open the input file for read access */
  result = gnome_vfs_open_uri (&read_handle, internal_uri, GNOME_VFS_OPEN_READ);
  /* if the operation was not successful, print the error and abort */
  if (result != GNOME_VFS_OK) {
    return print_error (result, gnome_vfs_uri_to_string(internal_uri, GNOME_VFS_URI_HIDE_NONE));
  }
  
  printf("wow! it seems to work\n");
  return 0;
}

int
print_error (GnomeVFSResult result, const char *uri_string)
{
  const char *error_string;
  /* get the string corresponding to this GnomeVFSResult value */
  error_string = gnome_vfs_result_to_string (result);
  printf ("Error %s occured opening location %s\n", error_string, uri_string);
  return 1;
}
