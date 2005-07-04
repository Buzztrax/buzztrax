/* $Id: pattern-methods.h,v 1.11 2005-07-04 14:05:11 ensonic Exp $
 * defines all public methods of the pattern class
 */

#ifndef BT_PATTERN_METHODS_H
#define BT_PATTERN_METHODS_H

#include "pattern.h"

extern BtPattern *bt_pattern_new(const BtSong *song, const gchar *id, const gchar *name, gulong length, gulong voices,const BtMachine *machine);
extern BtPattern *bt_pattern_copy(const BtPattern *self);

extern gulong bt_pattern_get_global_param_index(const BtPattern *self, const gchar *name, GError **error);
extern gulong bt_pattern_get_voice_param_index(const BtPattern *self, const gchar *name, GError **error);

extern gboolean bt_pattern_set_global_event(const BtPattern *self, gulong tick, gulong param, const gchar *value);
extern gboolean bt_pattern_set_voice_event(const BtPattern *self, gulong tick, gulong voice, gulong param, const gchar *value);
extern gchar *bt_pattern_get_global_event(const BtPattern *self, gulong tick, gulong param);
extern gchar *bt_pattern_get_voice_event(const BtPattern *self, gulong tick, gulong voice, gulong param);

extern gboolean bt_pattern_tick_has_data(const BtPattern *self, gulong index);

extern void bt_pattern_play_tick(const BtPattern *self, gulong index);

#endif // BT_PATTERN_METHDOS_H
