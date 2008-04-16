/* $Id$
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

#ifndef BTIC_ABS_RANGE_CONTROL_METHODS_H
#define BTIC_ABS_RANGE_CONTROL_METHODS_H

#include "device.h"
#include "abs-range-control.h"

extern BtIcAbsRangeControl *btic_abs_range_control_new(const BtIcDevice *device,const gchar *name,gint32 min,gint32 max,gint32 def);

#endif // BTIC_ABS_RANGE_CONTROL_METHDOS_H
