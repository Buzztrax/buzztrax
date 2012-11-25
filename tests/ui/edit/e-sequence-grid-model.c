/* Buzztard
 * Copyright (C) 2012 Buzztard team <buzztard-devel@lists.sf.net>
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

//-- globals

static BtEditApplication *app;
static BtSong *song;
static BtSequence *sequence;
static BtSongInfo *song_info;
static BtMainWindow *main_window;

//-- fixtures

static void
case_setup (void)
{
  BT_CASE_START;
}

static void
test_setup (void)
{
  bt_edit_setup ();
  app = bt_edit_application_new ();
  bt_edit_application_new_song (app);
  g_object_get (app, "song", &song, "main-window", &main_window, NULL);
  g_object_get (song, "sequence", &sequence, "song-info", &song_info, NULL);

  flush_main_loop ();
}

static void
test_teardown (void)
{
  g_object_unref (song_info);
  g_object_unref (sequence);
  g_object_unref (song);

  flush_main_loop ();

  gtk_widget_destroy (GTK_WIDGET (main_window));
  flush_main_loop ();

  g_object_checked_unref (app);
  bt_edit_teardown ();
}

static void
case_teardown (void)
{
}

//-- tests

static void
test_bt_sequence_grid_model_create (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */

  /* act */
  BtSequenceGridModel *model = bt_sequence_grid_model_new (sequence, song_info,
      16);

  /* assert */
  fail_unless (model != NULL, NULL);

  /* cleanup */
  g_object_unref (model);
  BT_TEST_END;
}

TCase *
bt_sequence_grid_model_example_case (void)
{
  TCase *tc = tcase_create ("BtSequenceGridModelExamples");

  tcase_add_test (tc, test_bt_sequence_grid_model_create);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
