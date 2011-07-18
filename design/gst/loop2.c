/** $Id$
 * test seemles looping in gstreamer
 *
 * In buzztard the loops are not smooth. The code below tries to reproduce the
 * issue.
 *
 * Things we excluded:
 * - it does not seem to get worse if we use a lot of fx
 * - it does not matter wheter fx is any of volume,queue,adder
 * - it is not the low-latency setting on the sink
 * - it does not seem to be the bins
 * - it is not the cpu load
 *
 * Things we noticed:
 * - on the netbook the loops are smoother when using alsasink, compared to
 *   pulsesink (CPU load is simmilar and less than 100% in both cases)
 *
 * What we can still try here:
 * - pad probes
 * - <add your idea here>
 *
 * What we can still try in buzztard:
 * - monitor timestamps
 * - <add your idea here>
 *
 * Theory 1:
 * - when the loop-end has ben reached the sink we receive SEGMENT_DONE on the
 *   application
 * - we then send a new seek event
 * - in the mean time the pipeline got drained
 * - we don't manage to catch up with refilling it in time
 *   (especially on low-latency)
 *
 * Theory 2:
 * - could we suffer from some timestamp mess-up (out of segment ts)
 *
 * gcc -g `pkg-config gstreamer-0.10 gstreamer-controller-0.10 --cflags --libs` loop2.c -o loop2
 */

#include <stdio.h>
#include <gst/gst.h>
#include <gst/controller/gstcontroller.h>

#define SINK_NAME "pulsesink"
#define SRC_NAME "audiotestsrc"
#define FX_NAME "volume"

#define MAX_NUM_FX 30

static GMainLoop *main_loop=NULL;
static GstEvent *play_seek_event=NULL;
static GstEvent *loop_seek_event=NULL;

static void message_received (GstBus * bus, GstMessage * message, GstPipeline * pipeline) {
  const GstStructure *s;

  s = gst_message_get_structure (message);
  g_print ("message from \"%s\" (%s): ",
      GST_STR_NULL (GST_ELEMENT_NAME (GST_MESSAGE_SRC (message))),
      gst_message_type_get_name (GST_MESSAGE_TYPE (message)));
  if (s) {
    gchar *sstr;

    sstr = gst_structure_to_string (s);
    puts (sstr);
    g_free (sstr);
  }
  else {
    puts ("no message details");
  }

  g_main_loop_quit(main_loop);
}

static void state_changed(const GstBus * const bus, GstMessage *message,  GstElement *bin) {
  if(GST_MESSAGE_SRC(message) == GST_OBJECT(bin)) {
    GstStateChangeReturn res;
    GstState oldstate,newstate,pending;

    gst_message_parse_state_changed(message,&oldstate,&newstate,&pending);
    switch(GST_STATE_TRANSITION(oldstate,newstate)) {
      case GST_STATE_CHANGE_READY_TO_PAUSED:
        GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(bin),GST_DEBUG_GRAPH_SHOW_ALL,"loop2");
        // seek to start time
        puts("initial seek ===========================================================\n");
        if(!(gst_element_send_event(bin,play_seek_event))) {
          fprintf(stderr,"element failed to handle seek event");
          g_main_loop_quit(main_loop);
        }
        // start playback
        puts("start playing ==========================================================\n");
        res=gst_element_set_state(bin,GST_STATE_PLAYING);
        if(res==GST_STATE_CHANGE_FAILURE) {
          fprintf(stderr,"can't go to playing state\n");
          g_main_loop_quit(main_loop);
        }
        else if(res==GST_STATE_CHANGE_ASYNC) {
          puts("->PLAYING needs async wait");
        }
        break;
      case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
        puts("playback started =======================================================\n");
        break;
      default:
        break;
    }
  }
}

static void segment_done(const GstBus * const bus, const GstMessage * const message,  GstElement *bin) {
  puts("loop playback ==========================================================\n");
  if(!(gst_element_send_event(bin,gst_event_ref(loop_seek_event)))) {
    fprintf(stderr,"element failed to handle continuing play seek event\n");
    g_main_loop_quit(main_loop);
  }
}

#if 0
static GstElement *make_block(void) {
  GstElement *e;
  
  e=gst_element_factory_make (FX_NAME, NULL);
  return e;
}
#endif
static GstElement *make_block(void) {
  GstElement *e,*b;
  GstPad *tp,*gp;
  
  b=gst_bin_new(NULL);
  e=gst_element_factory_make ("volume", NULL);
  g_object_set(e,"volume",0.95,NULL);
  gst_bin_add(GST_BIN(b),e);
  
  /* pads */
  tp=gst_element_get_static_pad(e,"src");
  gp=gst_ghost_pad_new("src",tp);
  gst_pad_set_active(gp,TRUE);
  gst_element_add_pad(b,gp);
  
  tp=gst_element_get_static_pad(e,"sink");
  gp=gst_ghost_pad_new("sink",tp);
  gst_pad_set_active(gp,TRUE);
  gst_element_add_pad(b,gp);
  return b;
}


int main(int argc, char **argv) {
  GstElement *bin;
  /* elements used in pipeline */
  GstElement *src,*fx[MAX_NUM_FX],*sink;
  GstController *ctrl;
  GstBus *bus;
  GstStateChangeReturn res;
  GValue val = { 0, };
  gint i,num_fx=2;
  gint64 chunk;
  
  if(argc>1) {
    num_fx=atoi(argv[1]);
    if(num_fx>=MAX_NUM_FX) num_fx=MAX_NUM_FX-1;
    if(num_fx<1) num_fx=1;
  }
    
  printf("using %d fx element(s)\n",num_fx);

  /* init gstreamer */
  gst_init(&argc, &argv);
  g_log_set_always_fatal(G_LOG_LEVEL_WARNING);

  /* create a new bin to hold the elements */
  bin = gst_pipeline_new ("song");
  /* see if we get errors */
  bus = gst_pipeline_get_bus (GST_PIPELINE (bin));
  gst_bus_add_signal_watch_full (bus, G_PRIORITY_HIGH);
  g_signal_connect (bus, "message::error", G_CALLBACK(message_received), bin);
  g_signal_connect (bus, "message::warning", G_CALLBACK(message_received), bin);
  g_signal_connect (bus, "message::eos", G_CALLBACK(message_received), bin);
  g_signal_connect (bus, "message::segment-done", G_CALLBACK(segment_done), bin);
  g_signal_connect (bus, "message::state-changed", G_CALLBACK(state_changed), bin);
  gst_object_unref (G_OBJECT (bus));

  main_loop=g_main_loop_new(NULL,FALSE);

  /* make elements and add them to the bin */
  if(!(src = gst_element_factory_make (SRC_NAME, NULL))) {
    fprintf(stderr,"Can't create element \""SRC_NAME"\"\n");exit (-1);
  }
  g_object_set (src, "wave", 2, NULL);
  gst_bin_add (GST_BIN (bin), src);
  for(i=0;i<num_fx;i++) {
    if(!(fx[i] = make_block ())) {
      fprintf(stderr,"Can't create element \""FX_NAME"\"\n");exit (-1);
    }
    gst_bin_add (GST_BIN (bin), fx[i]);
  }
  if(!(sink = gst_element_factory_make (SINK_NAME, "sink"))) {
    fprintf(stderr,"Can't create element \""SINK_NAME"\"\n");exit (-1);
  }
  chunk=GST_TIME_AS_USECONDS((GST_SECOND*60)/(120*4));
  printf("changing audio chunk-size for sink to %"G_GUINT64_FORMAT" µs = %"G_GUINT64_FORMAT" ms\n",
    chunk, (chunk/G_GINT64_CONSTANT(1000)));
  g_object_set(sink,
    "latency-time",chunk,
    "buffer-time",chunk<<1,
    NULL);
  gst_bin_add (GST_BIN (bin), sink);

  /* link elements */
  if(!gst_element_link (src, fx[0])) {
    fprintf(stderr,"Can't link elements\n");exit (-1);
  }
  for(i=1;i<num_fx;i++) {
    if(!gst_element_link (fx[i-1],fx[i])) {
      fprintf(stderr,"Can't link elements\n");exit (-1);
    }
  }
  if(!gst_element_link (fx[i-1],sink)) {
    fprintf(stderr,"Can't link elements\n");exit (-1);
  }

  /* prepare controller queues */
  g_value_init (&val, G_TYPE_DOUBLE);

  /* add a controller to the source */
  if (!(ctrl = gst_controller_new (G_OBJECT (src), "freq", "volume", NULL))) {
    fprintf(stderr,"can't control source element");exit (-1);
  }
  /* set interpolation */
  gst_controller_set_interpolation_mode (ctrl, "volume", GST_INTERPOLATE_LINEAR);
  gst_controller_set_interpolation_mode (ctrl, "freq", GST_INTERPOLATE_LINEAR);
  /* set control values */
  g_value_set_double (&val, 1.0);
  gst_controller_set (ctrl, "volume",   0 * GST_MSECOND, &val);
  g_value_set_double (&val, 0.0);
  gst_controller_set (ctrl, "volume", 249 * GST_MSECOND, &val);
  g_value_set_double (&val, 1.0);
  gst_controller_set (ctrl, "volume", 250 * GST_MSECOND, &val);
  g_value_set_double (&val, 0.0);
  gst_controller_set (ctrl, "volume", 499 * GST_MSECOND, &val);
  g_value_set_double (&val, 1.0);
  gst_controller_set (ctrl, "volume", 500 * GST_MSECOND, &val);
  g_value_set_double (&val, 0.0);
  gst_controller_set (ctrl, "volume", 749 * GST_MSECOND, &val);
  g_value_set_double (&val, 1.0);
  gst_controller_set (ctrl, "volume", 750 * GST_MSECOND, &val);
  g_value_set_double (&val, 0.0);
  gst_controller_set (ctrl, "volume", 999 * GST_MSECOND, &val);

  g_value_set_double (&val, 110.0);
  gst_controller_set (ctrl, "freq",   0 * GST_MSECOND, &val);
  g_value_set_double (&val, 440.0);
  gst_controller_set (ctrl, "freq", 999 * GST_MSECOND, &val);

  /* initial seek event */
  play_seek_event = gst_event_new_seek(1.0, GST_FORMAT_TIME,
        GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SEGMENT,
        GST_SEEK_TYPE_SET, (GstClockTime)0,
        GST_SEEK_TYPE_SET, (GstClockTime)GST_SECOND);
  /* loop seek event (without flush) */
  loop_seek_event = gst_event_new_seek(1.0, GST_FORMAT_TIME,
        GST_SEEK_FLAG_SEGMENT,
        GST_SEEK_TYPE_SET, (GstClockTime)0,
        GST_SEEK_TYPE_SET, (GstClockTime)GST_SECOND);

  /* prepare playing */
  puts("prepare playing ========================================================\n");
  res=gst_element_set_state (bin, GST_STATE_PAUSED);
  if(res==GST_STATE_CHANGE_FAILURE) {
    fprintf(stderr,"Can't go to paused\n");exit(-1);
  }
  else if(res==GST_STATE_CHANGE_ASYNC) {
    puts("->PAUSED needs async wait");
  }
  g_main_loop_run(main_loop);

  /* stop the pipeline */
  puts("exiting ================================================================\n");
  gst_element_set_state (bin, GST_STATE_NULL);

  /* we don't need a reference to these objects anymore */
  gst_object_unref (GST_OBJECT (bin));
  g_main_loop_unref(main_loop);

  exit (0);
}
