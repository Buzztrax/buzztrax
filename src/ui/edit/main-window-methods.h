/* $Id: main-window-methods.h,v 1.2 2004-08-07 23:48:02 ensonic Exp $
 * defines all public methods of the main window class
 */

#ifndef BT_MAIN_WINDOW_METHODS_H
#define BT_MAIN_WINDOW_METHODS_H

#include "main-window.h"
#include "edit-application.h"

extern BtMainWindow *bt_main_window_new(const BtEditApplication *app);

extern gboolean bt_main_window_show_and_run(const BtMainWindow *self);

#endif // BT_MAIN_WINDOW_METHDOS_H
