/* $Id
 * testing helpes
 */

#include "bt-check.h"
#include <sys/types.h>
#include <signal.h>
//-- gtk/gdk
#include <gdk/gdk.h>

/*
 * error trapping:
 * Check for certain log-output.
 */

static gboolean __check_error_trapped=FALSE;
static gchar *__check_method=NULL;
static gchar *__check_test=NULL;
static GLogLevelFlags __fatal_mask=0;

// is set during setup_log_capture()
static gchar *__log_file_name=NULL;

void check_init_error_trapp(gchar *method, gchar *test) {
  __check_method=method;
  __check_test=test;
  __fatal_mask=g_log_set_always_fatal(G_LOG_FATAL_MASK);
}

gboolean check_has_error_trapped(void) {
  g_log_set_always_fatal(__fatal_mask);
  g_log_set_always_fatal(G_LOG_LEVEL_WARNING|G_LOG_LEVEL_CRITICAL|G_LOG_LEVEL_ERROR);
  return(__check_error_trapped);
}


/*
 * log helper:
 * Helpers for handling glib log-messages.
 */

static void check_print_handler(const gchar * const message) {
  if(message) {
    FILE *logfile;
    gboolean add_nl=FALSE;
    guint sl=strlen(message);
    //-- check if messages has no newline 
    if((sl>1) && (message[sl-1]!='\n')) add_nl=TRUE;
  
    //-- check message contents
    //__check_error_trapped=(strstr(message,"assertion failed:")!=NULL);
    __check_error_trapped=TRUE;
    if(__check_method) __check_error_trapped&=(strstr(message,__check_method)!=NULL);
    if(__check_test) __check_error_trapped&=(strstr(message,__check_test)!=NULL);

    if((logfile=fopen(__log_file_name, "a")) || (logfile=fopen(__log_file_name, "w"))) {
      (void)fwrite(message,strlen(message),1,logfile);
      if(add_nl) (void)fwrite("\n",1,1,logfile);
      fclose(logfile);
    }
    else { /* Fall back to console output if unable to open file */
      printf("%s",message);
      if(add_nl) putchar('\n');
    }
  }
}

static void check_log_handler(const gchar * const log_domain, const GLogLevelFlags log_level, const gchar * const message, gpointer const user_data) {
    gchar *msg,*level;

    switch(log_level&G_LOG_LEVEL_MASK) {
      case G_LOG_LEVEL_ERROR:     level="ERROR";break;
      case G_LOG_LEVEL_CRITICAL:  level="CRITICAL";break;
      case G_LOG_LEVEL_WARNING:   level="WARNING";break;
      case G_LOG_LEVEL_MESSAGE:   level="MESSAGE";break;
      case G_LOG_LEVEL_INFO:      level="INFO";break;
      case G_LOG_LEVEL_DEBUG:     level="DEBUG";break;
      default:                    level="???";break;
    }

    msg=g_alloca(strlen(log_domain)+strlen(level)+strlen(message)+3);
    g_sprintf(msg,"%s-%s %s",log_domain,level,message);
    check_print_handler(msg);
}

/**
 * setup_log:
 * @argc: command line argument count received in main()
 * @argv: command line arguments received in main()
 *
 * Initializes the logoutput channel.
 */
void setup_log(int argc, char **argv) {
  gchar *basename,*str;
  
  __log_file_name="/tmp/buzztard.log";
  // get basename from argv[0]; -> lt-bt_edit
  if((str=g_path_get_basename(argv[0]))) {
    //fprintf(stderr,"name : '%s'\n",str);fflush(stderr);
    if((basename=g_strdup_printf("%s.log",&str[3]))) {
      //fprintf(stderr,"basename : '%s'\n",basename);fflush(stderr);
      // build path rooted in tmpdir (this is a tiny memleak, as we never free __log_file_name)
      if(!(__log_file_name=g_build_filename(g_get_tmp_dir(),basename,NULL))) {
        fprintf(stderr,"can't build logname from '%s','%s'\n",g_get_tmp_dir(),basename);fflush(stderr);
        __log_file_name="/tmp/buzztard.log";
      }
      //fprintf(stderr,"logfilename : '%s'\n",__log_file_name);fflush(stderr);
      g_free(basename);
    }
    else {
      fprintf(stderr,"can't build basename from: '%s'\n",str);fflush(stderr);
    }
    g_free(str);
  }
  else {
    fprintf(stderr,"can't get basename from: '%s'\n",argv[0]);fflush(stderr);
  }
  // reset logfile
  unlink(__log_file_name);
}

/**
 * setup_log_capture:
 *
 * Installs own logging handlers to capture and channelize all diagnostic output
 * during testing. In case of probelms that can help to locate the errors.
 */
void setup_log_capture(void) {
#ifdef HAVE_GLIB_2_6
  (void)g_log_set_default_handler(check_log_handler,NULL);
#else
  (void)g_log_set_handler(G_LOG_DOMAIN  ,G_LOG_LEVEL_DEBUG|G_LOG_LEVEL_INFO|G_LOG_LEVEL_MESSAGE|G_LOG_LEVEL_WARNING|G_LOG_LEVEL_CRITICAL|G_LOG_LEVEL_ERROR|G_LOG_FLAG_FATAL|G_LOG_FLAG_RECURSION, check_log_handler, NULL);
  (void)g_log_set_handler("buzztard"    ,G_LOG_LEVEL_DEBUG|G_LOG_LEVEL_INFO|G_LOG_LEVEL_MESSAGE|G_LOG_LEVEL_WARNING|G_LOG_LEVEL_CRITICAL|G_LOG_LEVEL_ERROR|G_LOG_FLAG_FATAL|G_LOG_FLAG_RECURSION, check_log_handler, NULL);
  (void)g_log_set_handler("GStreamer"   ,G_LOG_LEVEL_DEBUG|G_LOG_LEVEL_INFO|G_LOG_LEVEL_MESSAGE|G_LOG_LEVEL_WARNING|G_LOG_LEVEL_CRITICAL|G_LOG_LEVEL_ERROR|G_LOG_FLAG_FATAL|G_LOG_FLAG_RECURSION, check_log_handler, NULL);
  (void)g_log_set_handler("GLib"        ,G_LOG_LEVEL_DEBUG|G_LOG_LEVEL_INFO|G_LOG_LEVEL_MESSAGE|G_LOG_LEVEL_WARNING|G_LOG_LEVEL_CRITICAL|G_LOG_LEVEL_ERROR|G_LOG_FLAG_FATAL|G_LOG_FLAG_RECURSION, check_log_handler, NULL);
  (void)g_log_set_handler("GLib-GObject",G_LOG_LEVEL_DEBUG|G_LOG_LEVEL_INFO|G_LOG_LEVEL_MESSAGE|G_LOG_LEVEL_WARNING|G_LOG_LEVEL_CRITICAL|G_LOG_LEVEL_ERROR|G_LOG_FLAG_FATAL|G_LOG_FLAG_RECURSION, check_log_handler, NULL);
  (void)g_log_set_handler(NULL          ,G_LOG_LEVEL_DEBUG|G_LOG_LEVEL_INFO|G_LOG_LEVEL_MESSAGE|G_LOG_LEVEL_WARNING|G_LOG_LEVEL_CRITICAL|G_LOG_LEVEL_ERROR|G_LOG_FLAG_FATAL|G_LOG_FLAG_RECURSION, check_log_handler, NULL);
#endif
  (void)g_set_printerr_handler(check_print_handler);
}


/*
 * file output tests:
 * Check if certain text has been outputted to a logfile.
 */

gboolean file_contains_str(gchar *tmp_file_name, gchar *str) {
  FILE *input_file;
  gchar read_str[1024];
  gboolean ret=FALSE;
  
  g_assert(tmp_file_name);
  g_assert(str);
  
  input_file=fopen(tmp_file_name,"rb");
  if (!input_file) {
    return ret;
  }
  while (!feof(input_file)) {
    if (!fgets(read_str, 1023, input_file)) {
      break;
    }
    read_str[1023]='\0';
    if (strstr(read_str, str)) {
      ret=TRUE;
      break;
    }
  }
  fclose(input_file);
  return ret;
}

// test plugins

void check_register_plugins(void) {
  // this is a hack to persuade the linker to not optimize out these :(
  if(!bt_test_mono_source_get_type()) g_print("registering mono-src faild");
}

// property tests

static gboolean check_read_int_param(GParamSpec *paramspec, GObject *toCheck) {
  gint check;
  gboolean ret=FALSE;
  
  g_object_get(toCheck,paramspec->name,&check,NULL);
  if ((check >= G_PARAM_SPEC_INT(paramspec)->minimum) && 
      (check <= G_PARAM_SPEC_INT(paramspec)->maximum)) {
    ret=TRUE;
  }
  else {
    GST_WARNING("property read range check failed for : %s : %d <= %d <= %d",
      paramspec->name,
      G_PARAM_SPEC_INT(paramspec)->minimum,
      check,
      G_PARAM_SPEC_INT(paramspec)->maximum
    );
  }
  return ret;
}

static gboolean check_read_uint_param(GParamSpec *paramspec, GObject *toCheck) {
  guint check;
  gboolean ret=FALSE;
  
  g_object_get(toCheck,paramspec->name,&check,NULL);
  if ((check >= G_PARAM_SPEC_UINT(paramspec)->minimum) && 
      (check <= G_PARAM_SPEC_UINT(paramspec)->maximum)) {
    ret=TRUE;
  }
  else {
    GST_WARNING("property read range check failed for : %s : %u <= %u <= %u",
      paramspec->name,
      G_PARAM_SPEC_UINT(paramspec)->minimum,
      check,
      G_PARAM_SPEC_UINT(paramspec)->maximum
    );
  }
  return ret;
}

static gboolean check_read_int64_param(GParamSpec *paramspec, GObject *toCheck) {
  gint64 check;
  gboolean ret=FALSE;
  
  g_object_get(toCheck,paramspec->name,&check,NULL);
  if ((check >= G_PARAM_SPEC_INT64(paramspec)->minimum) && 
      (check <= G_PARAM_SPEC_INT64(paramspec)->maximum)) {
    ret=TRUE;
  }
  else {
    GST_WARNING("property read range check failed for : %s",paramspec->name);
  }
  return ret;
}

static gboolean check_read_long_param(GParamSpec *paramspec, GObject *toCheck) {
  glong check;
  gboolean ret=FALSE;
  
  g_object_get(toCheck,paramspec->name,&check,NULL);
  if ((check >= G_PARAM_SPEC_LONG(paramspec)->minimum) && 
      (check <= G_PARAM_SPEC_LONG(paramspec)->maximum)) {
    ret=TRUE;
  }
  else {
    GST_WARNING("property read range check failed for : %s : %ld <= %ld <= %ld",
      paramspec->name,
      G_PARAM_SPEC_LONG(paramspec)->minimum,
      check,
      G_PARAM_SPEC_LONG(paramspec)->maximum
    );
  }
  return ret;
}

static gboolean check_read_ulong_param(GParamSpec *paramspec, GObject *toCheck) {
  gulong check;
  gboolean ret=FALSE;

  g_object_get(toCheck,paramspec->name,&check,NULL);
  if ((check >= G_PARAM_SPEC_ULONG(paramspec)->minimum) && 
      (check <= G_PARAM_SPEC_ULONG(paramspec)->maximum)) {
    ret=TRUE;
  }
  else {
    GST_WARNING("property read range check failed for : %s : %lu <= %lu <= %lu",
      paramspec->name,
      G_PARAM_SPEC_ULONG(paramspec)->minimum,
      check,
      G_PARAM_SPEC_ULONG(paramspec)->maximum
    );
  }
  return ret;
}

static gboolean check_read_property(GParamSpec *paramspec, GObject *toCheck) {
  GType param_type;
  gboolean ret=FALSE;
  
  param_type=G_PARAM_SPEC_TYPE(paramspec);
  if(param_type == G_TYPE_PARAM_INT) {
    ret=check_read_int_param(paramspec, toCheck);
  } else if (param_type == G_TYPE_PARAM_UINT) {
    ret=check_read_uint_param(paramspec, toCheck);
  } else if (param_type == G_TYPE_PARAM_INT64) {
    ret=check_read_int64_param(paramspec, toCheck);
  } else if (param_type == G_TYPE_PARAM_LONG) {
    ret=check_read_long_param(paramspec, toCheck);
  } else if (param_type == G_TYPE_PARAM_ULONG) {
    ret=check_read_ulong_param(paramspec, toCheck); 
  } else { // no check performed
    ret=TRUE;
  }
  return ret;
}

static gboolean check_write_int_param(GParamSpec *paramspec, GObject *toCheck) {
  gint check1, check2;

  check1=G_PARAM_SPEC_INT(paramspec)->minimum;
  g_object_set(toCheck,paramspec->name,check1,NULL);
  
  check2=G_PARAM_SPEC_INT(paramspec)->maximum;
  g_object_set(toCheck,paramspec->name,check2,NULL);
  
  return TRUE;
}

static gboolean check_write_uint_param(GParamSpec *paramspec, GObject *toCheck) {
  guint check1, check2;

  check1=G_PARAM_SPEC_UINT(paramspec)->minimum;
  g_object_set(toCheck,paramspec->name,check1,NULL);
  
  check2=G_PARAM_SPEC_UINT(paramspec)->maximum;
  g_object_set(toCheck,paramspec->name,check2,NULL);
  
  return TRUE;
}

static gboolean check_write_int64_param(GParamSpec *paramspec, GObject *toCheck) {
  gint64 check1, check2;

  check1=G_PARAM_SPEC_INT64(paramspec)->minimum;
  g_object_set(toCheck,paramspec->name,check1,NULL);
  
  check2=G_PARAM_SPEC_INT64(paramspec)->maximum;
  g_object_set(toCheck,paramspec->name,check2,NULL);
  
  return TRUE;
}

static gboolean check_write_long_param(GParamSpec *paramspec, GObject *toCheck) {
  glong check1, check2;

  check1=G_PARAM_SPEC_LONG(paramspec)->minimum;
  g_object_set(toCheck,paramspec->name,check1,NULL);
  
  check2=G_PARAM_SPEC_LONG(paramspec)->maximum;
  g_object_set(toCheck,paramspec->name,check2,NULL);
  
  return TRUE;
}

static gboolean check_write_ulong_param(GParamSpec *paramspec, GObject *toCheck) {
  gulong check1, check2;

  check1=G_PARAM_SPEC_ULONG(paramspec)->minimum;
  g_object_set(toCheck,paramspec->name,check1,NULL);
  
  check2=G_PARAM_SPEC_ULONG(paramspec)->maximum;
  g_object_set(toCheck,paramspec->name,check2,NULL);
  
  return TRUE;
}

static gboolean check_write_property(GParamSpec *paramspec, GObject *toCheck) {
  GType param_type;
  gboolean ret=FALSE;
  
  param_type=G_PARAM_SPEC_TYPE(paramspec);
  if(param_type == G_TYPE_PARAM_INT) {
    ret=check_write_int_param(paramspec, toCheck);
  } else if (param_type == G_TYPE_PARAM_UINT) {
    ret=check_write_uint_param(paramspec, toCheck);
  } else if (param_type == G_TYPE_PARAM_INT64) {
    ret=check_write_int64_param(paramspec, toCheck);
  } else if (param_type == G_TYPE_PARAM_LONG) {
    ret=check_write_long_param(paramspec, toCheck);
  } else if (param_type == G_TYPE_PARAM_ULONG) {
    ret=check_write_ulong_param(paramspec, toCheck); 
  } else { // no check performed
    ret=TRUE;
  }
  return ret;
}

static gboolean check_readwrite_int_param(GParamSpec *paramspec, GObject *toCheck) {
  gulong ival,oval;
  gboolean ret=TRUE;

  ival=G_PARAM_SPEC_INT(paramspec)->minimum;
  g_object_set(toCheck,paramspec->name, ival,NULL);
  g_object_get(toCheck,paramspec->name,&oval,NULL);
  if(ival!=oval) {
    GST_WARNING("property read/write minimum check failed for : %s : %d != %d",
      paramspec->name,
      ival,oval
    );
    ret=FALSE;
  }
  /*
  ival=G_PARAM_SPEC_INT(paramspec)->maximum;
  g_object_set(toCheck,paramspec->name, ival,NULL);
  g_object_get(toCheck,paramspec->name,&oval,NULL);
  if(ival!=oval) {
    GST_WARNING("property read/write maximum check failed for : %s : %d != %d",
      paramspec->name,
      ival,oval
    );
    ret=FALSE;
  }
  */
  return ret;
}

static gboolean check_readwrite_uint_param(GParamSpec *paramspec, GObject *toCheck) {
  gulong ival,oval;
  gboolean ret=TRUE;

  ival=G_PARAM_SPEC_UINT(paramspec)->minimum;
  g_object_set(toCheck,paramspec->name, ival,NULL);
  g_object_get(toCheck,paramspec->name,&oval,NULL);
  if(ival!=oval) {
    GST_WARNING("property read/write minimum check failed for : %s : %u != %u",
      paramspec->name,
      ival,oval
    );
    ret=FALSE;
  }
  /*
  ival=G_PARAM_SPEC_UINT(paramspec)->maximum;
  g_object_set(toCheck,paramspec->name, ival,NULL);
  g_object_get(toCheck,paramspec->name,&oval,NULL);
  if(ival!=oval) {
    GST_WARNING("property read/write maximum check failed for : %s : %u != %u",
      paramspec->name,
      ival,oval
    );
    ret=FALSE;
  }
  */
  return ret;
}

static gboolean check_readwrite_int64_param(GParamSpec *paramspec, GObject *toCheck) {
  gulong ival,oval;
  gboolean ret=TRUE;

  ival=G_PARAM_SPEC_INT64(paramspec)->minimum;
  g_object_set(toCheck,paramspec->name, ival,NULL);
  g_object_get(toCheck,paramspec->name,&oval,NULL);
  if(ival!=oval) {
    GST_WARNING("property read/write minimum check failed for : %s",paramspec->name);
    ret=FALSE;
  }
  /*
  ival=G_PARAM_SPEC_INT64(paramspec)->maximum;
  g_object_set(toCheck,paramspec->name, ival,NULL);
  g_object_get(toCheck,paramspec->name,&oval,NULL);
  if(ival!=oval) {
    GST_WARNING("property read/write maxmum check failed for : %s",paramspec->name);
    ret=FALSE;
  }
  */
  return ret;
}

static gboolean check_readwrite_long_param(GParamSpec *paramspec, GObject *toCheck) {
  gulong ival,oval;
  gboolean ret=TRUE;

  ival=G_PARAM_SPEC_LONG(paramspec)->default_value;
  g_object_set(toCheck,paramspec->name, ival,NULL);
  g_object_get(toCheck,paramspec->name,&oval,NULL);
  if(ival!=oval) {
    GST_WARNING("property read/write default_value check failed for : %s : %ld != %ld",
      paramspec->name,
      ival,oval
    );
    ret=FALSE;
  }
  /*
  ival=G_PARAM_SPEC_LONG(paramspec)->maximum;
  g_object_set(toCheck,paramspec->name, ival,NULL);
  g_object_get(toCheck,paramspec->name,&oval,NULL);
  if(ival!=oval) {
    GST_WARNING("property read/write maxmum check failed for : %s : %ld != %ld",
      paramspec->name,
      ival,oval
    );
    ret=FALSE;
  }
  */
  return ret;
}

static gboolean check_readwrite_ulong_param(GParamSpec *paramspec, GObject *toCheck) {
  gulong ival,oval;
  gboolean ret=TRUE;

  ival=G_PARAM_SPEC_ULONG(paramspec)->default_value;
  g_object_set(toCheck,paramspec->name, ival,NULL);
  g_object_get(toCheck,paramspec->name,&oval,NULL);
  if(ival!=oval) {
    GST_WARNING("property read/write default_value check failed for : %s : %lu != %lu",
      paramspec->name,
      ival,oval
    );
    ret=FALSE;
  }
  /*
  ival=G_PARAM_SPEC_ULONG(paramspec)->maximum;
  g_object_set(toCheck,paramspec->name, ival,NULL);
  g_object_get(toCheck,paramspec->name,&oval,NULL);
  if(ival!=oval) {
    GST_WARNING("property read/write maxmum check failed for : %s : %lu != %lu",
      paramspec->name,
      ival,oval
    );
    ret=FALSE;
  }
  */
  return ret;
}

static gboolean check_readwrite_property(GParamSpec *paramspec, GObject *toCheck) {  
  GType param_type;
  gboolean ret=FALSE;
  
  param_type=G_PARAM_SPEC_TYPE(paramspec);
  if(param_type == G_TYPE_PARAM_INT) {
    ret=check_readwrite_int_param(paramspec, toCheck);
  } else if (param_type == G_TYPE_PARAM_UINT) {
    ret=check_readwrite_uint_param(paramspec, toCheck);
  } else if (param_type == G_TYPE_PARAM_INT64) {
    ret=check_readwrite_int64_param(paramspec, toCheck);
  } else if (param_type == G_TYPE_PARAM_LONG) {
    ret=check_readwrite_long_param(paramspec, toCheck);
  } else if (param_type == G_TYPE_PARAM_ULONG) {
    ret=check_readwrite_ulong_param(paramspec, toCheck); 
  } else { // no check performed
    ret=TRUE;
  }
  return ret;
}

/** test gobject properties
 * @toCheck: the object to examine
 *
 * The function runs tests against all registered properties of the given
 * object. Depending on the flags and the type of the properties, tests include
 * read/write and boundary checks.
 */

gboolean check_gobject_properties(GObject *toCheck) {
  gboolean ret=FALSE;
  gboolean check_read=TRUE;
  gboolean check_write=TRUE;
  gboolean check_readwrite=TRUE;
  guint n_properties;
  guint loop;
  GParamSpec **return_params;
  
  return_params=g_object_class_list_properties(G_OBJECT_GET_CLASS(toCheck),&n_properties);
  // iterate over properties
  for (loop=0; loop<n_properties; loop++) {
    GParamSpec *paramspec=return_params[loop];
    
    GST_DEBUG("property check for : %s",paramspec->name);
    if (paramspec->flags&G_PARAM_READABLE) {
      if(!(check_read=check_read_property(paramspec, toCheck))) {
        GST_WARNING("property read check failed for : %s",paramspec->name);
      }
    } 
    if (paramspec->flags&G_PARAM_WRITABLE) {
      if(!(check_write=check_write_property(paramspec, toCheck))) {
        GST_WARNING("property write check failed for : %s",paramspec->name);
      }
    }
    if (paramspec->flags&G_PARAM_READWRITE) {
      if(!(check_readwrite=check_readwrite_property(paramspec, toCheck))) {
        GST_WARNING("property read/write check failed for : %s",paramspec->name);
      }
    }
  }
  g_free(return_params);
  if (check_read && check_write && check_readwrite) {
    ret=TRUE;
  }
  return ret;
}

// gtk+ gui tests

static GPid server_pid;
static GdkDisplayManager *display_manager=NULL;
static GdkDisplay *default_display=NULL,*test_display=NULL;
static volatile gboolean wait_for_server;
static gchar display_name[3];
static gint display_number;

static void __test_server_watch(GPid pid,gint status,gpointer data) {
  if(status==0) {
    GST_INFO("test x server %d process finished okay",pid);
  }
  else {
    GST_WARNING("test x server %d process finished with error %d",pid,status);
  }
  wait_for_server=FALSE;
  g_spawn_close_pid(pid);
  server_pid=0;
}

void check_setup_test_server(void) {
#ifdef HAVE_XVFB
  //gulong flags=G_SPAWN_SEARCH_PATH|G_SPAWN_STDOUT_TO_DEV_NULL|G_SPAWN_STDERR_TO_DEV_NULL;
  gulong flags=G_SPAWN_SEARCH_PATH;
  GError *error=NULL;
  gchar display_file[18];
  gchar *argv[]={
    "Xvfb",
    ":9",
    "-ac",
    "-nolisten","tcp",
		"-fp","/usr/share/fonts/misc",
    /*"-reset",
    "-terminate",*/
    "-screen","0","1024x786x16",
    NULL
  };
  gboolean found=FALSE,trying=TRUE;

  server_pid=0;
  display_number=0;

  // try display ids starting with '0'
  while(trying) {
    wait_for_server=TRUE;
    g_sprintf(display_name,":%1d",display_number);
    g_sprintf(display_file,"/tmp/.X11-unix/X%1d",display_number);
    
    // this display is not yet in use
    if(!g_file_test(display_file, G_FILE_TEST_EXISTS)) {
      // create the testing server
      argv[1]=display_name;
      if(!(g_spawn_async(NULL,argv,NULL,flags,NULL,NULL,&server_pid,&error))) {
        GST_ERROR("error creating virtual x-server : \"%s\"", error->message);
        g_error_free(error);
      }
      else {
        g_child_watch_add(server_pid,__test_server_watch,NULL);
  
        while(wait_for_server) {
          // try also waiting for /tmp/X%1d-lock" files
          if(g_file_test(display_file, G_FILE_TEST_EXISTS)) {
            wait_for_server=trying=FALSE;
            found=TRUE;
          }
          else {
            sleep(1);
          }
        }
        //sleep(2);
      }
    }
    if(!found) {
      display_number++;
      // stop after trying the first ten displays
      if(display_number==10) trying=FALSE;
    }
  }
  if(!found) {
    display_number=-1;
  }
	else {
		GST_INFO("test server \"%s\" is up (pid=%d)",display_name,server_pid);
	}
#endif
}

void check_setup_test_display(void) {
#ifdef HAVE_XVFB
  if(display_number>-1) {
    // activate the display for use with gtk
    if((display_manager = gdk_display_manager_get())) {
			default_display = gdk_display_manager_get_default_display(display_manager);
			if((test_display = gdk_display_open(display_name))) {
				gdk_display_manager_set_default_display(display_manager,test_display);
				GST_INFO("display %p,\"%s\" is active",test_display,gdk_display_get_name(test_display));
			}
			else {
				GST_WARNING("failed to open display: \"%s\"",display_name);
			}
		}
		else {
			GST_WARNING("can't get display-manager");
		}
  }
#endif
}

void check_shutdown_test_display(void) {
#ifdef HAVE_XVFB
  if(test_display) {
    wait_for_server=TRUE;

		g_assert(GDK_IS_DISPLAY_MANAGER(display_manager));
    g_assert(GDK_IS_DISPLAY(test_display));
    g_assert(GDK_IS_DISPLAY(default_display));

    GST_INFO("trying to shut down test server %d",server_pid);
    // restore default and close our display
    GST_DEBUG("display_manager=%p, test_display=%p,\"%s\" default_display=%p,\"%s\"",
        display_manager,
        test_display,gdk_display_get_name(test_display),
        default_display,gdk_display_get_name(default_display));
    gdk_display_manager_set_default_display(display_manager,default_display);
    GST_INFO("display has been restored");
		// TODO here it hangs
    //gdk_display_close(test_display);
		/* gdk_display_close does basically (which still hangs):
		//g_object_run_dispose (G_OBJECT (test_display));
		GST_INFO("test_display has been disposed");
    //g_object_unref (test_display);
		*/
    GST_INFO("display has been closed");
    test_display=NULL;
  }
  else {
    GST_WARNING("no test display");
  }
#endif
}

void check_shutdown_test_server(void) {
#ifdef HAVE_XVFB
  if(server_pid) {
		guint wait_count=5;
    wait_for_server=TRUE;
		GST_INFO("shuting down test server");

    // kill the testing server - @todo try other signals (SIGQUIT, SIGTERM).
    kill(server_pid, SIGINT);
    // wait for the server to finish (use waitpid() here ?)
    while(wait_for_server && wait_count) {
      sleep(1);
			wait_count--;
    }
    GST_INFO("test server has been shut down");
  }
  else {
    GST_WARNING("no test server");
  }
#endif
}
