/* $Id: settings-dialog.h,v 1.4 2005-10-08 18:12:13 ensonic Exp $
 * class for the editor settings dialog
 */

#ifndef BT_SETTINGS_DIALOG_H
#define BT_SETTINGS_DIALOG_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_SETTINGS_DIALOG             (bt_settings_dialog_get_type ())
#define BT_SETTINGS_DIALOG(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_SETTINGS_DIALOG, BtSettingsDialog))
#define BT_SETTINGS_DIALOG_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_SETTINGS_DIALOG, BtSettingsDialogClass))
#define BT_IS_SETTINGS_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_SETTINGS_DIALOG))
#define BT_IS_SETTINGS_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_SETTINGS_DIALOG))
#define BT_SETTINGS_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_SETTINGS_DIALOG, BtSettingsDialogClass))

/* type macros */

typedef struct _BtSettingsDialog BtSettingsDialog;
typedef struct _BtSettingsDialogClass BtSettingsDialogClass;
typedef struct _BtSettingsDialogPrivate BtSettingsDialogPrivate;

/**
 * BtSettingsDialog:
 *
 * the root window for the editor application
 */
struct _BtSettingsDialog {
  GtkDialog parent;
  
  /*< private >*/
  BtSettingsDialogPrivate *priv;
};
/* structure of the settings-dialog class */
struct _BtSettingsDialogClass {
  GtkDialogClass parent;
  
};

/* used by SETTINGS_DIALOG_TYPE */
GType bt_settings_dialog_get_type(void) G_GNUC_CONST;

#endif // BT_SETTINGS_DIALOG_H
