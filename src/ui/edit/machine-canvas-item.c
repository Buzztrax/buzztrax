/* $Id: machine-canvas-item.c,v 1.4 2004-11-04 10:38:14 ensonic Exp $
 * class for the editor machine views machine canvas item
 */

#define BT_EDIT
#define BT_MACHINE_CANVAS_ITEM_C

#include "bt-edit.h"

//-- signal ids

enum {
  POSITION_CHANGED,
  LAST_SIGNAL
};

//-- property ids

enum {
  MACHINE_CANVAS_ITEM_APP=1,
  MACHINE_CANVAS_ITEM_MACHINE,
};


struct _BtMachineCanvasItemPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* the application */
  BtEditApplication *app;
  
  /* the underlying machine */
  BtMachine *machine;
  /* and its properties */
  GHashTable *properties;
  
  /* machine context_menu */
  GtkMenu *context_menu;

  /* interaction state */
  gboolean dragging,moved;
  gdouble offx,offy,dragx,dragy;
  
};

static guint signals[LAST_SIGNAL]={0,};

static GnomeCanvasGroupClass *parent_class=NULL;

//-- event handler

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
    GtkWidget *label,*icon,*box;
    GtkWidget *dialog;
    gchar *str,*str_author;

    g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
    dialog = gtk_dialog_new_with_buttons(_("About ..."),
                                          GTK_WINDOW(main_window),
                                          GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_OK,
                                          GTK_RESPONSE_ACCEPT,
                                          NULL);
  
    box=gtk_hbox_new(FALSE,12);
    gtk_container_set_border_width(GTK_CONTAINER(box),6);
  
    icon=gtk_image_new_from_stock(GTK_STOCK_DIALOG_INFO,GTK_ICON_SIZE_DIALOG);
    gtk_container_add(GTK_CONTAINER(box),icon);
    
    label=gtk_label_new(NULL);
    str_author=g_markup_escape_text(element_author,strlen(element_author));
    GST_INFO("machine_info: \"%s\" -> \"%s\"",element_author,str_author);
    
    str=g_strdup_printf(
      "<big><b>%s</b></big>\nby %s\n\n%s\n\n",
      element_longname,str_author,element_description
    );
    gtk_label_set_markup(GTK_LABEL(label),str);
    g_free(str);g_free(str_author);
    gtk_container_add(GTK_CONTAINER(box),label);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox),box);
    gtk_widget_show_all(dialog);
  
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    g_object_try_unref(main_window);
  }
  g_object_try_unref(machine);
}

//-- helper methods

static gboolean bt_machine_canvas_item_init_context_menu(const BtMachineCanvasItem *self) {
  GtkWidget *menu_item,*image;

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
  gtk_widget_show(menu_item);
  menu_item=gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES,NULL); // static part
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_show(menu_item);

  menu_item=gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_set_sensitive(menu_item,FALSE);
  gtk_widget_show(menu_item);

  menu_item=gtk_menu_item_new_with_label(_("Rename ..."));
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_show(menu_item);
  menu_item=gtk_image_menu_item_new_from_stock(GTK_STOCK_DELETE,NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_show(menu_item);

  menu_item=gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_set_sensitive(menu_item,FALSE);
  gtk_widget_show(menu_item);

  menu_item=gtk_image_menu_item_new_with_label(_("About"));
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  image=create_pixmap("stock_about.png");
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
  gtk_widget_show(menu_item);gtk_widget_show(image);
  g_signal_connect(G_OBJECT(menu_item),"activate",G_CALLBACK(on_context_menu_about_activate),(gpointer)self);
}

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
    case MACHINE_CANVAS_ITEM_APP: {
      g_value_set_object(value, self->priv->app);
    } break;
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
    case MACHINE_CANVAS_ITEM_APP: {
      g_object_try_unref(self->priv->app);
      self->priv->app = g_object_try_ref(g_value_get_object(value));
      //GST_DEBUG("set the app for machine_canvas_item: %p",self->priv->app);
    } break;
    case MACHINE_CANVAS_ITEM_MACHINE: {
      g_object_try_unref(self->priv->machine);
      self->priv->machine = g_object_try_ref(g_value_get_object(value));
      if(self->priv->machine) {
        g_object_get(self->priv->machine,"properties",&(self->priv->properties),NULL);
        GST_DEBUG("set the machine for machine_canvas_item: %p, properties: %p",self->priv->machine,self->priv->properties);
        bt_machine_canvas_item_init_context_menu(self);
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

  g_object_try_unref(self->priv->app);
  g_object_try_unref(self->priv->machine);
  
  g_object_unref(self->priv->context_menu);
  
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
  gdouble w=MACHINE_VIEW_MACHINE_SIZE_X,h=MACHINE_VIEW_MACHINE_SIZE_Y;
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
  gdouble dx, dy, px, py;
  GdkCursor *fleur;
  gboolean res=FALSE;

  //GST_DEBUG("event for machine occured");
  
  switch (event->type) {
    case GDK_BUTTON_PRESS:
      GST_DEBUG("GDK_BUTTON_PRESS: %d",event->button.button);
      if(event->button.button==1) {
        /* dragxy coords are world coords of button press */
        self->priv->dragx=event->button.x;
        self->priv->dragy=event->button.y;
        /* set some flags */
        self->priv->dragging=TRUE;
        self->priv->moved=FALSE;
        gnome_canvas_item_raise_to_top(citem);
        fleur=gdk_cursor_new(GDK_FLEUR);
        gnome_canvas_item_grab(citem, GDK_POINTER_MOTION_MASK |
                              /* GDK_ENTER_NOTIFY_MASK | */
                              /* GDK_LEAVE_NOTIFY_MASK | */
            GDK_BUTTON_RELEASE_MASK, fleur, event->button.time);
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
        dx=event->button.x-self->priv->dragx;
        dy=event->button.y-self->priv->dragy;
        gnome_canvas_item_move(citem, dx, dy);
        // change position properties of the machines
        g_object_get(citem,"x",&px,"y",&py,NULL);
        px/=MACHINE_VIEW_ZOOM_X;
        py/=MACHINE_VIEW_ZOOM_Y;
        //GST_DEBUG("GDK_MOTION_NOTIFY: %f,%f -> %f,%f",event->button.x,event->button.y,px,py);
        g_hash_table_insert(self->priv->properties,g_strdup("xpos"),g_strdup_printf("%f",px));
        g_hash_table_insert(self->priv->properties,g_strdup("ypos"),g_strdup_printf("%f",py));
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
        gnome_canvas_item_ungrab(citem,event->button.time);
        //g_signal_emit(citem,signals[POSITION_CHANGED],0);
        res=TRUE;
      }
      break;
    default:
      break;
  }
  /* we don't want the click falling through to the parent canvas item, if we have handled it */
  return res;
}

static void bt_machine_canvas_item_init(GTypeInstance *instance, gpointer g_class) {
  BtMachineCanvasItem *self = BT_MACHINE_CANVAS_ITEM(instance);
  
  self->priv = g_new0(BtMachineCanvasItemPrivate,1);
  self->priv->dispose_has_run = FALSE;

  // generate the context menu  
  self->priv->context_menu=gtk_menu_new();
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
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));
  
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

