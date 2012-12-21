/* Buzztard
 * Copyright (C) 2007 Buzztard team <buzztard-devel@lists.sf.net>
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

#ifndef BT_ABOUT_DIALOG_H
#define BT_ABOUT_DIALOG_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_ABOUT_DIALOG            (bt_about_dialog_get_type ())
#define BT_ABOUT_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_ABOUT_DIALOG, BtAboutDialog))
#define BT_ABOUT_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_ABOUT_DIALOG, BtAboutDialogClass))
#define BT_IS_ABOUT_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_ABOUT_DIALOG))
#define BT_IS_ABOUT_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_ABOUT_DIALOG))
#define BT_ABOUT_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_ABOUT_DIALOG, BtAboutDialogClass))

/* type macros */

typedef struct _BtAboutDialog BtAboutDialog;
typedef struct _BtAboutDialogClass BtAboutDialogClass;
typedef struct _BtAboutDialogPrivate BtAboutDialogPrivate;

/**
 * BtAboutDialog:
 *
 * the about dialog for the editor application
 */
struct _BtAboutDialog {
  GtkAboutDialog parent;
  
  /*< private >*/
  BtAboutDialogPrivate *priv;
};

struct _BtAboutDialogClass {
  GtkAboutDialogClass parent;
  
};

GType bt_about_dialog_get_type(void) G_GNUC_CONST;

BtAboutDialog *bt_about_dialog_new(void);

#endif // BT_ABOUT_DIALOG_H
