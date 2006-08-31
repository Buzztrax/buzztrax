/* $Id: wire-analysis-dialog.h,v 1.2 2006-08-31 19:57:57 ensonic Exp $
 *
 * Buzztard
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
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
