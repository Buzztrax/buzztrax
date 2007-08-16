/* $Id: learn.h,v 1.1 2007-08-16 12:34:42 berzerka Exp $
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

struct _BtIcLearnInterface {
  const GTypeInterface parent;

  gboolean (*learn_start)(const BtIcLearn * const self);
  gboolean (*learn_stop)(const BtIcLearn * const self);
  BtIcControl* (*register_learned_control)(const BtIcLearn * const self, const gchar *name);
};

GType btic_learn_get_type(void) G_GNUC_CONST;

#endif // BTIC_LEARN_H
