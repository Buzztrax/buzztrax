/* $Id: machine-canvas-item.c,v 1.1 2004-10-13 16:05:15 ensonic Exp $
 * class for the editor machine views machine canvas item
 */

#define BT_EDIT
#define BT_MACHINE_CANVAS_ITEM_C

#include "bt-edit.h"

enum {
  MACHINE_CANVAS_ITEM_MACHINE=1,
};


struct _BtMachineCanvasItemPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* the application */
  BtMachine *machine;
  
};

static GnomeCanvasGroupClass *parent_class=NULL;

//-- event handler

//-- helper methods

//-- constructor methods

//-- methods

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_machine_canvas_item_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtMachineCanvasItem *self = BT_MACHINE_CANVAS_ITEM(object);
  return_if_disposed();
  switch (property_id) {
    case MACHINE_CANVAS_ITEM_MACHINE: {
      g_value_set_object(value, self->priv->machine);
    } break;
    default: {
 			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_machine_canvas_item_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtMachineCanvasItem *self = BT_MACHINE_CANVAS_ITEM(object);
  return_if_disposed();
  switch (property_id) {
    case MACHINE_CANVAS_ITEM_MACHINE: {
      g_object_try_unref(self->priv->machine);
      self->priv->machine = g_object_try_ref(g_value_get_object(value));
      //GST_DEBUG("set the machine for machine_canvas_item: %p",self->priv->machine);
    } break;
    default: {
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_machine_canvas_item_dispose(GObject *object) {
  BtMachineCanvasItem *self = BT_MACHINE_CANVAS_ITEM(object);
	return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  g_object_try_unref(self->priv->machine);
  // this disposes the pages for us
  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_machine_canvas_item_finalize(GObject *object) {
  BtMachineCanvasItem *self = BT_MACHINE_CANVAS_ITEM(object);
  
  g_free(self->priv);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

/**
 * bt_machine_canvas_item_realize:
 *
 * draw something that looks a bit like a buzz-machine
 */
static void bt_machine_canvas_item_realize(GnomeCanvasItem *citem) {
  BtMachineCanvasItem *self=BT_MACHINE_CANVAS_ITEM(citem);
  GnomeCanvasItem *item;
  gdouble w=12.0,h=7.0;
  guint bg_color=0xFFFFFFFF;
  gchar *id;
  
  if(GNOME_CANVAS_ITEM_CLASS(parent_class)->realize)
    (GNOME_CANVAS_ITEM_CLASS(parent_class)->realize)(citem);
  
  GST_DEBUG("realize for machine occured, machine=%p",self->priv->machine);

  // @todo that should be handled by subclassing
  if(BT_IS_SOURCE_MACHINE(self->priv->machine)) {
    bg_color=0xFFAFAFFF;
  }
  if(BT_IS_PROCESSOR_MACHINE(self->priv->machine)) {
    bg_color=0xAFFFAFFF;
  }
  if(BT_IS_SINK_MACHINE(self->priv->machine)) {
    bg_color=0xAFAFFFFF;
  }
  g_object_get(self->priv->machine,"id",&id,NULL);

  // add machine visualisation components
  item=gnome_canvas_item_new(GNOME_CANVAS_GROUP(citem),
                           GNOME_TYPE_CANVAS_RECT,
                           "x1", -w,
                           "y1", -h,
                           "x2", +w,
                           "y2", +h,
                           "fill-color-rgba", bg_color,
                           "outline_color", "black",
                           "width-pixels", 1,
                           NULL);
  item=gnome_canvas_item_new(GNOME_CANVAS_GROUP(citem),
                           GNOME_TYPE_CANVAS_TEXT,
                           "x", +0.0,
                           "y", -3.0,
                           "justification", GTK_JUSTIFY_CENTER,
                           "size-points", 10.0,
                           "size-set", TRUE,
                           "text", id,
                           "fill-color", "black",
                           NULL);
  g_free(id);
  //item->realized = TRUE;
}

static gboolean bt_machine_canvas_item_event(GnomeCanvasItem *citem, GdkEvent *event) {
  BtMachineCanvasItem *self=BT_MACHINE_CANVAS_ITEM(citem);

  //GST_DEBUG("event for machine occured");
  
  switch (event->type) {
    case GDK_BUTTON_PRESS:
      GST_DEBUG("GDK_BUTTON_PRESS: %d",event->button.button);
      break;
    case GDK_MOTION_NOTIFY:
      GST_DEBUG("GDK_MOTION_NOTIFY: %d,%d",event->button.x,event->button.y);
      break;
    case GDK_BUTTON_RELEASE:
      GST_DEBUG("GDK_BUTTON_RELEASE: %d",event->button.button);
      break;
    default:
      break;
  }
  /* we don't want the click falling through to the parent canvas item */
  return TRUE;
}

static void bt_machine_canvas_item_init(GTypeInstance *instance, gpointer g_class) {
  BtMachineCanvasItem *self = BT_MACHINE_CANVAS_ITEM(instance);
  self->priv = g_new0(BtMachineCanvasItemPrivate,1);
  self->priv->dispose_has_run = FALSE;
}

static void bt_machine_canvas_item_class_init(BtMachineCanvasItemClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GnomeCanvasItemClass *citem_class=GNOME_CANVAS_ITEM_CLASS(klass);

  parent_class=g_type_class_ref(GNOME_TYPE_CANVAS_GROUP);

  gobject_class->set_property = bt_machine_canvas_item_set_property;
  gobject_class->get_property = bt_machine_canvas_item_get_property;
  gobject_class->dispose      = bt_machine_canvas_item_dispose;
  gobject_class->finalize     = bt_machine_canvas_item_finalize;

  citem_class->realize        = bt_machine_canvas_item_realize;
  citem_class->event          = bt_machine_canvas_item_event;

  g_object_class_install_property(gobject_class,MACHINE_CANVAS_ITEM_MACHINE,
                                  g_param_spec_object("machine",
                                     "machine contruct prop",
                                     "Set machine object, the item belongs to",
                                     BT_TYPE_MACHINE, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));
}

GType bt_machine_canvas_item_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtMachineCanvasItemClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_machine_canvas_item_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtMachineCanvasItem),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_machine_canvas_item_init, // instance_init
    };
		type = g_type_register_static(GNOME_TYPE_CANVAS_GROUP,"BtMachineCanvasItem",&info,0);
  }
  return type;
}

