// $Id: main-page-machines.c,v 1.86 2006-08-05 23:29:22 ensonic Exp $
/**
 * SECTION:btmainpagemachines
 * @short_description: the editor main machines page
 */ 

#define BT_EDIT
#define BT_MAIN_PAGE_MACHINES_C

#include "bt-edit.h"

enum {
  MAIN_PAGE_MACHINES_APP=1,
  MAIN_PAGE_MACHINES_CANVAS
};

struct _BtMainPageMachinesPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* the application */
  union {
    BtEditApplication *app;
    gpointer app_ptr;
  };

  /* the toolbar widget */
  GtkWidget *toolbar;
  
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
  gboolean connecting,moved;
  gdouble offx,offy,dragx,dragy;

  /* cursor for moving */
  GdkCursor *drag_cursor;

  /* used when interactivly adding a new wire*/
  GnomeCanvasItem *new_wire;
  GnomeCanvasPoints *new_wire_points;
  BtMachineCanvasItem *new_wire_src,*new_wire_dst;
  
  /* cached setup properties */
  GHashTable *properties;

  /* mouse coodinates on context menu invokation (used for placing machines there */
  gdouble mouse_x,mouse_y;
  
  /* volume popup slider */
  BtVolumePopup *vol_popup;
  GtkObject *vol_popup_adj;
  BtWire *vol_popup_wire;
};

static GtkVBoxClass *parent_class=NULL;

//-- data helper

gboolean canvas_item_destroy(gpointer key,gpointer value,gpointer user_data) {
  gtk_object_destroy(GTK_OBJECT(value));
  return(TRUE);
}

//-- event handler helper

// @todo this method probably should go to BtMachine, but on the other hand it is GUI related
void machine_view_get_machine_position(GHashTable *properties, gdouble *pos_x,gdouble *pos_y) {
  gchar *prop;

  *pos_x=*pos_y=0.0;
  if(properties) {
    prop=(gchar *)g_hash_table_lookup(properties,"xpos");
    if(prop) {
      *pos_x=MACHINE_VIEW_ZOOM_X*g_ascii_strtod(prop,NULL);
      // do not g_free(prop);
      //GST_DEBUG("  xpos: %+5.1f  %p=\"%s\"",*pos_x,prop,prop);
    }
    else GST_WARNING("no xpos property found");
    prop=(gchar *)g_hash_table_lookup(properties,"ypos");
    if(prop) {
      *pos_y=MACHINE_VIEW_ZOOM_Y*g_ascii_strtod(prop,NULL);
      // do not g_free(prop);
      //GST_DEBUG("  ypos: %+5.1f  %p=\"%s\"",*pos_y,prop,prop);
    }
    else GST_WARNING("no ypos property found");
  }
  else GST_WARNING("no properties supplied");
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
  
  g_hash_table_foreach(self->priv->machines,update_machine_zoom,&self->priv->zoom);
  gtk_widget_set_sensitive(self->priv->zoom_out,(self->priv->zoom>0.4));
  gtk_widget_set_sensitive(self->priv->zoom_in,(self->priv->zoom<3.0));
}

static void machine_item_new(const BtMainPageMachines *self,BtMachine *machine,gdouble xpos,gdouble ypos) {
  BtMachineCanvasItem *item;

  item=bt_machine_canvas_item_new(self,machine,xpos,ypos,self->priv->zoom);
  g_hash_table_insert(self->priv->machines,machine,item);
}

static void wire_item_new(const BtMainPageMachines *self,BtWire *wire,gdouble pos_xs,gdouble pos_ys,gdouble pos_xe,gdouble pos_ye,BtMachineCanvasItem *src_machine_item,BtMachineCanvasItem *dst_machine_item) {
  BtWireCanvasItem *item;

  item=bt_wire_canvas_item_new(self,wire,pos_xs,pos_ys,pos_xe,pos_ye,src_machine_item,dst_machine_item);
  g_hash_table_insert(self->priv->wires,wire,item);
}

static void machine_view_refresh(const BtMainPageMachines *self,const BtSetup *setup) {
  GHashTable *properties;
  BtMachineCanvasItem *src_machine_item,*dst_machine_item;
  BtMachine *machine,*src_machine,*dst_machine;
  BtWire *wire;
  gdouble pos_x,pos_y;
  gdouble pos_xs,pos_ys,pos_xe,pos_ye;
  GList *node,*list;
  gchar *prop;
  
  // clear the canvas
  GST_DEBUG("before destoying machine canvas items");
  g_hash_table_foreach_remove(self->priv->machines,canvas_item_destroy,NULL);
  GST_DEBUG("before destoying wire canvas items");
  g_hash_table_foreach_remove(self->priv->wires,canvas_item_destroy,NULL);
  GST_DEBUG("done");
  
  // update view
  g_object_get(G_OBJECT(setup),"properties",&self->priv->properties,NULL);
  if((prop=(gchar *)g_hash_table_lookup(self->priv->properties,"zoom"))) {
    self->priv->zoom=g_ascii_strtod(prop,NULL);
    gnome_canvas_set_pixels_per_unit(self->priv->canvas,self->priv->zoom);
    GST_INFO("set zoom to %6.4lf",self->priv->zoom);
  }
  if((prop=(gchar *)g_hash_table_lookup(self->priv->properties,"xpos"))) {
    gtk_adjustment_set_value(self->priv->hadjustment,g_ascii_strtod(prop,NULL));
  }
  if((prop=(gchar *)g_hash_table_lookup(self->priv->properties,"ypos"))) {
    gtk_adjustment_set_value(self->priv->vadjustment,g_ascii_strtod(prop,NULL));
  }

  // draw all machines
  g_object_get(G_OBJECT(setup),"machines",&list,NULL);
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
  g_object_get(G_OBJECT(setup),"wires",&list,NULL);
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
    g_object_try_unref(src_machine);
    g_object_try_unref(dst_machine);
  }
  g_list_free(list);
  gnome_canvas_item_lower_to_bottom(self->priv->grid);
  GST_DEBUG("drawing done");
}

static void bt_main_page_machines_draw_grid(const BtMainPageMachines *self) {
  GnomeCanvasPoints *points;
  gdouble s,step;
  gulong color;
  
  GST_INFO("redrawing grid : density=%d  canvas=%p",self->priv->grid_density,self->priv->canvas);
  
  // delete old grid-item and generate a new one (pushing it to bottom)
  if(self->priv->grid) gtk_object_destroy(GTK_OBJECT(self->priv->grid));
  self->priv->grid=gnome_canvas_item_new(gnome_canvas_root(self->priv->canvas),
                           GNOME_TYPE_CANVAS_GROUP,"x",0,0,"y",0,0,NULL);
  gnome_canvas_item_lower_to_bottom(self->priv->grid);
  
  if(!self->priv->grid_density) return;
  
  points=gnome_canvas_points_new(2);
  
  // low=1->2, mid=2->4, high=3->8
  step=(MACHINE_VIEW_ZOOM_X+MACHINE_VIEW_ZOOM_X)/(gdouble)(1<<self->priv->grid_density);
  for(s=-(MACHINE_VIEW_ZOOM_X*MACHINE_VIEW_GRID_FC);s<=(MACHINE_VIEW_ZOOM_X*MACHINE_VIEW_GRID_FC);s+=step) {
    points->coords[0]=s;points->coords[1]=-(MACHINE_VIEW_ZOOM_Y*MACHINE_VIEW_GRID_FC);
    points->coords[2]=s;points->coords[3]= (MACHINE_VIEW_ZOOM_Y*MACHINE_VIEW_GRID_FC);
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
  for(s=-(MACHINE_VIEW_ZOOM_Y*MACHINE_VIEW_GRID_FC);s<=(MACHINE_VIEW_ZOOM_Y*MACHINE_VIEW_GRID_FC);s+=step) {
    points->coords[0]=-(MACHINE_VIEW_ZOOM_X*MACHINE_VIEW_GRID_FC);points->coords[1]=s;
    points->coords[2]= (MACHINE_VIEW_ZOOM_X*MACHINE_VIEW_GRID_FC);points->coords[3]=s;
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
  BtSetup *setup;
  BtWire *wire;
  BtMachine *src_machine,*dst_machine;
  GHashTable *properties;
  gdouble pos_xs,pos_ys,pos_xe,pos_ye;

  g_assert(self->priv->new_wire_src);
  g_assert(self->priv->new_wire_dst);
  
  g_object_get(self->priv->app,"song",&song,NULL);
  g_object_get(song,"setup",&setup,NULL);
  g_object_get(self->priv->new_wire_src,"machine",&src_machine,NULL);
  g_object_get(self->priv->new_wire_dst,"machine",&dst_machine,NULL);

  // try to establish a new connection
  if((wire=bt_wire_new(song,src_machine,dst_machine))) {
    g_object_get(src_machine,"properties",&properties,NULL);
    machine_view_get_machine_position(properties,&pos_xs,&pos_ys);
    g_object_get(dst_machine,"properties",&properties,NULL);
    machine_view_get_machine_position(properties,&pos_xe,&pos_ye);
    // draw wire
    wire_item_new(self,wire,pos_xs,pos_ys,pos_xe,pos_ye,self->priv->new_wire_src,self->priv->new_wire_dst);
    g_object_unref(wire);
  }
  g_object_try_unref(dst_machine);
  g_object_try_unref(src_machine);
  g_object_try_unref(setup);
  g_object_try_unref(song);
}

static BtMachineCanvasItem *bt_main_page_machines_get_machine_canvas_item_at(const BtMainPageMachines *self,gdouble mouse_x,gdouble mouse_y) {
  BtMachineCanvasItem *mitem=NULL;
  GnomeCanvasItem *ci,*pci;

  //GST_DEBUG("is there a machine at pos ?");
  
  if((ci=gnome_canvas_get_item_at(self->priv->canvas,mouse_x,mouse_y))) {
    g_object_get(G_OBJECT(ci),"parent",&pci,NULL);
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
  BtSong *song;
  BtSetup *setup;
  BtWire *wire1=NULL,*wire2=NULL;
  BtMachine *src_machine,*dst_machine;
  
  GST_INFO("can we link to it ?");
  
  g_assert(self->priv->new_wire_src);
  g_assert(self->priv->new_wire_dst);

  g_object_get(self->priv->app,"song",&song,NULL);
  g_object_get(song,"setup",&setup,NULL);
  g_object_get(self->priv->new_wire_src,"machine",&src_machine,NULL);
  g_object_get(self->priv->new_wire_dst,"machine",&dst_machine,NULL);
  
  // if the citem->machine is a sink/processor-machine
  if(BT_IS_SINK_MACHINE(dst_machine) || BT_IS_PROCESSOR_MACHINE(dst_machine)) {
    // check if these machines are not yet connected
    wire1=bt_setup_get_wire_by_machines(setup,src_machine,dst_machine);
    wire2=bt_setup_get_wire_by_machines(setup,dst_machine,src_machine);
    if((!wire1) && (!wire2)) {
      ret=TRUE;
      GST_INFO("  yes!");
    }
    else {
      g_object_try_unref(wire1);
      g_object_try_unref(wire2);
    }
  }
  g_object_try_unref(dst_machine);
  g_object_try_unref(src_machine);
  g_object_try_unref(setup);
  g_object_try_unref(song);
  return(ret);
}


//-- event handler

static void on_machine_added(BtSetup *setup,BtMachine *machine,gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);
  GHashTable *properties;
  
  g_assert(user_data);
  GST_INFO("new machine has been added");

  g_object_get(machine,"properties",&properties,NULL);
  if(properties) {
    gchar str[G_ASCII_DTOSTR_BUF_SIZE];
    g_hash_table_insert(properties,g_strdup("xpos"),g_strdup(g_ascii_dtostr(str,G_ASCII_DTOSTR_BUF_SIZE,(self->priv->mouse_x/MACHINE_VIEW_ZOOM_X))));
    g_hash_table_insert(properties,g_strdup("ypos"),g_strdup(g_ascii_dtostr(str,G_ASCII_DTOSTR_BUF_SIZE,(self->priv->mouse_y/MACHINE_VIEW_ZOOM_Y))));
  }

  // draw machine
  machine_item_new(self,machine,self->priv->mouse_x,self->priv->mouse_y);
}

static void on_song_changed(const BtEditApplication *app,GParamSpec *arg,gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);
  BtSong *song;
  BtSetup *setup;

  g_assert(user_data);

  GST_INFO("song has changed : app=%p, self=%p",app,self);
  // get song from app
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_return_if_fail(song);

  g_object_get(G_OBJECT(song),"setup",&setup,NULL);
  // update page
  machine_view_refresh(self,setup);
  g_signal_connect(G_OBJECT(setup),"machine-added",G_CALLBACK(on_machine_added),(gpointer)self);
  // release the reference
  g_object_try_unref(setup);
  g_object_try_unref(song);
}

static void on_toolbar_zoom_fit_clicked(GtkButton *button, gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);
  BtSong *song;
  BtSetup *setup;
  BtMachine *machine;
  GHashTable *properties;
  GList *node,*list;
  gdouble fc_x,fc_y,c_x,c_y;
  // machine area
  gdouble ma_xs=MACHINE_VIEW_ZOOM_X,ma_x,ma_xe=-MACHINE_VIEW_ZOOM_X,ma_xd;
  gdouble ma_ys=MACHINE_VIEW_ZOOM_Y,ma_y,ma_ye=-MACHINE_VIEW_ZOOM_Y,ma_yd;
  // page area
  gdouble pg_xs,pg_x,pg_xe,pg_xd;
  gdouble pg_ys,pg_y,pg_ye,pg_yd;

  g_assert(user_data);

  //calculate bounds
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_object_get(G_OBJECT(song),"setup",&setup,NULL);
  g_object_get(G_OBJECT(setup),"machines",&list,NULL);
  for(node=list;node;node=g_list_next(node)) {
    machine=BT_MACHINE(node->data);
    // get position
    g_object_get(machine,"properties",&properties,NULL);
    machine_view_get_machine_position(properties,&ma_x,&ma_y);
    ma_x/=MACHINE_VIEW_ZOOM_X;
    ma_y/=MACHINE_VIEW_ZOOM_Y;
    if(ma_x<ma_xs) ma_xs=ma_x;
    if(ma_x>ma_xe) ma_xe=ma_x;
    if(ma_y<ma_ys) ma_ys=ma_y;
    if(ma_y>ma_ye) ma_ye=ma_y;
    GST_INFO("machines: x:%+6.4lf y:%+6.4lf -> ranging from x:%+6.4lf...%+6.4lf and y:%+6.4lf...%+6.4lf",ma_x,ma_y,ma_xs,ma_xe,ma_ys,ma_ye);
  }
  g_list_free(list);
  g_object_unref(setup);
  g_object_unref(song);
  /* @todo: need to add machine extends + some space */
  GST_INFO("machines ranging from x:%+6.4lf...%+6.4lf and y:%+6.4lf...%+6.4lf",ma_xs,ma_xe,ma_ys,ma_ye);
  ma_xd=ma_xe-ma_xs;
  ma_yd=ma_ye-ma_ys;
  
  g_object_get(G_OBJECT(self->priv->hadjustment),"lower",&pg_xs,"value",&pg_x,"upper",&pg_xe,NULL);
  g_object_get(G_OBJECT(self->priv->vadjustment),"lower",&pg_ys,"value",&pg_y,"upper",&pg_ye,NULL);
  pg_xd=(pg_xe-pg_xs)/MACHINE_VIEW_ZOOM_X;
  pg_yd=(pg_ye-pg_ys)/MACHINE_VIEW_ZOOM_Y;
  GST_INFO("page: x:%+6.4lf y:%+6.4lf -> ranging from x:%+6.4lf...%+6.4lf and y:%+6.4lf...%+6.4lf",pg_x,pg_y,pg_xs,pg_xe,pg_ys,pg_ye);

  // zoom
  fc_x=pg_xd/ma_xd;
  fc_y=pg_yd/ma_yd;
  GST_INFO("zoom x:%+6.4lf / %+6.4lf = %+6.4lf and y:%+6.4lf / %+6.4lf = %+6.4lf",pg_xd,ma_xd,fc_x,pg_yd,ma_yd,fc_y);
  self->priv->zoom=MIN(fc_x,fc_y);
  gnome_canvas_set_pixels_per_unit(self->priv->canvas,self->priv->zoom);
  // center
  /* @todo: this still is wrong :( */
  c_x=MACHINE_VIEW_ZOOM_X+MACHINE_VIEW_ZOOM_X*(ma_xs+(ma_xd/2.0));
  c_y=MACHINE_VIEW_ZOOM_Y+MACHINE_VIEW_ZOOM_Y*(ma_ys+(ma_yd/2.0));
  gtk_adjustment_set_value(self->priv->hadjustment,c_x);
  gtk_adjustment_set_value(self->priv->vadjustment,c_y);
  
  GST_INFO("toolbar zoom_fit event occurred: zoom = %lf, center x/y = %+6.4lf,%+6.4lf",self->priv->zoom,c_x,c_y);
  update_machines_zoom(self);
}

static void on_toolbar_zoom_in_clicked(GtkButton *button, gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);

  g_assert(user_data);

  self->priv->zoom*=1.5;
  GST_INFO("toolbar zoom_in event occurred : %lf",self->priv->zoom);
  gnome_canvas_set_pixels_per_unit(self->priv->canvas,self->priv->zoom);
  update_machines_zoom(self);
}

static void on_toolbar_zoom_out_clicked(GtkButton *button, gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);

  g_assert(user_data);

  self->priv->zoom/=1.5;
  GST_INFO("toolbar zoom_out event occurred : %lf",self->priv->zoom);
  gnome_canvas_set_pixels_per_unit(self->priv->canvas,self->priv->zoom);
  update_machines_zoom(self);
}

/*
static void on_toolbar_grid_clicked(GtkButton *button, gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);

  g_assert(user_data);

  GST_INFO("toolbar grid clicked event occurred");
  gtk_menu_popup(self->priv->grid_density_menu,NULL,NULL,NULL,NULL,1,gtk_get_current_event_time());
}
*/

static void on_toolbar_grid_density_off_activated(GtkMenuItem *menuitem, gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);
  BtSettings *settings;

  if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem))) return;

  g_assert(user_data);
  self->priv->grid_density=0;

  g_object_get(G_OBJECT(self->priv->app),"settings",&settings,NULL);
  g_object_set(G_OBJECT(settings),"grid-density","off",NULL);
  g_object_unref(settings);

  bt_main_page_machines_draw_grid(self);
}

static void on_toolbar_grid_density_low_activated(GtkMenuItem *menuitem, gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);
  BtSettings *settings;

  if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem))) return;

  g_assert(user_data);
  self->priv->grid_density=1;

  g_object_get(G_OBJECT(self->priv->app),"settings",&settings,NULL);
  g_object_set(G_OBJECT(settings),"grid-density","low",NULL);
  g_object_unref(settings);

  bt_main_page_machines_draw_grid(self);
}

static void on_toolbar_grid_density_mid_activated(GtkMenuItem *menuitem, gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);
  BtSettings *settings;

  if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem))) return;

  g_assert(user_data);
  self->priv->grid_density=2;

  g_object_get(G_OBJECT(self->priv->app),"settings",&settings,NULL);
  g_object_set(G_OBJECT(settings),"grid-density","medium",NULL);
  g_object_unref(settings);

  bt_main_page_machines_draw_grid(self);
}

static void on_toolbar_grid_density_high_activated(GtkMenuItem *menuitem, gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);
  BtSettings *settings;

  if(!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem))) return;

  g_assert(user_data);
  self->priv->grid_density=3;

  g_object_get(G_OBJECT(self->priv->app),"settings",&settings,NULL);
  g_object_set(G_OBJECT(settings),"grid-density","high",NULL);
  g_object_unref(settings);

  bt_main_page_machines_draw_grid(self);
}

static void on_vadjustment_changed(GtkAdjustment *adjustment, gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);
  gchar str[G_ASCII_DTOSTR_BUF_SIZE];
  
  g_hash_table_insert(self->priv->properties,g_strdup("ypos"),g_strdup(g_ascii_dtostr(str,G_ASCII_DTOSTR_BUF_SIZE,gtk_adjustment_get_value(adjustment))));  
}

static void on_hadjustment_changed(GtkAdjustment *adjustment, gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);
  gchar str[G_ASCII_DTOSTR_BUF_SIZE];
  
  g_hash_table_insert(self->priv->properties,g_strdup("xpos"),g_strdup(g_ascii_dtostr(str,G_ASCII_DTOSTR_BUF_SIZE,gtk_adjustment_get_value(adjustment))));  
}


static gboolean on_canvas_event(GnomeCanvas *canvas, GdkEvent *event, gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);
  gboolean res=FALSE;
  GnomeCanvasItem *ci,*pci;
  gdouble mouse_x,mouse_y;
  gchar *color;
  BtMachine *machine;
  
  //GST_INFO("canvas event received: type=%d", event->type);

  g_assert(user_data);
  switch(event->type) {
    case GDK_BUTTON_PRESS:
      /*
      {
        GdkEventButton *e=(GdkEventButton*)event;
        GST_INFO("type=%4d, window=%p, send_event=%3d, time=%8d",e->type,e->window,e->send_event,e->time);
        GST_INFO("x=%6.4lf, y=%6.4lf, axes=%p, state=%4d",e->x,e->y,e->axes,e->state);
        GST_INFO("button=%4d, device=%p, x_root=%6.4lf, y_root=%6.4lf\n",e->button,e->device,e->x_root,e->y_root);
      }
      */
    
      // store mouse coordinates, so that we can later place a newly added machine there
      gnome_canvas_window_to_world(self->priv->canvas,event->button.x,event->button.y,&self->priv->mouse_x,&self->priv->mouse_y);
      if(!(ci=gnome_canvas_get_item_at(self->priv->canvas,self->priv->mouse_x,self->priv->mouse_y))) {
        GST_DEBUG("GDK_BUTTON_PRESS: %d",event->button.button);
        if(event->button.button==3) {
          // show context menu
          gtk_menu_popup(self->priv->context_menu,NULL,NULL,NULL,NULL,3,gtk_get_current_event_time());
          res=TRUE;
        }
      }
      else {
        if((event->button.button==1) && (event->button.state&GDK_SHIFT_MASK)) {
          g_object_get(G_OBJECT(ci),"parent",&pci,NULL);
          if(BT_IS_MACHINE_CANVAS_ITEM(pci)) {
            self->priv->new_wire_src=BT_MACHINE_CANVAS_ITEM(pci);
            g_object_get(G_OBJECT(pci),"machine",&machine,NULL);
            // if the citem->machine is a source/processor-machine
            if(BT_IS_SOURCE_MACHINE(machine) || BT_IS_PROCESSOR_MACHINE(machine)) {
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
              self->priv->connecting=TRUE;
              self->priv->moved=FALSE;
              res=TRUE;
            }
            g_object_unref(machine);
          }
          else g_object_unref(pci);
        }
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
        gtk_object_destroy(GTK_OBJECT(self->priv->new_wire));self->priv->new_wire=NULL;
        gnome_canvas_points_free(self->priv->new_wire_points);self->priv->new_wire_points=NULL;
        self->priv->connecting=FALSE;
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
  
  g_object_get(G_OBJECT(settings),"toolbar-style",&toolbar_style,NULL);
  
  GST_INFO("!!!  toolbar style has changed '%s'",toolbar_style);
  gtk_toolbar_set_style(GTK_TOOLBAR(self->priv->toolbar),gtk_toolbar_get_style_from_string(toolbar_style));
  g_free(toolbar_style);
}


static void on_volume_popup_changed(GtkAdjustment *adj, gpointer user_data) {
  BtMainPageMachines *self=BT_MAIN_PAGE_MACHINES(user_data);
  gdouble gain;

  GST_INFO(">>>> CHANGED");

  //if (self->priv->lock) return;
  //self->priv->lock = TRUE;

  gain = gtk_adjustment_get_value (adj);
  g_object_set(G_OBJECT(self->priv->vol_popup_wire),"gain",gain,NULL);

  //self->priv->lock = FALSE;
  //self->priv->force_next_update = TRUE;
}


//-- helper methods

static void bt_main_page_machines_init_main_context_menu(const BtMainPageMachines *self) {
  GtkWidget *menu_item,*menu,*image;

  self->priv->context_menu=GTK_MENU(gtk_menu_new());

  //menu_item=gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD,NULL);
  menu_item=gtk_image_menu_item_new_with_label(_("Add nachine"));
  image=gtk_image_new_from_stock(GTK_STOCK_ADD,GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_show(menu_item);
  // add machine selection sub-menu
  menu=GTK_WIDGET(bt_machine_menu_new(self->priv->app));
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item),menu);

  menu_item=gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_set_sensitive(menu_item,FALSE);
  gtk_widget_show(menu_item);

  menu_item=gtk_menu_item_new_with_label(_("Unmute all machines"));
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  /* @todo: implement me */
  gtk_widget_show(menu_item);
}

static void bt_main_page_machines_init_grid_density_menu(const BtMainPageMachines *self) {
  GtkWidget *menu_item;
  
  // create grid-density menu with grid-density={off,low,mid,high}  
  self->priv->grid_density_menu=GTK_MENU(gtk_menu_new());

  menu_item=gtk_radio_menu_item_new_with_label(self->priv->grid_density_group,_("Off"));
  self->priv->grid_density_group=gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
  if(self->priv->grid_density==0) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),TRUE);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->grid_density_menu),menu_item);
  gtk_widget_show(menu_item);
  g_signal_connect(G_OBJECT(menu_item),"activate",G_CALLBACK(on_toolbar_grid_density_off_activated),(gpointer)self);

  menu_item=gtk_radio_menu_item_new_with_label(self->priv->grid_density_group,_("Low"));
  self->priv->grid_density_group=gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
  if(self->priv->grid_density==1) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),TRUE);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->grid_density_menu),menu_item);
  gtk_widget_show(menu_item);
  g_signal_connect(G_OBJECT(menu_item),"activate",G_CALLBACK(on_toolbar_grid_density_low_activated),(gpointer)self);

  menu_item=gtk_radio_menu_item_new_with_label(self->priv->grid_density_group,_("Medium"));
  self->priv->grid_density_group=gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
  if(self->priv->grid_density==2) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),TRUE);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->grid_density_menu),menu_item);
  gtk_widget_show(menu_item);
  g_signal_connect(G_OBJECT(menu_item),"activate",G_CALLBACK(on_toolbar_grid_density_mid_activated),(gpointer)self);

  menu_item=gtk_radio_menu_item_new_with_label(self->priv->grid_density_group,_("High"));
  self->priv->grid_density_group=gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));
  if(self->priv->grid_density==3) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),TRUE);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->grid_density_menu),menu_item);
  gtk_widget_show(menu_item);
  g_signal_connect(G_OBJECT(menu_item),"activate",G_CALLBACK(on_toolbar_grid_density_high_activated),(gpointer)self);
}

static gboolean bt_main_page_machines_init_ui(const BtMainPageMachines *self) {
  BtSettings *settings;
  GtkWidget *image,*scrolled_window;
  GtkWidget *tool_item;
  GtkTooltips *tips;
  gchar *density;

  GST_DEBUG("!!!! self=%p",self);
  
  gtk_widget_set_name(GTK_WIDGET(self),_("machine view"));
  
  tips=gtk_tooltips_new();
  g_object_get(G_OBJECT(self->priv->app),"settings",&settings,NULL);
  
  g_object_get(G_OBJECT(settings),"grid-density",&density,NULL);
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
  gtk_widget_set_name(self->priv->toolbar,_("machine view tool bar"));
 
  tool_item=GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_ZOOM_FIT));
  gtk_widget_set_name(tool_item,_("Zoom Fit"));
  gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(tool_item),GTK_TOOLTIPS(tips),_("Zoom in/out so that everything is visible"),NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->toolbar),GTK_TOOL_ITEM(tool_item),-1);
  g_signal_connect(G_OBJECT(tool_item),"clicked",G_CALLBACK(on_toolbar_zoom_fit_clicked),(gpointer)self);

  self->priv->zoom_in=GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_ZOOM_IN));
  gtk_widget_set_name(self->priv->zoom_in,_("Zoom In"));
  gtk_widget_set_sensitive(self->priv->zoom_in,(self->priv->zoom<3.0));
  gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(self->priv->zoom_in),GTK_TOOLTIPS(tips),_("Zoom in so more details are visible"),NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->toolbar),GTK_TOOL_ITEM(self->priv->zoom_in),-1);
  g_signal_connect(G_OBJECT(self->priv->zoom_in),"clicked",G_CALLBACK(on_toolbar_zoom_in_clicked),(gpointer)self);

  self->priv->zoom_out=GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_ZOOM_OUT));
  gtk_widget_set_name(self->priv->zoom_out,_("Zoom Out"));
  gtk_widget_set_sensitive(self->priv->zoom_out,(self->priv->zoom>0.4));
  gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(self->priv->zoom_out),GTK_TOOLTIPS(tips),_("Zoom out for better overview"),NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->toolbar),GTK_TOOL_ITEM(self->priv->zoom_out),-1);
  g_signal_connect(G_OBJECT(self->priv->zoom_out),"clicked",G_CALLBACK(on_toolbar_zoom_out_clicked),(gpointer)self);

  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->toolbar),gtk_separator_tool_item_new(),-1);

  // grid density toolbar icon
  image=gtk_image_new_from_filename("grid.png");
  tool_item=GTK_WIDGET(gtk_menu_tool_button_new(image,_("Grid")));
  gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(tool_item),GTK_WIDGET(self->priv->grid_density_menu));
  gtk_menu_tool_button_set_arrow_tooltip(GTK_MENU_TOOL_BUTTON(tool_item),GTK_TOOLTIPS(tips),_("Show background grid"),NULL);
  gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(tool_item),GTK_TOOLTIPS(tips),_("Show background grid"),NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->toolbar),GTK_TOOL_ITEM(tool_item),-1);
  //g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(on_toolbar_grid_clicked),(gpointer)self);

  gtk_box_pack_start(GTK_BOX(self),self->priv->toolbar,FALSE,FALSE,0);

  // add canvas
  scrolled_window=gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),GTK_SHADOW_ETCHED_IN);
  // generate an antialiased canvas
  gtk_widget_push_colormap(gdk_rgb_get_colormap());
  self->priv->canvas=GNOME_CANVAS(gnome_canvas_new_aa());
  /* the non antialisaed (and faster) version 
  gtk_widget_push_colormap(gdk_imlib_get_colormap());
  self->priv->canvas=GNOME_CANVAS(gnome_canvas_new());
  */
  gtk_widget_pop_colormap();
  // seems to be ignored
  //gtk_widget_add_events(GTK_WIDGET(self->priv->canvas),GDK_KEY_PRESS_MASK|GDK_KEY_RELEASE_MASK);
  gnome_canvas_set_center_scroll_region(self->priv->canvas,TRUE);
  gnome_canvas_set_scroll_region(self->priv->canvas,
    -MACHINE_VIEW_ZOOM_X,-MACHINE_VIEW_ZOOM_Y,
     MACHINE_VIEW_ZOOM_X, MACHINE_VIEW_ZOOM_Y);
  gnome_canvas_set_pixels_per_unit(self->priv->canvas,self->priv->zoom);
  
  gtk_container_add(GTK_CONTAINER(scrolled_window),GTK_WIDGET(self->priv->canvas));
  gtk_box_pack_start(GTK_BOX(self),scrolled_window,TRUE,TRUE,0);
  bt_main_page_machines_draw_grid(self);

  // register event handlers
  g_signal_connect(G_OBJECT(self->priv->app), "notify::song", G_CALLBACK(on_song_changed), (gpointer)self);
  g_signal_connect(G_OBJECT(self->priv->canvas),"event",G_CALLBACK(on_canvas_event),(gpointer)self);
  
  self->priv->vadjustment=gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolled_window));
  g_signal_connect(G_OBJECT(self->priv->vadjustment),"value-changed",G_CALLBACK(on_vadjustment_changed),(gpointer)self);
  self->priv->hadjustment=gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scrolled_window));
  g_signal_connect(G_OBJECT(self->priv->hadjustment),"value-changed",G_CALLBACK(on_hadjustment_changed),(gpointer)self);

  // let settings control toolbar style
  on_toolbar_style_changed(settings,NULL,(gpointer)self);
  g_signal_connect(G_OBJECT(settings), "notify::toolbar-style", G_CALLBACK(on_toolbar_style_changed), (gpointer)self);

  // create volume popup
  self->priv->vol_popup_adj=gtk_adjustment_new(1.0, 0.0, 4.0, 0.05, 0.1, 0.0);
  self->priv->vol_popup=BT_VOLUME_POPUP(bt_volume_popup_new(GTK_ADJUSTMENT(self->priv->vol_popup_adj)));
  g_signal_connect(G_OBJECT(self->priv->vol_popup_adj),"value-changed",G_CALLBACK(on_volume_popup_changed),(gpointer)self);

  g_object_unref(settings);

  GST_DEBUG("  done");
  return(TRUE);
}

//-- constructor methods

/**
 * bt_main_page_machines_new:
 * @app: the application the window belongs to
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtMainPageMachines *bt_main_page_machines_new(const BtEditApplication *app) {
  BtMainPageMachines *self;

  if(!(self=BT_MAIN_PAGE_MACHINES(g_object_new(BT_TYPE_MAIN_PAGE_MACHINES,"app",app,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_main_page_machines_init_ui(self)) {
    goto Error;
  }
  return(self);
Error:
  g_object_try_unref(self);
  return(NULL);
}

//-- methods

/**
 * bt_main_page_machines_remove_machine_item:
 * @self: the machines page the contains the item
 * @item: the machine canvas object to remove
 *
 * Removes a machine canvas item from the canvas.
 */
void bt_main_page_machines_remove_machine_item(const BtMainPageMachines *self, BtMachineCanvasItem *item) {
  BtMachine *machine;
  
  g_object_get(G_OBJECT(item),"machine",&machine,NULL);
  g_hash_table_remove(self->priv->machines,machine);
  gtk_object_destroy(GTK_OBJECT(item));

  g_object_try_unref(machine);
}

/**
 * bt_main_page_machines_remove_wire_item:
 * @self: the machines page the contains the item
 * @item: the wire canvas object to remove
 *
 * Removes a wire canvas item from the canvas.
 */
void bt_main_page_machines_remove_wire_item(const BtMainPageMachines *self, BtWireCanvasItem *item) {
  BtWire *wire;
  
  g_object_get(G_OBJECT(item),"wire",&wire,NULL);
  g_hash_table_remove(self->priv->wires,wire);
  gtk_object_destroy(GTK_OBJECT(item));

  g_object_try_unref(wire);
}

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
  GtkWidget *popup=GTK_WIDGET(self->priv->vol_popup);
  BtMainWindow *main_window;
  gdouble gain;
  
  g_object_get(self->priv->app,"main-window",&main_window,NULL);
  gtk_window_set_transient_for(GTK_WINDOW(self->priv->vol_popup),GTK_WINDOW(main_window));
  g_object_try_unref(main_window);

  self->priv->vol_popup_wire=wire;
  /* set initial value */
  g_object_get(G_OBJECT(wire),"gain",&gain,NULL);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(self->priv->vol_popup_adj),gain);
  
  /* @todo: show directly over mouse-pos (check: bacon_volume_button_press) */
  gtk_window_move(GTK_WINDOW(popup),xpos,ypos);
  bt_volume_popup_show(self->priv->vol_popup);

  return(TRUE);
}


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
    case MAIN_PAGE_MACHINES_CANVAS: {
      g_value_set_object(value, self->priv->canvas);
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
      g_object_try_weak_unref(self->priv->app);
      self->priv->app = BT_EDIT_APPLICATION(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->app);
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

  GST_DEBUG("!!!! self=%p",self);  
  if(self->priv->vol_popup) {
    GST_DEBUG("  unrefing popup");
    bt_volume_popup_hide(self->priv->vol_popup);
    //gtk_widget_unparent(GTK_WIDGET(self->priv->vol_popup));
    gtk_object_destroy(GTK_OBJECT(self->priv->vol_popup));
  }

  //g_hash_table_foreach_remove(self->priv->machines,canvas_item_destroy,NULL);
  //g_hash_table_foreach_remove(self->priv->wires,canvas_item_destroy,NULL);
  g_signal_handlers_disconnect_matched(self->priv->app,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_song_changed,NULL);
  g_object_try_weak_unref(self->priv->app);
  
  gtk_object_destroy(GTK_OBJECT(self->priv->grid_density_menu));
  gtk_object_destroy(GTK_OBJECT(self->priv->context_menu));

  gdk_cursor_unref(self->priv->drag_cursor);

  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_main_page_machines_finalize(GObject *object) {
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES(object);

  g_hash_table_destroy(self->priv->machines);
  g_hash_table_destroy(self->priv->wires);

  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void bt_main_page_machines_init(GTypeInstance *instance, gpointer g_class) {
  BtMainPageMachines *self = BT_MAIN_PAGE_MACHINES(instance);
  
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_MAIN_PAGE_MACHINES, BtMainPageMachinesPrivate);

  self->priv->zoom=MACHINE_VIEW_ZOOM_FC;
  self->priv->grid_density=1;
  
  self->priv->machines=g_hash_table_new(g_direct_hash,g_direct_equal);
  self->priv->wires=g_hash_table_new(g_direct_hash,g_direct_equal);

  // the cursor for dragging
  self->priv->drag_cursor=gdk_cursor_new(GDK_FLEUR);
}

static void bt_main_page_machines_class_init(BtMainPageMachinesClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtMainPageMachinesPrivate));
  
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

  g_object_class_install_property(gobject_class,MAIN_PAGE_MACHINES_CANVAS,
                                  g_param_spec_object("canvas",
                                     "canvas prop",
                                     "Get the machine canvas",
                                     GNOME_TYPE_CANVAS, /* object type */
                                     G_PARAM_READABLE));
}

GType bt_main_page_machines_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    static const GTypeInfo info = {
      G_STRUCT_SIZE(BtMainPageMachinesClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_main_page_machines_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtMainPageMachines),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_main_page_machines_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(GTK_TYPE_VBOX,"BtMainPageMachines",&info,0);
  }
  return type;
}
