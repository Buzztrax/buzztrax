/* $Id: main-page-machines.c,v 1.17 2004-10-12 17:41:02 ensonic Exp $
 * class for the editor main machines page
 */

#define BT_EDIT
#define BT_MAIN_PAGE_MACHINES_C

#include "bt-edit.h"

enum {
  MAIN_PAGE_MACHINES_APP=1,
};

#define ZOOM_X 100.0
#define ZOOM_Y  80.0

struct _BtMainPageMachinesPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* the application */
  BtEditApplication *app;

  /* canvas for machine view */
  GnomeCanvas *canvas;
  /* the zoomration in pixels/per unit */
  double zoom;

  /* we probably need a list of canvas items that we have drawn, so that we can
   * easily clear them later
   */
};

static GtkVBoxClass *parent_class=NULL;

//-- graphics helpers

// @todo this needs more parameters
/**
 * bt_main_page_machines_draw_machine:
 * @self: the machine-view page to draw
 *
 * draw something that looks a bit like a buzz-machine
 */
static void bt_main_page_machines_draw_machine(const BtMainPageMachines *self, gdouble pos_x, gdouble pos_y, guint bg_color, gchar *name) {
  GnomeCanvasItem *item;
  GnomeCanvasGroup *group;
  gdouble w=25.0,h=15.0;

  GST_INFO("  draw machine \"%s\" at %lf,%lf",name,pos_x,pos_y);
  
  group=GNOME_CANVAS_GROUP(gnome_canvas_item_new(gnome_canvas_root(self->priv->canvas),
                           GNOME_TYPE_CANVAS_GROUP,
                           "x", pos_x-(w/2.0),
                           "y", pos_y-(h/2.0),
                           NULL));

  // add machine visualisation components
  item=gnome_canvas_item_new(group,
                           GNOME_TYPE_CANVAS_RECT,
                           "x1", 0.0,
                           "y1", 0.0,
                           "x2", w,
                           "y2", h,
                           "fill-color-rgba", bg_color,
                           /*"fill-color", "gray",*/
                           "outline_color", "black",
                           "width-pixels", 1,
                           NULL);
  item=gnome_canvas_item_new(group,
                           GNOME_TYPE_CANVAS_TEXT,
                           "x", (w/2.0),
                           "y", 4.0,
                           "justification", GTK_JUSTIFY_CENTER,
                           "size-points", 10.0,
                           "size-set", TRUE,
                           "text", name,
                           "fill-color", "black",
                           NULL);
                           
  // @todo connect the event signal to on_machine_event()
}

static void bt_main_page_machines_draw_wire(const BtMainPageMachines *self, gdouble pos_xs, gdouble pos_ys,gdouble pos_xe, gdouble pos_ye) {
  GnomeCanvasItem *item;
  GnomeCanvasPoints *points;
  
  GST_INFO("  draw wire from %lf,%lf to %lf,%lf",pos_xs,pos_ys,pos_xe,pos_ye);
  
  points=gnome_canvas_points_new(2);
  /* fill out the points */
  points->coords[0]=pos_xs;
  points->coords[1]=pos_ys;
  points->coords[2]=pos_xe;
  points->coords[3]=pos_ye;
  item=gnome_canvas_item_new(gnome_canvas_root(self->priv->canvas),
                           GNOME_TYPE_CANVAS_LINE,
                           "points", points,
                           "fill-color", "black",
                           "width-pixels", 1,
                           NULL);
  gnome_canvas_points_free(points);
  
  // @todo add trinagle pointing (GnomeCanvasPolygon) to dest at the middle of the wire
  /*
  mid_x=pos_xs+(pos_xe-pos_xs)/2.0;
  mid_y=pos_ys+(pos_ye-pos_ys)/2.0;
  // go X points forth on the wire, that is the tip of the triangle
  // go X points back on the wire and cast an orthogonal line of length X*2
  */
  // @todo connect the event signal to on_wire_event()
}

//-- event handler helper

static void machine_view_get_machine_position(GHashTable *properties, gdouble *pos_x,gdouble *pos_y) {
  char *prop;

  *pos_x=*pos_y=0.0;
  if(properties) {
    prop=(gchar *)g_hash_table_lookup(properties,"xpos");
    if(prop) {
      *pos_x=ZOOM_X*g_ascii_strtod(prop,NULL);
      // do not g_free(prop);
    }
    else GST_WARNING("no xpos property found");
    prop=(gchar *)g_hash_table_lookup(properties,"ypos");
    if(prop) {
      *pos_y=ZOOM_Y*g_ascii_strtod(prop,NULL);
      // do not g_free(prop);
    }
    else GST_WARNING("no ypos property found");
  }
  else GST_WARNING("no properties supplied");
}

static void machine_view_refresh(const BtMainPageMachines *self,const BtSetup *setup) {
  gpointer iter;
  GHashTable *properties;
  BtMachine *machine;
  BtWire *wire;
  char *id,*prop;
  guint bg_color=0xFFFFFFFF;
  gdouble pos_x,pos_y;
  gdouble pos_xs,pos_ys,pos_xe,pos_ye;
  GList *items;
  
  // @todo clear the canvas, problem: items is always NULL
  if((items=gtk_container_get_children(GTK_CONTAINER(self->priv->canvas)))) {
    GList* node=g_list_first(items);
    GnomeCanvasItem *item;
    GST_DEBUG("before destoying canvas items");
    while(node) {
      GST_DEBUG("destoying canvas item");
      item=GNOME_CANVAS_ITEM(node->data);
      gtk_object_destroy(GTK_OBJECT(item));
      node=g_list_next(node);
    }
    g_list_free(items);
  }
  else {
    GST_DEBUG("no items on canvas");
  }
  
  // draw all wires
  iter=bt_setup_wire_iterator_new(setup);
  while(iter) {
    wire=bt_setup_wire_iterator_get_wire(iter);
    // get positions of source and dest
    g_object_get(wire,"src",&machine,NULL);
    g_object_get(machine,"properties",&properties,NULL);
    machine_view_get_machine_position(properties,&pos_xs,&pos_ys);
    GST_DEBUG("src-machine is %p",machine);
    g_object_try_unref(machine);
    g_object_get(wire,"dst",&machine,NULL);
    g_object_get(machine,"properties",&properties,NULL);
    machine_view_get_machine_position(properties,&pos_xe,&pos_ye);
    GST_DEBUG("dst-machine is %p",machine);
    g_object_try_unref(machine);
    // draw wire
    bt_main_page_machines_draw_wire(self,pos_xs,pos_ys,pos_xe,pos_ye);
    iter=bt_setup_wire_iterator_next(iter);
  }

  // draw all machines
  iter=bt_setup_machine_iterator_new(setup);
  while(iter) {
    machine=bt_setup_machine_iterator_get_machine(iter);
    // get position, name and machine type
    g_object_get(machine,"properties",&properties,"id",&id,NULL);
    GST_DEBUG("machine is %p",machine);
    machine_view_get_machine_position(properties,&pos_x,&pos_y);
    if(BT_IS_SOURCE_MACHINE(machine)) {
      bg_color=0xFFAFAFFF;
    }
    if(BT_IS_PROCESSOR_MACHINE(machine)) {
      bg_color=0xAFFFAFFF;
    }
    if(BT_IS_SINK_MACHINE(machine)) {
      bg_color=0xAFAFFFFF;
    }
    // draw machine
    bt_main_page_machines_draw_machine(self,pos_x,pos_y,bg_color,id);
    g_free(id);
    iter=bt_setup_machine_iterator_next(iter);
  }
}

//-- event handler

static void on_song_changed(const BtEditApplication *app, gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);
  BtSong *song;
  BtSetup *setup;

  g_assert(user_data);

  GST_INFO("song has changed : app=%p, self=%p",app,self);
  // get song from app
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_object_get(G_OBJECT(song),"setup",&setup,NULL);
  // update page
  machine_view_refresh(self,setup);
  // release the reference
  g_object_try_unref(song);
}

static void on_toolbar_zoom_in_clicked(GtkButton *button, gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);

  g_assert(user_data);

  self->priv->zoom*=1.75;
  GST_INFO("toolbar zoom_in event occurred : %lf",self->priv->zoom);
  gnome_canvas_set_pixels_per_unit(self->priv->canvas,self->priv->zoom);
}

static void on_toolbar_zoom_out_clicked(GtkButton *button, gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);

  g_assert(user_data);

  self->priv->zoom/=1.75;
  GST_INFO("toolbar zoom_out event occurred : %lf",self->priv->zoom);
  gnome_canvas_set_pixels_per_unit(self->priv->canvas,self->priv->zoom);
}


//-- helper methods

static gboolean bt_main_page_machines_init_ui(const BtMainPageMachines *self, const BtEditApplication *app) {
  GtkWidget *toolbar;
  GtkWidget *icon,*button,*image,*scrolled_window;
  GtkTooltips *tips;

  // add toolbar
  toolbar=gtk_toolbar_new();
  gtk_widget_set_name(toolbar,_("machine view tool bar"));
  gtk_box_pack_start(GTK_BOX(self),GTK_WIDGET(toolbar),FALSE,FALSE,0);
  gtk_toolbar_set_style(GTK_TOOLBAR(toolbar),GTK_TOOLBAR_BOTH);

  tips=gtk_tooltips_new();
  
  icon=gtk_image_new_from_stock(GTK_STOCK_ZOOM_FIT, gtk_toolbar_get_icon_size(GTK_TOOLBAR(toolbar)));
  button=gtk_toolbar_append_element(GTK_TOOLBAR(toolbar),
                                GTK_TOOLBAR_CHILD_BUTTON,
                                NULL,
                                _("Zoom Fit"),
                                NULL,NULL,
                                icon,NULL,NULL);
  gtk_label_set_use_underline(GTK_LABEL(((GtkToolbarChild*)(g_list_last(GTK_TOOLBAR(toolbar)->children)->data))->label),TRUE);
  gtk_widget_set_name(button,_("Zoom Fit"));
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tips),button,_("Zoom in/out so that everything is visible"),NULL);
  //g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(on_toolbar_zoom_fit_clicked),(gpointer)self);

  icon=gtk_image_new_from_stock(GTK_STOCK_ZOOM_IN, gtk_toolbar_get_icon_size(GTK_TOOLBAR(toolbar)));
  button=gtk_toolbar_append_element(GTK_TOOLBAR(toolbar),
                                GTK_TOOLBAR_CHILD_BUTTON,
                                NULL,
                                _("Zoom In"),
                                NULL,NULL,
                                icon,NULL,NULL);
  gtk_label_set_use_underline(GTK_LABEL(((GtkToolbarChild*)(g_list_last(GTK_TOOLBAR(toolbar)->children)->data))->label),TRUE);
  gtk_widget_set_name(button,_("Zoom In"));
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tips),button,_("Zoom in so more details are visible"),NULL);
  g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(on_toolbar_zoom_in_clicked),(gpointer)self);
  
  icon=gtk_image_new_from_stock(GTK_STOCK_ZOOM_OUT, gtk_toolbar_get_icon_size(GTK_TOOLBAR(toolbar)));
  button=gtk_toolbar_append_element(GTK_TOOLBAR(toolbar),
                                GTK_TOOLBAR_CHILD_BUTTON,
                                NULL,
                                _("Zoom Out"),
                                NULL,NULL,
                                icon,NULL,NULL);
  gtk_label_set_use_underline(GTK_LABEL(((GtkToolbarChild*)(g_list_last(GTK_TOOLBAR(toolbar)->children)->data))->label),TRUE);
  gtk_widget_set_name(button,_("Zoom Out"));
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tips),button,_("Zoom out for better overview"),NULL);
  g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(on_toolbar_zoom_out_clicked),(gpointer)self);
  
  // add canvas
  scrolled_window=gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),GTK_SHADOW_NONE);
  gtk_widget_push_visual(gdk_imlib_get_visual());
  // @todo try gtk_widget_push_colormap(gdk_colormap_get_system());
  //gtk_widget_push_colormap((GdkColormap *)gdk_imlib_get_colormap());
  self->priv->canvas=GNOME_CANVAS(gnome_canvas_new_aa());
  gnome_canvas_set_center_scroll_region(self->priv->canvas,TRUE);
  gnome_canvas_set_scroll_region(self->priv->canvas,-ZOOM_X,-ZOOM_Y,ZOOM_X,ZOOM_Y);
  gnome_canvas_set_pixels_per_unit(self->priv->canvas,self->priv->zoom);
  //gtk_widget_pop_colormap();
  gtk_widget_pop_visual();
  gtk_container_add(GTK_CONTAINER(scrolled_window),GTK_WIDGET(self->priv->canvas));
  gtk_box_pack_start(GTK_BOX(self),scrolled_window,TRUE,TRUE,0);
  
  // register event handlers
  g_signal_connect(G_OBJECT(app), "song-changed", (GCallback)on_song_changed, (gpointer)self);
  return(TRUE);
}

//-- constructor methods

/**
 * bt_main_page_machines_new:
 * @app: the application the window belongs to
 *
 * Create a new instance
 *
 * Returns: the new instance or NULL in case of an error
 */
BtMainPageMachines *bt_main_page_machines_new(const BtEditApplication *app) {
  BtMainPageMachines *self;

  if(!(self=BT_MAIN_PAGE_MACHINES(g_object_new(BT_TYPE_MAIN_PAGE_MACHINES,"app",app,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_main_page_machines_init_ui(self,app)) {
    goto Error;
  }
  return(self);
Error:
  g_object_try_unref(self);
  return(NULL);
}

//-- methods

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_main_page_machines_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES(object);
  return_if_disposed();
  switch (property_id) {
    case MAIN_PAGE_MACHINES_APP: {
      g_value_set_object(value, self->priv->app);
    } break;
    default: {
 			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_main_page_machines_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES(object);
  return_if_disposed();
  switch (property_id) {
    case MAIN_PAGE_MACHINES_APP: {
      g_object_try_unref(self->priv->app);
      self->priv->app = g_object_try_ref(g_value_get_object(value));
      //GST_DEBUG("set the app for main_page_machines: %p",self->priv->app);
    } break;
    default: {
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_main_page_machines_dispose(GObject *object) {
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES(object);
	return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  g_object_try_unref(self->priv->app);

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_main_page_machines_finalize(GObject *object) {
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES(object);
  
  g_free(self->priv);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_main_page_machines_init(GTypeInstance *instance, gpointer g_class) {
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES(instance);
  self->priv = g_new0(BtMainPageMachinesPrivate,1);
  self->priv->dispose_has_run = FALSE;

  self->priv->zoom=5.0;
}

static void bt_main_page_machines_class_init(BtMainPageMachinesClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GParamSpec *g_param_spec;

  parent_class=g_type_class_ref(GTK_TYPE_VBOX);
  
  gobject_class->set_property = bt_main_page_machines_set_property;
  gobject_class->get_property = bt_main_page_machines_get_property;
  gobject_class->dispose      = bt_main_page_machines_dispose;
  gobject_class->finalize     = bt_main_page_machines_finalize;

  g_object_class_install_property(gobject_class,MAIN_PAGE_MACHINES_APP,
                                  g_param_spec_object("app",
                                     "app contruct prop",
                                     "Set application object, the window belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));
}

GType bt_main_page_machines_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtMainPageMachinesClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_main_page_machines_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtMainPageMachines),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_main_page_machines_init, // instance_init
    };
		type = g_type_register_static(GTK_TYPE_VBOX,"BtMainPageMachines",&info,0);
  }
  return type;
}

