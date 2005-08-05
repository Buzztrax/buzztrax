/* $Id: source-machine.h,v 1.13 2005-08-05 09:36:17 ensonic Exp $
 * class for a source machine
 */

#ifndef BT_SOURCE_MACHINE_H
#define BT_SOURCE_MACHINE_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_SOURCE_MACHINE            (bt_source_machine_get_type ())
#define BT_SOURCE_MACHINE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_SOURCE_MACHINE, BtSourceMachine))
#define BT_SOURCE_MACHINE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_SOURCE_MACHINE, BtSourceMachineClass))
#define BT_IS_SOURCE_MACHINE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_SOURCE_MACHINE))
#define BT_IS_SOURCE_MACHINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_SOURCE_MACHINE))
#define BT_SOURCE_MACHINE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_SOURCE_MACHINE, BtSourceMachineClass))

/* type macros */

typedef struct _BtSourceMachine BtSourceMachine;
typedef struct _BtSourceMachineClass BtSourceMachineClass;
typedef struct _BtSourceMachinePrivate BtSourceMachinePrivate;

/**
 * BtSourceMachine:
 *
 * Sub-class of a #BtMachine that implements a signal generator
 * (a machine with outputs only).
 */
struct _BtSourceMachine {
  BtMachine parent;
  
  /*< private >*/
  BtSourceMachinePrivate *priv;
};
/* structure of the source_machine class */
struct _BtSourceMachineClass {
  BtMachineClass parent_class;
};

/* used by SOURCE_MACHINE_TYPE */
GType bt_source_machine_get_type(void);

/**
 * BtSourceMachinePatternIndex:
 * @BT_SOURCE_MACHINE_PATTERN_INDEX_BREAK: 
 * @BT_SOURCE_MACHINE_PATTERN_INDEX_MUTE:
 * @BT_SOURCE_MACHINE_PATTERN_INDEX_SOLO:
 * @BT_SOURCE_MACHINE_PATTERN_INDEX_OFFSET:
 *
 * Use this with bt_machine_get_pattern_by_index() to get the command patterns.
 */
typedef enum {
  BT_SOURCE_MACHINE_PATTERN_INDEX_BREAK=0,
  BT_SOURCE_MACHINE_PATTERN_INDEX_MUTE,
  BT_SOURCE_MACHINE_PATTERN_INDEX_SOLO,
  BT_SOURCE_MACHINE_PATTERN_INDEX_OFFSET
} BtSourceMachinePatternIndex;

#endif // BT_SOURCE_MACHINE_H
