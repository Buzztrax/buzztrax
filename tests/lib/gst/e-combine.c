/* Buzztrax
 * Copyright (C) 2015 Stefan Sauer <ensonic@users.sf.net>
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

#include "m-bt-gst.h"

#include "gst/combine.h"
#include "gst/osc-synth.h"

//-- globals

#define WAVE_SIZE 200

//-- fixtures

static void
case_setup (void)
{
  BT_CASE_START;
}

static void
case_teardown (void)
{
}

//-- tests

START_TEST (test_create_obj)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");

  GST_INFO ("-- act --");
  GstBtCombine *mix = gstbt_combine_new ();

  GST_INFO ("-- assert --");
  fail_unless (mix != NULL, NULL);
  fail_unless (G_OBJECT (mix)->ref_count == 1, NULL);

  GST_INFO ("-- cleanup --");
  ck_gst_object_final_unref (mix);
  BT_TEST_END;
}
END_TEST

// cp /tmp/lt-bt_gst_combine_*.svg docs/reference/bt-gst/images/
START_TEST (test_combine_modes)
{
  BT_TEST_START;
  gint16 data1[WAVE_SIZE], data2[WAVE_SIZE];

  GST_INFO ("-- arrange --");
  GstBtCombine *mix = gstbt_combine_new ();
  g_object_set (mix, "combine", _i, NULL);
  GstBtOscSynth *osc1 = gstbt_osc_synth_new ();
  g_object_set (osc1, "wave", 2, "sample-rate", WAVE_SIZE, "frequency", 1.0,
      NULL);
  gstbt_osc_synth_process (osc1, WAVE_SIZE, data1);
  GstBtOscSynth *osc2 = gstbt_osc_synth_new ();
  g_object_set (osc2, "wave", 2, "sample-rate", WAVE_SIZE, "frequency", 2.0,
      NULL);
  gstbt_osc_synth_process (osc2, WAVE_SIZE, data2);

  GST_INFO ("-- act --");
  gstbt_combine_process (mix, WAVE_SIZE, data1, data2);

  GST_INFO ("-- plot --");
  GEnumClass *enum_class = g_type_class_peek_static (GSTBT_TYPE_COMBINE_TYPE);
  GEnumValue *enum_value = g_enum_get_value (enum_class, _i);
  check_plot_data_int16 (data1, WAVE_SIZE, "combine", enum_value->value_name);

  GST_INFO ("-- cleanup --");
  gst_object_unref (osc1);
  gst_object_unref (osc2);
  ck_gst_object_final_unref (mix);
  BT_TEST_END;
}
END_TEST


TCase *
gst_buzztrax_combine_example_case (void)
{
  TCase *tc = tcase_create ("GstBtCombineExamples");

  tcase_add_test (tc, test_create_obj);
  tcase_add_loop_test (tc, test_combine_modes, 0, GSTBT_COMBINE_COUNT);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
