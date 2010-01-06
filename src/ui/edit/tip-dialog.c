/* $Id$
 *
 * Buzztard
 * Copyright (C) 2010 Buzztard team <buzztard-devel@lists.sf.net>
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
 * SECTION:bttipdialog
 * @short_description: class for the editor tip-of-the-day dialog
 *
 * Show a tip, that has not yet been shown to the user.
 */
/*
Algorithmus:
- max_tips = G_N_ELEMENTS(tips)
- fill gint pending_tips[max_tips] with 0...(max_tips-1)
- read presented-tips from settings
- parse and set pending_tips[i]=-1; (only if i<max_tips)
- loop over pending_tips[] and set pending_tips_compressed[j]=i
  while skipping pending_tips[i]==-1;
- take a random number r < j
- show that tip and set pending_tips[j]=-1;
- loop over pending_tips[] and build new presented-tips string
  to be stored in settings
*/

#define BT_EDIT
#define BT_TIP_DIALOG_C

#include "bt-edit.h"

static gchar *tips[]={
  N_("New machines are added in machine view from the context menu."),
  N_("Songs can be recoderd as single waves per track to give it to remixers."),
  N_("Fill the details on the info page. When recording songs the metadata is added to the recording as tags."),
  N_("Use jackaudio sink in audio device settings to get lower latencies for live machine control."),
  N_("You can use input devices such as joysticks, beside midi devices to live control machine parameters."),
  N_("You can use a upnp media client (e.g. media streamer on nokia tablets) to remote control buzztard."),
  N_("To enter notes, imagine you pc keyboard as a music keyboard in two rows. Bottom left y/z key becomes a 'c', s a 'c#', x a 'd' and so on."),
  N_("You can get more help from the community on irc://irc.freenode.net/#buzztard.")
};

enum {
  TIP_DIALOG_APP=1
};

struct _BtTipDialogPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  BtSettings *settings;
  GtkLabel *tip_view;

  /* the application */
  G_POINTER_ALIAS(BtEditApplication *,app);
};

static GtkDialogClass *parent_class=NULL;

//-- event handler

static void on_refresh_clicked(GtkButton *button, gpointer user_data) {
  BtTipDialog *self = BT_TIP_DIALOG(user_data);
  guint ix=g_random_int_range(0,G_N_ELEMENTS(tips));

  // @todo: use and update settings
  gtk_label_set_text(self->priv->tip_view, tips[ix]);
}

//-- helper methods

static gboolean bt_tip_dialog_init_ui(const BtTipDialog *self) {
  GtkWidget *label,*icon,*hbox,*vbox,*btn;
  gchar *str;
  
  gtk_window_set_title(GTK_WINDOW(self),_("tip of the day"));

  // add dialog commision widgets (okay)
  gtk_dialog_add_buttons(GTK_DIALOG(self),
                          GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                          NULL);

  // content area
  // @todo: need a "show tips on startup" checkbox
  hbox=gtk_hbox_new(FALSE,12);
  
  icon=gtk_image_new_from_stock(GTK_STOCK_DIALOG_INFO,GTK_ICON_SIZE_DIALOG);
  gtk_box_pack_start(GTK_BOX(hbox),icon,FALSE,FALSE,0);
  
  vbox=gtk_vbox_new(FALSE,6);
  str=g_strdup_printf("<big><b>%s</b></big>\n",_("tip of the day"));
  label=g_object_new(GTK_TYPE_LABEL,"use-markup",TRUE,"label",str,NULL);
  g_free(str);
  gtk_box_pack_start(GTK_BOX(vbox),label,FALSE,FALSE,0);
  // @todo: make readonly textview (see about dialog)
  self->priv->tip_view=GTK_LABEL(g_object_new(GTK_TYPE_LABEL,"selectable",TRUE,"wrap",TRUE,"label",NULL,NULL));
  gtk_box_pack_start(GTK_BOX(vbox),GTK_WIDGET(self->priv->tip_view),TRUE,TRUE,0);
  gtk_box_pack_start(GTK_BOX(hbox),vbox,TRUE,TRUE,0);

  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(self)->vbox),hbox,TRUE,TRUE,0);
  
  // add "refresh" button to action area
  btn=gtk_button_new_from_stock(GTK_STOCK_REFRESH);
  g_signal_connect(G_OBJECT(btn), "clicked", G_CALLBACK(on_refresh_clicked), (gpointer)self);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(self)->action_area),btn,FALSE,FALSE,0);

  gtk_dialog_set_has_separator(GTK_DIALOG(self),TRUE);

  on_refresh_clicked(GTK_BUTTON(btn),(gpointer)self);
  return(TRUE);
}

//-- constructor methods

/**
 * bt_tip_dialog_new:
 * @app: the application the dialog belongs to
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtTipDialog *bt_tip_dialog_new(const BtEditApplication *app) {
  BtTipDialog *self;

  if(!(self=BT_TIP_DIALOG(g_object_new(BT_TYPE_TIP_DIALOG,"app",app,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_tip_dialog_init_ui(self)) {
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

/* sets the given properties for this object */
static void bt_tip_dialog_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtTipDialog *self = BT_TIP_DIALOG(object);
  return_if_disposed();
  switch (property_id) {
    case TIP_DIALOG_APP: {
      g_object_try_weak_unref(self->priv->app);
      self->priv->app = BT_EDIT_APPLICATION(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->app);
      //GST_DEBUG("set the app for about_dialog: %p",self->priv->app);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_tip_dialog_dispose(GObject *object) {
  BtTipDialog *self = BT_TIP_DIALOG(object);
  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  g_object_try_weak_unref(self->priv->app);

  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_tip_dialog_finalize(GObject *object) {
  //BtTipDialog *self = BT_TIP_DIALOG(object);

  //GST_DEBUG("!!!! self=%p",self);

  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void bt_tip_dialog_init(GTypeInstance *instance, gpointer g_class) {
  BtTipDialog *self = BT_TIP_DIALOG(instance);

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_TIP_DIALOG, BtTipDialogPrivate);
}

static void bt_tip_dialog_class_init(BtTipDialogClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtTipDialogPrivate));

  gobject_class->set_property = bt_tip_dialog_set_property;
  gobject_class->dispose      = bt_tip_dialog_dispose;
  gobject_class->finalize     = bt_tip_dialog_finalize;

  g_object_class_install_property(gobject_class,TIP_DIALOG_APP,
                                  g_param_spec_object("app",
                                     "app construct prop",
                                     "Set application object, the dialog belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_WRITABLE|G_PARAM_STATIC_STRINGS));
}

GType bt_tip_dialog_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      sizeof(BtTipDialogClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_tip_dialog_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof(BtTipDialog),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_tip_dialog_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(GTK_TYPE_DIALOG,"BtTipDialog",&info,0);
  }
  return type;
}
