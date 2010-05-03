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
 * SECTION:btchangeloger
 * @short_description: interface for the editor action journaling
 *
 * Defines undo/redo interface.
 */
 
#define BT_EDIT
#define BT_CHANGE_LOGGER_C

#include "bt-edit.h"


//-- wrapper

/**
 * bt_change_logger_change:
 * @self: an object that implements logging changes
 * @data: serialised data of the action to apply
 *
 * Run the editor action pointed to by @data.
 *
 * Returns: %TRUE for success.
 */
gboolean bt_change_logger_change(const BtChangeLogger *self,const gchar *data) {
  g_return_val_if_fail (BT_IS_CHANGE_LOGGER (self), FALSE);
  
  return (BT_CHANGE_LOGGER_GET_INTERFACE (self)->change(self, data));
}

//-- interface internals

GType bt_change_logger_get_type(void) {
  static GType type = 0;

  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      sizeof (BtChangeLoggerInterface),
      NULL,   /* base_init */
      NULL,   /* base_finalize */
      NULL,   /* class_init */
      NULL,   /* class_finalize */
      NULL,   /* class_data */
      0,
      0,      /* n_preallocs */
      NULL    /* instance_init */
    };
    type=g_type_register_static(G_TYPE_INTERFACE,"BtChangeLogger",&info,0);
  }
  return type;
}
