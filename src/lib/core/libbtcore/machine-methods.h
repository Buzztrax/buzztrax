/* $Id: machine-methods.h,v 1.21 2005-02-12 12:56:50 ensonic Exp $
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

extern void bt_machine_add_pattern(const BtMachine *self, const BtPattern *pattern);
extern BtPattern *bt_machine_get_pattern_by_id(const BtMachine *self,const gchar *id);
extern BtPattern *bt_machine_get_pattern_by_index(const BtMachine *self,gulong index);

extern glong bt_machine_get_global_dparam_index(const BtMachine *self, const gchar *name, GError **error);
extern glong bt_machine_get_voice_dparam_index(const BtMachine *self, const gchar *name, GError **error);

extern GstDParam *bt_machine_get_global_dparam(const BtMachine *self, gulong index);

extern GType bt_machine_get_global_dparam_type(const BtMachine *self, gulong index);
extern GType bt_machine_get_voice_dparam_type(const BtMachine *self, gulong index);

extern void bt_machine_set_global_dparam_value(const BtMachine *self, gulong index, GValue *event);
extern void bt_machine_set_voice_dparam_value(const BtMachine *self, gulong voice, gulong index, GValue *event);

#endif // BT_MACHINE_METHDOS_H
