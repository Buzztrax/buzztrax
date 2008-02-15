/* $Id: missing-song-elements-dialog.c,v 1.2 2007-08-16 11:07:46 ensonic Exp $
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
 * SECTION:btmissingsongelementsdialog
 * @short_description: missing song machine and wave elements
 *
 * A dialog to inform about missing song machine and wave elements.
 */
/*
 * @todo: support gst-codec-install (see missing-framework-elements-dialog.c)
 */
#define BT_EDIT
#define BT_MISSING_SONG_ELEMENTS_DIALOG_C

#include "bt-edit.h"

//-- property ids

enum {
  MISSING_SONG_ELEMENTS_DIALOG_APP=1,
  MISSING_SONG_ELEMENTS_DIALOG_MACHINES,
  MISSING_SONG_ELEMENTS_DIALOG_WAVES,
};

struct _BtMissingSongElementsDialogPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* list of missing elements */
  GList *machines, *waves;

  GtkWidget *ignore_button;
};

static GtkDialogClass *parent_class=NULL;

//-- event handler

//-- helper methods

static void make_listview(GtkWidget *vbox,GList *missing_elements,const gchar *msg) {
  GtkWidget *label,*missing_list,*missing_list_view;
  GList *node;
  gchar *missing_text,*ptr;
  gint length=0;

  label=gtk_label_new(msg);
  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
  gtk_box_pack_start(GTK_BOX(vbox),label,FALSE,FALSE,0);

  for(node=missing_elements;node;node=g_list_next(node)) {
    length+=2+strlen((gchar *)(node->data));
  }
  ptr=missing_text=g_malloc(length);
  for(node=missing_elements;node;node=g_list_next(node)) {
    length=g_sprintf(ptr,"%s\n",(gchar *)(node->data));
    ptr=&ptr[length];
  }
  ptr[-1]='\0'; // remove last '\n'

  missing_list = gtk_text_view_new();
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(missing_list), FALSE);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(missing_list), FALSE);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(missing_list), GTK_WRAP_WORD);
  gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(missing_list)),missing_text,-1);
  gtk_widget_show(missing_list);
  g_free(missing_text);

  missing_list_view = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW (missing_list_view), GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (missing_list_view), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(missing_list_view), missing_list);
  gtk_widget_show(missing_list_view);
  gtk_box_pack_start(GTK_BOX(vbox),missing_list_view,TRUE,TRUE,0);
}

static gboolean bt_missing_song_elements_dialog_init_ui(const BtMissingSongElementsDialog *self) {
  GtkWidget *label,*icon,*hbox,*vbox;
  gchar *str;
  gboolean res=TRUE;
  //GdkPixbuf *window_icon=NULL;

  gtk_widget_set_name(GTK_WIDGET(self),_("Missing elements in song"));

  // create and set window icon
  /*
  if((window_icon=bt_ui_ressources_get_pixbuf_by_machine(self->priv->machine))) {
    gtk_window_set_icon(GTK_WINDOW(self),window_icon);
  }
  */

  // set a title
  gtk_window_set_title(GTK_WINDOW(self),_("Missing elements in song"));

    // add dialog commision widgets (okay, cancel)
  gtk_dialog_add_buttons(GTK_DIALOG(self),
                          GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                          NULL);

  gtk_dialog_set_default_response(GTK_DIALOG(self),GTK_RESPONSE_ACCEPT);

  hbox=gtk_hbox_new(FALSE,12);
  gtk_container_set_border_width(GTK_CONTAINER(hbox),6);

  icon=gtk_image_new_from_stock(GTK_STOCK_DIALOG_WARNING,GTK_ICON_SIZE_DIALOG);
  gtk_container_add(GTK_CONTAINER(hbox),icon);

  vbox=gtk_vbox_new(FALSE,6);
  label=gtk_label_new(NULL);
  str=g_strdup_printf("<big><b>%s</b></big>",_("Missing elements in song"));
  gtk_label_set_markup(GTK_LABEL(label),str);
  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
  g_free(str);
  gtk_box_pack_start(GTK_BOX(vbox),label,FALSE,FALSE,0);
  if(self->priv->machines) {
    GST_DEBUG("%d missing machines",g_list_length(self->priv->machines));
    make_listview(vbox,self->priv->machines,_("The machines listed below are missing or failed to load."));
  }
  if(self->priv->waves) {
    GST_DEBUG("%d missing core elements",g_list_length(self->priv->waves));
    make_listview(vbox,self->priv->waves,_("The waves listed below are missing or failed to load."));
  }
  gtk_container_add(GTK_CONTAINER(hbox),vbox);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(self)->vbox),hbox);

  return(res);
}

//-- constructor methods

/**
 * bt_missing_song_elements_dialog_new:
 * @app: the application the dialog belongs to
 * @machines: list of missing machine elements
 * @waves: list of missing wave files
 *
 * Create a new instance
 *
 * Returns: the new instance or NULL in case of an error
 */
BtMissingSongElementsDialog *bt_missing_song_elements_dialog_new(const BtEditApplication *app,GList *machines,GList *waves) {
  BtMissingSongElementsDialog *self;

  if(!(self=BT_MISSING_SONG_ELEMENTS_DIALOG(g_object_new(BT_TYPE_MISSING_SONG_ELEMENTS_DIALOG,"app",app,"machines",machines,"waves",waves,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_missing_song_elements_dialog_init_ui(self)) {
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
static void bt_missing_song_elements_dialog_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtMissingSongElementsDialog *self = BT_MISSING_SONG_ELEMENTS_DIALOG(object);
  return_if_disposed();
  switch (property_id) {
    case MISSING_SONG_ELEMENTS_DIALOG_APP: {
      g_value_set_object(value, self->priv->app);
    } break;
    case MISSING_SONG_ELEMENTS_DIALOG_MACHINES: {
      g_value_set_pointer(value, self->priv->machines);
    } break;
    case MISSING_SONG_ELEMENTS_DIALOG_WAVES: {
      g_value_set_pointer(value, self->priv->waves);
    } break;
    default: {
       G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_missing_song_elements_dialog_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtMissingSongElementsDialog *self = BT_MISSING_SONG_ELEMENTS_DIALOG(object);
  return_if_disposed();
  switch (property_id) {
    case MISSING_SONG_ELEMENTS_DIALOG_APP: {
      g_object_try_unref(self->priv->app);
      self->priv->app = g_object_try_ref(g_value_get_object(value));
      GST_DEBUG("set the app for missing_song_elements_dialog: %p",self->priv->app);
    } break;
    case MISSING_SONG_ELEMENTS_DIALOG_MACHINES: {
      self->priv->machines = g_value_get_pointer(value);
    } break;
    case MISSING_SONG_ELEMENTS_DIALOG_WAVES: {
      self->priv->waves = g_value_get_pointer(value);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_missing_song_elements_dialog_dispose(GObject *object) {
  BtMissingSongElementsDialog *self = BT_MISSING_SONG_ELEMENTS_DIALOG(object);

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);

  g_object_try_unref(self->priv->app);

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_missing_song_elements_dialog_finalize(GObject *object) {
  BtMissingSongElementsDialog *self = BT_MISSING_SONG_ELEMENTS_DIALOG(object);

  GST_DEBUG("!!!! self=%p",self);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_missing_song_elements_dialog_init(GTypeInstance *instance, gpointer g_class) {
  BtMissingSongElementsDialog *self = BT_MISSING_SONG_ELEMENTS_DIALOG(instance);

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_MISSING_SONG_ELEMENTS_DIALOG, BtMissingSongElementsDialogPrivate);
}

static void bt_missing_song_elements_dialog_class_init(BtMissingSongElementsDialogClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtMissingSongElementsDialogPrivate));

  gobject_class->set_property = bt_missing_song_elements_dialog_set_property;
  gobject_class->get_property = bt_missing_song_elements_dialog_get_property;
  gobject_class->dispose      = bt_missing_song_elements_dialog_dispose;
  gobject_class->finalize     = bt_missing_song_elements_dialog_finalize;

  g_object_class_install_property(gobject_class,MISSING_SONG_ELEMENTS_DIALOG_APP,
                                  g_param_spec_object("app",
                                     "app construct prop",
                                     "Set application object, the dialog belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,MISSING_SONG_ELEMENTS_DIALOG_MACHINES,
                                  g_param_spec_pointer("machines",
                                     "machines construct prop",
                                     "Set missing machines list, the dialog handles",
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,MISSING_SONG_ELEMENTS_DIALOG_WAVES,
                                  g_param_spec_pointer("waves",
                                     "waves construct prop",
                                     "Set missing waves list, the dialog handles",
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));
}

GType bt_missing_song_elements_dialog_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    const GTypeInfo info = {
      sizeof (BtMissingSongElementsDialogClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_missing_song_elements_dialog_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtMissingSongElementsDialog),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_missing_song_elements_dialog_init, // instance_init
    };
    type = g_type_register_static(GTK_TYPE_DIALOG,"BtMissingSongElementsDialog",&info,0);
  }
  return type;
}
