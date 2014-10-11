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

static void
test_bt_audio_session_singleton (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtAudioSession *session1 = bt_audio_session_new ();

  GST_INFO ("-- act --");
  BtAudioSession *session2 = bt_audio_session_new ();

  GST_INFO ("-- assert --");
  fail_unless (session1 == session2, NULL);

  GST_INFO ("-- cleanup --");
  g_object_unref (session2);
  g_object_checked_unref (session1);
  BT_TEST_END;
}

;


TCase *
bt_audio_session_example_case (void)
{
  TCase *tc = tcase_create ("BtAudioSessionExamples");

  tcase_add_test (tc, test_bt_audio_session_singleton);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
