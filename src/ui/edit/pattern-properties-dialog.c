/* $Id: pattern-properties-dialog.c,v 1.2 2005-04-23 10:33:09 ensonic Exp $
 * class for the pattern properties dialog
 */

#define BT_EDIT
#define BT_PATTERN_PROPERTIES_DIALOG_C

#include "bt-edit.h"

//-- property ids

enum {
  PATTERN_PROPERTIES_DIALOG_APP=1,
  PATTERN_PROPERTIES_DIALOG_PATTERN
};

struct _BtPatternPropertiesDialogPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* the application */
  BtEditApplication *app;

  /* the underlying pattern */
  BtPattern *pattern;
	
	/* widgets and their handlers */
	//GtkWidget *widgets;
	//gulong *notify_id,*change_id;
};

static GtkDialogClass *parent_class=NULL;

//-- event handler

//-- helper methods

static gboolean bt_pattern_properties_dialog_init_ui(const BtPatternPropertiesDialog *self) {
  GtkWidget *box,*label,*widget,*table,*scrolled_window;
	gchar *name,*title;
	//GdkPixbuf *window_icon=NULL;

  // create and set window icon
	/* e.v. tab_pattern.png
  if((window_icon=bt_ui_ressources_get_pixbuf_by_machine(self->priv->machine))) {
    gtk_window_set_icon(GTK_WINDOW(self),window_icon);
  }
	*/
  
	// set a title
	g_object_get(self->priv->pattern,"name",&name,NULL);
	title=g_strdup_printf(_("%s properties"),name);
  gtk_window_set_title(GTK_WINDOW(self),title);
	g_free(name);g_free(title);
   
	  // add dialog commision widgets (okay, cancel)
  gtk_dialog_add_buttons(GTK_DIALOG(self),
                          GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                          //GTK_STOCK_CANCEL,GTK_RESPONSE_REJECT,
                          NULL);

  // add widgets to the dialog content area
  box=GTK_DIALOG(self)->vbox;	//gtk_vbox_new(FALSE,12);
	gtk_box_set_spacing(GTK_BOX(box),12);	
  gtk_container_set_border_width(GTK_CONTAINER(box),6);
	
	table=gtk_table_new(/*rows=*/3,/*columns=*/2,/*homogenous=*/FALSE);
	gtk_container_add(GTK_CONTAINER(box),table);
	
	// GtkEntry : pattern name
  label=gtk_label_new(_("name"));
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(table),label, 0, 1, 0, 1, GTK_SHRINK,GTK_SHRINK, 2,1);
  widget=gtk_entry_new();
  gtk_table_attach(GTK_TABLE(table),widget, 1, 2, 0, 1, GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND, 2,1);
	//g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(on_name_changed), (gpointer)self);

	// GtkComboBox : pattern length
  label=gtk_label_new(_("length"));
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(table),label, 0, 1, 1, 2, GTK_SHRINK,GTK_SHRINK, 2,1);

	// GtkSpinButton : number of voices
  label=gtk_label_new(_("voices"));
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(table),label, 0, 1, 2, 3, GTK_SHRINK,GTK_SHRINK, 2,1);
	
  return(TRUE);
}

//-- constructor methods

/**
 * bt_pattern_properties_dialog_new:
 * @app: the application the dialog belongs to
 * @machine: the machine to create the dialog for
 *
 * Create a new instance
 *
 * Returns: the new instance or NULL in case of an error
 */
BtPatternPropertiesDialog *bt_pattern_properties_dialog_new(const BtEditApplication *app,const BtPattern *pattern) {
  BtPatternPropertiesDialog *self;

  if(!(self=BT_PATTERN_PROPERTIES_DIALOG(g_object_new(BT_TYPE_PATTERN_PROPERTIES_DIALOG,"app",app,"pattern",pattern,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_pattern_properties_dialog_init_ui(self)) {
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
static void bt_pattern_properties_dialog_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtPatternPropertiesDialog *self = BT_PATTERN_PROPERTIES_DIALOG(object);
  return_if_disposed();
  switch (property_id) {
    case PATTERN_PROPERTIES_DIALOG_APP: {
      g_value_set_object(value, self->priv->app);
    } break;
    case PATTERN_PROPERTIES_DIALOG_PATTERN: {
      g_value_set_object(value, self->priv->pattern);
    } break;
    default: {
 			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_pattern_properties_dialog_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtPatternPropertiesDialog *self = BT_PATTERN_PROPERTIES_DIALOG(object);
  return_if_disposed();
  switch (property_id) {
    case PATTERN_PROPERTIES_DIALOG_APP: {
      g_object_try_unref(self->priv->app);
      self->priv->app = g_object_try_ref(g_value_get_object(value));
      //GST_DEBUG("set the app for settings_dialog: %p",self->priv->app);
    } break;
    case PATTERN_PROPERTIES_DIALOG_PATTERN: {
      g_object_try_unref(self->priv->pattern);
      self->priv->pattern = g_object_try_ref(g_value_get_object(value));
    } break;
    default: {
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_pattern_properties_dialog_dispose(GObject *object) {
  BtPatternPropertiesDialog *self = BT_PATTERN_PROPERTIES_DIALOG(object);
	gulong i,global_params;
	GstDParam *dparam;
	
	return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);

  g_object_try_unref(self->priv->app);
  g_object_try_unref(self->priv->pattern);

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_pattern_properties_dialog_finalize(GObject *object) {
  BtPatternPropertiesDialog *self = BT_PATTERN_PROPERTIES_DIALOG(object);

  GST_DEBUG("!!!! self=%p",self);
  g_free(self->priv);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_pattern_properties_dialog_init(GTypeInstance *instance, gpointer g_class) {
  BtPatternPropertiesDialog *self = BT_PATTERN_PROPERTIES_DIALOG(instance);
  self->priv = g_new0(BtPatternPropertiesDialogPrivate,1);
  self->priv->dispose_has_run = FALSE;
}

static void bt_pattern_properties_dialog_class_init(BtPatternPropertiesDialogClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS(klass);

  parent_class=g_type_class_ref(GTK_TYPE_DIALOG);
  
  gobject_class->set_property = bt_pattern_properties_dialog_set_property;
  gobject_class->get_property = bt_pattern_properties_dialog_get_property;
  gobject_class->dispose      = bt_pattern_properties_dialog_dispose;
  gobject_class->finalize     = bt_pattern_properties_dialog_finalize;

  g_object_class_install_property(gobject_class,PATTERN_PROPERTIES_DIALOG_APP,
                                  g_param_spec_object("app",
                                     "app construct prop",
                                     "Set application object, the dialog belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,PATTERN_PROPERTIES_DIALOG_PATTERN,
                                  g_param_spec_object("pattern",
                                     "pattern construct prop",
                                     "Set pattern object, the dialog handles",
                                     BT_TYPE_PATTERN, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

}

GType bt_pattern_properties_dialog_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtPatternPropertiesDialogClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_pattern_properties_dialog_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtPatternPropertiesDialog),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_pattern_properties_dialog_init, // instance_init
    };
		type = g_type_register_static(GTK_TYPE_DIALOG,"BtPatternPropertiesDialog",&info,0);
  }
  return type;
}
