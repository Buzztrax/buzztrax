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

static BtApplication *app;
static BtSong *song;
static BtSettings *settings;

static gchar *song_names[] = {
  "test-simple0.xml",
  "test-simple1.xml",
  "test-simple2.xml",
  "test-simple3.xml",
  "test-simple4.xml",
  "test-simple5.xml"
};


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
make_song_without_externals (void)
{
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (song, "master", NULL));
  BtMachine *gen = BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztrax-test-mono-source", 0L, NULL));
  bt_wire_new (song, gen, sink, NULL);
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, gen);
  g_object_unref (pattern);
  GST_INFO ("  song created");
}

static void
make_song_with_externals (const gchar * ext_data_uri)
{
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (song, "master", NULL));
  BtMachine *gen = BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztrax-test-mono-source", 0L, NULL));
  bt_wire_new (song, gen, sink, NULL);
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, gen);
  BtWave *wave =
      bt_wave_new (song, "sample1", ext_data_uri, 1, 1.0, BT_WAVE_LOOP_MODE_OFF,
      0);
  g_object_unref (wave);
  g_object_unref (pattern);
  GST_INFO ("  song created");
}


//-- tests

static void
test_bt_song_io_native_new (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");

  GST_INFO ("-- act --");
  BtSongIO *song_io =
      bt_song_io_from_file (check_get_test_song_path ("simple2.xml"), NULL);

  GST_INFO ("-- assert --");
  fail_unless (song_io != NULL, NULL);

  GST_INFO ("-- cleanup --");
  ck_g_object_final_unref (song_io);
  BT_TEST_END;
}

static void
test_bt_song_io_native_formats (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSongIOFormatInfo *fi = &bt_song_io_native_module_info.formats[_i];
  gchar *song_path = make_tmp_song_path ("bt-test0-song.", fi->extension);

  GST_INFO ("-- act --");
  BtSongIO *song_io = bt_song_io_from_file (song_path, NULL);

  GST_INFO ("-- assert --");
  fail_unless (song_io != NULL, NULL);

  GST_INFO ("-- cleanup --");
  ck_g_object_final_unref (song_io);
  g_free (song_path);
  BT_TEST_END;
}

static void
test_bt_song_io_native_load (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSongIO *song_io =
      bt_song_io_from_file (check_get_test_song_path ("simple2.xml"), NULL);

  GST_INFO ("-- act --");

  GST_INFO ("-- assert --");
  fail_unless (bt_song_io_load (song_io, song, NULL) == TRUE, NULL);

  GST_INFO ("-- cleanup --");
  ck_g_object_final_unref (song_io);
  BT_TEST_END;
}

static void
test_bt_song_io_native_core_refcounts (BT_TEST_ARGS)
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

static void
test_bt_song_io_native_setup_refcounts_0 (BT_TEST_ARGS)
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

static void
test_bt_song_io_native_setup_refcounts_1 (BT_TEST_ARGS)
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

static void
test_bt_song_io_native_setup_refcounts_2 (BT_TEST_ARGS)
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

static void
test_bt_song_io_native_song_refcounts (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  GstElement *bin =
      (GstElement *) check_gobject_get_object_property (app, "bin");

  GST_INFO ("-- act --");
  const gchar *song_name = song_names[_i];
  BtSongIO *song_io =
      bt_song_io_from_file (check_get_test_song_path (song_name), NULL);
  GST_INFO ("song[%d:%s].elements=%d", _i, song_name,
      GST_BIN_NUMCHILDREN (bin));

  /* assert  */
  ck_assert_int_eq (G_OBJECT_REF_COUNT (song), 1);
  assert_song_part_refcounts (song);

  ck_g_object_final_unref (song_io);
  ck_g_object_final_unref (song);
  song = bt_song_new (app);

  ck_assert_int_eq (GST_BIN_NUMCHILDREN (bin), 0);

  GST_INFO ("-- cleanup --");
  gst_object_unref (bin);
  BT_TEST_END;
}

static void
test_bt_song_io_write_empty_song (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSongIOFormatInfo *fi = &bt_song_io_native_module_info.formats[_i];
  gchar *song_path = make_tmp_song_path ("bt-test1-song.", fi->extension);

  /* save empty song */
  BtSongIO *song_io = bt_song_io_from_file (song_path, NULL);
  gboolean res = bt_song_io_save (song_io, song, NULL);
  fail_unless (res == TRUE, NULL);

  ck_g_object_final_unref (song_io);
  ck_g_object_final_unref (song);
  song = bt_song_new (app);

  /* load the song */
  song_io = bt_song_io_from_file (song_path, NULL);
  res = bt_song_io_load (song_io, song, NULL);
  fail_unless (res == TRUE, NULL);

  ck_g_object_final_unref (song_io);
  ck_g_object_final_unref (song);
  song = bt_song_new (app);

  GST_INFO ("-- cleanup --");
  g_free (song_path);
  BT_TEST_END;
}

static void
test_bt_song_io_write_song_without_externals (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSongIOFormatInfo *fi = &bt_song_io_native_module_info.formats[_i];
  gchar *song_path = make_tmp_song_path ("bt-test2-song.", fi->extension);

  make_song_without_externals ();

  /* save the song */
  BtSongIO *song_io = bt_song_io_from_file (song_path, NULL);
  gboolean res = bt_song_io_save (song_io, song, NULL);
  fail_unless (res == TRUE, NULL);

  ck_g_object_final_unref (song_io);
  ck_g_object_final_unref (song);
  song = bt_song_new (app);

  /* load the song */
  song_io = bt_song_io_from_file (song_path, NULL);
  res = bt_song_io_load (song_io, song, NULL);
  fail_unless (res == TRUE, NULL);

  ck_g_object_final_unref (song_io);
  ck_g_object_final_unref (song);
  song = bt_song_new (app);

  g_free (song_path);
  BT_TEST_END;
}

static void
test_bt_song_io_write_song_with_externals (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSongIOFormatInfo *fi = &bt_song_io_native_module_info.formats[_i];
  gchar *ext_data_path = g_build_filename (g_get_tmp_dir (), "test.wav", NULL);
  gchar *ext_data_uri = g_strconcat ("file://", ext_data_path, NULL);
  gchar *song_path = make_tmp_song_path ("bt-test3-song.", fi->extension);

  make_song_with_externals (ext_data_uri);

  /* save the song */
  BtSongIO *song_io = bt_song_io_from_file (song_path, NULL);
  gboolean res = bt_song_io_save (song_io, song, NULL);
  fail_unless (res == TRUE, NULL);

  ck_g_object_final_unref (song_io);
  ck_g_object_final_unref (song);
  song = bt_song_new (app);

  GST_INFO ("  song saved");

  /* load the song */
  song_io = bt_song_io_from_file (song_path, NULL);
  res = bt_song_io_load (song_io, song, NULL);
  fail_unless (res == TRUE, NULL);

  GST_INFO ("  song re-loaded");

  ck_g_object_final_unref (song_io);
  ck_g_object_final_unref (song);
  song = bt_song_new (app);

  g_free (song_path);

  GST_INFO ("-- cleanup --");
  g_free (ext_data_uri);
  g_free (ext_data_path);
  BT_TEST_END;
}

static void
test_bt_song_io_native_load_legacy_0_7 (BT_TEST_ARGS)
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
  fail_unless (pattern1 == pattern2, NULL);

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern1);
  g_object_unref (pattern2);
  g_object_unref (machine);
  g_object_unref (setup);
  g_object_unref (sequence);
  ck_g_object_final_unref (song_io);
  BT_TEST_END;
}

TCase *
bt_song_io_native_example_case (void)
{
  TCase *tc = tcase_create ("BtSongIONativeExamples");

  static gint num_formats = 0;
  BtSongIOFormatInfo *fi = bt_song_io_native_module_info.formats;
  while (fi->name) {
    num_formats++;
    fi++;
  }

  tcase_add_test (tc, test_bt_song_io_native_new);
  tcase_add_loop_test (tc, test_bt_song_io_native_formats, 0, num_formats);
  tcase_add_test (tc, test_bt_song_io_native_load);
  tcase_add_test (tc, test_bt_song_io_native_core_refcounts);
  tcase_add_test (tc, test_bt_song_io_native_setup_refcounts_0);
  tcase_add_test (tc, test_bt_song_io_native_setup_refcounts_1);
  tcase_add_test (tc, test_bt_song_io_native_setup_refcounts_2);
  tcase_add_loop_test (tc, test_bt_song_io_native_song_refcounts, 0,
      G_N_ELEMENTS (song_names));
  tcase_add_loop_test (tc, test_bt_song_io_write_empty_song, 0, num_formats);
  tcase_add_loop_test (tc, test_bt_song_io_write_song_without_externals, 0,
      num_formats);
  tcase_add_loop_test (tc, test_bt_song_io_write_song_with_externals, 0,
      num_formats);
  tcase_add_test (tc, test_bt_song_io_native_load_legacy_0_7);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
