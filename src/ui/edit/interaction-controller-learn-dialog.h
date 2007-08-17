/* $Id: interaction-controller-learn-dialog.h,v 1.1 2007-08-17 13:37:50 berzerka Exp $
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

#ifndef BT_INTERACTION_CONTROLLER_LEARN_DIALOG_H
#define BT_INTERACTION_CONTROLLER_LEARN_DIALOG_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_INTERACTION_CONTROLLER_LEARN_DIALOG            (bt_interaction_controller_learn_dialog_get_type ())
#define BT_INTERACTION_CONTROLLER_LEARN_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_INTERACTION_CONTROLLER_LEARN_DIALOG, BtInteractionControllerLearnDialog))
#define BT_INTERACTION_CONTROLLER_LEARN_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_INTERACTION_CONTROLLER_LEARN_DIALOG, BtInteractionControllerLearnDialogClass))
#define BT_IS_INTERACTION_CONTROLLER_LEARN_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_INTERACTION_CONTROLLER_LEARN_DIALOG))
#define BT_IS_INTERACTION_CONTROLLER_LEARN_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_INTERACTION_CONTROLLER_LEARN_DIALOG))
#define BT_INTERACTION_CONTROLLER_LEARN_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_INTERACTION_CONTROLLER_LEARN_DIALOG, BtInteractionControllerLearnDialogClass))

/* type macros */

typedef struct _BtInteractionControllerLearnDialog BtInteractionControllerLearnDialog;
typedef struct _BtInteractionControllerLearnDialogClass BtInteractionControllerLearnDialogClass;
typedef struct _BtInteractionControllerLearnDialogPrivate BtInteractionControllerLearnDialogPrivate;

/**
 * BtInteractionControllerLearnDialog:
 *
 * dynamic controller learn dialog
 */
struct _BtInteractionControllerLearnDialog {
  GtkDialog parent;
  
  /*< private >*/
  BtInteractionControllerLearnDialogPrivate *priv;
};
/* structure of the learn-dialog class */
struct _BtInteractionControllerLearnDialogClass {
  GtkDialogClass parent;
};

GType bt_interaction_controller_learn_dialog_get_type(void) G_GNUC_CONST;

#endif // BT_INTERACTION_CONTROLLER_LEARN_DIALOG_H
