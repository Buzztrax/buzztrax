/* $Id$
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
/**
 * SECTION:bticlearn
 * @short_description: interface for devices with learn function
 *
 * An interface which all devices which support interactive learning of
 * controls should implement.
 */

/* @todo: store controller maps for trainable devices
 * - whenever we have learned a control update the file on disk
 *   use btic_learn_register_learned_control()
 * - whenever we discover a device load the previous settings from disk
 * - files
 *   - use gkeyfile as a format
 *   - store them as g_get_user_data_dir(),PACKAGE_NAME,"$device_name.map"
 *   
 */
#define BTIC_CORE
#define BTIC_LEARN_C

#include "ic_private.h"

//-- the iface

G_DEFINE_INTERFACE (BtIcLearn, btic_learn, 0);

//-- helper

//-- wrapper

static gboolean btic_learn_default_start(gconstpointer self) {
  GST_ERROR("virtual method btic_learn_start(self=%p) called",self);
  return(FALSE);  // this is a base class that can't do anything
}

static gboolean btic_learn_default_stop(gconstpointer self) {
  GST_ERROR("virtual method btic_learn_stop(self=%p) called",self);
  return(FALSE);  // this is a base class that can't do anything
}

static BtIcControl* btic_learn_default_register_learned_control(gconstpointer self, const gchar *name) {
  GST_ERROR("virtual method btic_learn_register_learned_control(self=%p) called",self);
  return(NULL);  // this is a base class that can't do anything
}


/**
 * btic_learn_start:
 * @self: the device which implements the #BtIcLearn interface
 *
 * Starts the device if needed and enables the learn function.
 * Starts emission of notify::devide-controlchange signals.
 *
 * Returns: %TRUE for success
 */
gboolean btic_learn_start(const BtIcLearn *self)
{
  return BTIC_LEARN_GET_INTERFACE(self)->learn_start(self);
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
gboolean btic_learn_stop(const BtIcLearn *self)
{
  return BTIC_LEARN_GET_INTERFACE(self)->learn_stop(self);
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
BtIcControl* btic_learn_register_learned_control(const BtIcLearn *self, const gchar* name)
{
  return BTIC_LEARN_GET_INTERFACE(self)->register_learned_control(self, name);
}

//-- interface internals

static void btic_learn_default_init(BtIcLearnInterface *iface) {
  iface->learn_start=btic_learn_default_start;
  iface->learn_stop=btic_learn_default_stop;
  iface->register_learned_control=btic_learn_default_register_learned_control;

  g_object_interface_install_property (iface,
    g_param_spec_string ("device-controlchange",
      "the last control detected by learn",
      "get the last detected control",
      NULL,
      G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));
}

