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
};

static BtSettingsClass *parent_class=NULL;

#define BT_GCONF_PATH_GSTREAMER "/system/gstreamer/default"
#define BT_GCONF_PATH_GNOME "/desktop/gnome/interface"
#define BT_GCONF_PATH_BUZZTARD "/apps/"PACKAGE

//-- event handler

static void bt_gconf_settings_notify_toolbar_style(GConfClient * const client, guint cnxn_id, GConfEntry  * const entry, gpointer const user_data) {
  BtGConfSettings *self=BT_GCONF_SETTINGS(user_data);

  GST_INFO("!!!  gconf notify for toolbar style");

  g_object_notify(G_OBJECT(self),"toolbar-style");
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
  BtGConfSettings * const self = BT_GCONF_SETTINGS(g_object_new(BT_TYPE_GCONF_SETTINGS,NULL));
  GError *error=NULL;

  if(!self) {
    goto Error;
  }

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

  return(self);
Error:
  g_object_try_unref(self);
  return(NULL);
}

//-- methods

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_gconf_settings_get_property(GObject      * const object,
                               const guint         property_id,
                               GValue       * const value,
                               GParamSpec   * const pspec)
{
  const BtGConfSettings * const self = BT_GCONF_SETTINGS(object);

  return_if_disposed();
  switch (property_id) {
    case BT_SETTINGS_AUDIOSINK: {
      gchar * const prop=gconf_client_get_string(self->priv->client,BT_GCONF_PATH_BUZZTARD"/audiosink",NULL);
      if(prop) {
        GST_DEBUG("application reads audiosink gconf_settings : '%s'",prop);
        g_value_set_string(value, prop);
        g_free(prop);
      }
      else {
        GST_DEBUG("application reads [def] audiosink gconf_settings : '%s'",((GParamSpecString *)pspec)->default_value);
        g_value_set_string(value, ((GParamSpecString *)pspec)->default_value);
      }
    } break;
    case BT_SETTINGS_SAMPLE_RATE: {
      guint prop=gconf_client_get_int(self->priv->client,BT_GCONF_PATH_BUZZTARD"/sample-rate",NULL);
      if(!prop) {
        GST_DEBUG("application reads [def] sample-rate gconf_settings : '%u'",((GParamSpecUInt *)pspec)->default_value);
        g_value_set_uint(value, ((GParamSpecUInt *)pspec)->default_value);
      }
      else {
        GST_DEBUG("application reads sample-rate gconf_settings : '%u'",prop);
        g_value_set_uint(value, prop);
      }
    } break;
    case BT_SETTINGS_CHANNELS: {
      guint prop=gconf_client_get_int(self->priv->client,BT_GCONF_PATH_BUZZTARD"/channels",NULL);
      if(!prop) {
        GST_DEBUG("application reads [def] channels gconf_settings : '%u'",((GParamSpecUInt *)pspec)->default_value);
        g_value_set_uint(value, ((GParamSpecUInt *)pspec)->default_value);
      }
      else {
        GST_DEBUG("application reads channels gconf_settings : '%u'",prop);
        g_value_set_uint(value, prop);
      }
    } break;
    case BT_SETTINGS_MENU_TOOLBAR_HIDE: {
      gboolean prop=gconf_client_get_bool(self->priv->client,BT_GCONF_PATH_BUZZTARD"/toolbar-hide",NULL);
      GST_DEBUG("application reads system toolbar-hide gconf_settings : %d",prop);
      g_value_set_boolean(value, prop);
    } break;
    case BT_SETTINGS_MENU_STATUSBAR_HIDE: {
      gboolean prop=gconf_client_get_bool(self->priv->client,BT_GCONF_PATH_BUZZTARD"/statusbar-hide",NULL);
      GST_DEBUG("application reads system statusbar-hide gconf_settings : %d",prop);
      g_value_set_boolean(value, prop);
    } break;
    case BT_SETTINGS_MENU_TABS_HIDE: {
      gboolean prop=gconf_client_get_bool(self->priv->client,BT_GCONF_PATH_BUZZTARD"/tabs-hide",NULL);
      GST_DEBUG("application reads tabs-hide gconf_settings : %d",prop);
      g_value_set_boolean(value, prop);
    } break;
    case BT_SETTINGS_MACHINE_VIEW_GRID_DENSITY: {
      gchar * const prop=gconf_client_get_string(self->priv->client,BT_GCONF_PATH_BUZZTARD"/grid-density",NULL);
      if(prop) {
        GST_DEBUG("application reads grid-density gconf_settings : '%s'",prop);
        g_value_set_string(value, prop);
        g_free(prop);
      }
      else {
        GST_DEBUG("application reads [def] grid-density gconf_settings : '%s'",((GParamSpecString *)pspec)->default_value);
        g_value_set_string(value, ((GParamSpecString *)pspec)->default_value);
      }
    } break;
    case BT_SETTINGS_NEWS_SEEN: {
      guint prop=gconf_client_get_int(self->priv->client,BT_GCONF_PATH_BUZZTARD"/news-seen",NULL);
      GST_DEBUG("application reads news-seen gconf_settings : '%u'",prop);
      g_value_set_uint(value, prop);
    } break;
    case BT_SETTINGS_MISSING_MACHINES: {
      gchar * const prop=gconf_client_get_string(self->priv->client,BT_GCONF_PATH_BUZZTARD"/missing-machines",NULL);
      GST_DEBUG("application reads missing-machines gconf_settings : '%s'",prop);
      g_value_set_string(value, prop);
      g_free(prop);
    } break;
    /* playback controller */
    case BT_SETTINGS_PLAYBACK_CONTROLLER_COHERENCE_UPNP_ACTIVE: {
      gboolean prop=gconf_client_get_bool(self->priv->client,BT_GCONF_PATH_BUZZTARD"/playback-controller/coherence-upnp-active",NULL);
      GST_DEBUG("application reads playback-controller/coherence-upnp-activee gconf_settings : %d",prop);
      g_value_set_boolean(value, prop);
    } break;
    case BT_SETTINGS_PLAYBACK_CONTROLLER_COHERENCE_UPNP_PORT: {
      guint prop=gconf_client_get_int(self->priv->client,BT_GCONF_PATH_BUZZTARD"/playback-controller/coherence-upnp-port",NULL);
      GST_DEBUG("application reads playback-controller/coherence-upnp-port gconf_settings : '%u'",prop);
      g_value_set_uint(value, prop);
    } break;
    /* directory settings */
    case BT_SETTINGS_FOLDER_SONG: {
      gchar * const prop=gconf_client_get_string(self->priv->client,BT_GCONF_PATH_BUZZTARD"/song-folder",NULL);
      if(prop && g_file_test(prop,G_FILE_TEST_IS_DIR)) {
        GST_DEBUG("application reads song-folder gconf_settings : '%s'",prop);
        g_value_set_string(value, prop);
        g_free(prop);
      }
      else {
        GST_DEBUG("application reads [def] song-folder gconf_settings : '%s'",((GParamSpecString *)pspec)->default_value);
        g_value_set_string(value, ((GParamSpecString *)pspec)->default_value);
      }
    } break;   
    case BT_SETTINGS_FOLDER_RECORD: {
      gchar * const prop=gconf_client_get_string(self->priv->client,BT_GCONF_PATH_BUZZTARD"/record-folder",NULL);
      if(prop && g_file_test(prop,G_FILE_TEST_IS_DIR)) {
        GST_DEBUG("application reads record-folder gconf_settings : '%s'",prop);
        g_value_set_string(value, prop);
        g_free(prop);
      }
      else {
        GST_DEBUG("application reads [def] record-folder gconf_settings : '%s'",((GParamSpecString *)pspec)->default_value);
        g_value_set_string(value, ((GParamSpecString *)pspec)->default_value);
      }
    } break;   
    case BT_SETTINGS_FOLDER_SAMPLE: {
      gchar * const prop=gconf_client_get_string(self->priv->client,BT_GCONF_PATH_BUZZTARD"/sample-folder",NULL);
      if(prop && g_file_test(prop,G_FILE_TEST_IS_DIR)) {
        GST_DEBUG("application reads sample-folder gconf_settings : '%s'",prop);
        g_value_set_string(value, prop);
        g_free(prop);
      }
      else {
        GST_DEBUG("application reads [def] sample-folder gconf_settings : '%s'",((GParamSpecString *)pspec)->default_value);
        g_value_set_string(value, ((GParamSpecString *)pspec)->default_value);
      }
    } break;   
    /* system settings */
    case BT_SETTINGS_SYSTEM_AUDIOSINK: {
      gchar * const prop=gconf_client_get_string(self->priv->client,BT_GCONF_PATH_GSTREAMER"/audiosink",NULL);
      GST_DEBUG("application reads system audiosink gconf_settings : '%s'",prop);
      g_value_set_string(value, prop);
      g_free(prop);
    } break;
    case BT_SETTINGS_SYSTEM_TOOLBAR_STYLE: {
      gchar * const prop=gconf_client_get_string(self->priv->client,BT_GCONF_PATH_GNOME"/toolbar_style",NULL);
      GST_DEBUG("application reads system toolbar style gconf_settings : '%s'",prop);
      g_value_set_string(value, prop);
      g_free(prop);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_gconf_settings_set_property(GObject      * const object,
                              const guint         property_id,
                              const GValue * const value,
                              GParamSpec   * const pspec)
{
  const BtGConfSettings * const self = BT_GCONF_SETTINGS(object);
  return_if_disposed();
  switch (property_id) {
    case BT_SETTINGS_AUDIOSINK: {
      gboolean gconf_ret=FALSE;
      gchar * const prop=g_value_dup_string(value);
      GST_DEBUG("application writes audiosink gconf_settings(%p) : %s",self,prop);
      gconf_ret=gconf_client_set_string(self->priv->client,BT_GCONF_PATH_BUZZTARD"/audiosink",prop,NULL);
      g_free(prop);
      g_return_if_fail(gconf_ret == TRUE);
    } break;
    case BT_SETTINGS_SAMPLE_RATE: {
      gboolean gconf_ret=FALSE;
      guint prop=g_value_get_uint(value);
      GST_DEBUG("application writes sample-rate gconf_settings : %u",prop);
      gconf_ret=gconf_client_set_int(self->priv->client,BT_GCONF_PATH_BUZZTARD"/sample-rate",prop,NULL);
      g_return_if_fail(gconf_ret == TRUE);
    } break;
    case BT_SETTINGS_CHANNELS: {
      gboolean gconf_ret=FALSE;
      guint prop=g_value_get_uint(value);
      GST_DEBUG("application writes channels gconf_settings : %u",prop);
      gconf_ret=gconf_client_set_int(self->priv->client,BT_GCONF_PATH_BUZZTARD"/channels",prop,NULL);
      g_return_if_fail(gconf_ret == TRUE);
    } break;
    case BT_SETTINGS_MENU_TOOLBAR_HIDE: {
      gboolean gconf_ret=FALSE;
      gboolean prop=g_value_get_boolean(value);
      GST_DEBUG("application writes toolbar-hide gconf_settings : %d",prop);
      gconf_ret=gconf_client_set_bool(self->priv->client,BT_GCONF_PATH_BUZZTARD"/toolbar-hide",prop,NULL);
      g_return_if_fail(gconf_ret == TRUE);
    } break;
    case BT_SETTINGS_MENU_STATUSBAR_HIDE: {
      gboolean gconf_ret=FALSE;
      gboolean prop=g_value_get_boolean(value);
      GST_DEBUG("application writes statusbar-hide gconf_settings : %d",prop);
      gconf_ret=gconf_client_set_bool(self->priv->client,BT_GCONF_PATH_BUZZTARD"/statusbar-hide",prop,NULL);
      g_return_if_fail(gconf_ret == TRUE);
    } break;
    case BT_SETTINGS_MENU_TABS_HIDE: {
      gboolean gconf_ret=FALSE;
      gboolean prop=g_value_get_boolean(value);
      GST_DEBUG("application writes tabs-hide gconf_settings : %d",prop);
      gconf_ret=gconf_client_set_bool(self->priv->client,BT_GCONF_PATH_BUZZTARD"/tabs-hide",prop,NULL);
      g_return_if_fail(gconf_ret == TRUE);
    } break;
    case BT_SETTINGS_MACHINE_VIEW_GRID_DENSITY: {
      gboolean gconf_ret=FALSE;
      gchar *prop=g_value_dup_string(value);
      GST_DEBUG("application writes grid-density gconf_settings : %s",prop);
      gconf_ret=gconf_client_set_string(self->priv->client,BT_GCONF_PATH_BUZZTARD"/grid-density",prop,NULL);
      g_free(prop);
      g_return_if_fail(gconf_ret == TRUE);
    } break;
    case BT_SETTINGS_NEWS_SEEN: {
      gboolean gconf_ret=FALSE;
      guint prop=g_value_get_uint(value);
      GST_DEBUG("application writes news-seen gconf_settings : %u",prop);
      gconf_ret=gconf_client_set_int(self->priv->client,BT_GCONF_PATH_BUZZTARD"/news-seen",prop,NULL);
      g_return_if_fail(gconf_ret == TRUE);
    } break;
    case BT_SETTINGS_MISSING_MACHINES: {
      gboolean gconf_ret=FALSE;
      gchar *prop=g_value_dup_string(value);
      GST_DEBUG("application writes missing-machines gconf_settings : %s",prop);
      gconf_ret=gconf_client_set_string(self->priv->client,BT_GCONF_PATH_BUZZTARD"/missing-machines",prop,NULL);
      g_free(prop);
      g_return_if_fail(gconf_ret == TRUE);
    } break;
    /* playback controller */
    case BT_SETTINGS_PLAYBACK_CONTROLLER_COHERENCE_UPNP_ACTIVE: {
      gboolean gconf_ret=FALSE;
      gboolean prop=g_value_get_boolean(value);
      GST_DEBUG("application writes playback-controller/coherence-upnp-active gconf_settings : %d",prop);
      gconf_ret=gconf_client_set_bool(self->priv->client,BT_GCONF_PATH_BUZZTARD"/playback-controller/coherence-upnp-active",prop,NULL);
      g_return_if_fail(gconf_ret == TRUE);
    } break;
    case BT_SETTINGS_PLAYBACK_CONTROLLER_COHERENCE_UPNP_PORT: {
     gboolean gconf_ret=FALSE;
      guint prop=g_value_get_uint(value);
      GST_DEBUG("application writes playback-controller/coherence-upnp-port gconf_settings : %u",prop);
      gconf_ret=gconf_client_set_int(self->priv->client,BT_GCONF_PATH_BUZZTARD"/playback-controller/coherence-upnp-port",prop,NULL);
      g_return_if_fail(gconf_ret == TRUE);
    } break;
    /* directory settings */
    case BT_SETTINGS_FOLDER_SONG: {
      gboolean gconf_ret=FALSE;
      gchar *prop=g_value_dup_string(value);
      GST_DEBUG("application writes song-folder gconf_settings : %s",prop);
      gconf_ret=gconf_client_set_string(self->priv->client,BT_GCONF_PATH_BUZZTARD"/song-folder",prop,NULL);
      g_free(prop);
      g_return_if_fail(gconf_ret == TRUE);
    } break;
    case BT_SETTINGS_FOLDER_RECORD: {
      gboolean gconf_ret=FALSE;
      gchar *prop=g_value_dup_string(value);
      GST_DEBUG("application writes record-folder gconf_settings : %s",prop);
      gconf_ret=gconf_client_set_string(self->priv->client,BT_GCONF_PATH_BUZZTARD"/record-folder",prop,NULL);
      g_free(prop);
      g_return_if_fail(gconf_ret == TRUE);
    } break;
    case BT_SETTINGS_FOLDER_SAMPLE: {
      gboolean gconf_ret=FALSE;
      gchar *prop=g_value_dup_string(value);
      GST_DEBUG("application writes sample-folder gconf_settings : %s",prop);
      gconf_ret=gconf_client_set_string(self->priv->client,BT_GCONF_PATH_BUZZTARD"/sample-folder",prop,NULL);
      g_free(prop);
      g_return_if_fail(gconf_ret == TRUE);
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
  gconf_client_suggest_sync(self->priv->client,NULL);
  g_object_unref(self->priv->client);

  GST_DEBUG("!!!! self=%p",self);
  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_gconf_settings_finalize(GObject * const object) {
  const BtGConfSettings * const self = BT_GCONF_SETTINGS(object);

  GST_DEBUG("!!!! self=%p",self);

  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void bt_gconf_settings_init(GTypeInstance * const instance, gconstpointer g_class) {
  BtGConfSettings * const self = BT_GCONF_SETTINGS(instance);

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_GCONF_SETTINGS, BtGConfSettingsPrivate);

  GST_DEBUG("!!!! self=%p",self);

  self->priv->client=gconf_client_get_default();
  gconf_client_set_error_handling(self->priv->client,GCONF_CLIENT_HANDLE_UNRETURNED);
  // register the config cache
  GST_DEBUG("listening to settings");
  gconf_client_add_dir(self->priv->client,BT_GCONF_PATH_GSTREAMER,GCONF_CLIENT_PRELOAD_ONELEVEL,NULL);
  gconf_client_add_dir(self->priv->client,BT_GCONF_PATH_GNOME,GCONF_CLIENT_PRELOAD_ONELEVEL,NULL);
  gconf_client_add_dir(self->priv->client,BT_GCONF_PATH_BUZZTARD,GCONF_CLIENT_PRELOAD_RECURSIVE,NULL);
}

static void bt_gconf_settings_class_init(BtGConfSettingsClass * const klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtGConfSettingsPrivate));

  gobject_class->set_property = bt_gconf_settings_set_property;
  gobject_class->get_property = bt_gconf_settings_get_property;
  gobject_class->dispose      = bt_gconf_settings_dispose;
  gobject_class->finalize     = bt_gconf_settings_finalize;
}

GType bt_gconf_settings_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      (guint16)(sizeof(BtGConfSettingsClass)),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_gconf_settings_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      (guint16)(sizeof(BtGConfSettings)),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_gconf_settings_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(BT_TYPE_SETTINGS,"BtGConfSettings",&info,0);
  }
  return type;
}
