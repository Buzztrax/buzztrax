/** $Id: dynlink2.c 1903 2008-08-07 19:04:18Z ensonic $
 * test dynamic linking
 *
 * gcc -Wall -g `pkg-config gstreamer-0.10 --cflags --libs` dynlink2.c -o dynlink2
 * GST_DEBUG="*:2" ./dynlink2
 * GST_DEBUG_DUMP_DOT_DIR=$PWD ./dynlink2
 * for file in dyn*.dot; do echo $file; dot -Tpng $file -o${file/dot/png}; done
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
 */

#include <stdio.h>
#include <gst/gst.h>

#define SINK_NAME "pulsesink"
#define FX_NAME "volume"
#define SRC_NAME "audiotestsrc"

#define GST_CAT_DEFAULT bt_dynlink2
GST_DEBUG_CATEGORY_STATIC(GST_CAT_DEFAULT);

static GstDebugGraphDetails graph_details = 
//  GST_DEBUG_GRAPH_SHOW_ALL & ~GST_DEBUG_GRAPH_SHOW_CAPS_DETAILS;
  GST_DEBUG_GRAPH_SHOW_ALL;

typedef struct {
  GstBin *bin;
  // elements
  GstElement *mix;
  GstElement *elem;
  GstElement *tee;
  gint pads;
} Machine;

typedef struct {
  GstBin *bin;
  GstElement *q;
  Machine *ms, *md;
  // element pads and ghost pad for bin
  GstPad *s, *sg;
  GstPad *d, *dg;
  // peer tee/mix request pad and ghost pad for bin
  GstPad *ps, *psg;
  GstPad *pd, *pdg;
  // activate/deactivate when linking/unlinking
  gboolean as, ad;
} Wire;

enum {
  M_SRC1 = 0,
  M_SRC2,
  M_FX,
  M_SINK,
  M_
};

typedef struct {
  GstBin *bin;
  Machine *m[M_];
  Wire    *w[M_][M_];
} Graph;

/* dot file index */
static gint dfix = 0;

/* setup helper */

static Machine *make_machine (Graph *g, const gchar *elem_name, const gchar *bin_name) {
  Machine *m;
  
  m = g_new0 (Machine, 1);
  
  if (!(m->bin = (GstBin *) gst_bin_new (bin_name))) {
    GST_ERROR ("Can't create bin");exit (-1);
  }
  gst_bin_add (g->bin, (GstElement *)m->bin);
  gst_element_set_locked_state ((GstElement *)m->bin, TRUE);

  if (!(m->elem = gst_element_factory_make (elem_name, NULL))) {
    GST_ERROR ("Can't create element %s", elem_name);exit (-1);
  }
  gst_bin_add (m->bin, m->elem);

  return (m);
}

static void add_tee (Machine *m) {
  if (!(m->tee = gst_element_factory_make ("tee", NULL))) {
    GST_ERROR ("Can't create element tee");exit (-1);
  }
  gst_bin_add (m->bin, m->tee);
  gst_element_link (m->elem, m->tee);
}

static void add_mix (Machine *m) {
  if (!(m->mix = gst_element_factory_make ("adder", NULL))) {
    GST_ERROR ("Can't create element adder");exit (-1);
  }
  gst_bin_add (m->bin, m->mix);
  gst_element_link (m->mix, m->elem);
}

static Machine *make_src (Graph *g, const gchar *m_name) {
  Machine *m;

  m = make_machine (g, SRC_NAME, m_name);
  add_tee (m);
  return (m);
}

static Machine *make_fx (Graph *g, const gchar *m_name) {
  Machine *m;

  m = make_machine (g, FX_NAME, m_name);
  add_tee (m);
  add_mix (m);
  return (m);
}

static Machine *make_sink (Graph *g, const gchar *m_name) {
  Machine *m;

  m = make_machine (g, SINK_NAME, m_name);
  add_mix (m);
  return (m);
}

static void post_link_add (GstPad *pad, gboolean blocked, gpointer user_data) {
  Wire *w = (Wire *)user_data;
  Machine *ms = w->ms, *md = w->md;
  GstPadLinkReturn plr;
  GstStateChangeReturn scr;
  
  if (!blocked) {
    GST_ERROR ("Pad block before linking elements failed");
    return;
  }
  
  if (pad)
    GST_WARNING ("+ link %s -> %s blocked", GST_OBJECT_NAME (ms->bin), GST_OBJECT_NAME (md->bin));

  // link ghostpads (downstream)
  plr = gst_pad_link (w->psg, w->dg);
  g_assert (plr == GST_PAD_LINK_OK);
  plr = gst_pad_link (w->sg, w->pdg);
  g_assert (plr == GST_PAD_LINK_OK);

  // change state (downstream) and unlock
  if (w->as) {
    GST_WARNING ("+ %s", GST_OBJECT_NAME (ms->bin));
    gst_element_set_locked_state ((GstElement *)ms->bin, FALSE);
    scr = gst_element_set_state ((GstElement *)ms->bin, GST_STATE_PLAYING);
    g_assert (scr != GST_STATE_CHANGE_FAILURE);
  }
  scr = gst_element_set_state ((GstElement *)w->bin, GST_STATE_PLAYING);
  g_assert (scr != GST_STATE_CHANGE_FAILURE);
  if (w->ad) {
    GST_WARNING ("+ %s", GST_OBJECT_NAME (md->bin));
    gst_element_set_locked_state ((GstElement *)md->bin, FALSE);
    scr = gst_element_set_state ((GstElement *)md->bin, GST_STATE_PLAYING);
    g_assert (scr != GST_STATE_CHANGE_FAILURE);
  }

  // unblock w->psg
  if (pad)
    gst_pad_set_blocked (pad, FALSE);

  GST_WARNING ("+ link %s -> %s done", GST_OBJECT_NAME (ms->bin), GST_OBJECT_NAME (md->bin));
}

static void link_add (Graph *g, gint s,  gint d) {
  Wire *w;
  Machine *ms = g->m[s], *md = g->m[d];
  gchar w_name[50];

  GST_WARNING ("+ link %s -> %s", GST_OBJECT_NAME (ms->bin), GST_OBJECT_NAME (md->bin));
  sprintf (w_name, "%s - %s", GST_OBJECT_NAME (ms->bin), GST_OBJECT_NAME (md->bin));
  
  g->w[s][d] = w = g_new0 (Wire, 1);
  w->ms = ms;
  w->md = md;
  w->as = (ms->pads == 0);
  w->ad = (md->pads == 0);

  if (!(w->bin = (GstBin *) gst_bin_new (w_name))) {
    GST_ERROR ("Can't create bin");exit (-1);
  }
  if (!(w->q = gst_element_factory_make ("queue", NULL))) {
    GST_ERROR ("Can't create element queue");exit (-1);
  }
  gst_bin_add (w->bin, w->q);
  
  // request machine pads
  w->ps = gst_element_get_request_pad (ms->tee, "src%d");
  g_assert (w->ps);
  w->psg = gst_ghost_pad_new (NULL, w->ps);
  g_assert (w->psg);
  if(!w->as) {
    gst_pad_set_active (w->psg, TRUE);
  }
  gst_element_add_pad ((GstElement *)ms->bin, w->psg);
  ms->pads++;

  w->pd = gst_element_get_request_pad (md->mix, "sink%d");
  g_assert (w->pd);
  w->pdg = gst_ghost_pad_new (NULL, w->pd);
  g_assert (w->pdg);
  if(!w->ad) {
    gst_pad_set_active (w->pdg, TRUE);
  }
  gst_element_add_pad ((GstElement *)md->bin, w->pdg);
  md->pads++;

  // create wire pads
  w->s = gst_element_get_static_pad (w->q, "src");
  g_assert (w->s);
  w->sg = gst_ghost_pad_new (NULL, w->s);
  g_assert (w->sg);
  gst_element_add_pad ((GstElement *)w->bin, w->sg);
  w->d = gst_element_get_static_pad (w->q, "sink");
  g_assert (w->d);
  w->dg = gst_ghost_pad_new (NULL, w->d);
  g_assert (w->dg);
  gst_element_add_pad ((GstElement *)w->bin, w->dg);  
    
  // add wire to pipeline
  gst_bin_add (g->bin, (GstElement *)w->bin);

  // block w->psg
  // - before adding?, no, elements are in locked state
  // - before linking!
  if (!w->as && GST_STATE (g->bin) == GST_STATE_PLAYING) {
    gst_pad_set_blocked_async (w->psg, TRUE, post_link_add, w);
  } else {
    post_link_add (NULL, TRUE, w);
  }
}

static void post_link_rem (GstPad *pad, gboolean blocked, gpointer user_data) {
  Wire *w = (Wire *)user_data;
  Machine *ms = w->ms, *md = w->md;
  gboolean plr;
  GstStateChangeReturn scr;

  if (!blocked) {
    GST_ERROR ("Pad block before unlinking elements failed");
    return;
  }
  
  if (pad)
    GST_WARNING ("- link %s -> %s blocked", GST_OBJECT_NAME (ms->bin), GST_OBJECT_NAME (md->bin));
  
  // change state (upstream) and lock
  if (w->ad) {
    GST_WARNING ("- %s", GST_OBJECT_NAME (md->bin));
    scr = gst_element_set_state ((GstElement *)md->bin, GST_STATE_NULL);
    g_assert (scr != GST_STATE_CHANGE_FAILURE);
    gst_element_set_locked_state ((GstElement *)md->bin, TRUE);
  }
  scr = gst_element_set_state ((GstElement *)w->bin, GST_STATE_NULL);
  g_assert (scr != GST_STATE_CHANGE_FAILURE);
  if (w->as) {
    GST_WARNING ("- %s", GST_OBJECT_NAME (md->bin));
    scr = gst_element_set_state ((GstElement *)ms->bin, GST_STATE_NULL);
    g_assert (scr != GST_STATE_CHANGE_FAILURE);
    gst_element_set_locked_state ((GstElement *)ms->bin, TRUE);
  }
  
  // unlink ghostpads (upstream)
  plr = gst_pad_unlink (w->sg, w->pdg);
  g_assert (plr == TRUE);
  plr = gst_pad_unlink (w->psg, w->dg);
  g_assert (plr == TRUE);

  // remove from pipeline
  gst_object_ref (w->bin);
  gst_bin_remove ((GstBin *)GST_OBJECT_PARENT (w->bin), (GstElement *)w->bin);
  
  // release request pads
  gst_element_remove_pad ((GstElement *)ms->bin, w->psg);
  gst_element_release_request_pad (ms->tee, w->ps);
  ms->pads--;
  gst_element_remove_pad ((GstElement *)md->bin, w->pdg);
  gst_element_release_request_pad (md->mix, w->pd);
  md->pads--;

  gst_object_unref (w->s);
  gst_object_unref (w->d);
  gst_object_unref (w->bin);
  g_free (w);  

  // unblock w->psg, the pad gets removed above
  // if (pad)
  //   gst_pad_set_blocked (pad, FALSE);

  GST_WARNING ("- link %s -> %s done", GST_OBJECT_NAME (ms->bin), GST_OBJECT_NAME (md->bin));
}

static void link_rem (Graph *g, gint s,  gint d) {
  Wire *w = g->w[s][d];
  Machine *ms = g->m[s], *md = g->m[d];
  
  GST_WARNING ("- link %s -> %s", GST_OBJECT_NAME (ms->bin), GST_OBJECT_NAME (md->bin));

  w->as = (ms->pads == 1);
  w->ad = (md->pads == 1);
  g->w[s][d] = NULL;

  // block w->psg
  if (GST_STATE (g->bin) == GST_STATE_PLAYING) {
    gst_pad_set_blocked_async (w->psg, TRUE, post_link_rem, w);
  } else {
    post_link_rem (NULL, TRUE, w);
  }
}

static void change_links (Graph *g, gint o, gint n) {
  gchar t[20];
  
  GST_WARNING ("== change %02d -> %02d ==", o, n);
  sprintf (t,"dyn%02d_%02d_%02d", dfix, o, n);
  GST_DEBUG_BIN_TO_DOT_FILE (g->bin, graph_details, t);
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

static void message_received (GstBus * bus, GstMessage * message, GstPipeline * pipeline) {
  const GstStructure *s;

  s = gst_message_get_structure (message);
  g_print ("message from \"%s\" (%s): ",
      GST_STR_NULL (GST_ELEMENT_NAME (GST_MESSAGE_SRC (message))),
      gst_message_type_get_name (GST_MESSAGE_TYPE (message)));
  if (s) {
    gchar *sstr;

    sstr = gst_structure_to_string (s);
    GST_WARNING ("%s", sstr);
    g_free (sstr);
  }
  else {
    GST_WARNING ("no message details");
  }
}

/*
static void state_changed_message_received (GstBus * bus, GstMessage * message, GstPipeline * pipeline) {
  GstState oldstate,newstate,pending;

  gst_message_parse_state_changed(message,&oldstate,&newstate,&pending);
}
*/

/* test application */

int main(int argc, char **argv) {
  Graph *g;
  GstBus *bus;
  gint i, j, c;
  gchar t[20];

  /* init gstreamer */
  gst_init(&argc, &argv);
  g_log_set_always_fatal(G_LOG_LEVEL_WARNING);
  
  GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "dynlink2", 0, "dynamic linking test");
  
  GST_WARNING ("setup");
  
  g = g_new0 (Graph, 1);

  /* create a new top-level pipeline to hold the elements */
  g->bin = (GstBin *)gst_pipeline_new ("song");
  /* and connect to the bus */
  bus = gst_pipeline_get_bus (GST_PIPELINE (g->bin));
  gst_bus_add_signal_watch_full (bus, G_PRIORITY_HIGH);
  g_signal_connect (bus, "message::error", G_CALLBACK(message_received), g->bin);
  g_signal_connect (bus, "message::warning", G_CALLBACK(message_received), g->bin);
  //g_signal_connect (bus, "message::state-changed", G_CALLBACK(state_changed_message_received), g->bin);
  
  /* make machines */
  g->m[M_SRC1] = make_src (g,"src1");
  g->m[M_SRC2] = make_src (g, "src2");
  g->m[M_FX]   = make_fx (g, "fx");
  g->m[M_SINK] = make_sink (g, "sink");
  
  /* configure the sources */
  g_object_set (g->m[M_SRC1]->elem, "freq", (gdouble)440.0, "wave", 2, NULL);
  g_object_set (g->m[M_SRC2]->elem, "freq", (gdouble)110.0, "wave", 1, NULL);

  /* do the initial link and play */
  link_add (g, M_SRC1, M_SINK);
  gst_element_set_state ((GstElement *)g->bin, GST_STATE_PLAYING);
  
  gst_element_seek_simple ((GstElement *)g->bin, GST_FORMAT_TIME, 
      GST_SEEK_FLAG_FLUSH, G_GINT64_CONSTANT (0));
  
  // wait a bit
  g_usleep (G_USEC_PER_SEC);

  GST_WARNING ("testing");
  sprintf (t,"dyn%02d_before", dfix);
  GST_DEBUG_BIN_TO_DOT_FILE (g->bin, graph_details, t);
  dfix++;
  
  /* run the dynlink variants */
  c = 1 << 4;
  for (j = 0, i = 1; i < c; i++) {
    /* skip some partially connected setups */
    if ((i==1) || (i==2) || (i==3) || (i==5) || (i==7) || (i==8))
      continue;
    change_links (g, j, i);
    j = i;
  }
  for (j = c - 1, i = c - 2; i > -1; i--) {
    /* skip some partially connected setups */
    if ((i==1) || (i==2) || (i==3) || (i==5) || (i==7) || (i==8))
      continue;
    change_links (g, j, i);
    j = i;
  }

  sprintf (t,"dyn%02d_after", dfix);
  GST_DEBUG_BIN_TO_DOT_FILE (g->bin, graph_details, t);
  dfix++;
  GST_WARNING ("cleanup");

  /* cleanup */
  gst_element_set_state ((GstElement *)g->bin, GST_STATE_NULL);
  gst_bus_remove_signal_watch (bus);
  gst_object_unref (GST_OBJECT (bus));
  gst_object_unref (GST_OBJECT (g->bin));
  for (i = 0; i < M_; i++) {
    g_free (g->m[i]); g->m[i] = NULL;
    for (j = 0; j < M_; j++) {
      g_free (g->w[i][j]); g->w[i][j] = NULL;
    }
  }
  g_free (g);
  
  GST_WARNING ("done");
  exit (0);
}
