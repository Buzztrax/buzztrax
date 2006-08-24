/* $Id: machine-methods.h,v 1.42 2006-08-24 20:00:52 ensonic Exp $
 *
 * Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef BT_MACHINE_METHODS_H
#define BT_MACHINE_METHODS_H

#include "machine.h"
#include "pattern.h"

extern gboolean bt_machine_enable_input_level(BtMachine *self);
extern gboolean bt_machine_enable_input_gain(BtMachine *self);
extern gboolean bt_machine_enable_output_gain(BtMachine *self);

extern gboolean bt_machine_activate_adder(BtMachine *self);
extern gboolean bt_machine_has_active_adder(BtMachine *self);
extern gboolean bt_machine_activate_spreader(BtMachine *self);
extern gboolean bt_machine_has_active_spreader(BtMachine *self);

// pattern handling

extern void bt_machine_add_pattern(const BtMachine *self, const BtPattern *pattern);
extern void bt_machine_remove_pattern(const BtMachine *self, const BtPattern *pattern);

extern BtPattern *bt_machine_get_pattern_by_id(const BtMachine *self,const gchar *id);
extern BtPattern *bt_machine_get_pattern_by_index(const BtMachine *self,gulong index);

extern gchar *bt_machine_get_unique_pattern_name(const BtMachine *self);

// global and voice param handling

extern gboolean bt_machine_is_polyphonic(const BtMachine *self);

extern glong bt_machine_get_global_param_index(const BtMachine *self, const gchar *name, GError **error);
extern glong bt_machine_get_voice_param_index(const BtMachine *self, const gchar *name, GError **error);

extern GParamSpec *bt_machine_get_global_param_spec(const BtMachine *self, gulong index);
extern GParamSpec *bt_machine_get_voice_param_spec(const BtMachine *self, gulong index);

extern GType bt_machine_get_global_param_type(const BtMachine *self, gulong index);
extern GType bt_machine_get_voice_param_type(const BtMachine *self, gulong index);

extern void bt_machine_set_global_param_value(const BtMachine *self, gulong index, GValue *event);
extern void bt_machine_set_voice_param_value(const BtMachine *self, gulong voice, gulong index, GValue *event);

extern void bt_machine_set_global_param_no_value(const BtMachine *self, gulong index);
extern void bt_machine_set_voice_param_no_value(const BtMachine *self, gulong voice, gulong index);

extern const gchar *bt_machine_get_global_param_name(const BtMachine *self, gulong index);
extern const gchar *bt_machine_get_voice_param_name(const BtMachine *self, gulong index);

extern GValue *bt_machine_get_global_param_min_value(const BtMachine *self, gulong index);
extern GValue *bt_machine_get_voice_param_min_value(const BtMachine *self, gulong index);

extern GValue *bt_machine_get_global_param_max_value(const BtMachine *self, gulong index);
extern GValue *bt_machine_get_voice_param_max_value(const BtMachine *self, gulong index);

extern gboolean bt_machine_is_global_param_trigger(const BtMachine *self, gulong index);
extern gboolean bt_machine_is_voice_param_trigger(const BtMachine *self, gulong index);

extern gboolean bt_machine_is_global_param_no_value(const BtMachine *self, gulong index, GValue *value);
extern gboolean bt_machine_is_voice_param_no_value(const BtMachine *self, gulong index, GValue *value);

extern gchar *bt_machine_describe_global_param_value(const BtMachine *self, gulong index, GValue *event);
extern gchar *bt_machine_describe_voice_param_value(const BtMachine *self, gulong index, GValue *event);

// controller handling

extern void bt_machine_global_controller_change_value(const BtMachine *self, gulong param, GstClockTime timestamp, GValue *value);
extern void bt_machine_voice_controller_change_value(const BtMachine *self, gulong param, gulong voice, GstClockTime timestamp, GValue *value);

// debug helper

extern GList *bt_machine_get_element_list(const BtMachine *self);
extern void bt_machine_dbg_print_parts(const BtMachine *self);
extern void bt_machine_dbg_dump_global_controller_queue(const BtMachine *self);

#endif // BT_MACHINE_METHDOS_H
