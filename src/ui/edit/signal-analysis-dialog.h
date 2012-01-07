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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef BT_SIGNAL_ANALYSIS_DIALOG_H
#define BT_SIGNAL_ANALYSIS_DIALOG_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_SIGNAL_ANALYSIS_DIALOG            (bt_signal_analysis_dialog_get_type ())
#define BT_SIGNAL_ANALYSIS_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_SIGNAL_ANALYSIS_DIALOG, BtSignalAnalysisDialog))
#define BT_SIGNAL_ANALYSIS_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_SIGNAL_ANALYSIS_DIALOG, BtSignalAnalysisDialogClass))
#define BT_IS_SIGNAL_ANALYSIS_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_SIGNAL_ANALYSIS_DIALOG))
#define BT_IS_SIGNAL_ANALYSIS_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_SIGNAL_ANALYSIS_DIALOG))
#define BT_SIGNAL_ANALYSIS_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_SIGNAL_ANALYSIS_DIALOG, BtSignalAnalysisDialogClass))

/* type macros */

typedef struct _BtSignalAnalysisDialog BtSignalAnalysisDialog;
typedef struct _BtSignalAnalysisDialogClass BtSignalAnalysisDialogClass;
typedef struct _BtSignalAnalysisDialogPrivate BtSignalAnalysisDialogPrivate;

/**
 * BtSignalAnalysisDialog:
 *
 * the root window for the editor application
 */
struct _BtSignalAnalysisDialog {
  GtkWindow parent;
  
  /*< private >*/
  BtSignalAnalysisDialogPrivate *priv;
};

struct _BtSignalAnalysisDialogClass {
  GtkWindowClass parent;
  
};

GType bt_signal_analysis_dialog_get_type(void) G_GNUC_CONST;

BtSignalAnalysisDialog *bt_signal_analysis_dialog_new(const GstBin *element);

#endif // BT_SIGNAL_ANALYSIS_DIALOG_H
