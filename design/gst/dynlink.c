/** $Id: dynlink.c,v 1.2 2007-05-07 16:34:33 ensonic Exp $
 * test dynamic linking
 *
 * gcc -Wall -g `pkg-config gstreamer-0.10 --cflags --libs` dynlink.c -o dynlink
 * GST_DEBUG="*:3" ./dynlink
 */

/* TODO:
 * - insert multiple elements
 * - handle new segment
 * THOUGHTS
 * - what about making the fragment only one element (or bin)
 *   this is easier to sync with the pipeline state
 * - regading the tags, what about sending a tag-query downstream
 *   and doing send_event(new_elem,tag_event) before linking it in
 */

#include <stdio.h>
#include <gst/gst.h>

#define SINK_NAME "alsasink"
#define PROC_NAME "volume"
#define SRC_NAME "audiotestsrc"

#define WAIT_LENGTH 2


/* live element (re)linking
 * We have a running pipeline with elements running_1...n and a pipeline
 * fragment with elements fragment_1..n. The point of operations is determined
 * by the src/sink pads of the elemnts.
 *
 * If a fragments spans multiple elements, those already have to be linked.
 * The fragment-elements need to be in PAUSED (READY?) if to be inserted into a
 * PLAYING pipeline.
 *
 * To use the api one has to gather NEWSEGMENT and TAG events on running_1 and
 * resend them to the fragment (when)
 *
 * Issues: it could be that we have to make these function async and call a user
 * provided callback after completion
 */

static gboolean
get_state(GstPad *run_src, GstPad *run_sink, GstState *state)
{
  gboolean res = FALSE;
  GstElement *elem;
  GstStateChangeReturn sc_ret;

  GST_INFO ("trying to get state from %p, %p",run_src,run_sink);

  if (run_src) {
    if (!(elem = GST_ELEMENT(gst_pad_get_parent (run_src))))
      return FALSE;
  }
  else if (run_sink) {
    if (!(elem = GST_ELEMENT(gst_pad_get_parent (run_sink))))
      return FALSE;
  }
  else {
    return FALSE;
  }

  sc_ret = gst_element_get_state (elem, state, NULL, G_GUINT64_CONSTANT (0));
  if (sc_ret == GST_STATE_CHANGE_FAILURE)
    goto state_change_fail;

  res = TRUE;
done:
  gst_object_unref (elem);
  return res;

  /* Errors */
state_change_fail:
  GST_INFO ("can't get state: %s",gst_element_state_get_name(sc_ret));
  goto done;

}

static gboolean
open_link (GstPad *run_src, GstPad *run_sink, gboolean is_playing)
{
  GstPad *run_src_peer = NULL;
  GstPad *run_sink_peer = NULL;

  GST_INFO ("opening link %p, %p",run_src,run_sink);

  if(run_src) {
    /* block src pad of running pipeline */
    if (is_playing)
      gst_pad_set_blocked (run_src, TRUE);

    run_src_peer = gst_pad_get_peer (run_src);
    /* unlink first set of pads */
    if (run_src_peer)
      gst_pad_unlink (run_src, run_src_peer);
  }
  if (run_sink && run_src_peer != run_sink) {
    /* TODO: flush data
    if (is_playing) {
      pad_probe_on(run_sink_peer);
      gst_pad_send_event(run_src_peer, gst_event_new_eos());
      wait_for_eos_in_probe();
    }
    */
    run_sink_peer = gst_pad_get_peer (run_sink);
    /* unlink second set of pads */
    if (run_sink_peer)
      gst_pad_unlink (run_sink_peer, run_sink);
  }

  if (run_src_peer)
    gst_object_unref(run_src_peer);
  if (run_sink_peer)
    gst_object_unref(run_sink_peer);
  return TRUE;
}

/**
 *
 *
 * Insert elements or swapp out elements
 *
 * we have: run_src ! run_sink
 * we have: run_src ! frag_1sink ! frag_2src ! run_sink
 * we have: run_src ! frag_1sink ! frag_1src ! run_sink
 *
 * we want: run_src ! frag_3sink ! frag_3src ! run_sink
 * we want: run_src ! frag_3sink ! frag_4src ! run_sink
 */
gboolean
gst_pads_insert_link (GstPad *run_src, GstPad *frag_sink, GstPad *frag_src, GstPad *run_sink)
{
  gboolean res = FALSE;
  GstPadLinkReturn link_ret;
  GstState state;

  if (!get_state (run_src, run_sink, &state))
    goto state_change_fail;

  if (!open_link (run_src, run_sink, (state==GST_STATE_PLAYING)))
    goto not_connected;

  if (run_src && frag_sink) {
    link_ret = gst_pad_link (run_src, frag_sink);
    if (GST_PAD_LINK_FAILED(link_ret))
      goto link_failed;
  }
  if (frag_src && run_sink) {
    link_ret = gst_pad_link (frag_src, run_sink);
    if (GST_PAD_LINK_FAILED(link_ret))
      goto link_failed;
  }
  /* sync fragment
    GstQuery *query;
    gdouble rate;
    GstFormat format;
    gint64 start_value, stop_value;
    GstEvent *event;

    query=gst_query_new_segment(GST_FORMAT_DEFAULT);
    gst_element_query(elem, query);
    gst_query_parse_segment(query, &rate, &format, &start_value, &stop_value);
    gst_query_unref(query);
    event = gst_event_new_seek(rate, format, GST_SEEK_FLAG_NONE,
                               GST_SEEK_TYPE_SET, start_value,
                               GST_SEEK_TYPE_SET, stop_value);
    //event = gst_event_new_new_segment(rate, format, start_value, stop_value, position);

    // this is tough, how do we iterate over all elements
    foreach(element in frag_sink.parent,frag_src.parent) {
      gst_element_change_state (element, state);
      gst_element_send_event (element, gst_event_ref (event));
    }
    gst_event_unref (event);
  */
  /* FIXME: this is not sufficient (assumes one element) */
  GstElement *new_elem = NULL;
  if (frag_sink) {
    new_elem=GST_ELEMENT(gst_pad_get_parent (frag_sink));
  }
  else if (frag_src) {
    new_elem=GST_ELEMENT(gst_pad_get_parent (frag_src));
  }
  if (new_elem) {
    gst_element_set_state (new_elem, state);
    gst_object_unref (new_elem);
  }

  /* unblock src pad of running pipeline */
  if (run_src && (state==GST_STATE_PLAYING))
    gst_pad_set_blocked (run_src, FALSE);
  res = TRUE;

done:
  return res;

  /* Errors */
state_change_fail:
  GST_INFO ("can't get state");
  goto done;
not_connected:
  GST_INFO ("pads are not connected");
  goto done;
link_failed:
  GST_INFO ("pad-link failed %d",link_ret);
  goto done;
}

/**
 *
 *
 * Remove elements
 * we have: run_src ! frag_1sink ! frag_2src ! run_sink
 * we have: run_src ! frag_1sink ! frag_1src ! run_sink
 *
 * we want: run_src ! run_sink
 */
gboolean
gst_pads_remove_link (GstPad *run_src, GstPad *run_sink)
{
  gboolean res = FALSE;
  GstPadLinkReturn link_ret;
  GstState state;

  if (!get_state (run_src, run_sink, &state))
    goto state_change_fail;

  if (!open_link (run_src, run_sink, (state==GST_STATE_PLAYING)))
    goto not_connected;

  link_ret = gst_pad_link (run_src, run_sink);
  if (GST_PAD_LINK_FAILED(link_ret))
    goto link_failed;

  /* unblock src pad of running pipeline */
  if (run_src && (state==GST_STATE_PLAYING))
    gst_pad_set_blocked (run_src, FALSE);
  res = TRUE;

done:
  return res;

  /* Errors */
state_change_fail:
  GST_INFO ("can't get state");
  goto done;
not_connected:
  GST_INFO ("pads are not connected");
  goto done;
link_failed:
  GST_INFO ("pad-link failed %d",link_ret);
  goto done;
}


/* test application */

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

int main(int argc, char **argv) {
  GstElement *bin;
  /* elements used in pipeline */
  GstElement *sink;
  GstElement *vol;
  GstElement *src1,*src2;
  GstClock *clock;
  GstClockID clock_id;
  GstClockReturn wait_ret;
  GstPad *src1_src,*src2_src;
  GstPad *sink_sink;
  GstPad *vol_src,*vol_sink;
  GstBus *bus;

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
  if(!(vol = gst_element_factory_make (PROC_NAME, "vol"))) {
    fprintf(stderr,"Can't create element \""PROC_NAME"\"\n");exit (-1);
  }
  g_object_set(vol,"volume",0.25,NULL);


  /* add objects to the main bin */
  gst_bin_add_many (GST_BIN (bin), src1, sink, NULL);

  /* link elements */
  if(!gst_element_link_many (src1, sink, NULL)) {
    fprintf(stderr,"Can't link src->sink\n");exit (-1);
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

  /* get pads */
  if(!(src1_src=gst_element_get_pad(src1,"src"))) {
    fprintf(stderr,"Can't get src pad of src1\n");exit (-1);
  }
  if(!(src2_src=gst_element_get_pad(src2,"src"))) {
    fprintf(stderr,"Can't get src pad of src2\n");exit (-1);
  }
  if(!(sink_sink=gst_element_get_pad(sink,"sink"))) {
    fprintf(stderr,"Can't get sink pad of sink\n");exit (-1);
  }
  if(!(vol_src=gst_element_get_pad(vol,"src"))) {
    fprintf(stderr,"Can't get src pad of vol\n");exit (-1);
  }
  if(!(vol_sink=gst_element_get_pad(vol,"sink"))) {
    fprintf(stderr,"Can't get sink pad of vol\n");exit (-1);
  }

  /* make a clock */
  clock_id = gst_clock_new_periodic_id (clock,
    gst_clock_get_time (clock)+(WAIT_LENGTH * GST_SECOND), (WAIT_LENGTH * GST_SECOND));

  /* start playing */
  if(gst_element_set_state (bin, GST_STATE_PLAYING)==GST_STATE_CHANGE_FAILURE) {
    fprintf(stderr,"Can't start playing\n");exit(-1);
  }

  /* do the linking tests */

  puts("play src ===============================================================\n");
  if ((wait_ret = gst_clock_id_wait (clock_id, NULL)) != GST_CLOCK_OK) {
    fprintf(stderr,"clock_id_wait returned: %d\n", wait_ret);
  }

  puts("trying to insert vol between src1 and sink ==============================\n");
  gst_bin_add(GST_BIN (bin),vol);
  if(gst_element_set_state (vol, GST_STATE_PAUSED)==GST_STATE_CHANGE_FAILURE) {
    fprintf(stderr,"Can't prepare playing\n");exit(-1);
  }
  gst_pads_insert_link(src1_src,vol_sink,vol_src,sink_sink);

  puts("inserting done =========================================================\n");
  if ((wait_ret = gst_clock_id_wait (clock_id, NULL)) != GST_CLOCK_OK) {
    fprintf(stderr,"clock_id_wait returned: %d\n", wait_ret);
  }

  puts("trying to swap src1 and src2 ===========================================\n");
  gst_bin_add(GST_BIN (bin),src2);
  if(gst_element_set_state (src2, GST_STATE_PAUSED)==GST_STATE_CHANGE_FAILURE) {
    fprintf(stderr,"Can't prepare playing\n");exit(-1);
  }
  gst_pads_insert_link(NULL,NULL,src2_src,vol_sink);
  if(gst_element_set_state (src1, GST_STATE_NULL)==GST_STATE_CHANGE_FAILURE) {
    fprintf(stderr,"Can't stop playing\n");exit(-1);
  }
  gst_bin_remove(GST_BIN (bin),src1);

  puts("inserting done =========================================================\n");
  if ((wait_ret = gst_clock_id_wait (clock_id, NULL)) != GST_CLOCK_OK) {
    fprintf(stderr,"clock_id_wait returned: %d\n", wait_ret);
  }

  puts("trying to remove vol from src2 and sink =================================\n");
  gst_pads_remove_link(src2_src,sink_sink);
  if(gst_element_set_state (vol, GST_STATE_NULL)==GST_STATE_CHANGE_FAILURE) {
    fprintf(stderr,"Can't stop playing\n");exit(-1);
  }
  gst_bin_remove(GST_BIN (bin),vol);

  puts("removing done ==========================================================\n");
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
