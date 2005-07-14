/* $Id: main-page-sequence-methods.h,v 1.5 2005-07-14 21:44:10 ensonic Exp $
 * defines all public methods of the main sequence page class
 */

#ifndef BT_MAIN_PAGE_SEQUENCE_METHODS_H
#define BT_MAIN_PAGE_SEQUENCE_METHODS_H

#include "main-page-sequence.h"
#include "edit-application.h"

extern BtMainPageSequence *bt_main_page_sequence_new(const BtEditApplication *app);

extern BtMachine *bt_main_page_sequence_get_current_machine(const BtMainPageSequence *self);
extern gboolean bt_main_page_sequence_get_current_pos(const BtMainPageSequence *self,gulong *time,gulong *track);

#endif // BT_MAIN_PAGE_SEQUENCE_METHDOS_H
