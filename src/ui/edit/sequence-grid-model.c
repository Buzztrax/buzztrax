/* $Id: sequence-grid-model.c 3349 2011-05-02 20:35:54Z ensonic $
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
/**
 * SECTION:btsequencegridmodel
 * @short_description: data model class for widgets showing the pattern sequence
 * of a song
 *
 * A generic model representing the track x time grid of patterns of a song.
 * Can be shown by a treeview.
 */
 
/* design:
 * - support reordering tracks without rebuilding the model
 *   - check on_track_move_left_activated()
 *   - maybe we need to emit notity::tracks when reordering too
 * - support n-dummy tick rows at the end
 *   - we want to extend the model when the cursor goes down
 * - when do we refresh+recolorize in the old code
 *   - todo in new code
 *     - loop-end changed
 *     - move track left/right
 *     - cursor below the end-of song
 *     - insert delete rows (sequence should emit changed signal)
 *   - already handled in new code
 *     - add/remove track
 *     - machine removed (should trigger notify::tracks)
 *     - song changed (will create the model)
 *
 * - we can also use this model on the label menu with a filter to show only
 *   rows with labels
 *
 * - we have a property for the pos-format and when it changes we refresh all rows
 *   - main-page-sequence should build the combobox from this enum
 *     (should we have a model for enums?)
 */

#define BT_EDIT
#define BT_SEQUENCE_GRID_MODEL_C

#include "bt-edit.h"

//-- defines
#define N_COLUMNS __BT_MACHINE_MODEL_N_COLUMNS

//-- property ids

enum {
  SEQUENCE_GRID_MODEL_POS_FORMAT=1,
  SEQUENCE_GRID_MODEL_BARS
};

//-- structs

struct _BtSequenceGridModelPrivate {
  gint stamp;
  BtSequence *sequence;
  
  BtSequenceGridModelPosFormat pos_format;
  gulong bars;
  
  GType param_types[N_COLUMNS];
  gulong length, tracks;
};

//-- enums

GType bt_sequence_grid_model_pos_format_get_type(void) {
  static GType type = 0;
  if(G_UNLIKELY(type == 0)) {
    static const GEnumValue values[] = {
      { BT_SEQUENCE_GRID_MODEL_POS_FORMAT_TICKS,"BT_SEQUENCE_GRID_MODEL_POS_FORMAT_TICKS","ticks" },
      { BT_SEQUENCE_GRID_MODEL_POS_FORMAT_TIME, "BT_SEQUENCE_GRID_MODEL_POS_FORMAT_TIME", "time"  },
      { 0, NULL, NULL},
    };
    type = g_enum_register_static("BtSequenceGridModelPosFormat", values);
  }
  return type;
}

//-- the class

static void bt_sequence_grid_model_tree_model_init(gpointer const g_iface, gpointer const iface_data);

G_DEFINE_TYPE_WITH_CODE (BtSequenceGridModel, bt_sequence_grid_model, G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL,
						bt_sequence_grid_model_tree_model_init));


//-- helper

static gchar *format_position(const BtSequenceGridModel *model,gulong pos) {
  static gchar pos_str[20];

  switch(model->priv->pos_format) {
    case BT_SEQUENCE_GRID_MODEL_POS_FORMAT_TICKS:
      g_snprintf(pos_str,5,"%lu",pos);
      break;
    case BT_SEQUENCE_GRID_MODEL_POS_FORMAT_TIME: {
      gulong msec,sec,min;
      const GstClockTime bar_time=bt_sequence_get_bar_time(model->priv->sequence);

      msec=(gulong)((pos*bar_time)/G_USEC_PER_SEC);
      min=(gulong)(msec/60000);msec-=(min*60000);
      sec=(gulong)(msec/ 1000);msec-=(sec* 1000);
      g_sprintf(pos_str,"%02lu:%02lu.%03lu",min,sec,msec);
    } break;
    default:
      *pos_str='\0';
      GST_WARNING("unimplemented time format %d",model->priv->pos_format);
  }
  return(pos_str);
}

static void bt_sequence_grid_model_rows_changed(BtSequenceGridModel *model) {
  GtkTreePath *path;
  GtkTreeIter iter;
  gulong i;
    
  // trigger row-changed for all rows
  iter.stamp=model->priv->stamp;
  for(i=0;i<model->priv->length;i++) {
    iter.user_data=GUINT_TO_POINTER(i);
    path=gtk_tree_path_new();
    gtk_tree_path_append_index(path,i);
    gtk_tree_model_row_changed(GTK_TREE_MODEL(model),path,&iter);
    gtk_tree_path_free(path);
  }
}

//-- signal handlers

static void on_pattern_name_changed(BtPattern *pattern,GParamSpec *arg,gpointer user_data) {
  BtSequenceGridModel *model=BT_SEQUENCE_GRID_MODEL(user_data);
  BtPattern *that_pattern;
  GtkTreePath *path;
  GtkTreeIter iter;
  gulong i,j;
  gulong length=model->priv->length;
  gulong tracks=model->priv->length;
  
  iter.stamp=model->priv->stamp;

  // find all affected rows and signal updates
  // FIXME: skip tracks with wrong machine (do a first run and build a list of tracks)
  for(i=0;i<length;i++) {
    for(j=0;j<tracks;j++) {
      that_pattern=bt_sequence_get_pattern(model->priv->sequence,i,j);
      if(that_pattern==pattern) {
        iter.user_data=GUINT_TO_POINTER(i);
        path=gtk_tree_path_new();
        gtk_tree_path_append_index(path,i);
        gtk_tree_model_row_changed(GTK_TREE_MODEL(model),path,&iter);
        gtk_tree_path_free(path);
        g_object_unref(that_pattern);
        break;
      }
      else {
        g_object_unref(that_pattern);
      }
    }
  }
}

static void on_sequence_length_changed(BtSequence *sequence,GParamSpec *arg,gpointer user_data) {
  BtSequenceGridModel *model=BT_SEQUENCE_GRID_MODEL(user_data);
  GtkTreePath *path;
  gulong i,old_length=model->priv->length;
  
  g_object_get((gpointer)sequence,"length",&model->priv->length,NULL);
  if(old_length<model->priv->length) {
    GtkTreeIter iter;

    // trigger row-inserted
    iter.stamp=model->priv->stamp;
    for(i=old_length;i<model->priv->length;i++) {
      iter.user_data=GUINT_TO_POINTER(i);
      path=gtk_tree_path_new();
      gtk_tree_path_append_index(path,i);
      gtk_tree_model_row_inserted(GTK_TREE_MODEL(model),path,&iter);
      gtk_tree_path_free(path);
    }
  }
  else {
    // trigger row-deleted
    for(i=old_length-1;i>=model->priv->length;i--) {
      path=gtk_tree_path_new();
      gtk_tree_path_append_index(path,i);
      gtk_tree_model_row_deleted(GTK_TREE_MODEL(model),path);
      gtk_tree_path_free(path);
    }
  } 
}

static void on_sequence_tracks_changed(BtSequence *sequence,GParamSpec *arg,gpointer user_data) {
  BtSequenceGridModel *model=BT_SEQUENCE_GRID_MODEL(user_data);
  
  g_object_get((gpointer)sequence,"tracks",&model->priv->tracks,NULL);
  if(model->priv->length)
    bt_sequence_grid_model_rows_changed(model);
}

static void on_sequence_pattern_added(BtSequence *sequence,BtPattern *pattern,gpointer user_data) {
  BtSequenceGridModel *model=BT_SEQUENCE_GRID_MODEL(user_data);

  g_signal_connect(pattern,"notify::name",G_CALLBACK(on_pattern_name_changed),(gpointer)model);
}

static void on_sequence_pattern_removed(BtSequence *sequence,BtPattern *pattern,gpointer user_data) {
  BtSequenceGridModel *model=BT_SEQUENCE_GRID_MODEL(user_data);

  g_signal_handlers_disconnect_matched(pattern,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_pattern_name_changed,(gpointer)model);
}

//-- constructor methods

BtSequenceGridModel *bt_sequence_grid_model_new(BtSequence *sequence,gulong bars) {
  BtSequenceGridModel *self;

  self=g_object_new(BT_TYPE_SEQUENCE_GRID_MODEL, NULL);

  self->priv->sequence=sequence;
  g_object_add_weak_pointer((GObject *)sequence,(gpointer *)&self->priv->sequence);
  self->priv->bars=bars;

  // static columns
  self->priv->param_types[BT_SEQUENCE_GRID_MODEL_SHADE ]=G_TYPE_BOOLEAN;
  self->priv->param_types[BT_SEQUENCE_GRID_MODEL_POS   ]=G_TYPE_LONG;
  self->priv->param_types[BT_SEQUENCE_GRID_MODEL_POSSTR]=G_TYPE_STRING;
  self->priv->param_types[BT_SEQUENCE_GRID_MODEL_LABEL ]=G_TYPE_STRING;

  // add enough rows for current length
  on_sequence_tracks_changed(sequence,NULL,(gpointer)self);
  on_sequence_length_changed(sequence,NULL,(gpointer)self);

  // follow sequence grid size changes
  g_signal_connect(sequence,"notify::length",G_CALLBACK(on_sequence_length_changed),(gpointer)self);
  g_signal_connect(sequence,"notify::tracks",G_CALLBACK(on_sequence_tracks_changed),(gpointer)self);
  // track pattern first additions, last removal
  g_signal_connect(sequence,"pattern-added",G_CALLBACK(on_sequence_pattern_added),(gpointer)self);
  g_signal_connect(sequence,"pattern-removed",G_CALLBACK(on_sequence_pattern_removed),(gpointer)self);

  return(self);
}

//-- methods

BtMachine *bt_sequence_grid_model_get_object(BtSequenceGridModel *model,GtkTreeIter *iter) {
  return(g_sequence_get(iter->user_data));
}

//-- tree model interface

static GtkTreeModelFlags bt_sequence_grid_model_tree_model_get_flags(GtkTreeModel *tree_model) {
  return(GTK_TREE_MODEL_ITERS_PERSIST | GTK_TREE_MODEL_LIST_ONLY);
}

static gint bt_sequence_grid_model_tree_model_get_n_columns(GtkTreeModel *tree_model) {
  BtSequenceGridModel *model=BT_SEQUENCE_GRID_MODEL(tree_model);

  return(N_COLUMNS+model->priv->tracks);
}

static GType bt_sequence_grid_model_tree_model_get_column_type(GtkTreeModel *tree_model,gint index) {
  BtSequenceGridModel *model=BT_SEQUENCE_GRID_MODEL(tree_model);

  g_return_val_if_fail(index<N_COLUMNS+model->priv->tracks,G_TYPE_INVALID);

  if(index<N_COLUMNS)
    return(model->priv->param_types[index]);
  else
    return(G_TYPE_STRING);
}

static gboolean bt_sequence_grid_model_tree_model_get_iter(GtkTreeModel *tree_model,GtkTreeIter *iter,GtkTreePath *path) {
  BtSequenceGridModel *model=BT_SEQUENCE_GRID_MODEL(tree_model);
  guint tick=gtk_tree_path_get_indices (path)[0];

  if(tick>=model->priv->length)
    return(FALSE);

  iter->stamp=model->priv->stamp;
  iter->user_data=GUINT_TO_POINTER(tick);

  return(TRUE);
}

static GtkTreePath *bt_sequence_grid_model_tree_model_get_path(GtkTreeModel *tree_model,GtkTreeIter *iter) {
  BtSequenceGridModel *model=BT_SEQUENCE_GRID_MODEL(tree_model);
  GtkTreePath *path;

  g_return_val_if_fail(iter->stamp==model->priv->stamp,NULL);

  if(g_sequence_iter_is_end(iter->user_data))
    return(NULL);

  path=gtk_tree_path_new();
  gtk_tree_path_append_index(path,GPOINTER_TO_UINT(iter->user_data));

  return(path);
}

static void bt_sequence_grid_model_tree_model_get_value(GtkTreeModel *tree_model,GtkTreeIter *iter,gint column,GValue *value) {
  BtSequenceGridModel *model=BT_SEQUENCE_GRID_MODEL(tree_model);
  BtPattern *pattern;
  guint track,tick;

  g_return_if_fail(column<N_COLUMNS);

  if(column<N_COLUMNS)
    g_value_init(value,model->priv->param_types[column]);
  else
    g_value_init(value,G_TYPE_STRING);
  tick=GPOINTER_TO_UINT(iter->user_data);

  switch(column) {
    case BT_SEQUENCE_GRID_MODEL_SHADE: {
      guint shade=(tick%(model->priv->bars*2));
      if(shade==0) {
        g_value_set_boolean(value,FALSE);
      } else {
        // we're only interested in shade==bars, but lets set the others too
        // even though we should not be called for those
        g_value_set_boolean(value,TRUE);
      }
    } break;
    case BT_SEQUENCE_GRID_MODEL_POS:
      g_value_set_long(value,tick);
      break;
    case BT_SEQUENCE_GRID_MODEL_POSSTR:
      g_value_set_string(value,format_position(model,tick));
      break;
    case BT_SEQUENCE_GRID_MODEL_LABEL:
      g_value_set_string(value,bt_sequence_get_label(model->priv->sequence,tick));
      break;
    default:
      track=column-N_COLUMNS;
      if((pattern=bt_sequence_get_pattern(model->priv->sequence,tick,track))) {
        g_object_get_property((GObject *)pattern,"name",value);
        g_object_unref(pattern);
      }
  }
}

static gboolean bt_sequence_grid_model_tree_model_iter_next(GtkTreeModel *tree_model,GtkTreeIter *iter) {
  BtSequenceGridModel *model=BT_SEQUENCE_GRID_MODEL(tree_model);
  guint tick;

  g_return_val_if_fail(model->priv->stamp==iter->stamp,FALSE);

  tick=GPOINTER_TO_UINT(iter->user_data)+1;
  if(tick<model->priv->length) {
    iter->user_data=GUINT_TO_POINTER(tick);
    return TRUE;
  }
  else {
    iter->stamp=0;
    return FALSE;
  }
}

static gboolean bt_sequence_grid_model_tree_model_iter_children(GtkTreeModel *tree_model,GtkTreeIter *iter,GtkTreeIter *parent) {
  BtSequenceGridModel *model=BT_SEQUENCE_GRID_MODEL(tree_model);

  /* this is a list, nodes have no children */
  if (!parent) {
    if (model->priv->length>0) {
      iter->stamp=model->priv->stamp;
      iter->user_data=GUINT_TO_POINTER(0);
      return(TRUE);
    }
  }
  iter->stamp=0;
  return(FALSE);
}

static gboolean bt_sequence_grid_model_tree_model_iter_has_child(GtkTreeModel *tree_model,GtkTreeIter *iter) {
  return(FALSE);
}

static gint bt_sequence_grid_model_tree_model_iter_n_children(GtkTreeModel *tree_model,GtkTreeIter *iter) {
  BtSequenceGridModel *model=BT_SEQUENCE_GRID_MODEL(tree_model);

  if (iter == NULL)
    return model->priv->length;

  g_return_val_if_fail(model->priv->stamp==iter->stamp,-1);
  return(0);
}

static gboolean  bt_sequence_grid_model_tree_model_iter_nth_child(GtkTreeModel *tree_model,GtkTreeIter *iter,GtkTreeIter *parent,gint n) {
  BtSequenceGridModel *model=BT_SEQUENCE_GRID_MODEL(tree_model);

  if (parent || n>=model->priv->length) {
    iter->stamp=0;
    return(FALSE);
  }

  iter->stamp=model->priv->stamp;
  iter->user_data=GUINT_TO_POINTER(n);

  return(TRUE);
}

static gboolean bt_sequence_grid_model_tree_model_iter_parent(GtkTreeModel *tree_model,GtkTreeIter *iter,GtkTreeIter *child) {
  iter->stamp=0;
  return(FALSE);
}

static void bt_sequence_grid_model_tree_model_init(gpointer const g_iface, gpointer const iface_data) {
  GtkTreeModelIface * const iface = g_iface;

  iface->get_flags       = bt_sequence_grid_model_tree_model_get_flags;
  iface->get_n_columns   = bt_sequence_grid_model_tree_model_get_n_columns;
  iface->get_column_type = bt_sequence_grid_model_tree_model_get_column_type;
  iface->get_iter        = bt_sequence_grid_model_tree_model_get_iter;
  iface->get_path        = bt_sequence_grid_model_tree_model_get_path;
  iface->get_value       = bt_sequence_grid_model_tree_model_get_value;
  iface->iter_next       = bt_sequence_grid_model_tree_model_iter_next;
  iface->iter_children   = bt_sequence_grid_model_tree_model_iter_children;
  iface->iter_has_child  = bt_sequence_grid_model_tree_model_iter_has_child;
  iface->iter_n_children = bt_sequence_grid_model_tree_model_iter_n_children;
  iface->iter_nth_child  = bt_sequence_grid_model_tree_model_iter_nth_child;
  iface->iter_parent     = bt_sequence_grid_model_tree_model_iter_parent;
}

//-- class internals

static void bt_sequence_grid_model_get_property(GObject * const object, const guint property_id, GValue * const value, GParamSpec * const pspec) {
  BtSequenceGridModel *self = BT_SEQUENCE_GRID_MODEL(object);

  switch (property_id) {
    case SEQUENCE_GRID_MODEL_POS_FORMAT: {
      g_value_set_enum(value, self->priv->pos_format);
    } break;
    case SEQUENCE_GRID_MODEL_BARS: {
      g_value_set_ulong(value, self->priv->bars);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_sequence_grid_model_set_property(GObject * const object, const guint property_id, const GValue * const value, GParamSpec * const pspec) {
  BtSequenceGridModel *self = BT_SEQUENCE_GRID_MODEL(object);

  switch (property_id) {
    case SEQUENCE_GRID_MODEL_POS_FORMAT: {
      BtSequenceGridModelPosFormat old_pos_format=self->priv->pos_format;      
      self->priv->pos_format=g_value_get_enum(value);
      if(self->priv->pos_format!=old_pos_format)
        bt_sequence_grid_model_rows_changed(self);
    } break;
    case SEQUENCE_GRID_MODEL_BARS: {
      gulong old_bars=self->priv->bars;
      self->priv->bars=g_value_get_ulong(value);
      if(self->priv->bars!=old_bars)
        bt_sequence_grid_model_rows_changed(self);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_sequence_grid_model_finalize(GObject *object) {
  BtSequenceGridModel *self = BT_SEQUENCE_GRID_MODEL(object);

  GST_DEBUG("!!!! self=%p",self);

  if(self->priv->sequence) {
    BtSequence *sequence=self->priv->sequence;

    g_signal_handlers_disconnect_matched(sequence,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_sequence_length_changed,(gpointer)self);
    g_signal_handlers_disconnect_matched(sequence,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_sequence_tracks_changed,(gpointer)self);
    g_signal_handlers_disconnect_matched(sequence,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_sequence_pattern_added,(gpointer)self);
    g_signal_handlers_disconnect_matched(sequence,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_sequence_pattern_removed,(gpointer)self);
  }

  G_OBJECT_CLASS(bt_sequence_grid_model_parent_class)->finalize(object);
}

static void bt_sequence_grid_model_init(BtSequenceGridModel *self) {
  self->priv=G_TYPE_INSTANCE_GET_PRIVATE(self,BT_TYPE_SEQUENCE_GRID_MODEL,BtSequenceGridModelPrivate);

  // random int to check whether an iter belongs to our model
  self->priv->stamp=g_random_int();
}

static void bt_sequence_grid_model_class_init(BtSequenceGridModelClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  g_type_class_add_private(klass,sizeof(BtSequenceGridModelPrivate));

  gobject_class->set_property = bt_sequence_grid_model_set_property;
  gobject_class->get_property = bt_sequence_grid_model_get_property;
  gobject_class->finalize     = bt_sequence_grid_model_finalize;

  g_object_class_install_property(gobject_class,SEQUENCE_GRID_MODEL_POS_FORMAT,
                                  g_param_spec_enum("pos-format",
                                     "position format prop",
                                     "the display format for position columns",
                                     BT_TYPE_SEQUENCE_GRID_MODEL_POS_FORMAT,  /* enum type */
                                     BT_SEQUENCE_GRID_MODEL_POS_FORMAT_TICKS, /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,SEQUENCE_GRID_MODEL_BARS,
                                  g_param_spec_ulong("bars",
                                     "bars prop",
                                     "tick stepping for the color shading",
                                     1,G_MAXUINT,
                                     16, /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));
}

