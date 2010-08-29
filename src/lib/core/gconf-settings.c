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
 * SECTION:btgconfsettings
 * @short_description: gconf based implementation sub class for buzztard
 * settings handling
 *
 * GConf is the standart mechanism used in GNOME to handle persistance of
 * application settings and status.
 */

#define BT_CORE
#define BT_GCONF_SETTINGS_C

#include "core_private.h"
#include <libbuzztard-core/settings-private.h>

struct _BtGConfSettingsPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* that is the handle we get config data from */
  GConfClient *client;
  
  /* flags if there was a write */
  gboolean dirty;
};

#define BT_GCONF_PATH_GSTREAMER "/system/gstreamer/default"
#define BT_GCONF_PATH_GNOME "/desktop/gnome/interface"
#define BT_GCONF_PATH_BUZZTARD "/apps/"PACKAGE

//-- the class

G_DEFINE_TYPE (BtGConfSettings, bt_gconf_settings, BT_TYPE_SETTINGS);

//-- event handler

static void bt_gconf_settings_notify_toolbar_style(GConfClient * const client, guint cnxn_id, GConfEntry  * const entry, gpointer const user_data) {
  BtGConfSettings *self=BT_GCONF_SETTINGS(user_data);

  GST_INFO("!!!  gconf notify for toolbar style");

  g_object_notify(G_OBJECT(self),"toolbar-style");
}

//-- helper

static void read_boolean(const BtGConfSettings * const self, const gchar *path, GValue * const value) {
  gboolean prop=gconf_client_get_bool(self->priv->client,path,NULL);
  GST_DEBUG("application reads '%s' : '%d'",path,prop);
  g_value_set_boolean(value,prop);
}

#if 0
static void read_int(const BtGConfSettings * const self, const gchar *path, GValue * const value) {
  guint prop=gconf_client_get_int(self->priv->client,path,NULL);
  GST_DEBUG("application reads '%s' : '%u'",path,prop);
  g_value_set_uint(value,prop);
}
#endif

static void read_int_def(const BtGConfSettings * const self, const gchar *path, GValue * const value, GParamSpecInt * const pspec) {
  gint prop=gconf_client_get_int(self->priv->client,path,NULL);
  if(prop) {
    GST_DEBUG("application reads '%s' : '%i'",path,prop);
    g_value_set_int(value,prop);
  }
  else {
    GST_DEBUG("application reads [def] '%s' : '%i'",path,pspec->default_value);
    g_value_set_int(value,pspec->default_value);
  }
}

static void read_uint(const BtGConfSettings * const self, const gchar *path, GValue * const value) {
  guint prop=gconf_client_get_int(self->priv->client,path,NULL);
  GST_DEBUG("application reads '%s' : '%u'",path,prop);
  g_value_set_uint(value,prop);
}

static void read_uint_def(const BtGConfSettings * const self, const gchar *path, GValue * const value, GParamSpecUInt * const pspec) {
  guint prop=gconf_client_get_int(self->priv->client,path,NULL);
  if(prop) {
    GST_DEBUG("application reads '%s' : '%u'",path,prop);
    g_value_set_uint(value,prop);
  }
  else {
    GST_DEBUG("application reads [def] '%s' : '%u'",path,pspec->default_value);
    g_value_set_uint(value,pspec->default_value);
  }
}

static void read_string(const BtGConfSettings * const self, const gchar *path, GValue * const value) {
  gchar * const prop=gconf_client_get_string(self->priv->client,path,NULL);
  GST_DEBUG("application reads '%s' : '%s'",path,prop);
  g_value_set_string(value,prop);
  g_free(prop);
}

static void read_string_def(const BtGConfSettings * const self, const gchar *path, GValue * const value, GParamSpecString * const pspec) {
  gchar * const prop=gconf_client_get_string(self->priv->client,path,NULL);
  if(prop) {
    GST_DEBUG("application reads '%s' : '%s'",path,prop);
    g_value_set_string(value,prop);
    g_free(prop);
  }
  else {
    GST_DEBUG("application reads [def] '%s' : '%s'",path,pspec->default_value);
    g_value_set_string(value,pspec->default_value);
  }
}


static void write_boolean(const BtGConfSettings * const self, const gchar *path, const GValue * const value) {
  gboolean prop=g_value_get_boolean(value);
  gboolean res=gconf_client_set_bool(self->priv->client,path,prop,NULL);
  GST_DEBUG("application wrote '%s' : '%d' (%s)",path,prop,(res?"okay":"fail"));
}

static void write_int(const BtGConfSettings * const self, const gchar *path, const GValue * const value) {
  gint prop=g_value_get_int(value);
  gboolean res=gconf_client_set_int(self->priv->client,path,prop,NULL);
  GST_DEBUG("application wrote '%s' : '%i' (%s)",path,prop,(res?"okay":"fail"));
}

static void write_uint(const BtGConfSettings * const self, const gchar *path, const GValue * const value) {
  guint prop=g_value_get_uint(value);
  gboolean res=gconf_client_set_int(self->priv->client,path,prop,NULL);
  GST_DEBUG("application wrote '%s' : '%u' (%s)",path,prop,(res?"okay":"fail"));
}

static void write_string(const BtGConfSettings * const self, const gchar *path, const GValue * const value) {
  const gchar *prop=g_value_get_string(value);
  gboolean res=gconf_client_set_string(self->priv->client,path,prop,NULL);
  GST_DEBUG("application wrote '%s' : '%s' (%s)",path,prop,(res?"okay":"fail"));
}

//-- constructor methods

/**
 * bt_gconf_settings_new:
 *
 * Create a new instance.
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtGConfSettings *bt_gconf_settings_new(void) {
  return(BT_GCONF_SETTINGS(g_object_new(BT_TYPE_GCONF_SETTINGS,NULL)));
}

//-- methods

//-- wrapper

//-- g_object overrides

/* returns a property for the given property_id for this object */
static void bt_gconf_settings_get_property(GObject * const object, const guint property_id, GValue * const value, GParamSpec * const pspec) {
  const BtGConfSettings * const self = BT_GCONF_SETTINGS(object);

  return_if_disposed();
  switch (property_id) {
    /* ui */
    case BT_SETTINGS_NEWS_SEEN: {
      read_uint(self,BT_GCONF_PATH_BUZZTARD"/news-seen",value);
    } break;
    case BT_SETTINGS_MISSING_MACHINES: {
      read_string(self,BT_GCONF_PATH_BUZZTARD"/missing-machines",value);
    } break;
    case BT_SETTINGS_PRESENTED_TIPS: {
      read_string(self,BT_GCONF_PATH_BUZZTARD"/presented-tips",value);
    } break;
    case BT_SETTINGS_SHOW_TIPS: {
      read_boolean(self,BT_GCONF_PATH_BUZZTARD"/show-tips",value);
    } break;
    case BT_SETTINGS_MENU_TOOLBAR_HIDE: {
      read_boolean(self,BT_GCONF_PATH_BUZZTARD"/toolbar-hide",value);
    } break;
    case BT_SETTINGS_MENU_STATUSBAR_HIDE: {
      read_boolean(self,BT_GCONF_PATH_BUZZTARD"/statusbar-hide",value);
    } break;
    case BT_SETTINGS_MENU_TABS_HIDE: {
      read_boolean(self,BT_GCONF_PATH_BUZZTARD"/tabs-hide",value);
    } break;
    case BT_SETTINGS_MACHINE_VIEW_GRID_DENSITY: {
      read_string_def(self,BT_GCONF_PATH_BUZZTARD"/grid-density",value,(GParamSpecString *)pspec);
    } break;
    case BT_SETTINGS_WINDOW_XPOS: {
      read_int_def(self,BT_GCONF_PATH_BUZZTARD"/window/x-pos",value,(GParamSpecInt *)pspec);
    } break;
    case BT_SETTINGS_WINDOW_YPOS: {
      read_int_def(self,BT_GCONF_PATH_BUZZTARD"/window/y-pos",value,(GParamSpecInt *)pspec);
    } break;
    case BT_SETTINGS_WINDOW_WIDTH: {
      read_int_def(self,BT_GCONF_PATH_BUZZTARD"/window/width",value,(GParamSpecInt *)pspec);
    } break;
    case BT_SETTINGS_WINDOW_HEIGHT: {
      read_int_def(self,BT_GCONF_PATH_BUZZTARD"/window/height",value,(GParamSpecInt *)pspec);
    } break;
    /* audio settings */
    case BT_SETTINGS_AUDIOSINK: {
      read_string_def(self,BT_GCONF_PATH_BUZZTARD"/audiosink",value,(GParamSpecString *)pspec);
    } break;
    case BT_SETTINGS_SAMPLE_RATE: {
      read_uint_def(self,BT_GCONF_PATH_BUZZTARD"/sample-rate",value,(GParamSpecUInt *)pspec);
    } break;
    case BT_SETTINGS_CHANNELS: {
      read_uint_def(self,BT_GCONF_PATH_BUZZTARD"/channels",value,(GParamSpecUInt *)pspec);
    } break;
    /* playback controller */
    case BT_SETTINGS_PLAYBACK_CONTROLLER_COHERENCE_UPNP_ACTIVE: {
      read_boolean(self,BT_GCONF_PATH_BUZZTARD"/playback-controller/coherence-upnp-active",value);
    } break;
    case BT_SETTINGS_PLAYBACK_CONTROLLER_COHERENCE_UPNP_PORT: {
      read_uint(self,BT_GCONF_PATH_BUZZTARD"/playback-controller/coherence-upnp-port",value);
    } break;
    /* directory settings */
    case BT_SETTINGS_FOLDER_SONG: {
      read_string_def(self,BT_GCONF_PATH_BUZZTARD"/song-folder",value,(GParamSpecString *)pspec);
    } break;
    case BT_SETTINGS_FOLDER_RECORD: {
      read_string_def(self,BT_GCONF_PATH_BUZZTARD"/record-folder",value,(GParamSpecString *)pspec);
    } break;
    case BT_SETTINGS_FOLDER_SAMPLE: {
      read_string_def(self,BT_GCONF_PATH_BUZZTARD"/sample-folder",value,(GParamSpecString *)pspec);
    } break;
    /* system settings */
    case BT_SETTINGS_SYSTEM_AUDIOSINK: {
      read_string(self,BT_GCONF_PATH_GSTREAMER"/audiosink",value);
    } break;
    case BT_SETTINGS_SYSTEM_TOOLBAR_STYLE: {
      read_string(self,BT_GCONF_PATH_GNOME"/toolbar_style",value);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_gconf_settings_set_property(GObject * const object, const guint property_id, const GValue * const value, GParamSpec * const pspec) {
  const BtGConfSettings * const self = BT_GCONF_SETTINGS(object);

  return_if_disposed();

  self->priv->dirty=TRUE;

  switch (property_id) {
    /* ui */
    case BT_SETTINGS_NEWS_SEEN: {
      write_uint(self,BT_GCONF_PATH_BUZZTARD"/news-seen",value);
    } break;
    case BT_SETTINGS_MISSING_MACHINES: {
      write_string(self,BT_GCONF_PATH_BUZZTARD"/missing-machines",value);
    } break;
    case BT_SETTINGS_PRESENTED_TIPS: {
      write_string(self,BT_GCONF_PATH_BUZZTARD"/presented-tips",value);
    } break;
    case BT_SETTINGS_SHOW_TIPS: {
      write_boolean(self,BT_GCONF_PATH_BUZZTARD"/show-tips",value);
    } break;
    case BT_SETTINGS_MENU_TOOLBAR_HIDE: {
      write_boolean(self,BT_GCONF_PATH_BUZZTARD"/toolbar-hide",value);
    } break;
    case BT_SETTINGS_MENU_STATUSBAR_HIDE: {
      write_boolean(self,BT_GCONF_PATH_BUZZTARD"/statusbar-hide",value);
    } break;
    case BT_SETTINGS_MENU_TABS_HIDE: {
      write_boolean(self,BT_GCONF_PATH_BUZZTARD"/tabs-hide",value);
    } break;
    case BT_SETTINGS_MACHINE_VIEW_GRID_DENSITY: {
      write_string(self,BT_GCONF_PATH_BUZZTARD"/grid-density",value);
    } break;
    case BT_SETTINGS_WINDOW_XPOS: {
      write_int(self,BT_GCONF_PATH_BUZZTARD"/window/x-pos",value);
    } break;
    case BT_SETTINGS_WINDOW_YPOS: {
      write_int(self,BT_GCONF_PATH_BUZZTARD"/window/y-pos",value);
    } break;
    case BT_SETTINGS_WINDOW_WIDTH: {
      write_int(self,BT_GCONF_PATH_BUZZTARD"/window/width",value);
    } break;
    case BT_SETTINGS_WINDOW_HEIGHT: {
      write_int(self,BT_GCONF_PATH_BUZZTARD"/window/height",value);
    } break;
    /* audio settings */
    case BT_SETTINGS_AUDIOSINK: {
      write_string(self,BT_GCONF_PATH_BUZZTARD"/audiosink",value);
    } break;
    case BT_SETTINGS_SAMPLE_RATE: {
      write_uint(self,BT_GCONF_PATH_BUZZTARD"/sample-rate",value);
    } break;
    case BT_SETTINGS_CHANNELS: {
      write_uint(self,BT_GCONF_PATH_BUZZTARD"/channels",value);
    } break;
    /* playback controller */
    case BT_SETTINGS_PLAYBACK_CONTROLLER_COHERENCE_UPNP_ACTIVE: {
      write_boolean(self,BT_GCONF_PATH_BUZZTARD"/playback-controller/coherence-upnp-active",value);
    } break;
    case BT_SETTINGS_PLAYBACK_CONTROLLER_COHERENCE_UPNP_PORT: {
      write_uint(self,BT_GCONF_PATH_BUZZTARD"/playback-controller/coherence-upnp-port",value);
    } break;
    /* directory settings */
    case BT_SETTINGS_FOLDER_SONG: {
      write_string(self,BT_GCONF_PATH_BUZZTARD"/song-folder",value);
    } break;
    case BT_SETTINGS_FOLDER_RECORD: {
      write_string(self,BT_GCONF_PATH_BUZZTARD"/record-folder",value);
    } break;
    case BT_SETTINGS_FOLDER_SAMPLE: {
      write_string(self,BT_GCONF_PATH_BUZZTARD"/sample-folder",value);
    } break;
    /* system settings */
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_gconf_settings_dispose(GObject * const object) {
  const BtGConfSettings * const self = BT_GCONF_SETTINGS(object);

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  // unregister directories to watch
  gconf_client_remove_dir(self->priv->client,BT_GCONF_PATH_GSTREAMER,NULL);
  gconf_client_remove_dir(self->priv->client,BT_GCONF_PATH_GNOME,NULL);
  gconf_client_remove_dir(self->priv->client,BT_GCONF_PATH_BUZZTARD,NULL);
  // shutdown gconf client
  if(self->priv->dirty) {
    // only do this if we have written something
    gconf_client_suggest_sync(self->priv->client,NULL);
  }
  g_object_unref(self->priv->client);

  GST_DEBUG("!!!! self=%p",self);
  G_OBJECT_CLASS(bt_gconf_settings_parent_class)->dispose(object);
}

//-- class internals

static void bt_gconf_settings_init(BtGConfSettings * self) {
  GError *error=NULL;

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_GCONF_SETTINGS, BtGConfSettingsPrivate);
  GST_DEBUG("!!!! self=%p",self);

  self->priv->client=gconf_client_get_default();
  gconf_client_set_error_handling(self->priv->client,GCONF_CLIENT_HANDLE_UNRETURNED);
  // register the config cache
  GST_DEBUG("listening to settings");
  gconf_client_add_dir(self->priv->client,BT_GCONF_PATH_GSTREAMER,GCONF_CLIENT_PRELOAD_ONELEVEL,NULL);
  gconf_client_add_dir(self->priv->client,BT_GCONF_PATH_GNOME,GCONF_CLIENT_PRELOAD_ONELEVEL,NULL);
  gconf_client_add_dir(self->priv->client,BT_GCONF_PATH_BUZZTARD,GCONF_CLIENT_PRELOAD_RECURSIVE,NULL);
  
  GST_DEBUG("about to register gconf notify handler");
  // register notify handlers for some properties
  gconf_client_notify_add(self->priv->client,
         BT_GCONF_PATH_GNOME"/toolbar_style",
         bt_gconf_settings_notify_toolbar_style,
         (gpointer)self, NULL, &error);
  if(error) {
    GST_WARNING("can't listen to notifies on %s: %s",BT_GCONF_PATH_GNOME"/toolbar_style",error->message);
    g_error_free(error);
  }
  /* @todo: also listen to BT_GCONF_PATH_GSTREAMER"/audiosink" */
}

static void bt_gconf_settings_class_init(BtGConfSettingsClass * const klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);

  g_type_class_add_private(klass,sizeof(BtGConfSettingsPrivate));

  gobject_class->set_property = bt_gconf_settings_set_property;
  gobject_class->get_property = bt_gconf_settings_get_property;
  gobject_class->dispose      = bt_gconf_settings_dispose;
}

