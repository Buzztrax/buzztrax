/* $Id: source-machine.h,v 1.2 2004-05-11 16:16:38 ensonic Exp $
 * class for a source machine
 */

#ifndef BT_SOURCE_MACHINE_H
#define BT_SOURCE_MACHINE_H

#include <glib.h>
#include <glib-object.h>

#define BT_SOURCE_MACHINE_TYPE		        (bt_source_machine_get_type ())
#define BT_SOURCE_MACHINE(obj)		        (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_SOURCE_MACHINE_TYPE, BtSourceMachine))
#define BT_SOURCE_MACHINE_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), BT_SOURCE_MACHINE_TYPE, BtSourceMachineClass))
#define BT_IS_SOURCE_MACHINE(obj)	        (G_TYPE_CHECK_TYPE ((obj), BT_SONG_IO_SOURCE_MACHINE))
#define BT_IS_SOURCE_MACHINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_SOURCE_MACHINE_TYPE))
#define BT_SOURCE_MACHINE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_SOURCE_MACHINE_TYPE, BtSourceMachineClass))

/* type macros */

typedef struct _BtSourceMachine BtSourceMachine;
typedef struct _BtSourceMachineClass BtSourceMachineClass;
typedef struct _BtSourceMachinePrivate BtSourceMachinePrivate;

struct _BtSourceMachine {
  BtMachine parent;
  
  /* private */
  BtSourceMachinePrivate *private;
};
/* structure of the source_machine class */
struct _BtSourceMachineClass {
  BtMachineClass parent;
};

/* used by SOURCE_MACHINE_TYPE */
GType bt_source_machine_get_type(void);

#endif // BT_SOURCE_MACHINE_H

