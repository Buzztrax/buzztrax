/* test transport state handling
 *
 * gcc -Wall -g `pkg-config gstreamer-0.10 --cflags --libs` transport.c -o transport
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <gst/gst.h>


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
        gst_message_unref (message);
        return;
      case GST_MESSAGE_REQUEST_STATE: {
        GstState state;
        
        gst_message_parse_request_state(message,&state);
        GST_INFO_OBJECT (GST_MESSAGE_SRC(message),
            "request state on the bin: %s",gst_element_state_get_name(state));
        //if(GST_MESSAGE_SRC(message) == GST_OBJECT(bin)) {
          gst_element_set_state (bin, state);
        //}
        gst_message_unref (message);
      } break;        
      case GST_MESSAGE_STATE_CHANGED:
        if(GST_MESSAGE_SRC(message) == GST_OBJECT(bin)) {
          GstState oldstate,newstate,pending;

          gst_message_parse_state_changed(message,&oldstate,&newstate,&pending);
          GST_INFO("state change on the bin: %s -> %s",gst_element_state_get_name(oldstate),gst_element_state_get_name(newstate));
        }
        gst_message_unref (message);
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
  /* elements used in pipeline */
  GstElement *sink,*src;
  GstState state;
  gint mode = 0;
  
  if (argc>1) {
    switch (argv[1][0]) {
      case 'm':
        mode = 1;
        state = GST_STATE_PLAYING;
        break;
      case 's':
        state = GST_STATE_PAUSED;
        mode = 2;
        break;
      default:
        state = GST_STATE_PLAYING;
        mode = 0;
        break;
    }
  }
  
  /* init gstreamer */
  gst_init (&argc, &argv);
  bin = gst_pipeline_new ("song");
  src = gst_element_factory_make ("audiotestsrc", NULL);
  g_object_set (src, "num-buffers", 500, NULL);
  sink = gst_element_factory_make ("jackaudiosink", NULL);
  g_object_set (sink, "transport", mode, NULL);
  gst_bin_add_many (GST_BIN (bin), src, sink, NULL);
  gst_element_link (src, sink);
  gst_element_set_state (bin, state);
  
  event_loop (bin);
  
  gst_element_set_state (bin, GST_STATE_NULL);
  g_object_unref (bin);
  return 0;
}
