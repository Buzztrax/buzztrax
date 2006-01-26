/* $Id: seek1.c,v 1.2 2006-01-26 17:04:50 ensonic Exp $
 *
 * Build a pipeline with testaudiosource->alsasink
 * and sweep frequency and volume
 *
 * Use seeks to play partially or as a loop
 *
 * gcc -Wall -g `pkg-config gstreamer-0.10 gstreamer-controller-0.10 --cflags --libs` seek1.c -o seek1
 */

#include <gst/gst.h>
#include <gst/controller/gstcontroller.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void
event_loop (GstElement * bin)
{
  GstBus *bus;
  GstMessage *message = NULL;

  bus = gst_element_get_bus (GST_ELEMENT (bin));

  while (TRUE) {
    message = gst_bus_poll (bus, GST_MESSAGE_ANY, -1);

    g_assert (message != NULL);

    switch (message->type) {
      case GST_MESSAGE_EOS:
        gst_message_unref (message);
        return;
      case GST_MESSAGE_SEGMENT_DONE: {
	GstEvent *event;
	
	event = gst_event_new_seek(1.0, GST_FORMAT_TIME,
	    GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SEGMENT,
	    GST_SEEK_TYPE_SET, (GstClockTime)0 * GST_SECOND,
	    GST_SEEK_TYPE_SET, (GstClockTime)6 * GST_SECOND);
	if(!(gst_element_send_event(GST_ELEMENT(bin),event))) {
	  GST_WARNING("element failed to handle seek event");
	}
      } break;
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

/*
static void
eos (GstBus * bus, GstMessage * message, GstPipeline * pipeline)
{
  const GstStructure *s = gst_message_get_structure (message);
  g_print ("message from \"%s\" (%s): ",
      GST_STR_NULL (GST_ELEMENT_NAME (GST_MESSAGE_SRC (message))),
      gst_message_type_get_name (GST_MESSAGE_TYPE (message)));
  if (s) {
    gchar *sstr;

    sstr = gst_structure_to_string (s);
    g_print ("%s\n", sstr);
    g_free (sstr);
  } else {
    g_print ("no message details\n");
  }
}

static void
segment_done (GstBus * bus, GstMessage * message, GstPipeline * pipeline)
{
  const GstStructure *s = gst_message_get_structure (message);
  GstEvent *event;
  
  event = gst_event_new_seek(1.0, GST_FORMAT_TIME,
      GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SEGMENT,
      GST_SEEK_TYPE_SET, (GstClockTime)0 * GST_SECOND,
      GST_SEEK_TYPE_SET, (GstClockTime)6 * GST_SECOND);
  if(!(gst_element_send_event(GST_ELEMENT(pipeline),event))) {
    GST_WARNING("element failed to handle seek event");
  }
  
  g_print ("message from \"%s\" (%s): ",
      GST_STR_NULL (GST_ELEMENT_NAME (GST_MESSAGE_SRC (message))),
      gst_message_type_get_name (GST_MESSAGE_TYPE (message)));
  if (s) {
    gchar *sstr;

    sstr = gst_structure_to_string (s);
    g_print ("%s\n", sstr);
    g_free (sstr);
  } else {
    g_print ("no message details\n");
  }
}
*/

gint
main (gint argc, gchar ** argv)
{
  gint res = 1;
  GstElement *src, *sink;
  GstElement *bin;
  GstController *ctrl;
  GstClock *clock;
  GstClockID clock_id;
  //GstClockReturn wait_ret;
  GValue vol = { 0, };
  GstStateChangeReturn state_res;
  GstEvent *event;
  GstBus *bus;

  // select loop or start offset mode
  gboolean loop=FALSE;

  if(argc>1) {
    if(!strcasecmp("loop",argv[1])) loop=TRUE;
    else if(!strcasecmp("seek",argv[1])) loop=FALSE;
  }
  else {
    puts("Usage: seek1 [loop|seek]\n  using seek by default");
  }

  gst_init (&argc, &argv);
  gst_controller_init (&argc, &argv);

  // build pipeline
  bin = gst_pipeline_new ("pipeline");
  clock = gst_pipeline_get_clock (GST_PIPELINE (bin));
  src = gst_element_factory_make ("audiotestsrc", "gen_audio");
  sink = gst_element_factory_make ("alsasink", "play_audio");
  gst_bin_add_many (GST_BIN (bin), src, sink, NULL);
  if (!gst_element_link (src, sink)) {
    GST_WARNING ("can't link elements");
    goto Error;
  }
  // add a controller to the source
  if (!(ctrl = gst_controller_new (G_OBJECT (src), "freq", "volume", NULL))) {
    GST_WARNING ("can't control source element");
    goto Error;
  }
  // set interpolation
  gst_controller_set_interpolation_mode (ctrl, "volume", GST_INTERPOLATE_LINEAR);
  gst_controller_set_interpolation_mode (ctrl, "freq", GST_INTERPOLATE_LINEAR);

  // set control values
  g_value_init (&vol, G_TYPE_DOUBLE);
  g_value_set_double (&vol, 0.0);
  gst_controller_set (ctrl, "volume", 0 * GST_SECOND, &vol);
  g_value_set_double (&vol, 1.0);
  gst_controller_set (ctrl, "volume", 5 * GST_SECOND, &vol);
  g_value_set_double (&vol, 440.0);
  gst_controller_set (ctrl, "freq", 0 * GST_SECOND, &vol);
  g_value_set_double (&vol, 3520.0);
  gst_controller_set (ctrl, "freq", 3 * GST_SECOND, &vol);
  g_value_set_double (&vol, 880.0);
  gst_controller_set (ctrl, "freq", 6 * GST_SECOND, &vol);

  clock_id =
      gst_clock_new_single_shot_id (clock,
      gst_clock_get_time (clock) + (7 * GST_SECOND));


  // connect to bus
  bus = gst_pipeline_get_bus (GST_PIPELINE (bin));
  gst_bus_add_signal_watch_full (bus, G_PRIORITY_HIGH);

  //g_signal_connect (bus, "message::eos", (GCallback) eos, bin);
  //g_signal_connect (bus, "message::segment-done", (GCallback) segment_done, bin);

  // prepare playback
  if ((state_res = gst_element_set_state(GST_ELEMENT(bin),GST_STATE_PAUSED))==GST_STATE_CHANGE_FAILURE) {
    GST_WARNING("can't go to paused state");
    goto Error;
  }

  if (loop) {
    event = gst_event_new_seek(1.0, GST_FORMAT_TIME,
        GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SEGMENT,
        GST_SEEK_TYPE_SET, (GstClockTime)0 * GST_SECOND,
        GST_SEEK_TYPE_SET, (GstClockTime)6 * GST_SECOND);
  } else {
    event = gst_event_new_seek(1.0, GST_FORMAT_TIME,
        GST_SEEK_FLAG_FLUSH,
        GST_SEEK_TYPE_SET, (GstClockTime)3 * GST_SECOND,
        GST_SEEK_TYPE_SET, (GstClockTime)7 * GST_SECOND);
  }
  if(!(gst_element_send_event(GST_ELEMENT(bin),event))) {
    GST_WARNING("element failed to handle seek event");
  }
  
  // start playback for 7 second
  if ((state_res = gst_element_set_state(GST_ELEMENT(bin),GST_STATE_PLAYING))==GST_STATE_CHANGE_FAILURE) {
    GST_WARNING("can't go to playing state");
    goto Error;
  }

  event_loop (bin);
  /*
  if ((wait_ret = gst_clock_id_wait (clock_id, NULL)) != GST_CLOCK_OK) {
    GST_WARNING ("clock_id_wait returned: %d", wait_ret);
  }
  */

  gst_element_set_state (bin, GST_STATE_NULL);
  res = 0;
Error:
  // cleanup
  gst_object_unref (G_OBJECT (ctrl));
  gst_object_unref (G_OBJECT (clock));
  gst_object_unref (G_OBJECT (bin));
  return (res);
}
