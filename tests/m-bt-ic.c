/* Buzztrax
 * Copyright (C) 2011 Buzztrax team <buzztrax-devel@buzztrax.org>
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
#include <stdlib.h>

GST_DEBUG_CATEGORY (GST_CAT_DEFAULT);
GST_DEBUG_CATEGORY_EXTERN (btic_debug);

gchar *test_argv[] = { "check_buzztrax" };

gchar **test_argvptr = test_argv;
gint test_argc = G_N_ELEMENTS (test_argv);

BT_TEST_SUITE_E ("BticDevice", bt_device);
BT_TEST_SUITE_T_E ("Btic", bt_ic);
BT_TEST_SUITE_E ("BticLearn", bt_learn);
BT_TEST_SUITE_T_E ("BticRegistry", bt_registry);
#if USE_ALSA
BT_TEST_SUITE_E ("BticAseqDiscoverer", bt_aseq_discoverer);
#endif

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
  btic_init (&test_argc, &test_argvptr);

  sr = srunner_create (bt_ic_suite ());
  srunner_add_suite (sr, bt_device_suite ());
  srunner_add_suite (sr, bt_learn_suite ());
  srunner_add_suite (sr, bt_registry_suite ());
#if USE_ALSA
  srunner_add_suite (sr, bt_aseq_discoverer_suite ());
#endif
  // srunner_set_xml (sr, get_suite_log_filename ("xml"));
  srunner_set_tap (sr, get_suite_log_filename ("tap"));
  srunner_run_all (sr, CK_NORMAL);
  nf = srunner_ntests_failed (sr);
  srunner_free (sr);
  collect_logs (nf == 0);

  //g_mem_profile();

  return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
