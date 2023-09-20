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

#include "gst/toneconversion.h"

//-- globals

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
  GstBtToneConversion *n2f;

  n2f = gstbt_tone_conversion_new (GSTBT_TONE_CONVERSION_EQUAL_TEMPERAMENT);
  ck_assert (n2f != NULL);
  ck_assert (G_OBJECT (n2f)->ref_count == 1);

  // free object
  ck_g_object_final_unref (n2f);
  BT_TEST_END;
}
END_TEST

START_TEST (test_translate_str_base)
{
  BT_TEST_START;
  GstBtToneConversion *n2f;
  gdouble frq;

  n2f = gstbt_tone_conversion_new (GSTBT_TONE_CONVERSION_EQUAL_TEMPERAMENT);
  ck_assert (n2f != NULL);

  frq = gstbt_tone_conversion_translate_from_string (n2f, "A-3");
  //g_print("frq=%lf\n",frq);
  ck_assert (frq == 440.0);

  frq = gstbt_tone_conversion_translate_from_string (n2f, "a-3");
  //g_print("frq=%lf\n",frq);
  ck_assert (frq == 440.0);

  // free object
  ck_g_object_final_unref (n2f);
  BT_TEST_END;
}
END_TEST

START_TEST (test_translate_str_series)
{
  BT_TEST_START;
  GstBtToneConversion *n2f;
  gdouble frq, frq_prev = 0.0;
  gchar *notes[] = {
    "c-0", "c#0", "d-0", "d#0", "e-0", "f-0", "f#0", "g-0", "g#0", "a-0", "a#0",
    "h-0",
    "c-1", "c#1", "d-1", "d#1", "e-1", "f-1", "f#1", "g-1", "g#1", "a-1", "a#1",
    "h-1",
    "c-2", NULL
  };
  guint i = 0;

  n2f = gstbt_tone_conversion_new (_i);
  ck_assert (n2f != NULL);

  while (notes[i]) {
    frq = gstbt_tone_conversion_translate_from_string (n2f, notes[i]);
    //g_print("%s -> frq=%lf\n",notes[i],frq);
    ck_assert (frq != 0.0);
    ck_assert (frq > frq_prev);
    frq_prev = frq;
    i++;
  }

  // free object
  ck_g_object_final_unref (n2f);
  BT_TEST_END;
}
END_TEST

START_TEST (test_translate_num_base)
{
  BT_TEST_START;
  GstBtToneConversion *n2f;
  gdouble frq;

  n2f = gstbt_tone_conversion_new (GSTBT_TONE_CONVERSION_EQUAL_TEMPERAMENT);
  ck_assert (n2f != NULL);

  frq = gstbt_tone_conversion_translate_from_number (n2f, GSTBT_NOTE_A_3);
  //g_print("frq=%lf\n",frq);
  ck_assert (frq == 440.0);

  // free object
  ck_g_object_final_unref (n2f);
  BT_TEST_END;
}
END_TEST

START_TEST (test_translate_num_series)
{
  BT_TEST_START;
  GstBtToneConversion *n2f;
  gdouble frq, frq_prev = 0.0;
  guint i, j;

  n2f = gstbt_tone_conversion_new (_i);
  ck_assert (n2f != NULL);

  for (i = 0; i < 9; i++) {
    for (j = 0; j < 12; j++) {
      frq = gstbt_tone_conversion_translate_from_number (n2f, 1 + (i * 16 + j));
      //g_print("%3u -> frq=%lf\n",i,frq);
      ck_assert (frq != 0.0);
      ck_assert (frq > frq_prev);
      frq_prev = frq;
    }
  }

  // free object
  ck_g_object_final_unref (n2f);
  BT_TEST_END;
}
END_TEST

START_TEST (test_convert_note_string_2_number)
{
  BT_TEST_START;
  ck_assert_int_eq (gstbt_tone_conversion_note_string_2_number ("c-0"),
      GSTBT_NOTE_C_0);
  ck_assert_int_eq (gstbt_tone_conversion_note_string_2_number ("off"),
      GSTBT_NOTE_OFF);
  BT_TEST_END;
}
END_TEST

START_TEST (test_convert_note_number_2_string)
{
  BT_TEST_START;
  ck_assert_str_eq (gstbt_tone_conversion_note_number_2_string (GSTBT_NOTE_C_0),
      "c-0");
  ck_assert_str_eq (gstbt_tone_conversion_note_number_2_string (GSTBT_NOTE_OFF),
      "off");
  BT_TEST_END;
}
END_TEST

START_TEST (test_note_number_offset)
{
  BT_TEST_START;
  ck_assert_int_eq (gstbt_tone_conversion_note_number_offset (GSTBT_NOTE_C_0,
          0), GSTBT_NOTE_C_0);
  ck_assert_int_eq (gstbt_tone_conversion_note_number_offset (GSTBT_NOTE_C_0,
          5), GSTBT_NOTE_F_0);
  ck_assert_int_eq (gstbt_tone_conversion_note_number_offset (GSTBT_NOTE_F_0,
          10), GSTBT_NOTE_DIS_1);
  BT_TEST_END;
}
END_TEST

TCase *
gst_buzztrax_toneconversion_example_case (void)
{
  TCase *tc = tcase_create ("GstBtToneConversionExamples");

  tcase_add_test (tc, test_create_obj);
  tcase_add_test (tc, test_translate_str_base);
  tcase_add_loop_test (tc, test_translate_str_series, 0,
      GSTBT_TONE_CONVERSION_COUNT);
  tcase_add_test (tc, test_translate_num_base);
  tcase_add_loop_test (tc, test_translate_num_series, 0,
      GSTBT_TONE_CONVERSION_COUNT);
  tcase_add_test (tc, test_convert_note_string_2_number);
  tcase_add_test (tc, test_convert_note_number_2_string);
  tcase_add_test (tc, test_note_number_offset);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
