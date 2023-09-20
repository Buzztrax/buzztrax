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

#include "gst/envelope-d.h"

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
  GstBtEnvelopeD *env;

  GST_INFO ("-- arrange --");
  GST_INFO ("-- act --");
  env = gstbt_envelope_d_new ();

  GST_INFO ("-- assert --");
  ck_assert (env != NULL);
  ck_assert (G_OBJECT (env)->ref_count == 1);

  GST_INFO ("-- cleanup --");
  ck_gst_object_final_unref (env);
  BT_TEST_END;
}
END_TEST

// cp /tmp/lt-bt_gst_envelope-d_*.svg docs/reference/bt-gst/images/
START_TEST (test_envelope_curves)
{
  BT_TEST_START;
  GstBtEnvelopeD *env;
  gdouble data[WAVE_SIZE];
  gdouble curve = _i / 4.0;
  gchar name[20];

  GST_INFO ("-- arrange --");
  env = gstbt_envelope_d_new ();
  g_object_set (env, "curve", curve, "peak-level", 1.0, "floor-level", 0.0,
      "decay", 1.0, NULL);

  GST_INFO ("-- act --");
  gstbt_envelope_d_setup (env, WAVE_SIZE);
  gst_control_source_get_value_array ((GstControlSource *) env, 0, 1, WAVE_SIZE,
      data);

  GST_INFO ("-- assert --");
  sprintf (name, "%4.2f", curve);
  check_plot_data_double (data, WAVE_SIZE, "envelope-d", name, NULL);

  GST_INFO ("-- cleanup --");
  ck_gst_object_final_unref (env);
  BT_TEST_END;
}
END_TEST


TCase *
gst_buzztrax_envelope_d_example_case (void)
{
  TCase *tc = tcase_create ("GstBtEnvelopeDExamples");

  tcase_add_test (tc, test_create_obj);
  tcase_add_loop_test (tc, test_envelope_curves, 0, 5);
  // test access beyond range
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
