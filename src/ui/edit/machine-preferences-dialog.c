/* $Id: machine-preferences-dialog.c,v 1.6 2005-01-20 16:18:53 ensonic Exp $
 * class for the machine preferences dialog
 */

#define BT_EDIT
#define BT_MACHINE_PREFERENCES_DIALOG_C

#include "bt-edit.h"

//-- property ids

enum {
  MACHINE_PREFERENCES_DIALOG_APP=1,
  MACHINE_PREFERENCES_DIALOG_MACHINE
};

struct _BtMachinePreferencesDialogPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* the application */
  BtEditApplication *app;

  /* the underlying machine */
  BtMachine *machine;
};

static GtkDialogClass *parent_class=NULL;

//-- event handler

static void on_range_property_notify(const GstElement *machine,GParamSpec *property,gpointer user_data) {
	GtkWidget *widget=GTK_WIDGET(user_data);
	gdouble value;
	
	g_assert(user_data);

	GST_INFO("preferences value notify received for: '%s'",property->name);
	
	g_object_get(G_OBJECT(machine),property->name,&value,NULL);
	gtk_range_set_value(GTK_RANGE(widget),value);
}

static void on_range_property_changed(GtkRange *range,gpointer user_data) {
	GstElement *machine=GST_ELEMENT(user_data);
	const gchar *name=gtk_widget_get_name(GTK_WIDGET(range));
	
	g_assert(user_data);

	//GST_INFO("preferences value change received for: '%s'",name);
	g_object_set(machine,name,gtk_range_get_value(range),NULL);
}

//-- helper methods

static gboolean bt_machine_preferences_dialog_init_ui(const BtMachinePreferencesDialog *self) {
	BtMainWindow *main_window;
  GtkWidget *label,*widget,*table,*scrolled_window;
	GtkTooltips *tips=gtk_tooltips_new();
	gchar *id;
	GdkPixbuf *window_icon=NULL;
	GstElement *machine;
	GParamSpec **properties,*property;
	guint i,number_of_properties;
	GType param_type;

	g_object_get(self->priv->app,"main-window",&main_window,NULL);
	gtk_window_set_transient_for(GTK_WINDOW(self),GTK_WINDOW(main_window));
	
  // create and set window icon
	if(BT_IS_SOURCE_MACHINE(self->priv->machine)) {
		window_icon=gdk_pixbuf_new_from_filename("menu_source_machine.png");
	}
	else if(BT_IS_PROCESSOR_MACHINE(self->priv->machine)) {
		window_icon=gdk_pixbuf_new_from_filename("menu_processor_machine.png");
  }
  else if(BT_IS_SINK_MACHINE(self->priv->machine)) {
		window_icon=gdk_pixbuf_new_from_filename("menu_sink_machine.png");
  }
  if(window_icon) {
    gtk_window_set_icon(GTK_WINDOW(self),window_icon);
  }
	
	// leave the choice of with to gtk
	gtk_window_set_default_size(GTK_WINDOW(self),-1,200);
	// set a title
	g_object_get(self->priv->machine,"id",&id,"machine",&machine,NULL);
  gtk_window_set_title(GTK_WINDOW(self),g_strdup_printf(_("%s preferences"),id));
	g_free(id);
  
	// get machine properties
	if((properties=g_object_class_list_properties(G_OBJECT_CLASS(GST_ELEMENT_GET_CLASS(machine)),&number_of_properties))) {
		gchar *signal_name;
		
		GST_INFO("machine has %d properties",number_of_properties);
		// machine preferences inside a scrolled window
		scrolled_window=gtk_scrolled_window_new(NULL,NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_NEVER,GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),GTK_SHADOW_NONE);
		// add machine preferences into the table
		table=gtk_table_new(/*rows=*/number_of_properties+1,/*columns=*/2,/*homogenous=*/FALSE);
		gtk_container_set_border_width(GTK_CONTAINER(table),6);
		for(i=0;i<number_of_properties;i++) {
			property=properties[i];
			GST_INFO("property %p has name '%s'",property,property->name);
			// get name
			label=gtk_label_new(property->name);
			gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
			gtk_table_attach(GTK_TABLE(table),label, 0, 1, i, i+1, GTK_SHRINK,GTK_SHRINK, 2,1);
			// @todo choose proper widgets
			param_type=G_PARAM_SPEC_TYPE(property);
			if(param_type==G_TYPE_PARAM_STRING) {
				//GParamSpecString *string_property=G_PARAM_SPEC_STRING(property);
				gchar *value;
				
				g_object_get(machine,property->name,&value,NULL);
				widget=gtk_entry_new();
				gtk_entry_set_text(GTK_ENTRY(widget),safe_string(value));g_free(value);
			}
			else if(param_type==G_TYPE_PARAM_BOOLEAN) {
				gboolean value;
				
				g_object_get(machine,property->name,&value,NULL);
				widget=gtk_check_button_new();
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),value);
			}
			else if(param_type==G_TYPE_PARAM_INT) {
				// @todo add (slider + entry) or spin button
				widget=gtk_entry_new();
				gtk_entry_set_text(GTK_ENTRY(widget),"int");
			}
			else if(param_type==G_TYPE_PARAM_DOUBLE) {
				GParamSpecDouble *double_property=G_PARAM_SPEC_DOUBLE(property);
				gdouble step,value;

				g_object_get(machine,property->name,&value,NULL);
				step=(double_property->maximum-double_property->minimum)/1024.0;
				widget=gtk_hscale_new_with_range(double_property->minimum,double_property->maximum,step);
				gtk_widget_set_name(GTK_WIDGET(widget),property->name);
				gtk_scale_set_draw_value(GTK_SCALE(widget),TRUE);
				gtk_range_set_value(GTK_RANGE(widget),value);
				// @todo add numerical entry as well ?
				signal_name=g_strdup_printf("notify::%s",property->name);
				g_signal_connect(G_OBJECT(machine), signal_name, (GCallback)on_range_property_notify, (gpointer)widget);
				g_signal_connect(G_OBJECT(widget), "value-changed", (GCallback)on_range_property_changed, (gpointer)machine);
				g_free(signal_name);
			}
			else {
				gchar *str=g_strdup_printf("unhandled type \"%s\"",G_PARAM_SPEC_TYPE_NAME(property));
				widget=gtk_label_new(str);g_free(str);
			}
			gtk_tooltips_set_tip(GTK_TOOLTIPS(tips),widget,g_param_spec_get_blurb(property),NULL);
			gtk_table_attach(GTK_TABLE(table),widget, 1, 2, i, i+1, GTK_FILL|GTK_EXPAND,GTK_SHRINK, 2,1);
		}
		// eat remaning space
		gtk_table_attach(GTK_TABLE(table),gtk_label_new(" "), 0, 2, i, i+1, GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND, 2,1);
		g_free(properties);
		gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window),table);
		gtk_container_add(GTK_CONTAINER(self),scrolled_window);
	}
	else {
		gtk_container_add(GTK_CONTAINER(self),gtk_label_new(_("machine has no preferences")));
	}
	
	g_object_try_unref(machine);
	g_object_try_unref(main_window);
  return(TRUE);
}

//-- constructor methods

/**
 * bt_machine_preferences_dialog_new:
 * @app: the application the dialog belongs to
 * @machine: the machine to create the dialog for
 *
 * Create a new instance
 *
 * Returns: the new instance or NULL in case of an error
 */
BtMachinePreferencesDialog *bt_machine_preferences_dialog_new(const BtEditApplication *app,const BtMachine *machine) {
  BtMachinePreferencesDialog *self;

  if(!(self=BT_MACHINE_PREFERENCES_DIALOG(g_object_new(BT_TYPE_MACHINE_PREFERENCES_DIALOG,"app",app,"machine",machine,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_machine_preferences_dialog_init_ui(self)) {
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
static void bt_machine_preferences_dialog_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtMachinePreferencesDialog *self = BT_MACHINE_PREFERENCES_DIALOG(object);
  return_if_disposed();
  switch (property_id) {
    case MACHINE_PREFERENCES_DIALOG_APP: {
      g_value_set_object(value, self->priv->app);
    } break;
    case MACHINE_PREFERENCES_DIALOG_MACHINE: {
      g_value_set_object(value, self->priv->machine);
    } break;
    default: {
 			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given preferences for this object */
static void bt_machine_preferences_dialog_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtMachinePreferencesDialog *self = BT_MACHINE_PREFERENCES_DIALOG(object);
  return_if_disposed();
  switch (property_id) {
    case MACHINE_PREFERENCES_DIALOG_APP: {
      g_object_try_unref(self->priv->app);
      self->priv->app = g_object_try_ref(g_value_get_object(value));
      //GST_DEBUG("set the app for settings_dialog: %p",self->priv->app);
    } break;
    case MACHINE_PREFERENCES_DIALOG_MACHINE: {
      g_object_try_unref(self->priv->machine);
      self->priv->machine = g_object_try_ref(g_value_get_object(value));
    } break;
    default: {
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_machine_preferences_dialog_dispose(GObject *object) {
  BtMachinePreferencesDialog *self = BT_MACHINE_PREFERENCES_DIALOG(object);
	return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  g_object_try_unref(self->priv->app);
  g_object_try_unref(self->priv->machine);

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_machine_preferences_dialog_finalize(GObject *object) {
  BtMachinePreferencesDialog *self = BT_MACHINE_PREFERENCES_DIALOG(object);

  GST_DEBUG("!!!! self=%p",self);
  g_free(self->priv);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_machine_preferences_dialog_init(GTypeInstance *instance, gpointer g_class) {
  BtMachinePreferencesDialog *self = BT_MACHINE_PREFERENCES_DIALOG(instance);
  self->priv = g_new0(BtMachinePreferencesDialogPrivate,1);
  self->priv->dispose_has_run = FALSE;
}

static void bt_machine_preferences_dialog_class_init(BtMachinePreferencesDialogClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS(klass);

  parent_class=g_type_class_ref(GTK_TYPE_DIALOG);
  
  gobject_class->set_property = bt_machine_preferences_dialog_set_property;
  gobject_class->get_property = bt_machine_preferences_dialog_get_property;
  gobject_class->dispose      = bt_machine_preferences_dialog_dispose;
  gobject_class->finalize     = bt_machine_preferences_dialog_finalize;

  g_object_class_install_property(gobject_class,MACHINE_PREFERENCES_DIALOG_APP,
                                  g_param_spec_object("app",
                                     "app construct prop",
                                     "Set application object, the dialog belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,MACHINE_PREFERENCES_DIALOG_MACHINE,
                                  g_param_spec_object("machine",
                                     "machine construct prop",
                                     "Set machine object, the dialog handles",
                                     BT_TYPE_MACHINE, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

}

GType bt_machine_preferences_dialog_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtMachinePreferencesDialogClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_machine_preferences_dialog_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtMachinePreferencesDialog),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_machine_preferences_dialog_init, // instance_init
    };
		type = g_type_register_static(GTK_TYPE_WINDOW,"BtMachinePreferencesDialog",&info,0);
  }
  return type;
}
