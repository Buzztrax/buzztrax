/* $Id: sequence.h,v 1.18 2006-09-03 13:21:44 ensonic Exp $
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

#ifndef BT_SEQUENCE_H
#define BT_SEQUENCE_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_SEQUENCE            (bt_sequence_get_type ())
#define BT_SEQUENCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_SEQUENCE, BtSequence))
#define BT_SEQUENCE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_SEQUENCE, BtSequenceClass))
#define BT_IS_SEQUENCE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_SEQUENCE))
#define BT_IS_SEQUENCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_SEQUENCE))
#define BT_SEQUENCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_SEQUENCE, BtSequenceClass))

/* type macros */

typedef struct _BtSequence BtSequence;
typedef struct _BtSequenceClass BtSequenceClass;
typedef struct _BtSequencePrivate BtSequencePrivate;

/**
 * BtSequence:
 *
 * Starting point for the #BtSong timeline data-structures.
 * Holds a series of #BtTimeLine objects, which define the events that are
 * sent to a #BtMachine at a time.
 */
struct _BtSequence {
  const GObject parent;
  
  /*< private >*/
  BtSequencePrivate *priv;
};
/* structure of the sequence class */
struct _BtSequenceClass {
  const GObjectClass parent;
};

/* used by SEQUENCE_TYPE */
GType bt_sequence_get_type(void) G_GNUC_CONST;


#endif // BT_SEQUENCE_H
