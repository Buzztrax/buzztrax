/** $Id: sink-machine.h,v 1.1 2004-05-07 16:29:28 ensonic Exp $
 * class for a sink machine
 */

#ifndef BT_SINK_MACHINE_H
#define BT_SINK_MACHINE_H

#include <glib.h>
#include <glib-object.h>

#define BT_SINK_MACHINE_TYPE		        (bt_sink_machine_get_type ())
#define BT_SINK_MACHINE(obj)		        (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_SINK_MACHINE_TYPE, BtSinkMachine))
#define BT_SINK_MACHINE_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), BT_SINK_MACHINE_TYPE, BtSinkMachineClass))
#define BT_IS_SINK_MACHINE(obj)	        (G_TYPE_CHECK_TYPE ((obj), BT_SONG_IO_SINK_MACHINE))
#define BT_IS_SINK_MACHINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_SINK_MACHINE_TYPE))
#define BT_SINK_MACHINE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_SINK_MACHINE_TYPE, BtSinkMachineClass))

/* type macros */

typedef struct _BtSinkMachine BtSinkMachine;
typedef struct _BtSinkMachineClass BtSinkMachineClass;
typedef struct _BtSinkMachinePrivate BtSinkMachinePrivate;

struct _BtSinkMachine {
  BtMachine parent;
  
  /* private */
  BtSinkMachinePrivate *private;
};
/* structure of the sink_machine class */
struct _BtSinkMachineClass {
  BtMachineClass parent;
};

/* used by SINK_MACHINE_TYPE */
GType bt_sink_machine_get_type(void);

#endif // BT_SINK_MACHINE_H

