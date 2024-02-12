/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@buzztrax.org>
 *
 * core library unit tests
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

#define BT_CHECK

#include "bt-check.h"
#include "core/core.h"
#include <stdlib.h>

GST_DEBUG_CATEGORY (GST_CAT_DEFAULT);
GST_DEBUG_CATEGORY_EXTERN (bt_core_debug);

gchar *test_argv[] = { "check_buzztrax" };

gchar **test_argvptr = test_argv;
gint test_argc = G_N_ELEMENTS (test_argv);

BT_TEST_SUITE_E ("BtApplication", bt_application);
BT_TEST_SUITE_E ("BtAudioSession", bt_audio_session);
BT_TEST_SUITE_T_E ("BtChildProxy", bt_child_proxy);
BT_TEST_SUITE_T_E ("BtCmdPattern", bt_cmd_pattern);
BT_TEST_SUITE_E ("BtCmdPatternControlSource", bt_cmd_pattern_control_source);
BT_TEST_SUITE_T_E ("BtCore", bt_core);
BT_TEST_SUITE_T_E ("BtMachine", bt_machine);
BT_TEST_SUITE_T_E ("BtParameterGroup", bt_parameter_group);
BT_TEST_SUITE_T_E ("BtPattern", bt_pattern);
BT_TEST_SUITE_T_E ("BtPatternControlSource", bt_pattern_control_source);
BT_TEST_SUITE_T_E ("BtProcessorMachine", bt_processor_machine);
BT_TEST_SUITE_T_E ("BtSequence", bt_sequence);
BT_TEST_SUITE_E ("BtSettings", bt_settings);
BT_TEST_SUITE_T_E ("BtSetup", bt_setup);
BT_TEST_SUITE_T_E ("BtSinkBin", bt_sink_bin);
BT_TEST_SUITE_T_E ("BtSinkMachine", bt_sink_machine);
BT_TEST_SUITE_T_E ("BtSongInfo", bt_song_info);
BT_TEST_SUITE_T_E ("BtSongIONative", bt_song_io_native);
BT_TEST_SUITE_T_E ("BtSongIO", bt_song_io);
BT_TEST_SUITE_T_E ("BtSong", bt_song);
BT_TEST_SUITE_T_E ("BtSourceMachine", bt_source_machine);
BT_TEST_SUITE_T_E ("BtTools", bt_tools);
BT_TEST_SUITE_T_E ("BtValueGroup", bt_value_group);
BT_TEST_SUITE_T_E ("BtWaveTable", bt_wave_table);
BT_TEST_SUITE_T_E ("BtWave", bt_wave);
BT_TEST_SUITE_T_E ("BtWire", bt_wire);

/* start the test run */
gint
main (gint argc, gchar ** argv)
{
  gint nf;
  SRunner *sr;

  setup_log_base (argc, argv);
  setup_log_capture ();
  gst_init (NULL, NULL);

  bt_check_init ();
  bt_init (NULL, &test_argc, &test_argvptr);

  sr = srunner_create (bt_application_suite ());
  srunner_add_suite (sr, bt_audio_session_suite ());
  srunner_add_suite (sr, bt_child_proxy_suite ());
  srunner_add_suite (sr, bt_cmd_pattern_suite ());
  srunner_add_suite (sr, bt_cmd_pattern_control_source_suite ());
  srunner_add_suite (sr, bt_core_suite ());
  srunner_add_suite (sr, bt_machine_suite ());
  srunner_add_suite (sr, bt_parameter_group_suite ());
  srunner_add_suite (sr, bt_pattern_suite ());
  srunner_add_suite (sr, bt_pattern_control_source_suite ());
  srunner_add_suite (sr, bt_processor_machine_suite ());
  srunner_add_suite (sr, bt_sequence_suite ());
  srunner_add_suite (sr, bt_settings_suite ());
  srunner_add_suite (sr, bt_setup_suite ());
  srunner_add_suite (sr, bt_sink_bin_suite ());
  srunner_add_suite (sr, bt_sink_machine_suite ());
  srunner_add_suite (sr, bt_song_info_suite ());
  srunner_add_suite (sr, bt_song_io_native_suite ());
  srunner_add_suite (sr, bt_song_io_suite ());
  srunner_add_suite (sr, bt_song_suite ());
  srunner_add_suite (sr, bt_source_machine_suite ());
  srunner_add_suite (sr, bt_tools_suite ());
  srunner_add_suite (sr, bt_value_group_suite ());
  srunner_add_suite (sr, bt_wave_suite ());
  srunner_add_suite (sr, bt_wave_table_suite ());
  srunner_add_suite (sr, bt_wire_suite ());
  // srunner_set_xml (sr, get_suite_log_filename ("xml"));
  srunner_set_tap (sr, get_suite_log_filename ("tap"));
  srunner_run_all (sr, CK_NORMAL);
  nf = srunner_ntests_failed (sr);
  srunner_free (sr);

  bt_deinit ();
  collect_logs (nf == 0);

  //g_mem_profile();

  return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
