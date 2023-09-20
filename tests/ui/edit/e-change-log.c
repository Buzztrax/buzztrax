/* Buzztrax
 * Copyright (C) 2010 Buzztrax team <buzztrax-devel@buzztrax.org>
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

#include "m-bt-edit.h"
#include <glib/gstdio.h>

//-- test class

typedef struct
{
  const GObject parent;

  gint val;

  gint *data;
  gint data_size;

} BtTestChangeLogger;

typedef struct
{
  const GObjectClass parent;

} BtTestChangeLoggerClass;

static GObjectClass *test_change_logger_parent_class = NULL;

static gboolean
bt_test_change_logger_change (const BtChangeLogger * owner, const gchar * data)
{
  BtTestChangeLogger *self = (BtTestChangeLogger *) owner;
  gboolean res = FALSE;

  GST_INFO ("executing: '%s'", data);

  if (!strncmp (data, "set_val ", 8)) {
    self->val = atoi (&data[8]);
    GST_DEBUG ("val=%d", self->val);
    res = TRUE;
  } else if (!strncmp (data, "new_data ", 9)) {
    g_free (self->data);
    self->data = NULL;
    self->data_size = atoi (&data[9]);
    if (self->data_size) {
      self->data = g_new0 (gint, self->data_size);
    }
    GST_DEBUG ("data_size=%d", self->data_size);
    res = TRUE;
  } else if (!strncmp (data, "inc_data ", 9)) {
    gint ix = atoi (&data[9]);
    if (ix < self->data_size) {
      self->data[ix]++;
      GST_DEBUG ("data[%d]=%d", ix, self->data[ix]);
    } else {
      GST_WARNING ("ix(%d) >= data_size(%d)", ix, self->data_size);
    }
    res = TRUE;
  } else if (!strncmp (data, "dec_data ", 9)) {
    gint ix = atoi (&data[9]);
    if (ix < self->data_size) {
      self->data[ix]--;
      GST_DEBUG ("data[%d]=%d", ix, self->data[ix]);
    } else {
      GST_WARNING ("ix(%d) >= data_size(%d)", ix, self->data_size);
    }
    res = TRUE;
  }
  return res;
}

static void
bt_test_change_logger_interface_init (gpointer const g_iface,
    gconstpointer const iface_data)
{
  BtChangeLoggerInterface *const iface = g_iface;

  iface->change = bt_test_change_logger_change;
}

static void
bt_test_change_logger_finalize (GObject * object)
{
  BtTestChangeLogger *self = (BtTestChangeLogger *) object;

  g_free (self->data);

  G_OBJECT_CLASS (test_change_logger_parent_class)->finalize (object);
}

static void
bt_test_change_logger_init (BtTestChangeLogger * self)
{
}

static void
bt_test_change_logger_class_init (BtTestChangeLoggerClass * const klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  test_change_logger_parent_class = g_type_class_peek_parent (klass);

  gobject_class->finalize = bt_test_change_logger_finalize;
}

G_DEFINE_TYPE_WITH_CODE (BtTestChangeLogger, bt_test_change_logger,
    G_TYPE_OBJECT, G_IMPLEMENT_INTERFACE (BT_TYPE_CHANGE_LOGGER,
        bt_test_change_logger_interface_init));

static BtTestChangeLogger *
bt_test_change_logger_new (void)
{
  return (BtTestChangeLogger *)
      g_object_new (bt_test_change_logger_get_type (), NULL);
}

static void
make_change (BtChangeLog * cl, BtTestChangeLogger * tcl,
    gchar * change, gchar * undo)
{
  bt_test_change_logger_change ((const BtChangeLogger *) tcl, change);
  bt_change_log_add (cl, (BtChangeLogger *) tcl, g_strdup (undo),
      g_strdup (change));
}

//-- globals

static BtEditApplication *app;
static BtMainWindow *main_window;

//-- fixtures

static void
case_setup (void)
{
  BT_CASE_START;
}

static void
test_setup (void)
{
  bt_edit_setup ();
  app = bt_edit_application_new ();
  bt_edit_application_new_song (app);
  g_object_get (app, "main-window", &main_window, NULL);

  flush_main_loop ();
}

static void
test_teardown (void)
{
  gtk_widget_destroy (GTK_WIDGET (main_window));
  flush_main_loop ();

  ck_g_object_final_unref (app);
  bt_edit_teardown ();
}

static void
case_teardown (void)
{
}

//-- tests

// test lifecycle/refcounts
START_TEST (test_bt_change_log_create_and_destroy)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");

  GST_INFO ("-- act --");
  BtChangeLog *cl = bt_change_log_new ();

  GST_INFO ("-- assert --");
  ck_assert (cl != NULL);

  GST_INFO ("-- cleanup --");
  g_object_unref (cl);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_change_log_initial_undo_redo_state)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtChangeLog *cl = bt_change_log_new ();
  gboolean can_undo, can_redo;

  GST_INFO ("-- act --");
  g_object_get (cl, "can-undo", &can_undo, "can-redo", &can_redo, NULL);

  GST_INFO ("-- assert --");
  ck_assert (!can_undo);
  ck_assert (!can_redo);

  GST_INFO ("-- cleanup --");
  g_object_unref (cl);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_change_log_undo_redo_state_after_single_change)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtChangeLog *cl = bt_change_log_new ();
  BtTestChangeLogger *tcl = bt_test_change_logger_new ();
  gboolean can_undo, can_redo;

  GST_INFO ("-- act --");
  make_change (cl, tcl, "set_val 5", "set_val 0");
  g_object_get (cl, "can-undo", &can_undo, "can-redo", &can_redo, NULL);

  GST_INFO ("-- assert --");
  ck_assert (can_undo);
  ck_assert (!can_redo);

  GST_INFO ("-- cleanup --");
  g_object_unref (tcl);
  g_object_unref (cl);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_change_log_undo_redo_state_single_change_after_undo)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtChangeLog *cl = bt_change_log_new ();
  BtTestChangeLogger *tcl = bt_test_change_logger_new ();
  gboolean can_undo, can_redo;

  GST_INFO ("-- act --");
  make_change (cl, tcl, "set_val 5", "set_val 0");
  bt_change_log_undo (cl);
  g_object_get (cl, "can-undo", &can_undo, "can-redo", &can_redo, NULL);

  GST_INFO ("-- assert --");
  ck_assert (!can_undo);
  ck_assert (can_redo);

  GST_INFO ("-- cleanup --");
  g_object_unref (tcl);
  g_object_unref (cl);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_change_log_undo_redo_state_single_change_after_redo)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtChangeLog *cl = bt_change_log_new ();
  BtTestChangeLogger *tcl = bt_test_change_logger_new ();
  gboolean can_undo, can_redo;

  GST_INFO ("-- act --");
  make_change (cl, tcl, "set_val 5", "set_val 0");
  bt_change_log_undo (cl);
  bt_change_log_redo (cl);
  g_object_get (cl, "can-undo", &can_undo, "can-redo", &can_redo, NULL);

  GST_INFO ("-- assert --");
  ck_assert (can_undo);
  ck_assert (!can_redo);

  GST_INFO ("-- cleanup --");
  g_object_unref (tcl);
  g_object_unref (cl);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_change_log_undo_redo_state_double_change_after_undo)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtChangeLog *cl = bt_change_log_new ();
  BtTestChangeLogger *tcl = bt_test_change_logger_new ();
  gboolean can_undo, can_redo;

  GST_INFO ("-- act --");
  make_change (cl, tcl, "set_val 5", "set_val 0");
  make_change (cl, tcl, "set_val 10", "set_val 0");
  bt_change_log_undo (cl);
  g_object_get (cl, "can-undo", &can_undo, "can-redo", &can_redo, NULL);

  GST_INFO ("-- assert --");
  ck_assert (can_undo);
  ck_assert (can_redo);

  GST_INFO ("-- cleanup --");
  g_object_unref (tcl);
  g_object_unref (cl);
  BT_TEST_END;
}
END_TEST


// test truncating the undo/redo stack
START_TEST (test_bt_change_log_undo_redo_state_stack_trunc)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtChangeLog *cl = bt_change_log_new ();
  BtTestChangeLogger *tcl = bt_test_change_logger_new ();
  gboolean can_undo, can_redo;

  GST_INFO ("-- act --");
  make_change (cl, tcl, "set_val 5", "set_val 0");
  bt_change_log_undo (cl);
  make_change (cl, tcl, "set_val 10", "set_val 0");
  g_object_get (cl, "can-undo", &can_undo, "can-redo", &can_redo, NULL);

  GST_INFO ("-- assert --");
  ck_assert (can_undo);
  ck_assert (!can_redo);

  GST_INFO ("-- cleanup --");
  g_object_unref (tcl);
  g_object_unref (cl);
  BT_TEST_END;
}
END_TEST

// test single undo/redo actions
START_TEST (test_bt_change_log_single_change_after_undo)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtChangeLog *cl = bt_change_log_new ();
  BtTestChangeLogger *tcl = bt_test_change_logger_new ();

  GST_INFO ("-- act --");
  make_change (cl, tcl, "set_val 5", "set_val 0");
  bt_change_log_undo (cl);

  GST_INFO ("-- assert --");
  ck_assert_int_eq (tcl->val, 0);

  GST_INFO ("-- cleanup --");
  g_object_unref (tcl);
  g_object_unref (cl);
  BT_TEST_END;
}
END_TEST

// test single undo/redo actions
START_TEST (test_bt_change_log_single_change_after_redo)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtChangeLog *cl = bt_change_log_new ();
  BtTestChangeLogger *tcl = bt_test_change_logger_new ();

  GST_INFO ("-- act --");
  make_change (cl, tcl, "set_val 5", "set_val 0");
  bt_change_log_undo (cl);
  bt_change_log_redo (cl);

  GST_INFO ("-- assert --");
  ck_assert_int_eq (tcl->val, 5);

  GST_INFO ("-- cleanup --");
  g_object_unref (tcl);
  g_object_unref (cl);
  BT_TEST_END;
}
END_TEST

// test double undo/redo actions
START_TEST (test_bt_change_log_two_changes)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtChangeLog *cl = bt_change_log_new ();
  BtTestChangeLogger *tcl = bt_test_change_logger_new ();

  /* act (make 2 changes) */
  make_change (cl, tcl, "set_val 5", "set_val 0");
  make_change (cl, tcl, "set_val 10", "set_val 5");

  // undo & verify
  bt_change_log_undo (cl);
  ck_assert_int_eq (tcl->val, 5);

  bt_change_log_undo (cl);
  ck_assert_int_eq (tcl->val, 0);

  // redo & verify
  bt_change_log_redo (cl);
  ck_assert_int_eq (tcl->val, 5);

  bt_change_log_redo (cl);
  ck_assert_int_eq (tcl->val, 10);

  GST_INFO ("-- cleanup --");
  g_object_unref (tcl);
  g_object_unref (cl);
  BT_TEST_END;
}
END_TEST

// test single and then double undo/redo actions
START_TEST (test_bt_change_log_single_then_double_change)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtChangeLog *cl = bt_change_log_new ();
  BtTestChangeLogger *tcl = bt_test_change_logger_new ();

  // make 1 change
  make_change (cl, tcl, "set_val 2", "set_val 0");

  // undo & verify
  bt_change_log_undo (cl);
  ck_assert_int_eq (tcl->val, 0);

  // redo & verify
  bt_change_log_redo (cl);
  ck_assert_int_eq (tcl->val, 2);

  // make 2 changes
  make_change (cl, tcl, "set_val 5", "set_val 2");
  make_change (cl, tcl, "set_val 10", "set_val 5");

  // undo & verify
  bt_change_log_undo (cl);
  ck_assert_int_eq (tcl->val, 5);

  bt_change_log_undo (cl);
  ck_assert_int_eq (tcl->val, 2);

  // redo & verify
  bt_change_log_redo (cl);
  ck_assert_int_eq (tcl->val, 5);

  bt_change_log_redo (cl);
  ck_assert_int_eq (tcl->val, 10);

  GST_INFO ("-- cleanup --");
  g_object_unref (tcl);
  g_object_unref (cl);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_change_log_group)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtChangeLog *cl = bt_change_log_new ();
  BtTestChangeLogger *tcl = bt_test_change_logger_new ();

  // make 1 change group
  bt_change_log_start_group (cl);
  make_change (cl, tcl, "new_data 2", "new_data 0");
  make_change (cl, tcl, "inc_data 0", "dec_data 0");
  bt_change_log_end_group (cl);

  // undo & verify
  bt_change_log_undo (cl);
  ck_assert (tcl->data == NULL);
  ck_assert_int_eq (tcl->data_size, 0);

  // redo & verify
  bt_change_log_redo (cl);
  ck_assert (tcl->data != NULL);
  ck_assert_int_eq (tcl->data_size, 2);
  ck_assert_int_eq (tcl->data[0], 1);

  GST_INFO ("-- cleanup --");
  g_object_unref (tcl);
  g_object_unref (cl);
  BT_TEST_END;
}
END_TEST


START_TEST (test_bt_change_log_nested_groups)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtChangeLog *cl = bt_change_log_new ();
  BtTestChangeLogger *tcl = bt_test_change_logger_new ();

  // make 1 change group
  bt_change_log_start_group (cl);
  make_change (cl, tcl, "new_data 2", "new_data 0");
  make_change (cl, tcl, "inc_data 0", "dec_data 0");
  bt_change_log_start_group (cl);
  make_change (cl, tcl, "inc_data 1", "dec_data 1");
  bt_change_log_end_group (cl);
  bt_change_log_end_group (cl);

  // undo & verify
  bt_change_log_undo (cl);
  ck_assert (tcl->data == NULL);
  ck_assert_int_eq (tcl->data_size, 0);

  // redo & verify
  bt_change_log_redo (cl);
  ck_assert (tcl->data != NULL);
  ck_assert_int_eq (tcl->data_size, 2);
  ck_assert_int_eq (tcl->data[0], 1);
  ck_assert_int_eq (tcl->data[1], 1);

  GST_INFO ("-- cleanup --");
  g_object_unref (tcl);
  g_object_unref (cl);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_change_log_recover)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtChangeLog *cl = bt_change_log_new ();
  gchar *log_name = g_build_filename (g_get_tmp_dir (), "bt-crash.log", NULL);
  gchar content[] =
      PACKAGE " edit journal : " PACKAGE_VERSION "\n"
      "\n"
      "BtMainPageMachines::add_machine 0,\"synth\",\"simsyn\"\n"
      "BtMainPageMachines::set_machine_property \"synth\",\"ypos\",\"-0.3\"\n"
      "BtMainPageMachines::set_machine_property \"synth\",\"xpos\",\"-0.4\"\n";
  g_file_set_contents (log_name, content, strlen (content), NULL);

  GST_INFO ("-- act --");
  gboolean res = bt_change_log_recover (cl, log_name);

  GST_INFO ("-- assert --");
  ck_assert (res);

  GST_INFO ("-- cleanup --");
  flush_main_loop ();
  g_unlink (log_name);
  g_free (log_name);
  g_object_unref (cl);
  BT_TEST_END;
}
END_TEST

TCase *
bt_change_log_example_case (void)
{
  TCase *tc = tcase_create ("BtChangeLogExamples");

  tcase_add_test (tc, test_bt_change_log_create_and_destroy);
  tcase_add_test (tc, test_bt_change_log_initial_undo_redo_state);
  tcase_add_test (tc, test_bt_change_log_undo_redo_state_after_single_change);
  tcase_add_test (tc,
      test_bt_change_log_undo_redo_state_single_change_after_undo);
  tcase_add_test (tc,
      test_bt_change_log_undo_redo_state_single_change_after_redo);
  tcase_add_test (tc,
      test_bt_change_log_undo_redo_state_double_change_after_undo);
  tcase_add_test (tc, test_bt_change_log_undo_redo_state_stack_trunc);
  tcase_add_test (tc, test_bt_change_log_single_change_after_undo);
  tcase_add_test (tc, test_bt_change_log_single_change_after_redo);
  tcase_add_test (tc, test_bt_change_log_two_changes);
  tcase_add_test (tc, test_bt_change_log_single_then_double_change);
  tcase_add_test (tc, test_bt_change_log_group);
  tcase_add_test (tc, test_bt_change_log_nested_groups);
  tcase_add_test (tc, test_bt_change_log_recover);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
