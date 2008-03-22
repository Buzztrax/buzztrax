/* $Id: plainfile-settings.c,v 1.23 2007-07-19 13:23:06 ensonic Exp $
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
 * SECTION:btplainfilesettings
 * @short_description: plain file based implementation sub class for buzztard 
 * settings handling
 *
 * Platform independent implementation for persistence of application settings.
 */ 

/*
  idea1: is to use a XML file like below
  <settings>
    <setting name="" value=""/>
    ...
  </settings>
  at init the whole document is loaded and
  get/set methods use xpath expr to get the nodes.
 
  idea2: is to use ini-files via glib
*/

#define BT_CORE
#define BT_PLAINFILE_SETTINGS_C

#include "core_private.h"
#include <libbtcore/settings-private.h>

struct _BtPlainfileSettingsPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* key=value list, keys are defined in BtSettings */
  //GHashTable *settings;
  //xmlDoc *settings;
};

static BtSettingsClass *parent_class=NULL;

//-- constructor methods

/**
 * bt_plainfile_settings_new:
 *
 * Create a new instance.
 *
 * Returns: the new instance or %NULL in case of an error
 */
const BtPlainfileSettings *bt_plainfile_settings_new(void) {
  const BtPlainfileSettings *self;
  self=BT_PLAINFILE_SETTINGS(g_object_new(BT_TYPE_PLAINFILE_SETTINGS,NULL));
  
  return(self);  
}

//-- methods

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_plainfile_settings_get_property(GObject      * const object,
                               const guint         property_id,
                               GValue       * const value,
                               GParamSpec   * const pspec)
{
  const BtPlainfileSettings * const self = BT_PLAINFILE_SETTINGS(object);
  return_if_disposed();
  switch (property_id) {
    case BT_SETTINGS_AUDIOSINK:
    case BT_SETTINGS_SYSTEM_AUDIOSINK: {
      g_value_set_string(value, "esdsink");
    } break;
    case BT_SETTINGS_SYSTEM_TOOLBAR_STYLE: {
      g_value_set_string(value, "both");
      } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_plainfile_settings_set_property(GObject      * const object,
                              const guint         property_id,
                              const GValue * const value,
                              GParamSpec   * const pspec)
{
  const BtPlainfileSettings * const self = BT_PLAINFILE_SETTINGS(object);
  return_if_disposed();
  switch (property_id) {
    case BT_SETTINGS_AUDIOSINK: {
      gchar * const prop=g_value_dup_string(value);
      GST_DEBUG("application writes audiosink plainfile_settings : %s",prop);
      // @todo set property value
      g_free(prop);
    } break;
     default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_plainfile_settings_dispose(GObject * const object) {
  const BtPlainfileSettings * const self = BT_PLAINFILE_SETTINGS(object);

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;
  
  GST_DEBUG("!!!! self=%p",self);
  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_plainfile_settings_finalize(GObject * const object) {
  const BtPlainfileSettings * const self = BT_PLAINFILE_SETTINGS(object);

  GST_DEBUG("!!!! self=%p",self);

  //g_hash_table_destroy(self->priv->settings);

  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void bt_plainfile_settings_init(GTypeInstance * const instance, gconstpointer g_class) {
  BtPlainfileSettings * const self = BT_PLAINFILE_SETTINGS(instance);
  
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_PLAINFILE_SETTINGS, BtPlainfileSettingsPrivate);
  //self->priv->settings=g_hash_table_new_full(g_str_hash,g_str_equal,g_free,g_free);
}

static void bt_plainfile_settings_class_init(BtPlainfileSettingsClass * const klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtPlainfileSettingsPrivate));

  gobject_class->set_property = bt_plainfile_settings_set_property;
  gobject_class->get_property = bt_plainfile_settings_get_property;
  gobject_class->dispose      = bt_plainfile_settings_dispose;
  gobject_class->finalize     = bt_plainfile_settings_finalize;
}

GType bt_plainfile_settings_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      sizeof(BtPlainfileSettingsClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_plainfile_settings_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof(BtPlainfileSettings),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_plainfile_settings_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(BT_TYPE_SETTINGS,"BtPlainfileSettings",&info,0);
  }
  return type;
}
