/* $Id: cmd-application-methods.h,v 1.3 2004-07-29 15:51:31 ensonic Exp $
 * defines all public methods of the command application class
 */

#ifndef BT_CMD_APPLICATION_METHODS_H
#define BT_CMD_APPLICATION_METHODS_H

#include "cmd-application.h"

gboolean bt_cmd_application_play(const BtCmdApplication *self, const gchar *input_file_name);
gboolean bt_cmd_application_info(const BtCmdApplication *self, const gchar *input_file_name);

#endif // BT_CMD_APPLICATION_METHDOS_H
