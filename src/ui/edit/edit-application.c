/* $Id: edit-application.c,v 1.1 2004-07-29 15:51:31 ensonic Exp $
 * class for a gtk based buzztard editor application
 */
 
//#define BT_CORE -> BT_EDIT ?
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
  GtkWidget *window;
};

//-- event handler

static gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data) {
  /* If you return FALSE in the "delete_event" signal handler, GTK will emit the
   * "destroy" signal. Returning TRUE means you don't want the window to be
   * destroyed.
   * This is useful for popping up 'are you sure to quit?' type dialogs.
   */

  GST_INFO("delete event occurred\n");
 return(FALSE);
}

/* Another callback */
static void destroy(GtkWidget *widget, gpointer data) {
  gtk_main_quit();
}

//-- helper methods

static gboolean bt_edit_application_init_ui(const BtEditApplication *self) {
  GtkWidget *box;
  
  // create the window
  self->private->window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
  // @todo make the title: PACKAGE_NAME": %s",song->name 
  gtk_window_set_title(GTK_WINDOW(self->private->window), PACKAGE_NAME);
  g_signal_connect(G_OBJECT(self->private->window), "delete_event", G_CALLBACK(delete_event), NULL);
  g_signal_connect(G_OBJECT(self->private->window), "destroy",      G_CALLBACK(destroy), NULL);
  
  // create main layout container
  box=gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(self->private->window),box);

  // @todo add menu-bar
  // @todo add tool-bar
  // @todo add notebook widget
  // @todo add status bar
}

static void bt_edit_application_run_ui(const BtEditApplication *self) {
  gtk_widget_show(self->private->window);
  gtk_main();
}

static void bt_edit_application_run_done(const BtEditApplication *self) {
  self->private->window=NULL;
}

static gboolean bt_edit_application_ui(const BtEditApplication *self) {
  gboolean res=FALSE;
  GST_INFO("before running the UI\n");
  // generate UI
  if(bt_edit_application_init_ui(self)) {
    // run UI loop
    bt_edit_application_run_ui(self);
    // shut down UI
    bt_edit_application_run_done(self);
    res=TRUE;
  }
  GST_INFO("after running the UI\n");
  return(res);
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

	self->private->song=(BtSong *)g_object_new(BT_TYPE_SONG,"bin",bt_g_object_get_object_property(G_OBJECT(self),"bin"),"name","empty song", NULL);
	
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

	self->private->song=(BtSong *)g_object_new(BT_TYPE_SONG,"bin",bt_g_object_get_object_property(G_OBJECT(self),"bin"), NULL);
	loader=(BtSongIO *)g_object_new(bt_song_io_detect(input_file_name),NULL);
	
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

