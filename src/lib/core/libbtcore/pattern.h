/* $Id: pattern.h,v 1.15 2005-07-15 22:26:26 ensonic Exp $
 * class for the pattern pattern
 *
 */

#ifndef BT_PATTERN_H
#define BT_PATTERN_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_PATTERN		         (bt_pattern_get_type ())
#define BT_PATTERN(obj)		         (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_PATTERN, BtPattern))
#define BT_PATTERN_CLASS(klass)	   (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_PATTERN, BtPatternClass))
#define BT_IS_PATTERN(obj)	       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_PATTERN))
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
 * A #BtTimeLineTrack denotes which pattern will be played at which time..
 */
struct _BtPattern {
  GObject parent;
  
  /*< private >*/
  BtPatternPrivate *priv;
};
/* structure of the pattern class */
struct _BtPatternClass {
  GObjectClass parent_class;

  void (*global_param_changed_event)(const BtPattern *pattern, gulong tick, gulong param, gpointer user_data);
  void (*voice_param_changed_event)(const BtPattern *pattern, gulong tick, gulong voice, gulong param, gpointer user_data);
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
  BT_PATTERN_CMD_BREAK,
  BT_PATTERN_CMD_MUTE,
  BT_PATTERN_CMD_SOLO,
  BT_PATTERN_CMD_BYPASS
} BtPatternCmd;

/* used by PATTERN_TYPE */
GType bt_pattern_get_type(void);
/* used by PATTERN_CMD_TYPE */
GType bt_pattern_cmd_get_type(void);

#endif /* BT_PATTERN_H */
