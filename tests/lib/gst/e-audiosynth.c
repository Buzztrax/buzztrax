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

#include "gst/audiosynth.h"
#include "gst/tempo.h"

//-- test element

#define GSTBT_TYPE_TEST_AUDIO_SYNTH    (bt_test_audio_synth_get_type())

typedef struct
{
  gsize size;
  GstClockTime ts, duration;
  guint64 offset, offset_end;
} BufferFields;

typedef struct _BtTestAudioSynth
{
  GstBtAudioSynth parent;

  GstCaps *caps;
  GList *buffer_info;
  gint num_disconts;
} BtTestAudioSynth;

typedef struct _BtTestAudioSynthClass
{
  GstBtAudioSynthClass parent_class;
} BtTestAudioSynthClass;

G_DEFINE_TYPE (BtTestAudioSynth, bt_test_audio_synth, GSTBT_TYPE_AUDIO_SYNTH);

static BufferFields *
get_buffer_info (BtTestAudioSynth * self, gint i)
{
  if (i >= 0) {
    return (BufferFields *) (g_list_nth_data (self->buffer_info, i));
  } else {
    gint len = g_list_length (self->buffer_info);
    return (BufferFields *) (g_list_nth_data (self->buffer_info, (len + i)));
  }
}

static void
reset_buffer_info (BtTestAudioSynth * self)
{
  if (self->buffer_info) {
    g_list_free_full (self->buffer_info, g_free);
    self->buffer_info = NULL;
  }
}

static void
bt_test_audio_synth_negotiate (GstBtAudioSynth * base, GstCaps * caps)
{
  BtTestAudioSynth *self = (BtTestAudioSynth *) base;

  self->caps = gst_caps_ref (caps);
}

static void
bt_test_audio_synth_reset (GstBtAudioSynth * base)
{
  BtTestAudioSynth *self = (BtTestAudioSynth *) base;

  self->num_disconts++;
  reset_buffer_info (self);
}

static gboolean
bt_test_audio_synth_process (GstBtAudioSynth * base, GstBuffer * data,
    GstMapInfo * info)
{
  BtTestAudioSynth *self = (BtTestAudioSynth *) base;
  BufferFields *bf = g_new (BufferFields, 1);

  bf->ts = GST_BUFFER_TIMESTAMP (data);
  bf->duration = GST_BUFFER_DURATION (data);
  bf->offset = GST_BUFFER_OFFSET (data);
  bf->offset_end = GST_BUFFER_OFFSET_END (data);
  bf->size = info->size;
  self->buffer_info = g_list_append (self->buffer_info, bf);
  return TRUE;
}

static void
bt_test_audio_synth_init (BtTestAudioSynth * self)
{
  self->caps = NULL;
  self->num_disconts = 0;
}

static void
bt_test_audio_synth_finalize (GObject * object)
{
  BtTestAudioSynth *self = (BtTestAudioSynth *) object;

  if (self->caps) {
    gst_caps_replace (&self->caps, NULL);
  }
  reset_buffer_info (self);

  G_OBJECT_CLASS (bt_test_audio_synth_parent_class)->finalize (object);
}

static void
bt_test_audio_synth_class_init (BtTestAudioSynthClass * klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;
  GstElementClass *element_class = (GstElementClass *) klass;
  GstBtAudioSynthClass *audio_synth_class = (GstBtAudioSynthClass *) klass;

  object_class->finalize = bt_test_audio_synth_finalize;

  audio_synth_class->process = bt_test_audio_synth_process;
  audio_synth_class->reset = bt_test_audio_synth_reset;
  audio_synth_class->negotiate = bt_test_audio_synth_negotiate;

  gst_element_class_set_static_metadata (element_class,
      "Audio synth for unit tests", "Source/Audio",
      "Use in unit tests", "Stefan Sauer <ensonic@users.sf.net>");
}

gboolean
bt_test_audio_synth_plugin_init (GstPlugin * const plugin)
{
  GST_INFO ("register element");
  gst_element_register (plugin, "buzztrax-test-audio-synth",
      GST_RANK_NONE, GSTBT_TYPE_TEST_AUDIO_SYNTH);
  return TRUE;
}

//-- fixtures

static GstClockTime ticktime =
    (GstClockTime) (0.5 + (GST_SECOND * (60.0 / 8)) / (120 * 4));
static GstContext *ctx;

static void
case_setup (void)
{
  BT_CASE_START;

  GST_INFO ("register plugin");
  gst_plugin_register_static (GST_VERSION_MAJOR,
      GST_VERSION_MINOR, "buzztrax-test-audio-synth", "test plugin",
      bt_test_audio_synth_plugin_init,
      VERSION, "LGPL", PACKAGE, PACKAGE_NAME, "http://www.buzztrax.org");

  ctx = gstbt_audio_tempo_context_new (120, 4, 8);
}

static void
case_teardown (void)
{
  gst_context_unref (ctx);
}

//-- tests

START_TEST (test_create_obj)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");

  GST_INFO ("-- act --");
  GstElement *e = gst_element_factory_make ("buzztrax-test-audio-synth", NULL);

  GST_INFO ("-- assert --");
  ck_assert (e != NULL);

  GST_INFO ("-- cleanup --");
  gst_object_unref (e);
  BT_TEST_END;
}
END_TEST

START_TEST (test_initialized_with_audio_caps)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  GstElement *p =
      gst_parse_launch
      ("buzztrax-test-audio-synth name=\"src\" num-buffers=1 ! fakesink async=false",
      NULL);
  BtTestAudioSynth *e =
      (BtTestAudioSynth *) gst_bin_get_by_name (GST_BIN (p), "src");
  GstBus *bus = gst_element_get_bus (p);

  GST_INFO ("-- act --");
  gst_element_set_state (p, GST_STATE_PLAYING);
  gst_bus_poll (bus, GST_MESSAGE_EOS | GST_MESSAGE_ERROR, GST_CLOCK_TIME_NONE);

  GST_INFO ("-- assert --");
  ck_assert (e->caps != NULL);
  gint i, cs = gst_caps_get_size (e->caps);
  ck_assert (cs > 0);
  for (i = 0; i < cs; i++) {
    ck_assert (gst_structure_has_name (gst_caps_get_structure (e->caps, i),
            "audio/x-raw"));
  }

  GST_INFO ("-- cleanup --");
  gst_element_set_state (p, GST_STATE_NULL);
  gst_object_unref (e);
  gst_object_unref (p);
  BT_TEST_END;
}
END_TEST

START_TEST (test_audio_context_configures_buffer_size)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  GstElement *p =
      gst_parse_launch
      ("buzztrax-test-audio-synth name=\"src\" num-buffers=1 ! fakesink async=false",
      NULL);
  BtTestAudioSynth *e =
      (BtTestAudioSynth *) gst_bin_get_by_name (GST_BIN (p), "src");
  GstBus *bus = gst_element_get_bus (p);

  GST_INFO ("-- act --");
  gst_element_set_state (p, GST_STATE_READY);
  gst_element_set_context (p, ctx);
  gst_element_set_state (p, GST_STATE_PLAYING);
  gst_bus_poll (bus, GST_MESSAGE_EOS | GST_MESSAGE_ERROR, GST_CLOCK_TIME_NONE);

  GST_INFO ("-- assert --");
  BufferFields *bf = get_buffer_info (e, 0);
  // sizeof(gint16) * (int)(0.5 + (44100 * (60.0 / 8)) / (120 * 4))
  ck_assert_uint_eq (bf->size, 1378);

  GST_INFO ("-- cleanup --");
  gst_element_set_state (p, GST_STATE_NULL);
  gst_object_unref (bus);
  gst_object_unref (e);
  gst_object_unref (p);
  BT_TEST_END;
}
END_TEST

START_TEST (test_forward_first_buffer_starts_at_zero)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  GstElement *p =
      gst_parse_launch
      ("buzztrax-test-audio-synth name=\"src\" num-buffers=1 ! fakesink async=false",
      NULL);
  BtTestAudioSynth *e =
      (BtTestAudioSynth *) gst_bin_get_by_name (GST_BIN (p), "src");
  GstBus *bus = gst_element_get_bus (p);

  GST_INFO ("-- act --");
  gst_element_set_state (p, GST_STATE_PLAYING);
  gst_bus_poll (bus, GST_MESSAGE_EOS | GST_MESSAGE_ERROR, GST_CLOCK_TIME_NONE);

  GST_INFO ("-- assert --");
  BufferFields *bf = get_buffer_info (e, 0);
  ck_assert_uint64_eq (bf->ts, 0L);
  ck_assert_uint64_eq (bf->offset, 0L);

  GST_INFO ("-- cleanup --");
  gst_element_set_state (p, GST_STATE_NULL);
  gst_object_unref (e);
  gst_object_unref (p);
  BT_TEST_END;
}
END_TEST

START_TEST (test_backwards_last_buffer_ends_at_zero)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  GstElement *p =
      gst_parse_launch
      ("buzztrax-test-audio-synth name=\"src\" num-buffers=1 ! fakesink async=false",
      NULL);
  BtTestAudioSynth *e =
      (BtTestAudioSynth *) gst_bin_get_by_name (GST_BIN (p), "src");
  GstBus *bus = gst_element_get_bus (p);

  GST_INFO ("-- act --");
  gst_element_set_state (p, GST_STATE_READY);
  gst_element_set_context (p, ctx);
  gst_element_set_state (p, GST_STATE_PAUSED);
  gst_element_get_state (p, NULL, NULL, GST_CLOCK_TIME_NONE);
  gst_element_seek (p, -1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
      GST_SEEK_TYPE_SET, G_GUINT64_CONSTANT (0), GST_SEEK_TYPE_SET, ticktime);
  gst_element_set_state (p, GST_STATE_PLAYING);
  gst_element_get_state (p, NULL, NULL, GST_CLOCK_TIME_NONE);
  gst_bus_poll (bus, GST_MESSAGE_EOS | GST_MESSAGE_ERROR, GST_CLOCK_TIME_NONE);

  GST_INFO ("-- assert --");
  BufferFields *bf = get_buffer_info (e, 0);
  ck_assert_uint64_eq (bf->ts, 0L);
  ck_assert_uint64_eq (bf->offset, 0L);

  GST_INFO ("-- cleanup --");
  gst_element_set_state (p, GST_STATE_NULL);
  gst_object_unref (e);
  gst_object_unref (p);

  BT_TEST_END;
}
END_TEST

START_TEST (test_buffers_are_contigous)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  GstElement *p =
      gst_parse_launch
      ("buzztrax-test-audio-synth name=\"src\" num-buffers=2 ! fakesink async=false",
      NULL);
  BtTestAudioSynth *e =
      (BtTestAudioSynth *) gst_bin_get_by_name (GST_BIN (p), "src");
  GstBus *bus = gst_element_get_bus (p);

  GST_INFO ("-- act --");
  gst_element_set_state (p, GST_STATE_PLAYING);
  gst_bus_poll (bus, GST_MESSAGE_EOS | GST_MESSAGE_ERROR, GST_CLOCK_TIME_NONE);

  GST_INFO ("-- assert --");
  BufferFields *bf0 = get_buffer_info (e, 0);
  BufferFields *bf1 = get_buffer_info (e, 1);
  ck_assert_uint64_eq (bf1->ts, bf0->ts + bf0->duration);
  ck_assert_uint64_eq (bf1->offset, bf0->offset + bf0->offset_end);

  GST_INFO ("-- cleanup --");
  gst_element_set_state (p, GST_STATE_NULL);
  gst_object_unref (e);
  gst_object_unref (p);
  BT_TEST_END;
}
END_TEST

START_TEST (test_num_buffers_with_stop_pos)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  GstElement *p =
      gst_parse_launch
      ("buzztrax-test-audio-synth name=\"src\" ! fakesink async=false",
      NULL);
  BtTestAudioSynth *e =
      (BtTestAudioSynth *) gst_bin_get_by_name (GST_BIN (p), "src");
  GstBus *bus = gst_element_get_bus (p);
  GstSeekFlags seek_flags[] = {
    GST_SEEK_FLAG_FLUSH, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SEGMENT
  };
  GstMessageType end_msg[] = {
    GST_MESSAGE_EOS, GST_MESSAGE_SEGMENT_DONE
  };

  GST_INFO ("-- act --");
  gst_element_set_state (p, GST_STATE_READY);
  gst_element_set_context (p, ctx);
  gst_element_set_state (p, GST_STATE_PAUSED);
  gst_element_get_state (p, NULL, NULL, GST_CLOCK_TIME_NONE);
  gst_element_seek (p, 1.0, GST_FORMAT_TIME, seek_flags[_i],
      GST_SEEK_TYPE_SET, G_GUINT64_CONSTANT (0),
      GST_SEEK_TYPE_SET, ticktime * 2);
  gst_element_set_state (p, GST_STATE_PLAYING);
  gst_element_get_state (p, NULL, NULL, GST_CLOCK_TIME_NONE);
  gst_bus_poll (bus, end_msg[_i] | GST_MESSAGE_ERROR, GST_CLOCK_TIME_NONE);

  GST_INFO ("-- assert --");
  gint num_buffers = g_list_length (e->buffer_info);
  ck_assert_uint_eq (num_buffers, 2);

  GST_INFO ("-- cleanup --");
  gst_element_set_state (p, GST_STATE_NULL);
  gst_object_unref (e);
  gst_object_unref (p);
  BT_TEST_END;
}
END_TEST

START_TEST (test_last_buffer_is_clipped)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  GstElement *p =
      gst_parse_launch
      ("buzztrax-test-audio-synth name=\"src\" ! fakesink async=false",
      NULL);
  BtTestAudioSynth *e =
      (BtTestAudioSynth *) gst_bin_get_by_name (GST_BIN (p), "src");
  GstBus *bus = gst_element_get_bus (p);

  GST_INFO ("-- act --");
  gst_element_set_state (p, GST_STATE_READY);
  gst_element_set_context (p, ctx);
  gst_element_set_state (p, GST_STATE_PAUSED);
  gst_element_get_state (p, NULL, NULL, GST_CLOCK_TIME_NONE);
  gst_element_seek (p, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
      GST_SEEK_TYPE_SET, G_GUINT64_CONSTANT (0),
      GST_SEEK_TYPE_SET, ticktime * 1.5);
  gst_element_set_state (p, GST_STATE_PLAYING);
  gst_element_get_state (p, NULL, NULL, GST_CLOCK_TIME_NONE);
  gst_bus_poll (bus, GST_MESSAGE_EOS | GST_MESSAGE_ERROR, GST_CLOCK_TIME_NONE);

  GST_INFO ("-- assert --");
  BufferFields *bf0 = get_buffer_info (e, 0);
  BufferFields *bf1 = get_buffer_info (e, 1);
  ck_assert_uint64_le (bf1->duration, bf0->duration / 2);
  ck_assert_uint_le (bf1->size, bf0->size / 2);

  GST_INFO ("-- cleanup --");
  gst_element_set_state (p, GST_STATE_NULL);
  gst_object_unref (e);
  gst_object_unref (p);
  BT_TEST_END;
}
END_TEST

START_TEST (test_no_reset_without_seeks)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  GstElement *p =
      gst_parse_launch
      ("buzztrax-test-audio-synth name=\"src\" ! fakesink async=false", NULL);
  BtTestAudioSynth *e =
      (BtTestAudioSynth *) gst_bin_get_by_name (GST_BIN (p), "src");

  GST_INFO ("-- act --");
  gst_element_set_state (p, GST_STATE_PLAYING);
  gst_element_get_state (p, NULL, NULL, GST_CLOCK_TIME_NONE);

  GST_INFO ("-- assert --");
  ck_assert_int_eq (e->num_disconts, 0);

  GST_INFO ("-- cleanup --");
  gst_element_set_state (p, GST_STATE_NULL);
  gst_object_unref (e);
  gst_object_unref (p);
  BT_TEST_END;
}
END_TEST

START_TEST (test_reset_on_seek)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  GstElement *p =
      gst_parse_launch
      ("buzztrax-test-audio-synth name=\"src\" ! fakesink async=false", NULL);
  BtTestAudioSynth *e =
      (BtTestAudioSynth *) gst_bin_get_by_name (GST_BIN (p), "src");
  GstBus *bus = gst_element_get_bus (p);

  GST_INFO ("-- act --");
  gst_element_set_state (p, GST_STATE_PAUSED);
  gst_element_get_state (p, NULL, NULL, GST_CLOCK_TIME_NONE);
  gst_element_seek (p, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
      GST_SEEK_TYPE_SET, GST_MSECOND * 100,
      GST_SEEK_TYPE_SET, GST_MSECOND * 200);
  gst_element_set_state (p, GST_STATE_PLAYING);
  gst_element_get_state (p, NULL, NULL, GST_CLOCK_TIME_NONE);
  gst_bus_poll (bus, GST_MESSAGE_EOS | GST_MESSAGE_ERROR, GST_CLOCK_TIME_NONE);

  GST_INFO ("-- assert --");
  ck_assert_int_eq (e->num_disconts, 1);

  GST_INFO ("-- cleanup --");
  gst_element_set_state (p, GST_STATE_NULL);
  gst_object_unref (e);
  gst_object_unref (p);
  BT_TEST_END;
}
END_TEST

START_TEST (test_position_query_time)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  GstElement *p =
      gst_parse_launch
      ("buzztrax-test-audio-synth name=\"src\" num-buffers=1 ! fakesink async=false",
      NULL);
  BtTestAudioSynth *e =
      (BtTestAudioSynth *) gst_bin_get_by_name (GST_BIN (p), "src");
  GstBus *bus = gst_element_get_bus (p);

  GST_INFO ("-- act --");
  gst_element_set_state (p, GST_STATE_PLAYING);
  gst_bus_poll (bus, GST_MESSAGE_EOS | GST_MESSAGE_ERROR, GST_CLOCK_TIME_NONE);

  GST_INFO ("-- assert --");
  BufferFields *bf = get_buffer_info (e, 0);
  gint64 pos;
  gboolean res = gst_element_query_position ((GstElement *) e, GST_FORMAT_TIME,
      &pos);
  ck_assert (res);
  ck_assert_uint64_eq (bf->duration, pos);

  GST_INFO ("-- cleanup --");
  gst_element_set_state (p, GST_STATE_NULL);
  gst_object_unref (e);
  gst_object_unref (p);
  BT_TEST_END;
}
END_TEST

/*
test buffer metadata in process
  - backwards playback
*/

TCase *
gst_buzztrax_audiosynth_example_case (void)
{
  TCase *tc = tcase_create ("GstBtAudiosynthExamples");

  tcase_add_test (tc, test_create_obj);
  tcase_add_test (tc, test_initialized_with_audio_caps);
  tcase_add_test (tc, test_audio_context_configures_buffer_size);
  tcase_add_test (tc, test_forward_first_buffer_starts_at_zero);
  tcase_add_test (tc, test_backwards_last_buffer_ends_at_zero);
  tcase_add_test (tc, test_buffers_are_contigous);
  tcase_add_loop_test (tc, test_num_buffers_with_stop_pos, 0, 2);
  tcase_add_test (tc, test_last_buffer_is_clipped);
  tcase_add_test (tc, test_no_reset_without_seeks);
  tcase_add_test (tc, test_reset_on_seek);
  tcase_add_test (tc, test_position_query_time);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
