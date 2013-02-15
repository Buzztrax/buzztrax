/* Buzztard
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
/**
 * SECTION:bticlearn
 * @short_description: interface for devices with learn function
 *
 * An interface which all devices which support interactive learning of
 * controls should implement.
 *
 * The interface comes with an implementation for caching the learned controls.
 * The implementor needs to call btic_learn_load_controller_map() after 
 * construction and btic_learn_store_controller_map() when a control got added.
 */

#define BTIC_CORE
#define BTIC_LEARN_C

#include "ic_private.h"
#include <sys/stat.h>

//-- the iface

G_DEFINE_INTERFACE (BtIcLearn, btic_learn, 0);

//-- helper

//-- wrapper

static gboolean
btic_learn_default_start (gconstpointer self)
{
  GST_ERROR ("virtual method btic_learn_start(self=%p) called", self);
  return (FALSE);               // this is a base class that can't do anything
}

static gboolean
btic_learn_default_stop (gconstpointer self)
{
  GST_ERROR ("virtual method btic_learn_stop(self=%p) called", self);
  return (FALSE);               // this is a base class that can't do anything
}

static BtIcControl *
btic_learn_default_register_learned_control (gconstpointer self,
    const gchar * name)
{
  GST_ERROR
      ("virtual method btic_learn_register_learned_control(self=%p) called",
      self);
  return (NULL);                // this is a base class that can't do anything
}

//-- interface vmethods

/**
 * btic_learn_start:
 * @self: the device which implements the #BtIcLearn interface
 *
 * Starts the device if needed and enables the learn function.
 * Starts emission of notify::devide-controlchange signals.
 *
 * Returns: %TRUE for success
 */
gboolean
btic_learn_start (const BtIcLearn * self)
{
  return BTIC_LEARN_GET_INTERFACE (self)->learn_start (self);
}

/**
 * btic_learn_stop:
 * @self: the device which implements the #BtIcLearn interface
 *
 * Eventually stops the device and disables the learn function.
 * Stops emission of notify::devide-controlchange signals.
 *
 * Returns: %TRUE for success
 */
gboolean
btic_learn_stop (const BtIcLearn * self)
{
  return BTIC_LEARN_GET_INTERFACE (self)->learn_stop (self);
}

/**
 * btic_learn_register_learned_control:
 * @self: the device which implements the #BtIcLearn interface
 * @name: the name under which to register the control
 *
 * Registers the last detected control with name @name.
 *
 * Returns: %TRUE for success
 */
BtIcControl *
btic_learn_register_learned_control (const BtIcLearn * self, const gchar * name)
{
  return BTIC_LEARN_GET_INTERFACE (self)->register_learned_control (self, name);
}

//-- interface methods

#define MAP_HEADER "_controller-map_"

/* keys of the preset header section */
#define MAP_HEADER_DEVICE_NAME "device-name"

/**
 * btic_learn_store_controller_map:
 * @self: the device
 *
 * Store a map of all controls to disk. Interface implementations should call
 * this from their btic_learn_register_learned_control() function after they
 * registered a new control.
 *
 * Returns: %TRUE for success
 */
gboolean
btic_learn_store_controller_map (const BtIcLearn * self)
{
  gchar *map_dir, *map_name, *real_device_name, *device_name, *control_name;
  GKeyFile *out;
  GError *error = NULL;
  gchar *data;
  gsize data_size;
  GList *controls, *node;
  BtIcControl *control;
  glong min_val, max_val, def_val;
  guint control_id;

  map_dir =
      g_build_filename (g_get_user_data_dir (), PACKAGE, "controller-maps",
      NULL);
  /* ensure mapdir, set to NULL if we can't make it */
  if (g_mkdir_with_parents (map_dir,
          S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)
      == -1)
    goto mkdir_failed;

  g_object_get ((GObject *) self, "name", &device_name, "controls", &controls,
      NULL);
  if ((real_device_name = strchr (device_name, ':'))) {
    real_device_name = &real_device_name[2];
  } else {
    real_device_name = device_name;
  }
  map_name = g_build_filename (map_dir, real_device_name, NULL);
  g_free (map_dir);

  /* create gkeyfile */
  GST_INFO ("store learned controllers under '%s'", map_name);

  out = g_key_file_new ();
  g_key_file_set_string (out, MAP_HEADER, MAP_HEADER_DEVICE_NAME,
      real_device_name);
  g_free (device_name);

  /* todo: write control data  */
  for (node = controls; node; node = g_list_next (node)) {
    control = BTIC_CONTROL (node->data);

    g_object_get (control, "name", &control_name, "id", &control_id, "min",
        &min_val, "max", &max_val, "def", &def_val, NULL);
    // TODO(ensonic): we ignore the type right now
    g_key_file_set_string (out, control_name, "type", "abs-range");
    g_key_file_set_integer (out, control_name, "id", control_id);
    g_key_file_set_integer (out, control_name, "min-val", min_val);
    g_key_file_set_integer (out, control_name, "max-val", max_val);
    g_key_file_set_integer (out, control_name, "def-val", def_val);
    g_free (control_name);
  }
  g_list_free (controls);

  /* get new contents, we need this to save it */
  if (!(data = g_key_file_to_data (out, &data_size, &error)))
    goto convert_failed;
  /* write data */
  if (!g_file_set_contents (map_name, data, data_size, &error))
    goto write_failed;
  g_free (data);

  g_key_file_free (out);
  g_free (map_name);
  return (TRUE);

  /* ERRORS */
mkdir_failed:
  GST_WARNING ("Can't create controll-maps dir: '%s': %s", map_dir,
      g_strerror (errno));
  g_free (map_dir);
  return (FALSE);
convert_failed:
  GST_WARNING ("can not get the keyfile contents: %s", error->message);
  g_error_free (error);
  g_free (data);
  g_key_file_free (out);
  g_free (map_name);
  return (FALSE);
write_failed:
  GST_WARNING ("Unable to store control-map file %s: %s", map_name,
      error->message);
  g_error_free (error);
  g_free (data);
  g_key_file_free (out);
  g_free (map_name);
  return (FALSE);
}

/**
 * btic_learn_load_controller_map:
 * @self: the device
 *
 * Create initial set of controls from a stored control map. Interface
 * implementations should call this from their #GObjectClass.constructed() function.
 *
 * Returns: %TRUE for success
 */
gboolean
btic_learn_load_controller_map (const BtIcLearn * self)
{
  gchar *map_name, *real_device_name, *device_name, *file_device_name;
  GKeyFile *in;
  GError *error = NULL;
  gchar **groups;
  gsize i, num_groups;
  gboolean res;
  glong min_val, max_val, def_val;
  guint control_id;

  g_object_get ((GObject *) self, "name", &device_name, NULL);
  if ((real_device_name = strchr (device_name, ':'))) {
    real_device_name = &real_device_name[2];
  } else {
    real_device_name = device_name;
  }
  map_name =
      g_build_filename (g_get_user_data_dir (), PACKAGE, "controller-maps",
      real_device_name, NULL);

  /* read gkeyfile */
  GST_INFO ("load learned controllers under '%s'", map_name);
  in = g_key_file_new ();

  res = g_key_file_load_from_file (in, map_name,
      G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, &error);
  if (!res || error != NULL)
    goto load_error;

  file_device_name =
      g_key_file_get_value (in, MAP_HEADER, MAP_HEADER_DEVICE_NAME, NULL);
  if (!file_device_name || strcmp (file_device_name, real_device_name))
    goto wrong_name;
  g_free (file_device_name);

  /* read controllers and create controls */
  groups = g_key_file_get_groups (in, &num_groups);
  for (i = 0; i < num_groups; i++) {
    /* ignore private groups */
    if (groups[i][0] == '_')
      continue;

    // TODO(ensonic): we ignore the type right now (field "type")
    control_id = g_key_file_get_integer (in, groups[i], "id", NULL);
    min_val = g_key_file_get_integer (in, groups[i], "min-val", NULL);
    max_val = g_key_file_get_integer (in, groups[i], "max-val", NULL);
    def_val = g_key_file_get_integer (in, groups[i], "def-val", NULL);
    /*control= */ btic_abs_range_control_new (BTIC_DEVICE (self), groups[i],
        control_id, min_val, max_val, def_val);
  }
  g_strfreev (groups);

  g_key_file_free (in);
  g_free (device_name);
  g_free (map_name);
  return (TRUE);

  /* Errors */
load_error:
  GST_WARNING ("Unable to load controller map file %s: %s", map_name,
      error->message);
  g_error_free (error);
  g_key_file_free (in);
  g_free (device_name);
  g_free (map_name);
  return (FALSE);
wrong_name:
  GST_WARNING
      ("Wrong device name in controller map file %s. Expected %s, got %s",
      map_name, real_device_name, GST_STR_NULL (file_device_name));
  g_key_file_free (in);
  g_free (file_device_name);
  g_free (device_name);
  g_free (map_name);
  return (FALSE);

}

//-- interface internals

static void
btic_learn_default_init (BtIcLearnInterface * iface)
{
  iface->learn_start = btic_learn_default_start;
  iface->learn_stop = btic_learn_default_stop;
  iface->register_learned_control = btic_learn_default_register_learned_control;

  g_object_interface_install_property (iface,
      g_param_spec_string ("device-controlchange",
          "the last control detected by learn",
          "get the last detected control",
          NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}
