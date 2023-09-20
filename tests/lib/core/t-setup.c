/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@buzztrax.org>
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
  ck_g_object_final_unref (song);
  ck_g_object_final_unref (app);
}

static void
case_teardown (void)
{
}


//-- tests

START_TEST (test_bt_setup_properties)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  GObject *setup = check_gobject_get_object_property (song, "setup");

  /* act & assert */
  ck_assert (check_gobject_properties (setup));

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

/* create a new setup with NULL for song object */
START_TEST (test_bt_setup_new_null_song)
{
  BT_TEST_START;
  GST_INFO ("-- act --");
  BtSetup *setup = bt_setup_new (NULL);

  GST_INFO ("-- assert --");
  ck_assert (setup != NULL);

  GST_INFO ("-- cleanup --");
  g_object_unref (setup);
  BT_TEST_END;
}
END_TEST

/* add the same machine twice to the setup */
START_TEST (test_bt_setup_add_machine_twice)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  
  BtMachineConstructorParams cparams;
  cparams.id = "gen";
  cparams.song = song;
  
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0, NULL));

  /* act & assert */
  ck_assert (!bt_setup_add_machine (setup, machine));

  GST_INFO ("-- cleanup --");
  g_object_unref (setup);
  BT_TEST_END;
}
END_TEST

/* add the same wire twice */
START_TEST (test_bt_setup_add_wire_twice)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  
  BtMachineConstructorParams cparams;
  cparams.id = "gen";
  cparams.song = song;
  
  BtMachine *source = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0L, NULL));

  cparams.id = "sink";
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (&cparams, NULL));
  BtWire *wire = bt_wire_new (song, source, sink, NULL);

  /* act & assert */
  ck_assert (!bt_setup_add_wire (setup, wire));

  GST_INFO ("-- cleanup --");
  g_object_unref (setup);
  BT_TEST_END;
}
END_TEST

/* call bt_setup_add_machine with NULL object for self */
START_TEST (test_bt_setup_obj4)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  check_init_error_trapp ("bt_setup_add_machine", "BT_IS_SETUP (self)");

  GST_INFO ("-- act --");
  bt_setup_add_machine (NULL, NULL);

  GST_INFO ("-- assert --");
  ck_assert (check_has_error_trapped ());

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

/* call bt_setup_add_machine with NULL object for machine */
START_TEST (test_bt_setup_obj5)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  check_init_error_trapp ("bt_setup_add_machine", "BT_IS_MACHINE (machine)");

  GST_INFO ("-- act --");
  bt_setup_add_machine (setup, NULL);

  GST_INFO ("-- assert --");
  ck_assert (check_has_error_trapped ());

  GST_INFO ("-- cleanup --");
  g_object_unref (setup);
  BT_TEST_END;
}
END_TEST

/* call bt_setup_add_wire with NULL object for self */
START_TEST (test_bt_setup_obj6)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  check_init_error_trapp ("bt_setup_add_wire", "BT_IS_SETUP (self)");

  GST_INFO ("-- act --");
  bt_setup_add_wire (NULL, NULL);

  GST_INFO ("-- assert --");
  ck_assert (check_has_error_trapped ());

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

/* call bt_setup_add_wire with NULL object for wire */
START_TEST (test_bt_setup_obj7)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  check_init_error_trapp ("bt_setup_add_wire", "BT_IS_WIRE (wire)");

  GST_INFO ("-- act --");
  bt_setup_add_wire (setup, NULL);

  GST_INFO ("-- assert --");
  ck_assert (check_has_error_trapped ());

  GST_INFO ("-- cleanup --");
  g_object_unref (setup);
  BT_TEST_END;
}
END_TEST

/* call bt_setup_get_machine_by_id with NULL object for self */
START_TEST (test_bt_setup_obj8)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  check_init_error_trapp ("bt_setup_get_machine_by_id", "BT_IS_SETUP (self)");

  GST_INFO ("-- act --");
  bt_setup_get_machine_by_id (NULL, NULL);

  GST_INFO ("-- assert --");
  ck_assert (check_has_error_trapped ());

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

/* call bt_setup_get_machine_by_id with NULL object for id */
START_TEST (test_bt_setup_obj9)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  check_init_error_trapp ("bt_setup_get_machine_by_id", NULL);

  GST_INFO ("-- act --");
  bt_setup_get_machine_by_id (setup, NULL);

  GST_INFO ("-- assert --");
  ck_assert (check_has_error_trapped ());

  GST_INFO ("-- cleanup --");
  g_object_unref (setup);
  BT_TEST_END;
}
END_TEST

/*
* call bt_setup_get_wire_by_src_machine with NULL for setup parameter
*/
START_TEST (test_bt_setup_get_wire_by_src_machine1)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  check_init_error_trapp ("bt_setup_get_wire_by_src_machine",
      "BT_IS_SETUP (self)");

  GST_INFO ("-- act --");
  bt_setup_get_wire_by_src_machine (NULL, NULL);

  GST_INFO ("-- assert --");
  ck_assert (check_has_error_trapped ());

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

/* call bt_setup_get_wire_by_src_machine with NULL for machine parameter */
START_TEST (test_bt_setup_get_wire_by_src_machine2)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  check_init_error_trapp ("bt_setup_get_wire_by_src_machine",
      "BT_IS_MACHINE (src)");

  GST_INFO ("-- act --");
  bt_setup_get_wire_by_src_machine (setup, NULL);

  GST_INFO ("-- assert --");
  ck_assert (check_has_error_trapped ());

  GST_INFO ("-- cleanup --");
  g_object_unref (setup);
  BT_TEST_END;
}
END_TEST

/* get wires by source machine with NULL for setup */
START_TEST (test_bt_setup_get_wires_by_src_machine1)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "src";
  
  BtSourceMachine *src_machine =
      bt_source_machine_new (&cparams, "buzztrax-test-mono-source", 0L,
      NULL);
  check_init_error_trapp ("bt_setup_get_wires_by_src_machine",
      "BT_IS_SETUP (self)");

  GST_INFO ("-- act --");
  bt_setup_get_wires_by_src_machine (NULL, BT_MACHINE (src_machine));

  GST_INFO ("-- assert --");
  ck_assert (check_has_error_trapped ());

  GST_INFO ("-- cleanup --");
  g_object_unref (setup);
  BT_TEST_END;
}
END_TEST

/* get wires by source machine with NULL for machine */
START_TEST (test_bt_setup_get_wires_by_src_machine2)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  check_init_error_trapp ("bt_setup_get_wires_by_src_machine",
      "BT_IS_MACHINE (src)");

  GST_INFO ("-- act --");
  bt_setup_get_wires_by_src_machine (setup, NULL);
  ck_assert (check_has_error_trapped ());

  GST_INFO ("-- cleanup --");
  g_object_unref (setup);
  BT_TEST_END;
}
END_TEST

/* get wires by source machine with a not added machine */
START_TEST (test_bt_setup_get_wires_by_src_machine3)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "src";
  
  BtSourceMachine *machine =
      bt_source_machine_new (&cparams, "buzztrax-test-mono-source", 0L,
      NULL);
  bt_setup_remove_machine (setup, BT_MACHINE (machine));

  /* act & assert */
  ck_assert (!bt_setup_get_wires_by_src_machine (setup, BT_MACHINE (machine)));

  GST_INFO ("-- cleanup --");
  g_object_unref (setup);
  BT_TEST_END;
}
END_TEST

/* call bt_setup_get_wire_by_dst_machine with NULL for setup parameter */
START_TEST (test_bt_setup_get_wire_by_dst_machine1)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  check_init_error_trapp ("bt_setup_get_wire_by_dst_machine",
      "BT_IS_SETUP (self)");

  GST_INFO ("-- act --");
  bt_setup_get_wire_by_dst_machine (NULL, NULL);

  GST_INFO ("-- assert --");
  ck_assert (check_has_error_trapped ());

  GST_INFO ("-- cleanup --");
  g_object_unref (setup);
  BT_TEST_END;
}
END_TEST

/* call bt_setup_get_wire_by_dst_machine with NULL for machine parameter */
START_TEST (test_bt_setup_get_wire_by_dst_machine2)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  check_init_error_trapp ("bt_setup_get_wire_by_dst_machine",
      "BT_IS_MACHINE (dst)");

  GST_INFO ("-- act --");
  bt_setup_get_wire_by_dst_machine (setup, NULL);

  GST_INFO ("-- assert --");
  ck_assert (check_has_error_trapped ());

  GST_INFO ("-- cleanup --");
  g_object_unref (setup);
  BT_TEST_END;
}
END_TEST

/* get wires by destination machine with NULL for setup */
START_TEST (test_bt_setup_get_wires_by_dst_machine1)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "dst";
  
  BtMachine *machine = BT_MACHINE (bt_sink_machine_new (&cparams, NULL));
  check_init_error_trapp ("bt_setup_get_wires_by_dst_machine",
      "BT_IS_SETUP (self)");

  GST_INFO ("-- act --");
  bt_setup_get_wires_by_dst_machine (NULL, machine);

  GST_INFO ("-- assert --");
  ck_assert (check_has_error_trapped ());

  GST_INFO ("-- cleanup --");
  g_object_unref (setup);
  BT_TEST_END;
}
END_TEST

/* get wires by sink machine with NULL for machine */
START_TEST (test_bt_setup_get_wires_by_dst_machine2)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  check_init_error_trapp ("bt_setup_get_wires_by_dst_machine",
      "BT_IS_MACHINE (dst)");

  GST_INFO ("-- act --");
  bt_setup_get_wires_by_dst_machine (setup, NULL);

  GST_INFO ("-- assert --");
  ck_assert (check_has_error_trapped ());

  GST_INFO ("-- cleanup --");
  g_object_unref (setup);
  BT_TEST_END;
}
END_TEST

/* get wires by sink machine with a not added machine */
START_TEST (test_bt_setup_get_wires_by_dst_machine3)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "dst";
  
  BtMachine *machine = BT_MACHINE (bt_sink_machine_new (&cparams, NULL));
  bt_setup_remove_machine (setup, machine);

  /* act & assert */
  ck_assert (!bt_setup_get_wires_by_dst_machine (setup, machine));

  GST_INFO ("-- cleanup --");
  g_object_unref (setup);
  BT_TEST_END;
}
END_TEST

/* remove a machine from setup with NULL pointer for setup */
START_TEST (test_bt_setup_remove_machine_null_setup)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  check_init_error_trapp ("bt_setup_remove_machine", NULL);

  GST_INFO ("-- act --");
  bt_setup_remove_machine (NULL, NULL);

  GST_INFO ("-- assert --");
  ck_assert (check_has_error_trapped ());

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

/* remove a wire from setup with NULL pointer for setup */
START_TEST (test_bt_setup_remove_wire_null_setup)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  check_init_error_trapp ("bt_setup_remove_wire", NULL);

  GST_INFO ("-- act --");
  bt_setup_remove_wire (NULL, NULL);

  GST_INFO ("-- assert --");
  ck_assert (check_has_error_trapped ());

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

/* remove a machine from setup with NULL pointer for machine */
START_TEST (test_bt_setup_remove_null_machine)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  check_init_error_trapp ("bt_setup_remove_machine", NULL);

  GST_INFO ("-- act --");
  bt_setup_remove_machine (setup, NULL);

  GST_INFO ("-- assert --");
  ck_assert (check_has_error_trapped ());

  GST_INFO ("-- cleanup --");
  g_object_unref (setup);
  BT_TEST_END;
}
END_TEST

/* remove a wire from setup with NULL pointer for wire */
START_TEST (test_bt_setup_remove_null_wire)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  check_init_error_trapp ("bt_setup_remove_wire", NULL);

  GST_INFO ("-- act --");
  bt_setup_remove_wire (setup, NULL);

  GST_INFO ("-- assert --");
  ck_assert (check_has_error_trapped ());

  GST_INFO ("-- cleanup --");
  g_object_unref (setup);
  BT_TEST_END;
}
END_TEST

/* remove a machine from setup with a machine witch is never added */
START_TEST (test_bt_setup_remove_machine_twice)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "src";
  BtMachine *gen = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0L, NULL));
  gst_object_ref (gen);
  bt_setup_remove_machine (setup, gen);
  check_init_error_trapp ("bt_setup_remove_machine", "machine is not in setup");

  GST_INFO ("-- act --");
  bt_setup_remove_machine (setup, gen);

  GST_INFO ("-- assert --");
  ck_assert (check_has_error_trapped ());

  GST_INFO ("-- cleanup --");
  gst_object_unref (gen);
  g_object_unref (setup);
  BT_TEST_END;
}
END_TEST

/* remove a wire from setup with a wire which is not added */
START_TEST (test_bt_setup_remove_wire_twice)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "src";
  
  BtMachine *gen = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0L, NULL));

  cparams.id = "dst";
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (&cparams, NULL));
  BtWire *wire = bt_wire_new (song, gen, sink, NULL);
  gst_object_ref (wire);
  bt_setup_remove_wire (setup, wire);
  check_init_error_trapp ("bt_setup_remove_wire", "wire is not in setup");

  GST_INFO ("-- act --");
  bt_setup_remove_wire (setup, wire);

  GST_INFO ("-- assert --");
  ck_assert (check_has_error_trapped ());

  GST_INFO ("-- cleanup --");
  gst_object_unref (wire);
  g_object_unref (setup);
  BT_TEST_END;
}
END_TEST

/**
 * Ensure that creating multiple machines with the same name results in the
 * correct unique name pattern.
 */
START_TEST (test_bt_setup_unique_machine_id)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");

  GST_INFO ("-- act --");
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "src1";
  
  BtMachine *elem1 =
      BT_MACHINE (bt_processor_machine_new (&cparams, "volume", 0L, NULL));
  
  cparams.id = bt_setup_get_unique_machine_id (setup, "src1");
  BtMachine *elem2 =
      BT_MACHINE (bt_processor_machine_new (&cparams, "volume", 0L, NULL));
  g_free (cparams.id);
  
  cparams.id = bt_setup_get_unique_machine_id (setup, "src1");
  BtMachine *elem3 =
      BT_MACHINE (bt_processor_machine_new (&cparams, "volume", 0L, NULL));
  g_free (cparams.id);

  GST_INFO ("-- assert --");
  gchar* id = NULL;
  
  g_object_get (G_OBJECT (elem1), "id", &id, NULL);
  ck_assert_str_eq_and_free (id, "src1");
  
  g_object_get (G_OBJECT (elem2), "id", &id, NULL);
  ck_assert_str_eq_and_free (id, "src1 00");

  g_object_get (G_OBJECT (elem3), "id", &id, NULL);
  ck_assert_str_eq_and_free (id, "src1 01");
  
  GST_INFO ("-- cleanup --");
  g_object_unref (setup);
  BT_TEST_END;
}
END_TEST

/* add wire(src,dst) and wire(dst,src) to setup. This should fail (cycle). */
START_TEST (test_bt_setup_wire_cycle1)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "src";
  
  BtMachine *src =
      BT_MACHINE (bt_processor_machine_new (&cparams, "volume", 0L, NULL));
  BtMachine *dst =
      BT_MACHINE (bt_processor_machine_new (&cparams, "volume", 0L, NULL));
  bt_wire_new (song, src, dst, NULL);

  GST_INFO ("-- act --");
  GError *err = NULL;
  BtWire *wire2 = bt_wire_new (song, dst, src, &err);

  GST_INFO ("-- assert --");
  ck_assert (wire2 != NULL);
  ck_assert (err != NULL);

  GST_INFO ("-- cleanup --");
  g_object_unref (setup);
  BT_TEST_END;
}
END_TEST

// test graph cycles
START_TEST (test_bt_setup_wire_cycle2)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSetup *setup =
      (BtSetup *) check_gobject_get_object_property (song, "setup");
  
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "src1";
  
  BtMachine *elem1 =
      BT_MACHINE (bt_processor_machine_new (&cparams, "volume", 0L, NULL));
  cparams.id = "src2";
  BtMachine *elem2 =
      BT_MACHINE (bt_processor_machine_new (&cparams, "volume", 0L, NULL));
  cparams.id = "src3";
  BtMachine *elem3 =
      BT_MACHINE (bt_processor_machine_new (&cparams, "volume", 0L, NULL));
  cparams.id = "sink";
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (&cparams, NULL));
  bt_wire_new (song, elem3, sink, NULL);
  bt_wire_new (song, elem1, elem2, NULL);
  bt_wire_new (song, elem2, elem3, NULL);

  GST_INFO ("-- act --");
  GError *err = NULL;
  BtWire *wire3 = bt_wire_new (song, elem3, elem1, &err);

  GST_INFO ("-- assert --");
  ck_assert (wire3 != NULL);
  ck_assert (err != NULL);

  GST_INFO ("-- cleanup --");
  g_object_unref (setup);
  BT_TEST_END;
}
END_TEST

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
  tcase_add_test (tc, test_bt_setup_unique_machine_id);
  tcase_add_test (tc, test_bt_setup_wire_cycle1);
  tcase_add_test (tc, test_bt_setup_wire_cycle2);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
