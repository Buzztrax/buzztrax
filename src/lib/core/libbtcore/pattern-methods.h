/* $Id: pattern-methods.h,v 1.3 2004-07-20 18:24:18 ensonic Exp $
 * defines all public methods of the pattern class
 */

#ifndef BT_PATTERN_METHODS_H
#define BT_PATTERN_METHODS_H

#include "pattern.h"

extern GValue *bt_pattern_get_global_event_data(const BtPattern *self, glong tick, glong param);
extern GValue *bt_pattern_get_voice_event_data(const BtPattern *self, glong tick, glong voice, glong param);

extern glong bt_pattern_get_global_dparam_index(const BtPattern *self, const gchar *name);
extern glong bt_pattern_get_voice_dparam_index(const BtPattern *self, const gchar *name);

extern void bt_pattern_init_global_event(const BtPattern *self, GValue *event, glong param);
extern void bt_pattern_init_voice_event(const BtPattern *self, GValue *event, glong param);

extern gboolean bt_pattern_set_event(const BtPattern *self, GValue *event, const gchar *value);

extern void bt_pattern_play_tick(const BtPattern *self, glong index);

#endif // BT_PATTERN_METHDOS_H
