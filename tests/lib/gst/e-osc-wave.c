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

static GstStructure *
get_mono_wave_buffer (gpointer user_data, guint wave_ix, guint wave_level_ix)
{
  static gint16 data[] = { G_MININT16, 0, 0, G_MAXINT16 };
  GstBuffer *buffer = gst_buffer_new_wrapped_full (GST_MEMORY_FLAG_READONLY,
      data, sizeof (data), 0, sizeof (data), NULL, NULL);

  return gst_structure_new ("audio/x-raw",
      "channels", G_TYPE_INT, 1,
      "root-note", GSTBT_TYPE_NOTE, (guint) GSTBT_NOTE_C_3,
      "buffer", GST_TYPE_BUFFER, buffer, NULL);
}

static GstStructure *
get_stereo_wave_buffer (gpointer user_data, guint wave_ix, guint wave_level_ix)
{
  static gint16 data[] = { G_MININT16, 0, 0, G_MAXINT16 };
  GstBuffer *buffer = gst_buffer_new_wrapped_full (GST_MEMORY_FLAG_READONLY,
      data, sizeof (data), 0, sizeof (data), NULL, NULL);

  return gst_structure_new ("audio/x-raw",
      "channels", G_TYPE_INT, 2,
      "root-note", GSTBT_TYPE_NOTE, (guint) GSTBT_NOTE_C_3,
      "buffer", GST_TYPE_BUFFER, buffer, NULL);
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

static void
test_create_obj (BT_TEST_ARGS)
{
  BT_TEST_START;
  GstBtOscWave *osc;

  GST_INFO ("-- arrange --");
  GST_INFO ("-- act --");
  osc = gstbt_osc_wave_new ();

  GST_INFO ("-- assert --");
  fail_unless (osc != NULL, NULL);
  fail_unless (G_OBJECT (osc)->ref_count == 1, NULL);

  GST_INFO ("-- cleanup --");
  ck_gst_object_final_unref (osc);
  BT_TEST_END;
}

static void
test_create_is_configured (BT_TEST_ARGS)
{
  BT_TEST_START;
  GstBtOscWave *osc;
  gpointer wave_callbacks[] = { NULL, get_mono_wave_buffer };

  GST_INFO ("-- arrange --");
  osc = gstbt_osc_wave_new ();

  GST_INFO ("-- act --");
  g_object_set (osc, "wave-callbacks", wave_callbacks, NULL);

  GST_INFO ("-- assert --");
  fail_unless (osc->process != NULL, NULL);

  GST_INFO ("-- cleanup --");
  ck_gst_object_final_unref (osc);
  BT_TEST_END;
}

static void
test_create_mono (BT_TEST_ARGS)
{
  BT_TEST_START;
  GstBtOscWave *osc;
  gint16 data[WAVE_SIZE];
  gpointer wave_callbacks[] = { NULL, get_mono_wave_buffer };

  GST_INFO ("-- arrange --");
  osc = gstbt_osc_wave_new ();
  g_object_set (osc, "wave-callbacks", wave_callbacks, NULL);

  GST_INFO ("-- act --");
  osc->process (osc, 0, WAVE_SIZE, data);

  GST_INFO ("-- assert --");
  ck_assert_int_eq (data[0], G_MININT16);
  ck_assert_int_eq (data[1], 0);
  ck_assert_int_eq (data[2], 0);
  ck_assert_int_eq (data[3], G_MAXINT16);

  GST_INFO ("-- cleanup --");
  ck_gst_object_final_unref (osc);
  BT_TEST_END;
}

static void
test_create_stereo (BT_TEST_ARGS)
{
  BT_TEST_START;
  GstBtOscWave *osc;
  gint16 data[WAVE_SIZE];
  gpointer wave_callbacks[] = { NULL, get_stereo_wave_buffer };

  GST_INFO ("-- arrange --");
  osc = gstbt_osc_wave_new ();
  g_object_set (osc, "wave-callbacks", wave_callbacks, NULL);

  GST_INFO ("-- act --");
  osc->process (osc, 0, WAVE_SIZE / 2, data);

  GST_INFO ("-- assert --");
  ck_assert_int_eq (data[0], G_MININT16);
  ck_assert_int_eq (data[1], 0);
  ck_assert_int_eq (data[2], 0);
  ck_assert_int_eq (data[3], G_MAXINT16);

  GST_INFO ("-- cleanup --");
  ck_gst_object_final_unref (osc);
  BT_TEST_END;
}

TCase *
gst_buzztrax_osc_wave_example_case (void)
{
  TCase *tc = tcase_create ("GstBtOscWaveExamples");

  tcase_add_test (tc, test_create_obj);
  tcase_add_test (tc, test_create_is_configured);
  tcase_add_test (tc, test_create_mono);
  tcase_add_test (tc, test_create_stereo);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
