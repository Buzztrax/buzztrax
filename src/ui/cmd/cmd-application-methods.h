/* $Id: cmd-application-methods.h,v 1.4 2004-07-30 15:15:51 ensonic Exp $
 * defines all public methods of the command application class
 */

#ifndef BT_CMD_APPLICATION_METHODS_H
#define BT_CMD_APPLICATION_METHODS_H

#include "cmd-application.h"

extern BtCmdApplication *bt_cmd_application_new(void);

extern gboolean bt_cmd_application_play(const BtCmdApplication *self, const gchar *input_file_name);
extern gboolean bt_cmd_application_info(const BtCmdApplication *self, const gchar *input_file_name);

#endif // BT_CMD_APPLICATION_METHDOS_H
