/* $Id: pattern-methods.h,v 1.9 2005-04-27 16:31:06 ensonic Exp $
 * defines all public methods of the pattern class
 */

#ifndef BT_PATTERN_METHODS_H
#define BT_PATTERN_METHODS_H

#include "pattern.h"

extern BtPattern *bt_pattern_new(const BtSong *song, const gchar *id, const gchar *name, glong length, glong voices,const BtMachine *machine);
extern BtPattern *bt_pattern_copy(const BtPattern *self);

extern GValue *bt_pattern_get_global_event_data(const BtPattern *self, gulong tick, gulong param);
extern GValue *bt_pattern_get_voice_event_data(const BtPattern *self, gulong tick, gulong voice, gulong param);

extern gulong bt_pattern_get_global_param_index(const BtPattern *self, const gchar *name, GError **error);
extern gulong bt_pattern_get_voice_param_index(const BtPattern *self, const gchar *name, GError **error);

extern void bt_pattern_init_global_event(const BtPattern *self, GValue *event, gulong param);
extern void bt_pattern_init_voice_event(const BtPattern *self, GValue *event, gulong param);

extern gboolean bt_pattern_set_event(const BtPattern *self, GValue *event, const gchar *value);
extern gchar *bt_pattern_get_event(const BtPattern *self, GValue *event);

extern gboolean bt_pattern_tick_has_data(const BtPattern *self, gulong index);

extern void bt_pattern_play_tick(const BtPattern *self, gulong index);

#endif // BT_PATTERN_METHDOS_H
