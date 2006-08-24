/* $Id: t-core.c,v 1.10 2006-08-24 20:00:55 ensonic Exp $
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


// test if the normal init call works with commandline arguments
START_TEST(test_btcore_init0) {
  bt_init(&test_argc,&test_argvptr);
}
END_TEST

// test if the init call handles correct null pointers
START_TEST(test_btcore_init1) {
  bt_init(NULL,NULL);
}
END_TEST

TCase *bt_core_test_case(void) {
  TCase *tc = tcase_create("BtCoreTests");

  tcase_add_test(tc,test_btcore_init0);
  tcase_add_test(tc,test_btcore_init1);
  return(tc);
}
