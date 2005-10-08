/* $Id: machine.h,v 1.20 2005-10-08 18:12:13 ensonic Exp $
 * base class for a machine
 */

#ifndef BT_MACHINE_H
#define BT_MACHINE_H

#include <glib.h>
#include <glib-object.h>
#include "pattern.h"

#define BT_TYPE_MACHINE             (bt_machine_get_type ())
#define BT_MACHINE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_MACHINE, BtMachine))
#define BT_MACHINE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_MACHINE, BtMachineClass))
#define BT_IS_MACHINE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_MACHINE))
#define BT_IS_MACHINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_MACHINE))
#define BT_MACHINE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_MACHINE, BtMachineClass))

/* type macros */

typedef struct _BtMachine BtMachine;
typedef struct _BtMachineClass BtMachineClass;
typedef struct _BtMachinePrivate BtMachinePrivate;

/**
 * BtMachine:
 *
 * base object for a virtual piece of hardware
 */
struct _BtMachine {
  GObject parent;

  /* convinience pointers (accessed alot by the wire object) */
  GstElement *dst_elem;
  GstElement *src_elem;

  /*< private >*/
  BtMachinePrivate *priv;
};
/* structure of the machine class */
struct _BtMachineClass {
  GObjectClass parent_class;

  void (*pattern_added_event)(const BtMachine *machine, const BtPattern *pattern, gpointer user_data);
  void (*pattern_removed_event)(const BtMachine *machine, const BtPattern *pattern, gpointer user_data);
};

#define BT_TYPE_MACHINE_STATE       (bt_machine_state_get_type())

/**
 * BtMachineState:
 * @BT_MACHINE_STATE_NORMAL: just working
 * @BT_MACHINE_STATE_MUTE: be quiet
 * @BT_MACHINE_STATE_SOLO: be the only one playing
 * @BT_MACHINE_STATE_BYPASS: be uneffective (pass through)
 *
 * A machine is always in one of the 4 states.
 * Use the "state" property of the #BtMachine to change or query the current state.
 */
typedef enum {
  BT_MACHINE_STATE_NORMAL=0,
  BT_MACHINE_STATE_MUTE,
  BT_MACHINE_STATE_SOLO,
  BT_MACHINE_STATE_BYPASS
} BtMachineState;

/* used by MACHINE_TYPE */
GType bt_machine_get_type(void) G_GNUC_CONST;
/* used by MACHINE_STATE_TYPE */
GType bt_machine_state_get_type(void) G_GNUC_CONST;

#endif // BT_MACHINE_H
