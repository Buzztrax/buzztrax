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
#include "core/song-io-native.h"

//-- globals

static gint num_formats = 0;

static BtApplication *app;
static BtSong *song;
static BtSettings *settings;
static BtIcRegistry *registry;
static BtIcDevice *device;

static gchar *input_song_names[] = {
  "test-simple0.xml",
  "test-simple1.xml",
  "test-simple2.xml",
  "test-simple3.xml",
  "test-simple4.xml",
  "test-simple5.xml"
};

enum
{
  EMPTY_SONG = 0,
  SONG_NORMAL,
  SONG_WITH_EXTERNALS,
  SONG_WITH_IC_BINDINGS,
  NUM_SONG_TYPES
};

static gchar *output_song_names[] = {
  "bt-test-empty.",
  "bt-test-normal.",
  "bt-test-with-externals.",
  "bt-test-with-ic-bindings.",
};

//-- fixtures

static void
case_setup (void)
{
  BT_CASE_START;
  btic_init (NULL, NULL);

  registry = btic_registry_new ();
  device = (BtIcDevice *) btic_test_device_new ("test");
  btic_registry_add_device (device);
}

static void
test_setup (void)
{
  app = bt_test_application_new ();
  settings = bt_settings_make ();
  g_object_set (settings, "audiosink", "fakesink", NULL);
  song = bt_song_new (app);
}

static void
test_teardown (void)
{
  ck_g_object_final_unref (song);
  g_object_unref (settings);
  ck_g_object_final_unref (app);
}

static void
case_teardown (void)
{
  g_object_unref (registry);
}

//-- helper

static gint
get_machine_refcount (BtSetup * setup, const gchar * id)
{
  BtMachine *machine = bt_setup_get_machine_by_id (setup, id);
  gint ref_ct = 0;

  if (machine) {
    ref_ct = G_OBJECT_REF_COUNT (machine) - 1;
    g_object_unref (machine);
  }
  return ref_ct;
}

static gint
get_wire_refcount (BtSetup * setup, const gchar * sid, const gchar * did)
{
  BtMachine *src = bt_setup_get_machine_by_id (setup, sid);
  BtMachine *dst = bt_setup_get_machine_by_id (setup, did);
  gint ref_ct = 0;

  if (src && dst) {
    BtWire *wire = bt_setup_get_wire_by_machines (setup, src, dst);

    if (wire) {
      ref_ct = G_OBJECT_REF_COUNT (wire) - 1;
      g_object_unref (wire);
    }
  }
  if (src)
    g_object_unref (src);
  if (dst)
    g_object_unref (dst);
  return ref_ct;
}

static void
assert_song_part_refcounts (BtSong * song)
{
  BtSetup *setup;
  BtSequence *sequence;
  BtSongInfo *songinfo;
  BtWavetable *wavetable;

  g_object_get (song, "setup", &setup, "sequence", &sequence, "song-info",
      &songinfo, "wavetable", &wavetable, NULL);

  ck_assert_int_eq (G_OBJECT_REF_COUNT (setup), 2);
  ck_assert_int_eq (G_OBJECT_REF_COUNT (sequence), 2);
  ck_assert_int_eq (G_OBJECT_REF_COUNT (songinfo), 2);
  ck_assert_int_eq (G_OBJECT_REF_COUNT (wavetable), 2);

  g_object_unref (setup);
  g_object_unref (sequence);
  g_object_unref (songinfo);
  g_object_unref (wavetable);
}

static gchar *
make_tmp_song_path (const gchar * name, const gchar * ext)
{
  gchar *song_name = g_strconcat (name, ext, NULL);
  gchar *song_path = g_build_filename (g_get_tmp_dir (), song_name, NULL);
  g_free (song_name);
  GST_INFO ("testing '%s'", song_path);
  return song_path;
}

static void
make_song_normal (void)
{
  BtSequence *sequence =
      BT_SEQUENCE (check_gobject_get_object_property (song, "sequence"));
  g_object_set (sequence, "length", 8L, NULL);

  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "master";
  
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (&cparams, NULL));

  cparams.id = "gen-m";
  BtMachine *gen1 = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0L, NULL));
  BtWire *wire = bt_wire_new (song, gen1, sink, NULL);
  BtPattern *pat_gen1 = bt_pattern_new (song, "melo", 8L, gen1);
  bt_pattern_set_global_event (pat_gen1, 0, 0, "5");
  bt_sequence_add_track (sequence, gen1, -1);
  bt_sequence_set_pattern (sequence, 0, 0, (BtCmdPattern *) pat_gen1);

  bt_pattern_set_wire_event (pat_gen1, 0, wire, 0, "100");

  cparams.id = "gen-p";
  BtMachine *gen2 = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-poly-source", 1L, NULL));
  bt_wire_new (song, gen2, sink, NULL);
  BtPattern *pat_gen2 = bt_pattern_new (song, "melo", 8L, gen2);
  bt_pattern_set_global_event (pat_gen2, 0, 0, "5");
  bt_pattern_set_voice_event (pat_gen2, 0, 0, 0, "5");
  bt_sequence_add_track (sequence, gen2, -1);
  bt_sequence_set_pattern (sequence, 0, 1, (BtCmdPattern *) pat_gen2);

  g_object_unref (pat_gen1);
  g_object_unref (sequence);
  GST_INFO ("  song created");
}

static gchar *
make_ext_data_uri (const gchar * file_name)
{
  gchar *ext_data_path = g_build_filename (g_get_tmp_dir (), file_name, NULL);
  gchar *ext_data_uri = g_strconcat ("file://", ext_data_path, NULL);
  g_free (ext_data_path);

  return ext_data_uri;
}

static void
make_song_with_externals (void)
{
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "master";
  
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (&cparams, NULL));

  cparams.id = "gen";
  BtMachine *gen = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0L, NULL));
  bt_wire_new (song, gen, sink, NULL);
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, gen);
  gchar *ext_data_uri = make_ext_data_uri ("test.wav");
  BtWave *wave =
      bt_wave_new (song, "sample1", ext_data_uri, 1, 1.0, BT_WAVE_LOOP_MODE_OFF,
      0);

  g_object_unref (wave);
  g_object_unref (pattern);
  g_free (ext_data_uri);
  GST_INFO ("  song created");
}

static void
make_song_with_ic_binding (void)
{
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "master";
  
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (&cparams, NULL));

  cparams.id = "gen";
  BtMachine *gen = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0L, NULL));
  bt_wire_new (song, gen, sink, NULL);

  BtParameterGroup *pg = bt_machine_get_global_param_group (gen);
  BtIcControl *control = btic_device_get_control_by_name (device, "abs1");
  GstObject *element =
      (GstObject *) check_gobject_get_object_property (gen, "machine");
  // FIXME: there seems to be insufficient ref-counting
  // it we remove a device, we end up with control-bindings without parents
  bt_machine_bind_parameter_control (gen, element, "g-uint", control, pg);

  gst_object_unref (element);
  g_object_unref (control);
  GST_INFO ("  song created");
}


//-- tests

START_TEST (test_bt_song_io_native_new)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");

  GST_INFO ("-- act --");
  BtSongIO *song_io =
      bt_song_io_from_file (check_get_test_song_path ("simple2.xml"), NULL);

  GST_INFO ("-- assert --");
  ck_assert (song_io != NULL);

  GST_INFO ("-- cleanup --");
  ck_g_object_final_unref (song_io);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_song_io_native_formats)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSongIOFormatInfo *fi = &bt_song_io_native_module_info.formats[_i];
  gchar *song_path = make_tmp_song_path ("bt-test0-song.", fi->extension);

  GST_INFO ("-- act --");
  BtSongIO *song_io = bt_song_io_from_file (song_path, NULL);

  GST_INFO ("-- assert --");
  ck_assert (song_io != NULL);

  GST_INFO ("-- cleanup --");
  ck_g_object_final_unref (song_io);
  g_free (song_path);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_song_io_native_load)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSongIO *song_io =
      bt_song_io_from_file (check_get_test_song_path ("simple2.xml"), NULL);

  GST_INFO ("-- act --");

  GST_INFO ("-- assert --");
  ck_assert (bt_song_io_load (song_io, song, NULL) == TRUE);

  GST_INFO ("-- cleanup --");
  ck_g_object_final_unref (song_io);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_song_io_native_core_refcounts)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSongIO *song_io =
      bt_song_io_from_file (check_get_test_song_path ("simple2.xml"), NULL);

  GST_INFO ("-- act --");
  bt_song_io_load (song_io, song, NULL);

  GST_INFO ("-- assert --");
  ck_assert_int_eq (G_OBJECT_REF_COUNT (song), 1);
  assert_song_part_refcounts (song);

  GST_INFO ("-- cleanup --");
  ck_g_object_final_unref (song_io);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_song_io_native_setup_refcounts_0)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSetup *setup;
  BtSongIO *song_io =
      bt_song_io_from_file (check_get_test_song_path ("test-simple0.xml"),
      NULL);
  g_object_get (song, "setup", &setup, NULL);

  GST_INFO ("-- act --");
  bt_song_io_load (song_io, song, NULL);

  GST_INFO ("-- assert --");
  // 1 x setup
  ck_assert_int_eq (get_machine_refcount (setup, "audio_sink"), 1);

  GST_INFO ("-- cleanup --");
  g_object_unref (setup);
  ck_g_object_final_unref (song_io);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_song_io_native_setup_refcounts_1)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSetup *setup;
  BtSongIO *song_io =
      bt_song_io_from_file (check_get_test_song_path ("test-simple1.xml"),
      NULL);
  g_object_get (song, "setup", &setup, NULL);

  GST_INFO ("-- act --");
  bt_song_io_load (song_io, song, NULL);

  GST_INFO ("-- assert --");
  // 1 x pipeline, 1 x setup, 1 x wire
  ck_assert_int_eq (get_machine_refcount (setup, "audio_sink"), 3);
  // 1 x pipeline, 1 x setup, 1 x wire, 1 x sequence
  ck_assert_int_eq (get_machine_refcount (setup, "sine1"), 4);
  // 1 x pipeline, 1 x setup
  ck_assert_int_eq (get_wire_refcount (setup, "sine1", "audio_sink"), 2);

  /* TODO(ensonic): debug by looking at
   * bt_song_write_to_highlevel_dot_file (song);
   * - or -
   * BT_CHECKS="test_bt_song_io_native_setup_refcounts_1" make bt_core.check
   * grep "ref_ct" /tmp/bt_core.log
   */

  GST_INFO ("-- cleanup --");
  g_object_unref (setup);
  ck_g_object_final_unref (song_io);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_song_io_native_setup_refcounts_2)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSetup *setup;
  BtSongIO *song_io =
      bt_song_io_from_file (check_get_test_song_path ("test-simple2.xml"),
      NULL);
  g_object_get (song, "setup", &setup, NULL);

  GST_INFO ("-- act --");
  bt_song_io_load (song_io, song, NULL);

  GST_INFO ("-- assert --");
  // 1 x pipeline, 1 x setup, 1 x wire
  ck_assert_int_eq (get_machine_refcount (setup, "audio_sink"), 3);
  // 1 x pipeline, 1 x setup, 2 x wire
  ck_assert_int_eq (get_machine_refcount (setup, "amp1"), 4);
  // 1 x pipeline, 1 x setup, 1 x wire, 1 x sequence
  ck_assert_int_eq (get_machine_refcount (setup, "sine1"), 4);
  // 1 x pipeline, 1 x setup
  ck_assert_int_eq (get_wire_refcount (setup, "sine1", "amp1"), 2);
  // 1 x pipeline, 1 x setup
  ck_assert_int_eq (get_wire_refcount (setup, "amp1", "audio_sink"), 2);

  /* TODO(ensonic): debug by looking at
   * bt_song_write_to_highlevel_dot_file (song);
   * - or -
   * BT_CHECKS="test_bt_song_io_native_setup_refcounts_1" make bt_core.check
   * grep "ref_ct" /tmp/bt_core.log
   */

  GST_INFO ("-- cleanup --");
  g_object_unref (setup);
  ck_g_object_final_unref (song_io);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_song_io_native_song_refcounts)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  GstElement *bin =
      (GstElement *) check_gobject_get_object_property (app, "bin");

  GST_INFO ("-- act --");
  const gchar *song_name = input_song_names[_i];
  BtSongIO *song_io =
      bt_song_io_from_file (check_get_test_song_path (song_name), NULL);
  GST_INFO ("song[%d:%s].elements=%d", _i, song_name,
      GST_BIN_NUMCHILDREN (bin));

  GST_INFO ("-- assert --");
  ck_assert_int_eq (G_OBJECT_REF_COUNT (song), 1);
  assert_song_part_refcounts (song);

  ck_g_object_final_unref (song_io);
  ck_g_object_final_unref (song);

  ck_assert_int_eq (GST_BIN_NUMCHILDREN (bin), 0);

  GST_INFO ("-- cleanup --");
  song = bt_song_new (app);
  gst_object_unref (bin);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_song_io_write_song)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  guint song_type = _i / num_formats;
  guint fmt = _i % num_formats;
  BtSongIOFormatInfo *fi = &bt_song_io_native_module_info.formats[fmt];
  gchar *song_path = make_tmp_song_path (output_song_names[song_type],
      fi->extension);

  switch (song_type) {
    case EMPTY_SONG:
      break;
    case SONG_NORMAL:
      make_song_normal ();
      break;
    case SONG_WITH_EXTERNALS:
      make_song_with_externals ();
      break;
    case SONG_WITH_IC_BINDINGS:
      make_song_with_ic_binding ();
      break;
    default:
      break;
  }
  GST_INFO ("testing [type=%d/fmt=%d] %s", song_type, fmt, song_path);

  /* save song */
  GST_INFO ("-- act --");
  BtSongIO *song_io = bt_song_io_from_file (song_path, NULL);
  gboolean res = bt_song_io_save (song_io, song, NULL);
  ck_assert (res == TRUE);

  ck_g_object_final_unref (song_io);
  ck_g_object_final_unref (song);
  song = bt_song_new (app);

  /* load the song */
  GST_INFO ("-- assert --");
  song_io = bt_song_io_from_file (song_path, NULL);
  res = bt_song_io_load (song_io, song, NULL);
  ck_assert (res == TRUE);

  GST_INFO ("-- cleanup --");
  ck_g_object_final_unref (song_io);
  g_free (song_path);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_song_io_native_load_legacy_0_7)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSetup *setup;
  BtSequence *sequence;
  BtSongIO *song_io =
      bt_song_io_from_file (check_get_test_song_path ("legacy1.xml"), NULL);
  g_object_get (song, "sequence", &sequence, "setup", &setup, NULL);

  GST_INFO ("-- act --");
  bt_song_io_load (song_io, song, NULL);

  GST_INFO ("-- assert --");
  BtMachine *machine = bt_setup_get_machine_by_id (setup, "beep");
  BtCmdPattern *pattern1 = bt_machine_get_pattern_by_name (machine, "beeps");
  BtCmdPattern *pattern2 = bt_sequence_get_pattern (sequence, 0, 0);
  ck_assert (pattern1 == pattern2);

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern1);
  g_object_unref (pattern2);
  g_object_unref (machine);
  g_object_unref (setup);
  g_object_unref (sequence);
  ck_g_object_final_unref (song_io);
  BT_TEST_END;
}
END_TEST

TCase *
bt_song_io_native_example_case (void)
{
  TCase *tc = tcase_create ("BtSongIONativeExamples");

  BtSongIOFormatInfo *fi = bt_song_io_native_module_info.formats;
  while (fi->name) {
    num_formats++;
    fi++;
  }
  GST_INFO ("testing %d formats", num_formats);

  tcase_add_test (tc, test_bt_song_io_native_new);
  tcase_add_loop_test (tc, test_bt_song_io_native_formats, 0, num_formats);
  tcase_add_test (tc, test_bt_song_io_native_load);
  tcase_add_test (tc, test_bt_song_io_native_core_refcounts);
  tcase_add_test (tc, test_bt_song_io_native_setup_refcounts_0);
  tcase_add_test (tc, test_bt_song_io_native_setup_refcounts_1);
  tcase_add_test (tc, test_bt_song_io_native_setup_refcounts_2);
  tcase_add_loop_test (tc, test_bt_song_io_native_song_refcounts, 0,
      G_N_ELEMENTS (input_song_names));
  tcase_add_loop_test (tc, test_bt_song_io_write_song, 0,
      num_formats * NUM_SONG_TYPES);
  tcase_add_test (tc, test_bt_song_io_native_load_legacy_0_7);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
