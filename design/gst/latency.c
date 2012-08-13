/* test audio latency
 * the latency tells us how much too early the audio is generated
 *
 * gcc -Wall -g `pkg-config gstreamer-0.10 --cflags --libs` latency.c -o latency
 * GST_DEBUG="latency:4" ./latency >latency.dbg
 * ./latency.sh | gnuplot; eog latency.png
 */
/* observations:
 * - latencies are different:
 *   - for sinks: alsasink > pulsesink
 *   - for sources: simsyn > audiotestsrc
 * - latencies have varying jitter:
 *   <jackaudiosink0> bpm=125, tpb=8, audio chunk-size to 60000 µs = 60 ms
 *     latency = 39887808 ns = 39 ms
 *     latency = 69659863 ns = 69 ms
 *     latency = 92879818 ns = 92 ms
 *     latency = 116099773 ns = 116 ms
 *     latency = 139319727 ns = 139 ms
 *     latency = 162539682 ns = 162 ms
 *     -- reached playing
 *     latency = 174149660 ns = 174 ms
 *     latency = 174149660 ns = 174 ms
 *     latency = 174149660 ns = 174 ms
 *   <pulsesink0> bpm=125, tpb=8, audio chunk-size to 60000 µs = 60 ms
 *     latency = 32601024 ns = 32 ms
 *     latency = 69659863 ns = 69 ms
 *     latency = 92879818 ns = 92 ms
 *     -- reached playing
 *     latency = 116099773 ns = 116 ms
 *     latency = 139319727 ns = 139 ms
 *     latency = 162539682 ns = 162 ms
 * - total_latency = (1 + nr_of_queues) * latency
 * - using a smaller blocksize (half of latency) mitigates the problem somewhat
 *   - resulting latency: target-latency + n-queues * block latency
 * - when using fakesink, our probes are not getting called ?
 * - alsasink fails with some (bpm,tpb) settings (125,4) and then the clock
 *   returns 0 all the time
 */
/* todo:
 * - test how many buffers we do with the clock not yet running for each sink
 *   alsa: 9, pulse: 6, jack: 6
 * - figure out why we do that for more than one buffer
 * - see if we could do a delayed wakeup on the queue
 *   - that would need to know how long it takes on average for upstream to
 *     produce a buffer
 *   - this would make more sense for a threadbarrier element (always one buffer)
 *     - we need to track when we ask upstream to produce a buffer and
 *       how long it takes to make it
 *     - if the buffer-duration is constant, we can delay making the buffers
 * - calculate min,max,avg latencies for each probe-stream
 *   sum=0; cut -d' ' -f2 audiotestsrc0_src_latency.dat | while read v; do sum=`expr $sum + $v`; echo $sum; done | tail -n1
 *
 * - try various sampling rates, right now it is 44100
 */

#include <stdio.h>
#include <stdlib.h>
#include <gst/gst.h>

/* other parameters are at the top of main */
#define USE_QUEUE 1

#define GST_CAT_DEFAULT bt_latency
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

typedef struct Machine_ Machine;
typedef struct Wire_ Wire;
typedef struct Graph_ Graph;

struct Machine_
{
  Graph *g;
  GstBin *bin;
  /* elements */
  GstElement *mix;
  GstElement *elem;
  GstElement *tee;
};

struct Wire_
{
  Graph *g;
  Machine *ms, *md;
  GstBin *bin;
  /* elements */
  GstElement *queue;
  /* element pads and ghost pad for bin */
  GstPad *src, *src_ghost;
  GstPad *dst, *dst_ghost;
  /* peer tee/mix request pad and ghost pad for bin */
  GstPad *peer_src, *peer_src_ghost;
  GstPad *peer_dst, *peer_dst_ghost;
};

enum
{
  M_SRC1 = 0,
  M_SRC2,
  M_FX1,
  M_SINK,
  M_
};

struct Graph_
{
  GstBin *bin;
  GstClock *clock;
  Machine *m[M_];
  Wire *w[M_][M_];
  GMainLoop *loop;
};

/* setup helper */

static Machine *
make_machine (Graph * g, const gchar * elem_name, const gchar * bin_name)
{
  Machine *m;

  m = g_new0 (Machine, 1);
  m->g = g;

#if 1
  if (!(m->bin =
          (GstBin *) gst_parse_bin_from_description (elem_name, FALSE, NULL))) {
    GST_ERROR ("Can't create bin");
    exit (-1);
  }
  gst_object_set_name ((GstObject *) m->bin, bin_name);
  gst_bin_add (g->bin, (GstElement *) m->bin);

  m->elem = m->bin->children->data;
#else
  if (!(m->bin = (GstBin *) gst_bin_new (bin_name))) {
    GST_ERROR ("Can't create bin");
    exit (-1);
  }
  gst_bin_add (g->bin, (GstElement *) m->bin);

  if (!(m->elem = gst_element_factory_make (elem_name, NULL))) {
    GST_ERROR ("Can't create element %s", elem_name);
    exit (-1);
  }
  gst_bin_add (m->bin, m->elem);
#endif

  return (m);
}

static void
add_tee (Machine * m)
{
  if (!(m->tee = gst_element_factory_make ("tee", NULL))) {
    GST_ERROR ("Can't create element tee");
    exit (-1);
  }
  gst_bin_add (m->bin, m->tee);
  gst_element_link (m->elem, m->tee);
}

static void
add_mix (Machine * m)
{
  if (!(m->mix = gst_element_factory_make ("adder", NULL))) {
    GST_ERROR ("Can't create element adder");
    exit (-1);
  }
  gst_bin_add (m->bin, m->mix);
  gst_element_link (m->mix, m->elem);
}

static gboolean
data_probe (GstPad * pad, GstBuffer * buf, Graph * g)
{
  GstClockTime ct, bt;
  GstClockTimeDiff d;

  if (!g->clock)
    return TRUE;

  ct = gst_clock_get_time (g->clock);
  bt = GST_BUFFER_TIMESTAMP (buf);
  if ( /*GST_CLOCK_TIME_IS_VALID (ct) && GST_CLOCK_TIME_IS_VALID (bt) && */ (ct
          < bt)) {
    d = GST_CLOCK_DIFF (ct, bt);
    // clock-time, latency
    GST_DEBUG_OBJECT (pad, "%" G_GUINT64_FORMAT " %" G_GUINT64_FORMAT,
        GST_TIME_AS_MSECONDS (ct), GST_TIME_AS_MSECONDS (d));
  }
  return TRUE;
}

static void
add_data_probe (Graph * g, GstElement * e, const gchar * pad_name)
{
  GstPad *p;

  p = gst_element_get_static_pad (e, pad_name);
  gst_pad_add_data_probe (p, (GCallback) data_probe, (gpointer) g);
}

static Machine *
make_src (Graph * g, const gchar * elem_name, const gchar * m_name)
{
  Machine *m;

  m = make_machine (g, elem_name, m_name);
  add_tee (m);
  add_data_probe (g, m->elem, "src");
  add_data_probe (g, m->tee, "sink");
  return (m);
}

static Machine *
make_fx (Graph * g, const gchar * elem_name, const gchar * m_name)
{
  Machine *m;

  m = make_machine (g, elem_name, m_name);
  add_tee (m);
  add_mix (m);
  add_data_probe (g, m->mix, "src");
  add_data_probe (g, m->elem, "sink");
  add_data_probe (g, m->elem, "src");
  add_data_probe (g, m->tee, "sink");
  return (m);
}

static Machine *
make_sink (Graph * g, const gchar * elem_name, const gchar * m_name)
{
  Machine *m;

  m = make_machine (g, elem_name, m_name);
  add_mix (m);
  add_data_probe (g, m->mix, "src");
  add_data_probe (g, m->elem, "sink");
  return (m);
}

static void
link_add (Graph * g, gint s, gint d)
{
  Wire *w;
  Machine *ms = g->m[s], *md = g->m[d];
  gchar w_name[50];
  GstPadLinkReturn plr;

  GST_WARNING ("link %s -> %s", GST_OBJECT_NAME (ms->bin),
      GST_OBJECT_NAME (md->bin));
  sprintf (w_name, "%s - %s", GST_OBJECT_NAME (ms->bin),
      GST_OBJECT_NAME (md->bin));

  g->w[s][d] = w = g_new0 (Wire, 1);
  w->g = g;
  w->ms = ms;
  w->md = md;

  if (!(w->bin = (GstBin *) gst_bin_new (w_name))) {
    GST_ERROR ("Can't create bin");
    exit (-1);
  }
#if USE_QUEUE
  if (!(w->queue = gst_element_factory_make ("queue", NULL))) {
    GST_ERROR ("Can't create element queue");
    exit (-1);
  }
  g_object_set (G_OBJECT (w->queue),
      "max-size-buffers", 1,
      "max-size-bytes", 0,
      "max-size-time", G_GUINT64_CONSTANT (0), "silent", TRUE, NULL);
#else
  // try and see if the queue is causing the latency
  if (!(w->queue = gst_element_factory_make ("identity", NULL))) {
    GST_ERROR ("Can't create element identity");
    exit (-1);
  }
#endif
  add_data_probe (g, w->queue, "src");
  add_data_probe (g, w->queue, "sink");
  gst_bin_add (w->bin, w->queue);

  /* request machine pads */
  w->peer_src = gst_element_get_request_pad (ms->tee, "src%d");
  g_assert (w->peer_src);
  w->peer_src_ghost = gst_ghost_pad_new (NULL, w->peer_src);
  g_assert (w->peer_src_ghost);
  gst_element_add_pad ((GstElement *) ms->bin, w->peer_src_ghost);

  w->peer_dst = gst_element_get_request_pad (md->mix, "sink%d");
  g_assert (w->peer_dst);
  w->peer_dst_ghost = gst_ghost_pad_new (NULL, w->peer_dst);
  g_assert (w->peer_dst_ghost);
  gst_element_add_pad ((GstElement *) md->bin, w->peer_dst_ghost);

  /* create wire pads */
  w->src = gst_element_get_static_pad (w->queue, "src");
  g_assert (w->src);
  w->src_ghost = gst_ghost_pad_new (NULL, w->src);
  g_assert (w->src_ghost);
  gst_element_add_pad ((GstElement *) w->bin, w->src_ghost);

  w->dst = gst_element_get_static_pad (w->queue, "sink");
  g_assert (w->dst);
  w->dst_ghost = gst_ghost_pad_new (NULL, w->dst);
  g_assert (w->dst_ghost);
  gst_element_add_pad ((GstElement *) w->bin, w->dst_ghost);

  /* add wire to pipeline */
  gst_bin_add (g->bin, (GstElement *) w->bin);

  /* link pads */
  plr = gst_pad_link (w->peer_src_ghost, w->dst_ghost);
  g_assert (plr == GST_PAD_LINK_OK);
  plr = gst_pad_link (w->src_ghost, w->peer_dst_ghost);
  g_assert (plr == GST_PAD_LINK_OK);
}

/* bus helper */

static void
message_received (GstBus * bus, GstMessage * message, Graph * g)
{
  const GstStructure *s;

  s = gst_message_get_structure (message);
  if (s) {
    gchar *sstr;

    sstr = gst_structure_to_string (s);
    GST_WARNING_OBJECT (GST_MESSAGE_SRC (message), "%s", sstr);
    g_free (sstr);
  } else {
    GST_WARNING_OBJECT (GST_MESSAGE_SRC (message), "no message details");
  }
}

static void
eos_message_received (GstBus * bus, GstMessage * message, Graph * g)
{
  if (GST_MESSAGE_SRC (message) == (GstObject *) g->bin) {
    GST_INFO ("got eos");
    g_main_loop_quit (g->loop);
  }
}

static void
state_changed_message_received (GstBus * bus, GstMessage * message, Graph * g)
{
  if (GST_MESSAGE_SRC (message) == (GstObject *) g->bin) {
    GstState oldstate, newstate, pending;

    gst_message_parse_state_changed (message, &oldstate, &newstate, &pending);
    switch (GST_STATE_TRANSITION (oldstate, newstate)) {
      case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
        GST_DEBUG_BIN_TO_DOT_FILE (g->bin, GST_DEBUG_GRAPH_SHOW_ALL, "latency");
        GST_INFO ("reached playing");
        break;
      default:
        break;
    }
  }
}

static void
new_clock_message_received (GstBus * bus, GstMessage * message, Graph * g)
{
  if (g->clock)
    gst_object_unref (g->clock);
  gst_message_parse_new_clock (message, &g->clock);
  gst_object_ref (g->clock);
  GST_INFO ("new clock: %p", g->clock);
}

/* test application */

#define SINK_NAME "pulsesink"
#define FX_NAME "volume"
#define SRC_NAME "audiotestsrc"
#define NUM_BUFFERS 50

int
main (int argc, char **argv)
{
  Graph *g;
  GstBus *bus;
  gint i, j;
  guint64 chunk;
  gulong blocksize;
  /* command line options */
  guint tpb = 4;
  guint bpm = 125;
  guint div = 1;
  gchar *sink_name = SINK_NAME;

  /* init gstreamer */
  gst_init (&argc, &argv);
  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "latency", 0, "latency test");

  if (argc > 1) {
    tpb = atoi (argv[1]);
    tpb = MAX (1, tpb);
    if (argc > 2) {
      bpm = atoi (argv[2]);
      bpm = MAX (1, bpm);
      if (argc > 3) {
        div = atoi (argv[3]);
        div = MAX (1, div);
        if (argc > 4) {
          sink_name = argv[4];
        }
      }
    }
  }
  chunk = (GST_SECOND * G_GINT64_CONSTANT (60)) / (guint64) (bpm * tpb);
  blocksize = ((chunk * 44100) / GST_SECOND);
  GST_INFO ("%s| bpm=%u, tpb=%u, div=%u, target-latency=%" G_GUINT64_FORMAT
      " µs=%" G_GUINT64_FORMAT " ms", sink_name, bpm, tpb, div,
      GST_TIME_AS_USECONDS (chunk), GST_TIME_AS_MSECONDS (chunk));


  g = g_new0 (Graph, 1);

  /* create a new top-level pipeline to hold the elements */
  g->bin = (GstBin *) gst_pipeline_new ("song");
  /* and connect to the bus */
  bus = gst_pipeline_get_bus (GST_PIPELINE (g->bin));
  gst_bus_add_signal_watch_full (bus, G_PRIORITY_HIGH);
  g_signal_connect (bus, "message::error", G_CALLBACK (message_received), g);
  g_signal_connect (bus, "message::warning", G_CALLBACK (message_received), g);
  g_signal_connect (bus, "message::eos", G_CALLBACK (eos_message_received), g);
  g_signal_connect (bus, "message::state-changed",
      G_CALLBACK (state_changed_message_received), g);
  g_signal_connect (bus, "message::new-clock",
      G_CALLBACK (new_clock_message_received), g);

  /* make machines */
  g->m[M_SRC1] = make_src (g, SRC_NAME, "src1");
  g->m[M_SINK] = make_sink (g, sink_name, "sink");

  g_object_set (g->m[M_SRC1]->elem,
      "num-buffers", (NUM_BUFFERS * div),
      "samplesperbuffer", (blocksize / div), NULL);

  /* simple setup */
#if 1
  link_add (g, M_SRC1, M_SINK);
#endif

  /* setup with fx */
#if 0
  g->m[M_FX1] = make_fx (g, FX_NAME, "fx1");

  link_add (g, M_SRC1, M_FX1);
  link_add (g, M_FX1, M_SINK);
#endif

  /* setup with fx and another source */
#if 0
  g->m[M_SRC2] = make_src (g, SRC_NAME, "src2");
  g->m[M_FX1] = make_fx (g, "fx1");
  g_object_set (g->m[M_SRC2]->elem,
      "num-buffers", NUM_BUFFERS, "blocksize", blocksize, NULL);

  link_add (g, M_SRC1, M_FX1);
  link_add (g, M_SRC2, M_FX1);
  link_add (g, M_FX1, M_SINK);
#endif

  /* configure the sink */
  g_object_set (g->m[M_SINK]->elem,
      "latency-time", GST_TIME_AS_USECONDS (chunk),
      "buffer-time", GST_TIME_AS_USECONDS (chunk << 1),
      // no observable effect
      "provide-clock", TRUE, "blocksize", blocksize, "sync", TRUE, NULL);

  /* create a main-loop */
  g->loop = g_main_loop_new (NULL, FALSE);

  gst_element_set_state ((GstElement *) g->bin, GST_STATE_PLAYING);

  /* run a main-loop */
  g_main_loop_run (g->loop);

  GST_WARNING ("cleanup");

  /* cleanup */
  gst_element_set_state ((GstElement *) g->bin, GST_STATE_NULL);
  gst_bus_remove_signal_watch (bus);
  gst_object_unref (GST_OBJECT (bus));
  gst_object_unref (GST_OBJECT (g->bin));
  for (i = 0; i < M_; i++) {
    g_free (g->m[i]);
    g->m[i] = NULL;
    for (j = 0; j < M_; j++) {
      g_free (g->w[i][j]);
      g->w[i][j] = NULL;
    }
  }
  if (g->clock)
    gst_object_unref (g->clock);
  g_free (g);

  GST_WARNING ("done");
  exit (0);
}
