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
 * SECTION:btparametergroup
 * @short_description: a group of parameter
 *
 * A group of parameters, such as used in machines or wires. Once created the
 * group will not change.
 */

/* FIXME(ensonic): consider to have a BtParameter struct and merge 6 arrays into
 * one
 * - for this we need to copy the values from parents & params though
 */
/* TODO(ensonic): should we have the default values in the param group to be
 * able to create the controllers as needed, right now we create them
 * unconditionally, just to track the default value.
 */
#define BT_CORE
#define BT_PARAMETER_GROUP_C

#include "core_private.h"
#include "gst/propertymeta.h"

//-- property ids

enum
{
  PARAMETER_GROUP_NUM_PARAMS = 1,
  PARAMETER_GROUP_PARENTS,
  PARAMETER_GROUP_PARAMS,
  PARAMETER_GROUP_SONG,
  PARAMETER_GROUP_MACHINE
};

struct _BtParameterGroupPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the number of parameter in the group */
  gulong num_params;

  /* song pointer */
  BtSong *song;

  /* machine pointer */
  BtMachine *machine;

  /* parameter data */
  GObject **parents;
  GParamSpec **params;
  GValue *no_val;
  GstControlBinding **cb;
};

//-- the class

G_DEFINE_TYPE_WITH_CODE (BtParameterGroup, bt_parameter_group, G_TYPE_OBJECT, 
    G_ADD_PRIVATE(BtParameterGroup));

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
  return res;
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
      break;
    }
    case G_TYPE_ENUM:{
      const GParamSpecEnum *p = G_PARAM_SPEC_ENUM (property);
      const GEnumClass *e = p->enum_class;
      gint nv = e->n_values - 1;        // don't use no_value
      gint v = nv * rnd;

      // handle sparse enums
      g_object_set (self, n, e->values[v].value, NULL);
      break;
    }
    default:
      GST_WARNING ("incomplete implementation for GParamSpec type '%s'",
          G_PARAM_SPEC_TYPE_NAME (property));
  }
}

//-- constructor methods

/**
 * bt_parameter_group_new:
 * @num_params: the number of parameters
 * @parents: array of parent #GObjects for each parameter
 * @params: array of #GParamSpecs for each parameter
 * @song: the song
 * @machine: the machine that is owns the parameter-group, use the target
 *   machine for wires.
 *
 * Create a parameter group.
 *
 * Returns: (transfer full): the new parameter group
 */
BtParameterGroup *
bt_parameter_group_new (gulong num_params, GObject ** parents,
    GParamSpec ** params, BtSong * song, const BtMachine * machine)
{
  return BT_PARAMETER_GROUP (g_object_new (BT_TYPE_PARAMETER_GROUP,
          "num-params", num_params, "parents", parents, "params", params,
          "song", song, "machine", machine, NULL));
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

  return !(self->priv->params[index]->flags & G_PARAM_READABLE);
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

  GValue *v = &self->priv->no_val[index];
  if (!BT_IS_GVALUE (v))
    return FALSE;

  if (gst_value_compare (v, value) == GST_VALUE_EQUAL)
    return TRUE;
  return FALSE;
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
 * Returns: (transfer none): the #GParamSpec for the requested param
 */
GParamSpec *
bt_parameter_group_get_param_spec (const BtParameterGroup * const self,
    const gulong index)
{
  g_return_val_if_fail (BT_IS_PARAMETER_GROUP (self), NULL);
  g_return_val_if_fail (index < self->priv->num_params, NULL);

  return self->priv->params[index];
}

/**
 * bt_parameter_group_get_param_parent:
 * @self: the parameter group to search for the param
 * @index: the offset in the list of params
 *
 * Retrieves the owner instance for the param
 *
 * Returns: (transfer none): the #GParamSpec for the requested param
 */
GObject *
bt_parameter_group_get_param_parent (const BtParameterGroup * const self,
    const gulong index)
{
  g_return_val_if_fail (BT_IS_PARAMETER_GROUP (self), NULL);
  g_return_val_if_fail (index < self->priv->num_params, NULL);

  return self->priv->parents[index];
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
 * @pspec: (out): place for the param spec
 * @min_val: (out): place to hold new GValue with minimum
 * @max_val: (out): place to hold new GValue with maximum
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

  return PARAM_TYPE (index);
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

  return PARAM_NAME (index);
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

  return &self->priv->no_val[index];
}

/**
 * bt_parameter_group_get_trigger_param_index:
 * @self: the parameter group to lookup the param from
 *
 * Searches for the first trigger parameter (if any).
 *
 * Returns: the index of the first trigger parameter or -1 if none.
 */
glong
bt_parameter_group_get_trigger_param_index (const BtParameterGroup * const self)
{
  const gulong num_params = self->priv->num_params;
  GParamSpec **params = self->priv->params;
  glong i;
  g_return_val_if_fail (BT_IS_PARAMETER_GROUP (self), -1);

  for (i = 0; i < num_params; i++) {
    if (!(params[i]->flags & G_PARAM_READABLE))
      return i;
  }
  return -1;
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
  const gulong num_params = self->priv->num_params;
  GParamSpec **params = self->priv->params;
  glong i;
  g_return_val_if_fail (BT_IS_PARAMETER_GROUP (self), -1);

  for (i = 0; i < num_params; i++) {
    if (params[i]->value_type == GSTBT_TYPE_WAVE_INDEX)
      return i;
  }
  return -1;
}

/**
 * bt_parameter_group_set_param_default:
 * @self: the parameter group
 * @index: the offset in the list of params
 *
 * Set the current value as default value that should be used before the first
 * control-point.
 */
void
bt_parameter_group_set_param_default (const BtParameterGroup * const self,
    const gulong index)
{
  g_return_if_fail (BT_IS_PARAMETER_GROUP (self));
  g_return_if_fail (index < self->priv->num_params);

  BtParameterGroupPrivate *p = self->priv;

  if (!p->cb[index]) {
    GST_WARNING_OBJECT (p->parents[index], "param %s is not controllable",
        PARAM_NAME (index));
    return;
  }

  if (!bt_parameter_group_is_param_trigger (self, index)) {
    GValue def_value = { 0, };

    GST_INFO_OBJECT (p->parents[index], "setting the current value for %s",
        PARAM_NAME (index));
    g_value_init (&def_value, PARAM_TYPE (index));
    g_object_get_property (p->parents[index], PARAM_NAME (index), &def_value);
    g_object_set (G_OBJECT (p->cb[index]), "default-value", &def_value, NULL);
    g_value_unset (&def_value);
  } else {
    GST_INFO_OBJECT (p->parents[index], "setting the no value for %s",
        PARAM_NAME (index));
    g_object_set (G_OBJECT (p->cb[index]), "default-value", &p->no_val[index],
        NULL);
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

  GST_DEBUG_OBJECT (self->priv->parents[index], "set value for %s",
      PARAM_NAME (index));
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
    guint prop_id = self->priv->params[index]->param_id;
    return
        gstbt_property_meta_describe_property (GSTBT_PROPERTY_META (self->priv->
            parents[index]), prop_id, event);
  } else if (bt_g_type_get_base_type (PARAM_TYPE (index)) == G_TYPE_ENUM) {
    // for enums return the nick
    const GParamSpecEnum *p = G_PARAM_SPEC_ENUM (self->priv->params[index]);
    GEnumValue *ev = g_enum_get_value (p->enum_class, g_value_get_enum (event));
    return g_strdup (ev->value_nick);
  }
  return NULL;
}

//-- group changes

/**
 * bt_parameter_group_set_param_defaults:
 * @self: the parameter group
 *
 * Set the current values as a default values that should be used before the
 * first control-point for each parameter.
 */
void
bt_parameter_group_set_param_defaults (const BtParameterGroup * const self)
{
  const gulong num_params = self->priv->num_params;
  gulong i;
  for (i = 0; i < num_params; i++) {
    bt_parameter_group_set_param_default (self, i);
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
  BtSequence *sequence;
  BtSongInfo *song_info;
  GstControlBinding *cb;

  if (G_OBJECT_CLASS (bt_parameter_group_parent_class)->constructed)
    G_OBJECT_CLASS (bt_parameter_group_parent_class)->constructed (object);

  // init the param group
  GST_INFO ("create group with %lu params", num_params);
  self->priv->no_val = (GValue *) g_new0 (GValue, num_params);
  self->priv->cb = (GstControlBinding **) g_new0 (gpointer, num_params);

  g_object_get (self->priv->song, "sequence", &sequence, "song-info",
      &song_info, NULL);

  for (i = 0; i < num_params; i++) {
    param = params[i];
    parent = parents[i];

    g_object_add_weak_pointer (parents[i], (gpointer *) & parents[i]);

    GST_DEBUG ("adding param [%lu/%lu] \"%s\"", i, num_params, param->name);

    if (GSTBT_IS_PROPERTY_META (parent)) {
      gconstpointer const has_meta =
          g_param_spec_get_qdata (param, gstbt_property_meta_quark);

      if (has_meta) {
        if (!(bt_parameter_group_get_property_meta_value (&self->
                    priv->no_val[i], param,
                    gstbt_property_meta_quark_no_val))) {
          GST_WARNING
              ("can't get no-val property-meta for param [%lu/%lu] \"%s\"", i,
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
    GST_DEBUG ("added param [%lu/%lu] \"%s\"", i, num_params, param->name);

    // create new control bindings
    if (param->flags & GST_PARAM_CONTROLLABLE) {
      GST_DEBUG_OBJECT (self->priv->machine,
          "creating control-source for param %s::%s",
          GST_OBJECT_NAME (parent), param->name);

      cb = (GstControlBinding *) bt_pattern_control_source_new ((GstObject *)
          parent, param->name, sequence, song_info, self->priv->machine, self);
      gst_object_add_control_binding ((GstObject *) parent, cb);
      self->priv->cb[i] = gst_object_ref (cb);
    }
  }
  g_object_unref (song_info);
  g_object_unref (sequence);
}

static void
bt_parameter_group_set_property (GObject * const object,
    const guint property_id, const GValue * const value,
    GParamSpec * const pspec)
{
  const BtParameterGroup *const self = BT_PARAMETER_GROUP (object);
  return_if_disposed ();

  switch (property_id) {
    case PARAMETER_GROUP_NUM_PARAMS:
      self->priv->num_params = g_value_get_ulong (value);
      break;
    case PARAMETER_GROUP_PARENTS:
      self->priv->parents = (GObject **) g_value_get_pointer (value);
      break;
    case PARAMETER_GROUP_PARAMS:
      self->priv->params = (GParamSpec **) g_value_get_pointer (value);
      break;
    case PARAMETER_GROUP_SONG:
      self->priv->song = BT_SONG (g_value_get_object (value));
      g_object_try_weak_ref (self->priv->song);
      break;
    case PARAMETER_GROUP_MACHINE:
      self->priv->machine = BT_MACHINE (g_value_get_object (value));
      g_object_try_weak_ref (self->priv->machine);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
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
    case PARAMETER_GROUP_NUM_PARAMS:
      g_value_set_ulong (value, self->priv->num_params);
      break;
    case PARAMETER_GROUP_PARENTS:
      g_value_set_pointer (value, self->priv->parents);
      break;
    case PARAMETER_GROUP_PARAMS:
      g_value_set_pointer (value, self->priv->params);
      break;
    case PARAMETER_GROUP_SONG:
      g_value_set_object (value, self->priv->song);
      break;
    case PARAMETER_GROUP_MACHINE:
      g_value_set_object (value, self->priv->machine);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_parameter_group_dispose (GObject * const object)
{
  BtParameterGroup *self = BT_PARAMETER_GROUP (object);
  GObject **parents = self->priv->parents;
  gulong i, num_params = self->priv->num_params;

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  for (i = 0; i < num_params; i++) {
    if (parents[i]) {
      if (self->priv->cb[i]) {
        gst_object_remove_control_binding ((GstObject *) parents[i],
            self->priv->cb[i]);
        gst_object_unref (self->priv->cb[i]);
        self->priv->cb[i] = NULL;
      }
      g_object_remove_weak_pointer (parents[i], (gpointer *) & parents[i]);
    }
  }

  g_object_try_weak_unref (self->priv->song);
  g_object_try_weak_unref (self->priv->machine);

  G_OBJECT_CLASS (bt_parameter_group_parent_class)->dispose (object);
}

static void
bt_parameter_group_finalize (GObject * const object)
{
  BtParameterGroup *self = BT_PARAMETER_GROUP (object);
  GValue *v;
  gulong i, num_params = self->priv->num_params;

  for (i = 0; i < num_params; i++) {
    v = &self->priv->no_val[i];
    if (BT_IS_GVALUE (v))
      g_value_unset (v);
  }

  g_free (self->priv->parents);
  g_free (self->priv->params);
  g_free (self->priv->no_val);
  g_free (self->priv->cb);

  G_OBJECT_CLASS (bt_parameter_group_parent_class)->finalize (object);
}

//-- class internals

static void
bt_parameter_group_init (BtParameterGroup * self)
{
  GST_DEBUG ("!!!! self=%p", self);
  self->priv = bt_parameter_group_get_instance_private(self);
}

static void
bt_parameter_group_class_init (BtParameterGroupClass * const klass)
{
  GObjectClass *const gobject_class = G_OBJECT_CLASS (klass);

  GST_DEBUG ("!!!!");

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
          "pointer to array containing the Objects that own the paramers, takes ownership",
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PARAMETER_GROUP_PARAMS,
      g_param_spec_pointer ("params",
          "params prop",
          "pointer to GParamSpec array, takes ownership",
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PARAMETER_GROUP_SONG,
      g_param_spec_object ("song",
          "song construct prop",
          "song object the param group belongs to", BT_TYPE_SONG,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PARAMETER_GROUP_MACHINE,
      g_param_spec_object ("machine",
          "machine construct prop",
          "the respective machine object", BT_TYPE_MACHINE,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}
