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

//-- fixtures

static void
case_setup (void)
{
  BT_CASE_START;
}

static void
test_setup (void)
{
}

static void
test_teardown (void)
{
}

static void
case_teardown (void)
{
}


//-- tests

START_TEST (test_bt_audio_session_singleton)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtAudioSession *session1 = bt_audio_session_new ();

  GST_INFO ("-- act --");
  BtAudioSession *session2 = bt_audio_session_new ();

  GST_INFO ("-- assert --");
  ck_assert (session1 == session2);

  GST_INFO ("-- cleanup --");
  g_object_unref (session2);
  ck_g_object_final_unref (session1);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_audio_session_no_session_element_for_fakesink)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtAudioSession *session = bt_audio_session_new ();

  GST_INFO ("-- act --");
  GstElement *e = bt_audio_session_get_sink_for ("fakesink", NULL);

  GST_INFO ("-- assert --");
  ck_assert (e == NULL);

  GST_INFO ("-- cleanup --");
  ck_g_object_final_unref (session);
  BT_TEST_END;
}
END_TEST


TCase *
bt_audio_session_example_case (void)
{
  TCase *tc = tcase_create ("BtAudioSessionExamples");

  tcase_add_test (tc, test_bt_audio_session_singleton);
  tcase_add_test (tc, test_bt_audio_session_no_session_element_for_fakesink);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
