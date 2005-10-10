/* $Id: bztloader.c,v 1.3 2005-10-10 15:00:39 waffel Exp $ */

#include "bztloader.h"

int main(int argc, char **argv) {
  GnomeVFSHandle *read_handle;
  const char *input_uri_string = argv[1];
  GnomeVFSFileSize bytes_read;
  guint buffer[BYTES_TO_PROCESS];
  GnomeVFSResult result;
  GnomeVFSURI *input_uri, *internal_uri;
  
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
  
  input_uri = gnome_vfs_uri_new(input_uri_string);
  
  /* make uri absolute from relative */
  if(gnome_vfs_uri_is_local(input_uri)) {
    /* if the uri exists we can open it.. no need for more tests */
		if(gnome_vfs_uri_exists(input_uri))
		{
       printf("uri seems to be ok\n");
      /* add suffix #gzip:#tar:/song.xml to input string */
      input_uri_string = g_strconcat(input_uri_string,"#gzip:#tar:/song.xml");
      printf("new input string: \"%s\"\n", input_uri_string);
      // create URI from string to check validity
      internal_uri = gnome_vfs_uri_new(input_uri_string);
     } else {
       printf("uri is not absolute\n");
       /* we still need to find out if its a valid path */
			char *dirname =  gnome_vfs_uri_extract_dirname(input_uri);
      if ( (dirname[0]=='.') || (strlen(dirname) == 1 && (dirname[0]=='/')) )
			//if(!strncmp(dirname, ".", 1) || (strlen(dirname) == 1 && !strncmp(dirname, "/", 1)))
			{
				/* there should be default function to get the full path from a relative one  */
				char *cur_dir_tmp = g_get_current_dir();
				char *path = NULL;
				/* grrrrrrrr stupid gnome_vfs needs a ending / in the base path */
				char *cur_dir = g_strdup_printf("%s/", cur_dir_tmp);
        printf("cur_dir %s\n",cur_dir);
        fflush(stdout);
				g_free(cur_dir_tmp);
				path =  gnome_vfs_uri_make_full_from_relative(cur_dir,input_uri_string);
				printf("new path: \"%s\"\n",path);
        fflush(stdout);
        /* add suffix #gzip:#tar:/song.xml to input string */
        char* new_input_uri_string = g_strconcat(path,"#gzip:#tar:/song.xml");
        printf("new input string: \"%s\"\n", new_input_uri_string);
        fflush(stdout);
        // create URI from string to check validity
        internal_uri = gnome_vfs_uri_new(new_input_uri_string);
				g_free(path);		
				g_free(cur_dir);
			}
			g_free(dirname);
		}
  }
  
  if (internal_uri == NULL) {
    printf("wrong uri\n");
  }
  /* check validity ... this crashes gnome vfs */
  /* TODO make bug report */
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
