/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@buzztrax.org>
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

#include "m-bt-core.h"

//-- globals

static BtApplication *app;
static BtSong *song;
static BtIcRegistry *registry;
static BtIcDevice *device;

//-- fixtures

static void
case_setup (void)
{
  BT_CASE_START;
  btic_init (NULL, NULL);
}

static void
test_setup (void)
{
  app = bt_test_application_new ();
  song = bt_song_new (app);
  BtMachineConstructorParams cparams;
  cparams.id = "master";
  cparams.song = song;
  
  bt_sink_machine_new (&cparams, NULL);

  registry = btic_registry_new ();
  device = (BtIcDevice *) btic_test_device_new ("test");
  btic_registry_add_device (device);
}

static void
test_teardown (void)
{
  ck_g_object_final_unref (registry);
  ck_g_object_final_unref (song);
  ck_g_object_final_unref (app);
}

static void
case_teardown (void)
{
}


//-- tests

// show machine properties dialog
static gchar *element_names[] = {
  "buzztrax-test-no-arg-mono-source", "buzztrax-test-mono-source",
  "buzztrax-test-poly-source", "buzztrax-test-poly-source"
};
static gulong element_voices[] = { 0, 0, 0, 1 };

START_TEST (test_bt_machine_create)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");

  GST_INFO ("-- act --");
  GError *err = NULL;
  BtMachineConstructorParams cparams;
  cparams.id = "gen";
  cparams.song = song;
  
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          element_names[_i], element_voices[_i], &err));

  GST_INFO ("-- assert --");
  ck_assert (machine != NULL);
  ck_assert (err == NULL);
  ck_assert (!bt_machine_has_patterns (machine));

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

/*
 * activate the input level meter in an unconnected machine
 */
START_TEST (test_bt_machine_enable_input_level1)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.id = "vol";
  cparams.song = song;
  
  BtMachine *machine =
      BT_MACHINE (bt_processor_machine_new (&cparams, "volume", 0, NULL));

  GST_INFO ("-- act --");
  gboolean res = bt_machine_enable_input_pre_level (machine);

  GST_INFO ("-- assert --");
  ck_assert (res == TRUE);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

/*
 * activate the input level meter in a connected machine
 */
START_TEST (test_bt_machine_enable_input_level2)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.id = "vol1";
  cparams.song = song;
  
  BtMachine *machine1 =
      BT_MACHINE (bt_processor_machine_new (&cparams, "volume", 0, NULL));

  cparams.id = "vol2";
  BtMachine *machine2 =
      BT_MACHINE (bt_processor_machine_new (&cparams, "volume", 0, NULL));
  bt_wire_new (song, machine1, machine2, NULL);

  GST_INFO ("-- act --");
  gboolean res = bt_machine_enable_input_pre_level (machine2);

  GST_INFO ("-- assert --");
  ck_assert (res == TRUE);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

/*
 * activate the input gain control in an unconnected machine
 */
START_TEST (test_bt_machine_enable_input_gain1)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.id = "vol";
  cparams.song = song;
  
  BtMachine *machine =
      BT_MACHINE (bt_processor_machine_new (&cparams, "volume", 0, NULL));

  GST_INFO ("-- act --");
  gboolean res = bt_machine_enable_input_gain (machine);

  GST_INFO ("-- assert --");
  ck_assert (res == TRUE);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

/*
 * activate the output gain control in an unconnected machine
 */
START_TEST (test_bt_machine_enable_output_gain1)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.id = "vol";
  cparams.song = song;
  
  BtMachine *machine =
      BT_MACHINE (bt_processor_machine_new (&cparams, "volume", 0, NULL));

  GST_INFO ("-- act --");
  gboolean res = bt_machine_enable_output_gain (machine);

  GST_INFO ("-- assert --");
  ck_assert (res == TRUE);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

/* add pattern */
START_TEST (test_bt_machine_add_pattern)
{
  BT_TEST_START;
  BtMachineConstructorParams cparams;
  cparams.id = "gen";
  cparams.song = song;
  
  GST_INFO ("-- arrange --");
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-poly-source", 1L, NULL));

  GST_INFO ("-- act --");
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);

  GST_INFO ("-- assert --");
  ck_assert (bt_machine_has_patterns (machine));

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_machine_rem_pattern)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.id = "gen";
  cparams.song = song;
  
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-poly-source", 1L, NULL));
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);

  GST_INFO ("-- act --");
  bt_machine_remove_pattern (machine, (BtCmdPattern *) pattern);

  GST_INFO ("-- assert --");
  ck_assert (!bt_machine_has_patterns (machine));

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_machine_unique_pattern_name)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.id = "gen";
  cparams.song = song;
  
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-poly-source", 1L, NULL));
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);

  GST_INFO ("-- act --");
  gchar *pattern_name = bt_machine_get_unique_pattern_name (machine);

  GST_INFO ("-- assert --");
  ck_assert_str_ne (pattern_name, "pattern-name");

  GST_INFO ("-- cleanup --");
  g_free (pattern_name);
  g_object_unref (pattern);

  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_machine_next_pattern_name)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.id = "gen";
  cparams.song = song;
  
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-poly-source", 1L, NULL));
  BtPattern *pattern = bt_pattern_new (song, "00", 8L, machine);

  GST_INFO ("-- act --");
  gchar *pattern_name = bt_machine_get_unique_pattern_name (machine);

  GST_INFO ("-- assert --");
  ck_assert_str_eq (pattern_name, "01");

  GST_INFO ("-- cleanup --");
  g_free (pattern_name);
  g_object_unref (pattern);

  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_machine_check_voices)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");

  GST_INFO ("-- act --");
  BtMachineConstructorParams cparams;
  cparams.id = "gen";
  cparams.song = song;
  
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-poly-source", 2L, NULL));

  GST_INFO ("-- assert --");
  GstChildProxy *element =
      (GstChildProxy *) check_gobject_get_object_property (machine, "machine");
  ck_assert_int_eq (gst_child_proxy_get_children_count (element), 2);
  //ck_assert_gobject_gulong_eq(machine,"voices",2);

  GST_INFO ("-- cleanup --");
  gst_object_unref (element);
  BT_TEST_END;
}
END_TEST

/*
 * change voices and verify that voices in machine and patetrn are in sync
 */
START_TEST (test_bt_machine_change_voices)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.id = "gen";
  cparams.song = song;
  
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-poly-source", 1L, NULL));
  BtPattern *p1 = bt_pattern_new (song, "pattern-name1", 8L, machine);
  BtPattern *p2 = bt_pattern_new (song, "pattern-name2", 8L, machine);

  GST_INFO ("-- act --");
  g_object_set (machine, "voices", 2, NULL);

  GST_INFO ("-- assert --");
  ck_assert_gobject_gulong_eq (p1, "voices", 2L);
  ck_assert_gobject_gulong_eq (p2, "voices", 2L);

  GST_INFO ("-- cleanup --");
  g_object_unref (p1);
  g_object_unref (p2);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_machine_state_mute_no_sideeffects)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.id = "gen";
  cparams.song = song;
  
  BtMachine *src =
      BT_MACHINE (bt_source_machine_new (&cparams, "audiotestsrc", 0L,
          NULL));

  cparams.id = "vol";
  BtMachine *proc =
      BT_MACHINE (bt_processor_machine_new (&cparams, "volume", 0L, NULL));
  cparams.id = "sink";
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (&cparams, NULL));
  bt_wire_new (song, src, proc, NULL);
  bt_wire_new (song, proc, sink, NULL);

  GST_INFO ("-- act --");
  g_object_set (src, "state", BT_MACHINE_STATE_MUTE, NULL);

  GST_INFO ("-- assert --");
  ck_assert_gobject_guint_eq (src, "state", BT_MACHINE_STATE_MUTE);
  ck_assert_gobject_guint_eq (proc, "state", BT_MACHINE_STATE_NORMAL);
  ck_assert_gobject_guint_eq (sink, "state", BT_MACHINE_STATE_NORMAL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_machine_state_solo_resets_others)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.id = "gen1";
  cparams.song = song;
  
  BtMachine *src1 =
      BT_MACHINE (bt_source_machine_new (&cparams, "audiotestsrc", 0L,
          NULL));

  cparams.id = "gen2";
  BtMachine *src2 =
      BT_MACHINE (bt_source_machine_new (&cparams, "audiotestsrc", 0L,
          NULL));
  cparams.id = "sink";
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (&cparams, NULL));
  bt_wire_new (song, src1, sink, NULL);
  bt_wire_new (song, src2, sink, NULL);

  GST_INFO ("-- act --");
  g_object_set (src1, "state", BT_MACHINE_STATE_SOLO, NULL);
  g_object_set (src2, "state", BT_MACHINE_STATE_SOLO, NULL);

  GST_INFO ("-- assert --");
  ck_assert_gobject_guint_eq (src1, "state", BT_MACHINE_STATE_NORMAL);
  ck_assert_gobject_guint_eq (src2, "state", BT_MACHINE_STATE_SOLO);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_machine_state_bypass_no_sideeffects)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.id = "gen";
  cparams.song = song;
  
  BtMachine *src =
      BT_MACHINE (bt_source_machine_new (&cparams, "audiotestsrc", 0L,
          NULL));

  cparams.id = "vol";
  BtMachine *proc =
      BT_MACHINE (bt_processor_machine_new (&cparams, "volume", 0L, NULL));
  cparams.id = "sink";
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (&cparams, NULL));
  bt_wire_new (song, src, proc, NULL);
  bt_wire_new (song, proc, sink, NULL);

  GST_INFO ("-- act --");
  g_object_set (proc, "state", BT_MACHINE_STATE_BYPASS, NULL);

  GST_INFO ("-- assert --");
  ck_assert_gobject_guint_eq (src, "state", BT_MACHINE_STATE_NORMAL);
  ck_assert_gobject_guint_eq (proc, "state", BT_MACHINE_STATE_BYPASS);
  ck_assert_gobject_guint_eq (sink, "state", BT_MACHINE_STATE_NORMAL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_machine_state_not_overridden)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSequence *sequence =
      (BtSequence *) check_gobject_get_object_property (song, "sequence");
  BtSongInfo *song_info =
      BT_SONG_INFO (check_gobject_get_object_property (song, "song-info"));
  
  BtMachineConstructorParams cparams;
  cparams.id = "gen";
  cparams.song = song;
  
  BtMachine *src = BT_MACHINE (bt_source_machine_new (&cparams,
          "simsyn", 0L, NULL));
  
  cparams.id = "sink";
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (&cparams, NULL));
  
  bt_wire_new (song, src, sink, NULL);
  BtCmdPattern *pattern = bt_cmd_pattern_new (song, src, BT_PATTERN_CMD_SOLO);
  GstElement *element =
      (GstElement *) check_gobject_get_object_property (src, "machine");

  /* duration: 0:00:00.480000000 */
  g_object_set (song_info, "bpm", 250L, "tpb", 16L, NULL);
  g_object_set (sequence, "length", 32L, NULL);
  bt_sequence_add_track (sequence, src, -1);
  bt_sequence_set_pattern (sequence, 4, 0, pattern);
  g_object_set (element, "wave", /* silence */ 4, NULL);

  GST_INFO ("-- act --");
  g_object_set (src, "state", BT_MACHINE_STATE_MUTE, NULL);
  bt_machine_update_default_state_value (src);
  bt_song_play (song);
  check_run_main_loop_until_eos_or_error (song);

  GST_INFO ("-- assert --");
  ck_assert_gobject_guint_eq (src, "state", BT_MACHINE_STATE_MUTE);

  GST_INFO ("-- cleanup --");
  gst_object_unref (element);
  g_object_unref (pattern);
  g_object_unref (sequence);
  g_object_unref (song_info);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_machine_pretty_name)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.id = "audiotestsrc";
  cparams.song = song;
  
  BtMachine *src =
      BT_MACHINE (bt_source_machine_new (&cparams, "audiotestsrc",
          0L, NULL));

  GST_INFO ("-- act --");

  GST_INFO ("-- assert --");
  ck_assert_gobject_str_eq (src, "pretty-name", "audiotestsrc");

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_machine_pretty_name_with_detail)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.id = "gen";
  cparams.song = song;
  
  BtMachine *src =
      BT_MACHINE (bt_source_machine_new (&cparams, "audiotestsrc", 0L,
          NULL));

  GST_INFO ("-- act --");

  GST_INFO ("-- assert --");
  ck_assert_gobject_str_eq (src, "pretty-name", "gen (audiotestsrc)");

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_machine_set_defaults)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.id = "id";
  cparams.song = song;
  
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0, NULL));
  GstObject *element =
      (GstObject *) check_gobject_get_object_property (machine, "machine");
  GstControlBinding *cb = gst_object_get_control_binding (element, "g-uint");
  g_object_set (element, "g-uint", 10, NULL);

  GST_INFO ("-- act --");
  bt_machine_set_param_defaults (machine);

  GST_INFO ("-- assert --");
  GValue *val = gst_control_binding_get_value (cb, G_GUINT64_CONSTANT (0));
  guint uval = g_value_get_uint (val);
  ck_assert_int_eq (uval, 10);

  GST_INFO ("-- cleanup --");
  gst_object_unref (element);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_machine_bind_parameter_control)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.id = "id";
  cparams.song = song;
  
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0, NULL));
  GstObject *element =
      (GstObject *) check_gobject_get_object_property (machine, "machine");
  BtParameterGroup *pg = bt_machine_get_global_param_group (machine);
  BtIcControl *control = btic_device_get_control_by_name (device, "abs1");
  g_object_set (element, "g-uint", 10, NULL);

  bt_machine_bind_parameter_control (machine, element, "g-uint", control, pg);

  GST_INFO ("-- act --");
  g_object_set (control, "value", 0, NULL);

  GST_INFO ("-- assert --");
  ck_assert_gobject_guint_eq (element, "g-uint", 0);

  GST_INFO ("-- cleanup --");
  gst_object_unref (element);
  g_object_unref (control);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_machine_unbind_parameter_control)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.id = "id";
  cparams.song = song;
  
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0, NULL));
  GstObject *element =
      (GstObject *) check_gobject_get_object_property (machine, "machine");
  BtParameterGroup *pg = bt_machine_get_global_param_group (machine);
  BtIcControl *control = btic_device_get_control_by_name (device, "abs1");
  g_object_set (element, "g-uint", 10, NULL);
  bt_machine_bind_parameter_control (machine, element, "g-uint", control, pg);
  g_object_set (control, "value", 0, NULL);

  GST_INFO ("-- act --");
  bt_machine_unbind_parameter_control (machine, element, "g-uint");
  g_object_set (control, "value", 100, NULL);

  GST_INFO ("-- assert --");
  ck_assert_gobject_guint_eq (element, "g-uint", 0);

  GST_INFO ("-- cleanup --");
  gst_object_unref (element);
  g_object_unref (control);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_machine_unbind_parameter_controls)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.id = "id";
  cparams.song = song;
  
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0, NULL));
  GstObject *element =
      (GstObject *) check_gobject_get_object_property (machine, "machine");
  BtParameterGroup *pg = bt_machine_get_global_param_group (machine);
  BtIcControl *control = btic_device_get_control_by_name (device, "abs1");
  g_object_set (element, "g-uint", 10, NULL);
  bt_machine_bind_parameter_control (machine, element, "g-uint", control, pg);
  g_object_set (control, "value", 0, NULL);

  GST_INFO ("-- act --");
  bt_machine_unbind_parameter_controls (machine);
  g_object_set (control, "value", 100, NULL);

  GST_INFO ("-- assert --");
  ck_assert_gobject_guint_eq (element, "g-uint", 0);

  GST_INFO ("-- cleanup --");
  gst_object_unref (element);
  g_object_unref (control);
  BT_TEST_END;
}
END_TEST


TCase *
bt_machine_example_case (void)
{
  TCase *tc = tcase_create ("BtMachineExamples");

  tcase_add_loop_test (tc, test_bt_machine_create, 0,
      G_N_ELEMENTS (element_names));
  tcase_add_test (tc, test_bt_machine_enable_input_level1);
  tcase_add_test (tc, test_bt_machine_enable_input_level2);
  tcase_add_test (tc, test_bt_machine_enable_input_gain1);
  tcase_add_test (tc, test_bt_machine_enable_output_gain1);
  tcase_add_test (tc, test_bt_machine_add_pattern);
  tcase_add_test (tc, test_bt_machine_rem_pattern);
  tcase_add_test (tc, test_bt_machine_unique_pattern_name);
  tcase_add_test (tc, test_bt_machine_next_pattern_name);
  tcase_add_test (tc, test_bt_machine_check_voices);
  tcase_add_test (tc, test_bt_machine_change_voices);
  tcase_add_test (tc, test_bt_machine_state_mute_no_sideeffects);
  tcase_add_test (tc, test_bt_machine_state_solo_resets_others);
  tcase_add_test (tc, test_bt_machine_state_bypass_no_sideeffects);
  tcase_add_test (tc, test_bt_machine_state_not_overridden);
  tcase_add_test (tc, test_bt_machine_pretty_name);
  tcase_add_test (tc, test_bt_machine_pretty_name_with_detail);
  tcase_add_test (tc, test_bt_machine_set_defaults);
  tcase_add_test (tc, test_bt_machine_bind_parameter_control);
  tcase_add_test (tc, test_bt_machine_unbind_parameter_control);
  tcase_add_test (tc, test_bt_machine_unbind_parameter_controls);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
