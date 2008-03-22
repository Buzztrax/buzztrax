/* $Id: persistence-location.c,v 1.5 2007-07-19 13:23:06 ensonic Exp $
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
 * SECTION:btpersistencelocation
 * @short_description: base-class for object deserialisation filters
 * @see_also: #BtPersistenceSelection
 *
 * Base class for a location that partial songs can be loaded in. A 'paste'
 * operation will use this to specify where data gets pasted.
 */

#define BT_CORE
#define BT_PERSISTENCE_LOCATION_C

#include "core_private.h"

GType bt_persistence_location_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    const GTypeInfo info = {
      sizeof(BtPersistenceLocationClass),
      NULL, // base_init
      NULL, // base_finalize
      NULL, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof(BtPersistenceLocation),
      0,   // n_preallocs
      NULL, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(G_TYPE_OBJECT,"BtPersistenceLocation",&info,G_TYPE_FLAG_ABSTRACT);
  }
  return type;
}
