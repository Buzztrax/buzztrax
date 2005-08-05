/* $Id: machine-properties-dialog.h,v 1.5 2005-08-05 09:36:18 ensonic Exp $
 * class for the machine properties dialog
 */

#ifndef BT_MACHINE_PROPERTIES_DIALOG_H
#define BT_MACHINE_PROPERTIES_DIALOG_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_MACHINE_PROPERTIES_DIALOG             (bt_machine_properties_dialog_get_type ())
#define BT_MACHINE_PROPERTIES_DIALOG(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_MACHINE_PROPERTIES_DIALOG, BtMachinePropertiesDialog))
#define BT_MACHINE_PROPERTIES_DIALOG_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_MACHINE_PROPERTIES_DIALOG, BtMachinePropertiesDialogClass))
#define BT_IS_MACHINE_PROPERTIES_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_MACHINE_PROPERTIES_DIALOG))
#define BT_IS_MACHINE_PROPERTIES_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_MACHINE_PROPERTIES_DIALOG))
#define BT_MACHINE_PROPERTIES_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_MACHINE_PROPERTIES_DIALOG, BtMachinePropertiesDialogClass))

/* type macros */

typedef struct _BtMachinePropertiesDialog BtMachinePropertiesDialog;
typedef struct _BtMachinePropertiesDialogClass BtMachinePropertiesDialogClass;
typedef struct _BtMachinePropertiesDialogPrivate BtMachinePropertiesDialogPrivate;

/**
 * BtMachinePropertiesDialog:
 *
 * the root window for the editor application
 */
struct _BtMachinePropertiesDialog {
  GtkWindow parent;
  
  /*< private >*/
  BtMachinePropertiesDialogPrivate *priv;
};
/* structure of the machine-properties-dialog class */
struct _BtMachinePropertiesDialogClass {
  GtkWindowClass parent;
  
};

/* used by MACHINE_PROPERTIES_DIALOG_TYPE */
GType bt_machine_properties_dialog_get_type(void);

#endif // BT_MACHINE_PROPERTIES_DIALOG_H
