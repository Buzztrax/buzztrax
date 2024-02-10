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

static GstMessageType
get_message_type (GstBus * bus)
{
  GstMessage *message = gst_bus_poll (bus, GST_MESSAGE_ANY, GST_SECOND / 2);
  GstMessageType message_type = GST_MESSAGE_UNKNOWN;

  if (message) {
    message_type = GST_MESSAGE_TYPE (message);
    gst_message_unref (message);
  }
  return message_type;
}

//-- tests

static gchar *bt_dec_pipelines[] = {
  "filesrc location=%s ! bt_dec ! fakesink sync=false",
  "filesrc location=%s ! queue ! bt_dec ! fakesink sync=false",
  "playbin uri=file://%s audio-sink=\"fakesink sync=false\""
};

static gchar *bt_dec_files[] = { "simple2.xml", "simple2.bzt" };

START_TEST (test_launch_bt_dec)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  guint pix = _i >> 1;
  guint fix = _i & 1;
  gchar *str = g_strdup_printf (bt_dec_pipelines[pix],
      check_get_test_song_path (bt_dec_files[fix]));
  GstElement *pipeline = gst_parse_launch (str, NULL);
  GstMessageType message_types =
      GST_MESSAGE_NEW_CLOCK | GST_MESSAGE_STATE_CHANGED |
      GST_MESSAGE_STREAM_STATUS | GST_MESSAGE_ASYNC_DONE |
      GST_MESSAGE_STREAM_START | GST_MESSAGE_TAG;
  GstMessageType tmessage = GST_MESSAGE_EOS;

  GST_INFO ("-- act --");
  gst_element_set_state (pipeline, GST_STATE_PLAYING);
  GstStateChangeReturn ret = gst_element_get_state (pipeline, NULL, NULL,
      GST_CLOCK_TIME_NONE);

  GST_INFO ("-- assert --");
  ck_assert_msg (ret == GST_STATE_CHANGE_SUCCESS,
      "Couldn't set pipeline to PLAYING: %s",
      gst_element_state_change_return_get_name (ret));

  GstBus *bus = gst_element_get_bus (pipeline);
  while (1) {
    GstMessageType rmessage = get_message_type (bus);
    if (rmessage == tmessage) {
      break;
    } else if (rmessage == GST_MESSAGE_UNKNOWN) {
      ck_assert_msg (FALSE, "Unexpected timeout in gst_bus_poll, looking for %d", tmessage);
      break;
    } else if (rmessage & message_types) {
      continue;
    }
    GST_INFO ("Unexpected message received of type %d, '%s', looking for %d",
        rmessage, gst_message_type_get_name (rmessage), tmessage);
  }

  GST_INFO ("-- cleanup --");
  gst_object_unref (bus);
  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (pipeline);
  g_free (str);
  BT_TEST_END;
}
END_TEST

static gchar *launch_pipelines[] = {
    // synthesizers
    "ebeats num-buffers=10 volume=100 ! fakesink sync=false",
#ifdef HAVE_FLUIDSYNTH
    "fluidsynth num-buffers=10 note=\"c-4\" ! fakesink sync=false",
#endif
    "sidsyn num-buffers=10 voice0::note=\"c-4\" ! fakesink sync=false",
    "simsyn num-buffers=10 note=\"c-4\" ! fakesink sync=false",
    "wavetabsyn num-buffers=10 note=\"c-4\" ! fakesink sync=false",
    "wavereplay num-buffers=10 ! fakesink sync=false",
    "bml-libTestBmGenerator num-buffers=10 ! fakesink sync=false",
    // effects
    "audiotestsrc num-buffers=10 wave=4 ! audiodelay ! fakesink sync=false",
    "audiotestsrc num-buffers=10 wave=4 ! bml-libTestBmEffect ! fakesink sync=false"};

START_TEST (test_launch_elements)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  GstElement *pipeline = gst_parse_launch (launch_pipelines[_i], NULL);
  GstMessageType message_types =
      GST_MESSAGE_NEW_CLOCK | GST_MESSAGE_STATE_CHANGED |
      GST_MESSAGE_STREAM_STATUS | GST_MESSAGE_ASYNC_DONE |
      GST_MESSAGE_STREAM_START | GST_MESSAGE_TAG;
  GstMessageType tmessage = GST_MESSAGE_EOS;

  GST_INFO ("-- act --");
  gst_element_set_state (pipeline, GST_STATE_PLAYING);
  GstStateChangeReturn ret = gst_element_get_state (pipeline, NULL, NULL,
      GST_CLOCK_TIME_NONE);

  GST_INFO ("-- assert --");
  ck_assert_msg (ret == GST_STATE_CHANGE_SUCCESS,
      "Couldn't set pipeline to PLAYING: %s",
      gst_element_state_change_return_get_name (ret));

  GstBus *bus = gst_element_get_bus (pipeline);
  while (1) {
    GstMessageType rmessage = get_message_type (bus);
    if (rmessage == tmessage) {
      break;
    } else if (rmessage == GST_MESSAGE_UNKNOWN) {
      ck_assert_msg (FALSE, "Unexpected timeout in gst_bus_poll, looking for %d", tmessage);
      break;
    } else if (rmessage & message_types) {
      continue;
    }
    GST_INFO ("Unexpected message received of type %d, '%s', looking for %d",
        rmessage, gst_message_type_get_name (rmessage), tmessage);
  }

  GST_INFO ("-- cleanup --");
  gst_object_unref (bus);
  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (pipeline);
  BT_TEST_END;
}
END_TEST

// TODO(ensonic): test with level that all synths produce data
// TODO(ensonic): if the synth supports presets, test all presets

TCase *
gst_buzztrax_elements_example_case (void)
{
  TCase *tc = tcase_create ("GstElementsExamples");

  tcase_add_loop_test (tc, test_launch_bt_dec, 0,
      G_N_ELEMENTS (bt_dec_pipelines) * G_N_ELEMENTS (bt_dec_files));
  tcase_add_loop_test (tc, test_launch_elements, 0,
      G_N_ELEMENTS (launch_pipelines));
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
