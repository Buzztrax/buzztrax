/* $Id: bztloader.c,v 1.1 2005-10-07 13:30:54 waffel Exp $ */

#include "bztloader.h"

int main(int argc, char **argv) {
  GnomeVFSHandle *read_handle;
  const char *input_uri_string = argv[1];
  GnomeVFSFileSize bytes_read;
  guint buffer[BYTES_TO_PROCESS];
  GnomeVFSResult result;
  GnomeVFSURI *input_uri;
  
  /* remember to initialize GnomeVFS! */
  if (!gnome_vfs_init ()) {
    printf ("Could not initialize GnomeVFS\n");
    return 1;
  }
  input_uri = gnome_vfs_uri_new(input_uri_string);
  if (input_uri == NULL) {
    printf ("Error: \"%s\" was a wrong gnome vfs uri\n",input_uri_string);
  }
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
