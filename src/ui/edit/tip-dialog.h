/* Buzztard
 * Copyright (C) 2010 Buzztard team <buzztard-devel@lists.sf.net>
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

#ifndef BT_TIP_DIALOG_H
#define BT_TIP_DIALOG_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_TIP_DIALOG            (bt_tip_dialog_get_type ())
#define BT_TIP_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_TIP_DIALOG, BtTipDialog))
#define BT_TIP_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_TIP_DIALOG, BtTipDialogClass))
#define BT_IS_TIP_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_TIP_DIALOG))
#define BT_IS_TIP_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_TIP_DIALOG))
#define BT_TIP_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_TIP_DIALOG, BtTipDialogClass))

/* type macros */

typedef struct _BtTipDialog BtTipDialog;
typedef struct _BtTipDialogClass BtTipDialogClass;
typedef struct _BtTipDialogPrivate BtTipDialogPrivate;

/**
 * BtTipDialog:
 *
 * the about dialog for the editor application
 */
struct _BtTipDialog {
  GtkDialog parent;
  
  /*< private >*/
  BtTipDialogPrivate *priv;
};
struct _BtTipDialogClass {
  GtkDialogClass parent;
  
};

GType bt_tip_dialog_get_type(void) G_GNUC_CONST;

BtTipDialog *bt_tip_dialog_new(void);

#endif // BT_TIP_DIALOG_H
