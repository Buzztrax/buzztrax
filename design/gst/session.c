/* test jack/pulseaudio session handling
 *
 * gcc -Wall -g session.c -o session `pkg-config gstreamer-0.10 --cflags --libs`
 */

#include <stdio.h>
#include <stdlib.h>
#include <gst/gst.h>

static gboolean
event_loop (GstElement * bin)
{
  GstBus *bus = gst_element_get_bus (bin);
  GstMessage *message = NULL;
  gboolean res = FALSE;

  while (TRUE) {
    message = gst_bus_poll (bus, GST_MESSAGE_ANY, -1);
    GST_DEBUG_OBJECT (GST_MESSAGE_SRC (message), "message %s received",
        GST_MESSAGE_TYPE_NAME (message));

    switch (message->type) {
      case GST_MESSAGE_EOS:
        gst_message_unref (message);
        res = TRUE;
        goto Done;
      case GST_MESSAGE_WARNING:
      case GST_MESSAGE_ERROR:{
        GError *gerror;
        gchar *debug;

        gst_message_parse_error (message, &gerror, &debug);
        gst_object_default_error (GST_MESSAGE_SRC (message), gerror, debug);
        gst_message_unref (message);
        g_error_free (gerror);
        g_free (debug);
        res = TRUE;
        goto Done;
      }
      default:
        gst_message_unref (message);
        break;
    }
  }
Done:
  gst_object_unref (bus);
  return res;
}

static gboolean
kick_start_loop (GstElement * bin)
{
  GstBus *bus = gst_element_get_bus (bin);
  GstMessage *message = NULL;
  gboolean res = FALSE;

  while (TRUE) {
    message = gst_bus_poll (bus, GST_MESSAGE_ANY, -1);
    GST_DEBUG_OBJECT (GST_MESSAGE_SRC (message), "message %s received",
        GST_MESSAGE_TYPE_NAME (message));

    switch (message->type) {
      case GST_MESSAGE_STATE_CHANGED:{
        gboolean res = FALSE;
        if (GST_MESSAGE_SRC (message) == GST_OBJECT (bin)) {
          GstState oldstate, newstate, pending;

          gst_message_parse_state_changed (message, &oldstate, &newstate,
              &pending);
          if (GST_STATE_TRANSITION (oldstate,
                  newstate) == GST_STATE_CHANGE_READY_TO_PAUSED)
            res = TRUE;
        }
        gst_message_unref (message);
        if (res)
          goto Done;
        break;
      }
      case GST_MESSAGE_WARNING:
      case GST_MESSAGE_ERROR:{
        GError *gerror;
        gchar *debug;

        gst_message_parse_error (message, &gerror, &debug);
        gst_object_default_error (GST_MESSAGE_SRC (message), gerror, debug);
        gst_message_unref (message);
        g_error_free (gerror);
        g_free (debug);
        res = FALSE;
        goto Done;
      }
      default:
        gst_message_unref (message);
        break;
    }
  }
Done:
  gst_object_unref (bus);
  return res;
}


static gint
get_key (void)
{
  gint c;

  do {
    c = fgetc (stdin);
  } while (c < 0);

  return c;
}

gint
main (gint argc, gchar ** argv)
{
  GstElement *bin;
  /* elements used in pipeline */
  GstElement *sink, *src, *audio_sink;
  gchar *sink_name = "jackaudiosink";
  gboolean running = TRUE;
  gint c;

  if (argc > 1) {
    switch (argv[1][0]) {
      case 'j':
        sink_name = "jackaudiosink";
        break;
      case 'p':
        sink_name = "pulsesink";
        break;
      default:
        break;
    }
  }

  /* init gstreamer */
  gst_init (&argc, &argv);

  puts ("preparing sink");
  // create audio session sink and remove floating ref
  audio_sink = gst_object_ref (gst_element_factory_make (sink_name, NULL));
  gst_object_sink (audio_sink);

  bin = gst_pipeline_new ("song");
  src = gst_element_factory_make ("audiotestsrc", NULL);
  sink = gst_object_ref (audio_sink);
  gst_bin_add_many (GST_BIN (bin), src, sink, NULL);
  gst_element_link (src, sink);
  puts ("go to paused");
  gst_element_set_locked_state (audio_sink, FALSE);
  gst_element_set_state (bin, GST_STATE_PAUSED);
  kick_start_loop (bin);
  gst_element_set_state (bin, GST_STATE_READY);
  gst_element_set_locked_state (audio_sink, TRUE);
  gst_element_set_state (bin, GST_STATE_NULL);
  g_object_unref (bin);

  puts ("prepared, press key to continue");
  c = get_key ();

  // play a beep pipeline and tear it down
  while (running) {
    bin = gst_pipeline_new ("song");
    src = gst_element_factory_make ("audiotestsrc", NULL);
    g_object_set (src, "num-buffers", 50, NULL);
    sink = gst_object_ref (audio_sink);
    gst_bin_add_many (GST_BIN (bin), src, sink, NULL);
    gst_element_link (src, sink);
    gst_element_set_locked_state (audio_sink, FALSE);
    gst_element_set_state (bin, GST_STATE_PLAYING);

    running = event_loop (bin);

    gst_element_set_state (bin, GST_STATE_READY);
    gst_element_set_locked_state (audio_sink, TRUE);
    gst_element_set_state (bin, GST_STATE_NULL);
    g_object_unref (bin);

    printf ("audio_sink.parent=%p\n", GST_OBJECT_PARENT (audio_sink));

    if (running) {
      puts ("loop done, press 'd' for done, continue otherwise");
      c = get_key ();
      if (c == 'd')
        running = FALSE;
      else
        puts ("play again");
    }
  }

  gst_element_set_locked_state (audio_sink, FALSE);
  gst_element_set_state (audio_sink, GST_STATE_NULL);
  gst_object_unref (audio_sink);
  return 0;
}
