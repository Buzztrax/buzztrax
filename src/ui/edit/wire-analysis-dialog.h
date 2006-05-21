/* $Id: wire-analysis-dialog.h,v 1.1 2006-05-21 20:24:31 ensonic Exp $
 * class for the wire analysis dialog
 */

#ifndef BT_WIRE_ANALYSIS_DIALOG_H
#define BT_WIRE_ANALYSIS_DIALOG_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_WIRE_ANALYSIS_DIALOG            (bt_wire_analysis_dialog_get_type ())
#define BT_WIRE_ANALYSIS_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_WIRE_ANALYSIS_DIALOG, BtWireAnalysisDialog))
#define BT_WIRE_ANALYSIS_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_WIRE_ANALYSIS_DIALOG, BtWireAnalysisDialogClass))
#define BT_IS_WIRE_ANALYSIS_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_WIRE_ANALYSIS_DIALOG))
#define BT_IS_WIRE_ANALYSIS_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_WIRE_ANALYSIS_DIALOG))
#define BT_WIRE_ANALYSIS_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_WIRE_ANALYSIS_DIALOG, BtWireAnalysisDialogClass))

/* type macros */

typedef struct _BtWireAnalysisDialog BtWireAnalysisDialog;
typedef struct _BtWireAnalysisDialogClass BtWireAnalysisDialogClass;
typedef struct _BtWireAnalysisDialogPrivate BtWireAnalysisDialogPrivate;

/**
 * BtWireAnalysisDialog:
 *
 * the root window for the editor application
 */
struct _BtWireAnalysisDialog {
  GtkWindow parent;
  
  /*< private >*/
  BtWireAnalysisDialogPrivate *priv;
};
/* structure of the machine-preferences-dialog class */
struct _BtWireAnalysisDialogClass {
  GtkWindowClass parent;
  
};

/* used by WIRE_ANALYSIS_DIALOG_TYPE */
GType bt_wire_analysis_dialog_get_type(void) G_GNUC_CONST;

#endif // BT_WIRE_ANALYSIS_DIALOG_H
