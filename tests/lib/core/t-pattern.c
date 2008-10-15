/* $Id$
 *
 * Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
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

#include "m-bt-core.h"

//-- globals

//-- fixtures

static void test_setup(void) {
  bt_core_setup();
  GST_INFO("================================================================================");
}

static void test_teardown(void) {
  bt_core_teardown();
}

//-- tests

/*
 * try creating a pattern for a NULL machine, with an invalid id and an invalid
 * name
 */
BT_START_TEST(test_btpattern_obj1) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtPattern *pattern=NULL;

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  /* create a new song */
  song=bt_song_new(app);

  check_init_error_trapp("bt_pattern_new","BT_IS_MACHINE(machine)");
  pattern=bt_pattern_new(song,"pattern-id","pattern-name",1L,NULL);
  fail_unless(check_has_error_trapped(), NULL);
  fail_unless(pattern == NULL, NULL);

  check_init_error_trapp("bt_pattern_new","BT_IS_STRING(id)");
  pattern=bt_pattern_new(song,NULL,"pattern-name",1L,NULL);
  fail_unless(check_has_error_trapped(), NULL);
  fail_unless(pattern == NULL, NULL);

  check_init_error_trapp("bt_pattern_new","BT_IS_STRING(name)");
  pattern=bt_pattern_new(song,"pattern-id",NULL,1L,NULL);
  fail_unless(check_has_error_trapped(), NULL);
  fail_unless(pattern == NULL, NULL);

  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

TCase *bt_pattern_test_case(void) {
  TCase *tc = tcase_create("BtPatternTests");

  tcase_add_test(tc,test_btpattern_obj1);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
