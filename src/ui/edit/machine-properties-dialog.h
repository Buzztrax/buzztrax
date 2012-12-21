/* Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
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

struct _BtMachinePropertiesDialogClass {
  GtkWindowClass parent;
  
};

GType bt_machine_properties_dialog_get_type(void) G_GNUC_CONST;

BtMachinePropertiesDialog *bt_machine_properties_dialog_new(const BtMachine *machine);

#endif // BT_MACHINE_PROPERTIES_DIALOG_H
