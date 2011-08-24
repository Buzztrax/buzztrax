/* $Id: sequence-grid-model.h 3349 2011-05-02 20:35:54Z ensonic $
 *
 * Buzztard
 * Copyright (C) 2011 Buzztard team <buzztard-devel@lists.sf.net>
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

#ifndef BT_SEQUENCE_GRID_MODEL_H
#define BT_SEQUENCE_GRID_MODEL_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_SEQUENCE_GRID_MODEL            (bt_sequence_grid_model_get_type())
#define BT_SEQUENCE_GRID_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_SEQUENCE_GRID_MODEL, BtSequenceGridModel))
#define BT_SEQUENCE_GRID_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_SEQUENCE_GRID_MODEL, BtSequenceGridModelClass))
#define BT_IS_SEQUENCE_GRID_MODEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_SEQUENCE_GRID_MODEL))
#define BT_IS_SEQUENCE_GRID_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_SEQUENCE_GRID_MODEL))
#define BT_SEQUENCE_GRID_MODEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_SEQUENCE_GRID_MODEL, BtSequenceGridModelClass))

/* type macros */

typedef struct _BtSequenceGridModel BtSequenceGridModel;
typedef struct _BtSequenceGridModelClass BtSequenceGridModelClass;
typedef struct _BtSequenceGridModelPrivate BtSequenceGridModelPrivate;

/**
 * BtSequenceGridModel:
 *
 * Data model for #GtkTreeView or #GtkComboBox.
 */
struct _BtSequenceGridModel {
  GObject parent;

  /*< private >*/
  BtSequenceGridModelPrivate *priv;
};
/* structure of the ui-resources class */
struct _BtSequenceGridModelClass {
  GObjectClass parent;
};

enum {
  BT_SEQUENCE_GRID_MODEL_SHADE=0,
  BT_SEQUENCE_GRID_MODEL_POS,
  BT_SEQUENCE_GRID_MODEL_POSSTR,
  BT_SEQUENCE_GRID_MODEL_LABEL,
  __BT_SEQUENCE_GRID_MODEL_N_COLUMNS
};


#define BT_TYPE_SEQUENCE_GRID_MODEL_POS_FORMAT   (bt_sequence_grid_model_pos_format_get_type())

typedef enum {
  BT_SEQUENCE_GRID_MODEL_POS_FORMAT_TICKS=0,
  BT_SEQUENCE_GRID_MODEL_POS_FORMAT_TIME,
  BT_SEQUENCE_GRID_MODEL_POS_FORMAT_BEATS
} BtSequenceGridModelPosFormat;


GType bt_sequence_grid_model_get_type(void) G_GNUC_CONST;
GType bt_sequence_grid_model_pos_format_get_type(void) G_GNUC_CONST;

#endif // BT_SEQUENCE_GRID_MODEL_H
