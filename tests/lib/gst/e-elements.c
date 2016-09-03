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

//-- helper

/*
 * run_pipeline:
 * @pipe: the pipeline to run
 * @desc: the description for use in messages
 * @message_types: is a mask of expected message_types
 * @tmessage: is the expected terminal message
 *
 * the poll call will time out after half a second.
 */
static gboolean
run_pipeline (GstElement * pipeline, GstMessageType message_types,
    GstMessageType tmessage)
{
  gboolean res = TRUE;
  gst_element_set_state (pipeline, GST_STATE_PLAYING);
  GstStateChangeReturn ret = gst_element_get_state (pipeline, NULL, NULL,
      GST_CLOCK_TIME_NONE);

  if (ret != GST_STATE_CHANGE_SUCCESS) {
    GST_ERROR_OBJECT (pipeline, "Couldn't set pipeline to PLAYING, %d", ret);
    res = FALSE;
    goto done;
  }

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
      GST_ERROR_OBJECT (pipeline,
          "Unexpected timeout in gst_bus_poll, looking for %d", tmessage);
      res = FALSE;
      break;
    } else if (rmessage & message_types) {
      continue;
    }
    GST_ERROR_OBJECT (pipeline,
        "Unexpected message received of type %d, '%s', looking for %d",
        rmessage, gst_message_type_get_name (rmessage), tmessage);
    res = FALSE;
  }
  gst_object_unref (bus);

done:
  gst_element_set_state (pipeline, GST_STATE_NULL);
  return res;
}

//-- tests

static void
test_launch_bt_dec_pull (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  gchar *str =
      g_strdup_printf
      ("filesrc location=%s ! buzztrax-dec ! fakesink sync=false",
      check_get_test_song_path ("simple2.xml"));
  GstElement *pipeline = gst_parse_launch (str, NULL);

  GST_INFO ("-- act && assert --");
  fail_unless (run_pipeline (pipeline,
          GST_MESSAGE_NEW_CLOCK | GST_MESSAGE_STATE_CHANGED |
          GST_MESSAGE_STREAM_STATUS | GST_MESSAGE_ASYNC_DONE |
          GST_MESSAGE_STREAM_START | GST_MESSAGE_TAG, GST_MESSAGE_EOS), NULL);

  GST_INFO ("-- cleanup --");
  gst_object_unref (pipeline);
  g_free (str);
  BT_TEST_END;
}

static void
test_launch_bt_dec_push (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  gchar *str =
      g_strdup_printf
      ("filesrc location=%s ! queue ! buzztrax-dec ! fakesink sync=false",
      check_get_test_song_path ("simple2.xml"));
  GstElement *pipeline = gst_parse_launch (str, NULL);

  GST_INFO ("-- act && assert --");
  fail_unless (run_pipeline (pipeline,
          GST_MESSAGE_NEW_CLOCK | GST_MESSAGE_STATE_CHANGED |
          GST_MESSAGE_STREAM_STATUS | GST_MESSAGE_ASYNC_DONE |
          GST_MESSAGE_STREAM_START | GST_MESSAGE_TAG, GST_MESSAGE_EOS), NULL);

  GST_INFO ("-- cleanup --");
  gst_object_unref (pipeline);
  g_free (str);
  BT_TEST_END;
}

static void
test_launch_bt_dec_playbin (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  gchar *str =
      g_strdup_printf
      ("playbin uri=file://%s audio-sink=\"fakesink sync=false\"",
      check_get_test_song_path ("simple2.xml"));
  GstElement *pipeline = gst_parse_launch (str, NULL);

  GST_INFO ("-- act && assert --");
  fail_unless (run_pipeline (pipeline,
          GST_MESSAGE_NEW_CLOCK | GST_MESSAGE_STATE_CHANGED |
          GST_MESSAGE_STREAM_STATUS | GST_MESSAGE_ASYNC_DONE |
          GST_MESSAGE_STREAM_START | GST_MESSAGE_TAG, GST_MESSAGE_EOS), NULL);

  GST_INFO ("-- cleanup --");
  gst_object_unref (pipeline);
  g_free (str);
  BT_TEST_END;
}


TCase *
gst_buzztrax_elements_example_case (void)
{
  TCase *tc = tcase_create ("GstElementExamples");

  tcase_add_test (tc, test_launch_bt_dec_pull);
  tcase_add_test (tc, test_launch_bt_dec_push);
  tcase_add_test (tc, test_launch_bt_dec_playbin);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
