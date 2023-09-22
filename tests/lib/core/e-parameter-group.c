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
  machine = NULL;
  ck_g_object_final_unref (song);
  ck_g_object_final_unref (app);
}

static void
case_teardown (void)
{
}

//-- helper

BtParameterGroup *
get_mono_parameter_group (void)
{
  BtMachineConstructorParams cparams;
  cparams.id = "id";
  cparams.song = song;
  
  machine =
      BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0, NULL));
  return bt_machine_get_global_param_group (machine);
}


//-- tests

START_TEST (test_bt_parameter_group_param)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtParameterGroup *pg = get_mono_parameter_group ();

  /* act && assert */
  ck_assert_int_eq (bt_parameter_group_get_param_index (pg, "g-double"), 1);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_parameter_group_size)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtParameterGroup *pg = get_mono_parameter_group ();

  /* act && assert */
  ck_assert_gobject_glong_eq (pg, "num-params", 6L);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

/* try describe on a machine that does not implement the interface */
START_TEST (test_bt_parameter_group_describe)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtParameterGroup *pg = get_mono_parameter_group ();
  GValue val = { 0, };
  
  g_value_init (&val, G_TYPE_ULONG);
  g_value_set_ulong (&val, 1L);

  GST_INFO ("-- act --");
  gchar *str = bt_parameter_group_describe_param_value (pg, 0, &val);

  GST_INFO ("-- assert --");
  ck_assert (str == NULL);

  GST_INFO ("-- cleanup --");
  g_value_unset (&val);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_parameter_group_get_trigger_param)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtParameterGroup *pg = get_mono_parameter_group ();

  GST_INFO ("-- act --");
  glong ix = bt_parameter_group_get_trigger_param_index (pg);

  GST_INFO ("-- assert --");
  ck_assert_int_eq (ix, 3);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_parameter_group_get_wave_param)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtParameterGroup *pg = get_mono_parameter_group ();

  GST_INFO ("-- act --");
  glong ix = bt_parameter_group_get_wave_param_index (pg);

  GST_INFO ("-- assert --");
  ck_assert_int_eq (ix, 5);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_parameter_group_set_value)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtParameterGroup *pg = get_mono_parameter_group ();
  GstObject *element =
      (GstObject *) check_gobject_get_object_property (machine, "machine");
  GValue val = { 0, };
  
  g_value_init (&val, G_TYPE_ULONG);
  g_value_set_ulong (&val, 10L);

  GST_INFO ("-- act --");
  bt_parameter_group_set_param_value (pg, 0, &val);

  GST_INFO ("-- assert --");
  ck_assert_gobject_guint_eq (element, "g-uint", 10);

  GST_INFO ("-- cleanup --");
  gst_object_unref (element);
  g_value_unset (&val);
  BT_TEST_END;
}
END_TEST

/* test setting a new default */
START_TEST (test_bt_parameter_group_set_default)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtParameterGroup *pg = get_mono_parameter_group ();
  GstObject *element =
      (GstObject *) check_gobject_get_object_property (machine, "machine");
  GstControlBinding *cb = gst_object_get_control_binding (element, "g-uint");
  g_object_set (element, "g-uint", 10, NULL);

  GST_INFO ("-- act --");
  bt_parameter_group_set_param_default (pg, 0);

  GST_INFO ("-- assert --");
  GValue *val = gst_control_binding_get_value (cb, G_GUINT64_CONSTANT (0));
  guint uval = g_value_get_uint (val);
  ck_assert_int_eq (uval, 10);

  GST_INFO ("-- cleanup --");
  gst_object_unref (element);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_parameter_group_set_default_trigger)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtParameterGroup *pg = get_mono_parameter_group ();
  GstObject *element =
      (GstObject *) check_gobject_get_object_property (machine, "machine");
  GstControlBinding *cb = gst_object_get_control_binding (element, "g-note");
  g_object_set (element, "g-note", 1, NULL);

  GST_INFO ("-- act --");
  bt_parameter_group_set_param_default (pg, 3);

  GST_INFO ("-- assert --");
  GValue *val = gst_control_binding_get_value (cb, G_GUINT64_CONSTANT (0));
  gint eval = g_value_get_enum (val);
  ck_assert_int_eq (eval, 0);   // default note is 0

  GST_INFO ("-- cleanup --");
  gst_object_unref (element);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_parameter_group_set_defaults)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtParameterGroup *pg = get_mono_parameter_group ();
  GstObject *element =
      (GstObject *) check_gobject_get_object_property (machine, "machine");
  GstControlBinding *cb = gst_object_get_control_binding (element, "g-uint");
  g_object_set (element, "g-uint", 10, NULL);

  GST_INFO ("-- act --");
  bt_parameter_group_set_param_defaults (pg);

  GST_INFO ("-- assert --");
  GValue *val = gst_control_binding_get_value (cb, G_GUINT64_CONSTANT (0));
  guint uval = g_value_get_uint (val);
  ck_assert_int_eq (uval, 10);

  GST_INFO ("-- cleanup --");
  gst_object_unref (element);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_parameter_group_reset_all)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtParameterGroup *pg = get_mono_parameter_group ();
  GstObject *element =
      (GstObject *) check_gobject_get_object_property (machine, "machine");
  g_object_set (element, "g-uint", 10, "g-switch", TRUE, NULL);

  GST_INFO ("-- act --");
  bt_parameter_group_reset_values (pg);

  GST_INFO ("-- assert --");
  ck_assert_gobject_guint_eq (element, "g-uint", 0);
  ck_assert_gobject_gboolean_eq (element, "g-switch", FALSE);

  GST_INFO ("-- cleanup --");
  gst_object_unref (element);
  BT_TEST_END;
}
END_TEST


TCase *
bt_parameter_group_example_case (void)
{
  TCase *tc = tcase_create ("BtParameterGroupExamples");

  tcase_add_test (tc, test_bt_parameter_group_param);
  tcase_add_test (tc, test_bt_parameter_group_size);
  tcase_add_test (tc, test_bt_parameter_group_describe);
  tcase_add_test (tc, test_bt_parameter_group_get_trigger_param);
  tcase_add_test (tc, test_bt_parameter_group_get_wave_param);
  tcase_add_test (tc, test_bt_parameter_group_set_value);
  tcase_add_test (tc, test_bt_parameter_group_set_default);
  tcase_add_test (tc, test_bt_parameter_group_set_default_trigger);
  tcase_add_test (tc, test_bt_parameter_group_set_defaults);
  tcase_add_test (tc, test_bt_parameter_group_reset_all);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
