/* Buzztrax
 * Copyright (C) 2012 Buzztrax team <buzztrax-devel@lists.sf.net>
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

#ifndef BT_PATTERN_CONTROL_SOURCE_H
#define BT_PATTERN_CONTROL_SOURCE_H

#include <glib.h>
#include <glib-object.h>

#include "sequence.h"
#include "song-info.h"

#define BT_TYPE_PATTERN_CONTROL_SOURCE						(bt_pattern_control_source_get_type ())
#define BT_PATTERN_CONTROL_SOURCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_PATTERN_CONTROL_SOURCE, BtPatternControlSource))
#define BT_PATTERN_CONTROL_SOURCE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_PATTERN_CONTROL_SOURCE, BtPatternControlSourceClass))
#define BT_IS_PATTERN_CONTROL_SOURCE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_PATTERN_CONTROL_SOURCE))
#define BT_IS_PATTERN_CONTROL_SOURCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_PATTERN_CONTROL_SOURCE))
#define BT_PATTERN_CONTROL_SOURCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_PATTERN_CONTROL_SOURCE, BtPatternControlSourceClass))

/* type macros */

typedef struct _BtPatternControlSource BtPatternControlSource;
typedef struct _BtPatternControlSourceClass BtPatternControlSourceClass;
typedef struct _BtPatternControlSourcePrivate BtPatternControlSourcePrivate;

/**
 * BtPatternControlSource:
 *
 * A pattern based control source
 */
struct _BtPatternControlSource {
  const GstControlBinding parent;

  /*< private > */
  BtPatternControlSourcePrivate *priv;
};

struct _BtPatternControlSourceClass {
  const GstControlBindingClass parent;
};

GType bt_pattern_control_source_get_type (void) G_GNUC_CONST;

BtPatternControlSource *bt_pattern_control_source_new (GstObject * object, const gchar * property_name, BtSequence * sequence, const BtSongInfo *song_info, const BtMachine * machine, BtParameterGroup * param_group);

#endif // BT_PATTERN_CONTROL_SOURCE_H
