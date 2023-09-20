/* Buzztrax
 *
 * unit test for state changes on all elements
 *
 * Copyright (C) <2005> Thomas Vander Stichele <thomas at apestaart dot org>
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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "m-bt-gst.h"

//-- globals

static GList *elements = NULL;

//-- fixtures

static void
case_setup (void)
{
  GList *features, *f;
  GList *plugins, *p;
  gchar **ignorelist = NULL;
  const gchar *STATE_IGNORE_ELEMENTS = NULL;

  BT_CASE_START;
  GST_DEBUG ("getting elements for package %s", PACKAGE);
  STATE_IGNORE_ELEMENTS = g_getenv ("GST_STATE_IGNORE_ELEMENTS");
  if (!g_getenv ("GST_NO_STATE_IGNORE_ELEMENTS") && STATE_IGNORE_ELEMENTS) {
    GST_DEBUG ("Will ignore element factories: '%s'", STATE_IGNORE_ELEMENTS);
    ignorelist = g_strsplit (STATE_IGNORE_ELEMENTS, " ", 0);
  }

  plugins = gst_registry_get_plugin_list (gst_registry_get ());

  for (p = plugins; p; p = p->next) {
    GstPlugin *plugin = p->data;

    if (strcmp (gst_plugin_get_source (plugin), PACKAGE) != 0)
      continue;

    features =
        gst_registry_get_feature_list_by_plugin (gst_registry_get (),
        gst_plugin_get_name (plugin));

    for (f = features; f; f = f->next) {
      GstPluginFeature *feature = f->data;
      const gchar *name;
      gboolean ignore = FALSE;

      if (!GST_IS_ELEMENT_FACTORY (feature))
        continue;

      name = GST_OBJECT_NAME (feature);

      if (ignorelist) {
        gchar **s;

        for (s = ignorelist; s && *s; ++s) {
          if (g_str_has_prefix (name, *s)) {
            GST_DEBUG ("ignoring element %s", name);
            ignore = TRUE;
          }
        }
        if (ignore)
          continue;
      }

      GST_DEBUG ("adding element %s", name);
      elements = g_list_prepend (elements, g_strdup (name));
    }
    gst_plugin_feature_list_free (features);
  }
  gst_plugin_list_free (plugins);
  g_strfreev (ignorelist);
}

static void
case_teardown (void)
{
  GList *e;

  for (e = elements; e; e = e->next) {
    g_free (e->data);
  }
  g_list_free (elements);
  elements = NULL;
}

//-- tests

START_TEST (test_state_changes_up_and_down_seq)
{
  BT_TEST_START;
  GstElement *element;
  GList *e;

  for (e = elements; e; e = e->next) {
    const gchar *name = e->data;

    GST_DEBUG ("testing element %s", name);
    element = gst_element_factory_make (name, name);
    ck_assert_msg (element != NULL, "Could not make element from factory %s", name);

    if (GST_IS_PIPELINE (element)) {
      GST_DEBUG ("element %s is a pipeline", name);
    }

    gst_element_set_state (element, GST_STATE_READY);
    gst_element_set_state (element, GST_STATE_PAUSED);
    gst_element_set_state (element, GST_STATE_PLAYING);
    gst_element_set_state (element, GST_STATE_PAUSED);
    gst_element_set_state (element, GST_STATE_READY);
    gst_element_set_state (element, GST_STATE_NULL);
    gst_element_set_state (element, GST_STATE_PAUSED);
    gst_element_set_state (element, GST_STATE_READY);
    gst_element_set_state (element, GST_STATE_PLAYING);
    gst_element_set_state (element, GST_STATE_PAUSED);
    gst_element_set_state (element, GST_STATE_NULL);
    gst_object_unref (GST_OBJECT (element));
  }
  BT_TEST_END;
}
END_TEST

START_TEST (test_state_changes_up_seq)
{
  BT_TEST_START;
  GstElement *element;
  GList *e;

  for (e = elements; e; e = e->next) {
    const gchar *name = e->data;

    GST_INFO ("testing element %s", name);
    element = gst_element_factory_make (name, name);
    ck_assert_msg (element != NULL, "Could not make element from factory %s", name);

    gst_element_set_state (element, GST_STATE_READY);

    gst_element_set_state (element, GST_STATE_PAUSED);
    gst_element_set_state (element, GST_STATE_READY);

    gst_element_set_state (element, GST_STATE_PAUSED);
    gst_element_set_state (element, GST_STATE_PLAYING);
    gst_element_set_state (element, GST_STATE_PAUSED);
    gst_element_set_state (element, GST_STATE_READY);

    gst_element_set_state (element, GST_STATE_NULL);
    gst_object_unref (GST_OBJECT (element));
  }
  BT_TEST_END;
}
END_TEST

START_TEST (test_state_changes_down_seq)
{
  BT_TEST_START;
  GstElement *element;
  GList *e;

  for (e = elements; e; e = e->next) {
    const gchar *name = e->data;

    GST_INFO ("testing element %s", name);
    element = gst_element_factory_make (name, name);
    ck_assert_msg (element != NULL, "Could not make element from factory %s", name);

    gst_element_set_state (element, GST_STATE_READY);
    gst_element_set_state (element, GST_STATE_PAUSED);
    gst_element_set_state (element, GST_STATE_PLAYING);

    gst_element_set_state (element, GST_STATE_PAUSED);
    gst_element_set_state (element, GST_STATE_PLAYING);

    gst_element_set_state (element, GST_STATE_PAUSED);
    gst_element_set_state (element, GST_STATE_READY);
    gst_element_set_state (element, GST_STATE_PAUSED);
    gst_element_set_state (element, GST_STATE_PLAYING);

    gst_element_set_state (element, GST_STATE_PAUSED);
    gst_element_set_state (element, GST_STATE_READY);
    gst_element_set_state (element, GST_STATE_NULL);
    gst_object_unref (GST_OBJECT (element));
  }
  BT_TEST_END;
}
END_TEST

TCase *
gst_buzztrax_elements_test_case (void)
{
  TCase *tc = tcase_create ("GstElementsTests");

  tcase_add_test (tc, test_state_changes_up_and_down_seq);
  tcase_add_test (tc, test_state_changes_up_seq);
  tcase_add_test (tc, test_state_changes_down_seq);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
