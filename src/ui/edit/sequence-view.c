/* $Id: sequence-view.c,v 1.7 2005-02-11 20:37:34 ensonic Exp $
 * class for the sequence view widget
 */

#define BT_EDIT
#define BT_SEQUENCE_VIEW_C

#include "bt-edit.h"

enum {
  SEQUENCE_VIEW_APP=1,
	SEQUENCE_VIEW_PLAY_POSITION,
	SEQUENCE_VIEW_LOOP_START,
	SEQUENCE_VIEW_LOOP_END,
	SEQUENCE_VIEW_VISIBLE_ROWS
};


struct _BtSequenceViewPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* the application */
  BtEditApplication *app;
	
	/* position of playing pointer from 0.0 ... 1.0 */
	gdouble play_pos;
	
	/* position of loop range from 0.0 ... 1.0 */
	gdouble loop_start,loop_end;

	/* number of visible rows, the height of one row */
	gulong visible_rows,row_height;
	
	/* cache some ressources */
	GdkWindow *window;
	//GdkBitmap *loop_pos_stipple;
	GdkGC *play_pos_gc,*loop_pos_gc;
};

static GtkTreeViewClass *parent_class=NULL;

static gint8 loop_pos_dash_list[]= {4};

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

static void bt_sequence_view_realize(GtkWidget *widget) {
	BtSequenceView *self = BT_SEQUENCE_VIEW(widget);
	GdkColor color;
	GtkTreePath *path;
	GdkRectangle br;

  // first let the parent realize itslf
  if(GTK_WIDGET_CLASS(parent_class)->realize) {
    (GTK_WIDGET_CLASS(parent_class)->realize)(widget);
  }
	self->priv->window=gtk_tree_view_get_bin_window(GTK_TREE_VIEW(self));

	color.red = 0;
	color.green = 0;
	color.blue = 65535;
	self->priv->play_pos_gc=gdk_gc_new(self->priv->window);
	gdk_gc_set_rgb_fg_color(self->priv->play_pos_gc,&color);
	gdk_gc_set_line_attributes(self->priv->play_pos_gc,2,GDK_LINE_SOLID,GDK_CAP_BUTT,GDK_JOIN_MITER);

	color.red = 65535;
	color.green = (gint)(0.75*65535.0);
	color.blue = 0;
	self->priv->loop_pos_gc=gdk_gc_new(self->priv->window);
	gdk_gc_set_rgb_fg_color(self->priv->loop_pos_gc,&color);
	gdk_gc_set_line_attributes(self->priv->loop_pos_gc,2,GDK_LINE_ON_OFF_DASH,GDK_CAP_BUTT,GDK_JOIN_MITER);
	gdk_gc_set_dashes(self->priv->loop_pos_gc,0,loop_pos_dash_list,1);
	
	path=gtk_tree_path_new_from_indices(0,-1);
	gtk_tree_view_get_background_area(GTK_TREE_VIEW(widget),path,NULL,&br);
	self->priv->row_height=br.height;
	GST_INFO(" cell background visible rect: %d x %d, %d x %d",br.x,br.y,br.width,br.height);
	
}

static void bt_sequence_view_unrealize (GtkWidget *widget) {
	BtSequenceView *self = BT_SEQUENCE_VIEW(widget);

  // first let the parent realize itslf
  if(GTK_WIDGET_CLASS(parent_class)->unrealize) {
    (GTK_WIDGET_CLASS(parent_class)->unrealize)(widget);
  }
	
	gdk_gc_unref(self->priv->loop_pos_gc);
	self->priv->play_pos_gc=NULL;
	
	gdk_gc_unref(self->priv->loop_pos_gc);
	self->priv->play_pos_gc=NULL;
	
}

// @todo when scrolling the lines leave garbage on screen
static gboolean bt_sequence_view_expose_event(GtkWidget *widget,GdkEventExpose *event) {
	BtSequenceView *self = BT_SEQUENCE_VIEW(widget);

	//GST_INFO("!!!! self=%p",self);
	
  // first let the parent handle its expose
  if(GTK_WIDGET_CLASS(parent_class)->expose_event) {
    (GTK_WIDGET_CLASS(parent_class)->expose_event)(widget,event);
  }

	/* We need to check to make sure that the expose event is actually occuring on
   * the window where the table data is being drawn.  If we don't do this check,
   * row zero spanners can be drawn on top of the column headers.
 	 */
  if(self->priv->window==event->window) {
		gint w,y;
		gdouble h;
		//GdkRectangle vr,br;
		//GtkTreePath *path;
		
		//gtk_tree_view_get_visible_rect(GTK_TREE_VIEW(widget),&vr);
		// path should point to the last row (of course there is no way the API will tell us ...)
		//path=gtk_tree_path_new_from_indices(0,-1);
		//gtk_tree_view_get_background_area(GTK_TREE_VIEW(widget),path,NULL,&br);
		//GST_INFO(" tree view visible rect: %d x %d, %d x %d",vr.x,vr.y,vr.width,vr.height);
		//GST_INFO(" cell background visible rect: %d x %d, %d x %d",br.x,br.y,br.width,br.height);
		//GST_INFO(" tree view allocation: %d x %d",widget->allocation.width,widget->allocation.height);

		//h=(gint)(self->priv->play_pos*(double)widget->allocation.height);
		//w=vr.width;
		//h=(gint)(self->priv->play_pos*(double)vr.height);

		w=widget->allocation.width;
		h=(gdouble)(self->priv->visible_rows*self->priv->row_height);

		y=(gint)(self->priv->play_pos*h);
  	gdk_draw_line(self->priv->window,self->priv->play_pos_gc,0,y,w,y);

		y=(gint)(self->priv->loop_start*h);
  	gdk_draw_line(self->priv->window,self->priv->loop_pos_gc,0,y,w,y);
		y=(gint)(self->priv->loop_end*h)-2;
  	gdk_draw_line(self->priv->window,self->priv->loop_pos_gc,0,y,w,y);
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
			gtk_widget_queue_draw(GTK_WIDGET(self));
    } break;
    case SEQUENCE_VIEW_LOOP_START: {
      self->priv->loop_start = g_value_get_double(value);
			GST_DEBUG("set the loop-start for sequence_view: %f",self->priv->loop_start);
			gtk_widget_queue_draw(GTK_WIDGET(self));
    } break;
    case SEQUENCE_VIEW_LOOP_END: {
      self->priv->loop_end = g_value_get_double(value);
			GST_DEBUG("set the loop-end for sequence_view: %f",self->priv->loop_end);
			gtk_widget_queue_draw(GTK_WIDGET(self));
    } break;
    case SEQUENCE_VIEW_VISIBLE_ROWS: {
      self->priv->visible_rows = g_value_get_ulong(value);
			gtk_widget_queue_draw(GTK_WIDGET(self));
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
	
	g_object_try_unref(self->priv->play_pos_gc);

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
	gtkwidget_class->realize = bt_sequence_view_realize;
	gtkwidget_class->unrealize = bt_sequence_view_unrealize;
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

  g_object_class_install_property(gobject_class,SEQUENCE_VIEW_LOOP_START,
                                  g_param_spec_double("loop-start",
                                     "loop start position prop.",
                                     "The start position of the loop range as a fraction",
                                     0.0,
																		 1.0,
																		 0.0,
                                     G_PARAM_WRITABLE));

  g_object_class_install_property(gobject_class,SEQUENCE_VIEW_LOOP_END,
                                  g_param_spec_double("loop-end",
                                     "loop end position prop.",
                                     "The end position of the loop range as a fraction",
                                     0.0,
																		 1.0,
																		 1.0,
                                     G_PARAM_WRITABLE));

g_object_class_install_property(gobject_class,SEQUENCE_VIEW_VISIBLE_ROWS,
                                  g_param_spec_ulong("visible-rows",
                                     "visible rows prop.",
                                     "The number of currntly visible sequence rows",
                                     0,
																		 G_MAXULONG,
																		 0,
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
