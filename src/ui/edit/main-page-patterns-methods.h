/* $Id: main-page-patterns-methods.h,v 1.2 2004-08-24 14:10:04 ensonic Exp $
 * defines all public methods of the main machines page class
 */

#ifndef BT_MAIN_PAGE_PATTERNS_METHODS_H
#define BT_MAIN_PAGE_PATTERNS_METHODS_H

#include "main-page-patterns.h"
#include "edit-application.h"

extern BtMainPagePatterns *bt_main_page_patterns_new(const BtEditApplication *app);

extern BtMachine *bt_main_page_patterns_get_current_machine(const BtMainPagePatterns *self);

#endif // BT_MAIN_PAGE_PATTERNS_METHDOS_H
