/* $Id$
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
 * SECTION:btmachinerenamedialog
 * @short_description: machine settings
 *
 * A dialog to (re)configure a #BtMachine.
 */

#define BT_EDIT
#define BT_MACHINE_RENAME_DIALOG_C

#include "bt-edit.h"

//-- property ids

enum {
  MACHINE_RENAME_DIALOG_APP=1,
  MACHINE_RENAME_DIALOG_MACHINE
};

struct _BtMachineRenameDialogPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* the underlying machine */
  BtMachine *machine;
  
  BtSetup *setup;

  /* dialog data */
  gchar *name;

  /* widgets and their handlers */
  GtkWidget *okay_button;
};

static GtkDialogClass *parent_class=NULL;

//-- event handler

static void on_name_changed(GtkEditable *editable,gpointer user_data) {
  BtMachineRenameDialog *self=BT_MACHINE_RENAME_DIALOG(user_data);
  BtMachine *machine;
  const gchar *name=gtk_entry_get_text(GTK_ENTRY(editable));
  gboolean unique=FALSE;

  GST_DEBUG("change name");
  // assure uniqueness of the entered data
  if(*name) {
    if((machine=bt_setup_get_machine_by_id(self->priv->setup,name))) {
      if(machine==self->priv->machine) {
        unique=TRUE;
      }
      g_object_unref(machine);
    }
    else {
      unique=TRUE;
    }
  }
  GST_INFO("%s""unique '%s'",(unique?"not ":""),name);
  gtk_widget_set_sensitive(self->priv->okay_button,unique);
  // update field
  g_free(self->priv->name);
  self->priv->name=g_strdup(name);
}

//-- helper methods

static void bt_machine_rename_dialog_init_ui(const BtMachineRenameDialog *self) {
  GtkWidget *box,*label,*widget,*table;
  gchar *title;
  //GdkPixbuf *window_icon=NULL;
  GList *buttons;
  BtSong *song;

  gtk_widget_set_name(GTK_WIDGET(self),"rename machine");

  // create and set window icon
  /* e.v. tab_machine.png
  if((window_icon=bt_ui_resources_get_icon_pixbuf_by_machine(self->priv->machine))) {
    gtk_window_set_icon(GTK_WINDOW(self),window_icon);
    g_object_unref(window_icon);
  }
  */

  // get dialog data
  g_object_get(self->priv->machine,"id",&self->priv->name,"song",&song,NULL);
  g_object_get(song,"setup",&self->priv->setup,NULL);
  g_object_unref(song);

  // set a title
  title=g_strdup_printf(_("%s name"),self->priv->name);
  gtk_window_set_title(GTK_WINDOW(self),title);
  g_free(title);

    // add dialog commision widgets (okay, cancel)
  gtk_dialog_add_buttons(GTK_DIALOG(self),
                          GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                          GTK_STOCK_CANCEL,GTK_RESPONSE_REJECT,
                          NULL);

  gtk_dialog_set_default_response(GTK_DIALOG(self),GTK_RESPONSE_ACCEPT);

  // grab okay button, so that we can block if input is not valid
  buttons=gtk_container_get_children(GTK_CONTAINER(gtk_dialog_get_action_area(GTK_DIALOG(self))));
  GST_INFO("dialog buttons: %d",g_list_length(buttons));
  self->priv->okay_button=GTK_WIDGET(g_list_nth_data(buttons,1));
  g_list_free(buttons);

  // add widgets to the dialog content area
  box=gtk_vbox_new(FALSE,12);
  gtk_container_set_border_width(GTK_CONTAINER(box),6);
  gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(self))),box);

  table=gtk_table_new(/*rows=*/1,/*columns=*/2,/*homogenous=*/FALSE);
  gtk_container_add(GTK_CONTAINER(box),table);

  // GtkEntry : machine name
  label=gtk_label_new(_("name"));
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(table),label, 0, 1, 0, 1, GTK_FILL,GTK_SHRINK, 2,1);
  widget=gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(widget),self->priv->name);
  gtk_entry_set_activates_default(GTK_ENTRY(widget),TRUE);
  gtk_table_attach(GTK_TABLE(table),widget, 1, 2, 0, 1, GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND, 2,1);
  g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(on_name_changed), (gpointer)self);
}

//-- constructor methods

/**
 * bt_machine_rename_dialog_new:
 * @app: the application the dialog belongs to
 * @machine: the machine for which to create the dialog for
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtMachineRenameDialog *bt_machine_rename_dialog_new(const BtEditApplication *app,const BtMachine *machine) {
  BtMachineRenameDialog *self;

  self=BT_MACHINE_RENAME_DIALOG(g_object_new(BT_TYPE_MACHINE_RENAME_DIALOG,"app",app,"machine",machine,NULL));
  bt_machine_rename_dialog_init_ui(self);
  return(self);
}

//-- methods

/**
 * bt_machine_rename_dialog_apply:
 * @self: the dialog which settings to apply
 *
 * Makes the dialog settings effective.
 */
void bt_machine_rename_dialog_apply(const BtMachineRenameDialog *self) {
  GST_INFO("applying machine settings");

  g_object_set(G_OBJECT(self->priv->machine),"id",self->priv->name,NULL);
}

//-- wrapper

//-- class internals

static void bt_machine_rename_dialog_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec) {
  BtMachineRenameDialog *self = BT_MACHINE_RENAME_DIALOG(object);
  return_if_disposed();
  switch (property_id) {
    case MACHINE_RENAME_DIALOG_APP: {
      g_object_try_unref(self->priv->app);
      self->priv->app = g_object_try_ref(g_value_get_object(value));
      //GST_DEBUG("set the app for settings_dialog: %p",self->priv->app);
    } break;
    case MACHINE_RENAME_DIALOG_MACHINE: {
      g_object_try_unref(self->priv->machine);
      self->priv->machine = g_object_try_ref(g_value_get_object(value));
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_machine_rename_dialog_dispose(GObject *object) {
  BtMachineRenameDialog *self = BT_MACHINE_RENAME_DIALOG(object);

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);

  g_object_try_unref(self->priv->app);
  g_object_try_unref(self->priv->machine);
  g_object_try_unref(self->priv->setup);

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_machine_rename_dialog_finalize(GObject *object) {
  BtMachineRenameDialog *self = BT_MACHINE_RENAME_DIALOG(object);

  GST_DEBUG("!!!! self=%p",self);

  g_free(self->priv->name);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_machine_rename_dialog_init(GTypeInstance *instance, gpointer g_class) {
  BtMachineRenameDialog *self = BT_MACHINE_RENAME_DIALOG(instance);

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_MACHINE_RENAME_DIALOG, BtMachineRenameDialogPrivate);
}

static void bt_machine_rename_dialog_class_init(BtMachineRenameDialogClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtMachineRenameDialogPrivate));

  gobject_class->set_property = bt_machine_rename_dialog_set_property;
  gobject_class->dispose      = bt_machine_rename_dialog_dispose;
  gobject_class->finalize     = bt_machine_rename_dialog_finalize;

  g_object_class_install_property(gobject_class,MACHINE_RENAME_DIALOG_APP,
                                  g_param_spec_object("app",
                                     "app construct prop",
                                     "Set application object, the dialog belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_WRITABLE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,MACHINE_RENAME_DIALOG_MACHINE,
                                  g_param_spec_object("machine",
                                     "machine construct prop",
                                     "Set machine object, the dialog handles",
                                     BT_TYPE_MACHINE, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_WRITABLE|G_PARAM_STATIC_STRINGS));

}

GType bt_machine_rename_dialog_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      sizeof (BtMachineRenameDialogClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_machine_rename_dialog_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtMachineRenameDialog),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_machine_rename_dialog_init, // instance_init
    };
    type = g_type_register_static(GTK_TYPE_DIALOG,"BtMachineRenameDialog",&info,0);
  }
  return type;
}
