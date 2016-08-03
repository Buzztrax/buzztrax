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

// no wavetable set, no format negotiated
static void
test_create_disconncted (BT_TEST_ARGS)
{
  BT_TEST_START;
  GstBtOscWave *osc;

  GST_INFO ("-- arrange --");
  osc = gstbt_osc_wave_new ();

  GST_INFO ("-- act --");

  GST_INFO ("-- assert --");
  fail_unless (osc->process == NULL, NULL);

  GST_INFO ("-- cleanup --");
  ck_gst_object_final_unref (osc);
  BT_TEST_END;
}

TCase *
gst_buzztrax_osc_wave_test_case (void)
{
  TCase *tc = tcase_create ("GstBtOscWaveTests");

  tcase_add_test (tc, test_create_disconncted);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
