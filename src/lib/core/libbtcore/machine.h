/* $Id: machine.h,v 1.3 2004-05-11 16:16:38 ensonic Exp $
 * base class for a machine
 */

#ifndef BT_MACHINE_H
#define BT_MACHINE_H

#include <glib.h>
#include <glib-object.h>

#define BT_MACHINE_TYPE		        (bt_machine_get_type ())
#define BT_MACHINE(obj)		        (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_MACHINE_TYPE, BtMachine))
#define BT_MACHINE_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), BT_MACHINE_TYPE, BtMachineClass))
#define BT_IS_MACHINE(obj)	        (G_TYPE_CHECK_TYPE ((obj), BT_MACHINE_TYPE))
#define BT_IS_MACHINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_MACHINE_TYPE))
#define BT_MACHINE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_MACHINE_TYPE, BtMachineClass))

/* type macros */

typedef struct _BtMachine BtMachine;
typedef struct _BtMachineClass BtMachineClass;
typedef struct _BtMachinePrivate BtMachinePrivate;

struct _BtMachine {
  GObject parent;

	/* gstreamer related */
	GstElement *machine,*adder,*spreader;
	GstDParamManager *dparam_manager;
	GstDParam **dparams;
	guint dparams_count;
	// convinience pointer
	GstElement *dst_elem,*src_elem;

  /* private */
  BtMachinePrivate *private;
};
/* structure of the machine class */
struct _BtMachineClass {
  GObjectClass parent;
  
};

/* used by MACHINE_TYPE */
GType bt_machine_get_type(void);


#endif // BT_MACHINE_H

