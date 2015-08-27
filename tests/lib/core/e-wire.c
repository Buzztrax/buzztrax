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

static void
test_bt_wire_can_link (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachine *gen =
      BT_MACHINE (bt_source_machine_new (song, "gen", "audiotestsrc", 0L,
          NULL));
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (song, "master", NULL));

  /* act & assert */
  fail_unless (bt_wire_can_link (gen, sink));

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}

static void
test_bt_wire_new (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachine *gen =
      BT_MACHINE (bt_source_machine_new (song, "gen", "audiotestsrc", 0L,
          NULL));
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (song, "master", NULL));

  GST_INFO ("-- act --");
  GError *err = NULL;
  BtWire *wire = bt_wire_new (song, gen, sink, &err);

  GST_INFO ("-- assert --");
  fail_unless (wire != NULL, NULL);
  fail_unless (err == NULL, NULL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}

static void
test_bt_wire_pretty_name (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachine *src =
      BT_MACHINE (bt_source_machine_new (song, "audiotestsrc", "audiotestsrc",
          0L, NULL));
  BtMachine *proc = BT_MACHINE (bt_processor_machine_new (song, "volume",
          "volume", 0L, NULL));
  BtWire *wire = bt_wire_new (song, src, proc, NULL);

  GST_INFO ("-- act --");

  GST_INFO ("-- assert --");
  ck_assert_gobject_str_eq (wire, "pretty-name", "audiotestsrc -> volume");

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}

static void
test_bt_wire_persistence (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachine *src =
      BT_MACHINE (bt_source_machine_new (song, "audiotestsrc", "audiotestsrc",
          0L, NULL));
  BtMachine *proc = BT_MACHINE (bt_processor_machine_new (song, "volume",
          "volume", 0L, NULL));
  BtWire *wire = bt_wire_new (song, src, proc, NULL);

  GST_INFO ("-- act --");
  xmlNodePtr parent = xmlNewNode (NULL, XML_CHAR_PTR ("buzztrax"));
  xmlNodePtr node = bt_persistence_save (BT_PERSISTENCE (wire), parent);

  GST_INFO ("-- assert --");
  fail_unless (node != NULL, NULL);
  ck_assert_str_eq ((gchar *) node->name, "wire");
  fail_unless (node->children != NULL, NULL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}


TCase *
bt_wire_example_case (void)
{
  TCase *tc = tcase_create ("BtWireExamples");

  tcase_add_test (tc, test_bt_wire_can_link);
  tcase_add_test (tc, test_bt_wire_new);
  tcase_add_test (tc, test_bt_wire_pretty_name);
  tcase_add_test (tc, test_bt_wire_persistence);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
