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

//-- fixtures

static void
test_setup (void)
{
  bt_edit_setup ();
}

static void
test_teardown (void)
{
  bt_edit_teardown ();
}

//-- tests

// test lifecycle/refcounts
BT_START_TEST (test_create_and_destroy1)
{
  BtChangeLog *cl;

  cl = bt_change_log_new ();
  fail_unless (cl != NULL, NULL);

  g_object_checked_unref (cl);
}

BT_END_TEST
BT_START_TEST (test_create_and_destroy2)
{
  BtEditApplication *app;
  BtMainWindow *main_window;
  BtChangeLog *cl;

  app = bt_edit_application_new ();
  GST_INFO ("app=%p, app->ref_ct=%d", app, G_OBJECT_REF_COUNT (app));
  fail_unless (app != NULL, NULL);

  cl = bt_change_log_new ();
  GST_INFO ("cl=%p, cl->ref_ct=%d", cl, G_OBJECT_REF_COUNT (cl));
  fail_unless (cl != NULL, NULL);

  // close window
  g_object_get (app, "main-window", &main_window, NULL);
  gtk_widget_destroy (GTK_WIDGET (main_window));
  while (gtk_events_pending ())
    gtk_main_iteration ();

  g_object_unref (cl);
  g_object_checked_unref (app);
}

BT_END_TEST
// test single undo/redo actions
BT_START_TEST (test_undo_redo_1)
{
  BtEditApplication *app;
  BtMainWindow *main_window;
  BtChangeLog *cl;
  BtTestChangeLogger *tcl;
  gboolean can_undo, can_redo;

  // testing the change log needs a active song as a context
  app = bt_edit_application_new ();
  GST_INFO ("back in test app=%p, app->ref_ct=%d", app,
      G_OBJECT_REF_COUNT (app));
  fail_unless (app != NULL, NULL);
  bt_edit_application_new_song (app);

  cl = bt_change_log_new ();
  fail_unless (cl != NULL, NULL);

  g_object_get (cl, "can-undo", &can_undo, "can-redo", &can_redo, NULL);
  fail_unless (!can_undo, NULL);
  fail_unless (!can_redo, NULL);

  // create a test instance that implements changelogger iface
  tcl = bt_test_change_logger_new ();
  fail_unless (tcl != NULL, NULL);

  // make 1 change
  tcl->val = 5;
  bt_change_log_add (tcl->change_log, BT_CHANGE_LOGGER (tcl),
      g_strdup ("set_val 0"), g_strdup ("set_val 5"));
  g_object_get (cl, "can-undo", &can_undo, "can-redo", &can_redo, NULL);
  fail_unless (can_undo, NULL);
  fail_unless (!can_redo, NULL);

  // undo & verify
  bt_change_log_undo (tcl->change_log);
  GST_DEBUG ("val=%d", tcl->val);
  fail_unless (tcl->val == 0, NULL);
  g_object_get (cl, "can-undo", &can_undo, "can-redo", &can_redo, NULL);
  fail_unless (!can_undo, NULL);
  fail_unless (can_redo, NULL);

  // redo & verify
  bt_change_log_redo (tcl->change_log);
  GST_DEBUG ("val=%d", tcl->val);
  fail_unless (tcl->val == 5, NULL);
  g_object_get (cl, "can-undo", &can_undo, "can-redo", &can_redo, NULL);
  fail_unless (can_undo, NULL);
  fail_unless (!can_redo, NULL);

  // close window
  g_object_get (app, "main-window", &main_window, NULL);
  gtk_widget_destroy (GTK_WIDGET (main_window));
  while (gtk_events_pending ())
    gtk_main_iteration ();

  g_object_unref (tcl);
  g_object_unref (cl);
  g_object_checked_unref (app);
}

BT_END_TEST
// test double undo/redo actions
BT_START_TEST (test_undo_redo_2)
{
  BtEditApplication *app;
  BtMainWindow *main_window;
  BtChangeLog *cl;
  BtTestChangeLogger *tcl;
  gboolean can_undo, can_redo;

  // testing the change log needs a active song as a context
  app = bt_edit_application_new ();
  GST_INFO ("back in test app=%p, app->ref_ct=%d", app,
      G_OBJECT_REF_COUNT (app));
  fail_unless (app != NULL, NULL);
  bt_edit_application_new_song (app);

  cl = bt_change_log_new ();
  fail_unless (cl != NULL, NULL);

  g_object_get (cl, "can-undo", &can_undo, "can-redo", &can_redo, NULL);
  fail_unless (!can_undo, NULL);
  fail_unless (!can_redo, NULL);

  // create a test instance that implements changelogger iface
  tcl = bt_test_change_logger_new ();
  fail_unless (tcl != NULL, NULL);

  // make 2 changes
  tcl->val = 5;
  bt_change_log_add (tcl->change_log, BT_CHANGE_LOGGER (tcl),
      g_strdup ("set_val 0"), g_strdup ("set_val 5"));
  g_object_get (cl, "can-undo", &can_undo, "can-redo", &can_redo, NULL);
  fail_unless (can_undo, NULL);
  fail_unless (!can_redo, NULL);

  tcl->val = 10;
  bt_change_log_add (tcl->change_log, BT_CHANGE_LOGGER (tcl),
      g_strdup ("set_val 5"), g_strdup ("set_val 10"));
  g_object_get (cl, "can-undo", &can_undo, "can-redo", &can_redo, NULL);
  fail_unless (can_undo, NULL);
  fail_unless (!can_redo, NULL);

  // undo & verify
  bt_change_log_undo (tcl->change_log);
  GST_DEBUG ("val=%d", tcl->val);
  fail_unless (tcl->val == 5, NULL);
  g_object_get (cl, "can-undo", &can_undo, "can-redo", &can_redo, NULL);
  fail_unless (can_undo, NULL);
  fail_unless (can_redo, NULL);

  bt_change_log_undo (tcl->change_log);
  GST_DEBUG ("val=%d", tcl->val);
  fail_unless (tcl->val == 0, NULL);
  g_object_get (cl, "can-undo", &can_undo, "can-redo", &can_redo, NULL);
  fail_unless (!can_undo, NULL);
  fail_unless (can_redo, NULL);

  // redo & verify
  bt_change_log_redo (tcl->change_log);
  GST_DEBUG ("val=%d", tcl->val);
  fail_unless (tcl->val == 5, NULL);
  g_object_get (cl, "can-undo", &can_undo, "can-redo", &can_redo, NULL);
  fail_unless (can_undo, NULL);
  fail_unless (can_redo, NULL);

  bt_change_log_redo (tcl->change_log);
  GST_DEBUG ("val=%d", tcl->val);
  fail_unless (tcl->val == 10, NULL);
  g_object_get (cl, "can-undo", &can_undo, "can-redo", &can_redo, NULL);
  fail_unless (can_undo, NULL);
  fail_unless (!can_redo, NULL);

  // close window
  g_object_get (app, "main-window", &main_window, NULL);
  gtk_widget_destroy (GTK_WIDGET (main_window));
  while (gtk_events_pending ())
    gtk_main_iteration ();

  g_object_unref (tcl);
  g_object_unref (cl);
  g_object_checked_unref (app);
}

BT_END_TEST
// test single and then double undo/redo actions
BT_START_TEST (test_undo_redo_3)
{
  BtEditApplication *app;
  BtMainWindow *main_window;
  BtChangeLog *cl;
  BtTestChangeLogger *tcl;
  gboolean can_undo, can_redo;

  // testing the change log needs a active song as a context
  app = bt_edit_application_new ();
  GST_INFO ("back in test app=%p, app->ref_ct=%d", app,
      G_OBJECT_REF_COUNT (app));
  fail_unless (app != NULL, NULL);
  bt_edit_application_new_song (app);

  cl = bt_change_log_new ();
  fail_unless (cl != NULL, NULL);

  g_object_get (cl, "can-undo", &can_undo, "can-redo", &can_redo, NULL);
  fail_unless (!can_undo, NULL);
  fail_unless (!can_redo, NULL);

  // create a test instance that implements changelogger iface
  tcl = bt_test_change_logger_new ();
  fail_unless (tcl != NULL, NULL);

  // make 1 change
  tcl->val = 2;
  bt_change_log_add (tcl->change_log, BT_CHANGE_LOGGER (tcl),
      g_strdup ("set_val 0"), g_strdup ("set_val 2"));
  g_object_get (cl, "can-undo", &can_undo, "can-redo", &can_redo, NULL);
  fail_unless (can_undo, NULL);
  fail_unless (!can_redo, NULL);

  // undo & verify
  bt_change_log_undo (tcl->change_log);
  GST_DEBUG ("val=%d", tcl->val);
  fail_unless (tcl->val == 0, NULL);
  g_object_get (cl, "can-undo", &can_undo, "can-redo", &can_redo, NULL);
  fail_unless (!can_undo, NULL);
  fail_unless (can_redo, NULL);

  // redo & verify
  bt_change_log_redo (tcl->change_log);
  GST_DEBUG ("val=%d", tcl->val);
  fail_unless (tcl->val == 2, NULL);
  g_object_get (cl, "can-undo", &can_undo, "can-redo", &can_redo, NULL);
  fail_unless (can_undo, NULL);
  fail_unless (!can_redo, NULL);

  // make 2 changes
  tcl->val = 5;
  bt_change_log_add (tcl->change_log, BT_CHANGE_LOGGER (tcl),
      g_strdup ("set_val 2"), g_strdup ("set_val 5"));
  g_object_get (cl, "can-undo", &can_undo, "can-redo", &can_redo, NULL);
  fail_unless (can_undo, NULL);
  fail_unless (!can_redo, NULL);

  tcl->val = 10;
  bt_change_log_add (tcl->change_log, BT_CHANGE_LOGGER (tcl),
      g_strdup ("set_val 5"), g_strdup ("set_val 10"));
  g_object_get (cl, "can-undo", &can_undo, "can-redo", &can_redo, NULL);
  fail_unless (can_undo, NULL);
  fail_unless (!can_redo, NULL);

  // undo & verify
  bt_change_log_undo (tcl->change_log);
  GST_DEBUG ("val=%d", tcl->val);
  fail_unless (tcl->val == 5, NULL);
  g_object_get (cl, "can-undo", &can_undo, "can-redo", &can_redo, NULL);
  fail_unless (can_undo, NULL);
  fail_unless (can_redo, NULL);

  bt_change_log_undo (tcl->change_log);
  GST_DEBUG ("val=%d", tcl->val);
  fail_unless (tcl->val == 2, NULL);
  g_object_get (cl, "can-undo", &can_undo, "can-redo", &can_redo, NULL);
  fail_unless (can_undo, NULL);
  fail_unless (can_redo, NULL);

  // redo & verify
  bt_change_log_redo (tcl->change_log);
  GST_DEBUG ("val=%d", tcl->val);
  fail_unless (tcl->val == 5, NULL);
  g_object_get (cl, "can-undo", &can_undo, "can-redo", &can_redo, NULL);
  fail_unless (can_undo, NULL);
  fail_unless (can_redo, NULL);

  bt_change_log_redo (tcl->change_log);
  GST_DEBUG ("val=%d", tcl->val);
  fail_unless (tcl->val == 10, NULL);
  g_object_get (cl, "can-undo", &can_undo, "can-redo", &can_redo, NULL);
  fail_unless (can_undo, NULL);
  fail_unless (!can_redo, NULL);

  // close window
  g_object_get (app, "main-window", &main_window, NULL);
  gtk_widget_destroy (GTK_WIDGET (main_window));
  while (gtk_events_pending ())
    gtk_main_iteration ();

  g_object_unref (tcl);
  g_object_unref (cl);
  g_object_checked_unref (app);
}

BT_END_TEST
// test truncating the undo/redo stack
BT_START_TEST (test_stack_trunc)
{
  BtEditApplication *app;
  BtMainWindow *main_window;
  BtChangeLog *cl;
  BtTestChangeLogger *tcl;
  gboolean can_undo, can_redo;

  // testing the change log needs a active song as a context
  app = bt_edit_application_new ();
  GST_INFO ("back in test app=%p, app->ref_ct=%d", app,
      G_OBJECT_REF_COUNT (app));
  fail_unless (app != NULL, NULL);
  bt_edit_application_new_song (app);

  cl = bt_change_log_new ();
  fail_unless (cl != NULL, NULL);

  g_object_get (cl, "can-undo", &can_undo, "can-redo", &can_redo, NULL);
  fail_unless (!can_undo, NULL);
  fail_unless (!can_redo, NULL);

  // create a test instance that implements changelogger iface
  tcl = bt_test_change_logger_new ();
  fail_unless (tcl != NULL, NULL);

  // make 1 change
  tcl->val = 5;
  bt_change_log_add (tcl->change_log, BT_CHANGE_LOGGER (tcl),
      g_strdup ("set_val 0"), g_strdup ("set_val 5"));
  g_object_get (cl, "can-undo", &can_undo, "can-redo", &can_redo, NULL);
  fail_unless (can_undo, NULL);
  fail_unless (!can_redo, NULL);

  // undo & verify
  bt_change_log_undo (tcl->change_log);
  GST_DEBUG ("val=%d", tcl->val);
  fail_unless (tcl->val == 0, NULL);
  g_object_get (cl, "can-undo", &can_undo, "can-redo", &can_redo, NULL);
  fail_unless (!can_undo, NULL);
  fail_unless (can_redo, NULL);

  // make 1 change
  tcl->val = 10;
  bt_change_log_add (tcl->change_log, BT_CHANGE_LOGGER (tcl),
      g_strdup ("set_val 0"), g_strdup ("set_val 10"));
  g_object_get (cl, "can-undo", &can_undo, "can-redo", &can_redo, NULL);
  fail_unless (can_undo, NULL);
  fail_unless (!can_redo, NULL);

  // close window
  g_object_get (app, "main-window", &main_window, NULL);
  gtk_widget_destroy (GTK_WIDGET (main_window));
  while (gtk_events_pending ())
    gtk_main_iteration ();

  g_object_unref (tcl);
  g_object_unref (cl);
  g_object_checked_unref (app);
}

BT_END_TEST TCase * bt_change_log_example_case (void)
{
  TCase *tc = tcase_create ("BtChangeLogExamples");

  tcase_add_test (tc, test_create_and_destroy1);
  tcase_add_test (tc, test_create_and_destroy2);
  tcase_add_test (tc, test_undo_redo_1);
  tcase_add_test (tc, test_undo_redo_2);
  tcase_add_test (tc, test_undo_redo_3);
  tcase_add_test (tc, test_stack_trunc);
  // we *must* use a checked fixture, as only this runs in the same context
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  return (tc);
}
