/* $Id: application.c,v 1.61 2006-09-06 20:17:39 ensonic Exp $
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
 * SECTION:btapplication
 * @short_description: base class for a buzztard based application
 *
 * Everyone who writes an application using the #btcore library should create a
 * child of the <classname>BtApplication</classname> class.
 *
 * The base class automatically creates a #GstBin element as a thread.
 * This can be retrieved via the bin property of an application instance.
 * When creating #BtSong instances, the #BtApplication instance needs to be passed
 * to the bt_song_new() constructor, so that it can retrieve the #GstBin element.
 * <informalexample>
 *  <programlisting language="c">song=bt_song_new(app)</programlisting>
 * </informalexample>
 *
 * Another module the application base class maintains is a settings instance (see
 * #BtSettings), that manages application preferences. 
 */ 
 
#define BT_CORE
#define BT_APPLICATION_C

#include <libbtcore/core.h>

enum {
  APPLICATION_BIN=1,
  APPLICATION_SETTINGS
};

struct _BtApplicationPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* the main gstreamer container element */
  GstElement *bin;
  /* a reference to the buzztard settings object */
  BtSettings *settings;
};

static GObjectClass *parent_class=NULL;

//-- helper

//-- handler

//-- constructor methods

/**
 * bt_application_new:
 * @self: instance to finish construction of
 *
 * This is the common part of application construction. It needs to be called from
 * within the sub-classes constructor methods.
 *
 * Returns: %TRUE for succes, %FALSE otherwise
 */
gboolean bt_application_new(const BtApplication * const self) {
  gboolean res=FALSE;

  g_return_val_if_fail(BT_IS_APPLICATION(self),FALSE);

  /*
  { // DEBUG
    gchar *audiosink_name,*system_audiosink_name;
    g_object_get(self->priv->settings,"audiosink",&audiosink_name,"system-audiosink",&system_audiosink_name,NULL);
    if(system_audiosink_name) {
      GST_INFO("default audiosink is \"%s\"",system_audiosink_name);
      g_free(system_audiosink_name);
    }
    if(audiosink_name) {
      GST_INFO("buzztard audiosink is \"%s\"",audiosink_name);
      g_free(audiosink_name);
    }
  } // DEBUG
  */
  res=TRUE;
//Error:
  return(res);
}

//-- methods

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_application_get_property(GObject      * const object,
                               const guint         property_id,
                               GValue       * const value,
                               GParamSpec   * const pspec)
{
  const BtApplication * const self = BT_APPLICATION(object);
  return_if_disposed();
  switch (property_id) {
    case APPLICATION_BIN: {
      g_value_set_object(value, self->priv->bin);
      #ifndef HAVE_GLIB_2_8
      gst_object_ref(self->priv->bin);
      #endif
    } break;
    case APPLICATION_SETTINGS: {
      g_value_set_object(value, self->priv->settings);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_application_set_property(GObject      * const object,
                              const guint         property_id,
                              const GValue * const value,
                              GParamSpec   * const pspec)
{
  const BtApplication * const self = BT_APPLICATION(object);
  return_if_disposed();
  switch (property_id) {
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_application_dispose(GObject * const object) {
  const BtApplication * const self = BT_APPLICATION(object);

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p, self->ref_ct=%d",self,G_OBJECT(self)->ref_count);
  GST_INFO("bin->ref_ct=%d",G_OBJECT(self->priv->bin)->ref_count);
  GST_INFO("bin->numchildren=%d",GST_BIN(self->priv->bin)->numchildren);
  GST_INFO("settings->ref_ct=%d",G_OBJECT(self->priv->settings)->ref_count);

  gst_object_unref(self->priv->bin);
  g_object_try_unref(self->priv->settings);

  GST_DEBUG("  chaining up");
  G_OBJECT_CLASS(parent_class)->dispose(object);
  GST_DEBUG("  done");
}

static void bt_application_finalize(GObject * const object) {
  const BtApplication * const self = BT_APPLICATION(object);

  GST_DEBUG("!!!! self=%p",self);

  GST_DEBUG("  chaining up");
  G_OBJECT_CLASS(parent_class)->finalize(object);
  GST_DEBUG("  done");
}

static void bt_application_init(const GTypeInstance * const instance, gconstpointer const g_class) {
  BtApplication * const self = BT_APPLICATION(instance);
  
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_APPLICATION, BtApplicationPrivate);
  self->priv->bin = gst_pipeline_new("song");
  g_assert(GST_IS_ELEMENT(self->priv->bin));
  GST_INFO("bin->ref_ct=%d",G_OBJECT(self->priv->bin)->ref_count);
  
 
  // if we enable this we get lots of diagnostics
  //g_signal_connect (self->priv->bin, "deep_notify", G_CALLBACK(gst_object_default_deep_notify), NULL);
  
  self->priv->settings=bt_settings_new();
  g_assert(BT_IS_SETTINGS(self->priv->settings));
}

static void bt_application_class_init(BtApplicationClass * const klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtApplicationPrivate));

  gobject_class->set_property = bt_application_set_property;
  gobject_class->get_property = bt_application_get_property;
  gobject_class->dispose      = bt_application_dispose;
  gobject_class->finalize     = bt_application_finalize;

  g_object_class_install_property(gobject_class,APPLICATION_BIN,
                                  g_param_spec_object("bin",
                                     "bin ro prop",
                                     "applications top-level GstElement container",
                                     GST_TYPE_BIN, /* object type */
                                     G_PARAM_READABLE));

  g_object_class_install_property(gobject_class,APPLICATION_SETTINGS,
                                  g_param_spec_object("settings",
                                     "settings ro prop",
                                     "applications configuration settings",
                                     BT_TYPE_SETTINGS, /* object type */
                                     G_PARAM_READABLE));
}

GType bt_application_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    static const GTypeInfo info = {
      (guint16)(sizeof(BtApplicationClass)),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_application_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      (guint16)(sizeof(BtApplication)),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_application_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(G_TYPE_OBJECT,"BtApplication",&info,0);
  }
  return type;
}
