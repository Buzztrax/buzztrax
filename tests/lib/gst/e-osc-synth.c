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

#include "m-bt-gst.h"

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
  GstBtOscSynth *osc;

  GST_INFO ("-- arrange --");
  GST_INFO ("-- act --");
  osc = gstbt_osc_synth_new ();

  GST_INFO ("-- assert --");
  ck_assert (osc != NULL);
  ck_assert (G_OBJECT (osc)->ref_count == 1);

  GST_INFO ("-- cleanup --");
  ck_gst_object_final_unref (osc);
  BT_TEST_END;
}
END_TEST

// cp /tmp/lt-bt_gst_osc-synth_*.svg docs/reference/bt-gst/images/
START_TEST (test_waves_not_silent)
{
  BT_TEST_START;
  GstBtOscSynth *osc;
  gint16 data[WAVE_SIZE];

  GST_INFO ("-- arrange --");
  osc = gstbt_osc_synth_new ();
  // plot 1 cycle
  // TODO: this is less useful for S&H, Spikes and S&G
  g_object_set (osc, "wave", _i, "sample-rate", WAVE_SIZE, "frequency", 1.0,
      NULL);

  GST_INFO ("-- act --");
  gstbt_osc_synth_process (osc, WAVE_SIZE, data);

  GST_INFO ("-- plot --");
  GEnumClass *enum_class = g_type_class_peek_static (GSTBT_TYPE_OSC_SYNTH_WAVE);
  GEnumValue *enum_value = g_enum_get_value (enum_class, _i);
  check_plot_data_int16 (data, WAVE_SIZE, "osc-synth", enum_value->value_name);

  GST_INFO ("-- assert --");
  if (_i != GSTBT_OSC_SYNTH_WAVE_SILENCE) {
    gint j;
    for (j = 0; j < WAVE_SIZE; j++) {
      if (data[j] != 0)
        break;
    }
    ck_assert_msg (j != WAVE_SIZE, "in %s all samples are 0", enum_value->value_name);
  }

  GST_INFO ("-- cleanup --");
  ck_gst_object_final_unref (osc);
  BT_TEST_END;
}
END_TEST

TCase *
gst_buzztrax_osc_synth_example_case (void)
{
  TCase *tc = tcase_create ("GstBtOscSynthExamples");

  tcase_add_test (tc, test_create_obj);
  tcase_add_loop_test (tc, test_waves_not_silent, 0,
      GSTBT_OSC_SYNTH_WAVE_COUNT);
  // test each wave with a volume and frequency decay env
  // test that for a larger wave, summing up all values should be ~0
  // test that for non noise waves, we should get min/max
  // test gstbt_osc_synth_trigger()
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
