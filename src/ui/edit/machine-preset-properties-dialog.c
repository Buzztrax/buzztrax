/* $Id: machine-preset-properties-dialog.c,v 1.3 2007-01-22 21:00:58 ensonic Exp $
 *
 * Buzztard
 * Copyright (C) 2007 Buzztard team <buzztard-devel@lists.sf.net>
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
 * SECTION:btmachinepresetpropertiesdialog
 * @short_description: machine preset settings
 */ 

#define BT_EDIT
#define BT_MACHINE_PRESET_PROPERTIES_DIALOG_C

#include "bt-edit.h"

//-- property ids

enum {
  MACHINE_PRESET_PROPERTIES_DIALOG_APP=1,
  MACHINE_PRESET_PROPERTIES_DIALOG_MACHINE,
  MACHINE_PRESET_PROPERTIES_DIALOG_NAME,
  MACHINE_PRESET_PROPERTIES_DIALOG_COMMENT
};

struct _BtMachinePresetPropertiesDialogPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* the application */
  BtEditApplication *app;
  
  /* the element that has the presets */
  GstElement *machine;
  GList *presets;

  /* dialog data */
  gchar *name,*comment;
  gchar **name_ptr,**comment_ptr;
  
  /* widgets and their handlers */
  GtkWidget *okay_button;
};

static GtkDialogClass *parent_class=NULL;

//-- event handler

static void on_name_changed(GtkEditable *editable,gpointer user_data) {
  BtMachinePresetPropertiesDialog *self=BT_MACHINE_PRESET_PROPERTIES_DIALOG(user_data);
  const gchar *name=gtk_entry_get_text(GTK_ENTRY(editable));
  gboolean valid=TRUE;

  GST_INFO("preset text changed: '%s'",name);

  g_assert(user_data);
  // assure validity & uniquness of the entered data
  if(!(*name)) 
    // empty box
    valid=FALSE;
  else if(*self->priv->name_ptr && strcmp(name,*self->priv->name_ptr) && g_list_find_custom(self->priv->presets,name,(GCompareFunc)strcmp))
    // non empty old name && name has changed && name already exists
    valid=FALSE;
  else if(!(*self->priv->name_ptr) && g_list_find_custom(self->priv->presets,name,(GCompareFunc)strcmp))
    // name already exists
    valid=FALSE;
  
  gtk_widget_set_sensitive(self->priv->okay_button,valid);
  g_free(self->priv->name);
  self->priv->name=g_strdup(name);
}

static void on_comment_changed(GtkEditable *editable,gpointer user_data) {
  BtMachinePresetPropertiesDialog *self=BT_MACHINE_PRESET_PROPERTIES_DIALOG(user_data);
  const gchar *comment=gtk_entry_get_text(GTK_ENTRY(editable));

  GST_INFO("preset comment changed");

  g_assert(user_data);
  g_free(self->priv->comment);
  self->priv->comment=g_strdup(comment);
}

//-- helper methods

static gboolean bt_machine_preset_properties_dialog_init_ui(const BtMachinePresetPropertiesDialog *self) {
  GtkWidget *label,*widget,*table;
  //GdkPixbuf *window_icon=NULL;
  GList *buttons;

  gtk_widget_set_name(GTK_WIDGET(self),_("preset name and comment"));
  
  // create and set window icon
  /*
  if((window_icon=bt_ui_ressources_get_pixbuf_by_machine(self->priv->machine))) {
    gtk_window_set_icon(GTK_WINDOW(self),window_icon);
  }
  */
  
  // set a title
  gtk_window_set_title(GTK_WINDOW(self),_("preset name and comment"));
   
    // add dialog commision widgets (okay, cancel)
  gtk_dialog_add_buttons(GTK_DIALOG(self),
                          GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                          GTK_STOCK_CANCEL,GTK_RESPONSE_REJECT,
                          NULL);
  
  gtk_dialog_set_default_response(GTK_DIALOG(self),GTK_RESPONSE_ACCEPT);

  // grab okay button, so that we can block if input is not valid
  buttons=gtk_container_get_children(GTK_CONTAINER(GTK_DIALOG(self)->action_area));
  GST_INFO("dialog buttons: %d",g_list_length(buttons));
  self->priv->okay_button=GTK_WIDGET(g_list_nth_data(buttons,1));
  g_list_free(buttons);

  table=gtk_table_new(/*rows=*/2,/*columns=*/2,/*homogenous=*/FALSE);

  // GtkEntry : preset name
  label=gtk_label_new(_("name"));
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(table),label, 0, 1, 0, 1, GTK_SHRINK,GTK_SHRINK, 2,1);
  widget=gtk_entry_new();
  if(self->priv->name)
    gtk_entry_set_text(GTK_ENTRY(widget),self->priv->name);
  else
    gtk_widget_set_sensitive(self->priv->okay_button,FALSE);
  gtk_entry_set_activates_default(GTK_ENTRY(widget),TRUE);
  gtk_table_attach(GTK_TABLE(table),widget, 1, 2, 0, 1, GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND, 2,1);
  g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(on_name_changed), (gpointer)self);
  
  // GtkEntry : preset comment
  label=gtk_label_new(_("comment"));
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(table),label, 0, 1, 1, 2, GTK_SHRINK,GTK_SHRINK, 2,1);
  widget=gtk_entry_new();
  if(self->priv->comment) gtk_entry_set_text(GTK_ENTRY(widget),self->priv->comment);
  gtk_entry_set_activates_default(GTK_ENTRY(widget),TRUE);
  gtk_table_attach(GTK_TABLE(table),widget, 1, 2, 1, 2, GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND, 2,1);
  g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(on_comment_changed), (gpointer)self);
  
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(self)->vbox),table);

  return(TRUE);
}

//-- constructor methods

/**
 * bt_machine_preset_properties_dialog_new:
 * @app: the application the dialog belongs to
 * @pattern: the pattern for which to create the dialog for
 *
 * Create a new instance
 *
 * Returns: the new instance or NULL in case of an error
 */
BtMachinePresetPropertiesDialog *bt_machine_preset_properties_dialog_new(const BtEditApplication *app,GstElement *machine,gchar **name,gchar **comment) {
  BtMachinePresetPropertiesDialog *self;

  if(!(self=BT_MACHINE_PRESET_PROPERTIES_DIALOG(g_object_new(BT_TYPE_MACHINE_PRESET_PROPERTIES_DIALOG,"app",app,"machine",machine,"name",name,"comment",comment,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_machine_preset_properties_dialog_init_ui(self)) {
    goto Error;
  }
  gtk_widget_show_all(GTK_WIDGET(self));
  return(self);
Error:
  g_object_try_unref(self);
  return(NULL);
}

//-- methods

/**
 * bt_machine_preset_properties_dialog_apply:
 * @self: the dialog which settings to apply
 *
 * Makes the dialog settings effective.
 */
void bt_machine_preset_properties_dialog_apply(const BtMachinePresetPropertiesDialog *self) {
  GST_INFO("applying prest changes settings");
  
  *self->priv->name_ptr=self->priv->name;
  self->priv->name=NULL;
  *self->priv->comment_ptr=self->priv->comment;
  self->priv->comment=NULL;
}

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_machine_preset_properties_dialog_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtMachinePresetPropertiesDialog *self = BT_MACHINE_PRESET_PROPERTIES_DIALOG(object);
  return_if_disposed();
  switch (property_id) {
    case MACHINE_PRESET_PROPERTIES_DIALOG_APP: {
      g_value_set_object(value, self->priv->app);
    } break;
    case MACHINE_PRESET_PROPERTIES_DIALOG_MACHINE: {
      g_value_set_object(value, self->priv->machine);
    } break;
    case MACHINE_PRESET_PROPERTIES_DIALOG_NAME: {
      g_value_set_pointer(value, self->priv->name_ptr);
    } break;
    case MACHINE_PRESET_PROPERTIES_DIALOG_COMMENT: {
      g_value_set_pointer(value, self->priv->comment_ptr);
    } break;
    default: {
       G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_machine_preset_properties_dialog_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtMachinePresetPropertiesDialog *self = BT_MACHINE_PRESET_PROPERTIES_DIALOG(object);
  return_if_disposed();
  switch (property_id) {
    case MACHINE_PRESET_PROPERTIES_DIALOG_APP: {
      g_object_try_unref(self->priv->app);
      self->priv->app = g_object_try_ref(g_value_get_object(value));
      //GST_DEBUG("set the app for preset_dialog: %p",self->priv->app);
    } break;
    case MACHINE_PRESET_PROPERTIES_DIALOG_MACHINE: {
      g_object_try_unref(self->priv->machine);
      self->priv->machine = g_object_try_ref(g_value_get_object(value));
      GST_DEBUG("set the machine for preset_dialog: %p",self->priv->machine);
      self->priv->presets=self->priv->machine?gst_preset_get_preset_names(GST_PRESET(self->priv->machine)):NULL;
    } break;
    case MACHINE_PRESET_PROPERTIES_DIALOG_NAME: {
      self->priv->name_ptr = g_value_get_pointer(value);
      if(self->priv->name_ptr) {
        self->priv->name=g_strdup(*self->priv->name_ptr);
      }
    } break;
    case MACHINE_PRESET_PROPERTIES_DIALOG_COMMENT: {
      self->priv->comment_ptr = g_value_get_pointer(value);
      if(self->priv->comment_ptr) {
        self->priv->comment=g_strdup(*self->priv->comment_ptr);
      }
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_machine_preset_properties_dialog_dispose(GObject *object) {
  BtMachinePresetPropertiesDialog *self = BT_MACHINE_PRESET_PROPERTIES_DIALOG(object);
  
  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);

  g_object_try_unref(self->priv->app);
  g_object_try_unref(self->priv->machine);

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_machine_preset_properties_dialog_finalize(GObject *object) {
  BtMachinePresetPropertiesDialog *self = BT_MACHINE_PRESET_PROPERTIES_DIALOG(object);

  GST_DEBUG("!!!! self=%p",self);
  
  g_free(self->priv->name);
  g_free(self->priv->comment);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_machine_preset_properties_dialog_init(GTypeInstance *instance, gpointer g_class) {
  BtMachinePresetPropertiesDialog *self = BT_MACHINE_PRESET_PROPERTIES_DIALOG(instance);
  
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_MACHINE_PRESET_PROPERTIES_DIALOG, BtMachinePresetPropertiesDialogPrivate);
}

static void bt_machine_preset_properties_dialog_class_init(BtMachinePresetPropertiesDialogClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtMachinePresetPropertiesDialogPrivate));
  
  gobject_class->set_property = bt_machine_preset_properties_dialog_set_property;
  gobject_class->get_property = bt_machine_preset_properties_dialog_get_property;
  gobject_class->dispose      = bt_machine_preset_properties_dialog_dispose;
  gobject_class->finalize     = bt_machine_preset_properties_dialog_finalize;

  g_object_class_install_property(gobject_class,MACHINE_PRESET_PROPERTIES_DIALOG_APP,
                                  g_param_spec_object("app",
                                     "app construct prop",
                                     "Set application object, the dialog belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,MACHINE_PRESET_PROPERTIES_DIALOG_MACHINE,
                                  g_param_spec_object("machine",
                                     "machine construct prop",
                                     "Set machine object, the dialog handles",
                                     GST_TYPE_ELEMENT, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,MACHINE_PRESET_PROPERTIES_DIALOG_NAME,
                                  g_param_spec_pointer("name",
                                     "name-pointer prop",
                                     "address of preset name",
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,MACHINE_PRESET_PROPERTIES_DIALOG_COMMENT,
                                  g_param_spec_pointer("comment",
                                     "comment-pointer prop",
                                     "address of preset comment",
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));
}

GType bt_machine_preset_properties_dialog_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    const GTypeInfo info = {
      sizeof (BtMachinePresetPropertiesDialogClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_machine_preset_properties_dialog_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtMachinePresetPropertiesDialog),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_machine_preset_properties_dialog_init, // instance_init
    };
    type = g_type_register_static(GTK_TYPE_DIALOG,"BtMachinePresetPropertiesDialog",&info,0);
  }
  return type;
}
