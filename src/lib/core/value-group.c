/* Buzztrax
 * Copyright (C) 2012 Buzztrax team <buzztrax-devel@buzztrax.org>
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
/**
 * SECTION:btvaluegroup
 * @short_description: a GValue array of parameter values
 *
 * A group of GValues, such as used in patterns. The class provides a variety of
 * methods to manipulate the data fields.
 *
 * The value group maintains two blocks of data values. One for validated fields
 * and one for plain fields. This allows step wise entry of data (multi column
 * entry of sparse enums). The validated cells are only set as the plain value
 * becomes valid. Invalid values are not copied nor are they stored in the song.
 */
/* TODO(ensonic): consider to lazily create the actual GValue *data array, by
 * keeping a count of non-empty cells
 * - if the count goes up to 1 we allocate
 * - if the count goes down to zero we can free the storage
 * - we can shortcut operations in empty groups
 */

#define BT_CORE
#define BT_VALUE_GROUP_C

#include "core_private.h"

//-- signal ids

enum
{
  PARAM_CHANGED_EVENT,
  GROUP_CHANGED_EVENT,
  LAST_SIGNAL
};

//-- property ids

enum
{
  VALUE_GROUP_PARAMETER_GROUP = 1,
  VALUE_GROUP_LENGTH
};

struct _BtValueGroupPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  gulong length;
  gulong params;
  gulong columns;
  BtParameterGroup *param_group;

  GValue *data;
};

static guint signals[LAST_SIGNAL] = { 0, };

//-- the class

G_DEFINE_TYPE_WITH_CODE (BtValueGroup, bt_value_group, G_TYPE_OBJECT, 
    G_ADD_PRIVATE(BtValueGroup));

//-- macros

//-- helper

/*
 * bt_value_group_resize_data_length:
 * @self: the value-group to resize the length
 * @length: the old length
 *
 * Resizes the event data grid to the new length. Keeps previous values.
 */
static void
bt_value_group_resize_data_length (const BtValueGroup * const self,
    const gulong length)
{
  const gulong old_data_count = length * self->priv->columns;
  const gulong new_data_count = self->priv->length * self->priv->columns;
  GValue *const data = self->priv->data;

  // allocate new space
  if ((self->priv->data = g_try_new0 (GValue, new_data_count))) {
    if (data) {
      gulong count = MIN (old_data_count, new_data_count);
      GST_DEBUG ("keeping data count=%lu, old=%lu, new=%lu", count,
          old_data_count, new_data_count);
      // copy old values over
      memcpy (self->priv->data, data, count * sizeof (GValue));
      // free gvalues
      if (old_data_count > new_data_count) {
        gulong i;

        for (i = new_data_count; i < old_data_count; i++) {
          if (BT_IS_GVALUE (&data[i]))
            g_value_unset (&data[i]);
        }
      }
      // free old data
      g_free (data);
    }
    GST_DEBUG
        ("extended value-group length from %lu to %lu, params = %lu",
        length, self->priv->length, self->priv->params);
  } else {
    GST_INFO
        ("extending value-group length from %lu to %lu failed, params = %lu",
        length, self->priv->length, self->priv->params);
    //self->priv->data=data;
    //self->priv->length=length;
  }
}

static GValue *
bt_value_group_get_event_data_unchecked (const BtValueGroup * const self,
    const gulong tick, const gulong param)
{
  return &self->priv->data[(tick * self->priv->columns) + param];
}

static GType
bt_value_group_get_param_type (const BtValueGroup * const self,
    const gulong param)
{
  return bt_parameter_group_get_param_type (self->priv->param_group, param);
}

//-- constructor methods

/**
 * bt_value_group_new:
 * @param_group: the parameter-group
 * @length: the number of ticks
 *
 * Create a new instance.
 *
 * Returns: (transfer full): the new instance or %NULL in case of an error
 *
 * Since: 0.7
 */
BtValueGroup *
bt_value_group_new (const BtParameterGroup * const param_group,
    const gulong length)
{
  return BT_VALUE_GROUP (g_object_new (BT_TYPE_VALUE_GROUP, "parameter-group",
          param_group, "length", length, NULL));
}

/**
 * bt_value_group_copy:
 * @self: the value-group to create a copy from
 *
 * Create a new instance as a copy of the given instance.
 *
 * Returns: (transfer full): the new instance or %NULL in case of an error
 *
 * Since: 0.7
 */
BtValueGroup *
bt_value_group_copy (const BtValueGroup * const self)
{
  BtValueGroup *value_group;
  gulong i, data_count;
  GValue *sdata, *ddata;

  g_return_val_if_fail (BT_IS_VALUE_GROUP (self), NULL);

  GST_INFO ("copying group vg = %p", self);

  value_group =
      bt_value_group_new (self->priv->param_group, self->priv->length);

  data_count = self->priv->length * self->priv->columns;
  sdata = self->priv->data;
  ddata = value_group->priv->data;
  // deep copy data
  for (i = 0; i < data_count; i++) {
    if (BT_IS_GVALUE (&sdata[i])) {
      g_value_init (&ddata[i], G_VALUE_TYPE (&sdata[i]));
      g_value_copy (&sdata[i], &ddata[i]);
    }
  }
  GST_INFO ("  group vg = %p copied", value_group);

  return value_group;
}

//-- methods

/**
 * bt_value_group_get_event_data:
 * @self: the pattern to search for the param
 * @tick: the tick (time) position starting with 0
 * @param: the number of the parameter starting with 0
 *
 * Fetches a cell from the given location in the pattern. If there is no event
 * there, then the %GValue is uninitialized. Test with BT_IS_GVALUE(event).
 *
 * Returns: the GValue or %NULL if out of the pattern range
 *
 * Since: 0.7
 */
GValue *
bt_value_group_get_event_data (const BtValueGroup * const self,
    const gulong tick, const gulong param)
{
  g_return_val_if_fail (BT_IS_VALUE_GROUP (self), NULL);
  g_return_val_if_fail (self->priv->data, NULL);
  g_return_val_if_fail (tick < self->priv->length, NULL);
  g_return_val_if_fail (param < self->priv->params, NULL);

  GST_LOG ("getting gvalue at tick=%lu/%lu and param %lu/%lu", tick,
      self->priv->length, param, self->priv->params);

  return bt_value_group_get_event_data_unchecked (self, tick, param);
}

/**
 * bt_value_group_set_event:
 * @self: the pattern the cell belongs to
 * @tick: the tick (time) position starting with 0
 * @param: the number of the  parameter starting with 0
 * @value: the string representation of the value to store
 *
 * Stores the supplied value into the specified pattern cell.
 *
 * Returns: %TRUE for success
 *
 * Since: 0.7
 */
gboolean
bt_value_group_set_event (const BtValueGroup * const self, const gulong tick,
    const gulong param, const gchar * const value)
{
  gboolean res = FALSE;
  GValue *event;
  GType type;

  g_return_val_if_fail (BT_IS_VALUE_GROUP (self), FALSE);
  g_return_val_if_fail (tick < self->priv->length, FALSE);
  g_return_val_if_fail (param < self->priv->params, FALSE);

  type = bt_value_group_get_param_type (self, param);
  // plain value
  if (G_TYPE_IS_ENUM (type)) {
    event = bt_value_group_get_event_data_unchecked (self, tick,
        self->priv->params + param);
    if (BT_IS_STRING (value)) {
      // set value
      if (!BT_IS_GVALUE (event)) {
        g_value_init (event, G_TYPE_INT);
      }
      bt_str_parse_gvalue (event, value);
      GST_DEBUG ("Set shadow value at: %lu,%lu: '%s'", tick, param, value);
    } else {
      // unset value
      if (BT_IS_GVALUE (event)) {
        g_value_unset (event);
      }
    }
  }
  // validated value
  event = bt_value_group_get_event_data_unchecked (self, tick, param);
  if (BT_IS_STRING (value)) {
    // set value
    if (!BT_IS_GVALUE (event)) {
      g_value_init (event, type);
    }
    if (bt_str_parse_gvalue (event, value)) {
      if (bt_parameter_group_is_param_no_value (self->priv->param_group, param,
              event)) {
        g_value_unset (event);
      } else {
        GST_DEBUG ("Set real value at: %lu,%lu: '%s'", tick, param, value);
      }
      res = TRUE;
    } else {
      g_value_unset (event);
      GST_DEBUG ("failed to set GValue for cell at tick=%lu, param=%lu", tick,
          param);
    }
  } else {
    // unset value
    if (BT_IS_GVALUE (event)) {
      g_value_unset (event);
    }
    res = TRUE;
  }
  if (res) {
    // notify others that the data has been changed
    g_signal_emit ((gpointer) self, signals[PARAM_CHANGED_EVENT], 0,
        self->priv->param_group, tick, param);
  }
  return res;
}

/**
 * bt_value_group_get_event:
 * @self: the pattern the cell belongs to
 * @tick: the tick (time) position starting with 0
 * @param: the number of the parameter starting with 0
 *
 * Returns the string representation of the specified cell. Free it when done.
 *
 * Returns: a newly allocated string with the data or %NULL on error
 *
 * Since: 0.7
 */
gchar *
bt_value_group_get_event (const BtValueGroup * const self, const gulong tick,
    const gulong param)
{
  gchar *value = NULL;
  GValue *event;

  g_return_val_if_fail (BT_IS_VALUE_GROUP (self), NULL);
  g_return_val_if_fail (tick < self->priv->length, NULL);
  g_return_val_if_fail (param < self->priv->params, NULL);

  // validated value
  event = bt_value_group_get_event_data_unchecked (self, tick, param);
  if (BT_IS_GVALUE (event)) {
    value = bt_str_format_gvalue (event);
    GST_DEBUG ("return valid value at: %lu,%lu: '%s'", tick, param, value);
  } else {
    // plain value
    event = bt_value_group_get_event_data_unchecked (self, tick,
        self->priv->params + param);
    if (BT_IS_GVALUE (event)) {
      value = bt_str_format_gvalue (event);
      GST_DEBUG ("return plain value at: %lu,%lu: '%s'", tick, param, value);
    }
  }
  return value;
}

/**
 * bt_value_group_test_event:
 * @self: the pattern the cell belongs to
 * @tick: the tick (time) position starting with 0
 * @param: the number of the parameter starting with 0
 *
 * Tests if there is an event in the specified cell.
 *
 * Returns: %TRUE if there is an event
 *
 * Since: 0.7
 */
gboolean
bt_value_group_test_event (const BtValueGroup * const self, const gulong tick,
    const gulong param)
{
  g_return_val_if_fail (BT_IS_VALUE_GROUP (self), FALSE);
  g_return_val_if_fail (tick < self->priv->length, FALSE);
  g_return_val_if_fail (param < self->priv->params, FALSE);

  const GValue *const event =
      bt_value_group_get_event_data_unchecked (self, tick, param);

  return BT_IS_GVALUE (event);
}

/**
 * bt_value_group_test_tick:
 * @self: the pattern to check
 * @tick: the tick index in the pattern
 *
 * Check if there are any event in the given pattern-row.
 *
 * Returns: %TRUE is there are events, %FALSE otherwise
 *
 * Since: 0.7
 */
gboolean
bt_value_group_test_tick (const BtValueGroup * const self, const gulong tick)
{
  const gulong params = self->priv->params;
  gulong i;
  GValue *data;

  g_return_val_if_fail (BT_IS_VALUE_GROUP (self), FALSE);
  g_return_val_if_fail (tick < self->priv->length, FALSE);

  data = &self->priv->data[tick * self->priv->columns];
  for (i = 0; i < params; i++) {
    if (BT_IS_GVALUE (data)) {
      return TRUE;
    }
    data++;
  }
  return FALSE;
}


static void
_insert_row (const BtValueGroup * const self, const gulong tick,
    const gulong param)
{
  gulong i, length = self->priv->length;
  gulong params = self->priv->columns;
  GValue *src = &self->priv->data[param + params * (length - 2)];
  GValue *dst = &self->priv->data[param + params * (length - 1)];

  GST_INFO ("insert row at %lu,%lu", tick, param);

  for (i = tick; i < length - 1; i++) {
    if (BT_IS_GVALUE (src)) {
      if (!BT_IS_GVALUE (dst))
        g_value_init (dst, G_VALUE_TYPE (src));
      g_value_copy (src, dst);
      g_value_unset (src);
    } else {
      if (BT_IS_GVALUE (dst))
        g_value_unset (dst);
    }
    src -= params;
    dst -= params;
  }
}

/**
 * bt_value_group_insert_row:
 * @self: the pattern
 * @tick: the position to insert at
 * @param: the parameter
 *
 * Insert one empty row for given @param.
 *
 * Since: 0.7
 */
void
bt_value_group_insert_row (const BtValueGroup * const self, const gulong tick,
    const gulong param)
{
  g_return_if_fail (BT_IS_VALUE_GROUP (self));
  g_return_if_fail (tick < self->priv->length);
  g_return_if_fail (self->priv->data);

  g_signal_emit ((gpointer) self, signals[GROUP_CHANGED_EVENT], 0,
      self->priv->param_group, TRUE);
  _insert_row (self, tick, param);
  g_signal_emit ((gpointer) self, signals[GROUP_CHANGED_EVENT], 0,
      self->priv->param_group, FALSE);
}

/**
 * bt_value_group_insert_full_row:
 * @self: the pattern
 * @tick: the position to insert at
 *
 * Insert one empty row for all parameters.
 *
 * Since: 0.7
 */
void
bt_value_group_insert_full_row (const BtValueGroup * const self,
    const gulong tick)
{
  g_return_if_fail (BT_IS_VALUE_GROUP (self));

  gulong j, params = self->priv->params;

  GST_DEBUG ("insert full-row at %lu", tick);

  g_signal_emit ((gpointer) self, signals[GROUP_CHANGED_EVENT], 0,
      self->priv->param_group, TRUE);
  for (j = 0; j < params; j++) {
    _insert_row (self, tick, j);
  }
  g_signal_emit ((gpointer) self, signals[GROUP_CHANGED_EVENT], 0,
      self->priv->param_group, FALSE);
}


static void
_delete_row (const BtValueGroup * const self, const gulong tick,
    const gulong param)
{
  gulong i, length = self->priv->length;
  gulong params = self->priv->columns;
  GValue *src = &self->priv->data[param + params * (tick + 1)];
  GValue *dst = &self->priv->data[param + params * tick];

  GST_INFO ("insert row at %lu,%lu", tick, param);

  for (i = tick; i < length - 1; i++) {
    if (BT_IS_GVALUE (src)) {
      if (!BT_IS_GVALUE (dst))
        g_value_init (dst, G_VALUE_TYPE (src));
      g_value_copy (src, dst);
      g_value_unset (src);
    } else {
      if (BT_IS_GVALUE (dst))
        g_value_unset (dst);
    }
    src += params;
    dst += params;
  }
}

/**
 * bt_value_group_delete_row:
 * @self: the pattern
 * @tick: the position to delete
 * @param: the parameter
 *
 * Delete row for given @param.
 *
 * Since: 0.7
 */
void
bt_value_group_delete_row (const BtValueGroup * const self, const gulong tick,
    const gulong param)
{
  g_return_if_fail (BT_IS_VALUE_GROUP (self));
  g_return_if_fail (tick < self->priv->length);
  g_return_if_fail (self->priv->data);

  g_signal_emit ((gpointer) self, signals[GROUP_CHANGED_EVENT], 0,
      self->priv->param_group, TRUE);
  _delete_row (self, tick, param);
  g_signal_emit ((gpointer) self, signals[GROUP_CHANGED_EVENT], 0,
      self->priv->param_group, FALSE);
}

/**
 * bt_value_group_delete_full_row:
 * @self: the pattern
 * @tick: the position to delete
 *
 * Delete row for all parameters.
 *
 * Since: 0.7
 */
void
bt_value_group_delete_full_row (const BtValueGroup * const self,
    const gulong tick)
{
  g_return_if_fail (BT_IS_VALUE_GROUP (self));

  gulong j, params = self->priv->params;

  GST_DEBUG ("insert full-row at %lu", tick);

  g_signal_emit ((gpointer) self, signals[GROUP_CHANGED_EVENT], 0,
      self->priv->param_group, TRUE);
  for (j = 0; j < params; j++) {
    _delete_row (self, tick, j);
  }
  g_signal_emit ((gpointer) self, signals[GROUP_CHANGED_EVENT], 0,
      self->priv->param_group, FALSE);
}


static void
_clear_column (const BtValueGroup * const self, BtValueGroupOp op,
    const gulong start_tick, const gulong end_tick, const gulong param)
{
  gulong params = self->priv->columns;
  GValue *beg = &self->priv->data[param + params * start_tick];
  gulong i, ticks = (end_tick + 1) - start_tick;

  for (i = 0; i < ticks; i++) {
    if (BT_IS_GVALUE (beg))
      g_value_unset (beg);
    beg += params;
  }
}


#define _BLEND(t,T)                                                            \
	case G_TYPE_ ## T: {                                                         \
		gdouble val=(gdouble)g_value_get_ ## t(beg);                               \
	  gdouble step=((gdouble)g_value_get_ ## t(end)-val)/(gdouble)ticks;         \
	                                                                             \
		for(i=0;i<ticks;i++) {                                                     \
			if(!BT_IS_GVALUE(beg))                                                   \
				g_value_init(beg,G_TYPE_ ## T);                                        \
			g_value_set_ ## t(beg,(g ## t)(val+(step*i)));                           \
			beg+=params;                                                             \
		}                                                                          \
	} break;

static void
_blend_column (const BtValueGroup * const self, BtValueGroupOp op,
    const gulong start_tick, const gulong end_tick, const gulong param)
{
  gulong params = self->priv->columns;
  GValue *beg = &self->priv->data[param + params * start_tick];
  GValue *end = &self->priv->data[param + params * end_tick];
  gulong i, ticks = end_tick - start_tick;
  GParamSpec *property;
  GType base_type;

  if (!BT_IS_GVALUE (beg) || !BT_IS_GVALUE (end)) {
    GST_INFO ("Can't blend, beg or end is empty");
    return;
  }
  property = bt_parameter_group_get_param_spec (self->priv->param_group, param);
  base_type = bt_g_type_get_base_type (property->value_type);

  GST_INFO ("blending gvalue type %s", g_type_name (base_type));

  // TODO(ensonic): should this honour the cursor stepping? e.g. enter only every second value

  switch (base_type) {
      _BLEND (int, INT)
        _BLEND (uint, UINT)
        _BLEND (long, LONG)
        _BLEND (ulong, ULONG)
        _BLEND (int64, INT64)
        _BLEND (uint64, UINT64)
        _BLEND (float, FLOAT)
        _BLEND (double, DOUBLE)
      case G_TYPE_BOOLEAN:
    {
      gdouble val = (gdouble) g_value_get_boolean (beg);
      gdouble step =
          ((gdouble) g_value_get_boolean (end) - val) / (gdouble) ticks;
      val += 0.5;
      for (i = 0; i < ticks; i++) {
        if (!BT_IS_GVALUE (beg))
          g_value_init (beg, G_TYPE_BOOLEAN);
        g_value_set_boolean (beg, (gboolean) (val + (step * i)));
        beg += params;
      }
    }
      break;
    case G_TYPE_ENUM:
    {
      GParamSpecEnum *p = G_PARAM_SPEC_ENUM (property);
      GEnumClass *e = p->enum_class;
      gdouble step;
      gint v, v1, v2;

      // we need the index of the enum value and the number of values inbetween
      v = g_value_get_enum (beg);
      for (v1 = 0; v1 < e->n_values; v1++) {
        if (e->values[v1].value == v)
          break;
      }
      v = g_value_get_enum (end);
      for (v2 = 0; v2 < e->n_values; v2++) {
        if (e->values[v2].value == v)
          break;
      }
      step = (gdouble) (v2 - v1) / (gdouble) ticks;
      //GST_DEBUG("v1 = %d, v2=%d, step=%lf",v1,v2,step);

      for (i = 0; i < ticks; i++) {
        if (!BT_IS_GVALUE (beg))
          g_value_init (beg, property->value_type);
        v = (gint) (v1 + (step * i));
        // handle sparse enums
        g_value_set_enum (beg, e->values[v].value);
        beg += params;
      }
    }
      break;
    default:
      GST_WARNING ("unhandled gvalue type %s", g_type_name (base_type));
  }
}


static void
_flip_column (const BtValueGroup * const self, BtValueGroupOp op,
    const gulong start_tick, const gulong end_tick, const gulong param)
{
  gulong params = self->priv->columns;
  GValue *beg = &self->priv->data[param + params * start_tick];
  GValue *end = &self->priv->data[param + params * end_tick];
  GParamSpec *property;
  GType base_type;
  GValue tmp = { 0, };

  property = bt_parameter_group_get_param_spec (self->priv->param_group, param);
  base_type = property->value_type;

  GST_INFO ("flipping gvalue type %s", g_type_name (base_type));

  g_value_init (&tmp, base_type);
  while (beg < end) {
    if (BT_IS_GVALUE (beg) && BT_IS_GVALUE (end)) {
      g_value_copy (beg, &tmp);
      g_value_copy (end, beg);
      g_value_copy (&tmp, end);
    } else if (!BT_IS_GVALUE (beg) && BT_IS_GVALUE (end)) {
      g_value_init (beg, base_type);
      g_value_copy (end, beg);
      g_value_unset (end);
    } else if (BT_IS_GVALUE (beg) && !BT_IS_GVALUE (end)) {
      g_value_init (end, base_type);
      g_value_copy (beg, end);
      g_value_unset (beg);
    }
    beg += params;
    end -= params;
  }
  g_value_unset (&tmp);
}


#define _RANDOMIZE(t,T,p)                                                      \
	case G_TYPE_ ## T: {                                                         \
      const GParamSpec ## p *p=G_PARAM_SPEC_ ## T(property);                   \
      g ## t d = p->maximum-p->minimum;                                        \
      for(i=0;i<ticks;i++) {                                                   \
        if(!BT_IS_GVALUE(beg))                                                 \
          g_value_init(beg,G_TYPE_ ## T);                                      \
        rnd=((gdouble)rand())/(RAND_MAX+1.0);                                  \
        g_value_set_ ## t(beg,(g ## t)(p->minimum+(d*rnd)));                   \
        beg+=params;                                                           \
      }                                                                        \
    } break;

static void
_randomize_column (const BtValueGroup * const self, BtValueGroupOp op,
    const gulong start_tick, const gulong end_tick, const gulong param)
{
  gulong params = self->priv->columns;
  GValue *beg = &self->priv->data[param + params * start_tick];
  gulong i, ticks = (end_tick + 1) - start_tick;
  GParamSpec *property;
  GType base_type;
  gdouble rnd;

  property = bt_parameter_group_get_param_spec (self->priv->param_group, param);
  base_type = bt_g_type_get_base_type (property->value_type);

  GST_INFO ("randomizing gvalue type %s", g_type_name (base_type));

  // TODO(ensonic): should this honour the cursor stepping? e.g. enter only every second value

  switch (base_type) {
      _RANDOMIZE (int, INT, Int)
        _RANDOMIZE (uint, UINT, UInt)
        _RANDOMIZE (long, LONG, Long)
        _RANDOMIZE (ulong, ULONG, ULong)
        _RANDOMIZE (int64, INT64, Int64)
        _RANDOMIZE (uint64, UINT64, UInt64)
        _RANDOMIZE (float, FLOAT, Float)
        _RANDOMIZE (double, DOUBLE, Double)
      case G_TYPE_BOOLEAN:
    {
      for (i = 0; i < ticks; i++) {
        if (!BT_IS_GVALUE (beg))
          g_value_init (beg, G_TYPE_BOOLEAN);
        rnd = ((gdouble) rand ()) / (RAND_MAX + 1.0);
        g_value_set_boolean (beg, (gboolean) (2 * rnd));
        beg += params;
      }
      break;
    }
    case G_TYPE_ENUM:{
      const GParamSpecEnum *p = G_PARAM_SPEC_ENUM (property);
      const GEnumClass *e = p->enum_class;
      gint d = e->n_values - 1; // don't use no_value
      gint v;

      for (i = 0; i < ticks; i++) {
        if (!BT_IS_GVALUE (beg))
          g_value_init (beg, property->value_type);
        rnd = ((gdouble) rand ()) / (RAND_MAX + 1.0);
        v = (gint) (d * rnd);
        // handle sparse enums
        g_value_set_enum (beg, e->values[v].value);
        beg += params;
      }
      break;
    }
    default:
      GST_WARNING ("unhandled gvalue type %s", g_type_name (base_type));
  }
}


#define _RANGE_RANDOMIZE(t,T)                                                  \
	case G_TYPE_ ## T: {                                                         \
      g ## t mi = g_value_get_ ## t(beg);                                      \
      g ## t ma = g_value_get_ ## t(end);                                      \
      if (ma < mi) {                                                           \
        g ## t d = ma;                                                         \
        ma = mi;                                                               \
        mi = d;                                                                \
      }                                                                        \
      g ## t d = ma - mi;                                                      \
      for(i=0;i<ticks;i++) {                                                   \
        if(!BT_IS_GVALUE(beg))                                                 \
          g_value_init(beg,G_TYPE_ ## T);                                      \
        rnd=((gdouble)rand())/(RAND_MAX+1.0);                                  \
        g_value_set_ ## t(beg,(g ## t)(mi+(d*rnd)));                           \
        beg+=params;                                                           \
      }                                                                        \
    } break;

static void
_range_randomize_column (const BtValueGroup * const self, BtValueGroupOp op,
    const gulong start_tick, const gulong end_tick, const gulong param)
{
  gulong params = self->priv->columns;
  GValue *beg = &self->priv->data[param + params * start_tick];
  GValue *end = &self->priv->data[param + params * end_tick];
  gulong i, ticks = (end_tick + 1) - start_tick;
  GParamSpec *property;
  GType base_type;
  gdouble rnd;

  if (!BT_IS_GVALUE (beg) || !BT_IS_GVALUE (end)) {
    if (!BT_IS_GVALUE (beg)) {
      GST_INFO ("Can't ranged randomize, beg is empty");
    }
    if (!BT_IS_GVALUE (end)) {
      GST_INFO ("Can't ranged randomize, end is empty");
    }
    return;
  }

  property = bt_parameter_group_get_param_spec (self->priv->param_group, param);
  base_type = bt_g_type_get_base_type (property->value_type);

  GST_INFO ("randomizing ranged gvalue type %s", g_type_name (base_type));

  // TODO(ensonic): should this honour the cursor stepping? e.g. enter only every second value
  // TODO(ensonic): if beg and end are not empty, shall we use them as upper and lower
  // bounds instead of the pspec values (ev. have a flag on the function)

  switch (base_type) {
      _RANGE_RANDOMIZE (int, INT)
        _RANGE_RANDOMIZE (uint, UINT)
        _RANGE_RANDOMIZE (long, LONG)
        _RANGE_RANDOMIZE (ulong, ULONG)
        _RANGE_RANDOMIZE (int64, INT64)
        _RANGE_RANDOMIZE (uint64, UINT64)
        _RANGE_RANDOMIZE (float, FLOAT)
        _RANGE_RANDOMIZE (double, DOUBLE)
      case G_TYPE_BOOLEAN:
    {
      for (i = 0; i < ticks; i++) {
        if (!BT_IS_GVALUE (beg))
          g_value_init (beg, G_TYPE_BOOLEAN);
        rnd = ((gdouble) rand ()) / (RAND_MAX + 1.0);
        g_value_set_boolean (beg, (gboolean) (2 * rnd));
        beg += params;
      }
      break;
    }
    case G_TYPE_ENUM:{
      const GParamSpecEnum *p = G_PARAM_SPEC_ENUM (property);
      const GEnumClass *e = p->enum_class;
      gint mi = g_value_get_enum (beg);
      for (i = 0; i < e->n_values; i++) {
        if (e->values[i].value == mi) {
          mi = i;
          break;
        }
      }
      gint ma = g_value_get_enum (end);
      for (i = 0; i < e->n_values; i++) {
        if (e->values[i].value == ma) {
          ma = i;
          break;
        }
      }
      if (ma < mi) {
        gint d = ma;
        ma = mi;
        mi = d;
      }
      gint d = ma - mi;
      gint v;

      for (i = 0; i < ticks; i++) {
        if (!BT_IS_GVALUE (beg))
          g_value_init (beg, property->value_type);
        rnd = ((gdouble) rand ()) / (RAND_MAX + 1.0);
        v = (gint) (d * rnd);
        // handle sparse enums
        g_value_set_enum (beg, e->values[mi + v].value);
        beg += params;
      }
      break;
    }
    default:
      GST_WARNING ("unhandled gvalue type %s", g_type_name (base_type));
  }
}


#define _TRANSPOSE_INT(t,T,p)                                                  \
	case G_TYPE_ ## T: {                                                         \
      const GParamSpec ## p *p=G_PARAM_SPEC_ ## T(property);                   \
      g ## t v;                                                                \
      step = dir * (fine ? 1.0 : ((p->maximum - p->minimum) / 16.0));          \
      for(i=0;i<ticks;i++) {                                                   \
        if(BT_IS_GVALUE(beg)) {                                                \
          v = g_value_get_ ## t(beg);                                          \
          if (step < 0) {                                                      \
            if (v >= (p->minimum - step)) {                                    \
              g_value_set_ ## t(beg,(g ## t) (v + step));                      \
            }                                                                  \
          } else {                                                             \
            if (v <= (p->maximum - step)) {                                    \
              g_value_set_ ## t(beg,(g ## t) (v + step));                      \
            }                                                                  \
          }                                                                    \
        }                                                                      \
        beg+=params;                                                           \
      }                                                                        \
    } break;

#define _TRANSPOSE_FLT(t,T,p)                                                  \
	case G_TYPE_ ## T: {                                                         \
      const GParamSpec ## p *p=G_PARAM_SPEC_ ## T(property);                   \
      g ## t v;                                                                \
      step = (dir * (p->maximum - p->minimum)) / (fine ? 65535.0 : 16.0);      \
      for(i=0;i<ticks;i++) {                                                   \
        if(BT_IS_GVALUE(beg)) {                                                \
          v = g_value_get_ ## t(beg);                                          \
          if (step < 0) {                                                      \
            if (v >= (p->minimum - step)) {                                    \
              g_value_set_ ## t(beg,(g ## t) (v + step));                      \
            }                                                                  \
          } else {                                                             \
            if (v <= (p->maximum - step)) {                                    \
              g_value_set_ ## t(beg,(g ## t) (v + step));                      \
            }                                                                  \
          }                                                                    \
        }                                                                      \
        beg+=params;                                                           \
      }                                                                        \
    } break;

static void
_transpose_column (const BtValueGroup * const self, BtValueGroupOp op,
    const gulong start_tick, const gulong end_tick, const gulong param)
{
  gulong params = self->priv->columns;
  GValue *beg = &self->priv->data[param + params * start_tick];
  gulong i, ticks = (end_tick + 1) - start_tick;
  GParamSpec *property;
  GType base_type;
  gdouble step, dir;
  gboolean fine;

  switch (op) {
    case BT_VALUE_GROUP_OP_TRANSPOSE_FINE_UP:
      dir = 1.0;
      fine = TRUE;
      break;
    case BT_VALUE_GROUP_OP_TRANSPOSE_FINE_DOWN:
      dir = -1.0;
      fine = TRUE;
      break;
    case BT_VALUE_GROUP_OP_TRANSPOSE_COARSE_UP:
      dir = 1.0;
      fine = FALSE;
      break;
    case BT_VALUE_GROUP_OP_TRANSPOSE_COARSE_DOWN:
      dir = -1.0;
      fine = FALSE;
      break;
    default:
      g_assert_not_reached ();
  }

  property = bt_parameter_group_get_param_spec (self->priv->param_group, param);
  base_type = bt_g_type_get_base_type (property->value_type);

  GST_INFO ("transposing gvalue type %s", g_type_name (base_type));

  switch (base_type) {
      _TRANSPOSE_INT (int, INT, Int)
        _TRANSPOSE_INT (uint, UINT, UInt)
        _TRANSPOSE_INT (long, LONG, Long)
        _TRANSPOSE_INT (ulong, ULONG, ULong)
        _TRANSPOSE_INT (int64, INT64, Int64)
        _TRANSPOSE_INT (uint64, UINT64, UInt64)
        _TRANSPOSE_FLT (float, FLOAT, Float)
        _TRANSPOSE_FLT (double, DOUBLE, Double)
      case G_TYPE_BOOLEAN:
    {
      gboolean v;

      for (i = 0; i < ticks; i++) {
        if (BT_IS_GVALUE (beg)) {
          v = g_value_get_boolean (beg);
          if (v == FALSE && dir > 0) {
            g_value_set_boolean (beg, TRUE);
          }
          if (v == TRUE && dir < 0) {
            g_value_set_boolean (beg, FALSE);
          }
        }
      }
      break;
    }
    case G_TYPE_ENUM:{
      const GParamSpecEnum *p = G_PARAM_SPEC_ENUM (property);
      const GEnumClass *e = p->enum_class;
      gint d = e->n_values - 1; // don't use no_value
      gint v, ev;

      if (fine) {
        step = 1.0;
      } else {
        if (property->value_type == GSTBT_TYPE_NOTE) {
          step = 12;
        } else {
          step = (gint) (0.5 + (d / 16.0));
        }
      }
      step *= dir;

      for (i = 0; i < ticks; i++) {
        if (BT_IS_GVALUE (beg)) {
          ev = g_value_get_enum (beg);
          for (v = 0; v < d; v++) {
            if (e->values[v].value == ev) {
              break;
            }
          }
          v += (gint) step;
          if ((v >= 0) && (v <= d)) {
            g_value_set_enum (beg, e->values[v].value);
          }
        }
        beg += params;
      }
      break;
    }
    default:
      GST_WARNING ("unhandled gvalue type %s", g_type_name (base_type));
  }
}


typedef void (*OpFunc) (const BtValueGroup * const self, BtValueGroupOp op,
    const gulong start_tick, const gulong end_tick, const gulong param);

static OpFunc ops[BT_VALUE_GROUP_OP_COUNT] = {
  _clear_column,
  _blend_column,
  _flip_column,
  _randomize_column,
  _range_randomize_column,
  _transpose_column,
  _transpose_column,
  _transpose_column,
  _transpose_column
};

/**
 * bt_value_group_transform_colum:
 * @self: the value group
 * @op: the transform operation
 * @start_tick: the start position for the range
 * @end_tick: the end position for the range
 * @param: the parameter
 *
 * Applies @op to values from @start_tick to @end_tick for @param.
 *
 * Since: 0.12
 */
void
bt_value_group_transform_colum (const BtValueGroup * const self,
    BtValueGroupOp op, const gulong start_tick, const gulong end_tick,
    const gulong param)
{
  g_return_if_fail (BT_IS_VALUE_GROUP (self));
  g_return_if_fail (start_tick < self->priv->length);
  g_return_if_fail (end_tick < self->priv->length);
  g_return_if_fail (self->priv->data);

  if (op == BT_VALUE_GROUP_OP_CLEAR) {
    g_signal_emit ((gpointer) self, signals[GROUP_CHANGED_EVENT], 0,
        self->priv->param_group, TRUE);
  }
  ops[op] (self, op, start_tick, end_tick, param);
  g_signal_emit ((gpointer) self, signals[GROUP_CHANGED_EVENT], 0,
      self->priv->param_group, FALSE);

}

/**
 * bt_value_group_transform_colums:
 * @self: the value group
 * @op: the transform operation
 * @start_tick: the start position for the range
 * @end_tick: the end position for the range
 *
 * Applies @op to values from @start_tick to @end_tick for all params.
 *
 * Since: 0.12
 */
void
bt_value_group_transform_colums (const BtValueGroup * const self,
    BtValueGroupOp op, const gulong start_tick, const gulong end_tick)
{
  g_return_if_fail (BT_IS_VALUE_GROUP (self));
  g_return_if_fail (start_tick < self->priv->length);
  g_return_if_fail (end_tick < self->priv->length);
  g_return_if_fail (self->priv->data);

  gulong j, params = self->priv->params;
  OpFunc opfunc = ops[op];

  if (op == BT_VALUE_GROUP_OP_CLEAR) {
    g_signal_emit ((gpointer) self, signals[GROUP_CHANGED_EVENT], 0,
        self->priv->param_group, TRUE);
  }
  for (j = 0; j < params; j++) {
    opfunc (self, op, start_tick, end_tick, j);
  }
  g_signal_emit ((gpointer) self, signals[GROUP_CHANGED_EVENT], 0,
      self->priv->param_group, FALSE);

}


static void
_serialize_column (const BtValueGroup * const self, const gulong start_tick,
    const gulong end_tick, const gulong param, GString * data)
{
  gulong params = self->priv->columns;
  GValue *beg = &self->priv->data[param + params * start_tick];
  gulong i, ticks = (end_tick + 1) - start_tick;
  gchar *val;

  g_string_append (data,
      g_type_name (bt_value_group_get_param_type (self, param)));
  for (i = 0; i < ticks; i++) {
    if (BT_IS_GVALUE (beg)) {
      if ((val = bt_str_format_gvalue (beg))) {
        g_string_append_c (data, ',');
        g_string_append (data, val);
        g_free (val);
      }
    } else {
      // empty cell
      g_string_append (data, ", ");
    }
    beg += params;
  }
  g_string_append_c (data, '\n');
}

/**
 * bt_value_group_serialize_column:
 * @self: the value group
 * @start_tick: the start position for the range
 * @end_tick: the end position for the range
 * @param: the parameter
 * @data: the target
 *
 * Serializes values from @start_tick to @end_tick for @param into @data.
 *
 * Since: 0.7
 */
void
bt_value_group_serialize_column (const BtValueGroup * const self,
    const gulong start_tick, const gulong end_tick, const gulong param,
    GString * data)
{
  g_return_if_fail (BT_IS_VALUE_GROUP (self));
  g_return_if_fail (start_tick < self->priv->length);
  g_return_if_fail (end_tick < self->priv->length);
  g_return_if_fail (self->priv->data);
  g_return_if_fail (data);

  _serialize_column (self, start_tick, end_tick, param, data);
}

/**
 * bt_value_group_serialize_columns:
 * @self: the value group
 * @start_tick: the start position for the range
 * @end_tick: the end position for the range
 * @data: the target
 *
 * Serializes values from @start_tick to @end_tick for all params into @data.
 *
 * Since: 0.7
 */
void
bt_value_group_serialize_columns (const BtValueGroup * const self,
    const gulong start_tick, const gulong end_tick, GString * data)
{
  g_return_if_fail (BT_IS_VALUE_GROUP (self));
  g_return_if_fail (start_tick < self->priv->length);
  g_return_if_fail (end_tick < self->priv->length);
  g_return_if_fail (data);

  gulong j, params = self->priv->params;

  for (j = 0; j < params; j++) {
    _serialize_column (self, start_tick, end_tick, j, data);
  }
}

/**
 * bt_value_group_deserialize_column:
 * @self: the value group
 * @start_tick: the start position for the range
 * @end_tick: the end position for the range
 * @param: the parameter
 * @data: the source data
 *
 * Deserializes values to @start_tick to @end_tick for @param from @data.
 *
 * Returns: %TRUE for success, %FALSE e.g. to indicate incompatible GType values
 * for the column specified by @param and the @data.
 *
 * Since: 0.7
 */
gboolean
bt_value_group_deserialize_column (const BtValueGroup * const self,
    const gulong start_tick, const gulong end_tick, const gulong param,
    const gchar * data)
{
  g_return_val_if_fail (BT_IS_VALUE_GROUP (self), FALSE);
  g_return_val_if_fail (start_tick < self->priv->length, FALSE);
  g_return_val_if_fail (end_tick < self->priv->length, FALSE);
  g_return_val_if_fail (self->priv->data, FALSE);
  g_return_val_if_fail (data, FALSE);
  g_return_val_if_fail (param < self->priv->params, FALSE);

  gboolean ret = TRUE;
  gchar **fields = g_strsplit_set (data, ",", 0);
  GType dtype = bt_value_group_get_param_type (self, param);
  GType stype = g_type_from_name (fields[0]);

  if (dtype == stype) {
    gint i = 1;
    gulong params = self->priv->columns;
    GValue *beg = &self->priv->data[param + params * start_tick];
    GValue *end = &self->priv->data[param + params * end_tick];

    GST_INFO ("types match %s <-> %s", fields[0], g_type_name (dtype));

    while (fields[i] && *fields[i] && (beg <= end)) {
      if (*fields[i] != ' ') {
        if (!BT_IS_GVALUE (beg)) {
          g_value_init (beg, dtype);
        }
        bt_str_parse_gvalue (beg, fields[i]);
      } else {
        if (BT_IS_GVALUE (beg)) {
          g_value_unset (beg);
        }
      }
      beg += params;
      i++;
    }
  } else {
    GST_INFO ("types don't match in %s <-> %s", fields[0], g_type_name (dtype));
    ret = FALSE;
  }
  g_strfreev (fields);
  return ret;
}

//-- g_object overrides

static void
bt_value_group_constructed (GObject * object)
{
  BtValueGroup *const self = BT_VALUE_GROUP (object);

  if (G_OBJECT_CLASS (bt_value_group_parent_class)->constructed)
    G_OBJECT_CLASS (bt_value_group_parent_class)->constructed (object);

  bt_value_group_resize_data_length (self, 0);
}

static void
bt_value_group_set_property (GObject * const object, const guint property_id,
    const GValue * const value, GParamSpec * const pspec)
{
  const BtValueGroup *const self = BT_VALUE_GROUP (object);
  return_if_disposed ();

  switch (property_id) {
    case VALUE_GROUP_PARAMETER_GROUP:{
      self->priv->param_group = BT_PARAMETER_GROUP (g_value_dup_object (value));
      g_object_get ((gpointer) (self->priv->param_group), "num-params",
          &self->priv->params, NULL);
      self->priv->columns = self->priv->params * 2;
      break;
    }
    case VALUE_GROUP_LENGTH:{
      gulong length = self->priv->length;
      self->priv->length = g_value_get_ulong (value);
      if (length != self->priv->length) {
        bt_value_group_resize_data_length (self, length);
      }
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_value_group_get_property (GObject * const object, const guint property_id,
    GValue * const value, GParamSpec * const pspec)
{
  const BtValueGroup *const self = BT_VALUE_GROUP (object);
  return_if_disposed ();

  switch (property_id) {
    case VALUE_GROUP_PARAMETER_GROUP:
      g_value_set_object (value, self->priv->param_group);
      break;
    case VALUE_GROUP_LENGTH:
      g_value_set_ulong (value, self->priv->length);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_value_group_dispose (GObject * const object)
{
  const BtValueGroup *const self = BT_VALUE_GROUP (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  g_object_unref (self->priv->param_group);

  G_OBJECT_CLASS (bt_value_group_parent_class)->dispose (object);
}

static void
bt_value_group_finalize (GObject * const object)
{
  const BtValueGroup *const self = BT_VALUE_GROUP (object);
  GValue *v;
  gulong i;

  if (self->priv->data) {
    const gulong data_count = self->priv->length * self->priv->columns;
    // free gvalues in self->priv->data
    for (i = 0; i < data_count; i++) {
      v = &self->priv->data[i];
      if (BT_IS_GVALUE (v))
        g_value_unset (v);
    }
    g_free (self->priv->data);
  }

  G_OBJECT_CLASS (bt_value_group_parent_class)->finalize (object);
}

//-- class internals

static void
bt_value_group_init (BtValueGroup * self)
{
  GST_DEBUG ("!!!! self=%p", self);
  self->priv = bt_value_group_get_instance_private(self);
}

static void
bt_value_group_class_init (BtValueGroupClass * const klass)
{
  GObjectClass *const gobject_class = G_OBJECT_CLASS (klass);

  GST_DEBUG ("!!!!");

  gobject_class->constructed = bt_value_group_constructed;
  gobject_class->set_property = bt_value_group_set_property;
  gobject_class->get_property = bt_value_group_get_property;
  gobject_class->dispose = bt_value_group_dispose;
  gobject_class->finalize = bt_value_group_finalize;

  /**
   * BtValueGroup::param-changed:
   * @self: the value-group object that emitted the signal
   * @param_group: the parameter group
   * @tick: the tick position inside the pattern
   * @param: the parameter index
   *
   * Signals that a param of this value-group has been changed.
   */
  signals[PARAM_CHANGED_EVENT] =
      g_signal_new ("param-changed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 0, NULL,
      NULL, bt_marshal_VOID__OBJECT_ULONG_ULONG, G_TYPE_NONE, 3,
      BT_TYPE_PARAMETER_GROUP, G_TYPE_ULONG, G_TYPE_ULONG);

  /**
   * BtValueGroup::group-changed:
   * @self: the value-group object that emitted the signal
   * @param_group: the related #BtParameterGroup
   * @intermediate: flag that is %TRUE to signal that more change are coming
   *
   * Signals that this value-group has been changed (more than in one place).
   * When doing e.g. line inserts, one will receive two updates, one before and
   * one after. The first will have @intermediate=%TRUE. Applications can use
   * that to defer change-consolidation.
   */
  signals[GROUP_CHANGED_EVENT] =
      g_signal_new ("group-changed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 0, NULL,
      NULL, bt_marshal_VOID__OBJECT_BOOLEAN, G_TYPE_NONE, 2,
      BT_TYPE_PARAMETER_GROUP, G_TYPE_BOOLEAN);

  g_object_class_install_property (gobject_class, VALUE_GROUP_PARAMETER_GROUP,
      g_param_spec_object ("parameter-group", "parameter group construct prop",
          "Parameter group for the values", BT_TYPE_PARAMETER_GROUP,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, VALUE_GROUP_LENGTH,
      g_param_spec_ulong ("length",
          "length prop",
          "length of the pattern in ticks",
          0,
          G_MAXULONG,
          1, G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}
