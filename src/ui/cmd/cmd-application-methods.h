/* $Id: cmd-application-methods.h,v 1.7 2005-09-27 17:59:15 ensonic Exp $
 * defines all public methods of the command application class
 */

#ifndef BT_CMD_APPLICATION_METHODS_H
#define BT_CMD_APPLICATION_METHODS_H

#include "cmd-application.h"

extern BtCmdApplication *bt_cmd_application_new(gboolean quiet);

extern gboolean bt_cmd_application_play(const BtCmdApplication *self, const gchar *input_file_name);
extern gboolean bt_cmd_application_info(const BtCmdApplication *self, const gchar *input_file_name, const gchar *output_file_name);
extern gboolean bt_cmd_application_convert(const BtCmdApplication *self, const gchar *input_file_name, const gchar *output_file_name);
extern gboolean bt_cmd_application_encode(const BtCmdApplication *self, const gchar *input_file_name, const gchar *output_file_name);

#endif // BT_CMD_APPLICATION_METHDOS_H
