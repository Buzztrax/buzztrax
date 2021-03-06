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

#ifndef BTIC_LEARN_H
#define BTIC_LEARN_H

#include <glib.h>
#include <glib-object.h>

#define BTIC_TYPE_LEARN                   (btic_learn_get_type())
#define BTIC_LEARN(obj)                   (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTIC_TYPE_LEARN, BtIcLearn))
#define BTIC_IS_LEARN(obj)                (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTIC_TYPE_LEARN))
#define BTIC_LEARN_GET_INTERFACE(obj)     (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BTIC_TYPE_LEARN, BtIcLearnInterface))

/* type macros */

typedef struct _BtIcLearn BtIcLearn; /* dummy object */
typedef struct _BtIcLearnInterface BtIcLearnInterface;

/**
 * btic_learn_virtual_start:
 * @self: device instance
 *
 * Subclasses will override this methods with a function which enables the
 * learning mode on this device.
 *
 * Returns: %TRUE for success
 */
typedef gboolean (*btic_learn_virtual_start)(gconstpointer self);

/**
 * btic_learn_virtual_stop:
 * @self: device instance
 *
 * Subclasses will override this methods with a function which disables the
 * learning mode on this device.
 *
 * Returns: %TRUE for success
 */
typedef gboolean (*btic_learn_virtual_stop)(gconstpointer self);

/**
 * btic_learn_virtual_register_learned_control:
 * @self: device instance
 * @name: the name under which to register the control
 *
 * Subclasses will override this methods with a function which registers
 * the last control which was detected in learn mode.
 *
 * Returns: %TRUE for success
 */
typedef BtIcControl* (*btic_learn_virtual_register_learned_control)(gconstpointer self, const gchar *name);

/**
 * BtIcLearn:
 *
 * interface for devices which implement a learn-function
 */
struct _BtIcLearnInterface {
  const GTypeInterface parent;

  btic_learn_virtual_start learn_start;
  btic_learn_virtual_stop  learn_stop;
  btic_learn_virtual_register_learned_control register_learned_control;
};

GType btic_learn_get_type(void) G_GNUC_CONST;

gboolean btic_learn_start(const BtIcLearn *self);
gboolean btic_learn_stop(const BtIcLearn *self);
BtIcControl* btic_learn_register_learned_control(const BtIcLearn *self, const gchar *name);

gboolean btic_learn_store_controller_map(const BtIcLearn *self);
gboolean btic_learn_load_controller_map(const BtIcLearn *self);

#endif // BTIC_LEARN_H
