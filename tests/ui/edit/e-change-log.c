/* Buzztard
 * Copyright (C) 2010 Buzztard team <buzztard-devel@lists.sf.net>
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "m-bt-edit.h"

//-- test class

typedef struct
{
  const GObject parent;

  BtChangeLog *change_log;

  gint val;
} BtTestChangeLogger;

typedef struct
{
  const GObjectClass parent;

} BtTestChangeLoggerClass;

static gboolean
bt_test_change_logger_change (const BtChangeLogger * owner, const gchar * data)
{
  BtTestChangeLogger *self = (BtTestChangeLogger *) owner;
  gboolean res = FALSE;

  if (!strncmp (data, "set_val ", 8)) {
    self->val = atoi (&data[8]);
    GST_DEBUG ("val=%d", self->val);
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
bt_test_change_logger_dispose (GObject * object)
{
  BtTestChangeLogger *self = (BtTestChangeLogger *) object;

  g_object_unref (self->change_log);
}

static void
bt_test_change_logger_init (BtTestChangeLogger * self)
{
  self->change_log = bt_change_log_new ();
}

static void
bt_test_change_logger_class_init (BtTestChangeLoggerClass * const klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = bt_test_change_logger_dispose;
}

G_DEFINE_TYPE_WITH_CODE (BtTestChangeLogger, bt_test_change_logger,
    G_TYPE_OBJECT, G_IMPLEMENT_INTERFACE (BT_TYPE_CHANGE_LOGGER,
        bt_test_change_logger_interface_init));

static BtTestChangeLogger *
bt_test_change_logger_new (void)
{
  return ((BtTestChangeLogger *)
      g_object_new (bt_test_change_logger_get_type (), NULL));
}

//-- globals

static BtEditApplication *app;
static BtMainWindow *main_window;

//-- fixtures

static void
case_setup (void)
{
  GST_INFO
      ("================================================================================");
}

static void
test_setup (void)
{
  bt_edit_setup ();
  app = bt_edit_application_new ();
  bt_edit_application_new_song (app);
  g_object_get (app, "main-window", &main_window, NULL);

  while (gtk_events_pending ())
    gtk_main_iteration ();
}

static void
test_teardown (void)
{
  gtk_widget_destroy (GTK_WIDGET (main_window));
  while (gtk_events_pending ())
    gtk_main_iteration ();

  g_object_checked_unref (app);
  bt_edit_teardown ();
}

static void
case_teardown (void)
{
}

//-- tests

// test lifecycle/refcounts
static void
test_bt_change_log_create_and_destroy (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */

  /* act */
  BtChangeLog *cl = bt_change_log_new ();

  /* assert */
  fail_unless (cl != NULL, NULL);

  /* cleanup */
  g_object_unref (cl);
  BT_TEST_END;
}

static void
test_bt_change_log_initial_undo_redo_state (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtChangeLog *cl = bt_change_log_new ();
  gboolean can_undo, can_redo;

  /* act */
  g_object_get (cl, "can-undo", &can_undo, "can-redo", &can_redo, NULL);

  /* assert */
  fail_unless (!can_undo, NULL);
  fail_unless (!can_redo, NULL);

  /* cleanup */
  g_object_unref (cl);
  BT_TEST_END;
}

static void
test_bt_change_log_undo_redo_state_after_single_change (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtChangeLog *cl = bt_change_log_new ();
  BtTestChangeLogger *tcl = bt_test_change_logger_new ();
  gboolean can_undo, can_redo;

  /* act */
  bt_change_log_add (tcl->change_log, BT_CHANGE_LOGGER (tcl),
      g_strdup ("set_val 0"), g_strdup ("set_val 5"));
  g_object_get (cl, "can-undo", &can_undo, "can-redo", &can_redo, NULL);

  /* assert */
  fail_unless (can_undo, NULL);
  fail_unless (!can_redo, NULL);

  /* cleanup */
  g_object_unref (tcl);
  g_object_unref (cl);
  BT_TEST_END;
}

static void
test_bt_change_log_undo_redo_state_single_change_after_undo (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtChangeLog *cl = bt_change_log_new ();
  BtTestChangeLogger *tcl = bt_test_change_logger_new ();
  gboolean can_undo, can_redo;

  /* act */
  bt_change_log_add (tcl->change_log, BT_CHANGE_LOGGER (tcl),
      g_strdup ("set_val 0"), g_strdup ("set_val 5"));
  bt_change_log_undo (tcl->change_log);
  g_object_get (cl, "can-undo", &can_undo, "can-redo", &can_redo, NULL);

  /* assert */
  fail_unless (!can_undo, NULL);
  fail_unless (can_redo, NULL);

  /* cleanup */
  g_object_unref (tcl);
  g_object_unref (cl);
  BT_TEST_END;
}

static void
test_bt_change_log_undo_redo_state_single_change_after_redo (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtChangeLog *cl = bt_change_log_new ();
  BtTestChangeLogger *tcl = bt_test_change_logger_new ();
  gboolean can_undo, can_redo;

  /* act */
  bt_change_log_add (tcl->change_log, BT_CHANGE_LOGGER (tcl),
      g_strdup ("set_val 0"), g_strdup ("set_val 5"));
  bt_change_log_undo (tcl->change_log);
  bt_change_log_redo (tcl->change_log);
  g_object_get (cl, "can-undo", &can_undo, "can-redo", &can_redo, NULL);

  /* assert */
  fail_unless (can_undo, NULL);
  fail_unless (!can_redo, NULL);

  /* cleanup */
  g_object_unref (tcl);
  g_object_unref (cl);
  BT_TEST_END;
}

static void
test_bt_change_log_undo_redo_state_double_change_after_undo (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtChangeLog *cl = bt_change_log_new ();
  BtTestChangeLogger *tcl = bt_test_change_logger_new ();
  gboolean can_undo, can_redo;

  /* act */
  bt_change_log_add (tcl->change_log, BT_CHANGE_LOGGER (tcl),
      g_strdup ("set_val 0"), g_strdup ("set_val 5"));
  bt_change_log_add (tcl->change_log, BT_CHANGE_LOGGER (tcl),
      g_strdup ("set_val 5"), g_strdup ("set_val 10"));
  bt_change_log_undo (tcl->change_log);
  g_object_get (cl, "can-undo", &can_undo, "can-redo", &can_redo, NULL);

  /* assert */
  fail_unless (can_undo, NULL);
  fail_unless (can_redo, NULL);

  /* cleanup */
  g_object_unref (tcl);
  g_object_unref (cl);
  BT_TEST_END;
}


// test truncating the undo/redo stack
static void
test_bt_change_log_undo_redo_state_stack_trunc (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtChangeLog *cl = bt_change_log_new ();
  BtTestChangeLogger *tcl = bt_test_change_logger_new ();
  gboolean can_undo, can_redo;

  /* act */
  bt_change_log_add (tcl->change_log, BT_CHANGE_LOGGER (tcl),
      g_strdup ("set_val 0"), g_strdup ("set_val 5"));
  bt_change_log_undo (tcl->change_log);
  bt_change_log_add (tcl->change_log, BT_CHANGE_LOGGER (tcl),
      g_strdup ("set_val 0"), g_strdup ("set_val 10"));
  g_object_get (cl, "can-undo", &can_undo, "can-redo", &can_redo, NULL);

  /* assert */
  fail_unless (can_undo, NULL);
  fail_unless (!can_redo, NULL);

  /* cleanup */
  g_object_unref (tcl);
  g_object_unref (cl);
  BT_TEST_END;
}

// test single undo/redo actions
static void
test_bt_change_log_single_change_after_undo (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtChangeLog *cl = bt_change_log_new ();
  BtTestChangeLogger *tcl = bt_test_change_logger_new ();

  /* act */
  tcl->val = 5;
  bt_change_log_add (tcl->change_log, BT_CHANGE_LOGGER (tcl),
      g_strdup ("set_val 0"), g_strdup ("set_val 5"));
  bt_change_log_undo (tcl->change_log);

  /* assert */
  GST_DEBUG ("val=%d", tcl->val);
  fail_unless (tcl->val == 0, NULL);

  /* cleanup */
  g_object_unref (tcl);
  g_object_unref (cl);
  BT_TEST_END;
}

// test single undo/redo actions
static void
test_bt_change_log_single_change_after_redo (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtChangeLog *cl = bt_change_log_new ();
  BtTestChangeLogger *tcl = bt_test_change_logger_new ();

  /* act */
  tcl->val = 5;
  bt_change_log_add (tcl->change_log, BT_CHANGE_LOGGER (tcl),
      g_strdup ("set_val 0"), g_strdup ("set_val 5"));
  bt_change_log_undo (tcl->change_log);
  bt_change_log_redo (tcl->change_log);

  /* assert */
  GST_DEBUG ("val=%d", tcl->val);
  fail_unless (tcl->val == 5, NULL);

  /* cleanup */
  g_object_unref (tcl);
  g_object_unref (cl);
  BT_TEST_END;
}

// test double undo/redo actions
static void
test_bt_change_log_two_changes (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtChangeLog *cl = bt_change_log_new ();
  BtTestChangeLogger *tcl = bt_test_change_logger_new ();

  /* act (make 2 changes) */
  tcl->val = 5;
  bt_change_log_add (tcl->change_log, BT_CHANGE_LOGGER (tcl),
      g_strdup ("set_val 0"), g_strdup ("set_val 5"));

  tcl->val = 10;
  bt_change_log_add (tcl->change_log, BT_CHANGE_LOGGER (tcl),
      g_strdup ("set_val 5"), g_strdup ("set_val 10"));

  // undo & verify
  bt_change_log_undo (tcl->change_log);
  GST_DEBUG ("val=%d", tcl->val);
  fail_unless (tcl->val == 5, NULL);

  bt_change_log_undo (tcl->change_log);
  GST_DEBUG ("val=%d", tcl->val);
  fail_unless (tcl->val == 0, NULL);

  // redo & verify
  bt_change_log_redo (tcl->change_log);
  GST_DEBUG ("val=%d", tcl->val);
  fail_unless (tcl->val == 5, NULL);

  bt_change_log_redo (tcl->change_log);
  GST_DEBUG ("val=%d", tcl->val);
  fail_unless (tcl->val == 10, NULL);

  /* cleanup */
  g_object_unref (tcl);
  g_object_unref (cl);
  BT_TEST_END;
}

// test single and then double undo/redo actions
static void
test_bt_change_log_single_then_double_change (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtChangeLog *cl = bt_change_log_new ();
  BtTestChangeLogger *tcl = bt_test_change_logger_new ();

  // make 1 change
  tcl->val = 2;
  bt_change_log_add (tcl->change_log, BT_CHANGE_LOGGER (tcl),
      g_strdup ("set_val 0"), g_strdup ("set_val 2"));

  // undo & verify
  bt_change_log_undo (tcl->change_log);
  GST_DEBUG ("val=%d", tcl->val);
  fail_unless (tcl->val == 0, NULL);

  // redo & verify
  bt_change_log_redo (tcl->change_log);
  GST_DEBUG ("val=%d", tcl->val);
  fail_unless (tcl->val == 2, NULL);

  // make 2 changes
  tcl->val = 5;
  bt_change_log_add (tcl->change_log, BT_CHANGE_LOGGER (tcl),
      g_strdup ("set_val 2"), g_strdup ("set_val 5"));

  tcl->val = 10;
  bt_change_log_add (tcl->change_log, BT_CHANGE_LOGGER (tcl),
      g_strdup ("set_val 5"), g_strdup ("set_val 10"));

  // undo & verify
  bt_change_log_undo (tcl->change_log);
  GST_DEBUG ("val=%d", tcl->val);
  fail_unless (tcl->val == 5, NULL);

  bt_change_log_undo (tcl->change_log);
  GST_DEBUG ("val=%d", tcl->val);
  fail_unless (tcl->val == 2, NULL);

  // redo & verify
  bt_change_log_redo (tcl->change_log);
  GST_DEBUG ("val=%d", tcl->val);
  fail_unless (tcl->val == 5, NULL);

  bt_change_log_redo (tcl->change_log);
  GST_DEBUG ("val=%d", tcl->val);
  fail_unless (tcl->val == 10, NULL);

  /* cleanup */
  g_object_unref (tcl);
  g_object_unref (cl);
  BT_TEST_END;
}

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
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
