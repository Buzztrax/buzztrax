/* $Id: edit-application-methods.h,v 1.1 2004-07-29 15:51:31 ensonic Exp $
 * defines all public methods of the edit application class
 */

#ifndef BT_EDIT_APPLICATION_METHODS_H
#define BT_EDIT_APPLICATION_METHODS_H

#include "edit-application.h"

gboolean bt_edit_application_run(const BtEditApplication *self);
gboolean bt_edit_application_load_and_run(const BtEditApplication *self, const gchar *input_file_name);

#endif // BT_EDIT_APPLICATION_METHDOS_H
