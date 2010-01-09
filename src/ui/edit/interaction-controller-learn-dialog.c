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
 * SECTION:btinteractioncontrollerlearndialog
 * @short_description: learn dialog for interaction devices
 *
 * A dialog to prompt the user for an input channel assignment.
 */

#define BT_EDIT
#define BT_INTERACTION_CONTROLLER_LEARN_DIALOG_C

#include "bt-edit.h"

//-- property ids
enum {
  LEARN_DIALOG_DEVICE=1
};

struct _BtInteractionControllerLearnDialogPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  G_POINTER_ALIAS(BtIcDevice *,device);

  GtkWidget *label_output, *entry_name;
};

static GtkDialogClass *parent_class=NULL;

//-- event handler

static void notify_device_controlchange(const BtIcLearn* learn,
					GParamSpec *arg,
					const BtInteractionControllerLearnDialog *user_data)
{
  gchar *control;
  BtInteractionControllerLearnDialog *self=BT_INTERACTION_CONTROLLER_LEARN_DIALOG(user_data);

  g_object_get(BTIC_LEARN(learn),
	       "device-controlchange", &control, NULL);
  gtk_label_set_text(GTK_LABEL(self->priv->label_output), control);
  gtk_entry_set_text(GTK_ENTRY(self->priv->entry_name), control);

  g_free(control);
}

static void on_dialog_response(GtkDialog *dialog,
			       gint signal,
			       const BtInteractionControllerLearnDialog *user_data)
{
  BtInteractionControllerLearnDialog *self=BT_INTERACTION_CONTROLLER_LEARN_DIALOG(user_data);

  switch(signal) {
  case GTK_RESPONSE_ACCEPT:
    //GST_INFO("learn dialog okay");
    btic_learn_register_learned_control(BTIC_LEARN(self->priv->device),
					gtk_entry_get_text(GTK_ENTRY(self->priv->entry_name)));
    break;
  case GTK_RESPONSE_REJECT:
    //GST_INFO("learn dialog cancel");
    break;
  }

  GST_DEBUG("object: %p refs: %d)",self->priv->device,(G_OBJECT(self->priv->device))->ref_count);

  btic_learn_stop(BTIC_LEARN(self->priv->device));
  gtk_widget_destroy(GTK_WIDGET(dialog));

  GST_DEBUG("object: %p refs: %d)",self->priv->device,(G_OBJECT(self->priv->device))->ref_count);
}

//-- helper methods

static void bt_interaction_controller_learn_dialog_init_ui(const BtInteractionControllerLearnDialog *self) {
  GtkWidget *label_detected, *label_naming, *box;
  gchar* title;

  gtk_widget_set_name(GTK_WIDGET(self),"interaction controller learn");

  // set a title
  g_object_get(self->priv->device,"name",&title,NULL);
  gtk_window_set_title(GTK_WINDOW(self), title);

  gtk_dialog_add_buttons(GTK_DIALOG(self),
                         GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                         GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
                         NULL);

  label_detected=gtk_label_new(_("detected control"));
  label_naming=gtk_label_new(_("register as"));
  self->priv->label_output=gtk_label_new(_("none"));
  self->priv->entry_name=gtk_entry_new();
  gtk_entry_set_alignment(GTK_ENTRY(self->priv->entry_name), 0.5f);

  box=GTK_DIALOG(self)->vbox;
  gtk_container_add(GTK_CONTAINER(box), label_detected);
  gtk_container_add(GTK_CONTAINER(box), self->priv->label_output);
  gtk_container_add(GTK_CONTAINER(box), label_naming);
  gtk_container_add(GTK_CONTAINER(box), self->priv->entry_name);
  
  g_signal_connect(self->priv->device, "notify::device-controlchange",
		   G_CALLBACK(notify_device_controlchange), (gpointer)self);

  g_signal_connect(GTK_DIALOG(self), "response",
		   G_CALLBACK(on_dialog_response), (gpointer)self);

  g_free(title);

  GST_INFO("BtInteractionControllerLearnDialog ui initialized");

  btic_learn_start(BTIC_LEARN(self->priv->device));
}

//-- constructor methods

/**
 * bt_interaction_controller_learn_dialog_new:
 * @device: the device of which the learn events are monitored
 *
 * Create a new instance.
 *
 * Returns: the new instance
 */
BtInteractionControllerLearnDialog *bt_interaction_controller_learn_dialog_new(BtIcDevice *device) {
  BtInteractionControllerLearnDialog *self;

  self=BT_INTERACTION_CONTROLLER_LEARN_DIALOG(g_object_new(BT_TYPE_INTERACTION_CONTROLLER_LEARN_DIALOG,"device",device,NULL));
  bt_interaction_controller_learn_dialog_init_ui(self);
  return(self);
}

//-- methods

//-- wrapper

//-- class internals

static void bt_interaction_controller_learn_dialog_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec) {
  BtInteractionControllerLearnDialog *self = BT_INTERACTION_CONTROLLER_LEARN_DIALOG(object);
  return_if_disposed();
  switch (property_id) {
    case LEARN_DIALOG_DEVICE: {
      g_object_try_weak_unref(self->priv->device);
      self->priv->device = BTIC_DEVICE(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->device);
      //GST_DEBUG("set the device for learn_dialog: %p",self->priv->device);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_interaction_controller_learn_dialog_dispose(GObject *object) {
  BtInteractionControllerLearnDialog *self = BT_INTERACTION_CONTROLLER_LEARN_DIALOG(object);

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);

  g_object_try_weak_unref(self->priv->device);
  g_signal_handlers_disconnect_matched(self->priv->device,G_SIGNAL_MATCH_FUNC,0,0,NULL,notify_device_controlchange,NULL);

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_interaction_controller_learn_dialog_finalize(GObject *object) {
  BtInteractionControllerLearnDialog *self = BT_INTERACTION_CONTROLLER_LEARN_DIALOG(object);

  GST_DEBUG("!!!! self=%p",self);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_interaction_controller_learn_dialog_init(GTypeInstance *instance, gpointer g_class) {
  BtInteractionControllerLearnDialog *self = BT_INTERACTION_CONTROLLER_LEARN_DIALOG(instance);

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_INTERACTION_CONTROLLER_LEARN_DIALOG, BtInteractionControllerLearnDialogPrivate);
}

static void bt_interaction_controller_learn_dialog_class_init(BtInteractionControllerLearnDialogClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtInteractionControllerLearnDialogPrivate));

  gobject_class->set_property = bt_interaction_controller_learn_dialog_set_property;
  gobject_class->dispose      = bt_interaction_controller_learn_dialog_dispose;
  gobject_class->finalize     = bt_interaction_controller_learn_dialog_finalize;

  g_object_class_install_property(gobject_class,LEARN_DIALOG_DEVICE,
                                  g_param_spec_object("device",
                                     "device construct prop",
                                     "Set the device we want to snoop for a new controler",
                                     BTIC_TYPE_DEVICE, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_WRITABLE|G_PARAM_STATIC_STRINGS));
}

GType bt_interaction_controller_learn_dialog_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      sizeof (BtInteractionControllerLearnDialogClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_interaction_controller_learn_dialog_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtInteractionControllerLearnDialog),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_interaction_controller_learn_dialog_init, // instance_init
    };
    type = g_type_register_static(GTK_TYPE_DIALOG,"BtInteractionControllerLearnDialog",&info,0);
  }
  return type;
}
