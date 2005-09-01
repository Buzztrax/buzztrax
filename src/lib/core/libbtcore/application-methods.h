/* $Id: application-methods.h,v 1.4 2005-09-01 22:05:04 ensonic Exp $
 * defines all public methods of the application class
 */

#ifndef BT_APPLICATION_METHODS_H
#define BT_APPLICATION_METHODS_H

#include "application.h"

extern void bt_application_add_bus_watch(const BtApplication *self,GstBusHandler handler,gpointer user_data);
extern void bt_application_remove_bus_watch(const BtApplication *self,GstBusHandler handler);

#endif // BT_APPLICATION_METHDOS_H
