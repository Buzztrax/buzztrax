/** $Id: processor-machine.h,v 1.1 2004-05-07 18:04:15 ensonic Exp $
 * class for a processor machine
 */

#ifndef BT_PROCESSOR_MACHINE_H
#define BT_PROCESSOR_MACHINE_H

#include <glib.h>
#include <glib-object.h>

#define BT_PROCESSOR_MACHINE_TYPE		        (bt_processor_machine_get_type ())
#define BT_PROCESSOR_MACHINE(obj)		        (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_PROCESSOR_MACHINE_TYPE, BtProcessorMachine))
#define BT_PROCESSOR_MACHINE_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), BT_PROCESSOR_MACHINE_TYPE, BtProcessorMachineClass))
#define BT_IS_PROCESSOR_MACHINE(obj)	        (G_TYPE_CHECK_TYPE ((obj), BT_SONG_IO_PROCESSOR_MACHINE))
#define BT_IS_PROCESSOR_MACHINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_PROCESSOR_MACHINE_TYPE))
#define BT_PROCESSOR_MACHINE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_PROCESSOR_MACHINE_TYPE, BtProcessorMachineClass))

/* type macros */

typedef struct _BtProcessorMachine BtProcessorMachine;
typedef struct _BtProcessorMachineClass BtProcessorMachineClass;
typedef struct _BtProcessorMachinePrivate BtProcessorMachinePrivate;

struct _BtProcessorMachine {
  BtMachine parent;
  
  /* private */
  BtProcessorMachinePrivate *private;
};
/* structure of the processor_machine class */
struct _BtProcessorMachineClass {
  BtMachineClass parent;
};

/* used by PROCESSOR_MACHINE_TYPE */
GType bt_processor_machine_get_type(void);

#endif // BT_PROCESSOR_MACHINE_H

