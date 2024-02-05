/* Buzztrax
 * Copyright (C) 2010 Buzztrax team <buzztrax-devel@buzztrax.org>
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

#ifndef BT_CRASH_RECOVER_DIALOG_H
#define BT_CRASH_RECOVER_DIALOG_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#define BT_TYPE_CRASH_RECOVER_DIALOG            (bt_crash_recover_dialog_get_type ())
#define BT_CRASH_RECOVER_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_CRASH_RECOVER_DIALOG, BtCrashRecoverDialog))
#define BT_CRASH_RECOVER_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_CRASH_RECOVER_DIALOG, BtCrashRecoverDialogClass))
#define BT_IS_CRASH_RECOVER_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_CRASH_RECOVER_DIALOG))
#define BT_IS_CRASH_RECOVER_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_CRASH_RECOVER_DIALOG))
#define BT_CRASH_RECOVER_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_CRASH_RECOVER_DIALOG, BtCrashRecoverDialogClass))

/* type macros */

typedef struct _BtCrashRecoverDialog BtCrashRecoverDialog;
typedef struct _BtCrashRecoverDialogClass BtCrashRecoverDialogClass;
typedef struct _BtCrashRecoverDialogPrivate BtCrashRecoverDialogPrivate;

/**
 * BtCrashRecoverDialog:
 *
 * the about dialog for the editor application
 */
struct _BtCrashRecoverDialog {
  GtkWindow parent;
  
  /*< private >*/
  BtCrashRecoverDialogPrivate *priv;
};

struct _BtCrashRecoverDialogClass {
  GtkDialogClass parent;
  
};

GType bt_crash_recover_dialog_get_type(void) G_GNUC_CONST;

BtCrashRecoverDialog *bt_crash_recover_dialog_new(GList *crash_entries);

#endif // BT_CRASH_RECOVER_DIALOG_H
