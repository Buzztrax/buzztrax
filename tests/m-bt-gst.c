/* Buzztrax
 * Copyright (C) 2012 Stefan Sauer <ensonic@users.sf.net>
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

#include "bt-check.h"
#include <stdlib.h>

GST_DEBUG_CATEGORY (GST_CAT_DEFAULT);

gchar *test_argv[] = { "check_buzztrax" };

gchar **test_argvptr = test_argv;
gint test_argc = G_N_ELEMENTS (test_argv);

BT_TEST_SUITE_T_E ("GstElements", gst_buzztrax_elements);
BT_TEST_SUITE_E ("GstBtAudiosynth", gst_buzztrax_audiosynth);
BT_TEST_SUITE_E ("GstBtCombine", gst_buzztrax_combine);
BT_TEST_SUITE_E ("GstBtEnvelopeAD", gst_buzztrax_envelope_ad);
BT_TEST_SUITE_E ("GstBtEnvelopeADSR", gst_buzztrax_envelope_adsr);
BT_TEST_SUITE_E ("GstBtEnvelopeD", gst_buzztrax_envelope_d);
BT_TEST_SUITE_E ("GstBtFilterSVF", gst_buzztrax_filter_svf);
BT_TEST_SUITE_E ("GstBtOscSynth", gst_buzztrax_osc_synth);
BT_TEST_SUITE_T_E ("GstBtOscWave", gst_buzztrax_osc_wave);
BT_TEST_SUITE_T_E ("GstBtTempo", gst_buzztrax_tempo);
BT_TEST_SUITE_T_E ("GstBtToneConversion", gst_buzztrax_toneconversion);

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

  sr = srunner_create (gst_buzztrax_elements_suite ());
  srunner_add_suite (sr, gst_buzztrax_audiosynth_suite ());
  srunner_add_suite (sr, gst_buzztrax_combine_suite ());
  srunner_add_suite (sr, gst_buzztrax_envelope_ad_suite ());
  srunner_add_suite (sr, gst_buzztrax_envelope_adsr_suite ());
  srunner_add_suite (sr, gst_buzztrax_envelope_d_suite ());
  srunner_add_suite (sr, gst_buzztrax_filter_svf_suite ());
  srunner_add_suite (sr, gst_buzztrax_osc_synth_suite ());
  srunner_add_suite (sr, gst_buzztrax_osc_wave_suite ());
  srunner_add_suite (sr, gst_buzztrax_tempo_suite ());
  srunner_add_suite (sr, gst_buzztrax_toneconversion_suite ());
  srunner_set_xml (sr, get_suite_log_filename ());
  srunner_run_all (sr, CK_NORMAL);
  nf = srunner_ntests_failed (sr);
  srunner_free (sr);

  bt_deinit ();
  collect_logs (nf == 0);

  return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
