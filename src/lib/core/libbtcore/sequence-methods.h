/* $Id: sequence-methods.h,v 1.3 2004-07-07 15:39:03 ensonic Exp $
 * defines all public methods of the sequence class
 */

#ifndef BT_SEQUENCE_METHODS_H
#define BT_SEQUENCE_METHODS_H

#include "sequence.h"

BtTimeLine *bt_sequence_get_timeline(const BtSequence *self,const glong time);

#endif // BT_SEQUENCE_METHDOS_H
