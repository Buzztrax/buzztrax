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
get_mono_wave_buffer (gpointer user_data, guint wave_ix, guint wave_level_ix)
{
  static gint16 data[] = { G_MININT16, -1, 1, G_MAXINT16 };
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
  static gint16 data[] = { G_MININT16, -1, 1, G_MAXINT16 };
  GstBuffer *buffer = gst_buffer_new_wrapped_full (GST_MEMORY_FLAG_READONLY,
      data, sizeof (data), 0, sizeof (data), NULL, NULL);

  return gst_structure_new ("audio/x-raw",
      "channels", G_TYPE_INT, 2,
      "root-note", GSTBT_TYPE_NOTE, (guint) GSTBT_NOTE_C_3,
      "buffer", GST_TYPE_BUFFER, buffer, NULL);
}

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

START_TEST (test_osc_wave_create_obj)
{
  BT_TEST_START;
  GstBtOscWave *osc;

  GST_INFO ("-- arrange --");
  GST_INFO ("-- act --");
  osc = gstbt_osc_wave_new ();

  GST_INFO ("-- assert --");
  ck_assert (osc != NULL);
  ck_assert (G_OBJECT (osc)->ref_count == 1);

  GST_INFO ("-- cleanup --");
  ck_gst_object_final_unref (osc);
  BT_TEST_END;
}
END_TEST

START_TEST (test_osc_wave_is_configured)
{
  BT_TEST_START;
  GstBtOscWave *osc;
  gpointer wave_callbacks[] = { NULL, get_mono_wave_buffer };

  GST_INFO ("-- arrange --");
  osc = gstbt_osc_wave_new ();

  GST_INFO ("-- act --");
  g_object_set (osc, "wave-callbacks", wave_callbacks, NULL);

  GST_INFO ("-- assert --");
  ck_assert (osc->process != NULL);

  GST_INFO ("-- cleanup --");
  ck_gst_object_final_unref (osc);
  BT_TEST_END;
}
END_TEST

START_TEST (test_osc_wave_reconfigure)
{
  BT_TEST_START;
  GstBtOscWave *osc;
  gpointer mono_wave_callbacks[] = { NULL, get_mono_wave_buffer };
  gpointer stereo_wave_callbacks[] = { NULL, get_stereo_wave_buffer };

  GST_INFO ("-- arrange --");
  osc = gstbt_osc_wave_new ();
  g_object_set (osc, "wave-callbacks", mono_wave_callbacks, NULL);
  gpointer mono_process_fn = osc->process;

  GST_INFO ("-- act --");
  g_object_set (osc, "wave-callbacks", stereo_wave_callbacks, NULL);

  GST_INFO ("-- assert --");
  ck_assert (osc->process != mono_process_fn);

  GST_INFO ("-- cleanup --");
  ck_gst_object_final_unref (osc);
  BT_TEST_END;
}
END_TEST

START_TEST (test_osc_wave_reconfigure_resets)
{
  BT_TEST_START;
  GstBtOscWave *osc;
  gpointer mono_wave_callbacks[] = { NULL, get_mono_wave_buffer };
  gpointer no_wave_callbacks[] = { NULL, get_no_wave_buffer };

  GST_INFO ("-- arrange --");
  osc = gstbt_osc_wave_new ();
  g_object_set (osc, "wave-callbacks", mono_wave_callbacks, NULL);

  GST_INFO ("-- act --");
  g_object_set (osc, "wave-callbacks", no_wave_callbacks, NULL);

  GST_INFO ("-- assert --");
  ck_assert (osc->process == NULL);

  GST_INFO ("-- cleanup --");
  ck_gst_object_final_unref (osc);
  BT_TEST_END;
}
END_TEST

START_TEST (test_osc_wave_create_mono)
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
  ck_assert_int_eq (data[1], -1);
  ck_assert_int_eq (data[2], 1);
  ck_assert_int_eq (data[3], G_MAXINT16);

  GST_INFO ("-- cleanup --");
  ck_gst_object_final_unref (osc);
  BT_TEST_END;
}
END_TEST

START_TEST (test_osc_wave_create_stereo)
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
  ck_assert_int_eq (data[1], -1);
  ck_assert_int_eq (data[2], 1);
  ck_assert_int_eq (data[3], G_MAXINT16);

  GST_INFO ("-- cleanup --");
  ck_gst_object_final_unref (osc);
  BT_TEST_END;
}
END_TEST

START_TEST (test_osc_wave_create_mono_beyond_size)
{
  BT_TEST_START;
  GstBtOscWave *osc;
  gint16 data[WAVE_SIZE];
  gpointer wave_callbacks[] = { NULL, get_mono_wave_buffer };
  gint i;

  GST_INFO ("-- arrange --");
  osc = gstbt_osc_wave_new ();
  g_object_set (osc, "wave-callbacks", wave_callbacks, NULL);

  GST_INFO ("-- act --");
  osc->process (osc, 4, WAVE_SIZE, data);

  GST_INFO ("-- assert --");
  for (i = 0; i < WAVE_SIZE; i++)
    ck_assert_int_eq (data[i], 0);

  GST_INFO ("-- cleanup --");
  ck_gst_object_final_unref (osc);
  BT_TEST_END;
}
END_TEST

START_TEST (test_osc_wave_create_stereo_beyond_size)
{
  BT_TEST_START;
  GstBtOscWave *osc;
  gint16 data[WAVE_SIZE];
  gpointer wave_callbacks[] = { NULL, get_stereo_wave_buffer };
  gint i;

  GST_INFO ("-- arrange --");
  osc = gstbt_osc_wave_new ();
  g_object_set (osc, "wave-callbacks", wave_callbacks, NULL);

  GST_INFO ("-- act --");
  osc->process (osc, 2, WAVE_SIZE / 2, data);

  GST_INFO ("-- assert --");
  for (i = 0; i < WAVE_SIZE; i++)
    ck_assert_int_eq (data[i], 0);

  GST_INFO ("-- cleanup --");
  ck_gst_object_final_unref (osc);
  BT_TEST_END;
}
END_TEST

TCase *
gst_buzztrax_osc_wave_example_case (void)
{
  TCase *tc = tcase_create ("GstBtOscWaveExamples");

  tcase_add_test (tc, test_osc_wave_create_obj);
  tcase_add_test (tc, test_osc_wave_is_configured);
  tcase_add_test (tc, test_osc_wave_reconfigure);
  tcase_add_test (tc, test_osc_wave_reconfigure_resets);
  tcase_add_test (tc, test_osc_wave_create_mono);
  tcase_add_test (tc, test_osc_wave_create_stereo);
  tcase_add_test (tc, test_osc_wave_create_mono_beyond_size);
  tcase_add_test (tc, test_osc_wave_create_stereo_beyond_size);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
