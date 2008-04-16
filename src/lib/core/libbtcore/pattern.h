/* $Id$
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

#ifndef BT_PATTERN_H
#define BT_PATTERN_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_PATTERN             (bt_pattern_get_type ())
#define BT_PATTERN(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_PATTERN, BtPattern))
#define BT_PATTERN_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_PATTERN, BtPatternClass))
#define BT_IS_PATTERN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_PATTERN))
#define BT_IS_PATTERN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_PATTERN))
#define BT_PATTERN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_PATTERN, BtPatternClass))

/* type macros */

typedef struct _BtPattern BtPattern;
typedef struct _BtPatternClass BtPatternClass;
typedef struct _BtPatternPrivate BtPatternPrivate;

/**
 * BtPattern:
 *
 * Class that holds a sequence of events for a #BtMachine.
 */
struct _BtPattern {
  const GObject parent;
  
  /*< private >*/
  BtPatternPrivate *priv;
};
/* structure of the pattern class */
struct _BtPatternClass {
  const GObjectClass parent;

  void (*global_param_changed_event)(const BtPattern * const pattern, const gulong tick, const gulong param, gconstpointer const user_data);
  void (*voice_param_changed_event)(const BtPattern * const pattern, const gulong tick, const gulong voice, const gulong param, gconstpointer const user_data);
  void (*pattern_changed_event)(const BtPattern * const pattern, gconstpointer const user_data);
};

#define BT_TYPE_PATTERN_CMD       (bt_pattern_cmd_get_type())

/**
 * BtPatternCmd:
 * @BT_PATTERN_CMD_NORMAL: just working
 * @BT_PATTERN_CMD_BREAK: no more notes
 * @BT_PATTERN_CMD_MUTE: be quiet immediately
 * @BT_PATTERN_CMD_SOLO: be the only one playing
 * @BT_PATTERN_CMD_BYPASS: be uneffective (pass through)
 *
 * A pattern has a command field for every tick.
 * The commands are only used in static internal patterns.
 */
typedef enum {
  BT_PATTERN_CMD_NORMAL=0,
  BT_PATTERN_CMD_MUTE,
  BT_PATTERN_CMD_BREAK,
  BT_PATTERN_CMD_SOLO,
  BT_PATTERN_CMD_BYPASS
} BtPatternCmd;

/* used by PATTERN_TYPE */
GType bt_pattern_get_type(void) G_GNUC_CONST;
/* used by PATTERN_CMD_TYPE */
GType bt_pattern_cmd_get_type(void) G_GNUC_CONST;

#endif /* BT_PATTERN_H */
