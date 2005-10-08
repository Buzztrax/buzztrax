/* $Id: sink-machine.h,v 1.14 2005-10-08 18:12:13 ensonic Exp $
 * class for a sink machine
 */

#ifndef BT_SINK_MACHINE_H
#define BT_SINK_MACHINE_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_SINK_MACHINE            (bt_sink_machine_get_type ())
#define BT_SINK_MACHINE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_SINK_MACHINE, BtSinkMachine))
#define BT_SINK_MACHINE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_SINK_MACHINE, BtSinkMachineClass))
#define BT_IS_SINK_MACHINE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_SINK_MACHINE))
#define BT_IS_SINK_MACHINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_SINK_MACHINE))
#define BT_SINK_MACHINE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_SINK_MACHINE, BtSinkMachineClass))

/* type macros */

typedef struct _BtSinkMachine BtSinkMachine;
typedef struct _BtSinkMachineClass BtSinkMachineClass;
typedef struct _BtSinkMachinePrivate BtSinkMachinePrivate;

/**
 * BtSinkMachine:
 *
 * Sub-class of a #BtMachine that implements a signal output
 * (a machine with inputs only).
 */
struct _BtSinkMachine {
  BtMachine parent;
  
  /*< private >*/
  BtSinkMachinePrivate *priv;
};
/* structure of the sink_machine class */
struct _BtSinkMachineClass {
  BtMachineClass parent_class;
};

/* used by SINK_MACHINE_TYPE */
GType bt_sink_machine_get_type(void) G_GNUC_CONST;

/**
 * BtSinkMachinePatternIndex:
 * @BT_SINK_MACHINE_PATTERN_INDEX_BREAK: 
 * @BT_SINK_MACHINE_PATTERN_INDEX_MUTE:
 * @BT_SINK_MACHINE_PATTERN_INDEX_OFFSET:
 *
 * Use this with bt_machine_get_pattern_by_index() to get the command patterns.
 */
typedef enum {
  BT_SINK_MACHINE_PATTERN_INDEX_BREAK=0,
  BT_SINK_MACHINE_PATTERN_INDEX_MUTE,
  BT_SINK_MACHINE_PATTERN_INDEX_OFFSET
} BtSinkMachinePatternIndex;

#endif // BT_SINK_MACHINE_H
