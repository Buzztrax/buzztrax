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
 * A dialog to prompt the user for an interaction controller assignment.
 */
/* @todo: having okay & assign would be nice
 * @todo: having a way to train multiple controllers would be nice
 * - we could have: okay, okay & assign, okay & learn more, cancel
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

  /* dialog widgets */
  GtkWidget *okay_button;
};

//-- the class

G_DEFINE_TYPE (BtInteractionControllerLearnDialog, bt_interaction_controller_learn_dialog, GTK_TYPE_DIALOG);

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
  gtk_editable_select_region(GTK_EDITABLE(self->priv->entry_name),0,-1);
  
  gtk_widget_set_sensitive(self->priv->okay_button,TRUE);

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

  GST_DEBUG("object: %p ref_ct=%d)",self->priv->device,G_OBJECT_REF_COUNT(self->priv->device));

  btic_learn_stop(BTIC_LEARN(self->priv->device));
  gtk_widget_destroy(GTK_WIDGET(dialog));

  GST_DEBUG("object: %p ref_ct=%d)",self->priv->device,G_OBJECT_REF_COUNT(self->priv->device));
}

//-- helper methods

static void bt_interaction_controller_learn_dialog_init_ui(const BtInteractionControllerLearnDialog *self) {
  GtkWidget *label,*box,*table;
  gchar *name,*title;
#if !GTK_CHECK_VERSION(2,20,0)
  GList *buttons;
#endif

  gtk_widget_set_name(GTK_WIDGET(self),"interaction controller learn");

  g_object_get(self->priv->device,"name",&name,NULL);
  // set dialog title
  title=g_strdup_printf(_("learn controller for %s"),name);
  gtk_window_set_title(GTK_WINDOW(self), title);
  g_free(name);g_free(title);

  gtk_dialog_add_buttons(GTK_DIALOG(self),
                         GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                         GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
                         NULL);

  gtk_dialog_set_default_response(GTK_DIALOG(self),GTK_RESPONSE_ACCEPT);

  // grab okay button, so that we can block if input is not valid
#if GTK_CHECK_VERSION(2,20,0)
  self->priv->okay_button=gtk_dialog_get_widget_for_response(GTK_DIALOG(self),GTK_RESPONSE_ACCEPT);
#else
  buttons=gtk_container_get_children(GTK_CONTAINER(gtk_dialog_get_action_area(GTK_DIALOG(self))));
  self->priv->okay_button=GTK_WIDGET(g_list_nth_data(buttons,1));
  g_list_free(buttons);
#endif

  gtk_widget_set_sensitive(self->priv->okay_button,FALSE);
  
  // add widgets to the dialog content area
  box=gtk_vbox_new(FALSE,12);
  gtk_container_set_border_width(GTK_CONTAINER(box),6);
  gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(self))),box);

  label=gtk_label_new(_("Move or press a controller to detect it."));
  gtk_container_add(GTK_CONTAINER(box),label);

  table=gtk_table_new(/*rows=*/2,/*columns=*/2,/*homogenous=*/FALSE);
  gtk_container_add(GTK_CONTAINER(box),table);

  label=gtk_label_new(_("detected control"));
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(table),label, 0, 1, 0, 1, GTK_FILL,GTK_SHRINK, 2,1);

  self->priv->label_output=gtk_label_new(_("none"));
  gtk_misc_set_alignment(GTK_MISC(self->priv->label_output),0.0,0.5);
  gtk_table_attach(GTK_TABLE(table),self->priv->label_output, 1, 2, 0, 1, GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND, 2,1);

  label=gtk_label_new(_("register as"));
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(table),label, 0, 1, 1, 2, GTK_FILL,GTK_SHRINK, 2,1);

  self->priv->entry_name=gtk_entry_new();
  gtk_entry_set_activates_default(GTK_ENTRY(self->priv->entry_name),TRUE);
  gtk_table_attach(GTK_TABLE(table),self->priv->entry_name, 1, 2, 1, 2, GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND, 2,1);


  g_signal_connect(self->priv->device, "notify::device-controlchange",
		   G_CALLBACK(notify_device_controlchange), (gpointer)self);

  g_signal_connect(GTK_DIALOG(self), "response",
		   G_CALLBACK(on_dialog_response), (gpointer)self);

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

  G_OBJECT_CLASS(bt_interaction_controller_learn_dialog_parent_class)->dispose(object);
}

static void bt_interaction_controller_learn_dialog_init(BtInteractionControllerLearnDialog *self) {
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_INTERACTION_CONTROLLER_LEARN_DIALOG, BtInteractionControllerLearnDialogPrivate);
}

static void bt_interaction_controller_learn_dialog_class_init(BtInteractionControllerLearnDialogClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  g_type_class_add_private(klass,sizeof(BtInteractionControllerLearnDialogPrivate));

  gobject_class->set_property = bt_interaction_controller_learn_dialog_set_property;
  gobject_class->dispose      = bt_interaction_controller_learn_dialog_dispose;

  g_object_class_install_property(gobject_class,LEARN_DIALOG_DEVICE,
                                  g_param_spec_object("device",
                                     "device construct prop",
                                     "Set the device we want to snoop for a new controler",
                                     BTIC_TYPE_DEVICE, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_WRITABLE|G_PARAM_STATIC_STRINGS));
}

