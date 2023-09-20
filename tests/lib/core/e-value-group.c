/* Buzztrax
 * Copyright (C) 2012 Buzztrax team <buzztrax-devel@buzztrax.org>
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

#include "m-bt-core.h"

//-- globals

static BtApplication *app;
static BtSong *song;
static BtMachine *machine;
static BtPattern *pattern;

#define NUM_COLUMNS 5

//-- fixtures

static void
case_setup (void)
{
  BT_CASE_START;
}

static void
test_setup (void)
{
  app = bt_test_application_new ();
  song = bt_song_new (app);
}

static void
test_teardown (void)
{
  g_object_unref (pattern);
  pattern = NULL;
  machine = NULL;
  ck_g_object_final_unref (song);
  ck_g_object_final_unref (app);
}

static void
case_teardown (void)
{
}

//-- helper

static BtValueGroup *
get_mono_value_group (void)
{
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "id";
  machine =
      BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0, NULL));
  pattern = bt_pattern_new (song, "pattern-name", 4L, machine);
  return bt_pattern_get_global_group (pattern);
}


//-- tests

START_TEST (test_bt_value_group_default_empty)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtValueGroup *vg = get_mono_value_group ();

  /* act && assert */
  ck_assert (!G_IS_VALUE (bt_value_group_get_event_data (vg, 0, 0)));

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_value_group_value)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtValueGroup *vg = get_mono_value_group ();

  GST_INFO ("-- act --");
  bt_value_group_set_event (vg, 0, 0, "10");

  GST_INFO ("-- assert --");
  ck_assert_str_eq_and_free (bt_value_group_get_event (vg, 0, 0), "10");

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_value_group_insert_row)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtValueGroup *vg = get_mono_value_group ();
  bt_value_group_set_event (vg, 0, 0, "10");
  bt_value_group_set_event (vg, 1, 0, "20");

  GST_INFO ("-- act --");
  bt_value_group_insert_full_row (vg, 1);

  GST_INFO ("-- assert --");
  ck_assert_str_eq_and_free (bt_value_group_get_event (vg, 0, 0), "10");
  ck_assert_str_eq_and_free (bt_value_group_get_event (vg, 1, 0), NULL);
  ck_assert_str_eq_and_free (bt_value_group_get_event (vg, 2, 0), "20");

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_value_group_delete_row)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtValueGroup *vg = get_mono_value_group ();
  bt_value_group_set_event (vg, 0, 0, "10");
  bt_value_group_set_event (vg, 1, 0, "20");

  GST_INFO ("-- act --");
  bt_value_group_delete_full_row (vg, 0);

  GST_INFO ("-- assert --");
  ck_assert_str_eq_and_free (bt_value_group_get_event (vg, 0, 0), "20");
  ck_assert_str_eq_and_free (bt_value_group_get_event (vg, 1, 0), NULL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_value_group_clear_column)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtValueGroup *vg = get_mono_value_group ();
  bt_value_group_set_event (vg, 0, 0, "10");
  bt_value_group_set_event (vg, 1, 0, "20");

  GST_INFO ("-- act --");
  bt_value_group_transform_colum (vg, BT_VALUE_GROUP_OP_CLEAR, 0, 3, 0);

  GST_INFO ("-- assert --");
  ck_assert_str_eq_and_free (bt_value_group_get_event (vg, 0, 0), NULL);
  ck_assert_str_eq_and_free (bt_value_group_get_event (vg, 1, 0), NULL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_value_group_clear_columns)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtValueGroup *vg = get_mono_value_group ();
  bt_value_group_set_event (vg, 0, 0, "10");
  bt_value_group_set_event (vg, 1, 0, "20");
  bt_value_group_set_event (vg, 0, 1, "10");
  bt_value_group_set_event (vg, 1, 1, "20");

  GST_INFO ("-- act --");
  bt_value_group_transform_colums (vg, BT_VALUE_GROUP_OP_CLEAR, 0, 3);

  GST_INFO ("-- assert --");
  ck_assert_str_eq_and_free (bt_value_group_get_event (vg, 0, 0), NULL);
  ck_assert_str_eq_and_free (bt_value_group_get_event (vg, 1, 0), NULL);
  ck_assert_str_eq_and_free (bt_value_group_get_event (vg, 0, 1), NULL);
  ck_assert_str_eq_and_free (bt_value_group_get_event (vg, 1, 1), NULL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

static struct _blend_column
{
  const gchar *init[2];
  const gchar *res[4];
} blend_column[] = {
  /* *INDENT-OFF* */
  {{ "10", "40"}, { "10", "20", "30", "40"}},       // uint
  {{ "10", "40"}, { "10", "20", "30", "40"}},       // gdouble
  {{ "0", "1"}, { "0", "0", "1", "1" }},            // switch: not impl.
  {{ "c-3", "d#3"}, { "c-3", "c#3", "d-3", "d#3"}}, // note
  {{ "0", "20"}, {"0", "1", "10", "20"}},           // sparse enum
  /* *INDENT-ON* */
};

START_TEST (test_bt_value_group_blend_column)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  struct _blend_column *p = &blend_column[_i];
  BtValueGroup *vg = get_mono_value_group ();
  bt_value_group_set_event (vg, 0, _i, p->init[0]);
  bt_value_group_set_event (vg, 3, _i, p->init[1]);

  GST_INFO ("-- act --");
  bt_value_group_transform_colum (vg, BT_VALUE_GROUP_OP_BLEND, 0, 3, _i);

  GST_INFO ("-- assert --");
  guint i;
  for (i = 0; i < 4; i++) {
    ck_assert_str_eq_and_free (bt_value_group_get_event (vg, i, _i), p->res[i]);
  }

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_value_group_flip_column)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtValueGroup *vg = get_mono_value_group ();
  bt_value_group_set_event (vg, 0, 0, "10");
  bt_value_group_set_event (vg, 3, 0, "40");

  GST_INFO ("-- act --");
  bt_value_group_transform_colum (vg, BT_VALUE_GROUP_OP_FLIP, 0, 3, 0);

  GST_INFO ("-- assert --");
  ck_assert_str_eq_and_free (bt_value_group_get_event (vg, 0, 0), "40");
  ck_assert_str_eq_and_free (bt_value_group_get_event (vg, 1, 0), NULL);
  ck_assert_str_eq_and_free (bt_value_group_get_event (vg, 2, 0), NULL);
  ck_assert_str_eq_and_free (bt_value_group_get_event (vg, 3, 0), "10");

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_value_group_randomize_column)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtValueGroup *vg = get_mono_value_group ();

  GST_INFO ("-- act --");
  bt_value_group_transform_colum (vg, BT_VALUE_GROUP_OP_RANDOMIZE, 0, 3, _i);

  GST_INFO ("-- assert --");
  guint i;
  for (i = 0; i < 4; i++) {
    ck_assert (G_IS_VALUE (bt_value_group_get_event_data (vg, 0, _i)));
  }

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_value_group_range_randomize_column)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtValueGroup *vg = get_mono_value_group ();
  bt_value_group_set_event (vg, 0, 0, "5");
  bt_value_group_set_event (vg, 3, 0, "50");

  GST_INFO ("-- act --");
  bt_value_group_transform_colum (vg, BT_VALUE_GROUP_OP_RANGE_RANDOMIZE, 0, 3,
      0);

  GST_INFO ("-- assert --");
  gchar *str;
  gint v, i;
  for (i = 0; i < 4; i++) {
    GST_DEBUG ("value[%d]", i);
    str = bt_value_group_get_event (vg, i, 0);
    ck_assert (str != NULL);
    v = atoi (str);
    ck_assert_int_ge (v, 5);
    ck_assert_int_le (v, 50);
    g_free (str);
  }

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_value_group_transpose_fine_up_column)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtValueGroup *vg = get_mono_value_group ();
  bt_value_group_set_event (vg, 0, 0, "5");
  bt_value_group_set_event (vg, 3, 0, "50");

  GST_INFO ("-- act --");
  bt_value_group_transform_colum (vg, BT_VALUE_GROUP_OP_TRANSPOSE_FINE_UP, 0, 3,
      0);

  GST_INFO ("-- assert --");
  ck_assert_str_eq_and_free (bt_value_group_get_event (vg, 0, 0), "6");
  ck_assert_str_eq_and_free (bt_value_group_get_event (vg, 1, 0), NULL);
  ck_assert_str_eq_and_free (bt_value_group_get_event (vg, 2, 0), NULL);
  ck_assert_str_eq_and_free (bt_value_group_get_event (vg, 3, 0), "51");
  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_value_group_transpose_fine_down_column)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtValueGroup *vg = get_mono_value_group ();
  bt_value_group_set_event (vg, 0, 0, "5");
  bt_value_group_set_event (vg, 3, 0, "50");

  GST_INFO ("-- act --");
  bt_value_group_transform_colum (vg, BT_VALUE_GROUP_OP_TRANSPOSE_FINE_DOWN, 0,
      3, 0);

  GST_INFO ("-- assert --");
  ck_assert_str_eq_and_free (bt_value_group_get_event (vg, 0, 0), "4");
  ck_assert_str_eq_and_free (bt_value_group_get_event (vg, 1, 0), NULL);
  ck_assert_str_eq_and_free (bt_value_group_get_event (vg, 2, 0), NULL);
  ck_assert_str_eq_and_free (bt_value_group_get_event (vg, 3, 0), "49");
  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_value_group_copy)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtValueGroup *vg1 = get_mono_value_group ();
  bt_value_group_set_event (vg1, 0, 0, "10");

  GST_INFO ("-- act --");
  BtValueGroup *vg2 = bt_value_group_copy (vg1);

  GST_INFO ("-- assert --");
  ck_assert_str_eq_and_free (bt_value_group_get_event (vg2, 0, 0), "10");

  GST_INFO ("-- cleanup --");
  g_object_unref (vg2);
  BT_TEST_END;
}
END_TEST

TCase *
bt_value_group_example_case (void)
{
  TCase *tc = tcase_create ("BtValueGroupExamples");

  tcase_add_test (tc, test_bt_value_group_default_empty);
  tcase_add_test (tc, test_bt_value_group_value);
  tcase_add_test (tc, test_bt_value_group_insert_row);
  tcase_add_test (tc, test_bt_value_group_delete_row);
  tcase_add_test (tc, test_bt_value_group_clear_column);
  tcase_add_test (tc, test_bt_value_group_clear_columns);
  tcase_add_loop_test (tc, test_bt_value_group_blend_column, 0, NUM_COLUMNS);
  tcase_add_test (tc, test_bt_value_group_flip_column);
  tcase_add_loop_test (tc, test_bt_value_group_randomize_column, 0,
      NUM_COLUMNS);
  tcase_add_test (tc, test_bt_value_group_range_randomize_column);
  tcase_add_test (tc, test_bt_value_group_transpose_fine_up_column);
  tcase_add_test (tc, test_bt_value_group_transpose_fine_down_column);
  tcase_add_test (tc, test_bt_value_group_copy);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
