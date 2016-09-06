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


#include "m-bt-gst.h"

//-- globals

//-- fixtures

static void
case_setup (void)
{
  BT_CASE_START;
}

static void
case_teardown (void)
{
}

//-- tests

gchar *pipeline_desc[] = {
  "filesrc location=%s ! buzztrax-dec ! fakesink sync=false",
  "filesrc location=%s ! queue ! buzztrax-dec ! fakesink sync=false",
  "playbin uri=file://%s audio-sink=\"fakesink sync=false\""
};

gchar *files[] = { "simple2.xml", "simple2.bzt" };

static void
test_launch_bt_dec (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  guint pix = _i >> 1;
  guint fix = _i & 1;
  gchar *str = g_strdup_printf (pipeline_desc[pix],
      check_get_test_song_path (files[fix]));
  GstElement *pipeline = gst_parse_launch (str, NULL);
  GstMessageType message_types =
      GST_MESSAGE_NEW_CLOCK | GST_MESSAGE_STATE_CHANGED |
      GST_MESSAGE_STREAM_STATUS | GST_MESSAGE_ASYNC_DONE |
      GST_MESSAGE_STREAM_START | GST_MESSAGE_TAG;
  GstMessageType tmessage = GST_MESSAGE_EOS;

  GST_INFO ("-- act && assert --");
  gst_element_set_state (pipeline, GST_STATE_PLAYING);
  GstStateChangeReturn ret = gst_element_get_state (pipeline, NULL, NULL,
      GST_CLOCK_TIME_NONE);

  fail_unless (ret == GST_STATE_CHANGE_SUCCESS,
      "Couldn't set pipeline to PLAYING: %s",
      gst_element_state_change_return_get_name (ret));

  GstBus *bus = gst_element_get_bus (pipeline);
  while (1) {
    GstMessage *message = gst_bus_poll (bus, GST_MESSAGE_ANY, GST_SECOND / 2);
    GstMessageType rmessage;

    if (message) {
      rmessage = GST_MESSAGE_TYPE (message);
      gst_message_unref (message);
    } else {
      rmessage = GST_MESSAGE_UNKNOWN;
    }

    if (rmessage == tmessage) {
      break;
    } else if (rmessage == GST_MESSAGE_UNKNOWN) {
      fail ("Unexpected timeout in gst_bus_poll, looking for %d", tmessage);
      break;
    } else if (rmessage & message_types) {
      continue;
    }
    fail ("Unexpected message received of type %d, '%s', looking for %d",
        rmessage, gst_message_type_get_name (rmessage), tmessage);
  }

  GST_INFO ("-- cleanup --");
  gst_object_unref (bus);
  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (pipeline);
  g_free (str);
  BT_TEST_END;
}


TCase *
gst_buzztrax_elements_example_case (void)
{
  TCase *tc = tcase_create ("GstElementExamples");

  tcase_add_loop_test (tc, test_launch_bt_dec, 0,
      G_N_ELEMENTS (pipeline_desc) * G_N_ELEMENTS (files));
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
