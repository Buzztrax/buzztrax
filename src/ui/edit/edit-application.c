/* $Id: edit-application.c,v 1.4 2004-08-06 19:42:45 ensonic Exp $
 * class for a gtk based buzztard editor application
 */
 
#define BT_EDIT
#define BT_EDIT_APPLICATION_C

#include "bt-edit.h"

// this needs to be here because of gtk-doc and unit-tests
GST_DEBUG_CATEGORY(GST_CAT_DEFAULT);

struct _BtEditApplicationPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* the currently loaded song */
  BtSong *song;
  /* the top-level window of our app */
  BtMainWindow *main_window;
};

//-- helper methods

static gboolean bt_edit_application_ui(const BtEditApplication *self) {
  gboolean res=FALSE;
  self->private->main_window=bt_main_window_new(self);
  
  if(self->private->main_window) {
    res=bt_main_window_show_and_run(self->private->main_window);
  }
  return(res);
}

//-- constructor methods

/**
 * bt_edit_application_new:
 *
 * Create a new instance
 *
 * Return: the new instance or NULL in case of an error
 */
BtEditApplication *bt_edit_application_new(void) {
  BtEditApplication *self;
  self=BT_EDIT_APPLICATION(g_object_new(BT_TYPE_EDIT_APPLICATION,NULL));
  
  return(self);
}

//-- methods

/**
 * bt_edit_application_run:
 * @self: the application instance to run
 *
 * start the gtk based editor application
 *
 * Returns: true for success
 */
gboolean bt_edit_application_run(const BtEditApplication *self) {
	gboolean res=FALSE;

	GST_INFO("application.play launched");

	self->private->song=bt_song_new_with_name(GST_BIN(bt_g_object_get_object_property(G_OBJECT(self),"bin")),"empty song");
	
	GST_INFO("objects initialized");
  
  res=bt_edit_application_ui(self);
	return(res);
}

/**
 * bt_edit_application_load_and_run:
 * @self: the application instance to run
 * @input_file_name: the file to load initially
 *
 * load the file of the supplied name and start the gtk based editor application
 *
 * Returns: true for success
 */
gboolean bt_edit_application_load_and_run(const BtEditApplication *self, const gchar *input_file_name) {
	gboolean res=FALSE;
	BtSong *song;
	BtSongIO *loader;

	GST_INFO("application.info launched");

	self->private->song=bt_song_new(GST_BIN(bt_g_object_get_object_property(G_OBJECT(self),"bin")));
	loader=bt_song_io_new(input_file_name);
	
	GST_INFO("objects initialized");
	
	if(bt_song_io_load(loader,self->private->song,input_file_name)) {
		res=bt_edit_application_ui(self);
	}
	else {
		GST_ERROR("could not load song \"%s\"",input_file_name);
	}
	return(res);
}

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_edit_application_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtEditApplication *self = BT_EDIT_APPLICATION(object);
  return_if_disposed();
  switch (property_id) {
    default: {
 			g_assert(FALSE);
      break;
    }
  }
}

/* sets the given properties for this object */
static void bt_edit_application_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtEditApplication *self = BT_EDIT_APPLICATION(object);
  return_if_disposed();
  switch (property_id) {
    default: {
			g_assert(FALSE);
      break;
    }
  }
}

static void bt_edit_application_dispose(GObject *object) {
  BtEditApplication *self = BT_EDIT_APPLICATION(object);
	return_if_disposed();
  self->private->dispose_has_run = TRUE;
}

static void bt_edit_application_finalize(GObject *object) {
  BtEditApplication *self = BT_EDIT_APPLICATION(object);
  
  g_object_unref(G_OBJECT(self->private->song));
  g_free(self->private);
}

static void bt_edit_application_init(GTypeInstance *instance, gpointer g_class) {
  BtEditApplication *self = BT_EDIT_APPLICATION(instance);
  self->private = g_new0(BtEditApplicationPrivate,1);
  self->private->dispose_has_run = FALSE;
}

static void bt_edit_application_class_init(BtEditApplicationClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GParamSpec *g_param_spec;
  
  gobject_class->set_property = bt_edit_application_set_property;
  gobject_class->get_property = bt_edit_application_get_property;
  gobject_class->dispose      = bt_edit_application_dispose;
  gobject_class->finalize     = bt_edit_application_finalize;
}

GType bt_edit_application_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtEditApplicationClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_edit_application_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtEditApplication),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_edit_application_init, // instance_init
    };
		type = g_type_register_static(BT_TYPE_APPLICATION,"BtEditApplication",&info,0);
  }
  return type;
}

