/* $Id: abs-range-control.h,v 1.1 2007-04-11 18:31:07 ensonic Exp $
 *
 * Buzztard
 * Copyright (C) 2007 Buzztard team <buzztard-devel@lists.sf.net>
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

#ifndef BTIC_ABS_RANGE_CONTROL_H
#define BTIC_ABS_RANGE_CONTROL_H

#include <glib.h>
#include <glib-object.h>

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
 * buzztards interaction controller single trigger control
 */
struct _BtIcAbsRangeControl {
  const BtIcControl parent;

  /*< private >*/
  BtIcAbsRangeControlPrivate *priv;
};
/* structure of the control class */
struct _BtIcAbsRangeControlClass {
  const BtIcControlClass parent;

};

/* used by DEVICE_TYPE */
GType btic_abs_range_control_get_type(void) G_GNUC_CONST;

#endif // BTIC_ABS_RANGE_CONTROL_H
