/* $Id: machine-methods.h,v 1.16 2005-01-04 18:02:17 ensonic Exp $
 * defines all public methods of the machine base class
 */

#ifndef BT_MACHINE_METHODS_H
#define BT_MACHINE_METHODS_H

#include "machine.h"
#include "pattern.h"

extern gboolean bt_machine_add_input_level(BtMachine *self);

extern gboolean bt_machine_activate_adder(BtMachine *self);
extern gboolean bt_machine_has_active_adder(BtMachine *self);
extern gboolean bt_machine_activate_spreader(BtMachine *self);
extern gboolean bt_machine_has_active_spreader(BtMachine *self);

extern void bt_machine_add_pattern(const BtMachine *self, const BtPattern *pattern);
extern BtPattern *bt_machine_get_pattern_by_id(const BtMachine *self,const gchar *id);
extern BtPattern *bt_machine_get_pattern_by_index(const BtMachine *self,gulong index);

extern glong bt_machine_get_global_dparam_index(const BtMachine *self, const gchar *name, GError **error);
extern glong bt_machine_get_voice_dparam_index(const BtMachine *self, const gchar *name, GError **error);

extern GType bt_machine_get_global_dparam_type(const BtMachine *self, gulong index);
extern GType bt_machine_get_voice_dparam_type(const BtMachine *self, gulong index);

extern void bt_machine_set_global_dparam_value(const BtMachine *self, gulong index, GValue *event);

extern gpointer bt_machine_pattern_iterator_new(const BtMachine *self);
extern gpointer bt_machine_pattern_iterator_next(gpointer iter);
extern BtPattern *bt_machine_pattern_iterator_get_pattern(gpointer iter);

#endif // BT_MACHINE_METHDOS_H
