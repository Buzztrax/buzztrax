/* test dynamic linking
 *
 * gcc -Wall -g dynlink3.c -o dynlink3 `pkg-config gstreamer-1.0 --cflags --libs`
 * GST_DEBUG="*:2" ./dynlink3
 * GST_DEBUG_DUMP_DOT_DIR=$PWD ./dynlink3
 * for file in dyn*.dot; do echo $file; dot -Tpng $file -o${file/dot/png}; done
 *
 * GST_DEBUG="*:5" GST_DEBUG_NO_COLOR=1 ./dynlink3 2>&1 | tee debug.log | grep "dynlink3.c:"
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

static gint in_idle_probe = FALSE;

static GstPadProbeReturn
post_link_add (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
  Wire *w = (Wire *) user_data;
  Graph *g = w->g;
  Machine *ms = w->ms, *md = w->md;
  GstPadLinkReturn plr;
  GstStateChangeReturn scr;

  if (pad) {
    if (!g_atomic_int_compare_and_exchange (&in_idle_probe, FALSE, TRUE))
      return GST_PAD_PROBE_OK;

    GST_WARNING ("link %s -> %s blocked", GST_OBJECT_NAME (ms->bin),
        GST_OBJECT_NAME (md->bin));
  }

  /* link ghostpads (downstream) */
  plr = gst_pad_link (w->src_ghost, w->peer_dst_ghost);
  g_assert_cmpint (plr, ==, GST_PAD_LINK_OK);

  dump_pipeline (g, step, "wire_add_blocking_linked");

  /* change state of machines */
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
    dump_pipeline (g, step, "wire_add_blocking_synced");
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
  gchar w_name[50];
  GstPadLinkReturn plr;

  g->w[s][d] = w = g_new0 (Wire, 1);
  w->g = g;
  w->ms = ms;
  w->md = md;
  w->as = (ms->pads == 0);      // activate if we don't yet have pads
  w->ad = (md->pads == 0);

  GST_WARNING ("link %s -> %s (as=%d,ad=%d)", GST_OBJECT_NAME (ms->bin),
      GST_OBJECT_NAME (md->bin), w->as, w->ad);
  sprintf (w_name, "%s - %s", GST_OBJECT_NAME (ms->bin),
      GST_OBJECT_NAME (md->bin));


  if (!(w->bin = (GstBin *) gst_bin_new (w_name))) {
    GST_ERROR ("Can't create bin");
    exit (-1);
  }
  if (!(w->queue = gst_element_factory_make ("queue", NULL))) {
    GST_ERROR ("Can't create element queue");
    exit (-1);
  }
  if (!gst_bin_add (w->bin, w->queue)) {
    GST_ERROR ("Can't add element queue to bin");
    exit (-1);
  }

  /* request machine pads */
  w->peer_src = gst_element_get_request_pad (ms->tee, "src_%u");
  g_assert (w->peer_src);
  w->peer_src_ghost = gst_ghost_pad_new (NULL, w->peer_src);
  g_assert (w->peer_src_ghost);
  if (!w->as) {
    if (!gst_pad_set_active (w->peer_src_ghost, TRUE)) {
      GST_WARNING_OBJECT (w->peer_src_ghost, "could not activate");
    }
  }
  if (!gst_element_add_pad ((GstElement *) ms->bin, w->peer_src_ghost)) {
    GST_ERROR_OBJECT (ms->bin, "Failed to add src ghost pad to element bin");
    exit (-1);
  }
  ms->pads++;

  w->peer_dst = gst_element_get_request_pad (md->mix, "sink_%u");
  g_assert (w->peer_dst);
  w->peer_dst_ghost = gst_ghost_pad_new (NULL, w->peer_dst);
  g_assert (w->peer_dst_ghost);
  if (!w->ad) {
    if (!gst_pad_set_active (w->peer_dst_ghost, TRUE)) {
      GST_WARNING_OBJECT (w->peer_dst_ghost, "could not activate");
    }
  }
  if (!gst_element_add_pad ((GstElement *) md->bin, w->peer_dst_ghost)) {
    GST_ERROR_OBJECT (md->bin, "Failed to add sink ghost pad to element bin");
    exit (-1);
  }
  md->pads++;

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

  /* block w->peer_src (before linking) */
  if ((GST_STATE (g->bin) == GST_STATE_PLAYING) && !w->as && M_IS_SRC (ms)) {
    GST_WARNING ("link %s -> %s blocking", GST_OBJECT_NAME (ms->bin),
        GST_OBJECT_NAME (md->bin));
    dump_pipeline (g, step, "wire_add_blocking");
    in_idle_probe = TRUE;
    gst_pad_add_probe (w->peer_src, PROBE_TYPE, post_link_add, w, NULL);
  } else {
    GST_WARNING ("link %s -> %s continuing", GST_OBJECT_NAME (ms->bin),
        GST_OBJECT_NAME (md->bin));
    post_link_add (NULL, NULL, w);
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
  dump_pipeline (g, step, "wire_rem_blocking_synced");

  /* unlink ghostpads (upstream) */
  plr = gst_pad_unlink (w->src_ghost, w->peer_dst_ghost);
  g_assert (plr == TRUE);
  plr = gst_pad_unlink (w->peer_src_ghost, w->dst_ghost);
  g_assert (plr == TRUE);
  dump_pipeline (g, step, "wire_rem_blocking_unlinked");

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

  /* unblock w->peer_src_ghost, the pad gets removed above
   * if (pad)
   *   gst_pad_set_blocked (pad, FALSE);
   */

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

  w->as = (ms->pads == 1);      // deactivate if this is the last pad
  w->ad = (md->pads == 1);
  g->w[s][d] = NULL;

  GST_WARNING ("link %s -> %s (as=%d,ad=%d)", GST_OBJECT_NAME (ms->bin),
      GST_OBJECT_NAME (md->bin), w->as, w->ad);

  /* block w->peer_dst */
  if (GST_STATE (g->bin) == GST_STATE_PLAYING) {
    GST_WARNING ("link %s -> %s blocking", GST_OBJECT_NAME (ms->bin),
        GST_OBJECT_NAME (md->bin));
    dump_pipeline (g, step, "wire_rem_blocking");
    gst_pad_add_probe (w->peer_dst, PROBE_TYPE, post_link_rem, w, NULL);
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
    gchar *sstr;

    sstr = gst_structure_to_string (s);
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
