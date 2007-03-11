/* $Id: persistence-selection.c,v 1.4 2007-03-11 20:19:19 ensonic Exp $
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
 * SECTION:btpersistenceselection
 * @short_description: base-class for object serialisation filters
 * @see_also: #BtPersistenceLocation
 *
 * Base class to specify the range of data that should be serialized as a
 * partial song. A 'copy' or 'cut' operation can use this to specify the
 * selection.
 */

#define BT_CORE
#define BT_PERSISTENCE_SELECTION_C

#include <libbtcore/core.h>

GType bt_persistence_selection_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    const GTypeInfo info = {
      G_STRUCT_SIZE(BtPersistenceSelectionClass),
      NULL, // base_init
      NULL, // base_finalize
      NULL, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtPersistenceSelection),
      0,   // n_preallocs
      NULL, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(G_TYPE_OBJECT,"BtPersistenceSelection",&info,G_TYPE_FLAG_ABSTRACT);
  }
  return type;
}
