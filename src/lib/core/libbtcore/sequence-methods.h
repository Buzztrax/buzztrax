/* $Id: sequence-methods.h,v 1.5 2004-07-15 16:56:07 ensonic Exp $
 * defines all public methods of the sequence class
 */

#ifndef BT_SEQUENCE_METHODS_H
#define BT_SEQUENCE_METHODS_H

#include "sequence.h"

extern BtTimeLine *bt_sequence_get_timeline(const BtSequence *self,const glong time);

extern void bt_sequence_play(const BtSequence *self);

#endif // BT_SEQUENCE_METHDOS_H
