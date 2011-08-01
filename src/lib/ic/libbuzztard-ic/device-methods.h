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

#ifndef BTIC_DEVICE_METHODS_H
#define BTIC_DEVICE_METHODS_H

#include "control.h"
#include "device.h"

extern void btic_device_add_control(const BtIcDevice *self, const BtIcControl *control);
extern BtIcControl *btic_device_get_control_by_id(const BtIcDevice *self,guint id);
extern gboolean btic_device_has_controls(const BtIcDevice *self);
extern gboolean btic_device_start(const BtIcDevice *self);
extern gboolean btic_device_stop(const BtIcDevice *self);

#endif // BTIC_DEVICE_METHDOS_H
