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
Algorithm:
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
  N_("Connect machines by holding the shift key and dragging a connection for the source to the target machine."),
  N_("Songs can be recoderd as single waves per track to give it to remixers."),
  N_("Fill the details on the info page. When recording songs, the metadata is added to the recording as tags."),
  N_("Use jackaudio sink in audio device settings to get lower latencies for live machine control."),
  N_("You can use input devices such as joysticks, beside midi devices to live control machine parameters."),
  N_("You can use a upnp media client (e.g. media streamer on nokia tablets) to remote control buzztard."),
  N_("To enter notes, imagine your pc keyboard as a music keyboard in two rows. Bottom left y/z key becomes a 'c', s a 'c#', x a 'd' and so on."),
  N_("You can get more help from the community on irc://irc.freenode.net/#buzztard.")
};

enum {
  TIP_DIALOG_APP=1
};

struct _BtTipDialogPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  BtSettings *settings;
  GtkTextView *tip_view;
  
  /* tip history */
  gint tip_status[G_N_ELEMENTS(tips)];
  gint pending_tips[G_N_ELEMENTS(tips)];
  gint n_pending_tips;

  /* the application */
  G_POINTER_ALIAS(BtEditApplication *,app);
};

static GtkDialogClass *parent_class=NULL;

//-- event handler

static void on_refresh_clicked(GtkButton *button, gpointer user_data) {
  BtTipDialog *self = BT_TIP_DIALOG(user_data);
  guint ix=g_random_int_range(0,G_N_ELEMENTS(tips));
  gint i,j;

  if(!self->priv->n_pending_tips) {
    // reset shown tips
    self->priv->n_pending_tips=G_N_ELEMENTS(tips);
    for(i=0;i<G_N_ELEMENTS(tips);i++) {
      self->priv->tip_status[i]=i;
      self->priv->pending_tips[i]=i;
    }
  }
  // get a tip index we haven't shown yet
  ix=self->priv->pending_tips[g_random_int_range(0,self->priv->n_pending_tips)];

  GST_DEBUG("show [%d]",ix);
  self->priv->tip_status[ix]=-1;
  for(i=j=0;i<G_N_ELEMENTS(tips);i++) {
    if(self->priv->tip_status[i]>-1)
      self->priv->pending_tips[j++]=i;
  }
  self->priv->n_pending_tips=j;
  gtk_text_buffer_set_text(gtk_text_view_get_buffer(self->priv->tip_view),tips[ix],-1);
}

static void on_show_tips_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
  BtTipDialog *self = BT_TIP_DIALOG(user_data);

  g_object_set(self->priv->settings,"show-tips",gtk_toggle_button_get_active(togglebutton),NULL);
}

//-- helper methods

static void bt_tip_dialog_init_ui(const BtTipDialog *self) {
  GtkWidget *label,*icon,*hbox,*vbox,*btn,*chk,*tip_view;
  gboolean show_tips;
  gchar *str;
  gint i,j;

  GST_DEBUG("read settings");

  g_object_get(G_OBJECT(self->priv->app),"settings",&self->priv->settings,NULL);
  g_object_get(self->priv->settings,"show-tips",&show_tips,"presented-tips",&str,NULL);
  GST_DEBUG("read [%s]",str);
  
  // parse str to update tip status
  for(i=0;i<G_N_ELEMENTS(tips);i++) {
    self->priv->tip_status[i]=i;
  }
  if(str) {
    gint ix;
    gchar *p1,*p2;
    
    p1=str;
    p2=strchr(p1,',');
    while(p2) {
      *p2='\0';
      ix=atoi(p1);
      if(ix<G_N_ELEMENTS(tips))
        self->priv->tip_status[ix]=-1;
      p1=&p2[1];
      p2=strchr(p1,',');
    }
    ix=atoi(p1);
    if(ix<G_N_ELEMENTS(tips))
      self->priv->tip_status[ix]=-1;
    g_free(str);
  }
  for(i=j=0;i<G_N_ELEMENTS(tips);i++) {
    if(self->priv->tip_status[i]>-1)
      self->priv->pending_tips[j++]=i;
  }
  self->priv->n_pending_tips=j;

  GST_DEBUG("prepare tips dialog");

  gtk_widget_set_name(GTK_WIDGET(self),"tip of the day");

  gtk_window_set_title(GTK_WINDOW(self),_("tip of the day"));

  // add dialog commision widgets (okay)
  gtk_dialog_add_buttons(GTK_DIALOG(self),
                          GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                          NULL);

  // content area
  hbox=gtk_hbox_new(FALSE,12);
  
  icon=gtk_image_new_from_stock(GTK_STOCK_DIALOG_INFO,GTK_ICON_SIZE_DIALOG);
  gtk_box_pack_start(GTK_BOX(hbox),icon,FALSE,FALSE,0);
  
  vbox=gtk_vbox_new(FALSE,6);
  str=g_strdup_printf("<big><b>%s</b></big>\n",_("tip of the day"));
  label=g_object_new(GTK_TYPE_LABEL,"use-markup",TRUE,"label",str,NULL);
  g_free(str);
  gtk_box_pack_start(GTK_BOX(vbox),label,FALSE,FALSE,0);

  self->priv->tip_view=GTK_TEXT_VIEW(gtk_text_view_new());
  gtk_text_view_set_cursor_visible(self->priv->tip_view, FALSE);
  gtk_text_view_set_editable(self->priv->tip_view, FALSE);
  gtk_text_view_set_wrap_mode(self->priv->tip_view, GTK_WRAP_WORD);
  
  tip_view = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(tip_view), GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(tip_view), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(tip_view),GTK_WIDGET(self->priv->tip_view));
  
  gtk_box_pack_start(GTK_BOX(vbox),tip_view,TRUE,TRUE,0);

  chk=gtk_check_button_new_with_label(_("Show tips on startup"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chk),show_tips);
  g_signal_connect(chk, "toggled", G_CALLBACK (on_show_tips_toggled), (gpointer)self);
  gtk_box_pack_start(GTK_BOX(vbox),chk,FALSE,FALSE,0);

  gtk_box_pack_start(GTK_BOX(hbox),vbox,TRUE,TRUE,0);
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(self))),hbox,TRUE,TRUE,0);
  
  // add "refresh" button to action area
  btn=gtk_button_new_from_stock(GTK_STOCK_REFRESH);
  g_signal_connect(G_OBJECT(btn), "clicked", G_CALLBACK(on_refresh_clicked), (gpointer)self);
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_action_area(GTK_DIALOG(self))),btn,FALSE,FALSE,0);

  gtk_dialog_set_has_separator(GTK_DIALOG(self),TRUE);

  on_refresh_clicked(GTK_BUTTON(btn),(gpointer)self);
}

//-- constructor methods

/**
 * bt_tip_dialog_new:
 * @app: the application the dialog belongs to
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtTipDialog *bt_tip_dialog_new(const BtEditApplication *app) {
  BtTipDialog *self;

  self=BT_TIP_DIALOG(g_object_new(BT_TYPE_TIP_DIALOG,"app",app,NULL));
  bt_tip_dialog_init_ui(self);
  return(self);
}

//-- methods

//-- wrapper

//-- class internals

static void bt_tip_dialog_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec) {
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
  gchar *str;
  gint i,j,shown;

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;
  
  // update tip-status in settings
  shown=G_N_ELEMENTS(tips)-self->priv->n_pending_tips;
  str=g_malloc(6*shown);
  for(i=j=0;i<G_N_ELEMENTS(tips);i++) {
    if(self->priv->tip_status[i]==-1) {
      j+=g_sprintf(&str[j],"%d,",i);
    }
  }
  if(j) {
    str[j-1]='\0';
  }
  GST_DEBUG("write [%s]",str);
  g_object_set(self->priv->settings,"presented-tips",str,NULL);
  g_free(str);

  GST_DEBUG("!!!! self=%p",self);
  g_object_try_weak_unref(self->priv->app);
  g_object_unref(self->priv->settings);

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
