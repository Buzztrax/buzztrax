/* test dynamic linking
 *
 * gcc -Wall -g dynlink2.c -o dynlink2 `pkg-config gstreamer-1.0 --cflags --libs`
 * GST_DEBUG="*:2" ./dynlink2
 * GST_DEBUG_DUMP_DOT_DIR=$PWD ./dynlink2
 * for file in dyn*.dot; do echo $file; dot -Tpng $file -o${file/dot/png}; done
 */

#include "dynlink.h"

/* dot file index */
static gint dfix = 0;

/* dynamic linking */

static GstPadProbeReturn
post_link_add (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
  Wire *w = (Wire *) user_data;
  Machine *ms = w->ms, *md = w->md;
  GstPadLinkReturn plr;
  GstStateChangeReturn scr;

  if (pad)
    GST_WARNING ("+ link %s -> %s blocked", GST_OBJECT_NAME (ms->bin),
        GST_OBJECT_NAME (md->bin));

  // link ghostpads (downstream)
  plr = gst_pad_link (w->peer_src_ghost, w->dst_ghost);
  g_assert (plr == GST_PAD_LINK_OK);
  plr = gst_pad_link (w->src_ghost, w->peer_dst_ghost);
  g_assert (plr == GST_PAD_LINK_OK);

  // change state (downstream) and unlock
  if (w->as) {
    GST_WARNING ("+ %s", GST_OBJECT_NAME (ms->bin));
    gst_element_set_locked_state ((GstElement *) ms->bin, FALSE);
    scr = gst_element_set_state ((GstElement *) ms->bin, GST_STATE_PLAYING);
    g_assert (scr != GST_STATE_CHANGE_FAILURE);
  }
  scr = gst_element_set_state ((GstElement *) w->bin, GST_STATE_PLAYING);
  g_assert (scr != GST_STATE_CHANGE_FAILURE);
  if (w->ad) {
    GST_WARNING ("+ %s", GST_OBJECT_NAME (md->bin));
    gst_element_set_locked_state ((GstElement *) md->bin, FALSE);
    scr = gst_element_set_state ((GstElement *) md->bin, GST_STATE_PLAYING);
    g_assert (scr != GST_STATE_CHANGE_FAILURE);
  }

  GST_WARNING ("+ link %s -> %s done", GST_OBJECT_NAME (ms->bin),
      GST_OBJECT_NAME (md->bin));

  return GST_PAD_PROBE_REMOVE;
}

static void
link_add (Graph * g, gint s, gint d)
{
  Wire *w;
  Machine *ms = g->m[s], *md = g->m[d];

  GST_WARNING ("+ link %s -> %s", GST_OBJECT_NAME (ms->bin),
      GST_OBJECT_NAME (md->bin));

  g->w[s][d] = w = make_wire (g, ms, md);

  // request machine pads
  add_request_pad (ms, ms->tee, &w->peer_src, &w->peer_src_ghost, "src_%u",
      w->as);
  add_request_pad (md, md->mix, &w->peer_dst, &w->peer_dst_ghost, "sink_%u",
      w->ad);

  // create wire pads
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

  // add wire to pipeline
  gst_bin_add (g->bin, (GstElement *) w->bin);

  // block w->peer_src_ghost
  // - before adding?, no, elements are in locked state
  // - before linking!
  if (!w->as && GST_STATE (g->bin) == GST_STATE_PLAYING) {
    gst_pad_add_probe (w->peer_src_ghost, GST_PAD_PROBE_TYPE_BLOCK,
        post_link_add, w, NULL);
  } else {
    post_link_add (NULL, NULL, w);
  }
}

static GstPadProbeReturn
post_link_rem (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
  Wire *w = (Wire *) user_data;
  Machine *ms = w->ms, *md = w->md;
  gboolean plr;
  GstStateChangeReturn scr;

  if (pad)
    GST_WARNING ("- link %s -> %s blocked", GST_OBJECT_NAME (ms->bin),
        GST_OBJECT_NAME (md->bin));

  // change state (upstream) and lock
  if (w->ad) {
    GST_WARNING ("- %s", GST_OBJECT_NAME (md->bin));
    scr = gst_element_set_state ((GstElement *) md->bin, GST_STATE_NULL);
    g_assert (scr != GST_STATE_CHANGE_FAILURE);
    gst_element_set_locked_state ((GstElement *) md->bin, TRUE);
  }
  scr = gst_element_set_state ((GstElement *) w->bin, GST_STATE_NULL);
  g_assert (scr != GST_STATE_CHANGE_FAILURE);
  if (w->as) {
    GST_WARNING ("- %s", GST_OBJECT_NAME (md->bin));
    scr = gst_element_set_state ((GstElement *) ms->bin, GST_STATE_NULL);
    g_assert (scr != GST_STATE_CHANGE_FAILURE);
    gst_element_set_locked_state ((GstElement *) ms->bin, TRUE);
  }
  // unlink ghostpads (upstream)
  plr = gst_pad_unlink (w->src_ghost, w->peer_dst_ghost);
  g_assert (plr == TRUE);
  plr = gst_pad_unlink (w->peer_src_ghost, w->dst_ghost);
  g_assert (plr == TRUE);

  // remove from pipeline
  gst_object_ref (w->bin);
  gst_bin_remove ((GstBin *) GST_OBJECT_PARENT (w->bin), (GstElement *) w->bin);

  // release request pads
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

  GST_WARNING ("- link %s -> %s done", GST_OBJECT_NAME (ms->bin),
      GST_OBJECT_NAME (md->bin));
  return GST_PAD_PROBE_REMOVE;
}

static void
link_rem (Graph * g, gint s, gint d)
{
  Wire *w = g->w[s][d];
  Machine *ms = g->m[s], *md = g->m[d];

  GST_WARNING ("- link %s -> %s", GST_OBJECT_NAME (ms->bin),
      GST_OBJECT_NAME (md->bin));

  w->as = (ms->pads == 1);
  w->ad = (md->pads == 1);
  g->w[s][d] = NULL;

  // block w->peer_src_ghost
  if (GST_STATE (g->bin) == GST_STATE_PLAYING) {
    gst_pad_add_probe (w->peer_src_ghost, GST_PAD_PROBE_TYPE_BLOCK,
        post_link_rem, w, NULL);
  } else {
    post_link_rem (NULL, NULL, w);
  }
}

static void
change_links (Graph * g, gint o, gint n)
{
  GST_WARNING ("==== change %02d -> %02d ====", o, n);
  dump_pipeline (g, dfix, NULL);
  dfix++;

  // bit 1
  if ((o & 1) == 0 && (n & 1) != 0) {
    link_add (g, M_SRC1, M_FX);
  } else if ((o & 1) != 0 && (n & 1) == 0) {
    link_rem (g, M_SRC1, M_FX);
  }
  // bit 2
  if ((o & 2) == 0 && (n & 2) != 0) {
    link_add (g, M_SRC2, M_FX);
  } else if ((o & 2) != 0 && (n & 2) == 0) {
    link_rem (g, M_SRC2, M_FX);
  }
  // bit 3
  if ((o & 4) == 0 && (n & 4) != 0) {
    link_add (g, M_SRC2, M_SINK);
  } else if ((o & 4) != 0 && (n & 4) == 0) {
    link_rem (g, M_SRC2, M_SINK);
  }
  // bit 4
  if ((o & 8) == 0 && (n & 8) != 0) {
    link_add (g, M_FX, M_SINK);
  } else if ((o & 8) != 0 && (n & 8) == 0) {
    link_rem (g, M_FX, M_SINK);
  }
  // wait a bit
  g_usleep (G_USEC_PER_SEC);
}

/* bus helper */

static void
message_received (GstBus * bus, GstMessage * message, GstPipeline * pipeline)
{
  const GstStructure *s;

  s = gst_message_get_structure (message);
  g_print ("message from \"%s\" (%s): ",
      GST_STR_NULL (GST_ELEMENT_NAME (GST_MESSAGE_SRC (message))),
      gst_message_type_get_name (GST_MESSAGE_TYPE (message)));
  if (s) {
    gchar *sstr = gst_structure_to_string (s);
    GST_WARNING ("%s", sstr);
    g_free (sstr);
  } else {
    GST_WARNING ("no message details");
  }
}

/* test application */

gint
main (gint argc, gchar ** argv)
{
  Graph *g;
  GstBus *bus;
  gint i, j, c;

  /* init gstreamer */
  gst_init (&argc, &argv);

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "dynlink", 0,
      "dynamic linking test");

  GST_WARNING ("setup");

  g = make_graph ();

  /* connect to the bus */
  bus = gst_pipeline_get_bus (GST_PIPELINE (g->bin));
  gst_bus_add_signal_watch_full (bus, G_PRIORITY_HIGH);
  g_signal_connect (bus, "message::error", G_CALLBACK (message_received),
      g->bin);
  g_signal_connect (bus, "message::warning", G_CALLBACK (message_received),
      g->bin);

  /* do the initial link and play */
  link_add (g, M_SRC1, M_SINK);
  gst_element_set_state ((GstElement *) g->bin, GST_STATE_PLAYING);

  gst_element_seek_simple ((GstElement *) g->bin, GST_FORMAT_TIME,
      GST_SEEK_FLAG_FLUSH, G_GINT64_CONSTANT (0));

  // wait a bit
  g_usleep (G_USEC_PER_SEC);

  GST_WARNING ("testing");
  dump_pipeline (g, dfix, "before");
  dfix++;

  /* run the dynlink variants */
  c = 1 << 4;
  for (j = 0, i = 1; i < c; i++) {
    /* skip some partially connected setups */
    if ((i == 1) || (i == 2) || (i == 3) || (i == 5) || (i == 7) || (i == 8))
      continue;
    change_links (g, j, i);
    j = i;
  }
  for (j = c - 1, i = c - 2; i > -1; i--) {
    /* skip some partially connected setups */
    if ((i == 1) || (i == 2) || (i == 3) || (i == 5) || (i == 7) || (i == 8))
      continue;
    change_links (g, j, i);
    j = i;
  }

  dump_pipeline (g, dfix, "after");
  dfix++;
  GST_WARNING ("cleanup");

  /* cleanup */
  gst_element_set_state ((GstElement *) g->bin, GST_STATE_NULL);
  gst_bus_remove_signal_watch (bus);
  gst_object_unref (GST_OBJECT (bus));
  free_graph (g);

  GST_WARNING ("done");
  exit (0);
}
