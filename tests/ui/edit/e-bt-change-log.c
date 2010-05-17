/* $Id$
 *
 * Buzztard
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

typedef struct {
  const GObject parent;
  
} BtTestChangeLogger;

typedef struct {
  const GObjectClass parent;
  
} BtTestChangeLoggerClass;

static gboolean bt_test_change_logger_change(const BtChangeLogger *owner,const gchar *data) {
  return TRUE;
}

static void bt_test_change_logger_interface_init(gpointer const g_iface, gconstpointer const iface_data) {
  BtChangeLoggerInterface * const iface = g_iface;

  iface->change = bt_test_change_logger_change;
}

static void bt_test_change_logger_init(BtTestChangeLogger *self) {
}

static void bt_test_change_logger_class_init(BtTestChangeLoggerClass * const klass) {
}

G_DEFINE_TYPE_WITH_CODE(BtTestChangeLogger, bt_test_change_logger, G_TYPE_OBJECT,
  G_IMPLEMENT_INTERFACE(BT_TYPE_CHANGE_LOGGER,
    bt_test_change_logger_interface_init));

static BtTestChangeLogger *bt_test_change_logger_new(void) {
  return((BtTestChangeLogger *)g_object_new(bt_test_change_logger_get_type(),NULL));
}

//-- globals

//-- fixtures

static void test_setup(void) {
  bt_edit_setup();
}

static void test_teardown(void) {
  bt_edit_teardown();
}

//-- tests

// test lifecycle/refcounts
BT_START_TEST(test_create_and_destry) {
  BtChangeLog *cl;

  cl=bt_change_log_new();
  fail_unless(cl != NULL, NULL);

  g_object_checked_unref(cl);
}
BT_END_TEST

// test undo/redo actions
BT_START_TEST(test_undo_redo) {
  BtChangeLog *cl;
  BtTestChangeLogger *tcl;
  gboolean can_undo,can_redo;

  cl=bt_change_log_new();
  fail_unless(cl != NULL, NULL);
  
  g_object_get(cl,"can-undo",&can_undo,"can-redo",&can_redo,NULL);
  fail_unless(!can_undo, NULL);
  fail_unless(!can_redo, NULL);
  
  // create a test instance that implements changelogger iface
  tcl=bt_test_change_logger_new();
  fail_unless(cl != NULL, NULL);

  // todo: make changes

  g_object_unref(tcl);
  g_object_checked_unref(cl);
}
BT_END_TEST

TCase *bt_change_log_example_case(void) {
  TCase *tc = tcase_create("BtChangeLogExamples");

  tcase_add_test(tc,test_create_and_destry);
  tcase_add_test(tc,test_undo_redo);
  // we *must* use a checked fixture, as only this runs in the same context
  tcase_add_checked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
