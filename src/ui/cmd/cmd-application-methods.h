/* $Id: cmd-application-methods.h,v 1.2 2004-07-28 13:25:21 ensonic Exp $
 * defines all public methods of the command application class
 */

#ifndef BT_CMD_APPLICATION_METHODS_H
#define BT_CMD_APPLICATION_METHODS_H

#include "cmd-application.h"

gboolean bt_cmd_application_play(const BtCmdApplication *app, const gchar *input_file_name);
gboolean bt_cmd_application_info(const BtCmdApplication *app, const gchar *input_file_name);

#endif // BT_CMD_APPLICATION_METHDOS_H
