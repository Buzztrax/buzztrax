/* Record n seconds of beep to a file
 *
 * gcc -Wall -g encode.c -o encode `pkg-config gstreamer-1.0 gstreamer-base-1.0 gstreamer-pbutils-1.0 --cflags --libs`
 *
 * for fmt in `seq 0 6`; do ./encode $fmt; done
 *
 * when passing '1' as a second parameter, we don't seek, but set a duration,
 * this is not enough to trigger an eos though.
 *
 * when passing '1' as a third parameter, it will use encodebin instead of manual
 * pipelines. 
 */

#include <gst/gst.h>
#include <gst/base/gstbasesrc.h>
#include <gst/pbutils/encoding-profile.h>

#include <stdio.h>
#include <stdlib.h>

enum
{
  FMT_OGG_VORBIS = 0,
  FMT_MP3,
  FMT_XING_MP3,
  FMT_WAV,
  FMT_OGG_FLAC,
  FMT_MP4_AAC,
  FMT_FLAC,
  FMT_RAW,
  FMT_CT
};

static void
event_loop (GstElement * bin)
{
  GstBus *bus = gst_element_get_bus (GST_ELEMENT (bin));
  GstMessage *message = NULL;

  while (TRUE) {
    message = gst_bus_poll (bus, GST_MESSAGE_ANY, -1);
    GST_INFO ("message %s received", GST_MESSAGE_TYPE_NAME (message));

    switch (message->type) {
      case GST_MESSAGE_EOS:
        gst_message_unref (message);
        return;
      case GST_MESSAGE_WARNING:
      case GST_MESSAGE_ERROR:{
        GError *gerror;
        gchar *debug;

        gst_message_parse_error (message, &gerror, &debug);
        gst_object_default_error (GST_MESSAGE_SRC (message), gerror, debug);
        gst_message_unref (message);
        g_error_free (gerror);
        g_free (debug);
        return;
      }
      default:
        gst_message_unref (message);
        break;
    }
  }
}


static GstEncodingProfile *
create_profile (gchar * name, gchar * desc, gchar * container_caps,
    gchar * container_caps2, gchar * audio_caps)
{
  GstEncodingContainerProfile *c_profile = NULL;
  GstEncodingContainerProfile *c_profile2 = NULL;
  GstEncodingAudioProfile *a_profile = NULL;
  GstCaps *caps;

  GST_INFO ("create profile for \"%s\", \"%s\", \"%s\"", name, container_caps,
      audio_caps);

  if ((!container_caps) && (!audio_caps)) {
    GST_INFO ("no container nor audio-caps \"%s\"", name);
    return NULL;
  }

  if (container_caps) {
    if (!(caps = gst_caps_from_string (container_caps))) {
      GST_WARNING ("can't parse caps \"%s\" for \"%s\"", container_caps, name);
      return NULL;
    }
    if (!(c_profile =
            gst_encoding_container_profile_new (name, desc, caps, NULL))) {
      GST_WARNING ("can't create container profile for caps \"%s\" for \"%s\"",
          container_caps, name);
    }
    gst_caps_unref (caps);
  }

  if (container_caps2) {
    if (!(caps = gst_caps_from_string (container_caps2))) {
      GST_WARNING ("can't parse caps \"%s\" for \"%s\"", container_caps2, name);
      return NULL;
    }
    if (!(c_profile2 =
            gst_encoding_container_profile_new (name, desc, caps, NULL))) {
      GST_WARNING ("can't create container profile for caps \"%s\" for \"%s\"",
          container_caps2, name);
    }
    gst_caps_unref (caps);
  }

  if (!(caps = gst_caps_from_string (audio_caps))) {
    GST_WARNING ("can't parse caps \"%s\" for \"%s\"", audio_caps, name);
    return NULL;
  }
  if (!(a_profile = gst_encoding_audio_profile_new (caps, NULL, NULL, 1))) {
    GST_WARNING ("can't create audio profile for caps \"%s\" for \"%s\"",
        audio_caps, name);
  }
  gst_caps_unref (caps);

  if (c_profile) {
    if (c_profile2) {
      // FIXME(ensonic): this is not supported
      gst_encoding_container_profile_add_profile (c_profile,
          (GstEncodingProfile *)
          c_profile2);
      gst_encoding_container_profile_add_profile (c_profile2,
          (GstEncodingProfile *)
          a_profile);
    } else {
      gst_encoding_container_profile_add_profile (c_profile,
          (GstEncodingProfile *)
          a_profile);
    }
    return (GstEncodingProfile *) c_profile;
  } else {
    return (GstEncodingProfile *) a_profile;
  }
}


gint
main (gint argc, gchar ** argv)
{
  GstElement *bin, *src;
  gchar *pipeline = NULL;
  gint format = 0;
  gint method = 0;
  gint64 duration;
  gboolean use_encodebin = FALSE;
  GstEncodingProfile *profile = NULL;

  gst_init (&argc, &argv);

  if (argc > 1) {
    format = atoi (argv[1]);
    if (argc > 2) {
      method = atoi (argv[2]);
      if (argc > 3) {
        use_encodebin = atoi (argv[3]);
      }
    }
  }
  if (use_encodebin) {
    GstElement *enc, *sink;

    bin = gst_pipeline_new (NULL);
    src = gst_element_factory_make ("audiotestsrc", "s");
    enc = gst_element_factory_make ("encodebin", NULL);
    sink = gst_element_factory_make ("filesink", NULL);
    switch (format) {
      case FMT_OGG_VORBIS:
        g_object_set (sink, "location", "encode.encodebin.vorbis.ogg", NULL);
        profile = create_profile ("Ogg Vorbis record format", "Ogg Vorbis",
            "audio/ogg", NULL, "audio/x-vorbis");
        puts ("encoding ogg vorbis");
        break;
      case FMT_MP3:
        g_object_set (sink, "location", "encode.encodebin.mp3", NULL);
        profile = create_profile ("MP3 record format", "MP3 Audio",
            "application/x-id3", NULL, "audio/mpeg, mpegversion=1, layer=3");
        puts ("encoding mp3");
        break;
      case FMT_XING_MP3:
        g_object_set (sink, "location", "encode.encodebin.xing.mp3", NULL);
        profile = create_profile ("MP3 record format", "MP3 Audio",
            "application/x-id3", "audio/mpeg, mpegversion=1, layer=3",
            "audio/mpeg, mpegversion=1, layer=3");
        puts ("encoding xing-mp3");
        break;
      case FMT_WAV:
        g_object_set (sink, "location", "encode.encodebin.wav", NULL);
        profile =
            create_profile ("WAV record format", "WAV Audio", "audio/x-wav",
            NULL, "audio/x-raw, format=(string)S16LE");
        puts ("encoding wav");
        break;
      case FMT_OGG_FLAC:
        g_object_set (sink, "location", "encode.encodebin.flac.ogg", NULL);
        profile =
            create_profile ("Ogg Flac record format", "Ogg Flac", "audio/ogg",
            NULL, "audio/x-flac");
        puts ("encoding ogg flac");
        break;
      case FMT_MP4_AAC:
        g_object_set (sink, "location", "encode.encodebin.m4a", NULL);
        profile = create_profile ("M4A record format", "M4A Audio",
            NULL, "video/quicktime", "audio/mpeg, mpegversion=(int)4");
        puts ("encoding m4a");
        break;
      case FMT_FLAC:
        g_object_set (sink, "location", "encode.encodebin.flac", NULL);
        profile = create_profile ("Flac record format", "Flac",
            NULL, NULL, "audio/x-flac");
        puts ("encoding flac");
        break;
      case FMT_RAW:
        g_object_set (sink, "location", "encode.encodebin.raw", NULL);
        puts ("encoding raw");
        break;
      default:
        puts ("format must be 0-6");
        return -1;
    }
    if (profile) {
      g_object_set (enc, "profile", profile, NULL);
      gst_bin_add_many (GST_BIN (bin), src, enc, sink, NULL);
      gst_element_link_many (src, enc, sink, NULL);
    } else {
      puts ("profile is NULL");
      gst_bin_add_many (GST_BIN (bin), src, sink, NULL);
      gst_element_link (src, sink);
    }
    src = gst_object_ref (src);
  } else {
    switch (format) {
      case FMT_OGG_VORBIS:
        pipeline =
            "audiotestsrc name=s ! vorbisenc ! oggmux ! filesink location=encode.direct.vorbis.ogg";
        puts ("encoding ogg vorbis");
        break;
      case FMT_MP3:
        pipeline =
            "audiotestsrc name=s ! lamemp3enc ! id3mux ! filesink location=encode.direct.mp3";
        puts ("encoding mp3");
        break;
      case FMT_XING_MP3:
        pipeline =
            "audiotestsrc name=s ! lamemp3enc ! xingmux ! id3mux ! filesink location=encode.direct.xing.mp3";
        puts ("encoding xing-mp3");
        break;
      case FMT_WAV:
        pipeline =
            "audiotestsrc name=s ! wavenc ! filesink location=encode.direct.wav";
        puts ("encoding wav");
        break;
      case FMT_OGG_FLAC:
        pipeline =
            "audiotestsrc name=s ! flacenc ! flacparse ! flactag ! oggmux ! filesink location=encode.direct.flac.ogg";
        puts ("encoding ogg flac");
        break;
      case FMT_MP4_AAC:
        pipeline =
            "audiotestsrc name=s ! voaacenc ! mp4mux ! filesink location=encode.direct.m4a";
        puts ("encoding m4a");
        break;
      case FMT_FLAC:
        pipeline =
            "audiotestsrc name=s ! flacenc ! flacparse ! flactag ! filesink location=encode.direct.flac";
        puts ("encoding flac");
        break;
      case FMT_RAW:
        pipeline = "audiotestsrc name=s ! filesink location=encode.direct.raw";
        puts ("encoding raw");
        break;
      default:
        printf ("format must be 0-%d", FMT_CT);
        return -1;
    }
    bin = gst_parse_launch (pipeline, NULL);
    src = gst_bin_get_by_name (GST_BIN (bin), "s");
  }

  GST_INFO ("pipeline prepared, going to ready");

  gst_element_set_state (bin, GST_STATE_READY);

  if (method == 1) {
    GST_BASE_SRC (src)->segment.duration = GST_SECOND;
  }
  if (method == 0) {
    if (!gst_element_seek (src, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
            GST_SEEK_TYPE_SET, GST_SECOND * 0,
            GST_SEEK_TYPE_SET, GST_SECOND * 1))
      puts ("seek on src in ready failed");
  }
  gst_object_unref (src);

  gst_element_set_state (bin, GST_STATE_PAUSED);
  GST_DEBUG_BIN_TO_DOT_FILE (GST_BIN (bin), GST_DEBUG_GRAPH_SHOW_ALL, "encode");
  GST_INFO ("start encoding");
  gst_element_set_state (bin, GST_STATE_PLAYING);

  if (!gst_element_query_duration (GST_ELEMENT (bin), GST_FORMAT_TIME,
          &duration)) {
    puts ("duration query failed");
  } else {
    printf ("duration: time :%" GST_TIME_FORMAT "\n", GST_TIME_ARGS (duration));
  }
  event_loop (bin);

  GST_INFO ("stopped encoding");
  gst_element_set_state (bin, GST_STATE_NULL);
  gst_object_unref (GST_OBJECT (bin));
  return 0;
}
