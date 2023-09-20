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

#include "gst/osc-wave.h"

//-- globals

#define WAVE_SIZE 10

static GstStructure *
get_no_wave_buffer (gpointer user_data, guint wave_ix, guint wave_level_ix)
{
  return NULL;
}

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

// no wavetable set, no format negotiated
START_TEST (test_osc_wave_create_disconncted)
{
  BT_TEST_START;
  GstBtOscWave *osc;

  GST_INFO ("-- arrange --");
  osc = gstbt_osc_wave_new ();

  GST_INFO ("-- act --");

  GST_INFO ("-- assert --");
  ck_assert (osc->process == NULL);

  GST_INFO ("-- cleanup --");
  ck_gst_object_final_unref (osc);
  BT_TEST_END;
}
END_TEST

START_TEST (test_osc_wave_no_wave)
{
  BT_TEST_START;
  GstBtOscWave *osc;
  gpointer wave_callbacks[] = { NULL, get_no_wave_buffer };

  GST_INFO ("-- arrange --");
  osc = gstbt_osc_wave_new ();
  g_object_set (osc, "wave-callbacks", wave_callbacks, NULL);

  GST_INFO ("-- act --");

  GST_INFO ("-- assert --");
  ck_assert (osc->process == NULL);

  GST_INFO ("-- cleanup --");
  ck_gst_object_final_unref (osc);
  BT_TEST_END;
}
END_TEST

TCase *
gst_buzztrax_osc_wave_test_case (void)
{
  TCase *tc = tcase_create ("GstBtOscWaveTests");

  tcase_add_test (tc, test_osc_wave_create_disconncted);
  tcase_add_test (tc, test_osc_wave_no_wave);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
