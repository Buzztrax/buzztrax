/* $Id: machine-methods.h,v 1.25 2005-04-22 17:34:18 ensonic Exp $
 * defines all public methods of the machine base class
 */

#ifndef BT_MACHINE_METHODS_H
#define BT_MACHINE_METHODS_H

#include "machine.h"
#include "pattern.h"

extern gboolean bt_machine_enable_input_level(BtMachine *self);
extern gboolean bt_machine_enable_input_gain(BtMachine *self);

extern gboolean bt_machine_activate_adder(BtMachine *self);
extern gboolean bt_machine_has_activated_adder(BtMachine *self);
extern gboolean bt_machine_activate_spreader(BtMachine *self);
extern gboolean bt_machine_has_activated_spreader(BtMachine *self);

// pattern handling

extern void bt_machine_add_pattern(const BtMachine *self, const BtPattern *pattern);
extern void bt_machine_remove_pattern(const BtMachine *self, const BtPattern *pattern);

extern BtPattern *bt_machine_get_pattern_by_id(const BtMachine *self,const gchar *id);
extern BtPattern *bt_machine_get_pattern_by_index(const BtMachine *self,gulong index);

extern gchar *bt_machine_get_unique_pattern_name(const BtMachine *self);

// global and voice param handling

extern gboolean bt_machine_is_polyphonic(const BtMachine *self);

extern glong bt_machine_get_global_dparam_index(const BtMachine *self, const gchar *name, GError **error);
extern glong bt_machine_get_voice_dparam_index(const BtMachine *self, const gchar *name, GError **error);

extern GstDParam *bt_machine_get_global_dparam(const BtMachine *self, gulong index);
extern GstDParam *bt_machine_get_voice_dparam(const BtMachine *self, gulong voice, gulong index);

extern GType bt_machine_get_global_dparam_type(const BtMachine *self, gulong index);
extern GType bt_machine_get_voice_dparam_type(const BtMachine *self, gulong index);

extern void bt_machine_set_global_dparam_value(const BtMachine *self, gulong index, GValue *event);
extern void bt_machine_set_voice_dparam_value(const BtMachine *self, gulong voice, gulong index, GValue *event);

extern const gchar *bt_machine_get_global_dparam_name(const BtMachine *self, gulong index);
extern const gchar *bt_machine_get_voice_dparam_name(const BtMachine *self, gulong index);

#endif // BT_MACHINE_METHDOS_H
