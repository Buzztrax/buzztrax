/* $Id: pattern-properties-dialog.c,v 1.21 2007-05-07 14:45:46 ensonic Exp $
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
 * SECTION:btpatternpropertiesdialog
 * @short_description: pattern settings
 *
 * A dialog to (re)configure a #BtPattern.
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

  /* the machine and its id the pattern belongs to, needed to veryfy unique name */
  BtMachine *machine;
  gchar *machine_id;

  /* dialog data */
  gchar *name;
  gulong length,voices;

  /* widgets and their handlers */
  GtkWidget *okay_button;
};

static GtkDialogClass *parent_class=NULL;

//-- event handler

static void on_name_changed(GtkEditable *editable,gpointer user_data) {
  BtPatternPropertiesDialog *self=BT_PATTERN_PROPERTIES_DIALOG(user_data);
  BtPattern *pattern;
  gchar *id;
  const gchar *name=gtk_entry_get_text(GTK_ENTRY(editable));

  g_assert(user_data);
  // assure uniqueness of the entered data
  id=g_strdup_printf("%s %s",self->priv->machine_id,name);
  pattern=bt_machine_get_pattern_by_id(self->priv->machine,id);
  if(!(*name) || (pattern && (pattern!=self->priv->pattern))) {
    GST_INFO("not unique '%s'",id);
    gtk_widget_set_sensitive(self->priv->okay_button,FALSE);
  }
  else {
    GST_INFO("unique    '%s'",id);
    gtk_widget_set_sensitive(self->priv->okay_button,TRUE);
  }
  g_free(id);
  // update field
  g_free(self->priv->name);
  self->priv->name=g_strdup(name);
}

static void on_length_changed(GtkEditable *editable,gpointer user_data) {
  BtPatternPropertiesDialog *self=BT_PATTERN_PROPERTIES_DIALOG(user_data);

  g_assert(user_data);
  // update field
  self->priv->length=atol(gtk_entry_get_text(GTK_ENTRY(editable)));
  //g_object_set(G_OBJECT(self->priv->pattern),"length",atol(gtk_entry_get_text(GTK_ENTRY(editable))),NULL);
}

static void on_voices_changed(GtkSpinButton *spinbutton,gpointer user_data) {
  BtPatternPropertiesDialog *self=BT_PATTERN_PROPERTIES_DIALOG(user_data);

  g_assert(user_data);

  // update field
  self->priv->voices=gtk_spin_button_get_value_as_int(spinbutton);
  //g_object_set(G_OBJECT(self->priv->pattern),"voices",gtk_spin_button_get_value_as_int(spinbutton),NULL);
}
//-- helper methods

static gboolean bt_pattern_properties_dialog_init_ui(const BtPatternPropertiesDialog *self) {
  GtkWidget *box,*label,*widget,*table;
  GtkAdjustment *spin_adjustment;
  gchar *name,*title,*length_str;
  //GdkPixbuf *window_icon=NULL;
  GList *buttons;

  gtk_widget_set_name(GTK_WIDGET(self),_("pattern properties"));

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
                          GTK_STOCK_CANCEL,GTK_RESPONSE_REJECT,
                          NULL);

  gtk_dialog_set_default_response(GTK_DIALOG(self),GTK_RESPONSE_ACCEPT);

  // grab okay button, so that we can block if input is not valid
  buttons=gtk_container_get_children(GTK_CONTAINER(GTK_DIALOG(self)->action_area));
  GST_INFO("dialog buttons: %d",g_list_length(buttons));
  self->priv->okay_button=GTK_WIDGET(g_list_nth_data(buttons,1));
  g_list_free(buttons);

  // add widgets to the dialog content area
  box=GTK_DIALOG(self)->vbox;  //gtk_vbox_new(FALSE,12);
  gtk_box_set_spacing(GTK_BOX(box),12);
  gtk_container_set_border_width(GTK_CONTAINER(box),6);

  table=gtk_table_new(/*rows=*/3,/*columns=*/2,/*homogenous=*/FALSE);
  gtk_container_add(GTK_CONTAINER(box),table);

  g_object_get(G_OBJECT(self->priv->pattern),"name",&self->priv->name,"length",&self->priv->length,"voices",&self->priv->voices,"machine",&self->priv->machine,NULL);
  g_object_get(G_OBJECT(self->priv->machine),"id",&self->priv->machine_id,NULL);

  // GtkEntry : pattern name
  label=gtk_label_new(_("name"));
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(table),label, 0, 1, 0, 1, GTK_SHRINK,GTK_SHRINK, 2,1);
  widget=gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(widget),self->priv->name);
  gtk_entry_set_activates_default(GTK_ENTRY(widget),TRUE);
  gtk_table_attach(GTK_TABLE(table),widget, 1, 2, 0, 1, GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND, 2,1);
  g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(on_name_changed), (gpointer)self);

  // GtkComboBox : pattern length
  label=gtk_label_new(_("length"));
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(table),label, 0, 1, 1, 2, GTK_SHRINK,GTK_SHRINK, 2,1);
  length_str=g_strdup_printf("%lu",self->priv->length);
  widget=gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(widget),length_str);g_free(length_str);
  gtk_table_attach(GTK_TABLE(table),widget, 1, 2, 1, 2, GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND, 2,1);
  g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(on_length_changed), (gpointer)self);

  // GtkSpinButton : number of voices
  label=gtk_label_new(_("voices"));
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(table),label, 0, 1, 2, 3, GTK_SHRINK,GTK_SHRINK, 2,1);
  // @todo get min/max number of voices
  spin_adjustment=GTK_ADJUSTMENT(gtk_adjustment_new(1.0, 1.0, 16.0, 1.0, 4.0, 4.0));
  widget=gtk_spin_button_new(spin_adjustment,(double)(self->priv->voices),0);
  if(bt_machine_is_polyphonic(self->priv->machine)) {
    g_signal_connect(G_OBJECT(widget), "value-changed", G_CALLBACK(on_voices_changed), (gpointer)self);
  }
  else {
    gtk_widget_set_sensitive(widget,FALSE);
  }
  gtk_table_attach(GTK_TABLE(table),widget, 1, 2, 2, 3, GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND, 2,1);

  return(TRUE);
}

//-- constructor methods

/**
 * bt_pattern_properties_dialog_new:
 * @app: the application the dialog belongs to
 * @pattern: the pattern for which to create the dialog for
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
  return(self);
Error:
  g_object_try_unref(self);
  return(NULL);
}

//-- methods

/**
 * bt_pattern_properties_dialog_apply:
 * @self: the dialog which settings to apply
 *
 * Makes the dialog settings effective.
 */
void bt_pattern_properties_dialog_apply(const BtPatternPropertiesDialog *self) {
  GST_INFO("applying pattern settings");

  g_object_set(G_OBJECT(self->priv->pattern),"name",self->priv->name,"length",self->priv->length,"voices",self->priv->voices,NULL);
}

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

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);

  g_object_try_unref(self->priv->app);
  g_object_try_unref(self->priv->pattern);
  g_object_try_unref(self->priv->machine);

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_pattern_properties_dialog_finalize(GObject *object) {
  BtPatternPropertiesDialog *self = BT_PATTERN_PROPERTIES_DIALOG(object);

  GST_DEBUG("!!!! self=%p",self);

  g_free(self->priv->machine_id);
  g_free(self->priv->name);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_pattern_properties_dialog_init(GTypeInstance *instance, gpointer g_class) {
  BtPatternPropertiesDialog *self = BT_PATTERN_PROPERTIES_DIALOG(instance);

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_PATTERN_PROPERTIES_DIALOG, BtPatternPropertiesDialogPrivate);
}

static void bt_pattern_properties_dialog_class_init(BtPatternPropertiesDialogClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtPatternPropertiesDialogPrivate));

  gobject_class->set_property = bt_pattern_properties_dialog_set_property;
  gobject_class->get_property = bt_pattern_properties_dialog_get_property;
  gobject_class->dispose      = bt_pattern_properties_dialog_dispose;
  gobject_class->finalize     = bt_pattern_properties_dialog_finalize;

  g_object_class_install_property(gobject_class,PATTERN_PROPERTIES_DIALOG_APP,
                                  g_param_spec_object("app",
                                     "app construct prop",
                                     "Set application object, the dialog belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,PATTERN_PROPERTIES_DIALOG_PATTERN,
                                  g_param_spec_object("pattern",
                                     "pattern construct prop",
                                     "Set pattern object, the dialog handles",
                                     BT_TYPE_PATTERN, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

}

GType bt_pattern_properties_dialog_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    const GTypeInfo info = {
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
