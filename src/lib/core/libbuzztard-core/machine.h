/* Buzztard
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

#ifndef BT_MACHINE_H
#define BT_MACHINE_H

#include <glib.h>
#include <glib-object.h>

#include <libbuzztard-ic/ic.h>

#define BT_TYPE_MACHINE            (bt_machine_get_type ())
#define BT_MACHINE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_MACHINE, BtMachine))
#define BT_MACHINE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_MACHINE, BtMachineClass))
#define BT_IS_MACHINE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_MACHINE))
#define BT_IS_MACHINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_MACHINE))
#define BT_MACHINE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_MACHINE, BtMachineClass))

/* type macros */

typedef struct _BtMachine BtMachine;
typedef struct _BtMachineClass BtMachineClass;
typedef struct _BtMachinePrivate BtMachinePrivate;

/**
 * BtMachine:
 * @src_wires: read-only list of outgoing #BtWire objects
 * @dst_wires: read-only list of incomming #BtWire objects
 *
 * Base object for a virtual piece of hardware (generator, effect, ...).
 */
struct _BtMachine {
  const GstBin parent;

  /*< public >*/
  
  /*< read-only >*/
  GList *src_wires;
  GList *dst_wires;

  /*< private >*/
  BtMachinePrivate *priv;
};

/**
 * BtMachineClass:
 * @check_type: sanity check that the given input/output characteristics are
 * okay for the implementation
 *
 * Base class for machines.
 */
struct _BtMachineClass {
  /*< private >*/
  const GstBinClass parent;

  /*< public >*/
  /* virtual methods for subclasses */
  gboolean (*check_type)(const BtMachine * const machine, const gulong pad_src_ct, const gulong pad_sink_ct);
};

#define BT_TYPE_MACHINE_STATE       (bt_machine_state_get_type())

/**
 * BtMachineState:
 * @BT_MACHINE_STATE_NORMAL: just working
 * @BT_MACHINE_STATE_MUTE: be quiet
 * @BT_MACHINE_STATE_SOLO: be the only one playing
 * @BT_MACHINE_STATE_BYPASS: be uneffective (pass through)
 *
 * A machine is always in one of the 4 states.
 * Use the "state" property of the #BtMachine to change or query the current state.
 */
typedef enum {
  BT_MACHINE_STATE_NORMAL=0,
  BT_MACHINE_STATE_MUTE,
  BT_MACHINE_STATE_SOLO,
  BT_MACHINE_STATE_BYPASS,
  /*< private >*/
  BT_MACHINE_STATE_COUNT
} BtMachineState;

GType bt_machine_get_type(void) G_GNUC_CONST;
GType bt_machine_state_get_type(void) G_GNUC_CONST;

gboolean bt_machine_enable_input_pre_level(BtMachine * const self);
gboolean bt_machine_enable_input_post_level(BtMachine * const self);
gboolean bt_machine_enable_output_pre_level(BtMachine * const self);
gboolean bt_machine_enable_output_post_level(BtMachine * const self);
gboolean bt_machine_enable_input_gain(BtMachine * const self);
gboolean bt_machine_enable_output_gain(BtMachine * const self);

gboolean bt_machine_activate_adder(BtMachine * const self);
gboolean bt_machine_has_active_adder(const BtMachine * const self);
gboolean bt_machine_activate_spreader(BtMachine * const self);
gboolean bt_machine_has_active_spreader(const BtMachine * const self);

// pattern handling

#include "pattern.h"
#include "wire.h"

void bt_machine_add_pattern(const BtMachine *self, const BtPattern *pattern);
void bt_machine_remove_pattern(const BtMachine *self, const BtPattern *pattern);

BtPattern *bt_machine_get_pattern_by_id(const BtMachine * const self, const gchar * const id);
BtPattern *bt_machine_get_pattern_by_name(const BtMachine * const self,const gchar * const name);
BtPattern *bt_machine_get_pattern_by_index(const BtMachine * const self, const gulong index);

gchar *bt_machine_get_unique_pattern_name(const BtMachine * const self);
gboolean bt_machine_has_patterns(const BtMachine * const self);

// global and voice param handling

gboolean bt_machine_is_polyphonic(const BtMachine * const self);

gboolean bt_machine_is_global_param_trigger(const BtMachine * const self, const gulong index);
gboolean bt_machine_is_voice_param_trigger(const BtMachine * const self, const gulong index);

gboolean bt_machine_is_global_param_no_value(const BtMachine * const self, const gulong index, GValue * const value);
gboolean bt_machine_is_voice_param_no_value(const BtMachine *const self, const gulong index, GValue * const value);

GValue *bt_machine_get_global_param_no_value(const BtMachine * const self, const gulong index);
GValue *bt_machine_get_voice_param_no_value(const BtMachine * const self, const gulong index);

glong bt_machine_get_global_wave_param_index(const BtMachine * const self);
glong bt_machine_get_voice_wave_param_index(const BtMachine * const self);

void bt_machine_set_global_param_default(const BtMachine * const self, const gulong index);
void bt_machine_set_voice_param_default(const BtMachine * const self, const gulong voice, const gulong index);
void bt_machine_set_param_defaults(const BtMachine *const self);

glong bt_machine_get_global_param_index(const BtMachine * const self, const gchar * const name, GError **error);
glong bt_machine_get_voice_param_index(const BtMachine * const self, const gchar * const name, GError **error);

GParamSpec *bt_machine_get_global_param_spec(const BtMachine * const self, const gulong index);
GParamSpec *bt_machine_get_voice_param_spec(const BtMachine * const self, const gulong index);

void bt_machine_get_global_param_details(const BtMachine * const self, const gulong index, GParamSpec **pspec, GValue **min_val, GValue **max_val);
void bt_machine_get_voice_param_details(const BtMachine * const self, const gulong index, GParamSpec **pspec, GValue **min_val, GValue **max_val);

GType bt_machine_get_global_param_type(const BtMachine * const self, const gulong index);
GType bt_machine_get_voice_param_type(const BtMachine * const self, const gulong index);

void bt_machine_set_global_param_value(const BtMachine * const self, const gulong index, GValue * const event);
void bt_machine_set_voice_param_value(const BtMachine * const self, const gulong voice, const gulong index, GValue * const event);

const gchar *bt_machine_get_global_param_name(const BtMachine * const self, const gulong index);
const gchar *bt_machine_get_voice_param_name(const BtMachine * const self, const gulong index);

gchar *bt_machine_describe_global_param_value(const BtMachine * const self, const gulong index, GValue * const event);
gchar *bt_machine_describe_voice_param_value(const BtMachine * const self, const gulong index, GValue * const event);

// controller handling

void bt_machine_global_controller_change_value(const BtMachine * const self, const gulong param, const GstClockTime timestamp, GValue *value);
void bt_machine_voice_controller_change_value(const BtMachine * const self, const gulong voice, const gulong param, const GstClockTime timestamp, GValue *value);

//-- interaction control

void bt_machine_bind_parameter_control(const BtMachine * const self, GstObject *object, const gchar *property_name, BtIcControl *control);
void bt_machine_unbind_parameter_control(const BtMachine * const self, GstObject *object, const char *property_name);
void bt_machine_unbind_parameter_controls(const BtMachine * const self);

//-- settings

void bt_machine_randomize_parameters(const BtMachine * const self);
void bt_machine_reset_parameters(const BtMachine * const self) ;

//-- linking

BtWire *bt_machine_get_wire_by_dst_machine(const BtMachine * const self, const BtMachine * const dst);

//-- debug helper

GList *bt_machine_get_element_list(const BtMachine * const self);
void bt_machine_dbg_print_parts(const BtMachine * const self);

#endif // BT_MACHINE_H
