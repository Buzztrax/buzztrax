/* $Id: wire-canvas-item.c,v 1.49 2007-09-09 19:54:07 ensonic Exp $
 *
 * Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/**
 * SECTION:btwirecanvasitem
 * @short_description: class for the editor wire views wire canvas item
 * @see_also: #btwireanalysisdialog
 *
 * Provides volume control on the wires, as well as a menu to disconnect wires
 * and to launch the analyzer screen.
 */
/*
 * @todo:
 * - can we visualize the parameters on the wire better
 *   - the wire has volume
 *   - the wire can have 1/2 panorama sliders
 */
#define BT_EDIT
#define BT_WIRE_CANVAS_ITEM_C

#include "bt-edit.h"

//-- property ids

enum {
  WIRE_CANVAS_ITEM_APP=1,
  WIRE_CANVAS_ITEM_MACHINES_PAGE,
  WIRE_CANVAS_ITEM_WIRE,
  WIRE_CANVAS_ITEM_W,
  WIRE_CANVAS_ITEM_H,
  WIRE_CANVAS_ITEM_SRC,
  WIRE_CANVAS_ITEM_DST
};

// @todo what do we set here?
#define BT_WIRE_MAX_EXTEND 100000.0

struct _BtWireCanvasItemPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  G_POINTER_ALIAS(BtEditApplication *,app);
  /* the machine page we are on */
  G_POINTER_ALIAS(BtMainPageMachines *,main_page_machines);

  /* the underlying wire */
  BtWire *wire;
  /* and its properties */
  GHashTable *properties;

  /* end-points of the wire, relative to the group x,y pos */
  gdouble w,h;

  /* source and dst machine canvas item */
  BtMachineCanvasItem *src,*dst;

  /* the graphcal components */
  GnomeCanvasItem *line,*triangle;

  /* wire context_menu */
  GtkMenu *context_menu;

  /* the analysis dialog */
  GtkWidget *analysis_dialog;

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

/*
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

static void on_wire_analysis_dialog_destroy(GtkWidget *widget, gpointer user_data) {
  BtWireCanvasItem *self=BT_WIRE_CANVAS_ITEM(user_data);

  g_assert(user_data);

  GST_INFO("wire analysis dialog destroy occurred");
  self->priv->analysis_dialog=NULL;
  // remember open/closed state
  g_hash_table_remove(self->priv->properties,"analyzer-shown");
}

static void on_machine_removed(BtSetup *setup,BtMachine *machine,gpointer user_data) {
  BtWireCanvasItem *self=BT_WIRE_CANVAS_ITEM(user_data);
  BtMachine *src,*dst;

  g_assert(user_data);
  g_return_if_fail(BT_IS_MACHINE(machine));

  GST_INFO("machine %p,machine->ref_ct=%d has been removed",machine,G_OBJECT(machine)->ref_count);

  g_object_get(self->priv->src,"machine",&src,NULL);
  g_object_get(self->priv->dst,"machine",&dst,NULL);

  GST_INFO("... machine %p,machine->ref_ct=%d has been removed, checking wire %p->%p",machine,G_OBJECT(machine)->ref_count,src,dst);
  if((src==machine) || (dst==machine)) {
    GST_INFO("the machine, this wire is connected to, has been removed");
    bt_setup_remove_wire(setup,self->priv->wire);
    bt_main_page_machines_remove_wire_item(self->priv->main_page_machines,self);
  }
  GST_INFO("... machine %p,ref_count=%d has been removed, src %p,ref=%d, dst %p,ref=%d",
    machine,G_OBJECT(machine)->ref_count,
    src,G_OBJECT(src)->ref_count,
    dst,G_OBJECT(dst)->ref_count
  );
  g_object_try_unref(src);
  g_object_try_unref(dst);
}

static void on_wire_position_changed(BtMachineCanvasItem *machine_item, gpointer user_data) {
  BtWireCanvasItem *self=BT_WIRE_CANVAS_ITEM(user_data);
  BtMachine *machine;
  GHashTable *properties;
  gdouble pos_xs,pos_ys,pos_xe,pos_ye;
  GnomeCanvasPoints *points;

  //GST_INFO("wire pos has changed : machine_item=%p, user_data=%p",machine_item,user_data);

  g_object_get(self->priv->src,"machine",&machine,NULL);
  g_object_get(machine,"properties",&properties,NULL);
  machine_view_get_machine_position(properties,&pos_xs,&pos_ys);
  g_object_unref(machine);
  g_object_get(self->priv->dst,"machine",&machine,NULL);
  g_object_get(machine,"properties",&properties,NULL);
  machine_view_get_machine_position(properties,&pos_xe,&pos_ye);
  g_object_unref(machine);

  //GST_INFO("  set new coords: %+5.1f,%+5.1f %+5.1f,%+5.1f",pos_xs,pos_ys,pos_xe,pos_ye);

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

static void on_context_menu_disconnect_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtWireCanvasItem *self=BT_WIRE_CANVAS_ITEM(user_data);
  BtSong *song;
  BtSetup *setup;

  g_assert(user_data);
  GST_INFO("context_menu disconnect item selected");

  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_object_get(G_OBJECT(song),"setup",&setup,NULL);

  bt_setup_remove_wire(setup,self->priv->wire);
  bt_main_page_machines_remove_wire_item(self->priv->main_page_machines,self);

  g_object_try_unref(setup);
  g_object_try_unref(song);
}

static void on_context_menu_analysis_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtWireCanvasItem *self=BT_WIRE_CANVAS_ITEM(user_data);

  g_assert(user_data);
  GST_INFO("context_menu analysis item selected");
  if(!self->priv->analysis_dialog) {
    if((self->priv->analysis_dialog=GTK_WIDGET(bt_wire_analysis_dialog_new(self->priv->app,self->priv->wire)))) {
      GST_INFO("analyzer dialog opened");
      // remember open/closed state
      g_hash_table_insert(self->priv->properties,g_strdup("analyzer-shown"),g_strdup("1"));
      g_signal_connect(G_OBJECT(self->priv->analysis_dialog),"destroy",G_CALLBACK(on_wire_analysis_dialog_destroy),(gpointer)self);
    }
  }
  else {
    gtk_window_present(GTK_WINDOW(self->priv->analysis_dialog));
  }
}
//-- helper methods

//-- constructor methods

/**
 * bt_wire_canvas_item_new:
 * @main_page_machines: the machine page the new item belongs to
 * @wire: the wire for which a canvas item should be created
 * @pos_xs: the horizontal start location
 * @pos_ys: the vertical start location
 * @pos_xe: the horizontal end location
 * @pos_ye: the vertical end location
 * @src_machine_item: the machine item at start
 * @dst_machine_item: the machine item at end
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtWireCanvasItem *bt_wire_canvas_item_new(const BtMainPageMachines *main_page_machines,BtWire *wire,gdouble pos_xs,gdouble pos_ys,gdouble pos_xe,gdouble pos_ye,BtMachineCanvasItem *src_machine_item,BtMachineCanvasItem *dst_machine_item) {
  BtWireCanvasItem *self;
  BtEditApplication *app;
  GnomeCanvas *canvas;
  BtSong *song;
  BtSetup *setup;
  gdouble w,h;

  g_object_get(G_OBJECT(main_page_machines),"app",&app,"canvas",&canvas,NULL);
  g_object_get(G_OBJECT(app),"song",&song,NULL);
  g_object_get(G_OBJECT(song),"setup",&setup,NULL);

  w=(pos_xe-pos_xs);
  h=(pos_ye-pos_ys);
  /* @todo: if we clip agains BT_WIRE_MAX_EXTEND here, the wire will look bad */

  self=BT_WIRE_CANVAS_ITEM(gnome_canvas_item_new(gnome_canvas_root(canvas),
                            BT_TYPE_WIRE_CANVAS_ITEM,
                            "machines-page",main_page_machines,
                            "app", app,
                            "wire", wire,
                            "x", pos_xs,
                            "y", pos_ys,
                            "w", w,
                            "h", h,
                            "src", src_machine_item,
                            "dst", dst_machine_item,
                            NULL));
  gnome_canvas_item_lower_to_bottom(GNOME_CANVAS_ITEM(self));
  g_signal_connect(G_OBJECT(setup),"machine-removed",G_CALLBACK(on_machine_removed),(gpointer)self);

  //GST_INFO("wire canvas item added");

  g_object_try_unref(setup);
  g_object_try_unref(song);
  g_object_try_unref(canvas);
  g_object_try_unref(app);
  return(self);
}

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
    case WIRE_CANVAS_ITEM_APP: {
      g_value_set_object(value, self->priv->app);
    } break;
    case WIRE_CANVAS_ITEM_MACHINES_PAGE: {
      g_value_set_object(value, self->priv->main_page_machines);
    } break;
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
    case WIRE_CANVAS_ITEM_APP: {
      g_object_try_weak_unref(self->priv->app);
      self->priv->app=BT_EDIT_APPLICATION(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->app);
      //GST_DEBUG("set the app for wire_canvas_item: %p",self->priv->app);
    } break;
    case WIRE_CANVAS_ITEM_MACHINES_PAGE: {
      g_object_try_weak_unref(self->priv->main_page_machines);
      self->priv->main_page_machines = BT_MAIN_PAGE_MACHINES(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->main_page_machines);
      //GST_DEBUG("set the main_page_machines for wire_canvas_item: %p",self->priv->main_page_machines);
    } break;
    case WIRE_CANVAS_ITEM_WIRE: {
      g_object_try_unref(self->priv->wire);
      self->priv->wire=BT_WIRE(g_value_dup_object(value));
      if(self->priv->wire) {
        //GST_DEBUG("set the wire for wire_canvas_item: %p",self->priv->wire);
        g_object_get(self->priv->wire,"properties",&(self->priv->properties),NULL);
      }
    } break;
    case WIRE_CANVAS_ITEM_W: {
      self->priv->w=g_value_get_double(value);
    } break;
    case WIRE_CANVAS_ITEM_H: {
      self->priv->h=g_value_get_double(value);
    } break;
    case WIRE_CANVAS_ITEM_SRC: {
      g_object_try_unref(self->priv->src);
      self->priv->src=BT_MACHINE_CANVAS_ITEM(g_value_dup_object(value));
      if(self->priv->src) {
        g_signal_connect(G_OBJECT(self->priv->src),"position-changed",G_CALLBACK(on_wire_position_changed),(gpointer)self);
        GST_DEBUG("set the src for wire_canvas_item: %p",self->priv->src);
      }
    } break;
    case WIRE_CANVAS_ITEM_DST: {
      g_object_try_unref(self->priv->dst);
      self->priv->dst=BT_MACHINE_CANVAS_ITEM(g_value_dup_object(value));
      if(self->priv->dst) {
        g_signal_connect(G_OBJECT(self->priv->dst),"position-changed",G_CALLBACK(on_wire_position_changed),(gpointer)self);
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
  BtSong *song;
  BtSetup *setup;

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  if(self->priv->src) {
    g_signal_handlers_disconnect_matched(G_OBJECT(self->priv->src),G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_wire_position_changed,(gpointer)self);
  }
  if(self->priv->dst) {
    g_signal_handlers_disconnect_matched(G_OBJECT(self->priv->dst),G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_wire_position_changed,(gpointer)self);
  }
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_object_get(G_OBJECT(song),"setup",&setup,NULL);
  g_signal_handlers_disconnect_matched(G_OBJECT(setup),G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_machine_removed,(gpointer)self);
  g_object_try_unref(setup);
  g_object_try_unref(song);
  GST_DEBUG("  signal disconected");

  g_object_try_weak_unref(self->priv->app);
  g_object_try_unref(self->priv->wire);
  g_object_try_unref(self->priv->src);
  g_object_try_unref(self->priv->dst);
  g_object_try_weak_unref(self->priv->main_page_machines);

  GST_DEBUG("  unrefing done");

  if(self->priv->analysis_dialog) {
    gtk_widget_destroy(self->priv->analysis_dialog);
  }

  gtk_object_destroy(GTK_OBJECT(self->priv->context_menu));

  GST_DEBUG("  chaining up");
  G_OBJECT_CLASS(parent_class)->dispose(object);
  GST_DEBUG("  done");
}

static void bt_wire_canvas_item_finalize(GObject *object) {
  //BtWireCanvasItem *self = BT_WIRE_CANVAS_ITEM(object);

  //GST_DEBUG("!!!! self=%p",self);

  G_OBJECT_CLASS(parent_class)->finalize(object);
  GST_DEBUG("  done");
}

/*
 * bt_wire_canvas_item_realize:
 *
 * draw something that looks a bit like a buzz-wire
 */
static void bt_wire_canvas_item_realize(GnomeCanvasItem *citem) {
  BtWireCanvasItem *self=BT_WIRE_CANVAS_ITEM(citem);
  GnomeCanvasPoints *points;
  gchar *prop;

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

  prop=(gchar *)g_hash_table_lookup(self->priv->properties,"analyzer-shown");
  if(prop && prop[0]=='1' && prop[1]=='\0') {
    if((self->priv->analysis_dialog=GTK_WIDGET(bt_wire_analysis_dialog_new(self->priv->app,self->priv->wire)))) {
      g_signal_connect(G_OBJECT(self->priv->analysis_dialog),"destroy",G_CALLBACK(on_wire_analysis_dialog_destroy),(gpointer)self);
    }
  }

  //item->realized = TRUE;
}

static gboolean bt_wire_canvas_item_event(GnomeCanvasItem *citem, GdkEvent *event) {
  BtWireCanvasItem *self=BT_WIRE_CANVAS_ITEM(citem);
  gboolean res=FALSE;

  //GST_DEBUG("event for wire occured");

  switch(event->type) {
    case GDK_BUTTON_PRESS:
      GST_DEBUG("GDK_BUTTON_PRESS: %d",event->button.button);
      if(event->button.button==1) {
        GST_INFO("showing volume-popup at %lf,%lf  %lf,%lf",
          event->button.x,event->button.y,
          event->button.x_root,event->button.y_root);
        bt_main_page_machines_wire_volume_popup(self->priv->main_page_machines, self->priv->wire, (gint)event->button.x_root, (gint)event->button.y_root);
        res=TRUE;
      }
      else if(event->button.button==3) {
        // show context menu
        gtk_menu_popup(self->priv->context_menu,NULL,NULL,NULL,NULL,3,gtk_get_current_event_time());
        res=TRUE;
      }
      break;
    case GDK_MOTION_NOTIFY:
      //GST_DEBUG("GDK_MOTION_NOTIFY: %f,%f",event->button.x,event->button.y);
      break;
    case GDK_BUTTON_RELEASE:
      GST_DEBUG("GDK_BUTTON_RELEASE: %d",event->button.button);
      break;
    case GDK_KEY_RELEASE:
      GST_DEBUG("GDK_KEY_RELEASE: %d",event->key.keyval);
      switch(event->key.keyval) {
        case GDK_Menu:
          // show context menu
          gtk_menu_popup(self->priv->context_menu,NULL,NULL,NULL,NULL,3,gtk_get_current_event_time());
          res=TRUE;
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
  /* we don't want the click falling through to the parent canvas item, if we have handled it */
  if(!res) {
    if(GNOME_CANVAS_ITEM_CLASS(parent_class)->event) {
      res=(GNOME_CANVAS_ITEM_CLASS(parent_class)->event)(citem,event);
    }
  }
  return res;
}

static void bt_wire_canvas_item_init(GTypeInstance *instance, gpointer g_class) {
  BtWireCanvasItem *self = BT_WIRE_CANVAS_ITEM(instance);
  GtkWidget *menu_item;

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_WIRE_CANVAS_ITEM, BtWireCanvasItemPrivate);

  // generate the context menu
  self->priv->context_menu=GTK_MENU(gtk_menu_new());

  menu_item=gtk_menu_item_new_with_label(_("Disconnect"));
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_show(menu_item);
  g_signal_connect(G_OBJECT(menu_item),"activate",G_CALLBACK(on_context_menu_disconnect_activate),(gpointer)self);

  menu_item=gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_set_sensitive(menu_item,FALSE);
  gtk_widget_show(menu_item);

  menu_item=gtk_menu_item_new_with_label(_("Signal Analysis ..."));
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_show(menu_item);
  g_signal_connect(G_OBJECT(menu_item),"activate",G_CALLBACK(on_context_menu_analysis_activate),(gpointer)self);

}

static void bt_wire_canvas_item_class_init(BtWireCanvasItemClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GnomeCanvasItemClass *citem_class=GNOME_CANVAS_ITEM_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtWireCanvasItemPrivate));

  gobject_class->set_property = bt_wire_canvas_item_set_property;
  gobject_class->get_property = bt_wire_canvas_item_get_property;
  gobject_class->dispose      = bt_wire_canvas_item_dispose;
  gobject_class->finalize     = bt_wire_canvas_item_finalize;

  citem_class->realize        = bt_wire_canvas_item_realize;
  citem_class->event          = bt_wire_canvas_item_event;

  g_object_class_install_property(gobject_class,WIRE_CANVAS_ITEM_APP,
                                  g_param_spec_object("app",
                                     "app contruct prop",
                                     "Set application object, the window belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
#ifndef GNOME_CANVAS_BROKEN_PROPERTIES
                                     G_PARAM_CONSTRUCT_ONLY |
#endif
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,WIRE_CANVAS_ITEM_MACHINES_PAGE,
                                  g_param_spec_object("machines-page",
                                     "machines-page contruct prop",
                                     "Set application object, the window belongs to",
                                     BT_TYPE_MAIN_PAGE_MACHINES, /* object type */
#ifndef GNOME_CANVAS_BROKEN_PROPERTIES
                                     G_PARAM_CONSTRUCT_ONLY |
#endif
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,WIRE_CANVAS_ITEM_WIRE,
                                  g_param_spec_object("wire",
                                     "wire contruct prop",
                                     "Set wire object, the item belongs to",
                                     BT_TYPE_WIRE, /* object type */
#ifndef GNOME_CANVAS_BROKEN_PROPERTIES
                                     G_PARAM_CONSTRUCT_ONLY |
#endif
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

   g_object_class_install_property(gobject_class,WIRE_CANVAS_ITEM_W,
                                  g_param_spec_double("w",
                                     "width prop",
                                     "width of the wire",
                                     -BT_WIRE_MAX_EXTEND,
                                     BT_WIRE_MAX_EXTEND,
                                     1.0,
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

   g_object_class_install_property(gobject_class,WIRE_CANVAS_ITEM_H,
                                  g_param_spec_double("h",
                                     "height prop",
                                     "height of the wire",
                                     -BT_WIRE_MAX_EXTEND,
                                     BT_WIRE_MAX_EXTEND,
                                     1.0,
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,WIRE_CANVAS_ITEM_SRC,
                                  g_param_spec_object("src",
                                     "src contruct prop",
                                     "Set wire src machine canvas item",
                                     BT_TYPE_MACHINE_CANVAS_ITEM, /* object type */
#ifndef GNOME_CANVAS_BROKEN_PROPERTIES
                                     G_PARAM_CONSTRUCT_ONLY |
#endif
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,WIRE_CANVAS_ITEM_DST,
                                  g_param_spec_object("dst",
                                     "dst contruct prop",
                                     "Set wire dst machine canvas item",
                                     BT_TYPE_MACHINE_CANVAS_ITEM, /* object type */
#ifndef GNOME_CANVAS_BROKEN_PROPERTIES
                                     G_PARAM_CONSTRUCT_ONLY |
#endif
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));
}

GType bt_wire_canvas_item_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      sizeof(BtWireCanvasItemClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_wire_canvas_item_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof(BtWireCanvasItem),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_wire_canvas_item_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(GNOME_TYPE_CANVAS_GROUP,"BtWireCanvasItem",&info,0);
  }
  return type;
}
