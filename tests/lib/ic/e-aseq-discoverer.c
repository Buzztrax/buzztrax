/* Buzztrax
 * Copyright (C) 2014 Buzztrax team <buzztrax-devel@buzztrax.org>
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

#include "m-bt-ic.h"
#if USE_ALSA
#include <alsa/asoundlib.h>
#endif

//-- globals

static BtIcRegistry *registry;

//-- fixtures

static void
case_setup (void)
{
  BT_CASE_START;
}

static void
test_setup (void)
{
  registry = btic_registry_new ();
}

static void
test_teardown (void)
{
  g_object_checked_unref (registry);
}

static void
case_teardown (void)
{
}

//-- tests

static void
test_btic_device_discovered (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  snd_seq_t *seq;
  fail_unless (snd_seq_open (&seq, "default", SND_SEQ_OPEN_DUPLEX, 0) >= 0);
  fail_unless (snd_seq_set_client_name (seq, PACKAGE "-tests") >= 0);

  GST_INFO ("-- act --");
  fail_unless (snd_seq_create_simple_port (seq, "test",
          SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
          SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION) >= 0);
  // TODO(ensonic): we don't receive the event in the discoverer :/
  /*check_run_main_loop_for_usec (1000);

     BtIcDevice *device = btic_registry_find_device_by_name (
     "alsa sequencer: test");

     GST_INFO ("-- assert --");
     fail_unless (device != NULL, NULL);

     GST_INFO ("-- cleanup --");
     g_object_unref (device);
   */
  snd_seq_close (seq);
  BT_TEST_END;
}

TCase *
bt_aseq_discoverer_example_case (void)
{
  TCase *tc = tcase_create ("BticAseqDiscovererExamples");

  tcase_add_test (tc, test_btic_device_discovered);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
