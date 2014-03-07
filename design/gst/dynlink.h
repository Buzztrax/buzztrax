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

//#define PROBE_TYPE GST_PAD_PROBE_TYPE_BLOCK
#define PROBE_TYPE GST_PAD_PROBE_TYPE_IDLE

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
  if (!gst_bin_add (m->bin, m->elem)) {
    GST_ERROR ("Can't add element %s to bin", elem_name);
    exit (-1);
  }

  return (m);
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

