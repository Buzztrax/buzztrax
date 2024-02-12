/* Buzztrax
 * Copyright (C) 2016 Stefan Sauer <ensonic@users.sf.net>
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

#include "bt-check.h"
#include <stdlib.h>

GST_DEBUG_CATEGORY (GST_CAT_DEFAULT);

BT_TEST_SUITE_T_E ("BmlCore", bml_core);
BT_TEST_SUITE_E ("BmlClass", bml_class);

/* start the test run */
gint
main (gint argc, gchar ** argv)
{
  gint nf;
  SRunner *sr;

  setup_log_base (argc, argv);
  setup_log_capture ();

  bt_check_init ();

  sr = srunner_create (bml_core_suite ());
  srunner_add_suite (sr, bml_class_suite ());
  // srunner_set_xml (sr, get_suite_log_filename ("xml"));
  srunner_set_tap (sr, get_suite_log_filename ("tap"));
  srunner_run_all (sr, CK_NORMAL);
  nf = srunner_ntests_failed (sr);
  srunner_free (sr);

  collect_logs (nf == 0);

  return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
