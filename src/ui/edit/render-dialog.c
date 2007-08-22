/* $Id: render-dialog.c,v 1.9 2007-08-22 13:46:10 ensonic Exp $
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
 * SECTION:btrenderdialog
 * @short_description: class for the editor render dialog
 *
 * Provides UI to access the song recording
 */
/* @todo: need readable properties for the settings
 * - folder + file_name should be combined when read
 */

#define BT_EDIT
#define BT_RENDER_DIALOG_C

#include "bt-edit.h"

enum {
  RENDER_DIALOG_APP=1,
  RENDER_DIALOG_FOLDER,
  RENDER_DIALOG_FILENAME,
  RENDER_DIALOG_FORMAT,
  RENDER_DIALOG_MODE
};

struct _BtRenderDialogPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  G_POINTER_ALIAS(BtEditApplication *,app);

  /* dialog data */
  gchar *folder,*filename;
  BtSinkBinRecordFormat format;
};

static GtkDialogClass *parent_class=NULL;

//-- event handler

static void on_filename_changed(GtkEditable *editable,gpointer user_data) {
  BtRenderDialog *self=BT_RENDER_DIALOG(user_data);

  g_assert(user_data);
  g_free(self->priv->filename);
  self->priv->filename=g_strdup(gtk_entry_get_text(GTK_ENTRY(editable)));
}

static void on_folder_changed(GtkFileChooser *chooser,gpointer user_data) {
  BtRenderDialog *self=BT_RENDER_DIALOG(user_data);

  g_free(self->priv->folder);
  self->priv->folder=gtk_file_chooser_get_current_folder(chooser);

  GST_WARNING("folder changed '%s'",self->priv->folder);
}

static void on_format_menu_changed(GtkComboBox *menu, gpointer user_data) {
  BtRenderDialog *self=BT_RENDER_DIALOG(user_data);

  self->priv->format=gtk_combo_box_get_active(menu);
}


//-- helper methods

static gboolean bt_render_dialog_init_ui(const BtRenderDialog *self) {
  GtkWidget *box,*label,*widget,*table;
  GEnumClass *enum_class;
  GEnumValue *enum_value;
  guint i;

  gtk_widget_set_name(GTK_WIDGET(self),_("song rendering"));

  gtk_window_set_title(GTK_WINDOW(self), _("song rendering"));

  // add dialog commision widgets (okay, cancel)
  gtk_dialog_add_buttons(GTK_DIALOG(self),
                          GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                          GTK_STOCK_CANCEL,GTK_RESPONSE_REJECT,
                          NULL);

  // add widgets to the dialog content area
  box=GTK_DIALOG(self)->vbox;  //gtk_vbox_new(FALSE,12);
  gtk_box_set_spacing(GTK_BOX(box),12);
  gtk_container_set_border_width(GTK_CONTAINER(box),6);

  table=gtk_table_new(/*rows=*/5,/*columns=*/2,/*homogenous=*/FALSE);
  gtk_container_add(GTK_CONTAINER(box),table);

  label=gtk_label_new(_("Folder"));
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(table),label, 0, 1, 0, 1, GTK_SHRINK,GTK_SHRINK, 2,1);

  widget=gtk_file_chooser_button_new(_("Select a folder"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
  //gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (button), "/etc");
  g_signal_connect(G_OBJECT(widget), "selection-changed", G_CALLBACK(on_folder_changed), (gpointer)self);
  gtk_table_attach(GTK_TABLE(table),widget, 1, 2, 0, 1, GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND, 2,1);


  label=gtk_label_new(_("Filename"));
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(table),label, 0, 1, 1, 2, GTK_SHRINK,GTK_SHRINK, 2,1);

  // query supported formats from sinkbin
  widget=gtk_entry_new();
  g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(on_filename_changed), (gpointer)self);
  gtk_table_attach(GTK_TABLE(table),widget, 1, 2, 1, 2, GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND, 2,1);


  label=gtk_label_new(_("Format"));
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(table),label, 0, 1, 2, 3, GTK_SHRINK,GTK_SHRINK, 2,1);

  // query supported formats from sinkbin
  widget=gtk_combo_box_new_text();
  enum_class=g_type_class_peek_static(BT_TYPE_SINK_BIN_RECORD_FORMAT);
  for(i=enum_class->minimum;i<=enum_class->maximum;i++) {
    if((enum_value=g_enum_get_value(enum_class,i))) {
      gtk_combo_box_append_text(GTK_COMBO_BOX(widget),enum_value->value_nick);
    }
  }
  gtk_combo_box_set_active(GTK_COMBO_BOX(widget),BT_SINK_BIN_RECORD_FORMAT_OGG_VORBIS);
  g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(on_format_menu_changed), (gpointer)self);
  gtk_table_attach(GTK_TABLE(table),widget, 1, 2, 2, 3, GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND, 2,1);


  label=gtk_label_new(_("Mode"));
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(table),label, 0, 1, 3, 4, GTK_SHRINK,GTK_SHRINK, 2,1);

  // query supported formats from sinkbin
  widget=gtk_combo_box_new_text();
  gtk_combo_box_append_text(GTK_COMBO_BOX(widget),_("Mixdown"));
  gtk_combo_box_append_text(GTK_COMBO_BOX(widget),_("Single tracks"));
  gtk_combo_box_set_active(GTK_COMBO_BOX(widget),0);
  gtk_table_attach(GTK_TABLE(table),widget, 1, 2, 3, 4, GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND, 2,1);

  /* @todo: add widgets and callbacks
    o write project file
      o none, jokosher, ...
  */

  return(TRUE);
}

//-- constructor methods

/**
 * bt_render_dialog_new:
 * @app: the application the dialog belongs to
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtRenderDialog *bt_render_dialog_new(const BtEditApplication *app) {
  BtRenderDialog *self;

  if(!(self=BT_RENDER_DIALOG(g_object_new(BT_TYPE_RENDER_DIALOG,"app",app,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_render_dialog_init_ui(self)) {
    goto Error;
  }
  return(self);
Error:
  gtk_widget_destroy(GTK_WIDGET(self));
  return(NULL);
}

//-- methods

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_render_dialog_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtRenderDialog *self = BT_RENDER_DIALOG(object);
  return_if_disposed();
  switch (property_id) {
    case RENDER_DIALOG_APP: {
      g_value_set_object(value, self->priv->app);
    } break;
    case RENDER_DIALOG_FOLDER: {
      g_value_set_string(value, self->priv->folder);
    } break;
    case RENDER_DIALOG_FILENAME: {
      g_value_set_string(value, self->priv->filename);
    } break;
    case RENDER_DIALOG_FORMAT: {
      g_value_set_enum(value, self->priv->format);
    } break;
    default: {
       G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_render_dialog_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtRenderDialog *self = BT_RENDER_DIALOG(object);
  return_if_disposed();
  switch (property_id) {
    case RENDER_DIALOG_APP: {
      g_object_try_weak_unref(self->priv->app);
      self->priv->app = BT_EDIT_APPLICATION(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->app);
      //GST_DEBUG("set the app for render_dialog: %p",self->priv->app);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_render_dialog_dispose(GObject *object) {
  BtRenderDialog *self = BT_RENDER_DIALOG(object);
  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  g_object_try_weak_unref(self->priv->app);

  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_render_dialog_finalize(GObject *object) {
  BtRenderDialog *self = BT_RENDER_DIALOG(object);

  GST_DEBUG("!!!! self=%p",self);
  g_free(self->priv->folder);
  g_free(self->priv->filename);

  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void bt_render_dialog_init(GTypeInstance *instance, gpointer g_class) {
  BtRenderDialog *self = BT_RENDER_DIALOG(instance);

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_RENDER_DIALOG, BtRenderDialogPrivate);
}

static void bt_render_dialog_class_init(BtRenderDialogClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtRenderDialogPrivate));

  gobject_class->set_property = bt_render_dialog_set_property;
  gobject_class->get_property = bt_render_dialog_get_property;
  gobject_class->dispose      = bt_render_dialog_dispose;
  gobject_class->finalize     = bt_render_dialog_finalize;

  g_object_class_install_property(gobject_class,RENDER_DIALOG_APP,
                                  g_param_spec_object("app",
                                     "app construct prop",
                                     "Set application object, the dialog belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,RENDER_DIALOG_FOLDER,
                                  g_param_spec_string("folder",
                                     "folder prop",
                                     "Get choosen folder",
                                     NULL,
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,RENDER_DIALOG_FILENAME,
                                  g_param_spec_string("file-name",
                                     "file-name prop",
                                     "Get choosen filename",
                                     NULL,
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,RENDER_DIALOG_FORMAT,
                                  g_param_spec_enum("format",
                                     "format prop",
                                     "format to use",
                                     BT_TYPE_SINK_BIN_RECORD_FORMAT,  /* enum type */
                                     BT_SINK_BIN_RECORD_FORMAT_OGG_VORBIS, /* default value */
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));
  // @todo: add mode
}

GType bt_render_dialog_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      sizeof(BtRenderDialogClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_render_dialog_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof(BtRenderDialog),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_render_dialog_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(GTK_TYPE_DIALOG,"BtRenderDialog",&info,0);
  }
  return type;
}
