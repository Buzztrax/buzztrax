/* $Id: sequence-view.c,v 1.2 2005-02-07 14:57:55 ensonic Exp $
 * class for the sequence view widget
 */

#define BT_EDIT
#define BT_SEQUENCE_VIEW_C

#include "bt-edit.h"

enum {
  SEQUENCE_VIEW_APP=1,
	SEQUENCE_VIEW_PLAY_POSITION
};


struct _BtSequenceViewPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* the application */
  BtEditApplication *app;
	
	/* position of playing pointer from 0.0 ... 1.0 */
	gdouble play_pos;
};

static GtkTreeViewClass *parent_class=NULL;

//-- event handler

//-- helper methods

//-- constructor methods

/**
 * bt_sequence_view_new:
 * @app: the application the window belongs to
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtSequenceView *bt_sequence_view_new(const BtEditApplication *app) {
  BtSequenceView *self;

  if(!(self=BT_SEQUENCE_VIEW(g_object_new(BT_TYPE_SEQUENCE_VIEW,"app",app,NULL)))) {
    goto Error;
  }
  // generate UI
  //if(!bt_sequence_view_init_ui(self)) {
    //goto Error;
  //}
  return(self);
Error:
  g_object_try_unref(self);
  return(NULL);
}

//-- methods

//-- wrapper

//-- class internals

static gboolean bt_sequence_view_expose_event(GtkWidget *widget,GdkEventExpose *event) {
	BtSequenceView *self = BT_SEQUENCE_VIEW(widget);
	GdkWindow *window;

	//GST_INFO("!!!! self=%p",self);
	
  // first let the parent handle its expose
  if(GTK_WIDGET_CLASS(parent_class)->expose_event) {
    (GTK_WIDGET_CLASS(parent_class)->expose_event)(widget,event);
  }

	/* We need to check to make sure that the expose event is actually
   * occuring on the window where the table data is being drawn.  If
   * we don't do this check, row zero spanners can be drawn on top
   * of the column headers.
 	 */
  window=gtk_tree_view_get_bin_window(GTK_TREE_VIEW(widget));
  if(window==event->window) {
		gint w,h;
		GdkGC *gc;
		GdkColor color;

		//GST_INFO(" draw line %d x %d",widget->allocation.width,widget->allocation.height);

		gc=gdk_gc_new(window);
		color.red = 0;
		color.green = 0;
		color.blue = 65535;
		gdk_gc_set_rgb_fg_color(gc,&color);
	
		w=widget->allocation.width;
		h=(gint)(self->priv->play_pos*(double)widget->allocation.height);
  	gdk_draw_line(window,gc,0,h,w,h);
		g_object_unref(gc);
	}
	return(FALSE);
}

/* returns a property for the given property_id for this object */
static void bt_sequence_view_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtSequenceView *self = BT_SEQUENCE_VIEW(object);
  return_if_disposed();
  switch (property_id) {
    case SEQUENCE_VIEW_APP: {
      g_value_set_object(value, self->priv->app);
			gtk_widget_queue_draw(GTK_WIDGET(self));
    } break;
    default: {
 			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_sequence_view_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtSequenceView *self = BT_SEQUENCE_VIEW(object);
  return_if_disposed();
  switch (property_id) {
    case SEQUENCE_VIEW_APP: {
      g_object_try_weak_unref(self->priv->app);
      self->priv->app = BT_EDIT_APPLICATION(g_value_get_object(value));
			g_object_try_weak_ref(self->priv->app);
      //GST_DEBUG("set the app for sequence_view: %p",self->priv->app);
    } break;
    case SEQUENCE_VIEW_PLAY_POSITION: {
      self->priv->play_pos = g_value_get_double(value);
      //GST_DEBUG("set the app for sequence_view: %p",self->priv->app);
    } break;
    default: {
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_sequence_view_dispose(GObject *object) {
  BtSequenceView *self = BT_SEQUENCE_VIEW(object);
	return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  g_object_try_weak_unref(self->priv->app);
  // this disposes the pages for us
  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_sequence_view_finalize(GObject *object) {
  BtSequenceView *self = BT_SEQUENCE_VIEW(object);
  
  GST_DEBUG("!!!! self=%p",self);
  g_free(self->priv);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_sequence_view_init(GTypeInstance *instance, gpointer g_class) {
  BtSequenceView *self = BT_SEQUENCE_VIEW(instance);
  self->priv = g_new0(BtSequenceViewPrivate,1);
  self->priv->dispose_has_run = FALSE;
}

static void bt_sequence_view_class_init(BtSequenceViewClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	GtkWidgetClass *gtkwidget_class = GTK_WIDGET_CLASS(klass);

  parent_class=g_type_class_ref(GTK_TYPE_TREE_VIEW);

  gobject_class->set_property = bt_sequence_view_set_property;
  gobject_class->get_property = bt_sequence_view_get_property;
  gobject_class->dispose      = bt_sequence_view_dispose;
  gobject_class->finalize     = bt_sequence_view_finalize;
	
	// override some gtkwidget methods
	gtkwidget_class->expose_event = bt_sequence_view_expose_event;

  g_object_class_install_property(gobject_class,SEQUENCE_VIEW_APP,
                                  g_param_spec_object("app",
                                     "app contruct prop",
                                     "Set application object, the window belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,SEQUENCE_VIEW_PLAY_POSITION,
                                  g_param_spec_double("play-position",
                                     "play position prop.",
                                     "The current playing position as a fraction",
                                     0.0,
																		 1.0,
																		 0.0,
                                     G_PARAM_WRITABLE));
}

GType bt_sequence_view_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      G_STRUCT_SIZE(BtSequenceViewClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_sequence_view_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtSequenceView),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_sequence_view_init, // instance_init
			NULL // value_table
    };
		type = g_type_register_static(GTK_TYPE_TREE_VIEW,"BtSequenceView",&info,0);
  }
  return type;
}
