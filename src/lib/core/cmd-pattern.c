/* Buzztard
 * Copyright (C) 2012 Buzztard team <buzztard-devel@lists.sf.net>
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
/**
 * SECTION:btcmdpattern
 * @short_description: class for an command pattern of a #BtMachine instance
 *
 * A command pattern is used in the #BtSequence to trigger certain actions.
 * The actions are defined as the #BtPatternCmd enum.
 */
/* TODO(ensonic): make the commands actually work
 * - break works as in breaking patterns
 * - mute, solo, bypass don't work at all
 *   - the mute-pattern would need to control the mute-parameter of a machines
 *     output-volume (input-volume for master)
 *   - solo would be interpreted as mute on all other generators
 *   - bypass would need a (controllable) gobject property
 *   - break would need to be able to undo mute, solo, bypass
 * - for now we would need to handle the special patterns in
 *   bt_sequence_repair_global_damage_entry()
 * - we could see if we can make BtMachine::state controllable
 *   - BtPatternCmd can be mapped 1:1 to BtMachineState (except _BREAK).
 *   - we would need a pad-probe to call gst_object_sync()
 *   - normal patterns + break would set the BtMachine::state=NORMAL
 *   - in sequence.c we need to do handle intern_damage (like global_damage) and
 *     we need BtMachine::bt_machine_intern_controller_change_value()
 */
#define BT_CORE
#define BT_CMD_PATTERN_C

#include "core_private.h"

//-- property ids

enum {
  CMD_PATTERN_SONG=1,
  CMD_PATTERN_MACHINE,
  CMD_PATTERN_ID,
  CMD_PATTERN_NAME,
  CMD_PATTERN_COMMAND
};

struct _BtCmdPatternPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* the song the pattern belongs to */
  G_POINTER_ALIAS(BtSong *,song);

  /* the machine the pattern belongs to */
  G_POINTER_ALIAS(BtMachine *,machine);
  
  /* the id, we can use to lookup the pattern */
  gchar *id;
  /* the display name of the pattern */
  gchar *name;

  BtPatternCmd cmd;
};

//-- the class

G_DEFINE_TYPE (BtCmdPattern, bt_cmd_pattern, G_TYPE_OBJECT);

//-- enums

GType bt_pattern_cmd_get_type(void) {
  static GType type = 0;
  if(G_UNLIKELY(type == 0)) {
    static const GEnumValue values[] = {
      { BT_PATTERN_CMD_NORMAL,"BT_PATTERN_CMD_NORMAL","normal" },
      { BT_PATTERN_CMD_MUTE,  "BT_PATTERN_CMD_MUTE",  "mute" },
      { BT_PATTERN_CMD_SOLO,  "BT_PATTERN_CMD_SOLO",  "solo" },
      { BT_PATTERN_CMD_BYPASS,"BT_PATTERN_CMD_BYPASS","bypass" },
      { BT_PATTERN_CMD_BREAK, "BT_PATTERN_CMD_BREAK", "break" },
      { 0, NULL, NULL},
    };
    type = g_enum_register_static("BtPatternCmd", values);
  }
  return type;
}

//-- helper methods

//-- signal handler

//-- constructor methods

/**
 * bt_cmd_pattern_new:
 * @song: the song the new instance belongs to
 * @machine: the machine the pattern belongs to
 * @cmd: a #BtPatternCmd
 *
 * Create a new default pattern instance containg the given @cmd event.
 * It will be automatically added to the machines pattern list.
 * If @cmd is %BT_PATTERN_CMD_NORMAL use bt_cmd_pattern_new() instead.
 *
 * Don't call this from applications.
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtCmdPattern *bt_cmd_pattern_new(const BtSong * const song, const BtMachine * const machine, const BtPatternCmd cmd) {
  BtCmdPattern *self;
  gchar *id=NULL,*name=NULL;

  if(BT_IS_MACHINE(machine)) {
    gchar *mid=NULL;
    // track commands in sequencer
    const gchar * const cmd_names[]={ N_("normal"),N_("break"),N_("mute"),N_("solo"),N_("bypass") };

    g_object_get((gpointer)machine,"id",&mid,NULL);
    // use spaces/_ to avoid clashes with normal patterns?
    id=g_strdup_printf("%s___%s",mid,cmd_names[cmd]);
    name=g_strdup_printf("   %s",_(cmd_names[cmd]));
    g_free(mid);
  }  
  // create the pattern
  self=BT_CMD_PATTERN(g_object_new(BT_TYPE_CMD_PATTERN,"song",song,"id",id,"name",name,"machine",machine,"command",cmd,NULL));
  g_free(id);
  g_free(name);
  return(self);
}

//-- methods

//-- wrapper

//-- g_object overrides

static void bt_cmd_pattern_constructed(GObject *object) {
  BtCmdPattern *self=BT_CMD_PATTERN(object);

  if(G_OBJECT_CLASS(bt_cmd_pattern_parent_class)->constructed)
    G_OBJECT_CLASS(bt_cmd_pattern_parent_class)->constructed(object);

  g_return_if_fail(BT_IS_SONG(self->priv->song));
  g_return_if_fail(BT_IS_STRING(self->priv->id));
  g_return_if_fail(BT_IS_STRING(self->priv->name));
  g_return_if_fail(BT_IS_MACHINE(self->priv->machine));
  
  GST_DEBUG("new cmd pattern. name='%s', id='%s'",self->priv->name,self->priv->id);

  // add the pattern to the machine
  bt_machine_add_pattern(self->priv->machine,self);
}

static void bt_cmd_pattern_get_property(GObject * const object, const guint property_id, GValue * const value, GParamSpec * const pspec) {
  const BtCmdPattern * const self = BT_CMD_PATTERN(object);

  return_if_disposed();
  switch (property_id) {
    case CMD_PATTERN_SONG: {
      g_value_set_object(value, self->priv->song);
    } break;
    case CMD_PATTERN_ID: {
      g_value_set_string(value, self->priv->id);
    } break;
    case CMD_PATTERN_NAME: {
      g_value_set_string(value, self->priv->name);
    } break;
    case CMD_PATTERN_MACHINE: {
      g_value_set_object(value, self->priv->machine);
    } break;
    case CMD_PATTERN_COMMAND: {
      g_value_set_enum(value, self->priv->cmd);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_cmd_pattern_set_property(GObject * const object, const guint property_id, const GValue * const value, GParamSpec * const pspec) {
  BtCmdPattern * const self = BT_CMD_PATTERN(object);

  return_if_disposed();
  switch (property_id) {
    case CMD_PATTERN_SONG: {
      self->priv->song = BT_SONG(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->song);
      //GST_DEBUG("set the song for pattern: %p",self->priv->song);
    } break;
    case CMD_PATTERN_ID: {
      g_free(self->priv->id);
      self->priv->id = g_value_dup_string(value);
      GST_DEBUG("set the id for pattern: %s",self->priv->id);
    } break;
    case CMD_PATTERN_NAME: {
      g_free(self->priv->name);
      self->priv->name = g_value_dup_string(value);
      GST_DEBUG("set the display name for the pattern: %s",self->priv->name);
    } break;
    case CMD_PATTERN_MACHINE: {
      self->priv->machine = BT_MACHINE(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->machine);
    } break;
    case CMD_PATTERN_COMMAND: {
      self->priv->cmd=g_value_get_enum(value);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_cmd_pattern_finalize(GObject * const object) {
  const BtCmdPattern * const self = BT_CMD_PATTERN(object);
  
  g_free(self->priv->id);
  g_free(self->priv->name);

  G_OBJECT_CLASS(bt_cmd_pattern_parent_class)->finalize(object);
}

//-- class internals

static void bt_cmd_pattern_init(BtCmdPattern *self) {
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_CMD_PATTERN, BtCmdPatternPrivate);
}

static void bt_cmd_pattern_class_init(BtCmdPatternClass * const klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);

  g_type_class_add_private(klass,sizeof(BtCmdPatternPrivate));

  gobject_class->constructed  = bt_cmd_pattern_constructed;
  gobject_class->set_property = bt_cmd_pattern_set_property;
  gobject_class->get_property = bt_cmd_pattern_get_property;
  gobject_class->finalize     = bt_cmd_pattern_finalize;

  g_object_class_install_property(gobject_class,CMD_PATTERN_SONG,
                                  g_param_spec_object("song",
                                     "song construct prop",
                                     "Song object, the pattern belongs to",
                                     BT_TYPE_SONG, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,CMD_PATTERN_MACHINE,
                                  g_param_spec_object("machine",
                                     "machine construct prop",
                                     "Machine object, the pattern belongs to",
                                     BT_TYPE_MACHINE, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,CMD_PATTERN_ID,
                                  g_param_spec_string("id",
                                     "id construct prop",
                                     "pattern identifier (unique per song)",
                                     "unnamed pattern", /* default value */
                                     G_PARAM_CONSTRUCT|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,CMD_PATTERN_NAME,
                                  g_param_spec_string("name",
                                     "name construct prop",
                                     "the display-name of the pattern",
                                     "unnamed", /* default value */
                                     G_PARAM_CONSTRUCT|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,CMD_PATTERN_COMMAND,
                                  g_param_spec_enum("command",
                                     "command prop",
                                     "the command of this pattern",
                                     BT_TYPE_PATTERN_CMD,  /* enum type */
                                     BT_PATTERN_CMD_BREAK, /* default value */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));
}

