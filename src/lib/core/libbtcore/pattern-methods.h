/* $Id: pattern-methods.h,v 1.2 2004-07-15 16:56:07 ensonic Exp $
 * defines all public methods of the pattern class
 */

#ifndef BT_PATTERN_METHODS_H
#define BT_PATTERN_METHODS_H

#include "pattern.h"

extern GValue *bt_pattern_get_global_event_data(const BtPattern *self, glong tick, glong param);
extern GValue *bt_pattern_get_voice_event_data(const BtPattern *self, glong tick, glong voice, glong param);

extern glong bt_pattern_get_global_dparam_index(const BtPattern *self, const gchar *name);
extern glong bt_pattern_get_voice_dparam_index(const BtPattern *self, const gchar *name);

extern gboolean bt_pattern_set_event(const BtPattern *self, GValue *event, const gchar *value);

#endif // BT_PATTERN_METHDOS_H
