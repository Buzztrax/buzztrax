/* $Id: registry.c,v 1.1 2007-03-09 19:39:19 ensonic Exp $
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
 * SECTION:bticregistry
 * @short_description: buzztards interaction controller registry
 *
 */ 
 
#define BTIC_CORE
#define BTIC_REGISTRY_C

#include <libbtic/ic.h>

enum {
  REGISTRY_DEVICE_LIST=1
};

struct _BtIcRegistryPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  GList *devices;
};

static GObjectClass *parent_class=NULL;

//-- helper

//-- handler

//-- constructor methods

/**
 * btic_registry_new:
 * @self: instance to finish construction of
 *
 * This is the common part of registry construction. It needs to be called from
 * within the sub-classes constructor methods.
 *
 * Returns: %TRUE for succes, %FALSE otherwise
 */
gboolean btic_registry_new(const BtIcRegistry * const self) {
  gboolean res=FALSE;

  g_return_val_if_fail(BTIC_IS_REGISTRY(self),FALSE);

  res=TRUE;
//Error:
  return(res);
}

//-- methods

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void btic_registry_get_property(GObject      * const object,
                               const guint         property_id,
                               GValue       * const value,
                               GParamSpec   * const pspec)
{
  const BtIcRegistry * const self = BTIC_REGISTRY(object);
  return_if_disposed();
  switch (property_id) {
    case REGISTRY_DEVICE_LIST: {
      g_value_set_pointer(value,g_list_copy(self->priv->devices));
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void btic_registry_set_property(GObject      * const object,
                              const guint         property_id,
                              const GValue * const value,
                              GParamSpec   * const pspec)
{
  const BtIcRegistry * const self = BTIC_REGISTRY(object);
  return_if_disposed();
  switch (property_id) {
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void btic_registry_dispose(GObject * const object) {
  const BtIcRegistry * const self = BTIC_REGISTRY(object);

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p, self->ref_ct=%d",self,G_OBJECT(self)->ref_count);

  GST_DEBUG("  chaining up");
  G_OBJECT_CLASS(parent_class)->dispose(object);
  GST_DEBUG("  done");
}

static void btic_registry_finalize(GObject * const object) {
  const BtIcRegistry * const self = BTIC_REGISTRY(object);

  GST_DEBUG("!!!! self=%p",self);
  
  if(self->priv->devices) {
    /* @todo: also free node data
    const GList* node;
    for(node=self->priv->devices;node;node=g_list_next(node)) {
      g_free(node->data);
    }
    */
    g_list_free(self->priv->devices);
    self->priv->devices=NULL;
  }

  GST_DEBUG("  chaining up");
  G_OBJECT_CLASS(parent_class)->finalize(object);
  GST_DEBUG("  done");
}

static void btic_registry_init(const GTypeInstance * const instance, gconstpointer const g_class) {
  BtIcRegistry * const self = BTIC_REGISTRY(instance);
  
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BTIC_TYPE_REGISTRY, BtIcRegistryPrivate);

  // @todo: scan devices via hal
  // @todo: register notify handler for add/remove
}

static void btic_registry_class_init(BtIcRegistryClass * const klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtIcRegistryPrivate));

  gobject_class->set_property = btic_registry_set_property;
  gobject_class->get_property = btic_registry_get_property;
  gobject_class->dispose      = btic_registry_dispose;
  gobject_class->finalize     = btic_registry_finalize;

  g_object_class_install_property(gobject_class,REGISTRY_DEVICE_LIST,
                                  g_param_spec_pointer("devices",
                                     "device list prop",
                                     "A copy of the list of control devices",
                                     G_PARAM_READABLE));
}

GType btic_registry_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      (guint16)(sizeof(BtIcRegistryClass)),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)btic_registry_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      (guint16)(sizeof(BtIcRegistry)),
      0,   // n_preallocs
      (GInstanceInitFunc)btic_registry_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(G_TYPE_OBJECT,"BtIcRegistry",&info,0);
  }
  return type;
}
