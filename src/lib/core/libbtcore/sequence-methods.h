/* $Id: sequence-methods.h,v 1.11 2004-08-25 16:25:22 ensonic Exp $
 * defines all public methods of the sequence class
 */

#ifndef BT_SEQUENCE_METHODS_H
#define BT_SEQUENCE_METHODS_H

#include "sequence.h"
#include "timeline.h"

extern BtSequence *bt_sequence_new(const BtSong *song);

extern BtTimeLine *bt_sequence_get_timeline_by_time(const BtSequence *self,const glong time);
extern gulong bt_sequence_get_bar_time(const BtSequence *self);
extern gulong bt_sequence_get_loop_time(const BtSequence *self);

extern gboolean bt_sequence_play(const BtSequence *self);
extern gboolean bt_sequence_stop(const BtSequence *self);

#endif // BT_SEQUENCE_METHDOS_H
