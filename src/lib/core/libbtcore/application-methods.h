/* $Id: application-methods.h,v 1.5 2005-09-19 21:23:07 ensonic Exp $
 * defines all public methods of the application class
 */

#ifndef BT_APPLICATION_METHODS_H
#define BT_APPLICATION_METHODS_H

#include "application.h"

extern void bt_application_add_bus_watch(const BtApplication *self,GstBusFunc handler,gpointer user_data);
extern void bt_application_remove_bus_watch(const BtApplication *self,GstBusFunc handler);

#endif // BT_APPLICATION_METHDOS_H
