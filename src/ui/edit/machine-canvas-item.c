/* $Id: machine-canvas-item.c,v 1.27 2005-01-06 22:12:03 ensonic Exp $
 * class for the editor machine views machine canvas item
 */

/* @todo:
 * - add muto/solo/bypass buttons
 * - add level meter widgets
 */

#define BT_EDIT
#define BT_MACHINE_CANVAS_ITEM_C

#include "bt-edit.h"

#define BASE_FONT_SIZE 8.0

//-- signal ids

enum {
  POSITION_CHANGED,
  LAST_SIGNAL
};

//-- property ids

enum {
  MACHINE_CANVAS_ITEM_APP=1,
	MACHINE_CANVAS_ITEM_MACHINES_PAGE,
  MACHINE_CANVAS_ITEM_MACHINE,
	MACHINE_CANVAS_ITEM_ZOOM
};


struct _BtMachineCanvasItemPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* the application */
  BtEditApplication *app;
  /* the machine page we are on */
  BtMainPageMachines *main_page_machines;

  /* the underlying machine */
  BtMachine *machine;
  /* and its properties */
  GHashTable *properties;
  
  /* machine context_menu */
  GtkMenu *context_menu;

	/* the properties and preferences dialogs */
	GtkWidget *properties_dialog;
	GtkWidget *preferences_dialog;

	/* the label element */
	GnomeCanvasItem *label;

  /* the zoomration in pixels/per unit */
  double zoom;

  /* interaction state */
  gboolean dragging,moved;
  gdouble offx,offy,dragx,dragy;
};

static guint signals[LAST_SIGNAL]={0,};

static GnomeCanvasGroupClass *parent_class=NULL;

//-- event handler

static void on_machine_properties_dialog_destroy(GtkWidget *widget, gpointer user_data) {
  BtMachineCanvasItem *self=BT_MACHINE_CANVAS_ITEM(user_data);

  g_assert(user_data);

  GST_INFO("machine properties dialog destroy occurred");
  self->priv->properties_dialog=NULL;
}

static void on_machine_preferences_dialog_destroy(GtkWidget *widget, gpointer user_data) {
  BtMachineCanvasItem *self=BT_MACHINE_CANVAS_ITEM(user_data);

  g_assert(user_data);

  GST_INFO("machine preferences dialog destroy occurred");
  self->priv->preferences_dialog=NULL;
}

static void on_context_menu_properties_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMachineCanvasItem *self=BT_MACHINE_CANVAS_ITEM(user_data);
  
  g_assert(user_data);

	if(!self->priv->properties_dialog) {
		self->priv->properties_dialog=GTK_WIDGET(bt_machine_properties_dialog_new(self->priv->app,self->priv->machine));
		gtk_widget_show_all(self->priv->properties_dialog);
		g_signal_connect(G_OBJECT(self->priv->properties_dialog),"destroy",G_CALLBACK(on_machine_properties_dialog_destroy),(gpointer)self);
	}
}

static void on_context_menu_preferences_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMachineCanvasItem *self=BT_MACHINE_CANVAS_ITEM(user_data);
  
  g_assert(user_data);

	if(!self->priv->preferences_dialog) {
		self->priv->preferences_dialog=GTK_WIDGET(bt_machine_preferences_dialog_new(self->priv->app,self->priv->machine));
		gtk_widget_show_all(self->priv->preferences_dialog);
		g_signal_connect(G_OBJECT(self->priv->preferences_dialog),"destroy",G_CALLBACK(on_machine_preferences_dialog_destroy),(gpointer)self);
	}
}

static void on_context_menu_rename_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMachineCanvasItem *self=BT_MACHINE_CANVAS_ITEM(user_data);
  BtMainWindow *main_window;
  gint answer;
  GtkWidget *label,*entry,*icon,*hbox,*vbox;
  gchar *str,*id; 
  GtkWidget *dialog;
  
  g_assert(user_data);
	GST_INFO("context_menu rename event occurred");

	g_object_get(self->priv->app,"main-window",&main_window,NULL);
	g_object_get(self->priv->machine,"id",&id,NULL);
	
	dialog = gtk_dialog_new_with_buttons(_("Rename ..."),
                                        GTK_WINDOW(main_window),
                                        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
                                        NULL);

  hbox=gtk_hbox_new(FALSE,12);
  gtk_container_set_border_width(GTK_CONTAINER(hbox),6);

  icon=gtk_image_new_from_stock(GTK_STOCK_DIALOG_QUESTION,GTK_ICON_SIZE_DIALOG);
  gtk_container_add(GTK_CONTAINER(hbox),icon);
  
	vbox=gtk_vbox_new(FALSE,12);
	gtk_container_set_border_width(GTK_CONTAINER(vbox),0);

  label=gtk_label_new(NULL);
  str=g_strdup_printf("<big><b>%s</b></big>\n\n%s",_("Rename ..."),_("choose a new name for the machine"));
  gtk_label_set_markup(GTK_LABEL(label),str);
  g_free(str);
  gtk_container_add(GTK_CONTAINER(vbox),label);
	entry=gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(entry),id);g_free(id);
	gtk_container_add(GTK_CONTAINER(vbox),entry);
	gtk_container_add(GTK_CONTAINER(hbox),vbox);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox),hbox);
  gtk_widget_show_all(dialog);
                                                  
  answer=gtk_dialog_run(GTK_DIALOG(dialog));
  switch(answer) {
    case GTK_RESPONSE_ACCEPT:
			id=(gchar *)gtk_entry_get_text(GTK_ENTRY(entry));
      GST_INFO("set new name : \"%s\"",id);
			g_object_set(self->priv->machine,"id",g_strdup(id),NULL);
			if(self->priv->label) {
				gnome_canvas_item_set(GNOME_CANVAS_ITEM(self->priv->label),"text",id,NULL);
			}
			// @todo machine needs to be redrawn, pattern and sequence page needs update
      break;
    case GTK_RESPONSE_REJECT:
      GST_INFO("do nothing");
      break;
    default:
      GST_WARNING("unhandled response code = %d",answer);
  }
  gtk_widget_destroy(dialog);
	
	g_object_try_unref(main_window);
}

static void on_context_menu_delete_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMachineCanvasItem *self=BT_MACHINE_CANVAS_ITEM(user_data);
  BtMainWindow *main_window;
	
  g_assert(user_data);
	GST_INFO("context_menu delete event occurred");
	
	g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
	if(bt_dialog_question(main_window,_("Delete machine..."),_("Delete machine..."),_("There is no undo for this."))) {
		BtSong *song;
		BtSetup *setup;
		
  	g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  	g_object_get(G_OBJECT(song),"setup",&setup,NULL);
		
		bt_setup_remove_machine(setup,self->priv->machine);
		bt_main_page_machines_remove_machine_item(self->priv->main_page_machines,self);
		
		g_object_try_unref(setup);
		g_object_try_unref(song);
	}
	g_object_try_unref(main_window);
}

static void on_context_menu_about_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMachineCanvasItem *self=BT_MACHINE_CANVAS_ITEM(user_data);
  GstElement *machine;
  GstElementFactory *element_factory;
  
  g_assert(user_data);

  GST_INFO("context_menu about event occurred");
  // show info about machine
  g_object_get(self->priv->machine,"machine",&machine,NULL);
  
  if(element_factory=gst_element_get_factory(machine)) {
    const gchar *element_longname=gst_element_factory_get_longname(element_factory);
    const gchar *element_author=gst_element_factory_get_author(element_factory);
    const gchar *element_description=gst_element_factory_get_description(element_factory);
    BtMainWindow *main_window;
    gchar *str,*str_author;

    g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
		
		str_author=g_markup_escape_text(element_author,strlen(element_author));
		str=g_strdup_printf(
			_("by %s\n\n%s"),
    	str_author,element_description
  	);
		bt_dialog_message(main_window,_("About ..."),element_longname,str);
		
    g_free(str);g_free(str_author);
    g_object_try_unref(main_window);
  }
  g_object_try_unref(machine);
}

//-- helper methods

static gboolean bt_machine_canvas_item_init_context_menu(const BtMachineCanvasItem *self) {
  GtkWidget *menu_item,*image,*label;

  menu_item=gtk_menu_item_new_with_label(_("Mute"));
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_show(menu_item);
  if(!BT_IS_SINK_MACHINE(self->priv->machine)) {
    menu_item=gtk_menu_item_new_with_label(_("Solo"));
    gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
    gtk_widget_show(menu_item);
  }
  if(BT_IS_PROCESSOR_MACHINE(self->priv->machine)) {
    menu_item=gtk_menu_item_new_with_label(_("Bypass"));
    gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
    gtk_widget_show(menu_item);
  }

  menu_item=gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_set_sensitive(menu_item,FALSE);
  gtk_widget_show(menu_item);

  menu_item=gtk_image_menu_item_new_from_stock(GTK_STOCK_PROPERTIES,NULL);  // dynamic part
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
	// make this menu item bold (default)
	label=gtk_bin_get_child(GTK_BIN(menu_item));
	if(GTK_IS_LABEL(label)) {
		gchar *str=g_strdup_printf("<b>%s</b>",gtk_label_get_text(GTK_LABEL(label)));
		gtk_label_set_markup(GTK_LABEL(label),str);
		g_free(str);
	}
  gtk_widget_show(menu_item);
	g_signal_connect(G_OBJECT(menu_item),"activate",G_CALLBACK(on_context_menu_properties_activate),(gpointer)self);
  menu_item=gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES,NULL); // static part
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_show(menu_item);
	g_signal_connect(G_OBJECT(menu_item),"activate",G_CALLBACK(on_context_menu_preferences_activate),(gpointer)self);

  menu_item=gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_set_sensitive(menu_item,FALSE);
  gtk_widget_show(menu_item);

  menu_item=gtk_menu_item_new_with_label(_("Rename ..."));
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_show(menu_item);
	g_signal_connect(G_OBJECT(menu_item),"activate",G_CALLBACK(on_context_menu_rename_activate),(gpointer)self);
  menu_item=gtk_image_menu_item_new_from_stock(GTK_STOCK_DELETE,NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_show(menu_item);
	g_signal_connect(G_OBJECT(menu_item),"activate",G_CALLBACK(on_context_menu_delete_activate),(gpointer)self);

  menu_item=gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_set_sensitive(menu_item,FALSE);
  gtk_widget_show(menu_item);

  menu_item=gtk_image_menu_item_new_with_label(_("About"));
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  image=gtk_image_new_from_filename("stock_about.png");
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
  gtk_widget_show(menu_item);gtk_widget_show(image);
  g_signal_connect(G_OBJECT(menu_item),"activate",G_CALLBACK(on_context_menu_about_activate),(gpointer)self);
}

//-- constructor methods

/**
 * bt_machine_canvas_item_new:
 * @main_page_machines: the machine page the new item belongs to
 * @xpos: the horizontal location
 * @ypos: the vertical location
 * @zoom: the zoom ratio
 *
 * Create a new instance
 *
 * Returns: the new instance or NULL in case of an error
 */
BtMachineCanvasItem *bt_machine_canvas_item_new(const BtMainPageMachines *main_page_machines,BtMachine *machine,gdouble xpos,gdouble ypos,gdouble zoom) {
	BtMachineCanvasItem *self;
	BtEditApplication *app;
	GnomeCanvas *canvas;

	g_object_get(G_OBJECT(main_page_machines),"app",&app,"canvas",&canvas,NULL);
	
  self=BT_MACHINE_CANVAS_ITEM(gnome_canvas_item_new(gnome_canvas_root(canvas),
                          	BT_TYPE_MACHINE_CANVAS_ITEM,
														"machines-page",main_page_machines,
                          	"app", app,
                          	"machine", machine,
                          	"x", xpos,
                          	"y", ypos,
														"zoom", zoom,
                          	NULL));

	//GST_INFO("machine canvas item added");

	g_object_try_unref(canvas);
	g_object_try_unref(app);
  return(self);
}

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
    case MACHINE_CANVAS_ITEM_APP: {
      g_value_set_object(value, self->priv->app);
    } break;
    case MACHINE_CANVAS_ITEM_MACHINES_PAGE: {
      g_value_set_object(value, self->priv->main_page_machines);
    } break;
    case MACHINE_CANVAS_ITEM_MACHINE: {
      g_value_set_object(value, self->priv->machine);
    } break;
    case MACHINE_CANVAS_ITEM_ZOOM: {
      g_value_set_double(value, self->priv->zoom);
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
    case MACHINE_CANVAS_ITEM_APP: {
      g_object_try_unref(self->priv->app);
      self->priv->app = g_object_try_ref(g_value_get_object(value));
      //GST_DEBUG("set the app for machine_canvas_item: %p",self->priv->app);
    } break;
    case MACHINE_CANVAS_ITEM_MACHINES_PAGE: {
      g_object_try_unref(self->priv->main_page_machines);
      self->priv->main_page_machines = g_object_try_ref(g_value_get_object(value));
      //GST_DEBUG("set the main_page_machines for wire_canvas_item: %p",self->priv->main_page_machines);
    } break;
    case MACHINE_CANVAS_ITEM_MACHINE: {
      g_object_try_unref(self->priv->machine);
      self->priv->machine = g_object_try_ref(g_value_get_object(value));
      if(self->priv->machine) {
        g_object_get(self->priv->machine,"properties",&(self->priv->properties),NULL);
        //GST_DEBUG("set the machine for machine_canvas_item: %p, properties: %p",self->priv->machine,self->priv->properties);
        bt_machine_canvas_item_init_context_menu(self);
      }
    } break;
    case MACHINE_CANVAS_ITEM_ZOOM: {
      self->priv->zoom=g_value_get_double(value);
      //GST_DEBUG("set the zoom for machine_canvas_item: %f",self->priv->zoom);
			if(self->priv->label) {
				gnome_canvas_item_set(GNOME_CANVAS_ITEM(self->priv->label),"size-points",BASE_FONT_SIZE*self->priv->zoom,NULL);
			}
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

	GST_DEBUG("disposing ...");
	
  g_object_try_unref(self->priv->app);
	g_object_try_unref(self->priv->main_page_machines);
  g_object_try_unref(self->priv->machine);
	
	if(self->priv->properties_dialog) {
		gtk_widget_destroy(self->priv->properties_dialog);
	}
  
	gtk_object_destroy(GTK_OBJECT(self->priv->context_menu));
  
  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_machine_canvas_item_finalize(GObject *object) {
  BtMachineCanvasItem *self = BT_MACHINE_CANVAS_ITEM(object);

	GST_DEBUG("finilizing ...");
  
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
  gdouble w=MACHINE_VIEW_MACHINE_SIZE_X,h=MACHINE_VIEW_MACHINE_SIZE_Y;
  guint bg_color=0xFFFFFFFF;
  gchar *id;
  
  if(GNOME_CANVAS_ITEM_CLASS(parent_class)->realize)
    (GNOME_CANVAS_ITEM_CLASS(parent_class)->realize)(citem);
  
  //GST_DEBUG("realize for machine occured, machine=%p",self->priv->machine);

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
  self->priv->label=gnome_canvas_item_new(GNOME_CANVAS_GROUP(citem),
                           GNOME_TYPE_CANVAS_TEXT,
                           "x", +0.0,
                           "y", -3.0,
                           "justification", GTK_JUSTIFY_CENTER,
                           "size-points", BASE_FONT_SIZE*self->priv->zoom,
                           "size-set", TRUE,
                           "text", id,
                           "fill-color", "black",
													 "clip", TRUE,
													 "clip-width",w+w,
													 "clip-height",h+h,
                           NULL);
  g_free(id);
  //item->realized = TRUE;
}

static gboolean bt_machine_canvas_item_event(GnomeCanvasItem *citem, GdkEvent *event) {
  BtMachineCanvasItem *self=BT_MACHINE_CANVAS_ITEM(citem);
  gboolean res=FALSE;
  gdouble dx, dy, px, py;
  GdkCursor *fleur;
  gchar str[G_ASCII_DTOSTR_BUF_SIZE];

  //GST_DEBUG("event for machine occured");
  
  switch (event->type) {
		case GDK_2BUTTON_PRESS:
			GST_DEBUG("GDK_2BUTTON_RELEASE: %d, 0x%x",event->button.button,event->button.state);
			if(!self->priv->properties_dialog) {
				self->priv->properties_dialog=GTK_WIDGET(bt_machine_properties_dialog_new(self->priv->app,self->priv->machine));
  		  gtk_widget_show_all(self->priv->properties_dialog);
				g_signal_connect(G_OBJECT(self->priv->properties_dialog),"destroy",G_CALLBACK(on_machine_properties_dialog_destroy),(gpointer)self);
			}
			res=TRUE;
			break;
    case GDK_BUTTON_PRESS:
      GST_DEBUG("GDK_BUTTON_PRESS: %d, 0x%x",event->button.button,event->button.state);
      if(event->button.button==1) {
       	// dragx/y coords are world coords of button press
       	self->priv->dragx=event->button.x;
       	self->priv->dragy=event->button.y;
       	// set some flags
       	self->priv->dragging=TRUE;
       	self->priv->moved=FALSE;
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
      if(self->priv->dragging) {
				if(!self->priv->moved) {
        	gnome_canvas_item_raise_to_top(citem);
        	fleur=gdk_cursor_new(GDK_FLEUR);
        	gnome_canvas_item_grab(citem, GDK_POINTER_MOTION_MASK |
          	                    /* GDK_ENTER_NOTIFY_MASK | */
            	                  /* GDK_LEAVE_NOTIFY_MASK | */
          GDK_BUTTON_RELEASE_MASK, fleur, event->button.time);					
				}
        dx=event->button.x-self->priv->dragx;
        dy=event->button.y-self->priv->dragy;
        gnome_canvas_item_move(citem, dx, dy);
        // change position properties of the machines
        g_object_get(citem,"x",&px,"y",&py,NULL);
        //GST_DEBUG("GDK_MOTION_NOTIFY: pre  %+5.1f,%+5.1f",px,py);
        px/=MACHINE_VIEW_ZOOM_X;
        py/=MACHINE_VIEW_ZOOM_Y;
        //GST_DEBUG("GDK_MOTION_NOTIFY: %+5.1f,%+5.1f -> %+5.1f,%+5.1f",event->button.x,event->button.y,px,py);
        g_hash_table_insert(self->priv->properties,g_strdup("xpos"),g_strdup(g_ascii_dtostr(str,G_ASCII_DTOSTR_BUF_SIZE,px)));
        g_hash_table_insert(self->priv->properties,g_strdup("ypos"),g_strdup(g_ascii_dtostr(str,G_ASCII_DTOSTR_BUF_SIZE,py)));
        g_signal_emit(citem,signals[POSITION_CHANGED],0);
        self->priv->dragx=event->button.x;
        self->priv->dragy=event->button.y;
        self->priv->moved=TRUE;
        res=TRUE;
      }
      break;
    case GDK_BUTTON_RELEASE:
      GST_DEBUG("GDK_BUTTON_RELEASE: %d",event->button.button);
      if(self->priv->dragging) {
        self->priv->dragging=FALSE;
				if(self->priv->moved) {
        	gnome_canvas_item_ungrab(citem,event->button.time);
				}
        res=TRUE;
      }
      break;
    default:
      break;
  }
  /* we don't want the click falling through to the parent canvas item, if we have handled it */
  //if(res) {
  //  g_signal_stop_emission_by_name(citem->canvas,"event-after");
  //}
	if(!res) {
		if(GNOME_CANVAS_ITEM_CLASS(parent_class)->event) {
			res=(GNOME_CANVAS_ITEM_CLASS(parent_class)->event)(citem,event);
		}
	}
  return res;
}

static void bt_machine_canvas_item_init(GTypeInstance *instance, gpointer g_class) {
  BtMachineCanvasItem *self = BT_MACHINE_CANVAS_ITEM(instance);
  
  self->priv = g_new0(BtMachineCanvasItemPrivate,1);
  self->priv->dispose_has_run = FALSE;

  // generate the context menu  
  self->priv->context_menu=GTK_MENU(gtk_menu_new());
  // the menu is generated in bt_machine_canvas_item_init_context_menu()
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

  klass->position_changed = NULL;

  /** 
	 * BtMachineCanvasItem::position-changed
   * @self: the machine-canvas-item object that emitted the signal
	 *
	 * signals that item has been moved around.
	 */
  signals[POSITION_CHANGED] = g_signal_new("position-changed",
                                        G_TYPE_FROM_CLASS(klass),
                                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                                        G_STRUCT_OFFSET(BtMachineCanvasItemClass,position_changed),
                                        NULL, // accumulator
                                        NULL, // acc data
                                        g_cclosure_marshal_VOID__VOID,
                                        G_TYPE_NONE, // return type
                                        0, // n_params
                                        NULL /* param data */ );

  g_object_class_install_property(gobject_class,MACHINE_CANVAS_ITEM_APP,
                                  g_param_spec_object("app",
                                     "app contruct prop",
                                     "Set application object, the window belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
#ifndef GNOME_CANVAS_BROKEN_PROPERTIES
                                     G_PARAM_CONSTRUCT_ONLY |
#endif
                                     G_PARAM_READWRITE));
  
  g_object_class_install_property(gobject_class,MACHINE_CANVAS_ITEM_MACHINES_PAGE,
                                  g_param_spec_object("machines-page",
                                     "machines-page contruct prop",
                                     "Set application object, the window belongs to",
                                     BT_TYPE_MAIN_PAGE_MACHINES, /* object type */
#ifndef GNOME_CANVAS_BROKEN_PROPERTIES
                                     G_PARAM_CONSTRUCT_ONLY |
#endif
                                     G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,MACHINE_CANVAS_ITEM_MACHINE,
                                  g_param_spec_object("machine",
                                     "machine contruct prop",
                                     "Set machine object, the item belongs to",
                                     BT_TYPE_MACHINE, /* object type */
#ifndef GNOME_CANVAS_BROKEN_PROPERTIES                                     
                                     G_PARAM_CONSTRUCT_ONLY |
#endif
                                     G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,MACHINE_CANVAS_ITEM_ZOOM,
                                  g_param_spec_double("zoom",
                                     "zoom prop",
                                     "Set zoom ratio for the machine item",
                                     0.0,
																		 100.0,
																		 1.0,
                                     G_PARAM_READWRITE));
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
			NULL // value_table
    };
		type = g_type_register_static(GNOME_TYPE_CANVAS_GROUP,"BtMachineCanvasItem",&info,0);
  }
  return type;
}
