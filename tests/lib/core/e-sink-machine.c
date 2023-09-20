/* Buzztrax
 * Copyright (C) 2009 Buzztrax team <buzztrax-devel@buzztrax.org>
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
#include <gst/base/gstbasesink.h>
#include <gst/audio/gstaudiobasesink.h>
#include "gst/tempo.h"

//-- globals

static BtApplication *app;
static BtSong *song;
static BtSettings *settings;

//-- fixtures

static void
case_setup (void)
{
  BT_CASE_START;
}

static void
test_setup (void)
{
  app = bt_test_application_new ();
  song = bt_song_new (app);
  settings = bt_settings_make ();
}

static void
test_teardown (void)
{
  g_object_unref (settings);
  ck_g_object_final_unref (song);
  ck_g_object_final_unref (app);
}

static void
case_teardown (void)
{
}

//-- helper

static GstElement *
get_sink_element (GstBin * bin)
{
  GstElement *e;
  GList *list = GST_BIN_CHILDREN (bin);
  GList *node;

  GST_INFO_OBJECT (bin, "looking for audio_sink in %d children",
      g_list_length (list));
  for (node = list; node; node = g_list_next (node)) {
    e = (GstElement *) node->data;
    GST_INFO_OBJECT (bin, "trying '%s'", GST_OBJECT_NAME (e));
    if (GST_IS_BIN (e) && GST_OBJECT_FLAG_IS_SET (e, GST_ELEMENT_FLAG_SINK)) {
      return get_sink_element ((GstBin *) e);
    }
    // while audiosink should subclass GstAudioBaseSink, they don't have to ...
    // we're relying on the bin implementation to ensure the sink takes audio
    if (GST_IS_BASE_SINK (e)) {
      return e;
    }
  }
  return NULL;
}


//-- tests

START_TEST (test_bt_sink_machine_new)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  g_object_set (settings, "audiosink", "fakesink", NULL);

  GST_INFO ("-- act --");
  GError *err = NULL;
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "master";
  
  BtSinkMachine *machine = bt_sink_machine_new (&cparams, &err);

  GST_INFO ("-- assert --");
  ck_assert (machine != NULL);
  ck_assert (err == NULL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_sink_machine_def_patterns)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  g_object_set (settings, "audiosink", "fakesink", NULL);
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "master";
  
  BtSinkMachine *machine = bt_sink_machine_new (&cparams, NULL);

  GST_INFO ("-- act --");
  GList *list = (GList *) check_gobject_get_ptr_property (machine, "patterns");

  GST_INFO ("-- assert --");
  ck_assert (list != NULL);
  ck_assert_int_eq (g_list_length (list), 2);   /* break+mute */

  GST_INFO ("-- cleanup --");
  g_list_foreach (list, (GFunc) g_object_unref, NULL);
  g_list_free (list);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_sink_machine_pattern)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  g_object_set (settings, "audiosink", "fakesink", NULL);
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "master";
  
  BtSinkMachine *machine = bt_sink_machine_new (&cparams, NULL);

  GST_INFO ("-- act --");
  BtCmdPattern *pattern =
      (BtCmdPattern *) bt_pattern_new (song, "pattern-name", 8L,
      BT_MACHINE (machine));

  GST_INFO ("-- assert --");
  ck_assert (pattern != NULL);
  ck_assert_gobject_gulong_eq (pattern, "voices", 0L);

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_sink_machine_pattern_by_id)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  g_object_set (settings, "audiosink", "fakesink", NULL);
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "master";
  
  BtSinkMachine *machine = bt_sink_machine_new (&cparams, NULL);

  GST_INFO ("-- act --");
  BtCmdPattern *pattern =
      (BtCmdPattern *) bt_pattern_new (song, "pattern-name", 8L,
      BT_MACHINE (machine));

  GST_INFO ("-- assert --");
  ck_assert_gobject_eq_and_unref (bt_machine_get_pattern_by_name (BT_MACHINE
          (machine), "pattern-name"), pattern);

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_sink_machine_pattern_by_index)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "master";
  
  BtSinkMachine *machine = bt_sink_machine_new (&cparams, NULL);

  GST_INFO ("-- act --");
  BtCmdPattern *pattern = bt_machine_get_pattern_by_index (BT_MACHINE (machine),
      BT_SINK_MACHINE_PATTERN_INDEX_MUTE);

  GST_INFO ("-- assert --");
  ck_assert (pattern != NULL);

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_sink_machine_pattern_by_list)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  g_object_set (settings, "audiosink", "fakesink", NULL);
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "master";
  
  BtSinkMachine *machine = bt_sink_machine_new (&cparams, NULL);
  BtCmdPattern *pattern =
      (BtCmdPattern *) bt_pattern_new (song, "pattern-name", 8L,
      BT_MACHINE (machine));
  GList *list = (GList *) check_gobject_get_ptr_property (machine, "patterns");

  GST_INFO ("-- act --");
  GList *node = g_list_last (list);

  GST_INFO ("-- assert --");
  ck_assert (node->data == pattern);

  GST_INFO ("-- cleanup --");
  g_list_foreach (list, (GFunc) g_object_unref, NULL);
  g_list_free (list);
  g_object_unref (pattern);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_sink_machine_default)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  g_object_set (settings, "audiosink", NULL, NULL);

  GST_INFO ("-- act --");
  GError *err = NULL;
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "master";
  
  BtSinkMachine *machine = bt_sink_machine_new (&cparams, &err);

  GST_INFO ("-- assert --");
  ck_assert (machine != NULL);
  ck_assert (err == NULL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_sink_machine_fallback)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  /* we have no test settings anymore, but we could write to the system
   * settings directy, as we're using the memory backend, we need the GSettings
   * instance though (which is private to BtSettings
   gchar *settings_str = NULL;
   bt_test_settings_set (BT_TEST_SETTINGS (settings), "system-audiosink",
   &settings_str);
   */
  g_object_set (settings, "audiosink", NULL, NULL);

  GST_INFO ("-- act --");
  GError *err = NULL;
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "master";
  
  BtSinkMachine *machine = bt_sink_machine_new (&cparams, &err);

  GST_INFO ("-- assert --");
  ck_assert (machine != NULL);
  ck_assert (err == NULL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_sink_machine_actual_sink)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "master";
  
  BtSinkMachine *machine = bt_sink_machine_new (&cparams, NULL);

  GST_INFO ("-- act --");
  GstElement *sink_bin =
      GST_ELEMENT (check_gobject_get_object_property (machine, "machine"));
  gst_element_set_state (sink_bin, GST_STATE_READY);
  GstElement *sink = get_sink_element ((GstBin *) sink_bin);

  GST_INFO ("-- assert --");
  ck_assert (sink != NULL);

  GST_INFO ("-- cleanup --");
  gst_object_unref (sink_bin);
  BT_TEST_END;
}
END_TEST

/* the parameter index _i is 2bits for latency, 2bits for bpm, 2 bits for tpb */
START_TEST (test_bt_sink_machine_latency)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSongInfo *song_info =
      BT_SONG_INFO (check_gobject_get_object_property (song, "song-info"));
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  BtMachine *gen = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0L, NULL));
  BtTestMonoSource *src =
      (BtTestMonoSource *) check_gobject_get_object_property (gen, "machine");
  cparams.id = "master";
  
  BtMachine *master = BT_MACHINE (bt_sink_machine_new (&cparams, NULL));
  GstElement *sink_bin =
      GST_ELEMENT (check_gobject_get_object_property (master, "machine"));
  bt_wire_new (song, gen, master, NULL);
  gst_element_set_state (sink_bin, GST_STATE_READY);
  GstElement *sink = get_sink_element ((GstBin *) sink_bin);
  if (!sink || !GST_IS_AUDIO_BASE_SINK (sink)) {
    GST_WARNING ("no sink or not AudioBaseSink");
    goto Cleanup;
  }
  gulong latency = 20 + 20 * (_i & 0x3);
  gulong bpm = 80 + 20 * ((_i >> 2) & 0x3);
  gulong tpb = 4 + 2 * ((_i >> 4) & 0x3);

  GST_INFO ("-- act --");
  // set various bpm, tpb on song_info, set various latency on settings
  // assert the resulting latency-time properties on the audio_sink
  g_object_set (settings, "latency", latency, NULL);
  g_object_set (song_info, "bpm", bpm, "tpb", tpb, NULL);
  gulong st = src->stpb, c_bpm = src->bpm, c_tpb = src->tpb;

  GST_INFO ("-- assert --");
  gint64 latency_time, c_latency_time;
  g_object_get (sink, "latency-time", &c_latency_time, NULL);
  latency_time = GST_TIME_AS_USECONDS ((GST_SECOND * 60) / (bpm * tpb * st));

  GST_INFO_OBJECT (sink,
      "bpm=%3lu=%3lu, tpb=%lu=%lu, stpb=%2lu, target-latency=%2lu , latency-time=%6"
      G_GINT64_FORMAT "=%6" G_GINT64_FORMAT ", delta=%+4" G_GINT64_FORMAT, bpm,
      c_bpm, tpb, c_tpb, st, latency, latency_time, c_latency_time,
      (latency_time - (latency * 1000)) / 1000);

  ck_assert_ulong_eq (c_bpm, bpm);
  ck_assert_ulong_eq (c_tpb, tpb);
  ck_assert_int64_eq (c_latency_time, latency_time);

Cleanup:
  GST_INFO ("-- cleanup --");
  gst_object_unref (sink_bin);
  g_object_unref (song_info);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_sink_machine_pretty_name)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "master";
  
  BtSinkMachine *machine = bt_sink_machine_new (&cparams, NULL);

  GST_INFO ("-- act --");

  GST_INFO ("-- assert --");
  ck_assert_gobject_str_eq (machine, "pretty-name", "master");

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST


TCase *
bt_sink_machine_example_case (void)
{
  TCase *tc = tcase_create ("BtSinkMachineExamples");

  tcase_add_test (tc, test_bt_sink_machine_new);
  tcase_add_test (tc, test_bt_sink_machine_def_patterns);
  tcase_add_test (tc, test_bt_sink_machine_pattern);
  tcase_add_test (tc, test_bt_sink_machine_pattern_by_id);
  tcase_add_test (tc, test_bt_sink_machine_pattern_by_index);
  tcase_add_test (tc, test_bt_sink_machine_pattern_by_list);
  tcase_add_test (tc, test_bt_sink_machine_default);
  tcase_add_test (tc, test_bt_sink_machine_fallback);
  tcase_add_test (tc, test_bt_sink_machine_actual_sink);
  tcase_add_loop_test (tc, test_bt_sink_machine_latency, 0, 64);
  tcase_add_test (tc, test_bt_sink_machine_pretty_name);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
