/* $Id: cmd-application-methods.h,v 1.1 2004-05-12 17:34:58 ensonic Exp $
 * defines all public methods of the command application class
 */

#ifndef BT_CMD_APPLICATION_METHODS_H
#define BT_CMD_APPLICATION_METHODS_H

#include "cmd-application.h"

gboolean bt_cmd_application_run(const BtCmdApplication *app, int argc, char **argv);

#endif // BT_CMD_APPLICATION_METHDOS_H
