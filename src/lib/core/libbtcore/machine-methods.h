/* $Id: machine-methods.h,v 1.8 2004-07-20 18:24:18 ensonic Exp $
 * defines all public methods of the machine base class
 */

#ifndef BT_MACHINE_METHODS_H
#define BT_MACHINE_METHODS_H

#include "machine.h"
#include "pattern.h"

extern void bt_machine_add_pattern(const BtMachine *self, const BtPattern *pattern);
extern BtPattern *bt_machine_get_pattern_by_id(const BtMachine *self,const gchar *id);

extern glong bt_machine_get_global_dparam_index(const BtMachine *self, const gchar *name);
extern glong bt_machine_get_voice_dparam_index(const BtMachine *self, const gchar *name);

extern GType bt_machine_get_global_dparam_type(const BtMachine *self, glong index);
extern GType bt_machine_get_voice_dparam_type(const BtMachine *self, glong index);

extern void bt_machine_set_global_dparam_value(const BtMachine *self, glong index, GValue *event);

#endif // BT_MACHINE_METHDOS_H
