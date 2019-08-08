/* Buzztrax
 * Copyright (C) 2005 Buzztrax team <buzztrax-devel@buzztrax.org>
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
 * SECTION:btchildproxy
 * @short_description: Helpers for multi child elements.
 *
 * These helpers provide an extension to the #GstChildProxy interface. In the
 * same way as that #GstChildProxy lets you access properties of named children
 * objects, these methods let you do that for all #GObject projects that return
 * #GObjects.
 *
 * Property names are written as "child-name::property-name". The whole naming
 * scheme is recursive. Thus "child1::child2::property" is valid too, if
 * "child1" and "child2" are objects that implement the interface or are
 * properties that return a #GObject. The later is a convenient way to set or
 * get properties a few hops down the hierarchy in one go (without being able to
 * forget the unrefs of the intermediate objects).
 */
/* IDEA(ensonic): allow implementors to provide a lookup cache if they have
 *   static name -> object mappings
 */
/* TODO(ensonic): right now, the varargs function would lookup the owner for
 *   each property even if it is the same
 */
#include "core_private.h"
#include <gobject/gvaluecollector.h>


/*
 * bt_child_proxy_lookup:
 * @object: object to lookup the property in
 * @name: child proxy name of the property to look up
 * @target: (out) (allow-none) (transfer full): pointer to a #GObject that
 *     takes the real object to set property on
 * @pspec: (out) (allow-none) (transfer none): pointer to take the #GParamSpec
 *     describing the property
 *
 * Looks up which object and #GParamSpec would be effected by the given @name.
 *
 * Returns: TRUE if @target and @pspec could be found. FALSE otherwise. In that
 * case the values for @pspec and @target are not modified. Unref @target after
 * usage.
 */
static gboolean
bt_child_proxy_lookup (GObject * object, const gchar * name, GObject ** target,
    GParamSpec ** pspec)
{
  gboolean res = FALSE;
  gchar **names, **current;
  GParamSpec *spec = NULL;

  g_return_val_if_fail (G_IS_OBJECT (object), FALSE);
  g_return_val_if_fail (name != NULL, FALSE);

  g_object_ref (object);

  current = names = g_strsplit (name, "::", -1);
  /* find the owner of the property */
  while (current[1]) {
    GObject *next;

    GST_LOG ("trying %s %p:%s for %s",
        (GST_IS_CHILD_PROXY (object) ? "child-proxy" : "plain-object"),
        object, G_OBJECT_TYPE_NAME (object), current[0]);

    if (!GST_IS_CHILD_PROXY (object)) {
      // what if we just do a find by object class property?
      spec =
          g_object_class_find_property (G_OBJECT_GET_CLASS (object),
          current[0]);
      if (spec && G_IS_PARAM_SPEC_OBJECT (spec)) {
        g_object_get (object, current[0], &next, NULL);
        spec = NULL;
      } else {
        GST_INFO
            ("object %p:%s is not a parent and neither has a child named %s "
            "that is an object", object, G_OBJECT_TYPE_NAME (object),
            current[0]);
        break;
      }
    } else {
      next =
          gst_child_proxy_get_child_by_name (GST_CHILD_PROXY (object),
          current[0]);
      if (!next) {
        GST_INFO ("object %p:%s has no child named %s", object,
            G_OBJECT_TYPE_NAME (object), current[0]);
        break;
      }
    }
    g_object_unref (object);
    object = next;
    if (!next) {
      GST_INFO ("%s is NULL", current[0]);
      // we found next, but it is NULL
      if (pspec)
        *pspec = NULL;
      if (target)
        *target = NULL;
      break;
    }
    current++;
  }
  GST_LOG ("name loop done, object %p, name %s", object, current[0]);
  if (object) {
    if (current[1] == NULL) {
      GST_LOG ("checking pspec for %s", current[0]);
      if (!spec) {
        spec = g_object_class_find_property (G_OBJECT_GET_CLASS (object),
            current[0]);
      }
      if (G_UNLIKELY (!spec)) {
        GST_INFO ("no param spec named %s", current[0]);
      } else {
        if (pspec)
          *pspec = spec;
        if (target)
          *target = g_object_ref (object);
        res = TRUE;
      }
    }
    g_object_unref (object);
  }
  g_strfreev (names);
  return res;
}

/**
 * bt_child_proxy_get_property:
 * @object: object to query
 * @name: name of the property
 * @value: (out caller-allocates): a #GValue that should take the result.
 *
 * Gets a single property using the BtChildProxy mechanism.
 * You are responsible for for freeing it by calling g_value_unset()
 */
void
bt_child_proxy_get_property (GObject * object, const gchar * name,
    GValue * value)
{
  GParamSpec *pspec;
  GObject *target;

  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (name != NULL);
  g_return_if_fail (G_IS_VALUE (value));

  if (!bt_child_proxy_lookup (object, name, &target, &pspec))
    goto not_found;

  g_object_get_property (target, pspec->name, value);
  g_object_unref (target);

  return;

not_found:
  g_warning ("no property %s in object", name);
  return;
}

/**
 * bt_child_proxy_get_valist:
 * @object: the object to query
 * @first_property_name: name of the first property to get
 * @var_args: return location for the first property, followed optionally by more name/return location pairs, followed by NULL
 *
 * Gets properties of the parent object and its children.
 */
void
bt_child_proxy_get_valist (GObject * object, const gchar * first_property_name,
    va_list var_args)
{
  const gchar *name;
  gchar *error = NULL;
  GValue value = { 0, };
  GParamSpec *pspec;
  GObject *target;

  g_return_if_fail (G_IS_OBJECT (object));

  name = first_property_name;

  /* iterate over pairs */
  while (name) {
    if (!bt_child_proxy_lookup (object, name, &target, &pspec))
      goto not_found;

    g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
    g_object_get_property (target, pspec->name, &value);
    g_object_unref (target);

    G_VALUE_LCOPY (&value, var_args, 0, &error);
    if (error)
      goto cant_copy;
    g_value_unset (&value);
    name = va_arg (var_args, gchar *);
  }
  return;

not_found:
  g_warning ("no property %s in object", name);
  return;
cant_copy:
  g_warning ("error copying value %s in object: %s", pspec->name, error);
  g_value_unset (&value);
  return;
}

/**
 * bt_child_proxy_get:
 * @object: the parent object
 * @first_property_name: name of the first property to get
 * @...: return location for the first property, followed optionally by more name/return location pairs, followed by NULL
 *
 * Gets properties of the parent object and its children.
 */
void
bt_child_proxy_get (gpointer object, const gchar * first_property_name, ...)
{
  va_list var_args;

  g_return_if_fail (G_IS_OBJECT (object));

  va_start (var_args, first_property_name);
  bt_child_proxy_get_valist (object, first_property_name, var_args);
  va_end (var_args);
}

/**
 * bt_child_proxy_set_property:
 * @object: the parent object
 * @name: name of the property to set
 * @value: new #GValue for the property
 *
 * Sets a single property using the BtChildProxy mechanism.
 */
void
bt_child_proxy_set_property (GObject * object, const gchar * name,
    const GValue * value)
{
  GParamSpec *pspec;
  GObject *target;

  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (name != NULL);
  g_return_if_fail (G_IS_VALUE (value));

  if (!bt_child_proxy_lookup (object, name, &target, &pspec))
    goto not_found;

  g_object_set_property (target, pspec->name, value);
  g_object_unref (target);
  return;

not_found:
  g_warning ("cannot set property %s on object", name);
  return;
}

/**
 * bt_child_proxy_set_valist:
 * @object: the parent object
 * @first_property_name: name of the first property to set
 * @var_args: value for the first property, followed optionally by more name/value pairs, followed by %NULL
 *
 * Sets properties of the parent object and its children.
 */
void
bt_child_proxy_set_valist (GObject * object, const gchar * first_property_name,
    va_list var_args)
{
  const gchar *name;
  gchar *error = NULL;
  GValue value = { 0, };
  GParamSpec *pspec;
  GObject *target;

  g_return_if_fail (G_IS_OBJECT (object));

  name = first_property_name;
  /* iterate over pairs */
  while (name) {
    if (!bt_child_proxy_lookup (object, name, &target, &pspec))
      goto not_found;

    G_VALUE_COLLECT_INIT (&value, G_PARAM_SPEC_VALUE_TYPE (pspec), var_args,
        G_VALUE_NOCOPY_CONTENTS, &error);
    if (error)
      goto cant_copy;

    g_object_set_property (target, pspec->name, &value);
    g_object_unref (target);

    g_value_unset (&value);
    name = va_arg (var_args, gchar *);
  }
  return;

not_found:
  g_warning ("no property %s in object of type %s", name,
      G_OBJECT_TYPE_NAME (object));
  return;
cant_copy:
  g_warning ("error copying value %s in object: %s", pspec->name, error);
  g_value_unset (&value);
  g_object_unref (target);
  return;
}

/**
 * bt_child_proxy_set:
 * @object: the parent object
 * @first_property_name: name of the first property to set
 * @...: value for the first property, followed optionally by more name/value pairs, followed by %NULL
 *
 * Sets properties of the parent object and its children.
 */
void
bt_child_proxy_set (gpointer object, const gchar * first_property_name, ...)
{
  va_list var_args;

  g_return_if_fail (G_IS_OBJECT (object));

  va_start (var_args, first_property_name);
  bt_child_proxy_set_valist (object, first_property_name, var_args);
  va_end (var_args);
}
