/* $Id: edit-application.h,v 1.12 2006-08-31 19:57:57 ensonic Exp $
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

#ifndef BT_EDIT_APPLICATION_H
#define BT_EDIT_APPLICATION_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_EDIT_APPLICATION            (bt_edit_application_get_type ())
#define BT_EDIT_APPLICATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_EDIT_APPLICATION, BtEditApplication))
#define BT_EDIT_APPLICATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_EDIT_APPLICATION, BtEditApplicationClass))
#define BT_IS_EDIT_APPLICATION(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_EDIT_APPLICATION))
#define BT_IS_EDIT_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_EDIT_APPLICATION))
#define BT_EDIT_APPLICATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_EDIT_APPLICATION, BtEditApplicationClass))

/* type macros */

typedef struct _BtEditApplication BtEditApplication;
typedef struct _BtEditApplicationClass BtEditApplicationClass;
typedef struct _BtEditApplicationPrivate BtEditApplicationPrivate;

/**
 * BtEditApplication:
 *
 * #BtApplication subclass for the gtk editor application
 */
struct _BtEditApplication {
  BtApplication parent;
  
  /*< private >*/
  BtEditApplicationPrivate *priv;
};
/* structure of the edit-application class */
struct _BtEditApplicationClass {
  BtApplicationClass parent;

  void (*song_changed)(const BtEditApplication *app, gpointer user_data);
};

/* used by EDIT_APPLICATION_TYPE */
GType bt_edit_application_get_type(void) G_GNUC_CONST;

#endif // BT_EDIT_APPLICATION_H
