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
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "master";
  bt_sink_machine_new (&cparams
                       , NULL);
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

START_TEST (test_bt_wire_can_link)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  BtMachine *gen =
      BT_MACHINE (bt_source_machine_new (&cparams, "audiotestsrc", 0L,
          NULL));
  cparams.id = "master";
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (&cparams, NULL));

  /* act & assert */
  fail_unless (bt_wire_can_link (gen, sink));

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_wire_new)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  BtMachine *gen =
      BT_MACHINE (bt_source_machine_new (&cparams, "audiotestsrc", 0L,
          NULL));
  cparams.id = "master";
  BtMachine *sink = BT_MACHINE (bt_sink_machine_new (&cparams, NULL));

  GST_INFO ("-- act --");
  GError *err = NULL;
  BtWire *wire = bt_wire_new (song, gen, sink, &err);

  GST_INFO ("-- assert --");
  ck_assert (wire != NULL);
  ck_assert (err == NULL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_wire_pretty_name)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "audiotestsrc";
  BtMachine *src =
      BT_MACHINE (bt_source_machine_new (&cparams, "audiotestsrc", 0L,
          NULL));
  cparams.id = "volume";
  BtMachine *proc = BT_MACHINE (bt_processor_machine_new (&cparams,
          "volume", 0L, NULL));
  BtWire *wire = bt_wire_new (song, src, proc, NULL);

  GST_INFO ("-- act --");

  GST_INFO ("-- assert --");
  ck_assert_gobject_str_eq (wire, "pretty-name", "audiotestsrc -> volume");

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_wire_pretty_name_gets_updated)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "audiotestsrc";
  BtMachine *src =
      BT_MACHINE (bt_source_machine_new (&cparams, "audiotestsrc", 0L,
          NULL));
  cparams.id = "volume";
  BtMachine *proc = BT_MACHINE (bt_processor_machine_new (&cparams,
          "volume", 0L, NULL));
  BtWire *wire = bt_wire_new (song, src, proc, NULL);

  GST_INFO ("-- act --");
  g_object_set (src, "id", "beep", NULL);

  GST_INFO ("-- assert --");
  ck_assert_gobject_str_eq (wire, "pretty-name",
      "beep (audiotestsrc) -> volume");

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_wire_persistence)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "audiotestsrc";
  BtMachine *src =
      BT_MACHINE (bt_source_machine_new (&cparams, "audiotestsrc", 0L,
          NULL));
  cparams.id = "volume";
  BtMachine *proc = BT_MACHINE (bt_processor_machine_new (&cparams,
          "volume", 0L, NULL));
  BtWire *wire = bt_wire_new (song, src, proc, NULL);

  GST_INFO ("-- act --");
  xmlNodePtr parent = xmlNewNode (NULL, XML_CHAR_PTR ("buzztrax"));
  xmlNodePtr node = bt_persistence_save (BT_PERSISTENCE (wire), parent, NULL);

  GST_INFO ("-- assert --");
  ck_assert (node != NULL);
  ck_assert_str_eq ((gchar *) node->name, "wire");
  ck_assert (node->children != NULL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST


TCase *
bt_wire_example_case (void)
{
  TCase *tc = tcase_create ("BtWireExamples");

  tcase_add_test (tc, test_bt_wire_can_link);
  tcase_add_test (tc, test_bt_wire_new);
  tcase_add_test (tc, test_bt_wire_pretty_name);
  tcase_add_test (tc, test_bt_wire_pretty_name_gets_updated);
  tcase_add_test (tc, test_bt_wire_persistence);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
