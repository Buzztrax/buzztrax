/* Buzztard
 * Copyright (C) 2012 Buzztard team <buzztard-devel@lists.sf.net>
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
 * SECTION:btparametergroup
 * @short_description: a group of parameter
 *
 * A group of parameters, such as used in machines or wires. Once created the
 * group will not change.
 */

/* FIXME(ensonic): consider to have a BtParameter struct and merge 7 arrays into
 * one
 * - for this we need to copy the values from parents & params though
 */
#define BT_CORE
#define BT_PARAMETER_GROUP_C

#include "core_private.h"

//-- property ids

enum
{
  PARAMETER_GROUP_NUM_PARAMS = 1,
  PARAMETER_GROUP_PARENTS,
  PARAMETER_GROUP_PARAMS
};

struct _BtParameterGroupPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the number of parameter in the group */
  gulong num_params;

  /* parameter data */
  GObject **parents;
  GParamSpec **params;
  guint *flags;                 // only used for wave_index and is_trigger
  GValue *no_val;
  GQuark *quarks;
  GstController **controller;
  GstInterpolationControlSource **control_sources;
};

//-- the class

G_DEFINE_TYPE (BtParameterGroup, bt_parameter_group, G_TYPE_OBJECT);

//-- macros

#define PARAM_NAME(ix) self->priv->params[ix]->name
#define PARAM_TYPE(ix) self->priv->params[ix]->value_type

//-- helper

/*
 * bt_parameter_group_get_property_meta_value:
 * @value: the value that will hold the result
 * @property: the paramspec object to get the meta data from
 * @key: the meta data key
 *
 * Fetches the meta data from the given paramspec object and sets the value.
 * The values needs to be initialized to the correct type.
 *
 * Returns: %TRUE if it could get the value
 */
static gboolean
bt_parameter_group_get_property_meta_value (GValue * const value,
    GParamSpec * const property, const GQuark key)
{
  gboolean res = FALSE;
  gconstpointer const has_meta =
      g_param_spec_get_qdata (property, gstbt_property_meta_quark);

  if (has_meta) {
    gconstpointer const qdata = g_param_spec_get_qdata (property, key);

    // it can be that qdata is NULL if the value is NULL
    //if(!qdata) {
    //  GST_WARNING_OBJECT(self,"no property metadata for '%s'",property->name);
    //  return(FALSE);
    //}

    res = TRUE;
    g_value_init (value, property->value_type);
    switch (bt_g_type_get_base_type (property->value_type)) {
      case G_TYPE_BOOLEAN:
        // TODO(ensonic): this does not work, for no_value it results in
        // g_value_set_boolean(value,255);
        // which is the same as TRUE
        g_value_set_boolean (value, GPOINTER_TO_INT (qdata));
        break;
      case G_TYPE_INT:
        g_value_set_int (value, GPOINTER_TO_INT (qdata));
        break;
      case G_TYPE_UINT:
        g_value_set_uint (value, GPOINTER_TO_UINT (qdata));
        break;
      case G_TYPE_ENUM:
        g_value_set_enum (value, GPOINTER_TO_INT (qdata));
        break;
      case G_TYPE_STRING:
        /* what is in qdata for this type? for buzz this is a note, so its an int
           if(qdata) {
           g_value_set_string(value,qdata);
           }
           else {
           g_value_set_static_string(value,"");
           }
         */
        g_value_set_static_string (value, "");
        break;
      default:
        if (qdata) {
          GST_WARNING ("unsupported GType for param %s", property->name);
          //GST_WARNING("unsupported GType=%d:'%s'",property->value_type,G_VALUE_TYPE_NAME(property->value_type));
          res = FALSE;
        }
    }
  }
  return (res);
}

#define _RANDOMIZE(t,T,p)                                                      \
	case G_TYPE_ ## T:{                                                          \
		const GParamSpec ## p *p=G_PARAM_SPEC_ ## T(property);                     \
		g_object_set(self,n,(g ## t)(p->minimum+((p->maximum-p->minimum)*rnd)),NULL); \
	} break;

static void
bt_g_object_randomize_parameter (GObject * self, GParamSpec * property)
{
  gdouble rnd = ((gdouble) rand ()) / (RAND_MAX + 1.0);
  const gchar *n = property->name;

  GST_DEBUG ("set random value for property: %s (type is %s)", n,
      G_PARAM_SPEC_TYPE_NAME (property));

  switch (bt_g_type_get_base_type (property->value_type)) {
      _RANDOMIZE (int, INT, Int)
        _RANDOMIZE (uint, UINT, UInt)
        _RANDOMIZE (int64, INT64, Int64)
        _RANDOMIZE (uint64, UINT64, UInt64)
        _RANDOMIZE (long, LONG, Long)
        _RANDOMIZE (ulong, ULONG, ULong)
        _RANDOMIZE (float, FLOAT, Float)
        _RANDOMIZE (double, DOUBLE, Double)
      case G_TYPE_BOOLEAN:
    {
      g_object_set (self, n, (gboolean) (2 * rnd), NULL);
    }
      break;
    case G_TYPE_ENUM:{
      const GParamSpecEnum *p = G_PARAM_SPEC_ENUM (property);
      const GEnumClass *e = p->enum_class;
      gint nv = e->n_values - 1;        // don't use no_value
      gint v = nv * rnd;

      // handle sparse enums
      g_object_set (self, n, e->values[v].value, NULL);
    } break;
    default:
      GST_WARNING ("incomplete implementation for GParamSpec type '%s'",
          G_PARAM_SPEC_TYPE_NAME (property));
  }
}

//-- handler

//-- constructor methods

/**
 * bt_parameter_group_new:
 * @num_params: the number of parameters
 * @parents: array of parent #GObjects for each parameter
 * @params: array of #GParamSpecs for each parameter
 *
 * Create a parameter group.
 *
 * Returns: the new parameter group
 */
BtParameterGroup *
bt_parameter_group_new (gulong num_params, GObject ** parents,
    GParamSpec ** params)
{
  return (BT_PARAMETER_GROUP (g_object_new (BT_TYPE_PARAMETER_GROUP,
              "num-params", num_params, "parents", parents, "params", params,
              NULL)));
}

//-- methods

/**
 * bt_parameter_group_is_param_trigger:
 * @self: the parameter group to check params from
 * @index: the offset in the list of params
 *
 * Tests if the param is a trigger param
 * (like a key-note or a drum trigger).
 *
 * Returns: %TRUE if it is a trigger
 */
gboolean
bt_parameter_group_is_param_trigger (const BtParameterGroup * const self,
    const gulong index)
{
  g_return_val_if_fail (BT_IS_PARAMETER_GROUP (self), FALSE);
  g_return_val_if_fail (index < self->priv->num_params, FALSE);

  if (!(self->priv->flags[index] & GSTBT_PROPERTY_META_STATE))
    return (TRUE);
  return (FALSE);
}

/**
 * bt_parameter_group_is_param_no_value:
 * @self: the parameter group to check params from
 * @index: the offset in the list of params
 * @value: the value to compare against the no-value
 *
 * Tests if the given value is the no-value of the param
 *
 * Returns: %TRUE if it is the no-value
 */
gboolean
bt_parameter_group_is_param_no_value (const BtParameterGroup * const self,
    const gulong index, GValue * const value)
{
  g_return_val_if_fail (BT_IS_PARAMETER_GROUP (self), FALSE);
  g_return_val_if_fail (index < self->priv->num_params, FALSE);
  g_return_val_if_fail (G_IS_VALUE (value), FALSE);

  if (!BT_IS_GVALUE (&self->priv->no_val[index]))
    return (FALSE);

  if (gst_value_compare (&self->priv->no_val[index], value) == GST_VALUE_EQUAL)
    return (TRUE);
  return (FALSE);
}


/**
 * bt_parameter_group_get_param_index:
 * @self: the parameter group to search for the param
 * @name: the name of the param
 *
 * Searches the list of registered param of a machine for a param of
 * the given name and returns the index if found.
 *
 * Returns: the index if found or returns -1.
 */
glong
bt_parameter_group_get_param_index (const BtParameterGroup * const self,
    const gchar * const name)
{
  const gulong params = self->priv->num_params;
  gulong i;

  g_return_val_if_fail (BT_IS_PARAMETER_GROUP (self), -1);
  g_return_val_if_fail (BT_IS_STRING (name), -1);

  for (i = 0; i < params; i++) {
    if (!strcmp (PARAM_NAME (i), name)) {
      return (glong) i;
    }
  }
  return -1;
}


/**
 * bt_parameter_group_get_param_spec:
 * @self: the parameter group to search for the param
 * @index: the offset in the list of params
 *
 * Retrieves the parameter specification for the param
 *
 * Returns: the #GParamSpec for the requested param
 */
GParamSpec *
bt_parameter_group_get_param_spec (const BtParameterGroup * const self,
    const gulong index)
{
  g_return_val_if_fail (BT_IS_PARAMETER_GROUP (self), NULL);
  g_return_val_if_fail (index < self->priv->num_params, NULL);

  return (self->priv->params[index]);
}

/**
 * bt_parameter_group_get_param_parent:
 * @self: the parameter group to search for the param
 * @index: the offset in the list of params
 *
 * Retrieves the owner instance for the param
 *
 * Returns: the #GParamSpec for the requested param
 */
GObject *
bt_parameter_group_get_param_parent (const BtParameterGroup * const self,
    const gulong index)
{
  g_return_val_if_fail (BT_IS_PARAMETER_GROUP (self), NULL);
  g_return_val_if_fail (index < self->priv->num_params, NULL);

  return (self->priv->parents[index]);
}


#define _DETAILS(t,T,p)                                                        \
	case G_TYPE_ ## T: {                                                         \
		const GParamSpec ## p *p=G_PARAM_SPEC_ ## T(property);                     \
		if(min_val) g_value_set_ ## t(*min_val,p->minimum);                        \
		if(max_val) g_value_set_ ## t(*max_val,p->maximum);                        \
	} break;

/**
 * bt_parameter_group_get_param_details:
 * @self: the parameter group to search for the param details
 * @index: the offset in the list of params
 * @pspec: place for the param spec
 * @min_val: place to hold new GValue with minimum
 * @max_val: place to hold new GValue with maximum
 *
 * Retrieves the details of a param. Any detail can be %NULL if its not
 * wanted.
 */
void
bt_parameter_group_get_param_details (const BtParameterGroup * const self,
    const gulong index, GParamSpec ** pspec, GValue ** min_val,
    GValue ** max_val)
{
  GParamSpec *property = self->priv->params[index];

  if (pspec) {
    *pspec = property;
  }
  if (min_val || max_val) {
    GType base_type = bt_g_type_get_base_type (property->value_type);
    gboolean done = FALSE;

    if (min_val)
      *min_val = g_new0 (GValue, 1);
    if (max_val)
      *max_val = g_new0 (GValue, 1);
    if (GSTBT_IS_PROPERTY_META (self->priv->parents[index])) {
      if (min_val)
        done =
            bt_parameter_group_get_property_meta_value (*min_val, property,
            gstbt_property_meta_quark_min_val);
      if (max_val) {
        if (!bt_parameter_group_get_property_meta_value (*max_val, property,
                gstbt_property_meta_quark_max_val)) {
          // if this failed max val has not been set
          if (done)
            g_value_unset (*min_val);
          done = FALSE;
        }
      }
    }
    if (!done) {
      if (min_val)
        g_value_init (*min_val, property->value_type);
      if (max_val)
        g_value_init (*max_val, property->value_type);
      switch (base_type) {
          _DETAILS (int, INT, Int)
            _DETAILS (uint, UINT, UInt)
            _DETAILS (int64, INT64, Int64)
            _DETAILS (uint64, UINT64, UInt64)
            _DETAILS (long, LONG, Long)
            _DETAILS (ulong, ULONG, ULong)
            _DETAILS (float, FLOAT, Float)
            _DETAILS (double, DOUBLE, Double)
          case G_TYPE_BOOLEAN:if (min_val)
              g_value_set_boolean (*min_val, 0);
          if (max_val)
            g_value_set_boolean (*max_val, 0);
          break;
        case G_TYPE_ENUM:{
          const GParamSpecEnum *enum_property = G_PARAM_SPEC_ENUM (property);
          const GEnumClass *enum_class = enum_property->enum_class;
          if (min_val)
            g_value_set_enum (*min_val, enum_class->minimum);
          if (max_val)
            g_value_set_enum (*max_val, enum_class->maximum);
        }
          break;
        case G_TYPE_STRING:
          // nothing to do for this
          break;
        default:
          GST_ERROR_OBJECT (self, "unsupported GType=%lu:'%s' ('%s')",
              (gulong) property->value_type, g_type_name (property->value_type),
              g_type_name (base_type));
      }
    }
  }
}

/**
 * bt_parameter_group_get_param_type:
 * @self: the parameter group to search for the param type
 * @index: the offset in the list of params
 *
 * Retrieves the GType of a param
 *
 * Returns: the requested GType
 */
GType
bt_parameter_group_get_param_type (const BtParameterGroup * const self,
    const gulong index)
{
  g_return_val_if_fail (BT_IS_PARAMETER_GROUP (self), G_TYPE_INVALID);
  g_return_val_if_fail (index < self->priv->num_params, G_TYPE_INVALID);

  return (PARAM_TYPE (index));
}

/**
 * bt_parameter_group_get_param_name:
 * @self: the parameter group to get the param name from
 * @index: the offset in the list of params
 *
 * Gets the param name. Do not modify returned content.
 *
 * Returns: the requested name
 */
const gchar *
bt_parameter_group_get_param_name (const BtParameterGroup * const self,
    const gulong index)
{
  g_return_val_if_fail (BT_IS_PARAMETER_GROUP (self), NULL);
  g_return_val_if_fail (index < self->priv->num_params, NULL);

  return (PARAM_NAME (index));
}

/**
 * bt_parameter_group_get_param_no_value:
 * @self: the parameter group to get params from
 * @index: the offset in the list of params
 *
 * Get the neutral value for the machines parameter.
 *
 * Returns: the value. Don't modify.
 *
 * Since: 0.6
 */
GValue *
bt_parameter_group_get_param_no_value (const BtParameterGroup * const self,
    const gulong index)
{
  g_return_val_if_fail (BT_IS_PARAMETER_GROUP (self), FALSE);
  g_return_val_if_fail (index < self->priv->num_params, FALSE);

  return (&self->priv->no_val[index]);
}

/**
 * bt_parameter_group_get_wave_param_index:
 * @self: the parameter group to lookup the param from
 *
 * Searches for the wave-table index parameter (if any). This parameter should
 * refer to a wavetable index that should be used to play a note.
 *
 * Returns: the index of the wave-table parameter or -1 if none.
 */
glong
bt_parameter_group_get_wave_param_index (const BtParameterGroup * const self)
{
  const gulong params = self->priv->num_params;
  guint *flags = self->priv->flags;
  glong i;
  g_return_val_if_fail (BT_IS_PARAMETER_GROUP (self), -1);

  for (i = 0; i < params; i++) {
    if (flags[i] & GSTBT_PROPERTY_META_WAVE)
      return (i);
  }
  return (-1);
}


/*
 * bt_parameter_group_has_param_default_set:
 * @self: the parameter group to check params from
 * @index: the offset in the list of params
 *
 * Tests if the param uses the default at timestamp=0. Parameters have a
 * default if there is no control-point at that timestamp. When interactively
 * changing the parameter, the default needs to be updated by calling
 * bt_parameter_group_controller_change_value().
 *
 * Returns: %TRUE if it has a default there
 */
static gboolean
bt_parameter_group_has_param_default_set (const BtParameterGroup * const self,
    const gulong index)
{
  return GPOINTER_TO_INT (g_object_get_qdata (self->priv->parents[index],
          self->priv->quarks[index]));
}

/**
 * bt_parameter_group_set_param_default:
 * @self: the parameter group
 * @index: the offset in the list of params
 *
 * Set a default value that should be used before the first control-point.
 */
void
bt_parameter_group_set_param_default (const BtParameterGroup * const self,
    const gulong index)
{
  g_return_if_fail (BT_IS_PARAMETER_GROUP (self));
  g_return_if_fail (index < self->priv->num_params);

  if (bt_parameter_group_has_param_default_set (self, index)) {
    GST_WARNING_OBJECT (self, "updating param %d at ts=0", index);
    bt_parameter_group_controller_change_value (self, index,
        G_GUINT64_CONSTANT (0), NULL);
  }
}

/**
 * bt_parameter_group_set_param_value:
 * @self: the parameter group to set the param value
 * @index: the offset in the list of params
 * @event: the new value
 *
 * Sets a the specified param to the give data value.
 */
void
bt_parameter_group_set_param_value (const BtParameterGroup * const self,
    const gulong index, GValue * const event)
{
  g_return_if_fail (BT_IS_PARAMETER_GROUP (self));
  g_return_if_fail (G_IS_VALUE (event));
  g_return_if_fail (index < self->priv->num_params);

  GST_DEBUG ("set value for %s", PARAM_NAME (index));
  g_object_set_property (self->priv->parents[index], PARAM_NAME (index), event);
}


/**
 * bt_parameter_group_describe_param_value:
 * @self: the parameter group to get a param description from
 * @index: the offset in the list of params
 * @event: the value to describe
 *
 * Described a param value in human readable form. The type of the given @value
 * must match the type of the paramspec of the param referenced by @index.
 *
 * Returns: the description as newly allocated string
 */
gchar *
bt_parameter_group_describe_param_value (const BtParameterGroup * const self,
    const gulong index, GValue * const event)
{
  g_return_val_if_fail (BT_IS_PARAMETER_GROUP (self), NULL);
  g_return_val_if_fail (index < self->priv->num_params, NULL);
  g_return_val_if_fail (G_IS_VALUE (event), NULL);

  if (GSTBT_IS_PROPERTY_META (self->priv->parents[index])) {
    return
        gstbt_property_meta_describe_property (GSTBT_PROPERTY_META (self->priv->
            parents[index]), index, event);
  }
  return NULL;
}

//-- controller handling

static gboolean
controller_need_activate (GstInterpolationControlSource * cs)
{
  if (cs && gst_interpolation_control_source_get_count (cs)) {
    return (FALSE);
  }
  return (TRUE);
}

static gboolean
controller_rem_value (GstInterpolationControlSource * cs,
    const GstClockTime timestamp, const gboolean has_default)
{
  if (cs) {
    gint count;

    gst_interpolation_control_source_unset (cs, timestamp);

    // check if the property is not having control points anymore
    count = gst_interpolation_control_source_get_count (cs);
    if (has_default)            // remove also if there is a default only left
      count--;
    // @bug: http://bugzilla.gnome.org/show_bug.cgi?id=538201 -> fixed in 0.10.21
    return (count == 0);
  }
  return (FALSE);
}

/**
 * bt_parameter_group_controller_change_value:
 * @self: the parameter group to change the param for
 * @param: the global parameter index
 * @timestamp: the time stamp of the change
 * @value: the new value or %NULL to unset a previous one
 *
 * Depending on whether the given value is NULL, sets or unsets the controller
 * value for the specified param and at the given time.
 * If @timestamp is 0 and @value is %NULL it set a default value for the start
 * of the controller sequence, taken from the current value of the parameter.
 */
void
bt_parameter_group_controller_change_value (const BtParameterGroup * const self,
    const gulong ix, const GstClockTime timestamp, GValue * value)
{
  GObject *param_parent;
  GValue def_value = { 0, };
  GstInterpolationControlSource *cs;
  const gchar *param_name;

  g_return_if_fail (BT_IS_PARAMETER_GROUP (self));
  g_return_if_fail (ix < self->priv->num_params);

  param_parent = self->priv->parents[ix];
  param_name = PARAM_NAME (ix);
  cs = self->priv->control_sources[ix];

  if (G_UNLIKELY (!timestamp)) {
    if (!value) {
      // we set it later
      value = &def_value;
      // need to remember that we set a default, so that we can update it
      // (bt_parameter_group_has_param_default_set)
      g_object_set_qdata (param_parent, self->priv->quarks[ix],
          GINT_TO_POINTER (TRUE));
      GST_INFO ("set global default for param %lu:%s", ix, param_name);
    } else {
      // we set a real value for ts=0, no need to update the default
      g_object_set_qdata (param_parent, self->priv->quarks[ix],
          GINT_TO_POINTER (FALSE));
    }
  }

  if (value) {
    gboolean add = controller_need_activate (cs);
    gboolean is_trigger = bt_parameter_group_is_param_trigger (self, ix);

    if (G_UNLIKELY (value == &def_value)) {
      // only set default value if this is not the first controlpoint
      if (!add) {
        if (!is_trigger) {
          g_value_init (&def_value, PARAM_TYPE (ix));
          g_object_get_property (param_parent, param_name, &def_value);
          GST_LOG ("set controller: %" GST_TIME_FORMAT " param %s:%s",
              GST_TIME_ARGS (G_GUINT64_CONSTANT (0)),
              g_type_name (PARAM_TYPE (ix)), param_name);
          gst_interpolation_control_source_set (cs, G_GUINT64_CONSTANT (0),
              &def_value);
          g_value_unset (&def_value);
        } else {
          gst_interpolation_control_source_set (cs, G_GUINT64_CONSTANT (0),
              &self->priv->no_val[ix]);
        }
      }
    } else {
      if (G_UNLIKELY (add)) {
        GstController *ctrl;

        if ((ctrl =
                gst_object_control_properties (param_parent, param_name,
                    NULL))) {
          cs = gst_interpolation_control_source_new ();
          gst_controller_set_control_source (ctrl, param_name,
              GST_CONTROL_SOURCE (cs));
          // set interpolation mode depending on param type
          gst_interpolation_control_source_set_interpolation_mode (cs,
              is_trigger ? GST_INTERPOLATE_TRIGGER : GST_INTERPOLATE_NONE);
          self->priv->control_sources[ix] = cs;
        }
        // TODO(ensonic): is this needed, we're in add=TRUE after all
        g_object_try_unref (self->priv->controller[ix]);
        self->priv->controller[ix] = ctrl;

        if (timestamp) {
          // also set default value, as first control point is not a time=0
          GST_LOG ("set controller: %" GST_TIME_FORMAT " param %s:%s",
              GST_TIME_ARGS (G_GUINT64_CONSTANT (0)),
              g_type_name (PARAM_TYPE (ix)), param_name);
          if (!is_trigger) {
            g_value_init (&def_value, PARAM_TYPE (ix));
            g_object_get_property (param_parent, param_name, &def_value);
            gst_interpolation_control_source_set (cs, G_GUINT64_CONSTANT (0),
                &def_value);
            g_value_unset (&def_value);
          } else {
            gst_interpolation_control_source_set (cs, G_GUINT64_CONSTANT (0),
                &self->priv->no_val[ix]);
          }
        }
      }
      GST_LOG ("set controller: %" GST_TIME_FORMAT " param %s:%s",
          GST_TIME_ARGS (timestamp), g_type_name (PARAM_TYPE (ix)), param_name);
      gst_interpolation_control_source_set (cs, timestamp, value);
    }
  } else {
    gboolean has_default = bt_parameter_group_has_param_default_set (self, ix);

    GST_LOG ("unset controller: %" GST_TIME_FORMAT " param %s:%s",
        GST_TIME_ARGS (timestamp), g_type_name (PARAM_TYPE (ix)), param_name);
    if (controller_rem_value (cs, timestamp, has_default)) {
      gst_controller_set_control_source (self->priv->controller[ix], param_name,
          NULL);
      g_object_unref (cs);
      self->priv->control_sources[ix] = NULL;
      gst_object_uncontrol_properties (param_parent, param_name, NULL);
    }
  }
}

//-- group changes

/**
 * bt_parameter_group_set_param_defaults:
 * @self: the parameter group
 *
 * Set a default value that should be used before the first control-point for
 * each parameter.
 */
void
bt_parameter_group_set_param_defaults (const BtParameterGroup * const self)
{
  const gulong num_params = self->priv->num_params;
  gulong i;

  for (i = 0; i < num_params; i++) {
    if (self->priv->controller[i]) {
      bt_parameter_group_set_param_default (self, i);
    }
  }
}

/**
 * bt_parameter_group_randomize_values:
 * @self: the parameter group
 *
 * Randomize all parameter values.
 */
void
bt_parameter_group_randomize_values (const BtParameterGroup * const self)
{
  const gulong num_params = self->priv->num_params;
  gulong i;

  for (i = 0; i < num_params; i++) {
    bt_g_object_randomize_parameter (self->priv->parents[i],
        self->priv->params[i]);
  }
}

/**
 * bt_parameter_group_reset_values:
 * @self: the parameter group
 *
 * Reset all parameter values to their defaults.
 */
void
bt_parameter_group_reset_values (const BtParameterGroup * const self)
{
  const gulong num_params = self->priv->num_params;
  GValue gvalue = { 0, };
  gulong i;

  for (i = 0; i < num_params; i++) {
    g_value_init (&gvalue, PARAM_TYPE (i));
    g_param_value_set_default (self->priv->params[i], &gvalue);
    g_object_set_property (self->priv->parents[i], PARAM_NAME (i), &gvalue);
    g_value_unset (&gvalue);
  }
}

//-- g_object overrides

static void
bt_parameter_group_constructed (GObject * object)
{
  BtParameterGroup *const self = BT_PARAMETER_GROUP (object);
  gulong i, num_params = self->priv->num_params;
  GParamSpec *param, **params = self->priv->params;
  GObject *parent, **parents = self->priv->parents;
  gchar *qname;

  if (G_OBJECT_CLASS (bt_parameter_group_parent_class)->constructed)
    G_OBJECT_CLASS (bt_parameter_group_parent_class)->constructed (object);

  // init the param group
  GST_INFO ("create group with %lu params", num_params);
  self->priv->flags = (guint *) g_new0 (guint, num_params);
  self->priv->no_val = (GValue *) g_new0 (GValue, num_params);
  self->priv->quarks = (GQuark *) g_new0 (GQuark, num_params);
  self->priv->controller = (GstController **) g_new0 (gpointer, num_params);
  self->priv->control_sources =
      (GstInterpolationControlSource **) g_new0 (gpointer, num_params);

  for (i = 0; i < num_params; i++) {
    param = params[i];
    parent = parents[i];

    GST_DEBUG ("adding param [%u/%lu] \"%s\"", i, num_params, param->name);

    qname =
        g_strdup_printf ("%s::%s", G_OBJECT_TYPE_NAME (parent), param->name);
    self->priv->quarks[i] = g_quark_from_string (qname);
    g_free (qname);

    // treat not readable params as triggers
    if (param->flags & G_PARAM_READABLE) {
      self->priv->flags[i] = GSTBT_PROPERTY_META_STATE;
    }

    if (GSTBT_IS_PROPERTY_META (parent)) {
      gconstpointer const has_meta =
          g_param_spec_get_qdata (param, gstbt_property_meta_quark);

      if (has_meta) {
        self->priv->flags[i] =
            GPOINTER_TO_INT (g_param_spec_get_qdata (param,
                gstbt_property_meta_quark_flags));
        if (!(bt_parameter_group_get_property_meta_value (&self->
                    priv->no_val[i], param,
                    gstbt_property_meta_quark_no_val))) {
          GST_WARNING
              ("can't get no-val property-meta for param [%u/%lu] \"%s\"", i,
              num_params, param->name);
        }
      }
    }
    // use the properties default value for triggers as a no_value
    if (!BT_IS_GVALUE (&self->priv->no_val[i])
        && !(param->flags & G_PARAM_READABLE)) {
      g_value_init (&self->priv->no_val[i], param->value_type);
      g_param_value_set_default (param, &self->priv->no_val[i]);
    }
    // bind param to machines controller (possibly returns ref to existing)
    GST_DEBUG ("added param [%u/%lu] \"%s\"", i, num_params, param->name);
  }
}

static void
bt_parameter_group_set_property (GObject * const object,
    const guint property_id, const GValue * const value,
    GParamSpec * const pspec)
{
  const BtParameterGroup *const self = BT_PARAMETER_GROUP (object);
  return_if_disposed ();

  switch (property_id) {
    case PARAMETER_GROUP_NUM_PARAMS:{
      self->priv->num_params = g_value_get_ulong (value);
    }
      break;
    case PARAMETER_GROUP_PARENTS:{
      self->priv->parents = (GObject **) g_value_get_pointer (value);
    }
      break;
    case PARAMETER_GROUP_PARAMS:{
      self->priv->params = (GParamSpec **) g_value_get_pointer (value);
    }
      break;
    default:{
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
      break;
  }
}

static void
bt_parameter_group_get_property (GObject * const object,
    const guint property_id, GValue * const value, GParamSpec * const pspec)
{
  const BtParameterGroup *const self = BT_PARAMETER_GROUP (object);
  return_if_disposed ();

  switch (property_id) {
    case PARAMETER_GROUP_NUM_PARAMS:{
      g_value_set_ulong (value, self->priv->num_params);
    }
      break;
    case PARAMETER_GROUP_PARENTS:{
      g_value_set_pointer (value, self->priv->parents);
    }
      break;
    case PARAMETER_GROUP_PARAMS:{
      g_value_set_pointer (value, self->priv->params);
    }
      break;
    default:{
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
      break;
  }
}

static void
bt_parameter_group_dispose (GObject * const object)
{
  const BtParameterGroup *const self = BT_PARAMETER_GROUP (object);
  gulong i;

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  for (i = 0; i < self->priv->num_params; i++) {
    g_object_try_unref (self->priv->control_sources[i]);
    //bt_gst_object_deactivate_controller(param_parent, PARAM_NAME(i));
    g_object_try_unref (self->priv->controller[i]);
  }

  G_OBJECT_CLASS (bt_parameter_group_parent_class)->dispose (object);
}

static void
bt_parameter_group_finalize (GObject * const object)
{
  const BtParameterGroup *const self = BT_PARAMETER_GROUP (object);
  GValue *v;
  gulong i;

  for (i = 0; i < self->priv->num_params; i++) {
    v = &self->priv->no_val[i];
    if (BT_IS_GVALUE (v))
      g_value_unset (v);
  }

  g_free (self->priv->parents);
  g_free (self->priv->params);
  g_free (self->priv->quarks);
  g_free (self->priv->no_val);
  g_free (self->priv->flags);
  g_free (self->priv->controller);
  g_free (self->priv->control_sources);

  G_OBJECT_CLASS (bt_parameter_group_parent_class)->finalize (object);
}

//-- class internals

static void
bt_parameter_group_init (BtParameterGroup * self)
{
  GST_DEBUG ("!!!! self=%p", self);
  self->priv =
      G_TYPE_INSTANCE_GET_PRIVATE (self, BT_TYPE_PARAMETER_GROUP,
      BtParameterGroupPrivate);
}

static void
bt_parameter_group_class_init (BtParameterGroupClass * const klass)
{
  GObjectClass *const gobject_class = G_OBJECT_CLASS (klass);

  GST_DEBUG ("!!!!");
  g_type_class_add_private (klass, sizeof (BtParameterGroupPrivate));

  gobject_class->constructed = bt_parameter_group_constructed;
  gobject_class->set_property = bt_parameter_group_set_property;
  gobject_class->get_property = bt_parameter_group_get_property;
  gobject_class->dispose = bt_parameter_group_dispose;
  gobject_class->finalize = bt_parameter_group_finalize;

  g_object_class_install_property (gobject_class, PARAMETER_GROUP_NUM_PARAMS,
      g_param_spec_ulong ("num-params",
          "num-params prop",
          "number of params",
          0,
          G_MAXULONG,
          0,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PARAMETER_GROUP_PARENTS,
      g_param_spec_pointer ("parents",
          "parents prop",
          "pointer to array conatining the Objects that own the paramers, takes ownership",
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PARAMETER_GROUP_PARAMS,
      g_param_spec_pointer ("params",
          "params prop",
          "pointer to GParamSpec array, takes ownership",
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}
