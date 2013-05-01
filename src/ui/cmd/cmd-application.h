/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@lists.sf.net>
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

#ifndef BT_CMD_APPLICATION_H
#define BT_CMD_APPLICATION_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_CMD_APPLICATION             (bt_cmd_application_get_type ())
#define BT_CMD_APPLICATION(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_CMD_APPLICATION, BtCmdApplication))
#define BT_CMD_APPLICATION_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_CMD_APPLICATION, BtCmdApplicationClass))
#define BT_IS_CMD_APPLICATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_CMD_APPLICATION))
#define BT_IS_CMD_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_CMD_APPLICATION))
#define BT_CMD_APPLICATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_CMD_APPLICATION, BtCmdApplicationClass))

/* type macros */

typedef struct _BtCmdApplication BtCmdApplication;
typedef struct _BtCmdApplicationClass BtCmdApplicationClass;
typedef struct _BtCmdApplicationPrivate BtCmdApplicationPrivate;

/**
 * BtCmdApplication:
 *
 * #BtApplication subclass for the commandline application
 */
struct _BtCmdApplication {
  BtApplication parent;
  
  /*< private >*/
  BtCmdApplicationPrivate *priv;
};

struct _BtCmdApplicationClass {
  BtApplicationClass parent;
  
};

GType bt_cmd_application_get_type(void) G_GNUC_CONST;

BtCmdApplication *bt_cmd_application_new(gboolean quiet);

gboolean bt_cmd_application_play(const BtCmdApplication *self, const gchar *input_file_name);
gboolean bt_cmd_application_info(const BtCmdApplication *self, const gchar *input_file_name, const gchar *output_file_name);
gboolean bt_cmd_application_convert(const BtCmdApplication *self, const gchar *input_file_name, const gchar *output_file_name);
gboolean bt_cmd_application_encode(const BtCmdApplication *self, const gchar *input_file_name, const gchar *output_file_name);

#endif // BT_CMD_APPLICATION_H
