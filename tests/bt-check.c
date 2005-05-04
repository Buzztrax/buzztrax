/* $Id
 * testing helpes
 */

#include "bt-check.h"

/*
 * error trapping:
 * Check for certain log-output.
 */

gboolean __check_error_trapped=FALSE;
gchar *__check_method=NULL;
gchar *__check_test=NULL;

// is set during setup_log_capture()
gchar *__log_file_name=NULL;

void check_init_error_trapp(gchar *method, gchar *test) {
  __check_method=method;
  __check_test=test;
  g_log_set_always_fatal(G_LOG_LEVEL_ERROR);
}

gboolean check_has_error_trapped(void) {
  g_log_set_always_fatal(G_LOG_LEVEL_WARNING);
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
    msg=g_strdup_printf("%s-%s %s",log_domain,level,message);
    check_print_handler(msg);
    g_free(msg);
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
	// no ansi color codes in logfiles please
	gst_debug_set_colored(FALSE);
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
 * one can use
 * g_object_class_find_property()
 * to get a list of gobject properties.
 * The function can then apply some tests to each of them.
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
	if (check_read && check_write && check_readwrite) {
		ret=TRUE;
	}
	return ret;
}
