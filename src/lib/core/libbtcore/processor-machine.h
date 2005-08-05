/* $Id: processor-machine.h,v 1.13 2005-08-05 09:36:17 ensonic Exp $
 * class for a processor machine
 */

#ifndef BT_PROCESSOR_MACHINE_H
#define BT_PROCESSOR_MACHINE_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_PROCESSOR_MACHINE             (bt_processor_machine_get_type ())
#define BT_PROCESSOR_MACHINE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_PROCESSOR_MACHINE, BtProcessorMachine))
#define BT_PROCESSOR_MACHINE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_PROCESSOR_MACHINE, BtProcessorMachineClass))
#define BT_IS_PROCESSOR_MACHINE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_PROCESSOR_MACHINE))
#define BT_IS_PROCESSOR_MACHINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_PROCESSOR_MACHINE))
#define BT_PROCESSOR_MACHINE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_PROCESSOR_MACHINE, BtProcessorMachineClass))

/* type macros */

typedef struct _BtProcessorMachine BtProcessorMachine;
typedef struct _BtProcessorMachineClass BtProcessorMachineClass;
typedef struct _BtProcessorMachinePrivate BtProcessorMachinePrivate;

/**
 * BtProcessorMachine:
 *
 * Sub-class of a #BtMachine that implements an effect-processor
 * (a machine with in and outputs).
 */
struct _BtProcessorMachine {
  BtMachine parent;
  
  /*< private >*/
  BtProcessorMachinePrivate *priv;
};
/* structure of the processor_machine class */
struct _BtProcessorMachineClass {
  BtMachineClass parent_class;
};

/* used by PROCESSOR_MACHINE_TYPE */
GType bt_processor_machine_get_type(void);

/**
 * BtProcessorMachinePatternIndex:
 * @BT_PROCESSOR_MACHINE_PATTERN_INDEX_BREAK: 
 * @BT_PROCESSOR_MACHINE_PATTERN_INDEX_MUTE:
 * @BT_PROCESSOR_MACHINE_PATTERN_INDEX_BYPASS:
 * @BT_PROCESSOR_MACHINE_PATTERN_INDEX_OFFSET:
 *
 * Use this with bt_machine_get_pattern_by_index() to get the command patterns.
 */
typedef enum {
  BT_PROCESSOR_MACHINE_PATTERN_INDEX_BREAK=0,
  BT_PROCESSOR_MACHINE_PATTERN_INDEX_MUTE,
  BT_PROCESSOR_MACHINE_PATTERN_INDEX_BYPASS,
  BT_PROCESSOR_MACHINE_PATTERN_INDEX_OFFSET
} BtProcessorMachinePatternIndex;

#endif // BT_PROCESSOR_MACHINE_H
