/* more complex controller example
 *
 * gcc -Wall -g `pkg-config --cflags --libs gstreamer-0.10 gstreamer-controller-0.10 libgstbuzztard` controller.c -o controller
 */

#include <glib.h>
#include <gst/gst.h>
#include <gst/controller/gstcontroller.h>
#include <gst/controller/gstinterpolationcontrolsource.h>
#include <gst/controller/gstlfocontrolsource.h>
#include <libgstbuzztard/tempo.h>

gint
main (gint argc, gchar **argv)
{
  GstElement *bin;
  GstElement *src, *fx, *sink;
  GstClock *clock;
  GstClockID clock_id;
  GstController *ctrl;
  GstInterpolationControlSource *int_csrc;
  GstLFOControlSource *lfo_csrc;
  GValue val = { 0, };
  GstClockTime ts, st;
  gint i;
  
  /* init gstreamer */
  gst_init (&argc, &argv);
  gst_controller_init (&argc, &argv);

  /* create a new bin to hold the elements */
  bin = gst_pipeline_new ("song");
  clock = gst_pipeline_get_clock (GST_PIPELINE (bin));

  /* make a sink */
  if(!(sink = gst_element_factory_make ("autoaudiosink", "sink"))) {
    GST_WARNING ("Can't create element \"autoaudiosink\"");
    return -1;
  }

  /* make an effect */
  if(!(fx = gst_element_factory_make ("audiodelay", "fx"))) {
    GST_WARNING ("Can't create element \"audiodelay\"");
    return -1;
  }
  g_object_set (fx, "delaytime", 25, NULL);

  /* make source and configure */
  if (!(src = gst_element_factory_make ("simsyn", "src"))) {
    GST_WARNING ("Can't create element \"simsyn\"");
    return -1;
  }
  g_object_set (src, "wave", /* saw */ 2, "filter", /* lowpass */ 1, "decay", 0.75, NULL);
  gstbt_tempo_change_tempo (GSTBT_TEMPO (src), 125, 32, -1);

  /* add objects to the main bin */
  gst_bin_add_many (GST_BIN (bin), src, fx, sink, NULL);
  
  /* link elements */
  if (!gst_element_link_many (src, fx, sink, NULL)) {
    GST_WARNING ("Can't link elements");
    return -1;
  }
  
  /* get the controller */
  if (!(ctrl = gst_controller_new (G_OBJECT (src), "note", "cut-off",
    "resonance", NULL))) {
    GST_WARNING ("can't control source element");
    return -1;
  }
  
  /* programm a pattern of notes */
  int_csrc = gst_interpolation_control_source_new ();
  gst_controller_set_control_source (ctrl, "note",
      GST_CONTROL_SOURCE (int_csrc));
  gst_interpolation_control_source_set_interpolation_mode (int_csrc,
      GST_INTERPOLATE_NONE);

  ts = G_GUINT64_CONSTANT (0);
  st = GST_SECOND / 2;
  g_value_init (&val, G_TYPE_STRING);
  for (i = 0; i < 2; i++) {
    g_value_set_string (&val, "C-3");
    gst_interpolation_control_source_set (int_csrc, ts, &val); ts += st;
    g_value_set_string (&val, "C-2");
    gst_interpolation_control_source_set (int_csrc, ts, &val); ts += st;
    g_value_set_string (&val, "D#2");
    gst_interpolation_control_source_set (int_csrc, ts, &val); ts += st;
    g_value_set_string (&val, "D-2");
    gst_interpolation_control_source_set (int_csrc, ts, &val); ts += st;
    g_value_set_string (&val, "G-2");
    gst_interpolation_control_source_set (int_csrc, ts, &val); ts += st;
    ts += st;
  }
  g_value_unset (&val);
  g_object_unref (int_csrc);

  /* programm a pattern for the cut off */
  int_csrc = gst_interpolation_control_source_new ();
  gst_controller_set_control_source (ctrl, "cut-off",
      GST_CONTROL_SOURCE (int_csrc));
  gst_interpolation_control_source_set_interpolation_mode (int_csrc,
      GST_INTERPOLATE_LINEAR);

  g_value_init (&val, G_TYPE_DOUBLE);
  g_value_set_double (&val, 0.0);
  gst_interpolation_control_source_set (int_csrc, 0 * GST_SECOND, &val);
  g_value_set_double (&val, 1.0);
  gst_interpolation_control_source_set (int_csrc, 5 * GST_SECOND, &val);

  g_value_unset (&val);
  g_object_unref (int_csrc);

  /* programm a pattern for the resonance */
  lfo_csrc = gst_lfo_control_source_new ();
  gst_controller_set_control_source (ctrl, "resonance",
      GST_CONTROL_SOURCE (lfo_csrc));
  
  g_value_init (&val, G_TYPE_DOUBLE);
  g_value_set_double (&val, 1.0);
  g_object_set (lfo_csrc, "frequency", 2.0, "amplitude", &val, NULL);

  g_value_unset (&val);
  g_object_unref (lfo_csrc);

  /* prepare playback */
  gst_element_set_state (bin, GST_STATE_PAUSED);

  /* we want to play for 5 sec. */
  clock_id =
        gst_clock_new_single_shot_id (clock,
        gst_clock_get_time (clock) + (7 * GST_SECOND));
  
  /* play and wait */
  gst_element_set_state (bin, GST_STATE_PLAYING);
  gst_clock_id_wait (clock_id, NULL);
  
  /* stop and cleanup */
  gst_element_set_state (bin, GST_STATE_NULL);
  gst_object_unref (GST_OBJECT (clock));
  gst_object_unref (GST_OBJECT (bin));
  return 0;
}
