/* example for using an initial seek to set the playback duration
 *
 * gcc -Wall -g seekinit.c -o seekinit `pkg-config --cflags --libs gstreamer-1.0`
 *
 * GST_DEBUG="*:2,seekinit:4" ./seekinit
 * GST_DEBUG_DUMP_DOT_DIR=$PWD GST_DEBUG="*:2,seekinit:4" ./seekinit
 *
 * TODO: try with tee, queue, adder
 */

#include <glib.h>
#include <gst/gst.h>

#define GST_CAT_DEFAULT bt_seekinit
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

static void
send_seek (GstElement * bin)
{
  /* we want to play for 5 sec., if we don't set the startpos, this blocks
   */
  if (!gst_element_send_event (bin, gst_event_new_seek (1.0, GST_FORMAT_TIME,
              GST_SEEK_FLAG_FLUSH,
              GST_SEEK_TYPE_SET, 0 * GST_SECOND,
              GST_SEEK_TYPE_SET, 5 * GST_SECOND))) {
    GST_WARNING_OBJECT (bin, "seek failed");
  } else {
    GST_INFO_OBJECT (bin, "seek done");
  }
}

static void
event_loop (GstElement * bin)
{
  GstBus *bus = gst_element_get_bus (bin);
  GstMessage *message = NULL;

  while (TRUE) {
    message = gst_bus_poll (bus, GST_MESSAGE_ANY, -1);

    switch (message->type) {
      case GST_MESSAGE_EOS:
        gst_message_unref (message);
        return;
      case GST_MESSAGE_STATE_CHANGED:
        if (GST_MESSAGE_SRC (message) == GST_OBJECT (bin)) {
          GstState oldstate, newstate;

          gst_message_parse_state_changed (message, &oldstate, &newstate, NULL);
          GST_INFO_OBJECT (bin, "state change on the bin: %s -> %s",
              gst_element_state_get_name (oldstate),
              gst_element_state_get_name (newstate));
          GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS ((GstBin *) bin,
              GST_DEBUG_GRAPH_SHOW_ALL, "seekinit");
          switch (GST_STATE_TRANSITION (oldstate, newstate)) {
            case GST_STATE_CHANGE_NULL_TO_READY:
              send_seek (bin);
              /* play and wait */
              gst_element_set_state (bin, GST_STATE_PLAYING);
              break;
            default:
              break;
          }
        }
        break;
      case GST_MESSAGE_WARNING:
      case GST_MESSAGE_ERROR:{
        GError *gerror;
        gchar *debug;

        gst_message_parse_error (message, &gerror, &debug);
        gst_object_default_error (GST_MESSAGE_SRC (message), gerror, debug);
        g_error_free (gerror);
        g_free (debug);
        return;
      }
      default:
        GST_INFO_OBJECT (bin, "%" GST_PTR_FORMAT, message);
        break;
    }
    gst_message_unref (message);
  }
}

gint
main (gint argc, gchar ** argv)
{
  GstElement *bin, *src, *sink;

  /* init gstreamer */
  gst_init (&argc, &argv);
  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "seekinit", 0, "initial seek test");

  /* build pipeline */
  bin = gst_pipeline_new ("song");
  if (!(sink = gst_element_factory_make ("autoaudiosink", NULL))) {
    GST_WARNING ("Can't create element \"autoaudiosink\"");
    return -1;
  }
  if (!(src = gst_element_factory_make ("audiotestsrc", NULL))) {
    GST_WARNING ("Can't create element \"audiotestsrc\"");
    return -1;
  }
  g_object_set (src, "wave", /* saw */ 2, "blocksize", 22050, NULL);
  gst_bin_add_many (GST_BIN (bin), src, sink, NULL);
  if (!gst_element_link_many (src, sink, NULL)) {
    GST_WARNING ("Can't link elements");
    return -1;
  }
  GST_INFO ("prepared");

  /* prepare playback */
  gst_element_set_state (bin, GST_STATE_READY);

  event_loop (bin);
  GST_INFO ("done");

  /* stop and cleanup */
  gst_element_set_state (bin, GST_STATE_NULL);
  gst_object_unref (GST_OBJECT (bin));
  return 0;
}
