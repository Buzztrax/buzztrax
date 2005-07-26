// $Id: ui-ressources.c,v 1.3 2005-07-26 06:44:38 waffel Exp $
/**
 * SECTION:btuiressources
 * @short_description: common shared ui ressources like icons and colors
 */ 

#define BT_EDIT
#define BT_UI_RESSOURCES_C

#include "bt-edit.h"

struct _BtUIRessourcesPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

	GdkPixbuf *source_machine_pixbuf;
	GdkPixbuf *processor_machine_pixbuf;
	GdkPixbuf *sink_machine_pixbuf;
	
};

static GObjectClass *parent_class=NULL;

BtUIRessources *ui_ressources=NULL;

//-- event handler

//-- helper methods

static gboolean bt_ui_ressources_init_colors(BtUIRessources *self) {
	/*
		we need for each machine type:
			BASE_{RGB|GDK}_COLOR		: machine view normal
			TRANS_{RGB|GDK}_COLOR		: machine move
			BRIGHT1_{RGB|GDK}_COLOR	: sequence odd empty
			BRIGHT2_{RGB|GDK}_COLOR	: sequence even empty
			DARK1_{RGB|GDK}_COLOR  	: sequence odd filled
			DARK2_{RGB|GDK}_COLOR  	: sequence even filled
	*/
	/* canvas-machine-item.c:
	guint bg_color=0xFFFFFFFF,bg_color2=0x99999999;
	
	if(BT_IS_SOURCE_MACHINE(self->priv->machine)) {
    bg_color=0xFFAFAFFF;bg_color2=0x99696999;
  }
  if(BT_IS_PROCESSOR_MACHINE(self->priv->machine)) {
    bg_color=0xAFFFAFFF;bg_color2=0x69996999;
  }
  if(BT_IS_SINK_MACHINE(self->priv->machine)) {
    bg_color=0xAFAFFFFF;bg_color2=0x69699999;
  }
	*/
	/* main-page-sequence.c:
  GdkColor source_bg1,source_bg2;
  GdkColor processor_bg1,processor_bg2;
  GdkColor sink_bg1,sink_bg2;

  colormap=gdk_colormap_get_system();
  self->priv->source_bg1.red=  (guint16)(1.0*65535);
  self->priv->source_bg1.green=(guint16)(0.9*65535);
  self->priv->source_bg1.blue= (guint16)(0.9*65535);
  gdk_colormap_alloc_color(colormap,&self->priv->source_bg1,FALSE,TRUE);
  self->priv->source_bg2.red=  (guint16)(1.0*65535);
  self->priv->source_bg2.green=(guint16)(0.8*65535);
  self->priv->source_bg2.blue= (guint16)(0.8*65535);
  gdk_colormap_alloc_color(colormap,&self->priv->source_bg2,FALSE,TRUE);
  self->priv->processor_bg1.red=  (guint16)(0.9*65535);
  self->priv->processor_bg1.green=(guint16)(1.0*65535);
  self->priv->processor_bg1.blue= (guint16)(0.9*65535);
  gdk_colormap_alloc_color(colormap,&self->priv->processor_bg1,FALSE,TRUE);
  self->priv->processor_bg2.red=  (guint16)(0.8*65535);
  self->priv->processor_bg2.green=(guint16)(1.0*65535);
  self->priv->processor_bg2.blue= (guint16)(0.8*65535);
  gdk_colormap_alloc_color(colormap,&self->priv->processor_bg2,FALSE,TRUE);
  self->priv->sink_bg1.red=  (guint16)(0.9*65535);
  self->priv->sink_bg1.green=(guint16)(0.9*65535);
  self->priv->sink_bg1.blue= (guint16)(1.0*65535);
  gdk_colormap_alloc_color(colormap,&self->priv->sink_bg1,FALSE,TRUE);
  self->priv->sink_bg2.red=  (guint16)(0.8*65535);
  self->priv->sink_bg2.green=(guint16)(0.8*65535);
  self->priv->sink_bg2.blue= (guint16)(1.0*65535);
  gdk_colormap_alloc_color(colormap,&self->priv->sink_bg2,FALSE,TRUE);	
	*/

	return(TRUE);
}

static gboolean bt_ui_ressources_init_icons(BtUIRessources *self) {
	self->priv->source_machine_pixbuf=gdk_pixbuf_new_from_filename("menu_source_machine.png");
	self->priv->processor_machine_pixbuf=gdk_pixbuf_new_from_filename("menu_processor_machine.png");
	self->priv->sink_machine_pixbuf=gdk_pixbuf_new_from_filename("menu_sink_machine.png");

	return(TRUE);
}

//-- constructor methods

/**
 * bt_ui_ressources_new:
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtUIRessources *bt_ui_ressources_new(void) {
  BtUIRessources *self;

  if(!(self=BT_UI_RESSOURCES(g_object_new(BT_TYPE_UI_RESSOURCES,NULL)))) {
    goto Error;
  }
  // initialise ressources
  if(!bt_ui_ressources_init_colors(self)) {
    goto Error;
  }
  if(!bt_ui_ressources_init_icons(self)) {
    goto Error;
  }
  return(self);
Error:
  g_object_try_unref(self);
  return(NULL);
}

//-- methods

/**
 * bt_ui_ressources_get_pixbuf_by_machine:
 * @machine: the machine to get the image for
 *
 * Gets a #GdkPixbuf image that matches the given machine type.
 *
 * Returns: a #GdkPixbuf image
 */
GdkPixbuf *bt_ui_ressources_get_pixbuf_by_machine(const BtMachine *machine) {
	if(BT_IS_SOURCE_MACHINE(machine)) {
		return(g_object_ref(ui_ressources->priv->source_machine_pixbuf));
	}
	else if(BT_IS_PROCESSOR_MACHINE(machine)) {
		return(g_object_ref(ui_ressources->priv->processor_machine_pixbuf));
  }
  else if(BT_IS_SINK_MACHINE(machine)) {
		return(g_object_ref(ui_ressources->priv->sink_machine_pixbuf));
  }
	return(NULL);
}

/**
 * bt_ui_ressources_get_image_by_machine:
 * @machine: the machine to get the image for
 *
 * Gets a #GtkImage that matches the given machine type.
 *
 * Returns: a #GtkImage widget
 */
GtkWidget *bt_ui_ressources_get_image_by_machine(const BtMachine *machine) {
	if(BT_IS_SOURCE_MACHINE(machine)) {
		return(gtk_image_new_from_pixbuf(ui_ressources->priv->source_machine_pixbuf));
	}
	else if(BT_IS_PROCESSOR_MACHINE(machine)) {
		return(gtk_image_new_from_pixbuf(ui_ressources->priv->processor_machine_pixbuf));
  }
  else if(BT_IS_SINK_MACHINE(machine)) {
		return(gtk_image_new_from_pixbuf(ui_ressources->priv->sink_machine_pixbuf));
  }
	return(NULL);
}

/**
 * bt_ui_ressources_get_image_by_machine_type:
 * @machine_type: the machine_type to get the image for
 *
 * Gets a #GtkImage that matches the given machine type.
 *
 * Returns: a #GtkImage widget
 */
GtkWidget *bt_ui_ressources_get_image_by_machine_type(GType machine_type) {
	if(machine_type==BT_TYPE_SOURCE_MACHINE) {
		return(gtk_image_new_from_pixbuf(ui_ressources->priv->source_machine_pixbuf));
	}
	else if(machine_type==BT_TYPE_PROCESSOR_MACHINE) {
		return(gtk_image_new_from_pixbuf(ui_ressources->priv->processor_machine_pixbuf));
	}
	else if(machine_type==BT_TYPE_SINK_MACHINE) {
		return(gtk_image_new_from_pixbuf(ui_ressources->priv->sink_machine_pixbuf));
	}
	return(NULL);
}

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_ui_ressources_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtUIRessources *self = BT_UI_RESSOURCES(object);
  return_if_disposed();
  switch (property_id) {
    default: {
 			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_ui_ressources_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtUIRessources *self = BT_UI_RESSOURCES(object);
  return_if_disposed();
  switch (property_id) {
    default: {
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_ui_ressources_dispose(GObject *object) {
  BtUIRessources *self = BT_UI_RESSOURCES(object);
	return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
	
	g_object_try_unref(self->priv->source_machine_pixbuf);
	g_object_try_unref(self->priv->processor_machine_pixbuf);
	g_object_try_unref(self->priv->sink_machine_pixbuf);

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_ui_ressources_finalize(GObject *object) {
  BtUIRessources *self = BT_UI_RESSOURCES(object);
  
  GST_DEBUG("!!!! self=%p",self);
  g_free(self->priv);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_ui_ressources_init(GTypeInstance *instance, gpointer g_class) {
  BtUIRessources *self = BT_UI_RESSOURCES(instance);
  self->priv = g_new0(BtUIRessourcesPrivate,1);
  self->priv->dispose_has_run = FALSE;
}

static void bt_ui_ressources_class_init(BtUIRessourcesClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_ref(G_TYPE_OBJECT);

  gobject_class->set_property = bt_ui_ressources_set_property;
  gobject_class->get_property = bt_ui_ressources_get_property;
  gobject_class->dispose      = bt_ui_ressources_dispose;
  gobject_class->finalize     = bt_ui_ressources_finalize;
}

GType bt_ui_ressources_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      G_STRUCT_SIZE(BtUIRessourcesClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_ui_ressources_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtUIRessources),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_ui_ressources_init, // instance_init
			NULL // value_table
    };
		type = g_type_register_static(G_TYPE_OBJECT,"BtUIRessources",&info,0);
  }
  return type;
}
