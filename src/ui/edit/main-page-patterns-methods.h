/* $Id: main-page-patterns-methods.h,v 1.4 2005-01-08 14:22:54 ensonic Exp $
 * defines all public methods of the main pattern page class
 */

#ifndef BT_MAIN_PAGE_PATTERNS_METHODS_H
#define BT_MAIN_PAGE_PATTERNS_METHODS_H

#include "main-page-patterns.h"
#include "edit-application.h"

extern BtMainPagePatterns *bt_main_page_patterns_new(const BtEditApplication *app);

extern BtMachine *bt_main_page_patterns_get_current_machine(const BtMainPagePatterns *self);
extern BtPattern *bt_main_page_patterns_get_current_pattern(const BtMainPagePatterns *self);

#endif // BT_MAIN_PAGE_PATTERNS_METHDOS_H
