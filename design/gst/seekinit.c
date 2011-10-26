/* $Id$
 * initial seek to set the duration example
 *
 * gcc -Wall -g `pkg-config --cflags --libs gstreamer-0.10` seekinit.c -o seekinit
 *
 * GST_DEBUG="*:2,seekinit:3" ./seekinit
 * GST_DEBUG_DUMP_DOT_DIR=$PWD GST_DEBUG="*:2,seekinit:3" ./seekinit
 *
 * try with adder, queue, tee
 */

#include <glib.h>
#include <gst/gst.h>

#define GST_CAT_DEFAULT bt_seekinit
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

static GstDebugGraphDetails graph_details =
    GST_DEBUG_GRAPH_SHOW_ALL & ~GST_DEBUG_GRAPH_SHOW_CAPS_DETAILS;
    //GST_DEBUG_GRAPH_SHOW_ALL;

static void
event_loop (GstElement * bin)
{
  GstBus *bus;
  GstMessage *message = NULL;

  bus = gst_element_get_bus (GST_ELEMENT (bin));

  while (TRUE) {
    message = gst_bus_poll (bus, GST_MESSAGE_ANY, -1);

    g_assert (message != NULL);
    
    GST_DEBUG_OBJECT(GST_MESSAGE_SRC(message), "message %s received",
        GST_MESSAGE_TYPE_NAME(message));

    switch (message->type) {
      case GST_MESSAGE_EOS:
        GST_INFO("eos");
        gst_message_unref (message);
        return;
      case GST_MESSAGE_DURATION: {
        GstFormat fmt;
        gint64 dur;

        gst_message_parse_duration (message, &fmt, &dur);
        GST_INFO_OBJECT(GST_MESSAGE_SRC(message), "duration msg: %"GST_TIME_FORMAT,
            GST_TIME_ARGS(dur));
        gst_message_unref (message);
        break;
      }
      case GST_MESSAGE_STATE_CHANGED:
        if(GST_MESSAGE_SRC(message) == GST_OBJECT(bin)) {
          GstState oldstate,newstate,pending;

          gst_message_parse_state_changed(message,&oldstate,&newstate,&pending);
          GST_INFO("state change on the bin: %s -> %s",gst_element_state_get_name(oldstate),gst_element_state_get_name(newstate));
          switch(GST_STATE_TRANSITION(oldstate,newstate)) {
            case GST_STATE_CHANGE_READY_TO_PAUSED: {
              GST_DEBUG_BIN_TO_DOT_FILE ((GstBin *)bin, graph_details, "ready_to_paused");
              /* we want to play for 2 sec. */
              if(!gst_element_send_event (bin, gst_event_new_seek (1.0, 
                  GST_FORMAT_TIME,
                  GST_SEEK_FLAG_NONE,
                  GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE,
                  GST_SEEK_TYPE_SET, 2 * GST_SECOND))) {
                GST_WARNING_OBJECT(bin, "seek failed");
              } else {
                GST_INFO_OBJECT(bin, "seek done");
              }
              break;
            }
            case GST_STATE_CHANGE_PAUSED_TO_PLAYING: {
              GstFormat fmt=GST_FORMAT_TIME;
              gint64 dur;
      
              GST_DEBUG_BIN_TO_DOT_FILE ((GstBin *)bin, graph_details, "paused_to_playing");
              if(gst_element_query_duration (bin, &fmt, &dur)) {
                GST_INFO_OBJECT(bin, "duration qry: %"GST_TIME_FORMAT,
                    GST_TIME_ARGS(dur));
              } else {
                GST_INFO_OBJECT(bin, "duration qry failed");
              }
            }
            default:
              break;
          }
        }
        break;
      case GST_MESSAGE_WARNING:
      case GST_MESSAGE_ERROR: {
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

gint
main (gint argc, gchar **argv)
{
  GstElement *bin;
  GstElement *src, *sink;
  
  /* init gstreamer */
  gst_init (&argc, &argv);

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "seekinit", 0,
      "initial seek test");

  /* create a new bin to hold the elements */
  bin = gst_pipeline_new ("song");

  /* make a sink */
  if(!(sink = gst_element_factory_make ("autoaudiosink", "sink"))) {
    GST_WARNING ("Can't create element \"autoaudiosink\"");
    return -1;
  }

  /* make source and configure */
  if (!(src = gst_element_factory_make ("audiotestsrc", "src"))) {
    GST_WARNING ("Can't create element \"audiotestsrc\"");
    return -1;
  }
  g_object_set (src, "wave", /* saw */ 2, "blocksize", 22050, NULL);

  /* add objects to the main bin */
  gst_bin_add_many (GST_BIN (bin), src, sink, NULL);
  
  /* link elements */
  if (!gst_element_link_many (src, sink, NULL)) {
    GST_WARNING ("Can't link elements");
    return -1;
  }
  
  /* prepare playback */
  gst_element_set_state (bin, GST_STATE_PAUSED);
  GST_INFO ("prepared");
  
  /* play and wait */
  gst_element_set_state (bin, GST_STATE_PLAYING);
  GST_INFO ("playing");
  
  event_loop (bin);
  
  /* stop and cleanup */
  gst_element_set_state (bin, GST_STATE_NULL);
  gst_object_unref (GST_OBJECT (bin));
  return 0;
}
