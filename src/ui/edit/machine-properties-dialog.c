/* $Id: machine-properties-dialog.c,v 1.24 2005-06-29 19:49:05 ensonic Exp $
 * class for the machine properties dialog
 */

#define BT_EDIT
#define BT_MACHINE_PROPERTIES_DIALOG_C

#include "bt-edit.h"

//-- property ids

enum {
  MACHINE_PROPERTIES_DIALOG_APP=1,
  MACHINE_PROPERTIES_DIALOG_MACHINE
};

struct _BtMachinePropertiesDialogPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* the application */
  BtEditApplication *app;

  /* the underlying machine */
  BtMachine *machine;
  
  /* widgets and their handlers */
  //GtkWidget *widgets;
  //gulong *notify_id,*change_id;
};

static GtkDialogClass *parent_class=NULL;

//-- event handler

static void on_double_range_property_changed(GtkRange *range,gpointer user_data);
static void on_int_range_property_changed(GtkRange *range,gpointer user_data);

#ifdef USE_GST_DPARAMS
static void on_double_range_property_notify(const GstDParam *dparam,GParamSpec *property,gpointer user_data) {
#endif
#ifdef USE_GST_CONTROLLER
static void on_double_range_property_notify(const GstElement *machine,GParamSpec *property,gpointer user_data) {
#endif  
  GtkWidget *widget=GTK_WIDGET(user_data);
  gdouble value;
  
  g_assert(user_data);
  //GST_INFO("property value notify received : %s ",property->name);

  gdk_threads_try_enter();
#ifdef USE_GST_DPARAMS
  g_object_get(G_OBJECT(dparam),property->name,&value,NULL);
  g_signal_handlers_block_matched(widget,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_double_range_property_changed,(gpointer)dparam);
  gtk_range_set_value(GTK_RANGE(widget),value);
  g_signal_handlers_unblock_matched(widget,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_double_range_property_changed,(gpointer)dparam);
#endif
#ifdef USE_GST_CONTROLLER
  g_object_get(G_OBJECT(machine),property->name,&value,NULL);
  g_signal_handlers_block_matched(widget,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_double_range_property_changed,(gpointer)machine);
  gtk_range_set_value(GTK_RANGE(widget),value);
  g_signal_handlers_unblock_matched(widget,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_double_range_property_changed,(gpointer)machine);
#endif
  gdk_threads_try_leave();
}

static void on_double_range_property_changed(GtkRange *range,gpointer user_data) {
#ifdef USE_GST_DPARAMS
  GstDParam *dparam=GST_DPARAM(user_data);
#endif
#ifdef USE_GST_CONTROLLER
  GstElement *machine=GST_ELEMENT(user_data);
  const gchar *name=gtk_widget_get_name(GTK_WIDGET(range));
#endif
  gdouble value;
  
  g_assert(user_data);
  //GST_INFO("property value change received");

  //gdk_threads_enter();
  value=gtk_range_get_value(range);
#ifdef USE_GST_DPARAMS
  g_signal_handlers_block_matched(dparam,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_double_range_property_notify,(gpointer)range);
  g_object_set(dparam,"value-double",value,NULL);
  g_signal_handlers_unblock_matched(dparam,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_double_range_property_notify,(gpointer)range);
#endif
#ifdef USE_GST_CONTROLLER
  g_signal_handlers_block_matched(machine,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_double_range_property_notify,(gpointer)range);
  g_object_set(machine,name,value,NULL);
  g_signal_handlers_unblock_matched(machine,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_double_range_property_notify,(gpointer)range);
#endif
  //gdk_threads_leave();
}

#ifdef USE_GST_DPARAMS
static void on_int_range_property_notify(const GstDParam *dparam,GParamSpec *property,gpointer user_data) {
#endif
#ifdef USE_GST_CONTROLLER
static void on_int_range_property_notify(const GstElement *machine,GParamSpec *property,gpointer user_data) {
#endif
  GtkWidget *widget=GTK_WIDGET(user_data);
  gint value;
  
  g_assert(user_data);
  //GST_INFO("property value notify received : %s ",property->name);

  gdk_threads_try_enter();
#ifdef USE_GST_DPARAMS
  g_object_get(G_OBJECT(dparam),property->name,&value,NULL);
  g_signal_handlers_block_matched(widget,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_int_range_property_changed,(gpointer)dparam);
  gtk_range_set_value(GTK_RANGE(widget),(gdouble)value);
  g_signal_handlers_unblock_matched(widget,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_int_range_property_changed,(gpointer)dparam);
#endif
#ifdef USE_GST_CONTROLLER
  g_object_get(G_OBJECT(machine),property->name,&value,NULL);
  g_signal_handlers_block_matched(widget,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_int_range_property_changed,(gpointer)machine);
  gtk_range_set_value(GTK_RANGE(widget),(gdouble)value);
  g_signal_handlers_unblock_matched(widget,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_int_range_property_changed,(gpointer)machine);
#endif
  gdk_threads_try_leave();
}

static void on_int_range_property_changed(GtkRange *range,gpointer user_data) {
#ifdef USE_GST_DPARAMS
  GstDParam *dparam=GST_DPARAM(user_data);
#endif
#ifdef USE_GST_CONTROLLER
  GstElement *machine=GST_ELEMENT(user_data);
  const gchar *name=gtk_widget_get_name(GTK_WIDGET(range));
#endif
  gint value;
  
  g_assert(user_data);
  //GST_INFO("property value change received");

  //gdk_threads_enter();
  value=(gint)gtk_range_get_value(range);
#ifdef USE_GST_DPARAMS
  g_signal_handlers_block_matched(dparam,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_int_range_property_notify,(gpointer)range);
  g_object_set(dparam,"value-int",value,NULL);
  g_signal_handlers_unblock_matched(dparam,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_int_range_property_notify,(gpointer)range);
#endif
#ifdef USE_GST_CONTROLLER
  g_signal_handlers_block_matched(machine,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_int_range_property_notify,(gpointer)range);
  g_object_set(machine,name,value,NULL);
  g_signal_handlers_unblock_matched(machine,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_int_range_property_notify,(gpointer)range);
#endif
  //gdk_threads_leave();
}

static gchar* on_int_range_property_format_value(GtkScale *scale, gdouble value, gpointer user_data) {
#ifdef USE_GST_CONTROLLER
  BtMachine *machine=BT_MACHINE(user_data);
  const gchar *name=gtk_widget_get_name(GTK_WIDGET(scale));
#endif
	GValue int_value={0,};
	glong index=bt_machine_get_global_param_index(machine,name,NULL);
	
  g_value_init(&int_value,G_TYPE_INT);
	g_value_set_int(&int_value,(gint)value);
  return(bt_machine_describe_global_param_value(machine,index,&int_value));
}

//-- helper methods

static gboolean bt_machine_properties_dialog_init_ui(const BtMachinePropertiesDialog *self) {
  BtMainWindow *main_window;
  GtkWidget *box,*label,*widget,*table,*scrolled_window;
  GtkTooltips *tips=gtk_tooltips_new();
  gchar *id,*title;
  GdkPixbuf *window_icon=NULL;
  gulong i,global_params;
  GParamSpec *property;
	GValue *range_min,*range_max;
  GType param_type;
  GstElement *machine;
#ifdef USE_GST_DPARAMS
  GstDParam *dparam;
#endif
  
  g_object_get(self->priv->app,"main-window",&main_window,NULL);
  gtk_window_set_transient_for(GTK_WINDOW(self),GTK_WINDOW(main_window));

  // create and set window icon
  if((window_icon=bt_ui_ressources_get_pixbuf_by_machine(self->priv->machine))) {
    gtk_window_set_icon(GTK_WINDOW(self),window_icon);
  }
  
  // leave the choice of width to gtk
  gtk_window_set_default_size(GTK_WINDOW(self),-1,200);
  // set a title
  g_object_get(self->priv->machine,"id",&id,"global-params",&global_params,"machine",&machine,NULL);
  title=g_strdup_printf(_("%s properties"),id);
  gtk_window_set_title(GTK_WINDOW(self),title);
  g_free(id);g_free(title);
    
  // add widgets to the dialog content area
  box=gtk_vbox_new(FALSE,12);
  //gtk_container_set_border_width(GTK_CONTAINER(box),6);
  
  // @todo add preset controls (combobox, edit button, copy, random, help)
  gtk_box_pack_start(GTK_BOX(box),gtk_label_new("no preset selection here yet"),FALSE,FALSE,0);

  // add separator
  gtk_box_pack_start(GTK_BOX(box),gtk_hseparator_new(),FALSE,FALSE,0);
  
  if(global_params/*+voices*voice_params*/) {
#ifdef USE_GST_CONTROLLER
    gchar *signal_name;
#endif
    
    GST_INFO("machine has %d properties",global_params);
    // machine controls inside a scrolled window
    scrolled_window=gtk_scrolled_window_new(NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_NEVER,GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),GTK_SHADOW_NONE);
    // add machine controls into the table
    table=gtk_table_new(/*rows=*/global_params+1,/*columns=*/2,/*homogenous=*/FALSE);
    for(i=0;i<global_params;i++) {
#ifdef USE_GST_DPARAMS
      dparam=bt_machine_get_global_dparam(self->priv->machine,i);
#endif
      property=bt_machine_get_global_param_spec(self->priv->machine,i);
      GST_INFO("property %p has name '%s','%s'",property,property->name,bt_machine_get_global_param_name(self->priv->machine,i));
      // get name
      label=gtk_label_new((gchar *)bt_machine_get_global_param_name(self->priv->machine,i));
      gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
      gtk_table_attach(GTK_TABLE(table),label, 0, 1, i, i+1, GTK_FILL,GTK_SHRINK, 2,1);

      param_type=bt_machine_get_global_param_type(self->priv->machine,i);
			range_min=bt_machine_get_global_param_min_value(self->priv->machine,i);
			range_max=bt_machine_get_global_param_max_value(self->priv->machine,i);
			// DEBUG
			if(range_min && range_max) {
				gchar *str_min=g_strdup_value_contents(range_min);
				gchar *str_max=g_strdup_value_contents(range_max);
				GST_INFO("... has range : %s ... %s",str_min,str_max);
				g_free(str_min);g_free(str_max);
			}
			// DEBUG

      // @todo choose proper widgets
      if(param_type==G_TYPE_STRING) {
        widget=gtk_label_new("string");
      }
      else if(param_type==G_TYPE_INT) {
        gint value;
        
#ifdef USE_GST_DPARAMS
        g_object_get(G_OBJECT(dparam),"value-int",&value,NULL);
#endif
#ifdef USE_GST_CONTROLLER
        g_object_get(G_OBJECT(machine),property->name,&value,NULL);
#endif
        // @todo make it a check box when range ist 0...1 ?
        // @todo how to detect option menus
        //step=(int_property->maximum-int_property->minimum)/1024.0;
        widget=gtk_hscale_new_with_range(g_value_get_int(range_min),g_value_get_int(range_max),1.0);
        gtk_scale_set_draw_value(GTK_SCALE(widget),TRUE);
        gtk_scale_set_value_pos(GTK_SCALE(widget),GTK_POS_RIGHT);
        gtk_range_set_value(GTK_RANGE(widget),value);
        gtk_widget_set_name(GTK_WIDGET(widget),property->name);
        // @todo add numerical entry as well ?
#ifdef USE_GST_DPARAMS
        g_signal_connect(G_OBJECT(dparam), "notify::value-int", (GCallback)on_int_range_property_notify, (gpointer)widget);
        g_signal_connect(G_OBJECT(widget), "value-changed", (GCallback)on_int_range_property_changed, (gpointer)dparam);
#endif
#ifdef USE_GST_CONTROLLER
        signal_name=g_strdup_printf("notify::%s",property->name);
        g_signal_connect(G_OBJECT(machine), signal_name, (GCallback)on_int_range_property_notify, (gpointer)widget);
        g_signal_connect(G_OBJECT(widget), "value-changed", (GCallback)on_int_range_property_changed, (gpointer)machine);
        g_signal_connect(G_OBJECT(widget), "format-value", (GCallback)on_int_range_property_format_value, (gpointer)self->priv->machine);
        g_free(signal_name);
#endif
      }
      else if(param_type==G_TYPE_DOUBLE) {
        gdouble step,value;
				gdouble value_min=g_value_get_double(range_min);
				gdouble value_max=g_value_get_double(range_max);

#ifdef USE_GST_DPARAMS
        g_object_get(G_OBJECT(dparam),"value-double",&value,NULL);
#endif
#ifdef USE_GST_CONTROLLER
        g_object_get(G_OBJECT(machine),property->name,&value,NULL);
#endif
        step=(value_max-value_min)/1024.0;
        widget=gtk_hscale_new_with_range(value_min,value_max,step);
        gtk_scale_set_draw_value(GTK_SCALE(widget),TRUE);
        gtk_scale_set_value_pos(GTK_SCALE(widget),GTK_POS_RIGHT);
        gtk_range_set_value(GTK_RANGE(widget),value);
        gtk_widget_set_name(GTK_WIDGET(widget),property->name);
        // @todo add numerical entry as well ?
#ifdef USE_GST_DPARAMS
        g_signal_connect(G_OBJECT(dparam), "notify::value-double", (GCallback)on_double_range_property_notify, (gpointer)widget);
        g_signal_connect(G_OBJECT(widget), "value-changed", (GCallback)on_double_range_property_changed, (gpointer)dparam);
#endif
#ifdef USE_GST_CONTROLLER
        signal_name=g_strdup_printf("notify::%s",property->name);
        g_signal_connect(G_OBJECT(machine), signal_name, (GCallback)on_double_range_property_notify, (gpointer)widget);
        g_signal_connect(G_OBJECT(widget), "value-changed", (GCallback)on_double_range_property_changed, (gpointer)machine);
				//g_signal_connect(G_OBJECT(widget), "format-value", (GCallback)on_double_range_property_format_value, (gpointer)machine);        g_free(signal_name);
#endif
      }
      else {
        gchar *str=g_strdup_printf("unhandled type \"%s\"",G_PARAM_SPEC_TYPE_NAME(property));
        widget=gtk_label_new(str);g_free(str);
      }
			if(range_min) { g_free(range_min);range_min=NULL; }
			if(range_max) { g_free(range_max);range_max=NULL; }
			
      gtk_tooltips_set_tip(GTK_TOOLTIPS(tips),widget,g_param_spec_get_blurb(property),NULL);
      gtk_table_attach(GTK_TABLE(table),widget, 1, 2, i, i+1, GTK_FILL|GTK_EXPAND,GTK_SHRINK, 2,1);
    }
    gtk_table_attach(GTK_TABLE(table),gtk_label_new(" "), 0, 2, i, i+1, GTK_FILL|GTK_EXPAND,GTK_SHRINK, 2,1);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window),table);
    gtk_box_pack_start(GTK_BOX(box),scrolled_window,TRUE,TRUE,0);
  }
  else {
    gtk_container_add(GTK_CONTAINER(box),gtk_label_new(_("machine has no params")));
  }
  gtk_container_add(GTK_CONTAINER(self),box);
  
  g_object_try_unref(machine);
  g_object_try_unref(main_window);
  return(TRUE);
}

//-- constructor methods

/**
 * bt_machine_properties_dialog_new:
 * @app: the application the dialog belongs to
 * @machine: the machine to create the dialog for
 *
 * Create a new instance
 *
 * Returns: the new instance or NULL in case of an error
 */
BtMachinePropertiesDialog *bt_machine_properties_dialog_new(const BtEditApplication *app,const BtMachine *machine) {
  BtMachinePropertiesDialog *self;

  if(!(self=BT_MACHINE_PROPERTIES_DIALOG(g_object_new(BT_TYPE_MACHINE_PROPERTIES_DIALOG,"app",app,"machine",machine,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_machine_properties_dialog_init_ui(self)) {
    goto Error;
  }
  gtk_widget_show_all(GTK_WIDGET(self));
  return(self);
Error:
  g_object_try_unref(self);
  return(NULL);
}

//-- methods

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_machine_properties_dialog_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtMachinePropertiesDialog *self = BT_MACHINE_PROPERTIES_DIALOG(object);
  return_if_disposed();
  switch (property_id) {
    case MACHINE_PROPERTIES_DIALOG_APP: {
      g_value_set_object(value, self->priv->app);
    } break;
    case MACHINE_PROPERTIES_DIALOG_MACHINE: {
      g_value_set_object(value, self->priv->machine);
    } break;
    default: {
       G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_machine_properties_dialog_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtMachinePropertiesDialog *self = BT_MACHINE_PROPERTIES_DIALOG(object);
  return_if_disposed();
  switch (property_id) {
    case MACHINE_PROPERTIES_DIALOG_APP: {
      g_object_try_unref(self->priv->app);
      self->priv->app = g_object_try_ref(g_value_get_object(value));
      //GST_DEBUG("set the app for settings_dialog: %p",self->priv->app);
    } break;
    case MACHINE_PROPERTIES_DIALOG_MACHINE: {
      g_object_try_unref(self->priv->machine);
      self->priv->machine = g_object_try_ref(g_value_get_object(value));
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_machine_properties_dialog_dispose(GObject *object) {
  BtMachinePropertiesDialog *self = BT_MACHINE_PROPERTIES_DIALOG(object);
#ifdef USE_GST_DPARAMS
  gulong i,global_params;
  GstDParam *dparam;
#endif
#ifdef USE_GST_CONTROLLER
  GstElement *machine;
#endif
  
  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  
  // disconnect all handlers that are connected to dparams
#ifdef USE_GST_DPARAMS
  g_object_get(self->priv->machine,"global-params",&global_params,NULL);
  for(i=0;i<global_params;i++) {
    dparam=bt_machine_get_global_dparam(self->priv->machine,i);
    g_signal_handlers_disconnect_matched(dparam,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_double_range_property_notify,NULL);
    g_signal_handlers_disconnect_matched(dparam,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_int_range_property_notify,NULL);
  }
#endif
#ifdef USE_GST_CONTROLLER
  g_object_get(self->priv->machine,"machine",&machine,NULL);
  g_signal_handlers_disconnect_matched(machine,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_double_range_property_notify,NULL);
  g_signal_handlers_disconnect_matched(machine,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_int_range_property_notify,NULL);
  g_object_unref(machine);
#endif
  
  g_object_try_unref(self->priv->app);
  g_object_try_unref(self->priv->machine);

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_machine_properties_dialog_finalize(GObject *object) {
  BtMachinePropertiesDialog *self = BT_MACHINE_PROPERTIES_DIALOG(object);

  GST_DEBUG("!!!! self=%p",self);
  g_free(self->priv);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_machine_properties_dialog_init(GTypeInstance *instance, gpointer g_class) {
  BtMachinePropertiesDialog *self = BT_MACHINE_PROPERTIES_DIALOG(instance);
  self->priv = g_new0(BtMachinePropertiesDialogPrivate,1);
  self->priv->dispose_has_run = FALSE;
}

static void bt_machine_properties_dialog_class_init(BtMachinePropertiesDialogClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  //GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS(klass);

  parent_class=g_type_class_ref(GTK_TYPE_DIALOG);
  
  gobject_class->set_property = bt_machine_properties_dialog_set_property;
  gobject_class->get_property = bt_machine_properties_dialog_get_property;
  gobject_class->dispose      = bt_machine_properties_dialog_dispose;
  gobject_class->finalize     = bt_machine_properties_dialog_finalize;

  g_object_class_install_property(gobject_class,MACHINE_PROPERTIES_DIALOG_APP,
                                  g_param_spec_object("app",
                                     "app construct prop",
                                     "Set application object, the dialog belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,MACHINE_PROPERTIES_DIALOG_MACHINE,
                                  g_param_spec_object("machine",
                                     "machine construct prop",
                                     "Set machine object, the dialog handles",
                                     BT_TYPE_MACHINE, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

}

GType bt_machine_properties_dialog_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtMachinePropertiesDialogClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_machine_properties_dialog_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtMachinePropertiesDialog),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_machine_properties_dialog_init, // instance_init
    };
    type = g_type_register_static(GTK_TYPE_WINDOW,"BtMachinePropertiesDialog",&info,0);
  }
  return type;
}
