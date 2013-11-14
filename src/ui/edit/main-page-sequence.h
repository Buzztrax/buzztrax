/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@buzztrax.org>
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BT_MAIN_PAGE_SEQUENCE_H
#define BT_MAIN_PAGE_SEQUENCE_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_MAIN_PAGE_SEQUENCE            (bt_main_page_sequence_get_type ())
#define BT_MAIN_PAGE_SEQUENCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_MAIN_PAGE_SEQUENCE, BtMainPageSequence))
#define BT_MAIN_PAGE_SEQUENCE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_MAIN_PAGE_SEQUENCE, BtMainPageSequenceClass))
#define BT_IS_MAIN_PAGE_SEQUENCE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_MAIN_PAGE_SEQUENCE))
#define BT_IS_MAIN_PAGE_SEQUENCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_MAIN_PAGE_SEQUENCE))
#define BT_MAIN_PAGE_SEQUENCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_MAIN_PAGE_SEQUENCE, BtMainPageSequenceClass))

/* type macros */

typedef struct _BtMainPageSequence BtMainPageSequence;
typedef struct _BtMainPageSequenceClass BtMainPageSequenceClass;
typedef struct _BtMainPageSequencePrivate BtMainPageSequencePrivate;

/**
 * BtMainPageSequence:
 *
 * the sequence page for the editor application
 */
struct _BtMainPageSequence {
  GtkBox parent;
  
  /*< private >*/
  BtMainPageSequencePrivate *priv;
};

struct _BtMainPageSequenceClass {
  GtkBoxClass parent;
  
};

GType bt_main_page_sequence_get_type(void) G_GNUC_CONST;

#include "main-pages.h"

BtMainPageSequence *bt_main_page_sequence_new(const BtMainPages *pages);

void bt_main_page_sequence_delete_selection(const BtMainPageSequence *self);
void bt_main_page_sequence_cut_selection(const BtMainPageSequence *self);
void bt_main_page_sequence_copy_selection(const BtMainPageSequence *self);
void bt_main_page_sequence_paste_selection(const BtMainPageSequence *self);

#endif // BT_MAIN_PAGE_SEQUENCE_H
