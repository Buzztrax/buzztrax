/* GStreamer
 * Copyright (C) 2012 Stefan Sauer <ensonic@users.sf.net>
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
GST_DEBUG_CATEGORY_EXTERN (gst_buzztrax_debug);

gchar *test_argv[] = { "check_buzztrax" };

gchar **test_argvptr = test_argv;
gint test_argc = G_N_ELEMENTS (test_argv);

BT_TEST_SUITE_T ("GstElement", gst_buzztrax_elements);
BT_TEST_SUITE_T_E ("GstBtToneConversion", gst_buzztrax_toneconversion);

/* start the test run */
gint
main (gint argc, gchar ** argv)
{
  gint nf;
  SRunner *sr;

  setup_log_base (argc, argv);
  setup_log_capture ();
  gst_init (NULL, NULL);

  bt_check_init ();
  bt_init (&test_argc, &test_argvptr);

  sr = srunner_create (gst_buzztrax_toneconversion_suite ());
  srunner_add_suite (sr, gst_buzztrax_elements_suite ());
  srunner_run_all (sr, CK_NORMAL);
  nf = srunner_ntests_failed (sr);
  srunner_free (sr);

  bt_deinit ();
  collect_logs (nf == 0);

  return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
