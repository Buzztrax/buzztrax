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
 * SECTION:btsettingspageplaybackcontroller
 * @short_description: playback controller configuration settings page
 *
 * Lists available playback controllers and allows to configure them.
 */

#define BT_EDIT
#define BT_SETTINGS_PAGE_PLAYBACK_CONTROLLER_C

#include "bt-edit.h"

enum {
  SETTINGS_PAGE_PLAYBACK_CONTROLLER_APP=1,
};

enum {
  //DEVICE_MENU_ICON=0,
  DEVICE_MENU_LABEL=0,
  DEVICE_MENU_DEVICE
};

struct _BtSettingsPagePlaybackControllerPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  G_POINTER_ALIAS(BtEditApplication *,app);

  GtkWidget *port_entry;
};

static GtkDialogClass *parent_class=NULL;

//-- event handler

static void on_activate_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
  BtSettingsPagePlaybackController *self=BT_SETTINGS_PAGE_PLAYBACK_CONTROLLER(user_data);
  BtSettings *settings;
  gboolean active=gtk_toggle_button_get_active(togglebutton);
  gboolean active_setting;

  g_assert(user_data);

  g_object_get(G_OBJECT(self->priv->app),"settings",&settings,NULL);
  g_object_get(G_OBJECT(settings),"coherence-upnp-active",&active_setting,NULL);
  GST_INFO("upnp active changes from %d -> %d",active_setting,active);
  if(active!=active_setting) {
    g_object_set(G_OBJECT(settings),"coherence-upnp-active",active,NULL);
  }
  gtk_widget_set_sensitive(self->priv->port_entry,active);
  g_object_unref(settings);
}

static void on_port_changed(GtkSpinButton *spinbutton,gpointer user_data) {
  BtSettingsPagePlaybackController *self=BT_SETTINGS_PAGE_PLAYBACK_CONTROLLER(user_data);
  BtSettings *settings;

  g_assert(user_data);

  g_object_get(G_OBJECT(self->priv->app),"settings",&settings,NULL);
  g_object_set(G_OBJECT(settings),"coherence-upnp-port",gtk_spin_button_get_value_as_int(spinbutton),NULL);
  g_object_unref(settings);
}

//-- helper methods

static gboolean bt_settings_page_playback_controller_init_ui(const BtSettingsPagePlaybackController *self) {
  BtSettings *settings;
  GtkWidget *label,*spacer,*widget;
  GtkAdjustment *spin_adjustment;
  gboolean active;
  guint port;
  gchar *str;

  gtk_widget_set_name(GTK_WIDGET(self),"playback controller settings");

  // get settings
  g_object_get(G_OBJECT(self->priv->app),"settings",&settings,NULL);
  g_object_get(G_OBJECT(settings),"coherence-upnp-active",&active,"coherence-upnp-port",&port,NULL);

  // add setting widgets
  spacer=gtk_label_new("    ");
  label=gtk_label_new(NULL);
  str=g_strdup_printf("<big><b>%s</b></big>",_("Playback Controller"));
  gtk_label_set_markup(GTK_LABEL(label),str);
  g_free(str);
  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
  gtk_table_attach(GTK_TABLE(self),label, 0, 3, 0, 1,  GTK_FILL|GTK_EXPAND, GTK_SHRINK, 2,1);
  gtk_table_attach(GTK_TABLE(self),spacer, 0, 1, 1, 4, GTK_SHRINK,GTK_SHRINK, 2,1);

  // don't translate, this is a product
  label=gtk_label_new("Coherence UPnP");
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(self),label, 1, 2, 1, 2, GTK_SHRINK,GTK_SHRINK, 2,1);

  widget=gtk_check_button_new();
  gtk_table_attach(GTK_TABLE(self),widget, 2, 3, 1, 2, GTK_SHRINK,GTK_SHRINK, 2,1);

  // local network port number for socket communication
  label=gtk_label_new(_("Port number"));
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(self),label, 1, 2, 2, 3, GTK_SHRINK,GTK_SHRINK, 2,1);

  spin_adjustment=GTK_ADJUSTMENT(gtk_adjustment_new((gdouble)port, 1024.0, 65536.0, 1.0, 5.0, 0.0));
  self->priv->port_entry=gtk_spin_button_new(spin_adjustment,1.0,0);
  gtk_table_attach(GTK_TABLE(self),self->priv->port_entry, 2, 3, 2, 3, GTK_SHRINK,GTK_SHRINK, 2,1);

  // add coherence URL
  label=gtk_label_new("Requires Coherence UPnP framework which can be found at: https://coherence.beebits.net.");
  gtk_label_set_line_wrap(GTK_LABEL(label),TRUE);
  gtk_label_set_selectable(GTK_LABEL(label),TRUE);
  gtk_table_attach(GTK_TABLE(self),label, 1, 3, 3, 4, GTK_SHRINK,GTK_SHRINK, 2,1);

  // set current settings
  g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(on_activate_toggled), (gpointer)self);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),active);
  g_signal_connect(G_OBJECT(self->priv->port_entry), "value-changed", G_CALLBACK(on_port_changed), (gpointer)self);

  g_object_unref(settings);
  return(TRUE);
}

//-- constructor methods

/**
 * bt_settings_page_playback_controller_new:
 * @app: the application the dialog belongs to
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtSettingsPagePlaybackController *bt_settings_page_playback_controller_new(const BtEditApplication *app) {
  BtSettingsPagePlaybackController *self;

  if(!(self=BT_SETTINGS_PAGE_PLAYBACK_CONTROLLER(g_object_new(BT_TYPE_SETTINGS_PAGE_PLAYBACK_CONTROLLER,
    "app",app,
    "n-rows",4,
    "n-columns",3,
    "homogeneous",FALSE,
    NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_settings_page_playback_controller_init_ui(self)) {
    goto Error;
  }
  gtk_widget_show_all(GTK_WIDGET(self));
  return(self);
Error:
  if(self) gtk_object_destroy(GTK_OBJECT(self));
  return(NULL);
}

//-- methods

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_settings_page_playback_controller_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtSettingsPagePlaybackController *self = BT_SETTINGS_PAGE_PLAYBACK_CONTROLLER(object);
  return_if_disposed();
  switch (property_id) {
    case SETTINGS_PAGE_PLAYBACK_CONTROLLER_APP: {
      g_value_set_object(value, self->priv->app);
    } break;
    default: {
       G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_settings_page_playback_controller_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtSettingsPagePlaybackController *self = BT_SETTINGS_PAGE_PLAYBACK_CONTROLLER(object);
  return_if_disposed();
  switch (property_id) {
    case SETTINGS_PAGE_PLAYBACK_CONTROLLER_APP: {
      g_object_try_weak_unref(self->priv->app);
      self->priv->app = BT_EDIT_APPLICATION(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->app);
      //GST_DEBUG("set the app for settings_page_playback_controller: %p",self->priv->app);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_settings_page_playback_controller_dispose(GObject *object) {
  BtSettingsPagePlaybackController *self = BT_SETTINGS_PAGE_PLAYBACK_CONTROLLER(object);

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);

  g_object_try_weak_unref(self->priv->app);

  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_settings_page_playback_controller_finalize(GObject *object) {
  BtSettingsPagePlaybackController *self = BT_SETTINGS_PAGE_PLAYBACK_CONTROLLER(object);

  GST_DEBUG("!!!! self=%p",self);

  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void bt_settings_page_playback_controller_init(GTypeInstance *instance, gpointer g_class) {
  BtSettingsPagePlaybackController *self = BT_SETTINGS_PAGE_PLAYBACK_CONTROLLER(instance);

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_SETTINGS_PAGE_PLAYBACK_CONTROLLER, BtSettingsPagePlaybackControllerPrivate);
}

static void bt_settings_page_playback_controller_class_init(BtSettingsPagePlaybackControllerClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtSettingsPagePlaybackControllerPrivate));

  gobject_class->set_property = bt_settings_page_playback_controller_set_property;
  gobject_class->get_property = bt_settings_page_playback_controller_get_property;
  gobject_class->dispose      = bt_settings_page_playback_controller_dispose;
  gobject_class->finalize     = bt_settings_page_playback_controller_finalize;

  g_object_class_install_property(gobject_class,SETTINGS_PAGE_PLAYBACK_CONTROLLER_APP,
                                  g_param_spec_object("app",
                                     "app construct prop",
                                     "Set application object, the dialog belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

}

GType bt_settings_page_playback_controller_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      sizeof(BtSettingsPagePlaybackControllerClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_settings_page_playback_controller_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof(BtSettingsPagePlaybackController),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_settings_page_playback_controller_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(GTK_TYPE_TABLE,"BtSettingsPagePlaybackController",&info,0);
  }
  return type;
}
