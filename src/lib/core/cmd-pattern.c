/* Buzztrax
 * Copyright (C) 2012 Buzztrax team <buzztrax-devel@buzztrax.org>
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
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
 *   - bypass would need a (controllable) gobject property on basetransform
 *   - break would need to be able to undo mute, solo, bypass
 *   - see bt_machine_change_state() for how the manual control works
 *
 * - we could see if we can make BtMachine::state controllable and attach a
 *     BtPatternControlSource to it.
 *   - bt_machine_constructed() calles bt_machine_init_xxx_params() that creates
 *     BtParamGroups, which in trun create the control-cources, we could create
 *     the control-source for the state param from bt_machine_init_ctrl_params()
 *     directly.
 *   - BtPatternCmd can be mapped 1:1 to BtMachineState (except _BREAK).
 *   - we would need a pad-probe to call gst_object_sync()
 *   - normal patterns + break would set the BtMachine::state=NORMAL
 *   - in sequence.c we need to do handle intern_damage (like global_damage) and
 *     we need BtMachine::bt_machine_intern_controller_change_value()
 *
 *   - or when we create the ctrl param group we do this like in
 *     bt_wire_init_params(), see also bt_machine_set_mute()
 *     - then the cmd patterns need to set the right values, instead of just the
 *       cmd
 *     - won't work, since e.g. for solo, we change the state in other machines
 *       too
 *
 *  - or we need a special control_source here (BtCmdPatternControlSource), that
 *    will check the other-machines, this is the only way to have the changes
 *    properly synced
 *      for each other_machine:
 *        switch state:
 *          case solo: mute myself, return
 *      switch state:
 *         case mute: mute myself
 *         case solo: unmute myself
 *         case bypass: set bypass on basetransform
 *    - since we control e.g. volume::mute it will work without any extra
 *      pad-probe.
 *
 *  - what happens if one uses the solo pattern twice in the sequencer?
 *    -> song would be completely muted
 *  - if we use a custom control-source we still would like to sync the machine
 *    status, so that if a pattern switches to mute, the machine icons shows
 *    that :/
 *  - what should happen if a solo pattern is place while the prev pattern still
 *    runs?
 */
#define BT_CORE
#define BT_CMD_PATTERN_C

#include "core_private.h"

//-- property ids

enum
{
  CMD_PATTERN_SONG = 1,
  CMD_PATTERN_MACHINE,
  CMD_PATTERN_NAME,
  CMD_PATTERN_COMMAND
};

struct _BtCmdPatternPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the song the pattern belongs to */
  BtSong *song;

  /* the machine the pattern belongs to */
  BtMachine *machine;

  /* the display name of the pattern (unique for the machine) */
  gchar *name;

  BtPatternCmd cmd;
};

//-- the class

G_DEFINE_TYPE_WITH_CODE (BtCmdPattern, bt_cmd_pattern, G_TYPE_OBJECT, 
    G_ADD_PRIVATE(BtCmdPattern));

//-- enums

GType
bt_pattern_cmd_get_type (void)
{
  static GType type = 0;
  if (G_UNLIKELY (type == 0)) {
    static const GEnumValue values[] = {
      {BT_PATTERN_CMD_NORMAL, "BT_PATTERN_CMD_NORMAL", "normal"},
      {BT_PATTERN_CMD_MUTE, "BT_PATTERN_CMD_MUTE", "mute"},
      {BT_PATTERN_CMD_SOLO, "BT_PATTERN_CMD_SOLO", "solo"},
      {BT_PATTERN_CMD_BYPASS, "BT_PATTERN_CMD_BYPASS", "bypass"},
      {BT_PATTERN_CMD_BREAK, "BT_PATTERN_CMD_BREAK", "break"},
      {0, NULL, NULL},
    };
    type = g_enum_register_static ("BtPatternCmd", values);
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
 * If @cmd is %BT_PATTERN_CMD_NORMAL use bt_pattern_new() instead.
 *
 * Don't call this from applications.
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtCmdPattern *
bt_cmd_pattern_new (const BtSong * const song, const BtMachine * const machine,
    const BtPatternCmd cmd)
{
  return BT_CMD_PATTERN (g_object_new (BT_TYPE_CMD_PATTERN, "song", song,
          "machine", machine, "command", cmd, NULL));
}

//-- methods

//-- wrapper

//-- g_object overrides

static void
bt_cmd_pattern_constructed (GObject * object)
{
  BtCmdPattern *self = BT_CMD_PATTERN (object);

  if (G_OBJECT_CLASS (bt_cmd_pattern_parent_class)->constructed)
    G_OBJECT_CLASS (bt_cmd_pattern_parent_class)->constructed (object);

  g_return_if_fail (BT_IS_SONG (self->priv->song));
  g_return_if_fail (BT_IS_MACHINE (self->priv->machine));

  // finish setup and add the pattern to the machine
  // (if not subclassed, irks)
  if (self->priv->cmd != BT_PATTERN_CMD_NORMAL) {
    gchar *name;
    /* track commands in sequencer:
     *  normal: pattern with data and no command
     * mute: silence the machine
     * solo: only play this machine
     * bypass: deactivate this effect
     * break : interrupt the pattern
     */
    const gchar *const cmd_names[] =
        { N_("normal"), N_("mute"), N_("solo"), N_("bypass"), N_("break") };

    // use 3 spaces to avoid clashes with normal patterns?
    name = g_strdup_printf ("   %s", _(cmd_names[self->priv->cmd]));
    g_object_set (object, "name", name, NULL);
    g_free (name);

    GST_DEBUG ("new cmd pattern. name='%s'", self->priv->name);
    bt_machine_add_pattern (self->priv->machine, self);
  } else {
    g_return_if_fail (BT_IS_STRING (self->priv->name));
  }
}

static void
bt_cmd_pattern_get_property (GObject * const object, const guint property_id,
    GValue * const value, GParamSpec * const pspec)
{
  const BtCmdPattern *const self = BT_CMD_PATTERN (object);

  return_if_disposed ();
  switch (property_id) {
    case CMD_PATTERN_SONG:
      g_value_set_object (value, self->priv->song);
      break;
    case CMD_PATTERN_NAME:
      g_value_set_string (value, self->priv->name);
      break;
    case CMD_PATTERN_MACHINE:
      g_value_set_object (value, self->priv->machine);
      break;
    case CMD_PATTERN_COMMAND:
      g_value_set_enum (value, self->priv->cmd);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_cmd_pattern_set_property (GObject * const object, const guint property_id,
    const GValue * const value, GParamSpec * const pspec)
{
  BtCmdPattern *const self = BT_CMD_PATTERN (object);

  return_if_disposed ();
  switch (property_id) {
    case CMD_PATTERN_SONG:
      self->priv->song = BT_SONG (g_value_get_object (value));
      g_object_try_weak_ref (self->priv->song);
      //GST_DEBUG("set the song for pattern: %p",self->priv->song);
      break;
    case CMD_PATTERN_NAME:
      g_free (self->priv->name);
      self->priv->name = g_value_dup_string (value);
      GST_DEBUG ("set the display name for the pattern: '%s'",
          self->priv->name);
      break;
    case CMD_PATTERN_MACHINE:
      self->priv->machine = BT_MACHINE (g_value_get_object (value));
      g_object_try_weak_ref (self->priv->machine);
      break;
    case CMD_PATTERN_COMMAND:
      self->priv->cmd = g_value_get_enum (value);
      GST_DEBUG ("set the cmd for the pattern: %d", self->priv->cmd);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_cmd_pattern_dispose (GObject * const object)
{
  const BtCmdPattern *const self = BT_CMD_PATTERN (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  g_object_try_weak_unref (self->priv->machine);
  g_object_try_weak_unref (self->priv->song);

  G_OBJECT_CLASS (bt_cmd_pattern_parent_class)->dispose (object);
}

static void
bt_cmd_pattern_finalize (GObject * const object)
{
  const BtCmdPattern *const self = BT_CMD_PATTERN (object);

  g_free (self->priv->name);

  G_OBJECT_CLASS (bt_cmd_pattern_parent_class)->finalize (object);
}

//-- class internals

static void
bt_cmd_pattern_init (BtCmdPattern * self)
{
  self->priv = bt_cmd_pattern_get_instance_private(self);
}

static void
bt_cmd_pattern_class_init (BtCmdPatternClass * const klass)
{
  GObjectClass *const gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructed = bt_cmd_pattern_constructed;
  gobject_class->set_property = bt_cmd_pattern_set_property;
  gobject_class->get_property = bt_cmd_pattern_get_property;
  gobject_class->dispose = bt_cmd_pattern_dispose;
  gobject_class->finalize = bt_cmd_pattern_finalize;

  g_object_class_install_property (gobject_class, CMD_PATTERN_SONG,
      g_param_spec_object ("song", "song construct prop",
          "Song object, the pattern belongs to", BT_TYPE_SONG,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, CMD_PATTERN_MACHINE,
      g_param_spec_object ("machine", "machine construct prop",
          "Machine object, the pattern belongs to", BT_TYPE_MACHINE,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, CMD_PATTERN_NAME,
      g_param_spec_string ("name", "name construct prop",
          "the display-name of the pattern", "unnamed",
          G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, CMD_PATTERN_COMMAND,
      g_param_spec_enum ("command", "command prop",
          "the command of this pattern", BT_TYPE_PATTERN_CMD,
          BT_PATTERN_CMD_NORMAL,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}
