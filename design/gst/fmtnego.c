/** $Id: fmtnego.c,v 1.3 2007-05-13 19:42:59 ensonic Exp $
 * test dynamic format negotiation (http://bugzilla.gnome.org/show_bug.cgi?id=418982)
 *
 * gcc -Wall -g `pkg-config gstreamer-0.10 --cflags --libs` fmtnego.c -o fmtnego
 * GST_DEBUG="*:2,adder:3,audioconvert:3,default*:3" ./fmtnego
 * GST_DEBUG="*:2,default*:3" ./fmtnego
 *
 * gst-launch audiotestsrc freq=100 ! audioconvert ! adder name=m ! audioconvert ! alsasink audiotestsrc freq=1000 ! audiopanorama panorama=0.5 ! audioconvert ! m.
 *
 * src1 -> conv1 \
 *                + mix -> caps-filter -> conv3 -> sink
 * src2 -> conv2 /
 *
 */

#include <stdio.h>
#include <string.h>
#include <gst/gst.h>

#define SINK_NAME "alsasink"
#define SRC_NAME "audiotestsrc"

#define WAIT_LENGTH 4

/* test application */

struct FmtData {
  GstElement *fmtflt;

  gint type; /* int/float */
  gint channels;
  gint width;
  gint depth;
};

static void message_received (GstBus * bus, GstMessage * message, GstPipeline * pipeline) {
  const GstStructure *s;

  s = gst_message_get_structure (message);
  g_print ("message from \"%s\" (%s): ",
      GST_STR_NULL (GST_ELEMENT_NAME (GST_MESSAGE_SRC (message))),
      gst_message_type_get_name (GST_MESSAGE_TYPE (message)));
  if (s) {
    gchar *sstr;

    sstr = gst_structure_to_string (s);
    printf ("%s\n", sstr);
    g_free (sstr);
  }
  else {
    printf ("no message details\n");
  }
}

static void on_format_negotiated(GstPad *pad,GParamSpec *arg,gpointer user_data) {
  GstCaps *pad_caps;
  struct FmtData *fmt_data=(struct FmtData *)user_data;

  if((pad_caps=gst_pad_get_negotiated_caps(pad))) {
    GstCaps *target_caps,*new_caps=NULL;
    GstStructure *ns;
    gboolean channels,width,depth,name=FALSE;
    const gchar *p_name=NULL;

    GST_INFO("caps negotiated for pad %p, %" GST_PTR_FORMAT, pad, pad_caps);

    g_object_get(fmt_data->fmtflt,"caps",&target_caps,NULL);
    if(!target_caps || !gst_caps_get_size(target_caps)) {
      GstStructure *ps;
      GST_INFO("no target caps yet, setting pad caps");

      /* limmit range by what we got */
      if((ps=gst_caps_get_structure(pad_caps,0))) {
        channels=gst_structure_get_int(ps,"channels",&fmt_data->channels);
        width=gst_structure_get_int(ps,"width",&fmt_data->width);
        depth=gst_structure_get_int(ps,"depth",&fmt_data->depth);

        p_name=gst_structure_get_name(ps);
        if(!strcmp(p_name,"audio/x-raw-int")) fmt_data->type=0;
        else if(!strcmp(p_name,"audio/x-raw-float")) fmt_data->type=1;

        new_caps=gst_caps_make_writable(pad_caps);
      }
      else {
        gst_caps_unref(pad_caps);
      }
    }
    else {
      GstStructure *ps;
      GST_INFO("target caps %d, pad caps %d",gst_caps_get_size(target_caps),gst_caps_get_size(pad_caps));

      /* max limmit range by what we just got and what we previously set */
      if((ps=gst_caps_get_structure(pad_caps,0))) {
        gint p_channels,p_width,p_depth;

        channels=gst_structure_get_int(ps,"channels",&p_channels);
        GST_INFO("target channels %d, pad channels %d",fmt_data->channels,p_channels);
        fmt_data->channels=MAX(fmt_data->channels,p_channels);
        width=gst_structure_get_int(ps,"width",&p_width);
        GST_INFO("target width %d, pad width %d",fmt_data->width,p_width);
        fmt_data->width=MAX(fmt_data->width,p_width);
        depth=gst_structure_get_int(ps,"depth",&p_depth);
        GST_INFO("target depth %d, pad depth %d",fmt_data->depth,p_depth);
        fmt_data->depth=MAX(fmt_data->depth,p_depth);

        p_name=gst_structure_get_name(ps);
        if(!strcmp(p_name,"audio/x-raw-float")) {
          if(fmt_data->type<1) {
            fmt_data->type=1;
            name=TRUE;
          }
        }

        new_caps=gst_caps_make_writable(target_caps);
      }
      gst_caps_unref(pad_caps);
    }
    if(new_caps) {
      ns=gst_caps_get_structure(new_caps,0);
      if(name)
        gst_structure_set_name(ns,p_name);
      if(channels)
        gst_structure_set(ns,"channels",GST_TYPE_INT_RANGE,fmt_data->channels,8,NULL);
      if(width)
        gst_structure_set(ns,"width",GST_TYPE_INT_RANGE,fmt_data->width,32,NULL);
      if(depth)
        gst_structure_set(ns,"depth",GST_TYPE_INT_RANGE,fmt_data->depth,32,NULL);

      GST_INFO("set new caps %" GST_PTR_FORMAT, new_caps);

      g_object_set(fmt_data->fmtflt,"caps",new_caps,NULL);
      gst_caps_unref(new_caps);
    }
  }
}

int main(int argc, char **argv) {
  GstElement *bin;
  /* elements used in pipeline */
  GstElement *sink,*fmtflt;
  GstElement *mix,*pan;
  GstElement *conv1,*conv2,*conv3;
  GstElement *src1,*src2;
  GstPad *conv1_sink_pad,*conv2_sink_pad;
  GstClock *clock;
  GstClockID clock_id;
  GstClockReturn wait_ret;
  GstBus *bus;
  struct FmtData fmt_data;

  /* init gstreamer */
  gst_init(&argc, &argv);
  g_log_set_always_fatal(G_LOG_LEVEL_WARNING);

  /* create a new bin to hold the elements */
  bin = gst_pipeline_new ("song");
  clock = gst_pipeline_get_clock (GST_PIPELINE (bin));

  /* make a sink */
  if(!(sink = gst_element_factory_make (SINK_NAME, "sink"))) {
    fprintf(stderr,"Can't create element \""SINK_NAME"\"\n");exit (-1);
  }
  /* make the caps filter */
  if(!(fmtflt = gst_element_factory_make ("capsfilter", "fmtflt"))) {
    fprintf(stderr,"Can't create element \"capsfilter\"\n");exit (-1);
  }

  /* make sources */
  if(!(src1 = gst_element_factory_make (SRC_NAME, "src1"))) {
    fprintf(stderr,"Can't create element \""SRC_NAME"\"\n");exit (-1);
  }
  g_object_set(src1,"freq",440.0,"wave",2,NULL);
  if(!(src2 = gst_element_factory_make (SRC_NAME, "src2"))) {
    fprintf(stderr,"Can't create element \""SRC_NAME"\"\n");exit (-1);
  }
  g_object_set(src2,"freq",110.0,"wave",1,NULL);
  /* make effect */
  if(!(pan = gst_element_factory_make ("audiopanorama", "pan"))) {
    fprintf(stderr,"Can't create element \"audiopanorama\"\n");exit (-1);
  }
  g_object_set(pan,"panorama",0.5,NULL);
  /* make mixer */
  if(!(mix = gst_element_factory_make ("adder", "mix"))) {
    fprintf(stderr,"Can't create element \"adder\"\n");exit (-1);
  }
  /* make converters */
  if(!(conv1 = gst_element_factory_make ("audioconvert", "conv1"))) {
    fprintf(stderr,"Can't create element \"audioconvert\"\n");exit (-1);
  }
  if(!(conv2 = gst_element_factory_make ("audioconvert", "conv2"))) {
    fprintf(stderr,"Can't create element \"audioconvert\"\n");exit (-1);
  }
  if(!(conv3 = gst_element_factory_make ("audioconvert", "conv3"))) {
    fprintf(stderr,"Can't create element \"audioconvert\"\n");exit (-1);
  }

  fmt_data.fmtflt=fmtflt;

  if((conv1_sink_pad=gst_element_get_pad(conv1,"sink"))) {
    g_signal_connect(conv1_sink_pad,"notify::caps",G_CALLBACK(on_format_negotiated),(gpointer)&fmt_data);
  }
  gst_object_unref(conv1_sink_pad);
  if((conv2_sink_pad=gst_element_get_pad(conv2,"sink"))) {
    g_signal_connect(conv2_sink_pad,"notify::caps",G_CALLBACK(on_format_negotiated),(gpointer)&fmt_data);
  }
  gst_object_unref(conv2_sink_pad);

  /* add objects to the main bin */
  gst_bin_add_many (GST_BIN (bin), src1, src2, pan, mix, conv1, conv2, conv3, fmtflt, sink, NULL);

  /* link elements */
  if(!gst_element_link_many (src1, conv1, mix, fmtflt, conv3, sink, NULL)) {
    fprintf(stderr,"Can't link src1->sink\n");exit (-1);
  }
  if(!gst_element_link_many (src2, pan, conv2, mix, NULL)) {
    fprintf(stderr,"Can't link src1->sink\n");exit (-1);
  }

  /* see if we get errors */
  bus = gst_pipeline_get_bus (GST_PIPELINE (bin));
  gst_bus_add_signal_watch_full (bus, G_PRIORITY_HIGH);
  g_signal_connect (bus, "message::error", G_CALLBACK(message_received), bin);
  g_signal_connect (bus, "message::warning", G_CALLBACK(message_received), bin);

  /* prepare playing */
  if(gst_element_set_state (bin, GST_STATE_PAUSED)==GST_STATE_CHANGE_FAILURE) {
    fprintf(stderr,"Can't prepare playing\n");exit(-1);
  }

  /* make a clock */
  clock_id = gst_clock_new_periodic_id (clock,
    gst_clock_get_time (clock)+(WAIT_LENGTH * GST_SECOND), (WAIT_LENGTH * GST_SECOND));

  /* do the format negotiation test */

  /* start playing */
  puts("play ===================================================================\n");
  if(gst_element_set_state (bin, GST_STATE_PLAYING)==GST_STATE_CHANGE_FAILURE) {
    fprintf(stderr,"Can't start playing\n");exit(-1);
  }
  if ((wait_ret = gst_clock_id_wait (clock_id, NULL)) != GST_CLOCK_OK) {
    fprintf(stderr,"clock_id_wait returned: %d\n", wait_ret);
  }

  /* stop the pipeline */
  puts("playing done ===========================================================\n");
  gst_element_set_state (bin, GST_STATE_NULL);

  /* we don't need a reference to these objects anymore */
  gst_object_unref (GST_OBJECT (bus));
  gst_object_unref (GST_OBJECT (clock));
  gst_object_unref (GST_OBJECT (bin));

  exit (0);
}
