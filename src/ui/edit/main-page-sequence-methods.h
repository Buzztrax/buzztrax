/* $Id: main-page-sequence-methods.h,v 1.2 2004-08-25 16:25:22 ensonic Exp $
 * defines all public methods of the main machines page class
 */

#ifndef BT_MAIN_PAGE_SEQUENCE_METHODS_H
#define BT_MAIN_PAGE_SEQUENCE_METHODS_H

#include "main-page-sequence.h"
#include "edit-application.h"

extern BtMainPageSequence *bt_main_page_sequence_new(const BtEditApplication *app);

extern BtMachine *bt_main_page_sequence_get_current_machine(const BtMainPageSequence *self);

#endif // BT_MAIN_PAGE_SEQUENCE_METHDOS_H
