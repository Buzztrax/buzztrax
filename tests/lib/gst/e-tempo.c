/* Buzztrax
 * Copyright (C) 2016 Stefan Sauer <ensonic@users.sf.net>
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

#include "gst/tempo.h"


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
  GstContext *ctx = gstbt_audio_tempo_context_new (120, 4, 8);

  GST_INFO ("-- assert --");
  ck_assert (ctx != NULL);

  GST_INFO ("-- cleanup --");
  gst_context_unref (ctx);
  BT_TEST_END;
}
END_TEST

START_TEST (test_get_tempo)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  GstContext *ctx = gstbt_audio_tempo_context_new (120, 4, 8);
  guint bpm, tpb, stpb;

  GST_INFO ("-- act --");
  gboolean res = gstbt_audio_tempo_context_get_tempo (ctx, &bpm, &tpb, &stpb);

  GST_INFO ("-- assert --");
  ck_assert (res);
  ck_assert_int_eq (bpm, 120);
  ck_assert_int_eq (tpb, 4);
  ck_assert_int_eq (stpb, 8);

  GST_INFO ("-- cleanup --");
  gst_context_unref (ctx);
  BT_TEST_END;
}
END_TEST

TCase *
gst_buzztrax_tempo_example_case (void)
{
  TCase *tc = tcase_create ("GstBtTempoExamples");

  tcase_add_test (tc, test_create_obj);
  tcase_add_test (tc, test_get_tempo);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
