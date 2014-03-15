/* test dynamic linking
 *
 * gcc -Wall -g dynlink4.c -o dynlink4 `pkg-config gstreamer-1.0 --cflags --libs`
 * GST_DEBUG="*:2" ./dynlink4
 * GST_DEBUG_DUMP_DOT_DIR=$PWD ./dynlink4
 * for file in dyn*.dot; do echo $file; dot -Tpng $file -o${file/dot/png}; done
 *
 * GST_DEBUG="*:5" GST_DEBUG_NO_COLOR=1 ./dynlink4 2>&1 | tee debug.log | grep "dynlink4.c:"
 * sort debug.log >debug2.log
 */

#include "dynlink.h"

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

/* dynamic linking */

static GstPadProbeReturn
post_link_add (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
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
      if (!gst_pad_set_active (w->peer_dst_ghost, TRUE)) {
        GST_WARNING_OBJECT (w->peer_dst_ghost, "could not activate");
      }
    }
    gst_element_add_pad ((GstElement *) md->bin, w->peer_dst_ghost);
    md->pads++;
  }

  /* link ghostpads (downstream) */
  plr = gst_pad_link (w->src_ghost, w->peer_dst_ghost);
  g_assert_cmpint (plr, ==, GST_PAD_LINK_OK);

  if (pad) {
    if (w->as && M_IS_SRC (ms)) {
      gint64 pos;

      if (gst_element_query_position ((GstElement *) g->bin, GST_FORMAT_TIME,
              &pos)) {
        /* seek on src to tell it about new segment, the flushing seek will
         * unflush the pad */
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
      } else {
        GST_WARNING ("position query failed");
      }

      scr = gst_element_set_state ((GstElement *) ms->bin, GST_STATE_PLAYING);
      g_assert (scr != GST_STATE_CHANGE_FAILURE);
    }
  }

  g->pending_changes--;
  GST_WARNING ("link %s -> %s done (%d pending changes)",
      GST_OBJECT_NAME (ms->bin), GST_OBJECT_NAME (md->bin), g->pending_changes);

  if (g->pending_changes == 0)
    if (GST_STATE (g->bin) == GST_STATE_PLAYING)        /* because of initial link */
      g_timeout_add_seconds (1, (GSourceFunc) do_test_step, g);

  return GST_PAD_PROBE_REMOVE;
}

static void
link_add (Graph * g, gint s, gint d)
{
  Wire *w;
  Machine *ms = g->m[s], *md = g->m[d];
  GstPadLinkReturn plr;
  GstStateChangeReturn scr;

  GST_WARNING ("link %s -> %s", GST_OBJECT_NAME (ms->bin),
      GST_OBJECT_NAME (md->bin));

  g->w[s][d] = w = make_wire (g, ms, md);

  /* request machine pads */
  add_request_pad (ms, ms->tee, &w->peer_src, &w->peer_src_ghost, "src_%u",
      w->as);

  /* do this in the blocked-callback for links to playing parts */
  if (GST_STATE (md) != GST_STATE_PLAYING) {
    add_request_pad (md, md->mix, &w->peer_dst, &w->peer_dst_ghost, "sink_%u",
        w->ad);
  }

  /* create wire pads */
  w->src = gst_element_get_static_pad (w->queue, "src");
  g_assert (w->src);
  w->src_ghost = gst_ghost_pad_new (NULL, w->src);
  g_assert (w->src_ghost);
  if (!gst_element_add_pad ((GstElement *) w->bin, w->src_ghost)) {
    GST_ERROR_OBJECT (md->bin, "Failed to add src ghost pad to wire bin");
    exit (-1);
  }
  w->dst = gst_element_get_static_pad (w->queue, "sink");
  g_assert (w->dst);
  w->dst_ghost = gst_ghost_pad_new (NULL, w->dst);
  g_assert (w->dst_ghost);
  if (!gst_element_add_pad ((GstElement *) w->bin, w->dst_ghost)) {
    GST_ERROR_OBJECT (md->bin, "Failed to add sink ghost pad to wire bin");
    exit (-1);
  }

  /* add wire to pipeline */
  if (!gst_bin_add (g->bin, (GstElement *) w->bin)) {
    GST_ERROR_OBJECT (w->bin, "Can't add wire bin to pipeline");
    exit (-1);
  }

  plr = gst_pad_link (w->peer_src_ghost, w->dst_ghost);
  g_assert_cmpint (plr, ==, GST_PAD_LINK_OK);

  /* block w->peer_dst (before linking) */
  if ((GST_STATE (g->bin) == GST_STATE_PLAYING) && w->as && M_IS_SRC (ms)) {
    GST_WARNING ("activate %s", GST_OBJECT_NAME (ms->bin));

    GST_WARNING ("link %s -> %s blocking", GST_OBJECT_NAME (ms->bin),
        GST_OBJECT_NAME (md->bin));
    gst_pad_add_probe (w->peer_dst, GST_PAD_PROBE_TYPE_BLOCK, post_link_add, w,
        NULL);

    // FIXME(ensonic): need to kick start the playback here

    dump_pipeline (g, step, "wire_add_blocking");
  } else {
    GST_WARNING ("link %s -> %s continuing", GST_OBJECT_NAME (ms->bin),
        GST_OBJECT_NAME (md->bin));
    post_link_add (NULL, NULL, w);
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
}

static GstPadProbeReturn
post_link_rem (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
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
    GST_WARNING ("deactivate %s", GST_OBJECT_NAME (ms->bin));
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
  if (!gst_bin_remove (w->g->bin, (GstElement *) w->bin)) {
    GST_WARNING_OBJECT (w->bin, "cannot remove wire bin from pipeline");
  }

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

  g->pending_changes--;
  GST_WARNING ("link %s -> %s done (%d pending changes)",
      GST_OBJECT_NAME (ms->bin), GST_OBJECT_NAME (md->bin), g->pending_changes);

  if (g->pending_changes == 0)
    g_timeout_add_seconds (1, (GSourceFunc) do_test_step, g);

  return GST_PAD_PROBE_REMOVE;
}

static void
link_rem (Graph * g, gint s, gint d)
{
  Wire *w = g->w[s][d];
  Machine *ms = g->m[s], *md = g->m[d];

  GST_WARNING ("link %s -> %s", GST_OBJECT_NAME (ms->bin),
      GST_OBJECT_NAME (md->bin));

  w->as = (ms->pads == 1);
  w->ad = (md->pads == 1);
  g->w[s][d] = NULL;

  /* block w->peer_src_ghost */
  if (GST_STATE (g->bin) == GST_STATE_PLAYING) {
    GST_WARNING ("link %s -> %s blocking", GST_OBJECT_NAME (ms->bin),
        GST_OBJECT_NAME (md->bin));
    gst_pad_add_probe (w->peer_dst, GST_PAD_PROBE_TYPE_BLOCK, post_link_rem, w,
        NULL);
    dump_pipeline (g, step, "wire_rem_blocking");
  } else {
    GST_WARNING ("link %s -> %s continuing", GST_OBJECT_NAME (ms->bin),
        GST_OBJECT_NAME (md->bin));
    post_link_rem (NULL, NULL, w);
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

  GST_WARNING ("==== change %02d -> %02d (%d link changes) ====", o, n,
      g->pending_changes);
  for (i = 0; i < M_; i++) {
    for (j = 0; j < M_; j++) {
      if (g->w[i][j]) {
        GST_WARNING ("have link %s -> %s",
            GST_OBJECT_NAME (g->m[i]->bin), GST_OBJECT_NAME (g->m[j]->bin));
      }
    }
  }

  dump_pipeline (g, step, NULL);

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
    dump_pipeline (g, step, NULL);
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
    gchar *sstr = gst_structure_to_string (s);
    GST_WARNING_OBJECT (GST_MESSAGE_SRC (message), "%s", sstr);
    g_free (sstr);
    dump_pipeline (g, step, "error");
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
        dump_pipeline (g, step, NULL);
        step = 1;
        g_timeout_add_seconds (1, (GSourceFunc) do_test_step, g);
        break;
      default:
        break;
    }
  }
}

/* test application */

gint
main (gint argc, gchar ** argv)
{
  Graph *g;
  GstBus *bus;

  /* init gstreamer */
  gst_init (&argc, &argv);

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "dynlink", 0,
      "dynamic linking test");

  GST_WARNING ("setup");

  g = make_graph ();

  /* connect to the bus */
  bus = gst_pipeline_get_bus (GST_PIPELINE (g->bin));
  gst_bus_add_signal_watch_full (bus, G_PRIORITY_HIGH);
  g_signal_connect (bus, "message::error", G_CALLBACK (message_received), g);
  g_signal_connect (bus, "message::warning", G_CALLBACK (message_received), g);
  g_signal_connect (bus, "message::state-changed",
      G_CALLBACK (state_changed_message_received), g);

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
  free_graph (g);

  GST_WARNING ("done");
  exit (0);
}
