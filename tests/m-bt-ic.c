/* Buzztrax
 * Copyright (C) 2011 Buzztrax team <buzztrax-devel@lists.sf.net>
 *
 * interaction controller library unit tests
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

#define BT_CHECK

#include "bt-check.h"
#include "../src/lib/ic/ic.h"

GST_DEBUG_CATEGORY (GST_CAT_DEFAULT);
GST_DEBUG_CATEGORY_EXTERN (btic_debug);

extern Suite *bt_device_suite (void);
extern Suite *bt_ic_suite (void);
extern Suite *bt_learn_suite (void);
extern Suite *bt_registry_suite (void);

gchar *test_argv[] = { "check_buzzard" };

gchar **test_argvptr = test_argv;
gint test_argc = G_N_ELEMENTS (test_argv);

/* start the test run */
gint
main (gint argc, gchar ** argv)
{
  gint nf;
  SRunner *sr;

  g_type_init ();
  setup_log_base (argc, argv);
  setup_log_capture ();

  gst_init (NULL, NULL);
  btic_init (&test_argc, &test_argvptr);
  bt_check_init ();

  // set this to e.g. DEBUG to see more from gst in the log
  gst_debug_set_default_threshold (GST_LEVEL_DEBUG);
  //g_log_set_always_fatal(g_log_set_always_fatal(G_LOG_FATAL_MASK)|G_LOG_LEVEL_CRITICAL);

  sr = srunner_create (bt_ic_suite ());
  srunner_add_suite (sr, bt_device_suite ());
  srunner_add_suite (sr, bt_learn_suite ());
  srunner_add_suite (sr, bt_registry_suite ());
  srunner_run_all (sr, CK_NORMAL);
  nf = srunner_ntests_failed (sr);
  srunner_free (sr);

  //g_mem_profile();

  return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
