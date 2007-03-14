/* $Id: midi-device-methods.h,v 1.1 2007-03-14 22:51:36 ensonic Exp $
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

#ifndef BTIC_MIDI_DEVICE_METHODS_H
#define BTIC_MIDI_DEVICE_METHODS_H

#include "midi-device.h"

extern BtIcMidiDevice *btic_midi_device_new(const gchar *udi,const gchar *name);

#endif // BTIC_MIDI_DEVICE_METHDOS_H
