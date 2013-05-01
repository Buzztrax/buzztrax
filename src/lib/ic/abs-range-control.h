/* Buzztrax
 * Copyright (C) 2007 Buzztrax team <buzztrax-devel@lists.sf.net>
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

#ifndef BTIC_ABS_RANGE_CONTROL_H
#define BTIC_ABS_RANGE_CONTROL_H

#include <glib.h>
#include <glib-object.h>

#include "control.h"
#include "device.h"

/* type macros */

#define BTIC_TYPE_ABS_RANGE_CONTROL            (btic_abs_range_control_get_type ())
#define BTIC_ABS_RANGE_CONTROL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTIC_TYPE_ABS_RANGE_CONTROL, BtIcAbsRangeControl))
#define BTIC_ABS_RANGE_CONTROL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTIC_TYPE_ABS_RANGE_CONTROL, BtIcAbsRangeControlClass))
#define BTIC_IS_ABS_RANGE_CONTROL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTIC_TYPE_ABS_RANGE_CONTROL))
#define BTIC_IS_ABS_RANGE_CONTROL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTIC_TYPE_ABS_RANGE_CONTROL))
#define BTIC_ABS_RANGE_CONTROL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTIC_TYPE_ABS_RANGE_CONTROL, BtIcAbsRangeControlClass))

typedef struct _BtIcAbsRangeControl BtIcAbsRangeControl;
typedef struct _BtIcAbsRangeControlClass BtIcAbsRangeControlClass;
typedef struct _BtIcAbsRangeControlPrivate BtIcAbsRangeControlPrivate;

/**
 * BtIcAbsRangeControl:
 *
 * buzztraxs interaction controller single trigger control
 */
struct _BtIcAbsRangeControl {
  const BtIcControl parent;

  /*< private >*/
  BtIcAbsRangeControlPrivate *priv;
};

struct _BtIcAbsRangeControlClass {
  const BtIcControlClass parent;

};

GType btic_abs_range_control_get_type(void) G_GNUC_CONST;

BtIcAbsRangeControl *btic_abs_range_control_new(const BtIcDevice *device,const gchar *name,guint id,glong min,glong max,glong def);

#endif // BTIC_ABS_RANGE_CONTROL_H
