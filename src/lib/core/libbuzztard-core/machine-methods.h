/* $Id$
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

#include <libbuzztard-ic/ic.h>

extern gboolean bt_machine_enable_input_level(BtMachine * const self);
extern gboolean bt_machine_enable_output_level(BtMachine * const self);
extern gboolean bt_machine_enable_input_gain(BtMachine * const self);
extern gboolean bt_machine_enable_output_gain(BtMachine * const self);

extern gboolean bt_machine_activate_adder(BtMachine * const self);
extern gboolean bt_machine_has_active_adder(const BtMachine * const self);
extern gboolean bt_machine_activate_spreader(BtMachine * const self);
extern gboolean bt_machine_has_active_spreader(const BtMachine * const self);

//extern void bt_machine_renegotiate_adder_format(const BtMachine * const self, GstPad *pad);
extern void bt_machine_renegotiate_adder_format(const BtMachine * const self);

// pattern handling

extern void bt_machine_add_pattern(const BtMachine *self, const BtPattern *pattern);
extern void bt_machine_remove_pattern(const BtMachine *self, const BtPattern *pattern);

extern BtPattern *bt_machine_get_pattern_by_id(const BtMachine * const self, const gchar * const id);
extern BtPattern *bt_machine_get_pattern_by_index(const BtMachine * const self, const gulong index);

extern gchar *bt_machine_get_unique_pattern_name(const BtMachine * const self);
extern gboolean bt_machine_has_patterns(const BtMachine * const self);

// global and voice param handling

extern gboolean bt_machine_is_polyphonic(const BtMachine * const self);

extern gboolean bt_machine_is_global_param_trigger(const BtMachine * const self, const gulong index);
extern gboolean bt_machine_is_voice_param_trigger(const BtMachine * const self, const gulong index);

extern gboolean bt_machine_is_global_param_no_value(const BtMachine * const self, const gulong index, GValue * const value);
extern gboolean bt_machine_is_voice_param_no_value(const BtMachine *const self, const gulong index, GValue * const value);

extern glong bt_machine_get_global_param_index(const BtMachine * const self, const gchar * const name, GError **error);
extern glong bt_machine_get_voice_param_index(const BtMachine * const self, const gchar * const name, GError **error);

extern GParamSpec *bt_machine_get_global_param_spec(const BtMachine * const self, const gulong index);
extern GParamSpec *bt_machine_get_voice_param_spec(const BtMachine * const self, const gulong index);

extern void bt_machine_get_global_param_details(const BtMachine * const self, const gulong index, GParamSpec **pspec, GValue **min_val, GValue **max_val);
extern void bt_machine_get_voice_param_details(const BtMachine * const self, const gulong index, GParamSpec **pspec, GValue **min_val, GValue **max_val);

extern GType bt_machine_get_global_param_type(const BtMachine * const self, const gulong index);
extern GType bt_machine_get_voice_param_type(const BtMachine * const self, const gulong index);

extern void bt_machine_set_global_param_value(const BtMachine * const self, const gulong index, GValue * const event);
extern void bt_machine_set_voice_param_value(const BtMachine * const self, const gulong voice, const gulong index, GValue * const event);

extern void bt_machine_set_global_param_no_value(const BtMachine * const self, const gulong index);
extern void bt_machine_set_voice_param_no_value(const BtMachine * const self, const gulong voice, const gulong index);

extern const gchar *bt_machine_get_global_param_name(const BtMachine * const self, const gulong index);
extern const gchar *bt_machine_get_voice_param_name(const BtMachine * const self, const gulong index);

extern gchar *bt_machine_describe_global_param_value(const BtMachine * const self, const gulong index, GValue * const event);
extern gchar *bt_machine_describe_voice_param_value(const BtMachine * const self, const gulong index, GValue * const event);

// controller handling

extern void bt_machine_global_controller_change_value(const BtMachine * const self, const gulong param, const GstClockTime timestamp, GValue * const value);
extern void bt_machine_voice_controller_change_value(const BtMachine * const self, const gulong param, const gulong voice,  const GstClockTime timestamp, GValue * const value);

//-- interaction control
extern void bt_machine_bind_parameter_control(const BtMachine * const self, GstObject *object, const gchar *property_name, BtIcControl *control);
extern void bt_machine_unbind_parameter_control(const BtMachine * const self, GstObject *object, const char *property_name);
extern void bt_machine_unbind_parameter_controls(const BtMachine * const self);

//-- settings

extern void bt_machine_randomize_parameters(const BtMachine * const self);
extern void bt_machine_reset_parameters(const BtMachine * const self) ;

// debug helper

extern GList *bt_machine_get_element_list(const BtMachine * const self);
extern void bt_machine_dbg_print_parts(const BtMachine * const self);
extern void bt_machine_dbg_dump_global_controller_queue(const BtMachine * const self);

#endif // BT_MACHINE_METHDOS_H
