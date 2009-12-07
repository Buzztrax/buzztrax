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
 * SECTION:bttestsettings
 * @short_description: unit test helper for buzztard settings handling
 *
 * Non-persistent dummy implementation to programmatically test application
 * settings.
 */ 

#define BT_CORE
#define BT_TEST_SETTINGS_C

#include "bt-check.h"

#include <libbuzztard-core/core.h>
#include "bt-test-settings.h"

struct _BtTestSettingsPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* key=value array, keys are defined in BtSettings */
  GValue *settings[BT_SETTINGS_COUNT];
};

static BtSettingsClass *parent_class=NULL;

//-- constructor methods

/**
 * bt_test_settings_new:
 *
 * Create a new instance.
 *
 * Returns: the new instance or %NULL in case of an error
 */
const BtTestSettings *bt_test_settings_new(void) {
  const BtTestSettings *self;
  self=BT_TEST_SETTINGS(g_object_new(BT_TYPE_TEST_SETTINGS,NULL));
  
  GST_INFO("created new settings object from factory %p",self);
  return(self);  
}

//-- methods

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_test_settings_get_property(GObject * const object,const guint property_id, GValue * const value, GParamSpec * const pspec) {
  const BtTestSettings * const self = BT_TEST_SETTINGS(object);
  GValue *prop=self->priv->settings[property_id];
  return_if_disposed();

  switch (property_id) {
    case BT_SETTINGS_AUDIOSINK:
    case BT_SETTINGS_SYSTEM_AUDIOSINK:
    case BT_SETTINGS_MACHINE_VIEW_GRID_DENSITY:
    case BT_SETTINGS_NEWS_SEEN:
    case BT_SETTINGS_MISSING_MACHINES:
    case BT_SETTINGS_SYSTEM_TOOLBAR_STYLE:
    case BT_SETTINGS_FOLDER_SONG:
    case BT_SETTINGS_FOLDER_RECORD:
    case BT_SETTINGS_FOLDER_SAMPLE:
    {
      if(prop) {
        g_value_set_string(value, g_value_get_string(prop));
      }
      else {
        g_value_set_string(value, ((GParamSpecString *)pspec)->default_value);
      }
    } break;
    case BT_SETTINGS_MENU_TOOLBAR_HIDE:
    case BT_SETTINGS_MENU_STATUSBAR_HIDE:
    case BT_SETTINGS_MENU_TABS_HIDE:
    case BT_SETTINGS_PLAYBACK_CONTROLLER_COHERENCE_UPNP_ACTIVE:
    {
      if(prop) {
        g_value_set_boolean(value, g_value_get_boolean(prop));
      }
      else {
        g_value_set_boolean(value, ((GParamSpecBoolean *)pspec)->default_value);
      }
    } break; 
    case BT_SETTINGS_SAMPLE_RATE:
    case BT_SETTINGS_CHANNELS:
    case BT_SETTINGS_PLAYBACK_CONTROLLER_COHERENCE_UPNP_PORT:
    {
      if(prop) {
        g_value_set_uint(value, g_value_get_uint(prop));
      }
      else {
        g_value_set_uint(value, ((GParamSpecUInt *)pspec)->default_value);
      }
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_test_settings_set_property(GObject * const object, const guint property_id, const GValue * const value, GParamSpec * const pspec) {
  const BtTestSettings * const self = BT_TEST_SETTINGS(object);
  GValue *prop=self->priv->settings[property_id];
  return_if_disposed();

  switch (property_id) {
    case BT_SETTINGS_AUDIOSINK:
    case BT_SETTINGS_SYSTEM_AUDIOSINK:
    case BT_SETTINGS_MACHINE_VIEW_GRID_DENSITY:
    case BT_SETTINGS_NEWS_SEEN:
    case BT_SETTINGS_SYSTEM_TOOLBAR_STYLE:
    case BT_SETTINGS_FOLDER_SONG:
    case BT_SETTINGS_FOLDER_RECORD:
    case BT_SETTINGS_FOLDER_SAMPLE:
    {
      if(!prop) {
        self->priv->settings[property_id]=prop=self->priv->settings[property_id]=g_new0(GValue,1);
        g_value_init(prop,G_TYPE_STRING);
      }
      g_value_set_string(prop, g_value_get_string(value));
    } break;
    case BT_SETTINGS_MENU_TOOLBAR_HIDE:
    case BT_SETTINGS_MENU_STATUSBAR_HIDE:
    case BT_SETTINGS_MENU_TABS_HIDE:
    {
      if(!prop) {
        self->priv->settings[property_id]=prop=self->priv->settings[property_id]=g_new0(GValue,1);
        g_value_init(prop,G_TYPE_BOOLEAN);
      }
      g_value_set_boolean(prop, g_value_get_boolean(value));
    } break; 
    case BT_SETTINGS_SAMPLE_RATE:
    case BT_SETTINGS_CHANNELS:
    case BT_SETTINGS_PLAYBACK_CONTROLLER_COHERENCE_UPNP_PORT:
    {
      if(!prop) {
        self->priv->settings[property_id]=prop=self->priv->settings[property_id]=g_new0(GValue,1);
        g_value_init(prop,G_TYPE_UINT);
      }
      g_value_set_boolean(prop, g_value_get_uint(value));
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_test_settings_dispose(GObject * const object) {
  const BtTestSettings * const self = BT_TEST_SETTINGS(object);

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;
  
  GST_DEBUG("!!!! self=%p",self);
  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_test_settings_finalize(GObject * const object) {
  const BtTestSettings * const self = BT_TEST_SETTINGS(object);
  guint i;

  GST_DEBUG("!!!! self=%p",self);

  // SETTINGS start at 1
  for(i=1;i<BT_SETTINGS_COUNT;i++) {
    if(self->priv->settings[i]) {
      g_value_unset(self->priv->settings[i]);
      g_free(self->priv->settings[i]);
    }
  }

  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void bt_test_settings_init(GTypeInstance * const instance, gconstpointer g_class) {
  BtTestSettings * const self = BT_TEST_SETTINGS(instance);
  
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_TEST_SETTINGS, BtTestSettingsPrivate);
  memset(self->priv->settings,0,BT_SETTINGS_COUNT*sizeof(gpointer));
}

static void bt_test_settings_class_init(BtTestSettingsClass * const klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtTestSettingsPrivate));

  gobject_class->set_property = bt_test_settings_set_property;
  gobject_class->get_property = bt_test_settings_get_property;
  gobject_class->dispose      = bt_test_settings_dispose;
  gobject_class->finalize     = bt_test_settings_finalize;
}

/* helper to test otherwise readonly settings */
void bt_test_settings_set(BtTestSettings * const self, gchar *property_name, gpointer value) {
  guint property_id=0;
  
  if(!strcmp(property_name,"system-audiosink")) property_id=BT_SETTINGS_SYSTEM_AUDIOSINK;
  if(!strcmp(property_name,"toolbar-style")) property_id=BT_SETTINGS_SYSTEM_TOOLBAR_STYLE;
  
  switch (property_id) {
    case BT_SETTINGS_SYSTEM_AUDIOSINK:
    case BT_SETTINGS_SYSTEM_TOOLBAR_STYLE: {
      gchar **ptr = (gchar **)value;
      gchar *val = *ptr;
      GValue *prop=self->priv->settings[property_id];

      if(!prop) {
        self->priv->settings[property_id]=prop=self->priv->settings[property_id]=g_new0(GValue,1);
        g_value_init(prop,G_TYPE_STRING);
      }
      g_value_set_string(prop, val);
    } break;
  }
}

GType bt_test_settings_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      sizeof(BtTestSettingsClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_test_settings_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof(BtTestSettings),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_test_settings_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(BT_TYPE_SETTINGS,"BtTestSettings",&info,0);
  }
  return type;
}
