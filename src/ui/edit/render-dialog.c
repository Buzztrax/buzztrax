/* $Id: render-dialog.c,v 1.1 2007-07-18 14:32:10 ensonic Exp $
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

#define BT_EDIT
#define BT_RENDER_DIALOG_C

#include "bt-edit.h"

enum {
  RENDER_DIALOG_APP=1
};

struct _BtRenderDialogPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  G_POINTER_ALIAS(BtEditApplication *,app);
};

static GtkDialogClass *parent_class=NULL;

//-- event handler

//-- helper methods

static gboolean bt_render_dialog_init_ui(const BtRenderDialog *self) {
  GtkWidget *box;

  gtk_widget_set_name(GTK_WIDGET(self),_("song rendering"));

  gtk_window_set_title(GTK_WINDOW(self), _("song rendering"));

  // add dialog commision widgets (okay, cancel)
  gtk_dialog_add_buttons(GTK_DIALOG(self),
                          GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                          GTK_STOCK_CANCEL,GTK_RESPONSE_REJECT,
                          NULL);

  // add widgets to the dialog content area
  box=gtk_hbox_new(FALSE,12);
  gtk_container_set_border_width(GTK_CONTAINER(box),6);

  /* @todo: add widgets
  - choose format
  - choose filename
  - choose mode
    - mixdown
    - one clip per track
  - write project file
    - none, jokosher, ...
  
  */

  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(self)->vbox),box);
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
  g_object_try_unref(self);
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
  //BtRenderDialog *self = BT_RENDER_DIALOG(object);

  //GST_DEBUG("!!!! self=%p",self);

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
}

GType bt_render_dialog_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      G_STRUCT_SIZE(BtRenderDialogClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_render_dialog_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtRenderDialog),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_render_dialog_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(GTK_TYPE_DIALOG,"BtRenderDialog",&info,0);
  }
  return type;
}
