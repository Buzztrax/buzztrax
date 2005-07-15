/* $Id: sequence-methods.h,v 1.16 2005-07-15 22:26:26 ensonic Exp $
 * defines all public methods of the sequence class
 */

#ifndef BT_SEQUENCE_METHODS_H
#define BT_SEQUENCE_METHODS_H

#include "sequence.h"
#include "pattern.h"
#include "machine.h"

extern BtSequence *bt_sequence_new(const BtSong *song);

extern BtMachine *bt_sequence_get_machine(const BtSequence *self,const gulong track);
extern void bt_sequence_set_machine(const BtSequence *self,const gulong track,const BtMachine *machine);
extern gchar *bt_sequence_get_label(const BtSequence *self,const gulong time);
extern void bt_sequence_set_label(const BtSequence *self,const gulong time, const gchar *label);
extern BtPattern *bt_sequence_get_pattern(const BtSequence *self,const gulong time,const gulong track);
extern void bt_sequence_set_pattern(const BtSequence *self,const gulong time,const gulong track,const BtPattern *pattern);

extern GstClockTime bt_sequence_get_bar_time(const BtSequence *self);
extern GstClockTime bt_sequence_get_loop_time(const BtSequence *self);

extern GList *bt_sequence_get_pattern_positions(const BtSequence *self,const BtMachine *machine,const BtPattern *pattern);

extern gboolean bt_sequence_play(const BtSequence *self);
extern gboolean bt_sequence_stop(const BtSequence *self);

#endif // BT_SEQUENCE_METHDOS_H
