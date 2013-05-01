/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@lists.sf.net>
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
  g_object_checked_unref (song);
  g_object_checked_unref (app);
}

static void
case_teardown (void)
{
}


//-- tests

static void
test_bt_setup_properties (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  GObject *setup = check_gobject_get_object_property (song, "setup");

  /* act & assert */
  fail_unless (check_gobject_properties (setup), NULL);

  /* cleanup */
  BT_TEST_END;
}

/* create a new setup with NULL for song object */
static void
test_bt_setup_new_null_song (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* act */
  BtSetup *setup = bt_setup_new (NULL);

  /* assert */
  fail_unless (setup != NULL, NULL);

  /* cleanup */
  g_object_unref (setup);
  BT_TEST_END;
}

/* add the same machine twice to the setup */
static void
test_bt_setup_add_machine_twice (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztrax-test-mono-source", 0, NULL));

  /* act & assert */
  fail_if (bt_setup_add_machine (setup, machine), NULL);

  /* cleanup */
  g_object_unref (setup);
  BT_TEST_END;
}

/* add the same wire twice */
static void
test_bt_setup_add_wire_twice (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  BtMachine *source = BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztrax-test-mono-source", 0L, NULL));
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (song, "sink", NULL));
  BtWire *wire = bt_wire_new (song, source, sink, NULL);

  /* act & assert */
  fail_if (bt_setup_add_wire (setup, wire), NULL);

  /* cleanup */
  g_object_unref (setup);
  BT_TEST_END;
}

/* call bt_setup_add_machine with NULL object for self */
static void
test_bt_setup_obj4 (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  check_init_error_trapp ("bt_setup_add_machine", "BT_IS_SETUP (self)");

  /* act */
  bt_setup_add_machine (NULL, NULL);

  /* assert */
  fail_unless (check_has_error_trapped (), NULL);

  /* cleanup */
  BT_TEST_END;
}

/* call bt_setup_add_machine with NULL object for machine */
static void
test_bt_setup_obj5 (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  check_init_error_trapp ("bt_setup_add_machine", "BT_IS_MACHINE (machine)");

  /* act */
  bt_setup_add_machine (setup, NULL);

  /* assert */
  fail_unless (check_has_error_trapped (), NULL);

  /* cleanup */
  g_object_unref (setup);
  BT_TEST_END;
}

/* call bt_setup_add_wire with NULL object for self */
static void
test_bt_setup_obj6 (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  check_init_error_trapp ("bt_setup_add_wire", "BT_IS_SETUP (self)");

  /* act */
  bt_setup_add_wire (NULL, NULL);

  /* assert */
  fail_unless (check_has_error_trapped (), NULL);

  /* cleanup */
  BT_TEST_END;
}

/* call bt_setup_add_wire with NULL object for wire */
static void
test_bt_setup_obj7 (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  check_init_error_trapp ("bt_setup_add_wire", "BT_IS_WIRE (wire)");

  /* act */
  bt_setup_add_wire (setup, NULL);

  /* assert */
  fail_unless (check_has_error_trapped (), NULL);

  /* cleanup */
  g_object_unref (setup);
  BT_TEST_END;
}

/* call bt_setup_get_machine_by_id with NULL object for self */
static void
test_bt_setup_obj8 (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  check_init_error_trapp ("bt_setup_get_machine_by_id", "BT_IS_SETUP (self)");

  /* act */
  bt_setup_get_machine_by_id (NULL, NULL);

  /* assert */
  fail_unless (check_has_error_trapped (), NULL);

  /* cleanup */
  BT_TEST_END;
}

/* call bt_setup_get_machine_by_id with NULL object for id */
static void
test_bt_setup_obj9 (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  check_init_error_trapp ("bt_setup_get_machine_by_id", NULL);

  /* act */
  bt_setup_get_machine_by_id (setup, NULL);

  /* assert */
  fail_unless (check_has_error_trapped (), NULL);

  /* cleanup */
  g_object_unref (setup);
  BT_TEST_END;
}

/*
* call bt_setup_get_wire_by_src_machine with NULL for setup parameter
*/
static void
test_bt_setup_get_wire_by_src_machine1 (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  check_init_error_trapp ("bt_setup_get_wire_by_src_machine",
      "BT_IS_SETUP (self)");

  /* act */
  bt_setup_get_wire_by_src_machine (NULL, NULL);

  /* assert */
  fail_unless (check_has_error_trapped (), NULL);

  /* cleanup */
  BT_TEST_END;
}

/* call bt_setup_get_wire_by_src_machine with NULL for machine parameter */
static void
test_bt_setup_get_wire_by_src_machine2 (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  check_init_error_trapp ("bt_setup_get_wire_by_src_machine",
      "BT_IS_MACHINE (src)");

  /* act */
  bt_setup_get_wire_by_src_machine (setup, NULL);

  /* assert */
  fail_unless (check_has_error_trapped (), NULL);

  /* cleanup */
  g_object_unref (setup);
  BT_TEST_END;
}

/* get wires by source machine with NULL for setup */
static void
test_bt_setup_get_wires_by_src_machine1 (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  BtSourceMachine *src_machine =
      bt_source_machine_new (song, "src", "buzztrax-test-mono-source", 0L,
      NULL);
  check_init_error_trapp ("bt_setup_get_wires_by_src_machine",
      "BT_IS_SETUP (self)");

  /* act */
  bt_setup_get_wires_by_src_machine (NULL, BT_MACHINE (src_machine));

  /* assert */
  fail_unless (check_has_error_trapped (), NULL);

  /* cleanup */
  g_object_unref (setup);
  BT_TEST_END;
}

/* get wires by source machine with NULL for machine */
static void
test_bt_setup_get_wires_by_src_machine2 (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  check_init_error_trapp ("bt_setup_get_wires_by_src_machine",
      "BT_IS_MACHINE (src)");

  /* act */
  bt_setup_get_wires_by_src_machine (setup, NULL);
  fail_unless (check_has_error_trapped (), NULL);

  /* cleanup */
  g_object_unref (setup);
  BT_TEST_END;
}

/* get wires by source machine with a not added machine */
static void
test_bt_setup_get_wires_by_src_machine3 (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (song, "src",
          "buzztrax-test-mono-source", 0L, NULL));
  bt_setup_remove_machine (setup, machine);

  /* act & assert */
  fail_if (bt_setup_get_wires_by_src_machine (setup, machine), NULL);

  /* cleanup */
  g_object_unref (setup);
  BT_TEST_END;
}

/* call bt_setup_get_wire_by_dst_machine with NULL for setup parameter */
static void
test_bt_setup_get_wire_by_dst_machine1 (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  check_init_error_trapp ("bt_setup_get_wire_by_dst_machine",
      "BT_IS_SETUP (self)");

  /* act */
  bt_setup_get_wire_by_dst_machine (NULL, NULL);

  /* assert */
  fail_unless (check_has_error_trapped (), NULL);

  /* cleanup */
  g_object_unref (setup);
  BT_TEST_END;
}

/* call bt_setup_get_wire_by_dst_machine with NULL for machine parameter */
static void
test_bt_setup_get_wire_by_dst_machine2 (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  check_init_error_trapp ("bt_setup_get_wire_by_dst_machine",
      "BT_IS_MACHINE (dst)");

  /* act */
  bt_setup_get_wire_by_dst_machine (setup, NULL);

  /* assert */
  fail_unless (check_has_error_trapped (), NULL);

  /* cleanup */
  g_object_unref (setup);
  BT_TEST_END;
}

/* get wires by destination machine with NULL for setup */
static void
test_bt_setup_get_wires_by_dst_machine1 (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  BtMachine *machine = BT_MACHINE (bt_sink_machine_new (song, "dst", NULL));
  check_init_error_trapp ("bt_setup_get_wires_by_dst_machine",
      "BT_IS_SETUP (self)");

  /* act */
  bt_setup_get_wires_by_dst_machine (NULL, machine);

  /* assert */
  fail_unless (check_has_error_trapped (), NULL);

  /* cleanup */
  g_object_unref (setup);
  BT_TEST_END;
}

/* get wires by sink machine with NULL for machine */
static void
test_bt_setup_get_wires_by_dst_machine2 (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  check_init_error_trapp ("bt_setup_get_wires_by_dst_machine",
      "BT_IS_MACHINE (dst)");

  /* act */
  bt_setup_get_wires_by_dst_machine (setup, NULL);

  /* assert */
  fail_unless (check_has_error_trapped (), NULL);

  /* cleanup */
  g_object_unref (setup);
  BT_TEST_END;
}

/* get wires by sink machine with a not added machine */
static void
test_bt_setup_get_wires_by_dst_machine3 (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  BtMachine *machine = BT_MACHINE (bt_sink_machine_new (song, "dst", NULL));
  bt_setup_remove_machine (setup, machine);

  /* act & assert */
  fail_if (bt_setup_get_wires_by_dst_machine (setup, machine), NULL);

  /* cleanup */
  g_object_unref (setup);
  BT_TEST_END;
}

/* remove a machine from setup with NULL pointer for setup */
static void
test_bt_setup_remove_machine_null_setup (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  check_init_error_trapp ("bt_setup_remove_machine", NULL);

  /* act */
  bt_setup_remove_machine (NULL, NULL);

  /* assert */
  fail_unless (check_has_error_trapped (), NULL);

  /* cleanup */
  BT_TEST_END;
}

/* remove a wire from setup with NULL pointer for setup */
static void
test_bt_setup_remove_wire_null_setup (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  check_init_error_trapp ("bt_setup_remove_wire", NULL);

  /* act */
  bt_setup_remove_wire (NULL, NULL);

  /* assert */
  fail_unless (check_has_error_trapped (), NULL);

  /* cleanup */
  BT_TEST_END;
}

/* remove a machine from setup with NULL pointer for machine */
static void
test_bt_setup_remove_null_machine (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  check_init_error_trapp ("bt_setup_remove_machine", NULL);

  /* act */
  bt_setup_remove_machine (setup, NULL);

  /* assert */
  fail_unless (check_has_error_trapped (), NULL);

  /* cleanup */
  g_object_unref (setup);
  BT_TEST_END;
}

/* remove a wire from setup with NULL pointer for wire */
static void
test_bt_setup_remove_null_wire (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  check_init_error_trapp ("bt_setup_remove_wire", NULL);

  /* act */
  bt_setup_remove_wire (setup, NULL);

  /* assert */
  fail_unless (check_has_error_trapped (), NULL);

  /* cleanup */
  g_object_unref (setup);
  BT_TEST_END;
}

/* remove a machine from setup with a machine witch is never added */
static void
test_bt_setup_remove_machine_twice (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  BtMachine *gen = BT_MACHINE (bt_source_machine_new (song, "src",
          "buzztrax-test-mono-source", 0L, NULL));
  gst_object_ref (gen);
  bt_setup_remove_machine (setup, gen);
  check_init_error_trapp ("bt_setup_remove_machine", "machine is not in setup");

  /* act */
  bt_setup_remove_machine (setup, gen);

  /* assert */
  fail_unless (check_has_error_trapped (), NULL);

  /* cleanup */
  gst_object_unref (gen);
  g_object_unref (setup);
  BT_TEST_END;
}

/* remove a wire from setup with a wire which is not added */
static void
test_bt_setup_remove_wire_twice (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  BtMachine *gen = BT_MACHINE (bt_source_machine_new (song, "src",
          "buzztrax-test-mono-source", 0L, NULL));
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (song, "dst", NULL));
  BtWire *wire = bt_wire_new (song, gen, sink, NULL);
  gst_object_ref (wire);
  bt_setup_remove_wire (setup, wire);
  check_init_error_trapp ("bt_setup_remove_wire", "wire is not in setup");

  /* act */
  bt_setup_remove_wire (setup, wire);

  /* assert */
  fail_unless (check_has_error_trapped (), NULL);

  /* cleanup */
  gst_object_unref (wire);
  g_object_unref (setup);
  BT_TEST_END;
}

/* add wire(src,dst) and wire(dst,src) to setup. This should fail (cycle). */
static void
test_bt_setup_wire_cycle1 (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  BtMachine *src =
      BT_MACHINE (bt_processor_machine_new (song, "src", "volume", 0L, NULL));
  BtMachine *dst =
      BT_MACHINE (bt_processor_machine_new (song, "src", "volume", 0L, NULL));
  bt_wire_new (song, src, dst, NULL);

  /* act */
  GError *err = NULL;
  BtWire *wire2 = bt_wire_new (song, dst, src, &err);

  /* assert */
  fail_unless (wire2 != NULL, NULL);
  fail_unless (err != NULL, NULL);

  /* cleanup */
  g_object_unref (setup);
  BT_TEST_END;
}

// test graph cycles
static void
test_bt_setup_wire_cycle2 (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  BtMachine *elem1 =
      BT_MACHINE (bt_processor_machine_new (song, "src1", "volume", 0L, NULL));
  BtMachine *elem2 =
      BT_MACHINE (bt_processor_machine_new (song, "src2", "volume", 0L, NULL));
  BtMachine *elem3 =
      BT_MACHINE (bt_processor_machine_new (song, "src3", "volume", 0L, NULL));
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (song, "sink", NULL));
  bt_wire_new (song, elem3, sink, NULL);
  bt_wire_new (song, elem1, elem2, NULL);
  bt_wire_new (song, elem2, elem3, NULL);

  /* act */
  GError *err = NULL;
  BtWire *wire3 = bt_wire_new (song, elem3, elem1, &err);

  /* assert */
  fail_unless (wire3 != NULL, NULL);
  fail_unless (err != NULL, NULL);

  /* cleanup */
  g_object_unref (setup);
  BT_TEST_END;
}

TCase *
bt_setup_test_case (void)
{
  TCase *tc = tcase_create ("BtSetupTests");

  tcase_add_test (tc, test_bt_setup_properties);
  tcase_add_test (tc, test_bt_setup_new_null_song);
  tcase_add_test (tc, test_bt_setup_add_machine_twice);
  tcase_add_test (tc, test_bt_setup_add_wire_twice);
  tcase_add_test (tc, test_bt_setup_obj4);
  tcase_add_test (tc, test_bt_setup_obj5);
  tcase_add_test (tc, test_bt_setup_obj6);
  tcase_add_test (tc, test_bt_setup_obj7);
  tcase_add_test (tc, test_bt_setup_obj8);
  tcase_add_test (tc, test_bt_setup_obj9);
  tcase_add_test (tc, test_bt_setup_get_wire_by_src_machine1);
  tcase_add_test (tc, test_bt_setup_get_wire_by_src_machine2);
  tcase_add_test (tc, test_bt_setup_get_wires_by_src_machine1);
  tcase_add_test (tc, test_bt_setup_get_wires_by_src_machine2);
  tcase_add_test (tc, test_bt_setup_get_wires_by_src_machine3);
  tcase_add_test (tc, test_bt_setup_get_wire_by_dst_machine1);
  tcase_add_test (tc, test_bt_setup_get_wire_by_dst_machine2);
  tcase_add_test (tc, test_bt_setup_get_wires_by_dst_machine1);
  tcase_add_test (tc, test_bt_setup_get_wires_by_dst_machine2);
  tcase_add_test (tc, test_bt_setup_get_wires_by_dst_machine3);
  tcase_add_test (tc, test_bt_setup_remove_machine_null_setup);
  tcase_add_test (tc, test_bt_setup_remove_wire_null_setup);
  tcase_add_test (tc, test_bt_setup_remove_null_machine);
  tcase_add_test (tc, test_bt_setup_remove_null_wire);
  tcase_add_test (tc, test_bt_setup_remove_machine_twice);
  tcase_add_test (tc, test_bt_setup_remove_wire_twice);
  tcase_add_test (tc, test_bt_setup_wire_cycle1);
  tcase_add_test (tc, test_bt_setup_wire_cycle2);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
