/* test dynamic linking
 *
 * gcc -Wall -g dynlink5.c -o dynlink5 `pkg-config gstreamer-1.0 --cflags --libs`
 * GST_DEBUG="*:2" ./dynlink5
 * GST_DEBUG_DUMP_DOT_DIR=$PWD ./dynlink5
 */

#include "dynlink.h"

static gint step = 0;

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
    if (!g_atomic_int_compare_and_exchange (&in_idle_probe, TRUE, FALSE))
      return GST_PAD_PROBE_OK;

    GST_WARNING ("link %s -> %s blocked", GST_OBJECT_NAME (ms->bin),
        GST_OBJECT_NAME (md->bin));
  }

  /* ms->tee ! w->queue */
  plr = gst_pad_link (w->peer_src_ghost, w->dst_ghost);
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

  return GST_PAD_PROBE_REMOVE;
}

static void
link_add (Graph * g, gint s, gint d)
{
  Wire *w;
  Machine *ms = g->m[s], *md = g->m[d];
  GstPadLinkReturn plr;

  g->w[s][d] = w = make_wire (g, ms, md);

  GST_WARNING ("link %s -> %s (as=%d,ad=%d)", GST_OBJECT_NAME (ms->bin),
      GST_OBJECT_NAME (md->bin), w->as, w->ad);

  /* request machine pads */
  add_request_pad (ms, ms->tee, &w->peer_src, &w->peer_src_ghost, "src_%u",
      w->as);
  add_request_pad (md, md->mix, &w->peer_dst, &w->peer_dst_ghost, "sink_%u",
      w->ad);

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

  /* w->queue ! md->mix */
  plr = gst_pad_link (w->src_ghost, w->peer_dst_ghost);
  g_assert_cmpint (plr, ==, GST_PAD_LINK_OK);

  /* block w->peer_src (before linking) */
  if ((GST_STATE (g->bin) == GST_STATE_PLAYING) && !w->as && M_IS_SRC (ms)) {
    GST_WARNING ("link %s -> %s blocking", GST_OBJECT_NAME (ms->bin),
        GST_OBJECT_NAME (md->bin));
    in_idle_probe = TRUE;
    gst_pad_add_probe (w->peer_src, PROBE_TYPE, post_link_add, w, NULL);
    dump_pipeline (g, step, "wire_add_blocking");
  } else {
    GST_WARNING ("link %s -> %s continuing", GST_OBJECT_NAME (ms->bin),
        GST_OBJECT_NAME (md->bin));
    post_link_add (NULL, NULL, w);
  }
}

static gboolean
do_test_step (Graph * g)
{
  if (g->pending_changes == 0) {
    if (step == 2) {
      dump_pipeline (g, step, NULL);
      g_main_loop_quit (g->loop);
    } else {
      g->pending_changes = 1;
      link_add (g, M_SRC1, M_SINK);
      step++;
    }
  } else {
    GST_WARNING ("wait (%d link changes)", g->pending_changes);
    dump_pipeline (g, step, NULL);
  }
  return TRUE;
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
        GST_WARNING_OBJECT (GST_MESSAGE_SRC (message), "reached playing");
        dump_pipeline (g, 0, NULL);
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
  g->pending_changes = 2;
  link_add (g, M_SRC1, M_FX);
  link_add (g, M_FX, M_SINK);
  step++;
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
