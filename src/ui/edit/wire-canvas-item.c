/* $Id: wire-canvas-item.c,v 1.2 2004-10-18 13:19:02 ensonic Exp $
 * class for the editor wire views wire canvas item
 */

#define BT_EDIT
#define BT_WIRE_CANVAS_ITEM_C

#include "bt-edit.h"

enum {
  WIRE_CANVAS_ITEM_WIRE=1,
  WIRE_CANVAS_ITEM_W,
  WIRE_CANVAS_ITEM_H,
	WIRE_CANVAS_ITEM_SRC,
	WIRE_CANVAS_ITEM_DST
};


struct _BtWireCanvasItemPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* the application */
  BtWire *wire;
  
  /* end-points of the wire, relative to the group x,y pos */
  gdouble w,h;
  
  /* source and dst machine canvas item */
  BtMachineCanvasItem *src,*dst;
  
  /* the parts of the wire item */
  GnomeCanvasItem *line,*triangle;
  
  /* interaction state */
  gboolean dragging,moved;
  gdouble offx,offy,dragx,dragy;

};

static GnomeCanvasGroupClass *parent_class=NULL;

//-- helper

static void wire_set_line_points(GnomeCanvasPoints *points,gdouble w,gdouble h) {
  points->coords[0]=0.0;
  points->coords[1]=0.0;
  points->coords[2]=w;
  points->coords[3]=h;
}

/**
 * add a triangle pointing from src to dest at the middle of the wire
 */
static void wire_set_triangle_points(GnomeCanvasPoints *points,gdouble w,gdouble h) {
  gdouble mid_x,mid_y;
  gdouble tip_x,tip_y;
  gdouble base_x,base_y;
  gdouble base1_x,base1_y;
  gdouble base2_x,base2_y;
  gdouble df,dx,dy,s=MACHINE_VIEW_WIRE_PAD_SIZE,sa,sb;

  // middle of wire
  mid_x=w/2.0;
  mid_y=h/2.0;
  // normalized ascent (gradient, slope) of the wire
  if((fabs(w)>G_MINDOUBLE) && (fabs(h)>G_MINDOUBLE)) {
    dx=w/h;
    dy=h/w;
    df=sqrt(w*w+h*h);dx=w/df;dy=h/df;
  }
  else if(fabs(w)>G_MINDOUBLE) {
    dx=1.0;
    dy=0.0;
    df=w;
  }
  else if(fabs(h)>G_MINDOUBLE) {
    dx=0.0;
    dy=1.0;
    df=h;
  }
  else {
    dx=dy=df=0.0;
  }
  // tip of triangle
  tip_x=mid_x+((s+s)*dx);
  tip_y=mid_y+((s+s)*dy);
  // intersection point of triangle base
  base_x=mid_x-(s*dx);
  base_y=mid_y-(s*dy);
  sa=3.0*s;
  sb=sa/sqrt(3.0);
  // point under the line
  base1_x=base_x-(sb*dy);
  base1_y=base_y+(sb*dx);
  // point over the line
  base2_x=base_x+(sb*dy);
  base2_y=base_y-(sb*dx);
  // debug
  /*
  GST_DEBUG(" delta=%f,%f, df=%f, s=%f, sa=%f sb=%f",dx,dy,df,s,sa,sb);
  GST_DEBUG(" w/h=%f,%f",w,h);
  GST_DEBUG(" mid=%f,%f",mid_x,mid_y);
  GST_DEBUG(" tip=%f,%f",tip_x,tip_y);
  GST_DEBUG(" base=%f,%f",base_x,base_y);
  GST_DEBUG(" base1=%f,%f",base1_x,base1_y);
  GST_DEBUG(" base2=%f,%f",base2_x,base2_y);
  */
  points->coords[0]=tip_x;
  points->coords[1]=tip_y;
  points->coords[2]=base1_x;
  points->coords[3]=base1_y;
  points->coords[4]=base2_x;
  points->coords[5]=base2_y;
}

//-- event handler

void on_wire_position_changed(BtMachineCanvasItem *machine_item, gpointer user_data) {
  BtWireCanvasItem *self=BT_WIRE_CANVAS_ITEM(user_data);
  BtMachine *src_machine,*dst_machine;
  GHashTable *properties;
  gdouble pos_xs,pos_ys,pos_xe,pos_ye;
  GnomeCanvasPoints *points;

  //GST_INFO("wire pos has changed : machine_item=%p, user_data=%p",machine_item,user_data);

  g_object_get(self->priv->src,"machine",&src_machine,NULL);
  g_object_get(src_machine,"properties",&properties,NULL);
  machine_view_get_machine_position(properties,&pos_xs,&pos_ys);
  g_object_get(self->priv->dst,"machine",&dst_machine,NULL);
  g_object_get(dst_machine,"properties",&properties,NULL);
  machine_view_get_machine_position(properties,&pos_xe,&pos_ye);

  //GST_INFO("  set new coords : %f,%f %f,%f",pos_xs,pos_ys,pos_xe,pos_ye);
  
  gnome_canvas_item_set(GNOME_CANVAS_ITEM(self),
                      "x", pos_xs,
                      "y", pos_ys,
                      "w", (pos_xe-pos_xs),
                      "h", (pos_ye-pos_ys),
                      NULL);
  // we need to reset all the coords for our wire items now
  points=gnome_canvas_points_new(2);
  wire_set_line_points(points,self->priv->w,self->priv->h);
  gnome_canvas_item_set(GNOME_CANVAS_ITEM(self->priv->line),"points",points,NULL);
  gnome_canvas_points_free(points);
  
  points=gnome_canvas_points_new(3);
  wire_set_triangle_points(points,self->priv->w,self->priv->h);
  gnome_canvas_item_set(GNOME_CANVAS_ITEM(self->priv->triangle),"points",points,NULL);
  gnome_canvas_points_free(points);
}

//-- helper methods

//-- constructor methods

//-- methods

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_wire_canvas_item_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtWireCanvasItem *self = BT_WIRE_CANVAS_ITEM(object);
  return_if_disposed();
  switch (property_id) {
    case WIRE_CANVAS_ITEM_WIRE: {
      g_value_set_object(value, self->priv->wire);
    } break;
    case WIRE_CANVAS_ITEM_W: {
      g_value_set_double(value, self->priv->w);
    } break;
    case WIRE_CANVAS_ITEM_H: {
      g_value_set_double(value, self->priv->h);
    } break;
    case WIRE_CANVAS_ITEM_SRC: {
      g_value_set_object(value, self->priv->src);
    } break;
    case WIRE_CANVAS_ITEM_DST: {
      g_value_set_object(value, self->priv->dst);
    } break;
    default: {
 			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_wire_canvas_item_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtWireCanvasItem *self = BT_WIRE_CANVAS_ITEM(object);
  return_if_disposed();
  switch (property_id) {
    case WIRE_CANVAS_ITEM_WIRE: {
      g_object_try_unref(self->priv->wire);
      self->priv->wire=g_object_try_ref(g_value_get_object(value));
      //GST_DEBUG("set the wire for wire_canvas_item: %p",self->priv->wire);
    } break;
    case WIRE_CANVAS_ITEM_W: {
      self->priv->w=g_value_get_double(value);
    } break;
    case WIRE_CANVAS_ITEM_H: {
      self->priv->h=g_value_get_double(value);
    } break;
    case WIRE_CANVAS_ITEM_SRC: {
      g_object_try_unref(self->priv->src);
      self->priv->src=g_object_try_ref(g_value_get_object(value));
      if(self->priv->src) {
        g_signal_connect(G_OBJECT(self->priv->src),"position-changed",(GCallback)on_wire_position_changed,(gpointer)self);
        GST_DEBUG("set the src for wire_canvas_item: %p",self->priv->src);
      }
    } break;
    case WIRE_CANVAS_ITEM_DST: {
      g_object_try_unref(self->priv->dst);
      self->priv->dst=g_object_try_ref(g_value_get_object(value));
      if(self->priv->dst) {
        g_signal_connect(G_OBJECT(self->priv->dst),"position-changed",(GCallback)on_wire_position_changed,(gpointer)self);
        GST_DEBUG("set the dst for wire_canvas_item: %p",self->priv->dst);
      }
    } break;
    default: {
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_wire_canvas_item_dispose(GObject *object) {
  BtWireCanvasItem *self = BT_WIRE_CANVAS_ITEM(object);
	return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  g_object_try_unref(self->priv->wire);
  g_object_try_unref(self->priv->src);
  g_object_try_unref(self->priv->dst);
  // this disposes the pages for us
  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_wire_canvas_item_finalize(GObject *object) {
  BtWireCanvasItem *self = BT_WIRE_CANVAS_ITEM(object);
  
  g_free(self->priv);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

/**
 * bt_wire_canvas_item_realize:
 *
 * draw something that looks a bit like a buzz-wire
 */
static void bt_wire_canvas_item_realize(GnomeCanvasItem *citem) {
  BtWireCanvasItem *self=BT_WIRE_CANVAS_ITEM(citem);
  GnomeCanvasItem *item;
  GnomeCanvasPoints *points;
  
  if(GNOME_CANVAS_ITEM_CLASS(parent_class)->realize)
    (GNOME_CANVAS_ITEM_CLASS(parent_class)->realize)(citem);
  
  GST_DEBUG("realize for wire occured, wire=%p : w=%f,h=%f",self->priv->wire,self->priv->w,self->priv->h);

  points=gnome_canvas_points_new(2);
  wire_set_line_points(points,self->priv->w,self->priv->h);
  self->priv->line=gnome_canvas_item_new(GNOME_CANVAS_GROUP(citem),
                           GNOME_TYPE_CANVAS_LINE,
                           "points", points,
                           "fill-color", "black",
                           "width-pixels", 1,
                           NULL);
  gnome_canvas_points_free(points);
  
  // set up polygon
  points=gnome_canvas_points_new(3);
  wire_set_triangle_points(points,self->priv->w,self->priv->h);
  
  self->priv->triangle=gnome_canvas_item_new(GNOME_CANVAS_GROUP(citem),
                           GNOME_TYPE_CANVAS_POLYGON,
                           "points", points,
                           "outline-color", "black",
                           "fill-color", "gray",
                           "width-pixels", 1,
                           NULL);
  gnome_canvas_points_free(points);
  //item->realized = TRUE;
}

static gboolean bt_wire_canvas_item_event(GnomeCanvasItem *citem, GdkEvent *event) {
  BtWireCanvasItem *self=BT_WIRE_CANVAS_ITEM(citem);

  //GST_DEBUG("event for wire occured");
  
  switch(event->type) {
    case GDK_BUTTON_PRESS:
      GST_DEBUG("GDK_BUTTON_PRESS: %d",event->button.button);
      break;
    case GDK_MOTION_NOTIFY:
      //GST_DEBUG("GDK_MOTION_NOTIFY: %f,%f",event->button.x,event->button.y);
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

static void bt_wire_canvas_item_init(GTypeInstance *instance, gpointer g_class) {
  BtWireCanvasItem *self = BT_WIRE_CANVAS_ITEM(instance);
  self->priv = g_new0(BtWireCanvasItemPrivate,1);
  self->priv->dispose_has_run = FALSE;
}

static void bt_wire_canvas_item_class_init(BtWireCanvasItemClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GnomeCanvasItemClass *citem_class=GNOME_CANVAS_ITEM_CLASS(klass);

  parent_class=g_type_class_ref(GNOME_TYPE_CANVAS_GROUP);

  gobject_class->set_property = bt_wire_canvas_item_set_property;
  gobject_class->get_property = bt_wire_canvas_item_get_property;
  gobject_class->dispose      = bt_wire_canvas_item_dispose;
  gobject_class->finalize     = bt_wire_canvas_item_finalize;

  citem_class->realize        = bt_wire_canvas_item_realize;
  citem_class->event          = bt_wire_canvas_item_event;

  g_object_class_install_property(gobject_class,WIRE_CANVAS_ITEM_WIRE,
                                  g_param_spec_object("wire",
                                     "wire contruct prop",
                                     "Set wire object, the item belongs to",
                                     BT_TYPE_WIRE, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

 	g_object_class_install_property(gobject_class,WIRE_CANVAS_ITEM_W,
																	g_param_spec_double("w",
                                     "width prop",
                                     "width of the wire",
                                     -10000.0,
                                     10000.0, // @todo what do we set here?
                                     1.0,
                                     G_PARAM_READWRITE));

 	g_object_class_install_property(gobject_class,WIRE_CANVAS_ITEM_H,
																	g_param_spec_double("h",
                                     "height prop",
                                     "height of the wire",
                                     -10000.0,
                                     10000.0, // @todo what do we set here?
                                     1.0,
                                     G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,WIRE_CANVAS_ITEM_SRC,
                                  g_param_spec_object("src",
                                     "src contruct prop",
                                     "Set wire src machine canvas item",
                                     BT_TYPE_MACHINE_CANVAS_ITEM, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,WIRE_CANVAS_ITEM_DST,
                                  g_param_spec_object("dst",
                                     "dst contruct prop",
                                     "Set wire dst machine canvas item",
                                     BT_TYPE_MACHINE_CANVAS_ITEM, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));
}

GType bt_wire_canvas_item_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtWireCanvasItemClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_wire_canvas_item_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtWireCanvasItem),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_wire_canvas_item_init, // instance_init
    };
		type = g_type_register_static(GNOME_TYPE_CANVAS_GROUP,"BtWireCanvasItem",&info,0);
  }
  return type;
}

