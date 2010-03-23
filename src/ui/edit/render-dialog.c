/* $Id$
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
/* @todo:
 * - use chooserbutton to choose name too - no, it can not do save_as
 * - use song-name based file-name by default
 */

#define BT_EDIT
#define BT_RENDER_DIALOG_C

#include "bt-edit.h"

enum {
  RENDER_DIALOG_FILENAME=1,
  RENDER_DIALOG_FORMAT,
  RENDER_DIALOG_MODE
};

struct _BtRenderDialogPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* dialog widgets */
  GtkWidget *file_name_entry;
  GtkWidget *format_menu;

  /* dialog data */
  gchar *folder,*filename;
  BtSinkBinRecordFormat format;
  BtRenderMode mode;
};

static GtkDialogClass *parent_class=NULL;


static void on_format_menu_changed(GtkComboBox *menu, gpointer user_data);


//-- enums

GType bt_render_mode_get_type(void) {
  static GType type = 0;
  if(G_UNLIKELY(type==0)) {
    static const GEnumValue values[] = {
      { BT_RENDER_MODE_MIXDOWN,       "BT_RENDER_MODE_MIXDOWN",       "mix to one track" },
      { BT_RENDER_MODE_SINGLE_TRACKS, "BT_RENDER_MODE_SINGLE_TRACKS", "record one track for each source" },
      { 0, NULL, NULL},
    };
    type = g_enum_register_static("BtRenderMode", values);
  }
  return type;
}


//-- event handler

static void on_folder_changed(GtkFileChooser *chooser,gpointer user_data) {
  BtRenderDialog *self=BT_RENDER_DIALOG(user_data);

  g_free(self->priv->folder);
  self->priv->folder=gtk_file_chooser_get_current_folder(chooser);

  GST_DEBUG("folder changed '%s'",self->priv->folder);
}

static void on_filename_changed(GtkEditable *editable,gpointer user_data) {
  BtRenderDialog *self=BT_RENDER_DIALOG(user_data);

  g_free(self->priv->filename);
  self->priv->filename=g_strdup(gtk_entry_get_text(GTK_ENTRY(editable)));

  // update format
  if(self->priv->filename) {
    GEnumClass *enum_class;
    GEnumValue *enum_value;
    guint i;

    enum_class=g_type_class_peek_static(BT_TYPE_SINK_BIN_RECORD_FORMAT);
    for(i=enum_class->minimum;i<=enum_class->maximum;i++) {
      if((enum_value=g_enum_get_value(enum_class,i))) {
        if(g_str_has_suffix(self->priv->filename,enum_value->value_name)) {
          g_signal_handlers_block_matched(self->priv->format_menu,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_format_menu_changed,(gpointer)user_data);
          gtk_combo_box_set_active(GTK_COMBO_BOX(self->priv->format_menu),i);
          g_signal_handlers_unblock_matched(self->priv->format_menu,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_format_menu_changed,(gpointer)user_data);
          break;
        }
      }
    }
  }
}

static void on_format_menu_changed(GtkComboBox *menu, gpointer user_data) {
  BtRenderDialog *self=BT_RENDER_DIALOG(user_data);

  self->priv->format=gtk_combo_box_get_active(menu);

  // update filename
  if(self->priv->filename) {
    GEnumClass *enum_class;
    GEnumValue *enum_value;
    guint i;
    gchar *fn=self->priv->filename;

    enum_class=g_type_class_peek_static(BT_TYPE_SINK_BIN_RECORD_FORMAT);
    for(i=enum_class->minimum;i<=enum_class->maximum;i++) {
      if((enum_value=g_enum_get_value(enum_class,i))) {
        if(g_str_has_suffix(fn,enum_value->value_name)) {
          GST_INFO("found matching ext: %s",enum_value->value_name);
          // replace this suffix, with the new
          fn[strlen(fn)-strlen(enum_value->value_name)]='\0';
          GST_INFO("cut fn to: %s",fn);
          break;
        }
      }
    }
    enum_value=g_enum_get_value(enum_class,self->priv->format);
    self->priv->filename=g_strdup_printf("%s%s",fn,enum_value->value_name);
    GST_INFO("set new fn to: %s",self->priv->filename);
    g_free(fn);
    g_signal_handlers_block_matched(self->priv->file_name_entry,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_filename_changed,(gpointer)user_data);
    gtk_entry_set_text(GTK_ENTRY(self->priv->file_name_entry), self->priv->filename);
    g_signal_handlers_unblock_matched(self->priv->file_name_entry,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_filename_changed,(gpointer)user_data);
  }
}

static void on_mode_menu_changed(GtkComboBox *menu, gpointer user_data) {
  BtRenderDialog *self=BT_RENDER_DIALOG(user_data);

  self->priv->mode=gtk_combo_box_get_active(menu);
}


//-- helper methods

static gchar *bt_render_dialog_make_file_name(const BtRenderDialog *self) {
  gchar *file_name=g_build_filename(self->priv->folder,self->priv->filename,NULL);
  gchar *track_str="";
  GEnumClass *enum_class;
  GEnumValue *enum_value;

  if(self->priv->mode==BT_RENDER_MODE_SINGLE_TRACKS) {
    track_str=".%03u";
  }

  // add file suffix if not yet there
  enum_class=g_type_class_peek_static(BT_TYPE_SINK_BIN_RECORD_FORMAT);
  enum_value=g_enum_get_value(enum_class,self->priv->format);
  if(!g_str_has_suffix(file_name,enum_value->value_name)) {
    gchar *tmp_file_name;

    tmp_file_name=g_strdup_printf("%s%s%s",
      file_name,track_str,enum_value->value_name);
    g_free(file_name);
    file_name=tmp_file_name;
  }
  else {
    if(self->priv->mode==BT_RENDER_MODE_SINGLE_TRACKS) {
      gchar *tmp_file_name;

      file_name[strlen(file_name)-strlen(enum_value->value_name)]='\0';
      tmp_file_name=g_strdup_printf("%s%s%s",
        file_name,track_str,enum_value->value_name);
      g_free(file_name);
      file_name=tmp_file_name;
    }
  }
  GST_INFO("record file template: '%s'",file_name);

  return(file_name);
}

static void bt_render_dialog_init_ui(const BtRenderDialog *self) {
  BtSettings *settings;
  GtkWidget *box,*label,*widget,*table;
  GEnumClass *enum_class;
  GEnumValue *enum_value;
  guint i;
  BtSong *song;
  BtSongInfo *song_info;
  gchar *file_name=NULL,*ext;
  
  GST_DEBUG("read settings");
  
  g_object_get(self->priv->app,"settings",&settings,NULL);
  g_object_get(settings,"record-folder",&self->priv->folder,NULL);
  g_object_unref(settings);

  GST_DEBUG("prepare render dialog");

  gtk_widget_set_name(GTK_WIDGET(self),"song rendering");

  gtk_window_set_title(GTK_WINDOW(self),_("song rendering"));

  // add dialog commision widgets (okay, cancel)
  gtk_dialog_add_buttons(GTK_DIALOG(self),
                          GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                          GTK_STOCK_CANCEL,GTK_RESPONSE_REJECT,
                          NULL);

  // add widgets to the dialog content area
  box=gtk_vbox_new(FALSE,12);
  gtk_container_set_border_width(GTK_CONTAINER(box),6);
  gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(self))),box);

  table=gtk_table_new(/*rows=*/5,/*columns=*/2,/*homogenous=*/FALSE);
  gtk_container_add(GTK_CONTAINER(box),table);


  label=gtk_label_new(_("Folder"));
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(table),label, 0, 1, 0, 1, GTK_FILL,GTK_SHRINK, 2,1);

  widget=gtk_file_chooser_button_new(_("Select a folder"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
  gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER (widget), self->priv->folder);
  g_signal_connect(widget, "selection-changed", G_CALLBACK(on_folder_changed), (gpointer)self);
  gtk_table_attach(GTK_TABLE(table),widget, 1, 2, 0, 1, GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND, 2,1);


  label=gtk_label_new(_("Filename"));
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(table),label, 0, 1, 1, 2, GTK_FILL,GTK_SHRINK, 2,1);

  // set deault name
  g_object_get(self->priv->app,"song",&song,NULL);
  g_object_get(song,"song-info",&song_info,NULL);
  g_object_get(song_info,"file-name",&file_name,NULL);
  if(file_name) {
    // cut off extension from file_name
    if((ext=strchr(file_name,'.'))) *ext='\0';
    self->priv->filename=g_strdup_printf("%s.ogg",file_name);
    g_free(file_name);
  }
  else {
    self->priv->filename=g_strdup_printf(".ogg");
  }
  g_object_unref(song_info);
  g_object_unref(song);

  self->priv->file_name_entry=widget=gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY (widget), self->priv->filename);
  g_signal_connect(widget, "changed", G_CALLBACK(on_filename_changed), (gpointer)self);
  gtk_table_attach(GTK_TABLE(table),widget, 1, 2, 1, 2, GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND, 2,1);


  label=gtk_label_new(_("Format"));
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(table),label, 0, 1, 2, 3, GTK_FILL,GTK_SHRINK, 2,1);

  // query supported formats from sinkbin
  self->priv->format_menu=widget=gtk_combo_box_new_text();
  enum_class=g_type_class_peek_static(BT_TYPE_SINK_BIN_RECORD_FORMAT);
  for(i=enum_class->minimum;i<=enum_class->maximum;i++) {
    if((enum_value=g_enum_get_value(enum_class,i))) {
      gtk_combo_box_append_text(GTK_COMBO_BOX(widget),enum_value->value_nick);
    }
  }
  gtk_combo_box_set_active(GTK_COMBO_BOX(widget),BT_SINK_BIN_RECORD_FORMAT_OGG_VORBIS);
  g_signal_connect(widget, "changed", G_CALLBACK(on_format_menu_changed), (gpointer)self);
  gtk_table_attach(GTK_TABLE(table),widget, 1, 2, 2, 3, GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND, 2,1);


  label=gtk_label_new(_("Mode"));
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(table),label, 0, 1, 3, 4, GTK_FILL,GTK_SHRINK, 2,1);

  // query supported formats from sinkbin
  widget=gtk_combo_box_new_text();
  enum_class=g_type_class_peek_static(BT_TYPE_RENDER_MODE);
  for(i=enum_class->minimum;i<=enum_class->maximum;i++) {
    if((enum_value=g_enum_get_value(enum_class,i))) {
      gtk_combo_box_append_text(GTK_COMBO_BOX(widget),enum_value->value_nick);
    }
  }
  gtk_combo_box_set_active(GTK_COMBO_BOX(widget),BT_RENDER_MODE_MIXDOWN);
  g_signal_connect(widget, "changed", G_CALLBACK(on_mode_menu_changed), (gpointer)self);
  gtk_table_attach(GTK_TABLE(table),widget, 1, 2, 3, 4, GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND, 2,1);

  /* @todo: add more widgets
    o write project file
      o none, jokosher, ...
  */

  GST_DEBUG("done");
}

//-- constructor methods

/**
 * bt_render_dialog_new:
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtRenderDialog *bt_render_dialog_new(void) {
  BtRenderDialog *self;

  self=BT_RENDER_DIALOG(g_object_new(BT_TYPE_RENDER_DIALOG,NULL));
  bt_render_dialog_init_ui(self);
  return(self);
}

//-- methods

//-- wrapper

//-- class internals

static void bt_render_dialog_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec) {
  BtRenderDialog *self = BT_RENDER_DIALOG(object);
  return_if_disposed();
  switch (property_id) {
    case RENDER_DIALOG_FILENAME: {
      gchar *file_name=bt_render_dialog_make_file_name(self);
      g_value_set_string(value, file_name);
      g_free(file_name);
    } break;
    case RENDER_DIALOG_FORMAT: {
      g_value_set_enum(value, self->priv->format);
    } break;
    case RENDER_DIALOG_MODE: {
      g_value_set_enum(value, self->priv->mode);
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
  g_object_unref(self->priv->app);

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
  GST_DEBUG("!!!! self=%p",self);
  self->priv->app = bt_edit_application_new();
}

static void bt_render_dialog_class_init(BtRenderDialogClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtRenderDialogPrivate));

  gobject_class->get_property = bt_render_dialog_get_property;
  gobject_class->dispose      = bt_render_dialog_dispose;
  gobject_class->finalize     = bt_render_dialog_finalize;

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

  g_object_class_install_property(gobject_class,RENDER_DIALOG_MODE,
                                  g_param_spec_enum("mode",
                                     "format mode",
                                     "render-mode to use",
                                     BT_TYPE_RENDER_MODE,  /* enum type */
                                     BT_RENDER_MODE_MIXDOWN, /* default value */
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));
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
