/* Buzztard
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
 * SECTION:btmainpagemachines
 * @short_description: the editor main machines page
 * @see_also: #BtSetup
 *
 * Displays the machine setup and wires on a canvas.
 */
/* TODO(ensonic): multiselect
 * - when clicking on the background
 *   - remove old selection if any
 *   - start new selection (semi-transparent rect)
 * - have context menu on the selection
 *   - delete
 *   - mute/solo/bypass ?
 * - move machines when moving the selection
 * - selected machines have a glowing border (would need another set of images)
 */
/* TODO(ensonic): move functions in context menu
 * - machines
 *   - clone machine (no patterns)
 *   - remove & relink (remove machine and relink wires)
 *     does not work in all scenarios (we might need to create more wires)
 *     A --\         /-- D
 *          +-- C --+
 *     B --/         \-- E
 *
 * - wires
 *   - insert machine (like menu on canvas)
 *     - what to do with wire-patterns?
 */
/* TODO(ensonic): easier machine manipulation
 * linking the machines with "shift" pressed comes from buzz and is hard to
 * discover. We now have a "connect machines" item in the connect menu, but
 * what about these:
 * 1.) on the toolbar I could have a button that toggles between "move" and
 *     "link". In "move" mode one can move machines with the mouse and in "link"
 *     mode one can link. Would need to have a keyboard shortcut for toggling.
 * 2.) click-zones on the machine-icons. Link the machine when click+drag the
 *     title bar. Make the 'leds' clickable and move the machines outside of
 *     title and led. Eventualy change the mouse-cursor when hovering over the
 *     zones.
 * Option '2' looks nice and would also help on touch-screens.
 */
#define BT_EDIT
#define BT_MAIN_PAGE_MACHINES_C

#include "bt-edit.h"

enum {
  MAIN_PAGE_MACHINES_CANVAS=1
};

struct _BtMainPageMachinesPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* the toolbar widget */
  GtkWidget *toolbar;
  
  /* the setup of machines and wires */
  BtSetup *setup;

  /* canvas for machine view */
  GnomeCanvas *canvas;
  GtkAdjustment *hadjustment,*vadjustment;
  /* canvas background grid */
  GnomeCanvasItem *grid;

  /* the zoomration in pixels/per unit */
  gdouble zoom;
  /* zomm in/out widgets */
  GtkWidget *zoom_in,*zoom_out;

  /* canvas context_menu */
  GtkMenu *context_menu;

  /* grid density menu */
  GtkMenu *grid_density_menu;
  GSList *grid_density_group;
  /* grid density */
  gulong grid_density;

  /* we probably need a list of canvas items that we have drawn, so that we can
   * easily clear them later
   */
  GHashTable *machines;  // each entry points to BtMachineCanvasItem
  GHashTable *wires;      // each entry points to BtWireCanvasItem

  /* interaction state */
  gboolean connecting,moved,dragging;

  /* cursor for moving */
  GdkCursor *drag_cursor;

  /* used when interactivly adding a new wire*/
  GnomeCanvasItem *new_wire;
  GnomeCanvasPoints *new_wire_points;
  BtMachineCanvasItem *new_wire_src,*new_wire_dst;

  /* cached setup properties */
  GHashTable *properties;

  /* mouse coodinates on context menu invokation (used for placing new machines) */
  gdouble mouse_x,mouse_y;
  /* machine coordinates before/after draging (needed for undo) */
  gdouble machine_xo,machine_yo;
  gdouble machine_xn,machine_yn;
  BtMachineCanvasItem *moving_machine_item;

  /* volume/panorama popup slider */
  BtVolumePopup *vol_popup;
  BtPanoramaPopup *pan_popup;
  GtkObject *vol_popup_adj, *pan_popup_adj;
  GstElement *wire_gain,*wire_pan;

  /* relative scrollbar position */
  gdouble scroll_x,scroll_y;

  /* editor change log */
  BtChangeLog *change_log;
};

//-- the class

static void bt_main_page_machines_change_logger_interface_init(gpointer const g_iface, gconstpointer const iface_data);

G_DEFINE_TYPE_WITH_CODE (BtMainPageMachines, bt_main_page_machines, GTK_TYPE_VBOX,
  G_IMPLEMENT_INTERFACE (BT_TYPE_CHANGE_LOGGER,
    bt_main_page_machines_change_logger_interface_init));

enum {
  METHOD_ADD_MACHINE=0,
  METHOD_REM_MACHINE,
  METHOD_SET_MACHINE_PROPERTY,
  METHOD_ADD_WIRE,
  METHOD_REM_WIRE,
  METHOD_SET_WIRE_PROPERTY,
};

static BtChangeLoggerMethods change_logger_methods[] = {
  BT_CHANGE_LOGGER_METHOD("add_machine",12,"([0-9]),\"([-_a-zA-Z0-9 ]+)\",\"([-_a-zA-Z0-9 ]+)\"$"),
  BT_CHANGE_LOGGER_METHOD("rem_machine",12,"\"([-_a-zA-Z0-9 ]+)\"$"),
  BT_CHANGE_LOGGER_METHOD("set_machine_property",21,"\"([-_a-zA-Z0-9 ]+)\",\"([-_a-zA-Z0-9]+)\",\"(.+)\"$"),
  BT_CHANGE_LOGGER_METHOD("add_wire",9,"\"([-_a-zA-Z0-9 ]+)\",\"([-_a-zA-Z0-9 ]+)\"$"),
  BT_CHANGE_LOGGER_METHOD("rem_wire",9,"\"([-_a-zA-Z0-9 ]+)\",\"([-_a-zA-Z0-9 ]+)\"$"),
  BT_CHANGE_LOGGER_METHOD("set_wire_property",18,"\"([-_a-zA-Z0-9 ]+)\",\"([-_a-zA-Z0-9 ]+)\",\"([-_a-zA-Z0-9]+)\",\"(.+)\"$"),
  { NULL, }
};

//-- data helper

static gboolean canvas_item_destroy(gpointer key,gpointer value,gpointer user_data) {
  gtk_object_destroy(GTK_OBJECT(value));
  return(TRUE);
}

//-- linking signal handler & helper

static void start_connect(BtMainPageMachines *self) {
  // handle drawing a new wire
  self->priv->new_wire_points=gnome_canvas_points_new(2);
  self->priv->new_wire_points->coords[0]=self->priv->mouse_x;
  self->priv->new_wire_points->coords[1]=self->priv->mouse_y;
  self->priv->new_wire_points->coords[2]=self->priv->mouse_x;
  self->priv->new_wire_points->coords[3]=self->priv->mouse_y;
  self->priv->new_wire=gnome_canvas_item_new(gnome_canvas_root(self->priv->canvas),
               GNOME_TYPE_CANVAS_LINE,
               "points", self->priv->new_wire_points,
               "fill-color", "red",
               "width-pixels", 1,
               NULL);
  gnome_canvas_item_lower_to_bottom(self->priv->new_wire);
  gnome_canvas_item_lower_to_bottom(self->priv->grid);
  self->priv->connecting=TRUE;
  self->priv->moved=FALSE;
}

static void on_machine_item_start_connect(BtMachineCanvasItem *machine_item, gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);

  self->priv->new_wire_src=g_object_ref(machine_item);
  g_object_get(machine_item,"x",&self->priv->mouse_x,"y",&self->priv->mouse_y,NULL);
  start_connect(self);
}

//-- event handler helper

static void machine_item_moved(const BtMainPageMachines *self, BtMachineCanvasItem *machine_item) {
  BtMachine *machine;
  gchar *undo_str,*redo_str;
  gchar *mid;
  gchar str[G_ASCII_DTOSTR_BUF_SIZE];

  g_object_get(machine_item,"machine",&machine,NULL);
  g_object_get(machine,"id",&mid,NULL);

  bt_change_log_start_group(self->priv->change_log);

  undo_str = g_strdup_printf("set_machine_property \"%s\",\"xpos\",\"%s\"",mid,g_ascii_dtostr(str,G_ASCII_DTOSTR_BUF_SIZE,self->priv->machine_xo));
  redo_str = g_strdup_printf("set_machine_property \"%s\",\"xpos\",\"%s\"",mid,g_ascii_dtostr(str,G_ASCII_DTOSTR_BUF_SIZE,self->priv->machine_xn));
  bt_change_log_add(self->priv->change_log,BT_CHANGE_LOGGER(self),undo_str,redo_str);
  undo_str = g_strdup_printf("set_machine_property \"%s\",\"ypos\",\"%s\"",mid,g_ascii_dtostr(str,G_ASCII_DTOSTR_BUF_SIZE,self->priv->machine_yo));
  redo_str = g_strdup_printf("set_machine_property \"%s\",\"ypos\",\"%s\"",mid,g_ascii_dtostr(str,G_ASCII_DTOSTR_BUF_SIZE,self->priv->machine_yn));
  bt_change_log_add(self->priv->change_log,BT_CHANGE_LOGGER(self),undo_str,redo_str);

  bt_change_log_end_group(self->priv->change_log);

  g_free(mid);
  g_object_unref(machine);
}

// TODO(ensonic): this method probably should go to BtMachine, but on the other hand it is GUI related
static gboolean machine_view_get_machine_position(GHashTable *properties, gdouble *pos_x,gdouble *pos_y) {
  gboolean res=FALSE;
  gchar *prop;

  *pos_x=*pos_y=0.0;
  if(properties) {
    prop=(gchar *)g_hash_table_lookup(properties,"xpos");
    if(prop) {
      *pos_x=MACHINE_VIEW_ZOOM_X*g_ascii_strtod(prop,NULL);
      // do not g_free(prop);
      //GST_DEBUG("  xpos: %+5.1f  %p=\"%s\"",*pos_x,prop,prop);
      res=TRUE;
    }
    else GST_WARNING("no xpos property found");
    prop=(gchar *)g_hash_table_lookup(properties,"ypos");
    if(prop) {
      *pos_y=MACHINE_VIEW_ZOOM_Y*g_ascii_strtod(prop,NULL);
      // do not g_free(prop);
      //GST_DEBUG("  ypos: %+5.1f  %p=\"%s\"",*pos_y,prop,prop);
      res&=TRUE;
    }
    else GST_WARNING("no ypos property found");
  }
  else GST_WARNING("no properties supplied");
  return(res);
}

/*
 * update_machine_zoom:
 *
 * workaround for gnome_canvas bug, that fails to change font-sizes when zooming
 */
static void update_machine_zoom(gpointer key,gpointer value,gpointer user_data) {
  g_object_set(BT_MACHINE_CANVAS_ITEM(value),"zoom",(*(gdouble*)user_data),NULL);
}

static void update_machines_zoom(const BtMainPageMachines *self) {
  gchar str[G_ASCII_DTOSTR_BUF_SIZE];

  g_hash_table_insert(self->priv->properties,g_strdup("zoom"),g_strdup(g_ascii_dtostr(str,G_ASCII_DTOSTR_BUF_SIZE,self->priv->zoom)));
  bt_edit_application_set_song_unsaved(self->priv->app);

  g_hash_table_foreach(self->priv->machines,update_machine_zoom,&self->priv->zoom);
  gtk_widget_set_sensitive(self->priv->zoom_out,(self->priv->zoom>0.4));
  gtk_widget_set_sensitive(self->priv->zoom_in,(self->priv->zoom<3.0));
}

static void machine_item_new(const BtMainPageMachines *self,BtMachine *machine,gdouble xpos,gdouble ypos) {
  BtMachineCanvasItem *item;

  item=bt_machine_canvas_item_new(self,machine,xpos,ypos,self->priv->zoom);
  g_hash_table_insert(self->priv->machines,machine,item);
  g_signal_connect(item,"start-connect",G_CALLBACK(on_machine_item_start_connect),(gpointer)self);
}

static void wire_item_new(const BtMainPageMachines *self,BtWire *wire,gdouble pos_xs,gdouble pos_ys,gdouble pos_xe,gdouble pos_ye,BtMachineCanvasItem *src_machine_item,BtMachineCanvasItem *dst_machine_item) {
  BtWireCanvasItem *item;

  item=bt_wire_canvas_item_new(self,wire,pos_xs,pos_ys,pos_xe,pos_ye,src_machine_item,dst_machine_item);
  g_hash_table_insert(self->priv->wires,wire,item);
}

static void machine_view_clear(const BtMainPageMachines *self) {
  // clear the canvas
  GST_DEBUG("before destroying machine canvas items");
  g_hash_table_foreach_remove(self->priv->machines,canvas_item_destroy,NULL);
  GST_DEBUG("before destoying wire canvas items");
  g_hash_table_foreach_remove(self->priv->wires,canvas_item_destroy,NULL);
  GST_DEBUG("done");
}

static void machine_view_refresh(const BtMainPageMachines *self) {
  GHashTable *properties;
  BtMachineCanvasItem *src_machine_item,*dst_machine_item;
  BtMachine *machine,*src_machine,*dst_machine;
  BtWire *wire;
  gdouble pos_x,pos_y;
  gdouble pos_xs,pos_ys,pos_xe,pos_ye;
  GList *node,*list;
  gchar *prop;

  machine_view_clear(self);

  // update view
  if((prop=(gchar *)g_hash_table_lookup(self->priv->properties,"zoom"))) {
    self->priv->zoom=g_ascii_strtod(prop,NULL);
    gnome_canvas_set_pixels_per_unit(self->priv->canvas,self->priv->zoom);
    GST_INFO("set zoom to %6.4lf",self->priv->zoom);
  }
  if((prop=(gchar *)g_hash_table_lookup(self->priv->properties,"xpos"))) {
    gtk_adjustment_set_value(self->priv->hadjustment,g_ascii_strtod(prop,NULL));
  }
  else {
    gdouble xs,xe,xp;
    // center
    g_object_get(self->priv->hadjustment,"lower",&xs,"upper",&xe,"page-size",&xp,NULL);
    gtk_adjustment_set_value(self->priv->hadjustment,xs+((xe-xs-xp)*0.5));
  }
  if((prop=(gchar *)g_hash_table_lookup(self->priv->properties,"ypos"))) {
    gtk_adjustment_set_value(self->priv->vadjustment,g_ascii_strtod(prop,NULL));
  }
  else {
    gdouble ys,ye,yp;
    // center
    g_object_get(self->priv->vadjustment,"lower",&ys,"upper",&ye,"page-size",&yp,NULL);
    gtk_adjustment_set_value(self->priv->vadjustment,ys+((ye-ys-yp)*0.5));
  }
  
  GST_INFO("creating machine canvas items");

  // draw all machines
  g_object_get(self->priv->setup,"machines",&list,NULL);
  for(node=list;node;node=g_list_next(node)) {
    machine=BT_MACHINE(node->data);
    // get position
    g_object_get(machine,"properties",&properties,NULL);
    machine_view_get_machine_position(properties,&pos_x,&pos_y);
    // draw machine
    machine_item_new(self,machine,pos_x,pos_y);
  }
  g_list_free(list);

  // draw all wires
  g_object_get(self->priv->setup,"wires",&list,NULL);
  for(node=list;node;node=g_list_next(node)) {
    wire=BT_WIRE(node->data);
    // get positions of source and dest
    g_object_get(wire,"src",&src_machine,"dst",&dst_machine,NULL);
    g_object_get(src_machine,"properties",&properties,NULL);
    machine_view_get_machine_position(properties,&pos_xs,&pos_ys);
    g_object_get(dst_machine,"properties",&properties,NULL);
    machine_view_get_machine_position(properties,&pos_xe,&pos_ye);
    src_machine_item=g_hash_table_lookup(self->priv->machines,src_machine);
    dst_machine_item=g_hash_table_lookup(self->priv->machines,dst_machine);
    // draw wire
    wire_item_new(self,wire,pos_xs,pos_ys,pos_xe,pos_ye,src_machine_item,dst_machine_item);
    g_object_unref(src_machine);
    g_object_unref(dst_machine);
    // TODO(ensonic): get "analyzer-window-state" and if set,
    // get xpos, ypos and open window
  }
  g_list_free(list);
  gnome_canvas_item_lower_to_bottom(self->priv->grid);
  GST_DEBUG("drawing done");
}

static void bt_main_page_machines_draw_grid(const BtMainPageMachines *self) {
  GnomeCanvasPoints *points;
  gdouble s,step;
  gulong color;

  GST_INFO("redrawing grid : density=%lu  canvas=%p",self->priv->grid_density,self->priv->canvas);

  // delete old grid-item and generate a new one (pushing it to bottom)
  if(self->priv->grid) gtk_object_destroy(GTK_OBJECT(self->priv->grid));
  self->priv->grid=gnome_canvas_item_new(gnome_canvas_root(self->priv->canvas),
                           GNOME_TYPE_CANVAS_GROUP,"x",0.0,"y",0.0,NULL);
  gnome_canvas_item_lower_to_bottom(self->priv->grid);

  if(!self->priv->grid_density) return;

  points=gnome_canvas_points_new(2);

  // low=1->2, mid=2->4, high=3->8
  step=(MACHINE_VIEW_ZOOM_X+MACHINE_VIEW_ZOOM_X)/(gdouble)(1<<self->priv->grid_density);
  points->coords[1]=-(MACHINE_VIEW_ZOOM_Y*MACHINE_VIEW_GRID_FC);
  points->coords[3]= (MACHINE_VIEW_ZOOM_Y*MACHINE_VIEW_GRID_FC);
  for(s=-(MACHINE_VIEW_ZOOM_X*MACHINE_VIEW_GRID_FC);s<=(MACHINE_VIEW_ZOOM_X*MACHINE_VIEW_GRID_FC);s+=step) {
    points->coords[0]=points->coords[2]=s;
    color=(((gdouble)((glong)(s/MACHINE_VIEW_ZOOM_X)))==(s/MACHINE_VIEW_ZOOM_X))?0xAAAAAAFF:0xCCCCCCFF;
    //GST_INFO("grid : s= %lf - %lf",s,s/MACHINE_VIEW_ZOOM_X);
    gnome_canvas_item_new(GNOME_CANVAS_GROUP(self->priv->grid),
                            GNOME_TYPE_CANVAS_LINE,
                            "points", points,
                            /*"fill-color", "gray",*/
                            "fill-color-rgba",color,
                            "width-pixels", 1,
                            NULL);
  }
  step=(MACHINE_VIEW_ZOOM_Y+MACHINE_VIEW_ZOOM_Y)/(gdouble)(1<<self->priv->grid_density);
  points->coords[0]=-(MACHINE_VIEW_ZOOM_X*MACHINE_VIEW_GRID_FC);
  points->coords[2]= (MACHINE_VIEW_ZOOM_X*MACHINE_VIEW_GRID_FC);
  for(s=-(MACHINE_VIEW_ZOOM_Y*MACHINE_VIEW_GRID_FC);s<=(MACHINE_VIEW_ZOOM_Y*MACHINE_VIEW_GRID_FC);s+=step) {
    points->coords[1]=points->coords[3]=s;
    color=(((gdouble)((glong)(s/MACHINE_VIEW_ZOOM_Y)))==(s/MACHINE_VIEW_ZOOM_Y))?0xAAAAAAFF:0xCCCCCCFF;
    gnome_canvas_item_new(GNOME_CANVAS_GROUP(self->priv->grid),
                            GNOME_TYPE_CANVAS_LINE,
                            "points", points,
                            /*"fill-color", "gray",*/
                            "fill-color-rgba",color,
                            "width-pixels", 1,
                            NULL);
  }

  gnome_canvas_points_free(points);
}

static void bt_main_page_machines_add_wire(const BtMainPageMachines *self) {
  BtSong *song;
  BtWire *wire;
  GError *err=NULL;
  BtMachine *src_machine,*dst_machine;
  GHashTable *properties;
  gdouble pos_xs,pos_ys,pos_xe,pos_ye;

  g_assert(self->priv->new_wire_src);
  g_assert(self->priv->new_wire_dst);

  g_object_get(self->priv->app,"song",&song,NULL);
  g_object_get(self->priv->new_wire_src,"machine",&src_machine,NULL);
  g_object_get(self->priv->new_wire_dst,"machine",&dst_machine,NULL);

  // try to establish a new connection
  wire=bt_wire_new(song,src_machine,dst_machine,&err);
  if(err==NULL) {
    gchar *undo_str,*redo_str;
    gchar *smid,*dmid;

    g_object_get(src_machine,"properties",&properties,"id",&smid,NULL);
    machine_view_get_machine_position(properties,&pos_xs,&pos_ys);
    g_object_get(dst_machine,"properties",&properties,"id",&dmid,NULL);
    machine_view_get_machine_position(properties,&pos_xe,&pos_ye);
    // draw wire
    wire_item_new(self,wire,pos_xs,pos_ys,pos_xe,pos_ye,self->priv->new_wire_src,self->priv->new_wire_dst);
    gnome_canvas_item_lower_to_bottom(self->priv->grid);

    undo_str = g_strdup_printf("rem_wire \"%s\",\"%s\"",smid,dmid);
    redo_str = g_strdup_printf("add_wire \"%s\",\"%s\"",smid,dmid);
    bt_change_log_add(self->priv->change_log,BT_CHANGE_LOGGER(self),undo_str,redo_str);
    g_free(smid);g_free(dmid);
  }
  else {
    GST_WARNING("failed to make wire: %s",err->message);
    g_error_free(err);
  }
  g_object_unref(wire);
  g_object_unref(dst_machine);
  g_object_unref(src_machine);
  g_object_unref(song);
}

static BtMachineCanvasItem *bt_main_page_machines_get_machine_canvas_item_at(const BtMainPageMachines *self,gdouble mouse_x,gdouble mouse_y) {
  BtMachineCanvasItem *mitem=NULL;
  GnomeCanvasItem *ci,*pci;

  //GST_DEBUG("is there a machine at pos?");

  if((ci=gnome_canvas_get_item_at(self->priv->canvas,mouse_x,mouse_y))) {
    g_object_get(ci,"parent",&pci,NULL);
    if(BT_IS_MACHINE_CANVAS_ITEM(pci)) {
      mitem=BT_MACHINE_CANVAS_ITEM(pci);
      //GST_DEBUG("  yes!");
    }
    else g_object_unref(pci);
  }
  return(mitem);
}

static gboolean bt_main_page_machines_check_wire(const BtMainPageMachines *self) {
  gboolean ret=FALSE;
  BtMachine *src_machine,*dst_machine;

  GST_INFO("can we link to it?");

  g_assert(self->priv->new_wire_src);
  g_assert(self->priv->new_wire_dst);

  g_object_get(self->priv->new_wire_src,"machine",&src_machine,NULL);
  g_object_get(self->priv->new_wire_dst,"machine",&dst_machine,NULL);

  // if the citem->machine is a sink/processor-machine
  if(BT_IS_SINK_MACHINE(dst_machine) || BT_IS_PROCESSOR_MACHINE(dst_machine)) {
    // check if these machines are not yet connected
    if(bt_wire_can_link(src_machine,dst_machine)) {
      ret=TRUE;
      GST_INFO("  yes!");
    }
  }
  g_object_unref(dst_machine);
  g_object_unref(src_machine);
  return(ret);
}


//-- event handler

static void on_machine_added(BtSetup *setup,BtMachine *machine,gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);
  GHashTable *properties;
  gdouble pos_x,pos_y;

  GST_INFO("new machine %p,ref_ct=%d has been added",machine,G_OBJECT_REF_COUNT(machine));

  g_object_get(machine,"properties",&properties,NULL);
  if(properties) {
    if(!machine_view_get_machine_position(properties,&pos_x,&pos_y)) {
      gchar str[G_ASCII_DTOSTR_BUF_SIZE];
      pos_x=self->priv->mouse_x;
      pos_y=self->priv->mouse_y;
      g_hash_table_insert(properties,g_strdup("xpos"),g_strdup(g_ascii_dtostr(str,G_ASCII_DTOSTR_BUF_SIZE,(pos_x/MACHINE_VIEW_ZOOM_X))));
      g_hash_table_insert(properties,g_strdup("ypos"),g_strdup(g_ascii_dtostr(str,G_ASCII_DTOSTR_BUF_SIZE,(pos_y/MACHINE_VIEW_ZOOM_Y))));
    }
  } else {
    pos_x=self->priv->mouse_x;
    pos_y=self->priv->mouse_y;
  }
	self->priv->machine_xn=pos_x/MACHINE_VIEW_ZOOM_X;
	self->priv->machine_yn=pos_y/MACHINE_VIEW_ZOOM_Y;

  GST_DEBUG_OBJECT(machine,"adding machine at %lf x %lf, mouse is at %lf x %lf",
    pos_x,pos_y,self->priv->mouse_x,self->priv->mouse_y);

  // draw machine
  machine_item_new(self,machine,pos_x,pos_y);

  GST_INFO("... machine %p,ref_ct=%d has been added",machine,G_OBJECT_REF_COUNT(machine));
}

static void on_machine_removed(BtSetup *setup,BtMachine *machine,gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);
  BtMachineCanvasItem *item;

  if(!machine) return;

  GST_INFO("machine %p,ref_ct=%d has been removed",machine,G_OBJECT_REF_COUNT(machine));

  if((item=g_hash_table_lookup(self->priv->machines,machine))) {
    GST_INFO("now removing machine-item : %p",item);
    g_hash_table_remove(self->priv->machines,machine);
    gtk_object_destroy(GTK_OBJECT(item));
  }

  GST_INFO("... machine %p,ref_ct=%d has been removed",machine,G_OBJECT_REF_COUNT(machine));
}

static void on_wire_removed(BtSetup *setup,BtWire *wire,gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);
  BtWireCanvasItem *item;

  if(!wire) return;

  GST_INFO("wire %p,ref_ct=%d has been removed",wire,G_OBJECT_REF_COUNT(wire));

  // add undo/redo details
  if(bt_change_log_is_active(self->priv->change_log)) {
    gchar *undo_str,*redo_str;
    gchar *prop;
    BtMachine *src_machine,*dst_machine;
    GHashTable *properties;
    gchar *smid,*dmid;

    g_object_get(wire,"src",&src_machine,"dst",&dst_machine,"properties",&properties,NULL);
    g_object_get(src_machine,"id",&smid,NULL);
    g_object_get(dst_machine,"id",&dmid,NULL);
    
    bt_change_log_start_group(self->priv->change_log);

    undo_str = g_strdup_printf("add_wire \"%s\",\"%s\"",smid,dmid);
    redo_str = g_strdup_printf("rem_wire \"%s\",\"%s\"",smid,dmid);
    bt_change_log_add(self->priv->change_log,BT_CHANGE_LOGGER(self),undo_str,redo_str);
    
    prop=(gchar *)g_hash_table_lookup(properties,"analyzer-shown");
    if(prop && *prop=='1') {
      undo_str = g_strdup_printf("set_wire_property \"%s\",\"%s\",\"analyzer-shown\",\"1\"",smid,dmid);
      bt_change_log_add(self->priv->change_log,BT_CHANGE_LOGGER(self),undo_str,g_strdup(undo_str));
    }
    // TODO(ensonic): volume and panorama
    g_free(smid);g_free(dmid);
    
    bt_change_log_end_group(self->priv->change_log);

    g_object_unref(dst_machine);
    g_object_unref(src_machine);
  }

  if((item=g_hash_table_lookup(self->priv->wires,wire))) {
    GST_INFO("now removing wire-item : %p",item);
    g_hash_table_remove(self->priv->wires,wire);
    gtk_object_destroy(GTK_OBJECT(item));
  }

  GST_INFO("... wire %p,ref_ct=%d has been removed",wire,G_OBJECT_REF_COUNT(wire));
}

static void on_song_changed(const BtEditApplication *app,GParamSpec *arg,gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);
  BtSong *song;

  GST_INFO("song has changed : app=%p, self=%p",app,self);
  g_object_try_unref(self->priv->setup);

  // get song from app
  g_object_get(self->priv->app,"song",&song,NULL);
  if(!song) {
    self->priv->setup=NULL;
    self->priv->properties=NULL;
    machine_view_clear(self);
    GST_INFO("song (null) has changed done");
    return;
  }
  GST_INFO("song->ref_ct=%d",G_OBJECT_REF_COUNT(song));

  g_object_get(song,"setup",&self->priv->setup,NULL);
  g_object_get(self->priv->setup,"properties",&self->priv->properties,NULL);
  // update page
  machine_view_refresh(self);
  g_signal_connect(self->priv->setup,"machine-added",G_CALLBACK(on_machine_added),(gpointer)self);
  g_signal_connect(self->priv->setup,"machine-removed",G_CALLBACK(on_machine_removed),(gpointer)self);
  g_signal_connect(self->priv->setup,"wire-removed",G_CALLBACK(on_wire_removed),(gpointer)self);
  // release the reference
  g_object_unref(song);
  GST_INFO("song has changed done");
}

static void on_toolbar_zoom_fit_clicked(GtkButton *button, gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);
  BtMachine *machine;
  GHashTable *properties;
  GList *node,*list;
  gdouble fc_x,fc_y,c_x,c_y,ms;
  // machine area
  gdouble ma_xs=MACHINE_VIEW_ZOOM_X,ma_x,ma_xe=-MACHINE_VIEW_ZOOM_X,ma_xd;
  gdouble ma_ys=MACHINE_VIEW_ZOOM_Y,ma_y,ma_ye=-MACHINE_VIEW_ZOOM_Y,ma_yd;
  // page area
  gdouble /*pg_xs,pg_x,pg_xe,pg_xd,*/pg_xl;
  gdouble /*pg_ys,pg_y,pg_ye,pg_yd,*/pg_yl;
  gdouble old_zoom=self->priv->zoom;

  //calculate bounds
  g_object_get(self->priv->setup,"machines",&list,NULL);
  for(node=list;node;node=g_list_next(node)) {
    machine=BT_MACHINE(node->data);
    // get position
    g_object_get(machine,"properties",&properties,NULL);
    machine_view_get_machine_position(properties,&ma_x,&ma_y);
    if(ma_x<ma_xs) ma_xs=ma_x;
    if(ma_x>ma_xe) ma_xe=ma_x;
    if(ma_y<ma_ys) ma_ys=ma_y;
    if(ma_y>ma_ye) ma_ye=ma_y;
    GST_DEBUG("machines: x:%+6.4lf y:%+6.4lf -> ranging from x:%+6.4lf...%+6.4lf and y:%+6.4lf...%+6.4lf",ma_x,ma_y,ma_xs,ma_xe,ma_ys,ma_ye);
  }
  g_list_free(list);
  /* need to add machine extends + some space */
  GST_INFO("machines ranging from x:%+6.4lf...%+6.4lf and y:%+6.4lf...%+6.4lf",ma_xs,ma_xe,ma_ys,ma_ye);
  ms=2*MACHINE_VIEW_MACHINE_SIZE_X;
  ma_xs-=ms;ma_xe+=ms;
  ma_xd=(ma_xe-ma_xs);
  ms=2*MACHINE_VIEW_MACHINE_SIZE_Y;
  ma_ys-=ms;ma_ye+=ms;
  ma_yd=(ma_ye-ma_ys);

  g_object_get(self->priv->hadjustment,/*"lower",&pg_xs,"value",&pg_x,"upper",&pg_xe,*/"page-size",&pg_xl,NULL);
  g_object_get(self->priv->vadjustment,/*"lower",&pg_ys,"value",&pg_y,"upper",&pg_ye,*/"page-size",&pg_yl,NULL);
  /*
  pg_xd=(pg_xe-pg_xs)/MACHINE_VIEW_ZOOM_X;
  pg_yd=(pg_ye-pg_ys)/MACHINE_VIEW_ZOOM_Y;
  GST_INFO("page: pos x/y:%+6.4lf %+6.4lf size x/y: %+6.4lf %+6.4lf -> ranging from x:%+6.4lf...%+6.4lf and y:%+6.4lf...%+6.4lf",
    pg_x,pg_y,pg_xl,pg_yl, pg_xs,pg_xe,pg_ys,pg_ye);
  */

  // zoom
  fc_x=pg_xl/ma_xd;
  fc_y=pg_yl/ma_yd;
  GST_INFO("zoom old=%6.4lf, x:%+6.4lf / %+6.4lf = %+6.4lf and y:%+6.4lf / %+6.4lf = %+6.4lf",self->priv->zoom, pg_xl,ma_xd,fc_x, pg_yl,ma_yd,fc_y);
  self->priv->zoom=MIN(fc_x,fc_y);

  // center region
  /* pos can be between: lower ... upper-page_size) */
  GST_INFO("x: (%+6.4lf-%+6.4lf)/2=%+6.4lf",pg_xl,(ma_xd*self->priv->zoom),((pg_xl-(ma_xd*self->priv->zoom))/2.0));
  GST_INFO("y: (%+6.4lf-%+6.4lf)/2=%+6.4lf",pg_yl,(ma_yd*self->priv->zoom),((pg_yl-(ma_yd*self->priv->zoom))/2.0));
  c_x=(MACHINE_VIEW_ZOOM_X+ma_xs)*self->priv->zoom-((pg_xl-(ma_xd*self->priv->zoom))/2.0);
  c_y=(MACHINE_VIEW_ZOOM_Y+ma_ys)*self->priv->zoom-((pg_yl-(ma_yd*self->priv->zoom))/2.0);
  gtk_adjustment_set_value(self->priv->hadjustment,c_x);
  gtk_adjustment_set_value(self->priv->vadjustment,c_y);
  GST_INFO("toolbar zoom_fit event occurred: zoom = %lf, center x/y = %+6.4lf,%+6.4lf",self->priv->zoom,c_x,c_y);

  if(self->priv->zoom>old_zoom) {
    gnome_canvas_set_pixels_per_unit(self->priv->canvas,self->priv->zoom);
    update_machines_zoom(self);
  }
  else {
    update_machines_zoom(self);
    gnome_canvas_set_pixels_per_unit(self->priv->canvas,self->priv->zoom);
  }

  gtk_widget_grab_focus_savely(GTK_WIDGET(self->priv->canvas));

}

static void on_toolbar_zoom_in_clicked(GtkButton *button, gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);

  self->priv->zoom*=1.5;
  GST_INFO("toolbar zoom_in event occurred : %lf",self->priv->zoom);

  gnome_canvas_set_pixels_per_unit(self->priv->canvas,self->priv->zoom);
  update_machines_zoom(self);

  gtk_widget_grab_focus_savely(GTK_WIDGET(self->priv->canvas));
}

static void on_toolbar_zoom_out_clicked(GtkButton *button, gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);

  self->priv->zoom/=1.5;
  GST_INFO("toolbar zoom_out event occurred : %lf",self->priv->zoom);

  update_machines_zoom(self);
  gnome_canvas_set_pixels_per_unit(self->priv->canvas,self->priv->zoom);

  gtk_widget_grab_focus_savely(GTK_WIDGET(self->priv->canvas));
}

#ifndef GRID_USES_MENU_TOOL_ITEM
static void on_toolbar_grid_clicked(GtkButton *button, gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);

  gtk_menu_popup(self->priv->grid_density_menu,NULL,NULL,NULL,NULL,1,gtk_get_current_event_time());
}
#endif

static void on_toolbar_grid_density_off_activated(GtkMenuItem *menuitem, gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);
  BtSettings *settings;

  if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem))) return;

  self->priv->grid_density=0;

  g_object_get(self->priv->app,"settings",&settings,NULL);
  g_object_set(settings,"grid-density","off",NULL);
  g_object_unref(settings);

  bt_main_page_machines_draw_grid(self);
}

static void on_toolbar_grid_density_low_activated(GtkMenuItem *menuitem, gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);
  BtSettings *settings;

  if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem))) return;

  self->priv->grid_density=1;

  g_object_get(self->priv->app,"settings",&settings,NULL);
  g_object_set(settings,"grid-density","low",NULL);
  g_object_unref(settings);

  bt_main_page_machines_draw_grid(self);
}

static void on_toolbar_grid_density_mid_activated(GtkMenuItem *menuitem, gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);
  BtSettings *settings;

  if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem))) return;

  self->priv->grid_density=2;

  g_object_get(self->priv->app,"settings",&settings,NULL);
  g_object_set(settings,"grid-density","medium",NULL);
  g_object_unref(settings);

  bt_main_page_machines_draw_grid(self);
}

static void on_toolbar_grid_density_high_activated(GtkMenuItem *menuitem, gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);
  BtSettings *settings;

  if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem))) return;

  self->priv->grid_density=3;

  g_object_get(self->priv->app,"settings",&settings,NULL);
  g_object_set(settings,"grid-density","high",NULL);
  g_object_unref(settings);

  bt_main_page_machines_draw_grid(self);
}

static void on_toolbar_menu_clicked(GtkButton *button, gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);

  gtk_menu_popup(self->priv->context_menu,NULL,NULL,NULL,NULL,1,gtk_get_current_event_time());
}

static void on_context_menu_unmute_all(GtkMenuItem *menuitem, gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);
  GList *node,*list;
  BtMachine *machine;
  
  g_object_get(self->priv->setup,"machines",&list,NULL);
  for(node=list;node;node=g_list_next(node)) {
    machine=BT_MACHINE(node->data);
    // TODO(ensonic): this also un-solos and un-bypasses
    g_object_set(machine,"state",BT_MACHINE_STATE_NORMAL,NULL);
  }
  g_list_free(list);
}

static void on_vadjustment_changed(GtkAdjustment *adjustment, gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);
  gdouble vs,ve,vp,val,v;

  g_object_get(self->priv->vadjustment,"value",&val,"lower",&vs,"upper",&ve,"page-size",&vp,NULL);
  v=(ve-vs-vp);
  if(v) {
    self->priv->scroll_y=(val-vs)/(ve-vs-vp);
  }

  if(self->priv->properties) {
    gchar str[G_ASCII_DTOSTR_BUF_SIZE];
    gchar *prop;
    gdouble oval=0.0;
    gboolean have_val=FALSE;

    //GST_INFO("ypos: %lf",val);
    if((prop=(gchar *)g_hash_table_lookup(self->priv->properties,"ypos"))) {
      oval=g_ascii_strtod(prop,NULL);
      have_val=TRUE;
    }
    if((!have_val) || (oval!=val)) {
      g_hash_table_insert(self->priv->properties,g_strdup("ypos"),g_strdup(g_ascii_dtostr(str,G_ASCII_DTOSTR_BUF_SIZE,val)));
      if(have_val)
        bt_edit_application_set_song_unsaved(self->priv->app);
    }
  }
}

static void on_hadjustment_changed(GtkAdjustment *adjustment, gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);
  gdouble vs,ve,vp,val,v;

  g_object_get(self->priv->hadjustment,"value",&val,"lower",&vs,"upper",&ve,"page-size",&vp,NULL);
  v=(ve-vs-vp);
  if(v) {
    self->priv->scroll_x=(val-vs)/(ve-vs-vp);
  }

  if(self->priv->properties) {
    gchar str[G_ASCII_DTOSTR_BUF_SIZE];
    gchar *prop;
    gdouble oval=0.0;
    gboolean have_val=FALSE;

    //GST_INFO("xpos: %lf",val);
    if((prop=(gchar *)g_hash_table_lookup(self->priv->properties,"xpos"))) {
      oval=g_ascii_strtod(prop,NULL);
      have_val=TRUE;
    }
    if((!have_val) || (oval!=val)) {
      g_hash_table_insert(self->priv->properties,g_strdup("xpos"),g_strdup(g_ascii_dtostr(str,G_ASCII_DTOSTR_BUF_SIZE,val)));
      if(have_val)
        bt_edit_application_set_song_unsaved(self->priv->app);
    }
  }
}

static void on_page_switched(GtkNotebook *notebook, GParamSpec *arg, gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);
  guint page_num;
  static gint prev_page_num=-1;

  g_object_get(notebook,"page",&page_num,NULL);

  if(page_num==BT_MAIN_PAGES_MACHINES_PAGE) {
    // only do this if the page really has changed
    if(prev_page_num != BT_MAIN_PAGES_MACHINES_PAGE) {
      GST_DEBUG("enter machine page");
    }
  }
  else {
    // only do this if the page was BT_MAIN_PAGES_MACHINES_PAGE
    if(prev_page_num == BT_MAIN_PAGES_MACHINES_PAGE) {
      BtMainWindow *main_window;
      GST_DEBUG("leave machine page");

      g_object_get(self->priv->app,"main-window",&main_window,NULL);
      if(main_window) {
        bt_child_proxy_set(main_window,"statusbar::status",NULL,NULL);
        g_object_unref(main_window);
      }
    }
  }
  prev_page_num = page_num;
}

static gboolean on_canvas_event(GnomeCanvas *canvas, GdkEvent *event, gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);
  gboolean res=FALSE;
  GnomeCanvasItem *ci,*pci;
  gdouble mouse_x,mouse_y;
  gchar *color;
  BtMachine *machine;

  //if(!gtk_widget_get_realized(GTK_WIDGET(canvas))) return(res);
  //GST_INFO("canvas event received: type=%d", event->type);

  switch(event->type) {
    case GDK_BUTTON_PRESS:
      /*
      {
        GdkEventButton *e=(GdkEventButton*)event;
        GST_WARNING("type=%4d, window=%p, send_event=%3d, time=%8d",e->type,e->window,e->send_event,e->time);
        GST_WARNING("x=%6.4lf, y=%6.4lf, axes=%p, state=%4d",e->x,e->y,e->axes,e->state);
        GST_WARNING("button=%4d, device=%p, x_root=%6.4lf, y_root=%6.4lf\n",e->button,e->device,e->x_root,e->y_root);
      }
      */
      // store mouse coordinates, so that we can later place a newly added machine there
      gnome_canvas_window_to_world(self->priv->canvas,event->button.x,event->button.y,&self->priv->mouse_x,&self->priv->mouse_y);
      if(!(ci=gnome_canvas_get_item_at(self->priv->canvas,self->priv->mouse_x,self->priv->mouse_y))) {
        GST_DEBUG("GDK_BUTTON_PRESS: %d",event->button.button);
        if(event->button.button==1) {
          // start dragging the canvas
          self->priv->dragging=TRUE;
        }
        else if(event->button.button==3) {
          // show context menu
          gtk_menu_popup(self->priv->context_menu,NULL,NULL,NULL,NULL,3,gtk_get_current_event_time());
          res=TRUE;
        }
      }
      else {
        if(event->button.button==1) {
          g_object_get(ci,"parent",&pci,NULL);
          if(BT_IS_MACHINE_CANVAS_ITEM(pci)) {
            if(event->button.state&GDK_SHIFT_MASK) {
              self->priv->new_wire_src=BT_MACHINE_CANVAS_ITEM(pci);
              g_object_get(pci,"machine",&machine,NULL);
              // if the citem->machine is a source/processor-machine
              if(BT_IS_SOURCE_MACHINE(machine) || BT_IS_PROCESSOR_MACHINE(machine)) {
                start_connect(self);
                res=TRUE;
              }
              g_object_unref(machine);
            } else {
              gdouble px, py;
              // store pos for later undo
              g_object_get(pci,"x",&px,"y",&py,NULL);
              self->priv->machine_xo=px/MACHINE_VIEW_ZOOM_X;
              self->priv->machine_yo=py/MACHINE_VIEW_ZOOM_Y;
              self->priv->moving_machine_item=BT_MACHINE_CANVAS_ITEM(pci);
            }
          }
          else g_object_unref(pci);
        }
        // gnome_canvas_get_item_at() does not ref()
        //g_object_unref(ci);
      }
      break;
    case GDK_MOTION_NOTIFY:
      //GST_DEBUG("GDK_MOTION_NOTIFY: %f,%f",event->button.x,event->button.y);
      if(self->priv->connecting) {
        if(!self->priv->moved) {
          gnome_canvas_item_grab(self->priv->new_wire, GDK_POINTER_MOTION_MASK |
                                /* GDK_ENTER_NOTIFY_MASK | */
                                /* GDK_LEAVE_NOTIFY_MASK | */
          GDK_BUTTON_RELEASE_MASK, self->priv->drag_cursor, event->button.time);
        }
        // handle setting the coords of the connection line
        gnome_canvas_window_to_world(self->priv->canvas,event->button.x,event->button.y,&mouse_x,&mouse_y);
        self->priv->new_wire_points->coords[2]=mouse_x;
        self->priv->new_wire_points->coords[3]=mouse_y;
        // IDEA(ensonic): the green is a bit bright, use ui_resources?, also what about having both colors in self->priv (should save the canvas the color parsing)
        color="red";
        if((self->priv->new_wire_dst=bt_main_page_machines_get_machine_canvas_item_at(self,mouse_x,mouse_y))) {
          if(bt_main_page_machines_check_wire(self)) {
            color="green";
          }
          g_object_unref(self->priv->new_wire_dst);
        }
        gnome_canvas_item_set(self->priv->new_wire,"points",self->priv->new_wire_points,"fill-color",color,NULL);
        self->priv->moved=TRUE;
        res=TRUE;
      } else if (self->priv->dragging) {
        // snapshot current mousepos and calculate delta
        gnome_canvas_window_to_world(self->priv->canvas,event->button.x,event->button.y,&mouse_x,&mouse_y);
        mouse_x=self->priv->mouse_x-mouse_x;
        mouse_y=self->priv->mouse_y-mouse_y;
        // scroll canvas
        gtk_adjustment_set_value(self->priv->hadjustment,gtk_adjustment_get_value(self->priv->hadjustment)+mouse_x);
        gtk_adjustment_set_value(self->priv->vadjustment,gtk_adjustment_get_value(self->priv->vadjustment)+mouse_y);
      }
      break;
    case GDK_BUTTON_RELEASE:
      GST_DEBUG("GDK_BUTTON_RELEASE: %d",event->button.button);
      if(self->priv->connecting) {
        if(self->priv->moved) {
          gnome_canvas_item_ungrab(self->priv->new_wire,event->button.time);
        }
        gnome_canvas_window_to_world(self->priv->canvas,event->button.x,event->button.y,&mouse_x,&mouse_y);
        if((self->priv->new_wire_dst=bt_main_page_machines_get_machine_canvas_item_at(self,mouse_x,mouse_y))) {
          if(bt_main_page_machines_check_wire(self)) {
            bt_main_page_machines_add_wire(self);
          }
          g_object_unref(self->priv->new_wire_dst);
        }
        g_object_unref(self->priv->new_wire_src);
        self->priv->new_wire_src=NULL;
        gtk_object_destroy(GTK_OBJECT(self->priv->new_wire));self->priv->new_wire=NULL;
        gnome_canvas_points_free(self->priv->new_wire_points);self->priv->new_wire_points=NULL;
        self->priv->connecting=FALSE;
      } else if (self->priv->dragging) {
        self->priv->dragging=FALSE;
      }
      if(self->priv->moving_machine_item) {
        gdouble px, py;

        g_object_get((GnomeCanvasItem *)self->priv->moving_machine_item,"x",&px,"y",&py,NULL);
        self->priv->machine_xn=px/MACHINE_VIEW_ZOOM_X;
        self->priv->machine_yn=py/MACHINE_VIEW_ZOOM_Y;
        machine_item_moved(self,self->priv->moving_machine_item);
        g_object_unref(self->priv->moving_machine_item);
        self->priv->moving_machine_item=NULL;
      }
      break;
    case GDK_KEY_RELEASE:
      // need mouse pos to check if there is a canvas item under pointer
      {
        gint pointer_x,pointer_y;
        gtk_widget_get_pointer(GTK_WIDGET(self->priv->canvas),&pointer_x,&pointer_y);
        gnome_canvas_window_to_world(self->priv->canvas,(gdouble)pointer_x,(gdouble)pointer_y,&self->priv->mouse_x,&self->priv->mouse_y);
        //GST_INFO("button: x=%6.3lf, y=%6.3lf",event->button.x,event->button.y);
        //GST_INFO("motion: x=%6.3lf, y=%6.3lf",event->motion.x,event->motion.y);
        //GST_INFO("motion: x=%6d, y=%6d",pointer_x,pointer_y);
      }

      if(!gnome_canvas_get_item_at(self->priv->canvas,self->priv->mouse_x,self->priv->mouse_y)) {
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
      }
      break;
    default:
      break;
  }
  /* we don't want the click falling through to the parent canvas item, if we have handled it */
  return res;
}

static void on_toolbar_style_changed(const BtSettings *settings,GParamSpec *arg,gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);
  gchar *toolbar_style;

  g_object_get((gpointer)settings,"toolbar-style",&toolbar_style,NULL);
  if(!BT_IS_STRING(toolbar_style)) return;

  GST_INFO("!!!  toolbar style has changed '%s'",toolbar_style);
  gtk_toolbar_set_style(GTK_TOOLBAR(self->priv->toolbar),gtk_toolbar_get_style_from_string(toolbar_style));
  g_free(toolbar_style);
}

static void on_volume_popup_changed(GtkAdjustment *adj, gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);
  // FIXME(ensonic): workaround for https://bugzilla.gnome.org/show_bug.cgi?id=667598
  gdouble gain = 4.0 - (gtk_adjustment_get_value (adj) / 100.0);
  g_object_set(self->priv->wire_gain,"volume",gain,NULL);
}

static void on_panorama_popup_changed(GtkAdjustment *adj, gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);
  gfloat pan = (gfloat)gtk_adjustment_get_value (adj) / 100.0;
  g_object_set(self->priv->wire_pan,"panorama",pan,NULL);
}

#if 0
static void on_canvas_size_allocate(GtkWidget *widget,GtkAllocation *allocation,gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);
  gdouble xs,xe,xp;
  gdouble ys,ye,yp;

  // center
  g_object_get(self->priv->hadjustment,"lower",&xs,"upper",&xe,"page-size",&xp,NULL);
  gtk_adjustment_set_value(self->priv->hadjustment,xs+((xe-xs-xp)*self->priv->scroll_x));
  g_object_get(self->priv->vadjustment,"lower",&ys,"upper",&ye,"page-size",&yp,NULL);
  gtk_adjustment_set_value(self->priv->vadjustment,ys+((ye-ys-yp)*self->priv->scroll_y));
  GST_WARNING("canvas: abs. scroll pos %lf x %lf, abs. scroll pos %lf x %lf",
    xs+((xe-xs-xp)*self->priv->scroll_x),
    ys+((ye-ys-yp)*self->priv->scroll_y),
    self->priv->scroll_x,self->priv->scroll_y);

  GST_WARNING("canvas size %d x %d",allocation->width,allocation->height);
}
#endif

//-- helper methods

static void bt_main_page_machines_init_main_context_menu(const BtMainPageMachines *self) {
  GtkWidget *menu_item,*menu,*image;

  self->priv->context_menu=GTK_MENU(g_object_ref_sink(gtk_menu_new()));

  //menu_item=gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD,NULL);
  menu_item=gtk_image_menu_item_new_with_label(_("Add machine"));
  image=gtk_image_new_from_stock(GTK_STOCK_ADD,GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_show(menu_item);
  // add machine selection sub-menu
  menu=GTK_WIDGET(bt_machine_menu_new(self));
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item),menu);

  menu_item=gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_show(menu_item);

  menu_item=gtk_menu_item_new_with_label(_("Unmute all machines"));
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  g_signal_connect(menu_item,"activate",G_CALLBACK(on_context_menu_unmute_all),(gpointer)self);
  gtk_widget_show(menu_item);
}

static void bt_main_page_machines_init_grid_density_menu(const BtMainPageMachines *self) {
  GtkWidget *menu_item;

  // create grid-density menu with grid-density={off,low,mid,high}
  self->priv->grid_density_menu=GTK_MENU(g_object_ref_sink(gtk_menu_new()));

  // background grid density
  menu_item=gtk_radio_menu_item_new_with_label(self->priv->grid_density_group,_("Off"));
  self->priv->grid_density_group=gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
  if(self->priv->grid_density==0) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),TRUE);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->grid_density_menu),menu_item);
  gtk_widget_show(menu_item);
  g_signal_connect(menu_item,"activate",G_CALLBACK(on_toolbar_grid_density_off_activated),(gpointer)self);

  menu_item=gtk_radio_menu_item_new_with_label(self->priv->grid_density_group,_("Low"));
  self->priv->grid_density_group=gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
  if(self->priv->grid_density==1) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),TRUE);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->grid_density_menu),menu_item);
  gtk_widget_show(menu_item);
  g_signal_connect(menu_item,"activate",G_CALLBACK(on_toolbar_grid_density_low_activated),(gpointer)self);

  menu_item=gtk_radio_menu_item_new_with_label(self->priv->grid_density_group,_("Medium"));
  self->priv->grid_density_group=gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
  if(self->priv->grid_density==2) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),TRUE);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->grid_density_menu),menu_item);
  gtk_widget_show(menu_item);
  g_signal_connect(menu_item,"activate",G_CALLBACK(on_toolbar_grid_density_mid_activated),(gpointer)self);

  menu_item=gtk_radio_menu_item_new_with_label(self->priv->grid_density_group,_("High"));
  self->priv->grid_density_group=gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
  if(self->priv->grid_density==3) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),TRUE);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->grid_density_menu),menu_item);
  gtk_widget_show(menu_item);
  g_signal_connect(menu_item,"activate",G_CALLBACK(on_toolbar_grid_density_high_activated),(gpointer)self);
}

static void bt_main_page_machines_init_ui(const BtMainPageMachines *self,const BtMainPages *pages) {
  BtSettings *settings;
  GtkWidget *image,*scrolled_window;
  GtkWidget *tool_item;
  gchar *density;

  GST_DEBUG("!!!! self=%p",self);

  gtk_widget_set_name(GTK_WIDGET(self),"machine view");

  g_object_get(self->priv->app,"settings",&settings,NULL);

  g_object_get(settings,"grid-density",&density,NULL);
  if(!strcmp(density,"off")) self->priv->grid_density=0;
  else if(!strcmp(density,"low")) self->priv->grid_density=1;
  else if(!strcmp(density,"medium")) self->priv->grid_density=2;
  else if(!strcmp(density,"high")) self->priv->grid_density=3;
  g_free(density);

  // create grid-density menu
  bt_main_page_machines_init_grid_density_menu(self);
  // create the context menu
  bt_main_page_machines_init_main_context_menu(self);

  // add toolbar
  self->priv->toolbar=gtk_toolbar_new();
  gtk_widget_set_name(self->priv->toolbar,"machine view toolbar");

  tool_item=GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_ZOOM_FIT));
  gtk_widget_set_name(tool_item,"Zoom Fit");
  gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM(tool_item),_("Zoom in/out so that everything is visible"));
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->toolbar),GTK_TOOL_ITEM(tool_item),-1);
  g_signal_connect(G_OBJECT(tool_item),"clicked",G_CALLBACK(on_toolbar_zoom_fit_clicked),(gpointer)self);

  self->priv->zoom_in=GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_ZOOM_IN));
  gtk_widget_set_name(self->priv->zoom_in,"Zoom In");
  gtk_widget_set_sensitive(self->priv->zoom_in,(self->priv->zoom<3.0));
  gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM(self->priv->zoom_in),_("Zoom in for more details"));
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->toolbar),GTK_TOOL_ITEM(self->priv->zoom_in),-1);
  g_signal_connect(self->priv->zoom_in,"clicked",G_CALLBACK(on_toolbar_zoom_in_clicked),(gpointer)self);

  self->priv->zoom_out=GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_ZOOM_OUT));
  gtk_widget_set_name(self->priv->zoom_out,"Zoom Out");
  gtk_widget_set_sensitive(self->priv->zoom_out,(self->priv->zoom>0.4));
  gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM(self->priv->zoom_out),_("Zoom out for better overview"));
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->toolbar),GTK_TOOL_ITEM(self->priv->zoom_out),-1);
  g_signal_connect(self->priv->zoom_out,"clicked",G_CALLBACK(on_toolbar_zoom_out_clicked),(gpointer)self);

  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->toolbar),gtk_separator_tool_item_new(),-1);

  // grid density toolbar icon
#ifdef GRID_USES_MENU_TOOL_ITEM
  // this is weird, we and up with a button and a menu, instead of a joint thing
  // so this is probably mean for e.g. undo, where the button undos and the menu allows to undo to stop step
  image=gtk_image_new_from_icon_name("buzztard_menu_grid",GTK_ICON_SIZE_MENU);
  tool_item=GTK_WIDGET(gtk_menu_tool_button_new(image,_("Grid")));
  gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(tool_item),GTK_WIDGET(self->priv->grid_density_menu));
  gtk_menu_tool_button_set_arrow_tooltip_text(GTK_MENU_TOOL_BUTTON(tool_item),_("Show background grid"));
  gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM(tool_item),_("Show background grid"));
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->toolbar),GTK_TOOL_ITEM(tool_item),-1);
  //g_signal_connect(tool_item,"clicked",G_CALLBACK(on_toolbar_grid_clicked),(gpointer)self);
#else
  image=gtk_image_new_from_icon_name("buzztard_menu_grid",GTK_ICON_SIZE_MENU);
  tool_item=GTK_WIDGET(gtk_tool_button_new(image,_("Grid")));
  gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM(tool_item),_("Show background grid"));
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->toolbar),GTK_TOOL_ITEM(tool_item),-1);
  g_signal_connect(tool_item,"clicked",G_CALLBACK(on_toolbar_grid_clicked),(gpointer)self);
#endif

#ifndef USE_COMPACT_UI
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->toolbar),gtk_separator_tool_item_new(),-1);
#endif

  // popup menu button
  image=gtk_image_new_from_filename("popup-menu.png");
  tool_item=GTK_WIDGET(gtk_tool_button_new(image,_("Machine view menu")));
  gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM(tool_item),_("Menu actions for machine view below"));
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->toolbar),GTK_TOOL_ITEM(tool_item),-1);
  g_signal_connect(tool_item,"clicked",G_CALLBACK(on_toolbar_menu_clicked),(gpointer)self);

  gtk_box_pack_start(GTK_BOX(self),self->priv->toolbar,FALSE,FALSE,0);

  // add canvas
  scrolled_window=gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),GTK_SHADOW_ETCHED_IN);
  // generate an antialiased canvas
  gtk_widget_push_colormap(gdk_screen_get_system_colormap(gdk_screen_get_default()));
  self->priv->canvas=GNOME_CANVAS(gnome_canvas_new_aa());
  /* the non antialisaed (and faster) version
  self->priv->canvas=GNOME_CANVAS(gnome_canvas_new());
  */
  gtk_widget_pop_colormap();
  gtk_widget_set_can_focus(GTK_WIDGET(self->priv->canvas), TRUE);
  gnome_canvas_set_center_scroll_region(self->priv->canvas,TRUE);
  gnome_canvas_set_scroll_region(self->priv->canvas,
    -MACHINE_VIEW_ZOOM_X,-MACHINE_VIEW_ZOOM_Y,
     MACHINE_VIEW_ZOOM_X, MACHINE_VIEW_ZOOM_Y);
  gnome_canvas_set_pixels_per_unit(self->priv->canvas,self->priv->zoom);
  gtk_widget_set_name(GTK_WIDGET(self->priv->canvas),"machine-and-wire-editor");
  /* DEBUG
  g_signal_connect(self->priv->canvas,"size-allocate",G_CALLBACK(on_canvas_size_allocate),(gpointer)self);
  DEBUG */

  gtk_container_add(GTK_CONTAINER(scrolled_window),GTK_WIDGET(self->priv->canvas));
  gtk_box_pack_start(GTK_BOX(self),scrolled_window,TRUE,TRUE,0);
  bt_main_page_machines_draw_grid(self);

  self->priv->vadjustment=gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolled_window));
  g_signal_connect(self->priv->vadjustment,"value-changed",G_CALLBACK(on_vadjustment_changed),(gpointer)self);
  self->priv->hadjustment=gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scrolled_window));
  g_signal_connect(self->priv->hadjustment,"value-changed",G_CALLBACK(on_hadjustment_changed),(gpointer)self);

  // create volume popup
  self->priv->vol_popup_adj=gtk_adjustment_new(100.0, 0.0, 400.0, 1.0, 10.0, 1.0);
  self->priv->vol_popup=BT_VOLUME_POPUP(bt_volume_popup_new(GTK_ADJUSTMENT(self->priv->vol_popup_adj)));
  g_signal_connect(self->priv->vol_popup_adj,"value-changed",G_CALLBACK(on_volume_popup_changed),(gpointer)self);

  // create panorama popup
  self->priv->pan_popup_adj=gtk_adjustment_new(0.0, -100.0, 100.0, 1.0, 10.0, 1.0);
  self->priv->pan_popup=BT_PANORAMA_POPUP(bt_panorama_popup_new(GTK_ADJUSTMENT(self->priv->pan_popup_adj)));
  g_signal_connect(self->priv->pan_popup_adj,"value-changed",G_CALLBACK(on_panorama_popup_changed),(gpointer)self);

  // register event handlers
  g_signal_connect(self->priv->app, "notify::song", G_CALLBACK(on_song_changed), (gpointer)self);
  g_signal_connect(self->priv->canvas,"event",G_CALLBACK(on_canvas_event),(gpointer)self);
  // listen to page changes
  g_signal_connect((gpointer)pages, "notify::page", G_CALLBACK(on_page_switched), (gpointer)self);

  // let settings control toolbar style
  on_toolbar_style_changed(settings,NULL,(gpointer)self);
  g_signal_connect(settings, "notify::toolbar-style", G_CALLBACK(on_toolbar_style_changed), (gpointer)self);
  g_object_unref(settings);

  GST_DEBUG("  done");
}

//-- constructor methods

/**
 * bt_main_page_machines_new:
 * @pages: the page collection
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtMainPageMachines *bt_main_page_machines_new(const BtMainPages *pages) {
  BtMainPageMachines *self;

  self=BT_MAIN_PAGE_MACHINES(g_object_new(BT_TYPE_MAIN_PAGE_MACHINES,NULL));
  bt_main_page_machines_init_ui(self,pages);
  return(self);
}

//-- methods

/**
 * bt_main_page_machines_wire_volume_popup:
 * @self: the machines page
 * @wire: the wire to popup the volume control for
 * @xpos: the x-position for the popup
 * @ypos: the y-position for the popup
 *
 * Activates the volume-popup for the given wire.
 *
 * Returns: %TRUE for succes.
 */
gboolean bt_main_page_machines_wire_volume_popup(const BtMainPageMachines *self, BtWire *wire, gint xpos, gint ypos) {
  BtMainWindow *main_window;
  gdouble gain;

  g_object_get(self->priv->app,"main-window",&main_window,NULL);
  gtk_window_set_transient_for(GTK_WINDOW(self->priv->vol_popup),GTK_WINDOW(main_window));
  g_object_unref(main_window);

  g_object_try_unref(self->priv->wire_gain);
  g_object_get(wire,"gain",&self->priv->wire_gain,NULL);
  /* set initial value */
  g_object_get(self->priv->wire_gain,"volume",&gain,NULL);
  // FIXME(ensonic): workaround for https://bugzilla.gnome.org/show_bug.cgi?id=667598
  gtk_adjustment_set_value(GTK_ADJUSTMENT(self->priv->vol_popup_adj),(4.0-gain)*100.0);

  /* show directly over mouse-pos */
  /* TODO(ensonic): move it so that the knob is under the mouse */
  //gint mid; // works only after t has once been shown, but we know the height
  //mid=GTK_WIDGET(self->priv->vol_popup)->allocation.height / 2;
  //gtk_window_move(GTK_WINDOW(self->priv->vol_popup),xpos,ypos-mid);
  gtk_window_move(GTK_WINDOW(self->priv->vol_popup),xpos,ypos);
  bt_volume_popup_show(self->priv->vol_popup);

  return(TRUE);
}

/**
 * bt_main_page_machines_wire_panorama_popup:
 * @self: the machines page
 * @wire: the wire to popup the panorama control for
 * @xpos: the x-position for the popup
 * @ypos: the y-position for the popup
 *
 * Activates the panorama-popup for the given wire.
 *
 * Returns: %TRUE for succes.
 */
gboolean bt_main_page_machines_wire_panorama_popup(const BtMainPageMachines *self, BtWire *wire, gint xpos, gint ypos) {
  BtMainWindow *main_window;
  gfloat pan;

  g_object_get(self->priv->app,"main-window",&main_window,NULL);
  gtk_window_set_transient_for(GTK_WINDOW(self->priv->pan_popup),GTK_WINDOW(main_window));
  g_object_unref(main_window);

  g_object_try_unref(self->priv->wire_pan);
  g_object_get(wire,"pan",&self->priv->wire_pan,NULL);
  if(self->priv->wire_pan) {
    /* set initial value */
    g_object_get(self->priv->wire_pan,"panorama",&pan,NULL);
    gtk_adjustment_set_value(GTK_ADJUSTMENT(self->priv->pan_popup_adj),(gdouble)pan*100.0);

    /* show directly over mouse-pos */
    /* TODO(ensonic): move it so that the knob is under the mouse */
    gtk_window_move(GTK_WINDOW(self->priv->pan_popup),xpos,ypos);
    bt_panorama_popup_show(self->priv->pan_popup);
  }
  return(TRUE);
}

static gboolean bt_main_page_machines_add_machine(const BtMainPageMachines *self, guint type, const gchar *id, const gchar *plugin_name) {
  BtSong *song;
  BtMachine *machine=NULL;
  gchar *uid;
  GError *err=NULL;

  g_object_get(self->priv->app,"song",&song,NULL);

  uid=bt_setup_get_unique_machine_id(self->priv->setup,id);
  // try with 1 voice, if monophonic, voices will be reset to 0 in
  // bt_machine_init_voice_params()
  switch(type) {
    case 0:
      machine=BT_MACHINE(bt_source_machine_new(song,uid,plugin_name,/*voices=*/1,&err));
      break;
    case 1:
      machine=BT_MACHINE(bt_processor_machine_new(song,uid,plugin_name,/*voices=*/1,&err));
      break;
  }
  if(err==NULL) {
    BtMachineCanvasItem *mi;
    gchar *undo_str,*redo_str;

    GST_INFO_OBJECT(machine,"created machine %p,ref_ct=%d",machine,G_OBJECT_REF_COUNT(machine));

    bt_change_log_start_group(self->priv->change_log);

    undo_str = g_strdup_printf("rem_machine \"%s\"",uid);
    redo_str = g_strdup_printf("add_machine %u,\"%s\",\"%s\"",type,uid,plugin_name);
    bt_change_log_add(self->priv->change_log,BT_CHANGE_LOGGER(self),undo_str,redo_str);

		if((mi=g_hash_table_lookup(self->priv->machines,machine))) {
			machine_item_moved(self,mi);
		}
		
		bt_change_log_end_group(self->priv->change_log);		
  }
  else {
    GST_WARNING("Can't create machine %s: %s",plugin_name,err->message);
    g_error_free(err);
  }
  g_object_unref(machine);
  g_free(uid);

  g_object_unref(song);

  return(err==NULL);
}

/**
 * bt_main_page_machines_add_source_machine:
 * @self: the machines page
 * @id: the id for the new machine
 * @plugin_name: the plugin-name for the new machine
 *
 * Add a new machine to the machine-page.
 */
gboolean bt_main_page_machines_add_source_machine(const BtMainPageMachines *self, const gchar *id, const gchar *plugin_name) {
  return(bt_main_page_machines_add_machine(self,0,id,plugin_name));
}

/**
 * bt_main_page_machines_add_processor_machine:
 * @self: the machines page
 * @id: the id for the new machine
 * @plugin_name: the plugin-name for the new machine
 *
 * Add a new machine to the machine-page.
 */
gboolean bt_main_page_machines_add_processor_machine(const BtMainPageMachines *self, const gchar *id, const gchar *plugin_name) {
  return(bt_main_page_machines_add_machine(self,1,id,plugin_name));
}

/**
 * bt_main_page_machines_delete_machine:
 * @self: the machines page
 * @machine: the machine to remove
 *
 * Remove a machine from the machine-page.
 */
void bt_main_page_machines_delete_machine(const BtMainPageMachines *self, BtMachine *machine) {
  BtMainWindow *main_window;
  BtMainPageSequence *sequence_page;
  GHashTable *properties;
  gchar *undo_str,*redo_str;
  gchar *mid,*pname,*prop;
  guint type=0;
  BtMachineState machine_state;
  gulong voices;
  gulong pattern_removed_id;

  GST_INFO("before removing machine : %p,ref_ct=%d",machine,G_OBJECT_REF_COUNT(machine));
  bt_change_log_start_group(self->priv->change_log);

  /* by doing undo/redo from setup:machine-removed it happens completely
   * unconstrained, we need to enforce a certain order. Right now the log will
   * reverse the actions. Thus we need to write out the object we remove and
   * then, within, the things that get removed by that.
   * Thus when removing X -
   * 1) we need to log it directly and not in on_x_removed
   * 2) trigger on_x_removed
   * Still this won't let us control the order or serialization of the children
   *
   * We can't control signal emission. Like emitting the signal with details or
   * similar approaches won't fix it nicely either.
   * Somehow the handlers would need to describe their dependencies and then
   * defer their execution. Which they can't as they don't know the context they
   * are called from. A pre-delete signal would not solve it either.
   *
   * BtMainPageMachines:bt_main_page_machines_delete_machine -> BtSetup:machine-removed { 
   *   BtMainPagePatterns:on_machine_removed -> BtMachine::pattern-removed { 
   *     BtMainPageSequence:on_pattern_removed
   *   }
   *   BtMainPageSequence:on_machine_removed
   *   BtWireCanvasItem:on_machine_removed -> BtSetup::wire-removed
   * }
   * Here we have the problem that we have two callbacks changing the sequence
   * data. When removing a machine, we just want handler for machine-removed to
   * do undo/redo, not e.g. pattern-removed.
   *
   * Should we store the signal in change log and check in signal handler if
   * we should be logging?
   *
   * when handling the ui interaction:
   * signal_id=g_signal_lookup("machine-removed",setup)
   * bt_change_log_set_hint(change_log,signal_id);
   *
   * inside the signal handler:
   * hint = g_signal_get_invocation_hint(object);
   * if (bt_change_log_is_hint(change_log,hint->signal_id) {
   *   // do undo/redo serialization
   * }
   *
   * For now we block conflicting handlers.
   */

  // remove patterns for machine, trigger setup::machine-removed
  GST_INFO("removing the machine : %p,ref_ct=%d",machine,G_OBJECT_REF_COUNT(machine));

  if(BT_IS_SOURCE_MACHINE(machine))
    type=0;
  else if(BT_IS_PROCESSOR_MACHINE(machine))
    type=1;
  g_object_get(machine,"id",&mid,"plugin-name",&pname,"properties",&properties,"state",&machine_state,"voices",&voices,NULL);

  bt_change_log_start_group(self->priv->change_log);

  undo_str = g_strdup_printf("add_machine %u,\"%s\",\"%s\"",type,mid,pname);
  redo_str = g_strdup_printf("rem_machine \"%s\"",mid);
  bt_change_log_add(self->priv->change_log,BT_CHANGE_LOGGER(self),undo_str,redo_str);

  undo_str = g_strdup_printf("set_machine_property \"%s\",\"voices\",\"%lu\"",mid,voices);
  bt_change_log_add(self->priv->change_log,BT_CHANGE_LOGGER(self),undo_str,g_strdup(undo_str));
  prop=(gchar *)g_hash_table_lookup(properties,"properties-shown");
  if(prop && *prop=='1') {
    undo_str = g_strdup_printf("set_machine_property \"%s\",\"properties-shown\",\"1\"",mid);
    bt_change_log_add(self->priv->change_log,BT_CHANGE_LOGGER(self),undo_str,g_strdup(undo_str));
  }

  if(machine_state!=BT_MACHINE_STATE_NORMAL) {
    GEnumClass *enum_class=g_type_class_peek_static(BT_TYPE_MACHINE_STATE);
    GEnumValue *enum_value=g_enum_get_value(enum_class,machine_state);
    undo_str = g_strdup_printf("set_machine_property \"%s\",\"state\",\"%s\"",mid,enum_value->value_nick);
    bt_change_log_add(self->priv->change_log,BT_CHANGE_LOGGER(self),undo_str,g_strdup(undo_str));
  }
  // TODO(ensonic): machine parameters
  g_free(mid);g_free(pname);

  // block machine:pattern-removed signal emission for sequence page to not clobber the sequence
  // in theory we we don't need to block it, as we are removing the machine anyway and thus disconnecting
  // but if we disconnect here, disconnecting them on the sequence page would fail
  pattern_removed_id=g_signal_lookup("pattern-removed",BT_TYPE_MACHINE);
  g_object_get(self->priv->app,"main-window",&main_window,NULL);
  bt_child_proxy_get(main_window,"pages::sequence-page",&sequence_page,NULL);
  g_signal_handlers_block_matched(machine,G_SIGNAL_MATCH_ID|G_SIGNAL_MATCH_DATA,pattern_removed_id,0,NULL,NULL,(gpointer)sequence_page);
  g_object_ref(machine);
  bt_setup_remove_machine(self->priv->setup,machine);
  g_signal_handlers_unblock_matched(machine,G_SIGNAL_MATCH_ID|G_SIGNAL_MATCH_DATA,pattern_removed_id,0,NULL,NULL,(gpointer)sequence_page);
  g_object_unref(machine);
  g_object_unref(sequence_page);
  g_object_unref(main_window);

  bt_change_log_end_group(self->priv->change_log);

  // this segfaults if the machine is finalized
  //GST_INFO("... machine : %p,ref_ct=%d",machine,G_OBJECT_REF_COUNT(machine));
  bt_change_log_end_group(self->priv->change_log);
}

/**
 * bt_main_page_machines_delete_wire:
 * @self: the machines page
 * @wire: the wire to remove
 *
 * Remove a wire from the machine-page (unlink the machines).
 */
void bt_main_page_machines_delete_wire(const BtMainPageMachines *self, BtWire *wire) {
  GST_INFO("now removing wire : %p,ref_ct=%d",wire,G_OBJECT_REF_COUNT(wire));
  bt_change_log_start_group(self->priv->change_log);
  bt_setup_remove_wire(self->priv->setup,wire);
  // this segfaults if the wire is finalized
  //GST_INFO("... wire : %p,ref_ct=%d",wire,G_OBJECT_REF_COUNT(wire));
  bt_change_log_end_group(self->priv->change_log);
}

/**
 * bt_main_page_machines_rename_machine:
 * @self: the machines page
 * @machine: the machine to renam
 *
 * Run the machine #BtMachineRenameDialog.
 */
void bt_main_page_machines_rename_machine(const BtMainPageMachines *self, BtMachine *machine) {
  GtkWidget *dialog=GTK_WIDGET(bt_machine_rename_dialog_new(machine));

  bt_edit_application_attach_child_window(self->priv->app,GTK_WINDOW(dialog));
  gtk_widget_show_all(dialog);
  if(gtk_dialog_run(GTK_DIALOG(dialog))==GTK_RESPONSE_ACCEPT) {
  	gchar *new_name;
  	gchar *undo_str,*redo_str;
  	gchar *mid;

    g_object_get(machine,"id",&mid,NULL);
    g_object_get(dialog,"name",&new_name,NULL);

    if(strcmp(mid,new_name)) {
      bt_change_log_start_group(self->priv->change_log);
			undo_str = g_strdup_printf("set_machine_property \"%s\",\"name\",\"%s\"",new_name,mid);
			redo_str = g_strdup_printf("set_machine_property \"%s\",\"name\",\"%s\"",mid,new_name);
			bt_change_log_add(self->priv->change_log,BT_CHANGE_LOGGER(self),undo_str,redo_str);
			bt_change_log_end_group(self->priv->change_log);
    }
    bt_machine_rename_dialog_apply(BT_MACHINE_RENAME_DIALOG(dialog));
    g_free(mid);g_free(new_name);
  }
  gtk_widget_destroy(dialog);
}

//-- change logger interface

static gboolean bt_main_page_machines_change_logger_change(const BtChangeLogger *owner,const gchar *data) {
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES(owner);
  BtSong *song;
  gboolean res=FALSE;
  BtMachine *machine,*smachine,*dmachine;
  BtWire *wire;
  GMatchInfo *match_info;
  gchar *s;

  GST_INFO("undo/redo: [%s]",data);
  // parse data and apply action
  switch (bt_change_logger_match_method(change_logger_methods, data, &match_info)) {
    case METHOD_ADD_MACHINE: {
      gchar *mid,*pname;
      guint type;

      s=g_match_info_fetch(match_info,1);type=atoi(s);g_free(s);
      mid=g_match_info_fetch(match_info,2);
      pname=g_match_info_fetch(match_info,3);
      g_match_info_free(match_info);

      GST_DEBUG("-> [%d|%s|%s]",type,mid,pname);

      g_object_get(self->priv->app,"song",&song,NULL);
      switch (type) {
        case 0:
          machine=BT_MACHINE(bt_source_machine_new(song,mid,pname,/*voices=*/1,NULL));
          break;
        case 1:
          machine=BT_MACHINE(bt_processor_machine_new(song,mid,pname,/*voices=*/1,NULL));
          break;
        default:
          machine=NULL;
          GST_WARNING("unhandled machine type: %d",type);
          break;
      }
      if(machine) {
        res=TRUE;
        g_object_unref(machine);
      }
      g_object_unref(song);

      g_free(mid);
      g_free(pname);
      break;
    }
    case METHOD_REM_MACHINE: {
      gchar *mid;

      mid=g_match_info_fetch(match_info,1);
      g_match_info_free(match_info);

      GST_DEBUG("-> [%s]",mid);

      if((machine=bt_setup_get_machine_by_id(self->priv->setup, mid))) {
        bt_setup_remove_machine(self->priv->setup,machine);
        g_object_unref(machine);
        res=TRUE;
      }
      g_free(mid);
      break;
    }
    case METHOD_SET_MACHINE_PROPERTY: {
      gchar *mid,*key,*val;
      GHashTable *properties;

      mid=g_match_info_fetch(match_info,1);
      key=g_match_info_fetch(match_info,2);
      val=g_match_info_fetch(match_info,3);
      g_match_info_free(match_info);

      GST_DEBUG("-> [%s|%s|%s]",mid,key,val);

      if((machine=bt_setup_get_machine_by_id(self->priv->setup,mid))) {
        BtMachineCanvasItem *item;
        gdouble pn,po;
        gboolean is_prop=FALSE;

        g_object_get(machine,"properties",&properties,NULL);

        res=TRUE;
        if(!strcmp(key,"xpos")) {
          if((item=g_hash_table_lookup(self->priv->machines,machine))) {
            g_object_get(item,"x",&po,NULL);
            pn=(gdouble)(MACHINE_VIEW_ZOOM_X*g_ascii_strtod(val,NULL));
            gnome_canvas_item_move((GnomeCanvasItem *)item,(pn-po),0.0);
            // for some reason a plain g_object_set(item,"x",pn,NULL); would not fully redraw
            g_signal_emit_by_name(item,"position-changed",0);
          }
          is_prop=TRUE;
        } else if(!strcmp(key,"ypos")) {
          if((item=g_hash_table_lookup(self->priv->machines,machine))) {
            g_object_get(item,"y",&po,NULL);
            pn=(gdouble)(MACHINE_VIEW_ZOOM_Y*g_ascii_strtod(val,NULL));
            gnome_canvas_item_move((GnomeCanvasItem *)item,0.0,(pn-po));
            // for some reason a plain g_object_set(item,"y",pn,NULL); would not fully redraw
            g_signal_emit_by_name(item,"position-changed",0);
          }
          is_prop=TRUE;
        } else if(!strcmp(key,"properties-shown")) {
          if(*val=='1') {
            bt_machine_show_properties_dialog(machine);
          }
          is_prop=TRUE;
        } else if(!strcmp(key,"state")) {
          GEnumClass *enum_class=g_type_class_peek_static(BT_TYPE_MACHINE_STATE);
          GEnumValue *enum_value=g_enum_get_value_by_nick(enum_class,val);
          if(enum_value) {
            g_object_set(machine,"state",enum_value->value,NULL);
          }
        }
        else if(!strcmp(key,"voices")) {
          g_object_set(machine,"voices",atol(val),NULL);
        }
        else if(!strcmp(key,"name")) {
          g_object_set(machine,"id",val,NULL);
        }
        else {
        	GST_WARNING("unhandled property '%s'",key);
        	res=FALSE;
        }
        if(is_prop && properties) {
          // take ownership of the strings
          g_hash_table_replace(properties,key,val);
          key=val=NULL;
        }
        g_object_unref(machine);
      }
      g_free(mid);g_free(key);g_free(val);
      break;
    }
    case METHOD_ADD_WIRE: {
      gchar *smid,*dmid;

      smid=g_match_info_fetch(match_info,1);
      dmid=g_match_info_fetch(match_info,2);
      g_match_info_free(match_info);

      GST_DEBUG("-> [%s|%s]",smid,dmid);

      g_object_get(self->priv->app,"song",&song,NULL);
      smachine=bt_setup_get_machine_by_id(self->priv->setup, smid);
      dmachine=bt_setup_get_machine_by_id(self->priv->setup, dmid);

      if(smachine && dmachine) {
        GHashTable *properties;
        gdouble pos_xs,pos_ys,pos_xe,pos_ye;
        BtMachineCanvasItem *src_machine_item,*dst_machine_item;

        wire=bt_wire_new(song,smachine,dmachine,NULL);
        g_object_unref(wire);

        g_object_get(smachine,"properties",&properties,NULL);
        machine_view_get_machine_position(properties,&pos_xs,&pos_ys);
        g_object_get(dmachine,"properties",&properties,NULL);
        machine_view_get_machine_position(properties,&pos_xe,&pos_ye);
        // draw wire
        src_machine_item=g_hash_table_lookup(self->priv->machines,smachine);
        dst_machine_item=g_hash_table_lookup(self->priv->machines,dmachine);
        wire_item_new(self,wire,pos_xs,pos_ys,pos_xe,pos_ye,src_machine_item,dst_machine_item);
        gnome_canvas_item_lower_to_bottom(self->priv->grid);

        res=TRUE;
      }
      g_object_try_unref(smachine);
      g_object_try_unref(dmachine);
      g_object_unref(song);

      g_free(smid);
      g_free(dmid);
      break;
    }
    case METHOD_REM_WIRE: {
      gchar *smid,*dmid;

      smid=g_match_info_fetch(match_info,1);
      dmid=g_match_info_fetch(match_info,2);
      g_match_info_free(match_info);

      GST_DEBUG("-> [%s|%s]",smid,dmid);

      smachine=bt_setup_get_machine_by_id(self->priv->setup, smid);
      dmachine=bt_setup_get_machine_by_id(self->priv->setup, dmid);

      if(smachine && dmachine) {
        if((wire=bt_setup_get_wire_by_machines(self->priv->setup,smachine,dmachine))) {
          bt_setup_remove_wire(self->priv->setup,wire);
          g_object_unref(wire);
          res=TRUE;
        }
      }
      g_object_try_unref(smachine);
      g_object_try_unref(dmachine);

      g_free(smid);
      g_free(dmid);
      break;
    }
    case METHOD_SET_WIRE_PROPERTY: {
      gchar *smid,*dmid,*key,*val;
      GHashTable *properties;

      smid=g_match_info_fetch(match_info,1);
      dmid=g_match_info_fetch(match_info,2);
      key=g_match_info_fetch(match_info,3);
      val=g_match_info_fetch(match_info,4);
      g_match_info_free(match_info);

      GST_DEBUG("-> [%s|%s|%s|%s]",smid,dmid,key,val);

      smachine=bt_setup_get_machine_by_id(self->priv->setup, smid);
      dmachine=bt_setup_get_machine_by_id(self->priv->setup, dmid);

      if(smachine && dmachine) {
        if((wire=bt_setup_get_wire_by_machines(self->priv->setup,smachine,dmachine))) {
          gboolean is_prop=FALSE;
  
          g_object_get(wire,"properties",&properties,NULL);
  
          if(!strcmp(key,"analyzer-shown")) {
            if(*val=='1') {
              bt_wire_show_analyzer_dialog(wire);
            }
            is_prop=TRUE;
          }
          else {
            GST_WARNING("unhandled property '%s'",key);
            res=FALSE;
          }
          if(is_prop && properties) {
            // take ownership of the strings
            g_hash_table_replace(properties,key,val);
            key=val=NULL;
          }
          g_object_unref(wire);
        }
      }
      g_object_try_unref(smachine);
      g_object_try_unref(dmachine);

      g_free(smid);g_free(dmid);g_free(key);g_free(val);
      break;
    }          
    /* TODO(ensonic):
    - machine parameters, wire parameters (volume/panorama)
      - only write them out on release to not blast to much crap into the log
    - open/close machine/wire windows
      - we do it on remove right, but we don't know about intermediate changes
        as thats still done on canvas-item classes
      - the canvas-items now expose their dialogs as a read-only (widget)
        property, we can listen to the notify
    */
    default:
      GST_WARNING("unhandled undo/redo method: [%s]",data);
  }

  GST_INFO("undo/redo: %s : [%s]",(res?"okay":"failed"),data);
  return res;
}

static void bt_main_page_machines_change_logger_interface_init(gpointer const g_iface, gconstpointer const iface_data) {
  BtChangeLoggerInterface * const iface = g_iface;

  iface->change = bt_main_page_machines_change_logger_change;
}

//-- wrapper

//-- class internals

static gboolean bt_main_page_machines_focus(GtkWidget *widget, GtkDirectionType direction) {
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES(widget);
  BtMainWindow *main_window;

  GST_DEBUG("focusing default widget");
  gtk_widget_grab_focus_savely(GTK_WIDGET(self->priv->canvas));

  /* use status bar */
  g_object_get(self->priv->app,"main-window",&main_window,NULL);
  if(main_window) {
    /* it would be nice if we could just do:
     * bt_child_proxy_set(self->priv->app,"main-window::statusbar::status",_(".."),NULL);
     */
    bt_child_proxy_set(main_window,"statusbar::status",_("Add new machines from right click context menu. Connect machines with shift+drag from source to target."),NULL);
    g_object_unref(main_window);
  }
  return FALSE;
}

static void bt_main_page_machines_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec) {
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES(object);
  return_if_disposed();
  switch (property_id) {
    case MAIN_PAGE_MACHINES_CANVAS: {
      g_value_set_object(value, self->priv->canvas);
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

  GST_DEBUG("!!!! self=%p",self);

  GST_DEBUG("  unrefing popups");

  g_object_try_unref(self->priv->wire_gain);
  if(self->priv->vol_popup) {
    bt_volume_popup_hide(self->priv->vol_popup);
    gtk_widget_destroy(GTK_WIDGET(self->priv->vol_popup));
  }
  g_object_try_unref(self->priv->wire_pan);
  if(self->priv->pan_popup) {
    bt_panorama_popup_hide(self->priv->pan_popup);
    gtk_widget_destroy(GTK_WIDGET(self->priv->pan_popup));
  }

  //g_hash_table_foreach_remove(self->priv->machines,canvas_item_destroy,NULL);
  //g_hash_table_foreach_remove(self->priv->wires,canvas_item_destroy,NULL);
  g_signal_handlers_disconnect_matched(self->priv->app,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_song_changed,NULL);

  g_object_try_unref(self->priv->setup);
  
  g_object_unref(self->priv->change_log);
  g_object_unref(self->priv->app);

  gtk_widget_destroy(GTK_WIDGET(self->priv->context_menu));
  g_object_unref(self->priv->context_menu);
  gtk_widget_destroy(GTK_WIDGET(self->priv->grid_density_menu));
  g_object_unref(self->priv->grid_density_menu);

  gdk_cursor_unref(self->priv->drag_cursor);

  GST_DEBUG("  chaining up");
  G_OBJECT_CLASS(bt_main_page_machines_parent_class)->dispose(object);
}

static void bt_main_page_machines_finalize(GObject *object) {
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES(object);

  g_hash_table_destroy(self->priv->machines);
  g_hash_table_destroy(self->priv->wires);

  G_OBJECT_CLASS(bt_main_page_machines_parent_class)->finalize(object);
}

static void bt_main_page_machines_init(BtMainPageMachines *self) {
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_MAIN_PAGE_MACHINES, BtMainPageMachinesPrivate);
  GST_DEBUG("!!!! self=%p",self);
  self->priv->app = bt_edit_application_new();

  self->priv->zoom=MACHINE_VIEW_ZOOM_FC;
  self->priv->grid_density=1;

  self->priv->machines=g_hash_table_new(NULL,NULL);
  self->priv->wires=g_hash_table_new(NULL,NULL);

  // the cursor for dragging
  self->priv->drag_cursor=gdk_cursor_new(GDK_FLEUR);

  self->priv->scroll_x=self->priv->scroll_y=0.5;

  // the undo/redo changelogger
  self->priv->change_log=bt_change_log_new();
  bt_change_log_register(self->priv->change_log,BT_CHANGE_LOGGER(self));
}

static void bt_main_page_machines_class_init(BtMainPageMachinesClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GtkWidgetClass *gtkwidget_class = GTK_WIDGET_CLASS(klass);

  g_type_class_add_private(klass,sizeof(BtMainPageMachinesPrivate));

  gobject_class->get_property = bt_main_page_machines_get_property;
  gobject_class->dispose      = bt_main_page_machines_dispose;
  gobject_class->finalize     = bt_main_page_machines_finalize;

  gtkwidget_class->focus      = bt_main_page_machines_focus;

  g_object_class_install_property(gobject_class,MAIN_PAGE_MACHINES_CANVAS,
                                  g_param_spec_object("canvas",
                                     "canvas prop",
                                     "Get the machine canvas",
                                     GNOME_TYPE_CANVAS, /* object type */
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));
}

