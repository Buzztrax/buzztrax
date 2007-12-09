/* $Id: persistence-selection.h,v 1.3 2006-09-03 13:21:44 ensonic Exp $
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

#ifndef BT_PERSISTENCE_SELECTION_H
#define BT_PERSISTENCE_SELECTION_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_PERSISTENCE_SELECTION            (bt_persistence_selection_get_type ())
#define BT_PERSISTENCE_SELECTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_PERSISTENCE_SELECTION, BtPersistenceSelection))
#define BT_PERSISTENCE_SELECTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_PERSISTENCE_SELECTION, BtPersistenceSelectionClass))
#define BT_IS_PERSISTENCE_SELECTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_PERSISTENCE_SELECTION))
#define BT_IS_PERSISTENCE_SELECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_PERSISTENCE_SELECTION))
#define BT_PERSISTENCE_SELECTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_PERSISTENCE_SELECTION, BtPersistenceSelectionClass))

/* type macros */

typedef struct _BtPersistenceSelection BtPersistenceSelection;
typedef struct _BtPersistenceSelectionClass BtPersistenceSelectionClass;

/**
 * BtPersistenceSelection:
 *
 * Abstract base class for the selection for a serialisation.
 */
struct _BtPersistenceSelection {
  const GObject parent;
};

struct _BtPersistenceSelectionClass {
  const GObjectClass parent;
};

GType bt_persistence_selection_get_type(void) G_GNUC_CONST;


#endif // BT_PERSISTENCE_SELECTION_H
