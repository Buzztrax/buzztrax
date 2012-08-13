/* test dynamic linking
 *
 * gcc -Wall -g `pkg-config gstreamer-0.10 --cflags --libs` dynlink4.c -o dynlink4
 * gcc -Wall -g -DGST_USE_UNSTABLE_API `pkg-config gstreamer-0.11 --cflags --libs` dynlink4.c -o dynlink4
 * GST_DEBUG="*:2" ./dynlink4
 * GST_DEBUG_DUMP_DOT_DIR=$PWD ./dynlink4
 * for file in dyn*.dot; do echo $file; dot -Tpng $file -o${file/dot/png}; done
 *
 * GST_DEBUG="*:5" GST_DEBUG_NO_COLOR=1 ./dynlink4 2>&1 | tee debug.log | grep "dynlink4.c:"
 * sort debug.log >debug2.log
 */

/* high level default scenario:
 *   src2                   fx
 *   
 *   src1 <---------------> sink
 *
 * possible links:
 *   src1 ! fx  : 1
 *   src2 ! fx  : 2
 *   src2 ! sink: 4
 *   fx ! sink  : 8
 *             => 16 different combinations
 * but skip: 1, 2, 3, 5, 7, 8
 *
 * prepare this as a list of items and run them async, in order to wait for
 * previous steps to finish
 * - wait for PLAYING and then kick the first item from the list
 * - this triggers the next when done with a g_timeout_add
 * - this way we can run a mainloop too
 * - on the last step we end the mainloop
 *
 * howto:
 * - no dataflow on disconnected pads
 * - on push, check source pads; on pull, check sink pads
 * - on existing elements, block pad, wait async, link, unblock
 * - on new elements, block pad, change state, wait async, link, unblock  
 *
 * questions:
 * - in the case of bins, which pads to block - ghostpads or their targets?
 *   - does not seem to make a big difference
 * - when to see on sources to tell them about the segment 
 */

#include <stdio.h>
#include <stdlib.h>
#include <gst/gst.h>

#define SINK_NAME "alsasink"
#define FX_NAME "volume"
#define SRC_NAME "audiotestsrc"

#define GST_CAT_DEFAULT bt_dynlink
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

typedef struct Machine_ Machine;
typedef struct Wire_ Wire;
typedef struct Graph_ Graph;

#define M_IS_SRC(m) (!m->mix)
#define M_IS_SINK(m) (!m->tee)

struct Machine_
{
  Graph *g;
  GstBin *bin;
  /* elements */
  GstElement *mix;
  GstElement *elem;
  GstElement *tee;
  gint pads;
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
  /* activate/deactivate when linking/unlinking */
  gboolean as, ad;
};

enum
{
  M_SRC1 = 0,
  M_SRC2,
  M_FX,
  M_SINK,
  M_
};

struct Graph_
{
  GstBin *bin;
  Machine *m[M_];
  Wire *w[M_][M_];
  GMainLoop *loop;
  gint pending_changes;
};

/* test step */
static gint step = 0;
/* skip some partially connected setups */
static int steps[] = {
  0,                            /* 0000000: */
  4,                            /* 0000100: src2 -> sink */
  9,                            /* 0001001: src1 -> fx, fx -> sink */
  10,                           /* 0001010: src2 -> fx, fx -> sink */
  11,                           /* 0001011: src1 -> fx, src2 -> fx, fx -> sink */
  13,                           /* 0001101: src1 -> fx, fx -> sink, src2 -> sink */
  14,                           /* 0001110: src2 -> fx, fx -> sink, src2 -> sink */
  15,                           /* 0001111: src1 -> fx, src2 -> fx, fx -> sink, src2 -> sink */
  14, 13, 11, 10, 9, 4, 0
};

static gboolean do_test_step (Graph * g);

/* misc helpers */

static void
dump_pipeline (Graph * g, gchar * suffix)
{
  gchar t[100];
  static GstDebugGraphDetails graph_details =
      GST_DEBUG_GRAPH_SHOW_ALL & ~GST_DEBUG_GRAPH_SHOW_CAPS_DETAILS;
  //GST_DEBUG_GRAPH_SHOW_ALL;

  snprintf (t, 100, "dyn%02d%s%s", step, (suffix ? "_" : ""),
      (suffix ? suffix : ""));
  t[99] = '\0';
  GST_DEBUG_BIN_TO_DOT_FILE (g->bin, graph_details, t);
}

/* setup helper */

static Machine *
make_machine (Graph * g, const gchar * elem_name, const gchar * bin_name)
{
  Machine *m;

  m = g_new0 (Machine, 1);
  m->g = g;

  if (!(m->bin = (GstBin *) gst_bin_new (bin_name))) {
    GST_ERROR ("Can't create bin");
    exit (-1);
  }
  gst_bin_add (g->bin, (GstElement *) m->bin);
  gst_element_set_locked_state ((GstElement *) m->bin, TRUE);

  if (!(m->elem = gst_element_factory_make (elem_name, NULL))) {
    GST_ERROR ("Can't create element %s", elem_name);
    exit (-1);
  }
  gst_bin_add (m->bin, m->elem);

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

static Machine *
make_src (Graph * g, const gchar * m_name)
{
  Machine *m;

  m = make_machine (g, SRC_NAME, m_name);
  add_tee (m);
  return (m);
}

static Machine *
make_fx (Graph * g, const gchar * m_name)
{
  Machine *m;

  m = make_machine (g, FX_NAME, m_name);
  add_tee (m);
  add_mix (m);
  return (m);
}

static Machine *
make_sink (Graph * g, const gchar * m_name)
{
  Machine *m;

  m = make_machine (g, SINK_NAME, m_name);
  add_mix (m);
  return (m);
}

#if GST_CHECK_VERSION(0,11,0)
static GstProbeReturn
post_link_add (GstPad * pad, GstProbeType type, gpointer type_data,
    gpointer user_data)
#else
static void
post_link_add (GstPad * pad, gboolean blocked, gpointer user_data)
#endif
{
  Wire *w = (Wire *) user_data;
  Graph *g = w->g;
  Machine *ms = w->ms, *md = w->md;
  GstPadLinkReturn plr;
  GstStateChangeReturn scr;

  if (pad)
    GST_WARNING ("link %s -> %s blocked", GST_OBJECT_NAME (ms->bin),
        GST_OBJECT_NAME (md->bin));

  /* do this in the blocked-callback for links to playing parts */
  if (GST_STATE (md) == GST_STATE_PLAYING) {
    w->peer_dst = gst_element_get_request_pad (md->mix, "sink%d");
    g_assert (w->peer_dst);
    w->peer_dst_ghost = gst_ghost_pad_new (NULL, w->peer_dst);
    g_assert (w->peer_dst_ghost);
    if (!w->ad) {
      gst_pad_set_active (w->peer_dst_ghost, TRUE);
    }
    gst_element_add_pad ((GstElement *) md->bin, w->peer_dst_ghost);
    md->pads++;
  }

  /* link ghostpads (downstream) */
  plr = gst_pad_link (w->src_ghost, w->peer_dst_ghost);
  g_assert (plr == GST_PAD_LINK_OK);

  if (pad) {
#if GST_CHECK_VERSION(0,11,0)
    if (w->as && M_IS_SRC (ms)) {
      scr = gst_element_set_state ((GstElement *) ms->bin, GST_STATE_PLAYING);
      g_assert (scr != GST_STATE_CHANGE_FAILURE);
    }
#else
    if (w->as && M_IS_SRC (ms)) {
      GstFormat fmt = GST_FORMAT_TIME;
      gint64 pos;

      gst_element_query_position ((GstElement *) g->bin, &fmt, &pos);

      /* seek on src to tell it about new segment
       * the flushing seek will? unlock the pad */
#if 0
      GST_WARNING ("seek on %s", GST_OBJECT_NAME (ms->bin));
      gst_element_seek_simple ((GstElement *) w->bin, GST_FORMAT_TIME,
          GST_SEEK_FLAG_FLUSH, pos);
#else
      GST_WARNING ("seek on %s", GST_OBJECT_NAME (w->src_ghost));
      gst_pad_send_event (w->src_ghost, gst_event_new_seek (1.0,
              GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, pos,
              GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE));
#endif

      scr = gst_element_set_state ((GstElement *) ms->bin, GST_STATE_PLAYING);
      g_assert (scr != GST_STATE_CHANGE_FAILURE);
    } else {
      /* unblock w->peer_src_ghost */
      gst_pad_set_blocked (pad, FALSE);
    }
#endif
  }

  g->pending_changes--;
  GST_WARNING ("link %s -> %s done (%d pending changes)",
      GST_OBJECT_NAME (ms->bin), GST_OBJECT_NAME (md->bin), g->pending_changes);

  if (g->pending_changes == 0)
    if (GST_STATE (g->bin) == GST_STATE_PLAYING)        /* because of initial link */
      g_timeout_add_seconds (1, (GSourceFunc) do_test_step, g);

#if GST_CHECK_VERSION(0,11,0)
  return GST_PROBE_REMOVE;
#endif
}

static void
link_add (Graph * g, gint s, gint d)
{
  Wire *w;
  Machine *ms = g->m[s], *md = g->m[d];
  gchar w_name[50];
  GstPadLinkReturn plr;
  GstStateChangeReturn scr;
  gboolean blocked = FALSE;

  GST_WARNING ("link %s -> %s", GST_OBJECT_NAME (ms->bin),
      GST_OBJECT_NAME (md->bin));
  sprintf (w_name, "%s - %s", GST_OBJECT_NAME (ms->bin),
      GST_OBJECT_NAME (md->bin));

  g->w[s][d] = w = g_new0 (Wire, 1);
  w->g = g;
  w->ms = ms;
  w->md = md;
  w->as = (ms->pads == 0);
  w->ad = (md->pads == 0);

  if (!(w->bin = (GstBin *) gst_bin_new (w_name))) {
    GST_ERROR ("Can't create bin");
    exit (-1);
  }
  if (!(w->queue = gst_element_factory_make ("queue", NULL))) {
    GST_ERROR ("Can't create element queue");
    exit (-1);
  }
  gst_bin_add (w->bin, w->queue);

  /* request machine pads */
  w->peer_src = gst_element_get_request_pad (ms->tee, "src%d");
  g_assert (w->peer_src);
  w->peer_src_ghost = gst_ghost_pad_new (NULL, w->peer_src);
  g_assert (w->peer_src_ghost);
  if (!w->as) {
    gst_pad_set_active (w->peer_src_ghost, TRUE);
  }
  gst_element_add_pad ((GstElement *) ms->bin, w->peer_src_ghost);
  ms->pads++;

  /* do this in the blocked-callback for links to playing parts */
  if (GST_STATE (md) != GST_STATE_PLAYING) {
    w->peer_dst = gst_element_get_request_pad (md->mix, "sink%d");
    g_assert (w->peer_dst);
    w->peer_dst_ghost = gst_ghost_pad_new (NULL, w->peer_dst);
    g_assert (w->peer_dst_ghost);
    if (!w->ad) {
      gst_pad_set_active (w->peer_dst_ghost, TRUE);
    }
    gst_element_add_pad ((GstElement *) md->bin, w->peer_dst_ghost);
    md->pads++;
  }

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

  plr = gst_pad_link (w->peer_src_ghost, w->dst_ghost);
  g_assert (plr == GST_PAD_LINK_OK);

  /* block w->peer_src_ghost (before linking) */
  if ((GST_STATE (g->bin) == GST_STATE_PLAYING) && w->as && M_IS_SRC (ms)) {
    GST_WARNING ("activate %s", GST_OBJECT_NAME (ms->bin));

    GST_WARNING ("link %s -> %s blocking", GST_OBJECT_NAME (ms->bin),
        GST_OBJECT_NAME (md->bin));
#if GST_CHECK_VERSION(0,11,0)
    blocked =
        (gst_pad_add_probe (w->peer_dst, GST_PROBE_TYPE_BLOCK, post_link_add, w,
            NULL) != 0);
#else
    blocked = gst_pad_set_blocked_async (w->peer_dst, TRUE, post_link_add, w);
#endif

    // FIXME(ensonic): need to kick start the playback here

    dump_pipeline (g, "wire_add_blocking");
  }

  /* change state (upstream) and unlock */
  if (w->ad) {
    GST_WARNING ("activate %s", GST_OBJECT_NAME (md->bin));
    gst_element_set_locked_state ((GstElement *) md->bin, FALSE);
    scr = gst_element_set_state ((GstElement *) md->bin, GST_STATE_PLAYING);
    g_assert (scr != GST_STATE_CHANGE_FAILURE);
  }
  scr = gst_element_set_state ((GstElement *) w->bin, GST_STATE_PLAYING);
  g_assert (scr != GST_STATE_CHANGE_FAILURE);
  if (w->as) {
    GST_WARNING ("activate %s", GST_OBJECT_NAME (ms->bin));
    gst_element_set_locked_state ((GstElement *) ms->bin, FALSE);
    scr = gst_element_set_state ((GstElement *) ms->bin, GST_STATE_PLAYING);
    g_assert (scr != GST_STATE_CHANGE_FAILURE);
  }

  if (!blocked) {
    GST_WARNING ("link %s -> %s continuing", GST_OBJECT_NAME (ms->bin),
        GST_OBJECT_NAME (md->bin));
#if GST_CHECK_VERSION(0,11,0)
    post_link_add (NULL, GST_PROBE_TYPE_BLOCK, NULL, w);
#else
    post_link_add (NULL, TRUE, w);
#endif
  }
}

#if GST_CHECK_VERSION(0,11,0)
static GstProbeReturn
post_link_rem (GstPad * pad, GstProbeType type, gpointer type_data,
    gpointer user_data)
#else
static void
post_link_rem (GstPad * pad, gboolean blocked, gpointer user_data)
#endif
{
  Wire *w = (Wire *) user_data;
  Graph *g = w->g;
  Machine *ms = w->ms, *md = w->md;
  gboolean plr;
  GstStateChangeReturn scr;

  if (pad)
    GST_WARNING ("link %s -> %s blocked", GST_OBJECT_NAME (ms->bin),
        GST_OBJECT_NAME (md->bin));

  /* change state (upstream) and lock */
  if (w->ad) {
    GST_WARNING ("deactivate %s", GST_OBJECT_NAME (md->bin));
    scr = gst_element_set_state ((GstElement *) md->bin, GST_STATE_NULL);
    g_assert (scr != GST_STATE_CHANGE_FAILURE);
    gst_element_set_locked_state ((GstElement *) md->bin, TRUE);
  }
  scr = gst_element_set_state ((GstElement *) w->bin, GST_STATE_NULL);
  g_assert (scr != GST_STATE_CHANGE_FAILURE);
  if (w->as) {
    GST_WARNING ("deactivate %s", GST_OBJECT_NAME (md->bin));
    scr = gst_element_set_state ((GstElement *) ms->bin, GST_STATE_NULL);
    g_assert (scr != GST_STATE_CHANGE_FAILURE);
    gst_element_set_locked_state ((GstElement *) ms->bin, TRUE);
  }
  /* unlink ghostpads (upstream) */
  plr = gst_pad_unlink (w->src_ghost, w->peer_dst_ghost);
  g_assert (plr == TRUE);
  plr = gst_pad_unlink (w->peer_src_ghost, w->dst_ghost);
  g_assert (plr == TRUE);

  /* remove from pipeline */
  gst_object_ref (w->bin);
  gst_bin_remove ((GstBin *) GST_OBJECT_PARENT (w->bin), (GstElement *) w->bin);

  /* release request pads */
  gst_element_remove_pad ((GstElement *) ms->bin, w->peer_src_ghost);
  gst_element_release_request_pad (ms->tee, w->peer_src);
  ms->pads--;
  gst_element_remove_pad ((GstElement *) md->bin, w->peer_dst_ghost);
  gst_element_release_request_pad (md->mix, w->peer_dst);
  md->pads--;

  gst_object_unref (w->src);
  gst_object_unref (w->dst);
  gst_object_unref (w->bin);
  g_free (w);

  /* unblock w->peer_src_ghost, the pad gets removed above
   * if (pad)
   *   gst_pad_set_blocked (pad, FALSE);
   */

  g->pending_changes--;
  GST_WARNING ("link %s -> %s done (%d pending changes)",
      GST_OBJECT_NAME (ms->bin), GST_OBJECT_NAME (md->bin), g->pending_changes);

  if (g->pending_changes == 0)
    g_timeout_add_seconds (1, (GSourceFunc) do_test_step, g);

#if GST_CHECK_VERSION(0,11,0)
  return GST_PROBE_REMOVE;
#endif
}

static void
link_rem (Graph * g, gint s, gint d)
{
  Wire *w = g->w[s][d];
  Machine *ms = g->m[s], *md = g->m[d];
  gboolean blocked = FALSE;

  GST_WARNING ("link %s -> %s", GST_OBJECT_NAME (ms->bin),
      GST_OBJECT_NAME (md->bin));

  w->as = (ms->pads == 1);
  w->ad = (md->pads == 1);
  g->w[s][d] = NULL;

  /* block w->peer_src_ghost */
  if (GST_STATE (g->bin) == GST_STATE_PLAYING) {
    GST_WARNING ("link %s -> %s blocking", GST_OBJECT_NAME (ms->bin),
        GST_OBJECT_NAME (md->bin));
#if GST_CHECK_VERSION(0,11,0)
    blocked =
        (gst_pad_add_probe (w->peer_dst, GST_PROBE_TYPE_BLOCK, post_link_rem, w,
            NULL) != 0);
#else
    blocked = gst_pad_set_blocked_async (w->peer_dst, TRUE, post_link_rem, w);
#endif
    dump_pipeline (g, "wire_rem_blocking");
  }
  if (!blocked) {
    GST_WARNING ("link %s -> %s continuing", GST_OBJECT_NAME (ms->bin),
        GST_OBJECT_NAME (md->bin));
#if GST_CHECK_VERSION(0,11,0)
    post_link_rem (NULL, GST_PROBE_TYPE_BLOCK, NULL, w);
#else
    post_link_rem (NULL, TRUE, w);
#endif
  }
}

static void
change_links (Graph * g, gint o, gint n)
{
  gint c = o ^ n;
  gint i, j;

  g->pending_changes = 0;
  if (c & 1)
    g->pending_changes++;
  if (c & 2)
    g->pending_changes++;
  if (c & 4)
    g->pending_changes++;
  if (c & 8)
    g->pending_changes++;

  GST_WARNING ("== change %02d -> %02d (%d link changes) ==", o, n,
      g->pending_changes);
  for (i = 0; i < M_; i++) {
    for (j = 0; j < M_; j++) {
      if (g->w[i][j]) {
        GST_WARNING ("have link %s -> %s",
            GST_OBJECT_NAME (g->m[i]->bin), GST_OBJECT_NAME (g->m[j]->bin));
      }
    }
  }

  dump_pipeline (g, NULL);

  /* add links */
  if ((o & 1) == 0 && (n & 1) != 0)
    link_add (g, M_SRC1, M_FX);
  if ((o & 2) == 0 && (n & 2) != 0)
    link_add (g, M_SRC2, M_FX);
  if ((o & 4) == 0 && (n & 4) != 0)
    link_add (g, M_SRC2, M_SINK);
  if ((o & 8) == 0 && (n & 8) != 0)
    link_add (g, M_FX, M_SINK);

  /* remove links */
  if ((o & 1) != 0 && (n & 1) == 0)
    link_rem (g, M_SRC1, M_FX);
  if ((o & 2) != 0 && (n & 2) == 0)
    link_rem (g, M_SRC2, M_FX);
  if ((o & 4) != 0 && (n & 4) == 0)
    link_rem (g, M_SRC2, M_SINK);
  if ((o & 8) != 0 && (n & 8) == 0)
    link_rem (g, M_FX, M_SINK);
}

static gboolean
do_test_step (Graph * g)
{
  if (g->pending_changes == 0) {
    if (step == G_N_ELEMENTS (steps)) {
      g_main_loop_quit (g->loop);
    } else {
      change_links (g, steps[step - 1], steps[step]);
      step++;
    }
  } else {
    GST_WARNING ("wait (%d link changes)", g->pending_changes);
    dump_pipeline (g, NULL);
  }
  return (FALSE);
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
    dump_pipeline (g, "error");
  } else {
    GST_WARNING_OBJECT (GST_MESSAGE_SRC (message), "no message details");
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
        GST_WARNING ("reached playing");
        dump_pipeline (g, NULL);
        step = 1;
        g_timeout_add_seconds (1, (GSourceFunc) do_test_step, g);
        break;
      default:
        break;
    }
  }
}

/* test application */

int
main (int argc, char **argv)
{
  Graph *g;
  GstBus *bus;
  gint i, j;

  /* init gstreamer */
  gst_init (&argc, &argv);

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "dynlink", 0,
      "dynamic linking test");

  GST_WARNING ("setup");

  g = g_new0 (Graph, 1);

  /* create a new top-level pipeline to hold the elements */
  g->bin = (GstBin *) gst_pipeline_new ("song");
  /* and connect to the bus */
  bus = gst_pipeline_get_bus (GST_PIPELINE (g->bin));
  gst_bus_add_signal_watch_full (bus, G_PRIORITY_HIGH);
  g_signal_connect (bus, "message::error", G_CALLBACK (message_received), g);
  g_signal_connect (bus, "message::warning", G_CALLBACK (message_received), g);
  g_signal_connect (bus, "message::state-changed",
      G_CALLBACK (state_changed_message_received), g);

  /* make machines */
  g->m[M_SRC1] = make_src (g, "src1");
  g->m[M_SRC2] = make_src (g, "src2");
  g->m[M_FX] = make_fx (g, "fx");
  g->m[M_SINK] = make_sink (g, "sink");

  /* configure the sources, select different sounds,
   * also use a bigger block size to have less dataflow logging noise */
  g_object_set (g->m[M_SRC1]->elem,
      "freq", (gdouble) 440.0, "wave", 2, "blocksize", 22050, NULL);
  g_object_set (g->m[M_SRC2]->elem,
      "freq", (gdouble) 110.0, "wave", 1, "blocksize", 22050, NULL);

  /* create a main-loop */
  g->loop = g_main_loop_new (NULL, FALSE);

  /* do the initial link and play */
  GST_WARNING ("lets go");
  g->pending_changes = 1;
  link_add (g, M_SRC1, M_SINK);
  gst_element_set_state ((GstElement *) g->bin, GST_STATE_PAUSED);

  gst_element_seek_simple ((GstElement *) g->bin, GST_FORMAT_TIME,
      GST_SEEK_FLAG_FLUSH, G_GINT64_CONSTANT (0));

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
  g_free (g);

  GST_WARNING ("done");
  exit (0);
}
