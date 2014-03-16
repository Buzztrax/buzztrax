/* test dynamic linking
 *
 * header with common data structures and helpers
 */
 
/* high level default scenario:
 *   src2                   fx
 *   
 *   src1 <---------------> sink
 *
 * possible links ('src1 ! fx' remains all the time):
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
 * - when to seek on sources to tell them about the segment 
 */

#include <stdio.h>
#include <stdlib.h>
#include <gst/gst.h>

#define SINK_NAME "pulsesink"
#define FX_NAME "volume"
#define SRC_NAME "audiotestsrc"

#define GST_CAT_DEFAULT bt_dynlink
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

typedef struct Machine_ Machine;
typedef struct Wire_ Wire;
typedef struct Graph_ Graph;

#define M_IS_SRC(m) (!m->mix)
#define M_IS_SINK(m) (!m->tee)

#define PROBE_TYPE (GST_PAD_PROBE_TYPE_BUFFER | GST_PAD_PROBE_TYPE_BUFFER_LIST)
//#define PROBE_TYPE GST_PAD_PROBE_TYPE_IDLE

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
  /* wire pads and ghost pad for bin */
  GstPad *src, *src_ghost;
  GstPad *dst, *dst_ghost;
  /* machine peer tee/mix request pad and ghost pad for bin */
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

/* misc helpers */

static void
dump_pipeline (Graph * g, gint ix, gchar * suffix)
{
  gchar t[100];
  static GstDebugGraphDetails graph_details =
      GST_DEBUG_GRAPH_SHOW_ALL & ~GST_DEBUG_GRAPH_SHOW_CAPS_DETAILS;
  //GST_DEBUG_GRAPH_SHOW_ALL;

  snprintf (t, 100, "dyn%02d%s%s", ix, (suffix ? "_" : ""),
      (suffix ? suffix : ""));
  t[99] = '\0';
  GST_DEBUG_BIN_TO_DOT_FILE (g->bin, graph_details, t);
}

/* setup helper */

static Machine *
make_machine (Graph * g, const gchar * elem_name, const gchar * bin_name)
{
  Machine *m = g_new0 (Machine, 1);
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
  if (!gst_bin_add (m->bin, m->elem)) {
    GST_ERROR ("Can't add element %s to bin", elem_name);
    exit (-1);
  }

  return m;
}

static void
add_tee (Machine * m)
{
  if (!(m->tee = gst_element_factory_make ("tee", NULL))) {
    GST_ERROR ("Can't create element tee");
    exit (-1);
  }
  if (!gst_bin_add (m->bin, m->tee)) {
    GST_ERROR ("Can't add element tee to bin");
    exit (-1);
  }
  if (!gst_element_link (m->elem, m->tee)) {
    GST_ERROR ("Can't link %s ! tee", GST_OBJECT_NAME (m->elem));
    exit (-1);
  }
}

static void
add_mix (Machine * m)
{
  if (!(m->mix = gst_element_factory_make ("adder", NULL))) {
    GST_ERROR ("Can't create element adder");
    exit (-1);
  }
  if (!gst_bin_add (m->bin, m->mix)) {
    GST_ERROR ("Can't add element adder to bin");
    exit (-1);
  }
  if (!gst_element_link (m->mix, m->elem)) {
    GST_ERROR ("Can't link %s ! tee", GST_OBJECT_NAME (m->elem));
    exit (-1);
  }
}

static Machine *
make_src (Graph * g, const gchar * m_name)
{
  Machine *m = make_machine (g, SRC_NAME, m_name);
  add_tee (m);
  return m;
}

static Machine *
make_fx (Graph * g, const gchar * m_name)
{
  Machine *m = make_machine (g, FX_NAME, m_name);
  add_tee (m);
  add_mix (m);
  return m;
}

static Machine *
make_sink (Graph * g, const gchar * m_name)
{
  Machine *m = make_machine (g, SINK_NAME, m_name);
  add_mix (m);
  return m;
}

static Wire *
make_wire (Graph * g, Machine *ms, Machine *md)
{
  Wire *w = g_new0 (Wire, 1);
  gchar w_name[50];
  
  w->g = g;
  w->ms = ms;
  w->md = md;
  w->as = (ms->pads == 0);      // activate if we don't yet have pads
  w->ad = (md->pads == 0);
  
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
  /*
  g_object_set (w->queue, "max-size-buffers", 1,
      "max-size-bytes", 0, "max-size-time", G_GUINT64_CONSTANT (0),
      "silent", TRUE, NULL);
      */
  if (!gst_bin_add (w->bin, w->queue)) {
    GST_ERROR ("Can't add element queue to bin");
    exit (-1);
  }
  
  return w;
}

static void
add_request_pad (Machine *m, GstElement *e, GstPad **pad, GstPad **ghost,
    const gchar *tmpl, gboolean active)
{
  *pad = gst_element_get_request_pad (e, tmpl);
  g_assert (*pad);
  *ghost = gst_ghost_pad_new (NULL, *pad);
  g_assert (*ghost);
  if (!active) {
    if (!gst_pad_set_active (*ghost, TRUE)) {
      GST_WARNING_OBJECT (*ghost, "could not activate");
    }
  }
  if (!gst_element_add_pad ((GstElement *) m->bin, *ghost)) {
    GST_ERROR_OBJECT (m->bin, "Failed to add ghost pad to element bin");
    exit (-1);
  }
  m->pads++;
}

static Graph *
make_graph (void)
{
  Graph *g = g_new0 (Graph, 1);
  
  /* create a new top-level pipeline to hold the elements */
  g->bin = (GstBin *) gst_pipeline_new ("song");
  
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
      "freq", (gdouble) 137.0, "wave", 1, "blocksize", 22050, NULL);
  
  return g;
}

static void
free_graph (Graph *g)
{
  gint i, j;

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
}
