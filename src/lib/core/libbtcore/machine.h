/* $Id: machine.h,v 1.11 2004-09-29 16:56:46 ensonic Exp $
 * base class for a machine
 */

#ifndef BT_MACHINE_H
#define BT_MACHINE_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_MACHINE		         (bt_machine_get_type ())
#define BT_MACHINE(obj)		         (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_MACHINE, BtMachine))
#define BT_MACHINE_CLASS(klass)	   (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_MACHINE, BtMachineClass))
#define BT_IS_MACHINE(obj)	       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_MACHINE))
#define BT_IS_MACHINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_MACHINE))
#define BT_MACHINE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_MACHINE, BtMachineClass))

/* type macros */

typedef struct _BtMachine BtMachine;
typedef struct _BtMachineClass BtMachineClass;
typedef struct _BtMachinePrivate BtMachinePrivate;

/**
 * BtMachine:
 * @machine: the #GstElement that does the main signal processing
 * @adder: fan-in input pad adaptor,
 * only in use, when several other machines connect to this machines input pad
 * @spreader: fan-out output pad adapter
 * only in use, when several other machines connect to this machines output pad
 *
 * base object for a virtual piece of hardware
 */
struct _BtMachine {
  GObject parent;

	/* gstreamer related (accessed alot by the wire object) */
	GstElement *machine,*adder,*spreader;
	/* convinience pointers (accessed alot by the wire object) */
	GstElement *dst_elem,*src_elem;

  /* private */
  BtMachinePrivate *priv;
};
/* structure of the machine class */
struct _BtMachineClass {
  GObjectClass parent_class;
  
};

/* used by MACHINE_TYPE */
GType bt_machine_get_type(void);


#endif // BT_MACHINE_H

