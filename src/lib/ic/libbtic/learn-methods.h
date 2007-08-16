/* $Id: learn-methods.h,v 1.1 2007-08-16 12:34:42 berzerka Exp $
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

#ifndef BTIC_LEARN_METHODS_H
#define BTIC_LEARN_METHODS_H

#include "learn.h"

extern gboolean btic_learn_start(const BtIcLearn *self);
extern gboolean btic_learn_stop(const BtIcLearn *self);
extern BtIcControl* btic_learn_register_learned_control(const BtIcLearn *self, const gchar *name);

#endif // BTIC_LEARN_METHDOS_H
