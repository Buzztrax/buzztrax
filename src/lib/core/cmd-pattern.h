/* Buzztrax
 * Copyright (C) 2012 Buzztrax team <buzztrax-devel@buzztrax.org>
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

#ifndef BT_CMD_PATTERN_H
#define BT_CMD_PATTERN_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_CMD_PATTERN             (bt_cmd_pattern_get_type ())
#define BT_CMD_PATTERN(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_CMD_PATTERN, BtCmdPattern))
#define BT_CMD_PATTERN_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_CMD_PATTERN, BtCmdPatternClass))
#define BT_IS_CMD_PATTERN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_CMD_PATTERN))
#define BT_IS_CMD_PATTERN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_CMD_PATTERN))
#define BT_CMD_PATTERN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_CMD_PATTERN, BtCmdPatternClass))

/* type macros */

typedef struct _BtCmdPattern BtCmdPattern;
typedef struct _BtCmdPatternClass BtCmdPatternClass;
typedef struct _BtCmdPatternPrivate BtCmdPatternPrivate;

/**
 * BtCmdPattern:
 *
 * Holds a sequence of events for a #BtMachine.
 */
struct _BtCmdPattern {
  const GObject parent;
  
  /*< private >*/
  BtCmdPatternPrivate *priv;
};

struct _BtCmdPatternClass {
  const GObjectClass parent;
};

#define BT_TYPE_PATTERN_CMD       (bt_pattern_cmd_get_type())

/**
 * BtPatternCmd:
 * @BT_PATTERN_CMD_NORMAL: no command
 * @BT_PATTERN_CMD_MUTE: be quiet immediately
 * @BT_PATTERN_CMD_SOLO: be the only one playing
 * @BT_PATTERN_CMD_BYPASS: be uneffective (pass through)
 * @BT_PATTERN_CMD_BREAK: no more notes
 *
 * The commands are only used in static internal patterns. Regular patterns use
 * @BT_PATTERN_CMD_NORMAL.
 */
typedef enum {
  BT_PATTERN_CMD_NORMAL=0,
  BT_PATTERN_CMD_MUTE,
  BT_PATTERN_CMD_SOLO,
  BT_PATTERN_CMD_BYPASS,
  BT_PATTERN_CMD_BREAK
} BtPatternCmd;

GType bt_cmd_pattern_get_type(void) G_GNUC_CONST;
GType bt_pattern_cmd_get_type(void) G_GNUC_CONST;

#include "machine.h"
#include "song.h"

BtCmdPattern *bt_cmd_pattern_new(const BtSong * const song, const BtMachine * const machine, const BtPatternCmd cmd);

const gchar *bt_cmd_pattern_get_name(const BtCmdPattern* self);

#endif /* BT_CMD_PATTERN_H */
