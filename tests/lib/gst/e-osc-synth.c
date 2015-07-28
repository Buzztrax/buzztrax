/* GStreamer
 * Copyright (C) 2012 Stefan Sauer <ensonic@users.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "m-bt-gst.h"

#include <stdlib.h>

#include "gst/osc-synth.h"

//-- globals

#define WAVE_SIZE 200

//-- helper
static gint
plot_data (gint16 * d, guint size, const gchar * _name)
{
  gchar *name, *base_name, *raw_name, *img_name, *cmd;
  FILE *f;
  gint ret;

  // convert spaces in names?
  name = g_ascii_strdown (g_strdelimit (g_strdup (_name), " ()", '_'), -1);

  // like in bt-check-ui.c:make_filename()
  // should we have some helper api for this in the test lib?
  base_name = g_strdup_printf ("%s" G_DIR_SEPARATOR_S "%s_%s",
      g_get_tmp_dir (), g_get_prgname (), name);
  raw_name = g_strconcat (base_name, ".raw16", NULL);
  img_name = g_strconcat (base_name, ".svg", NULL);

  if ((f = fopen (raw_name, "wt"))) {
    fwrite (d, size * sizeof (gint16), 1, f);
    fclose (f);
  }
  // http://stackoverflow.com/questions/5826701/plot-audio-data-in-gnuplot
  cmd =
      g_strdup_printf
      ("/bin/sh -c \"echo \\\"set terminal svg size 200,160 fsize 6;set output '%s';"
      "set yrange [-33000:32999];set grid xtics;set grid ytics;"
      "set key outside below;"
      "plot '%s' binary format='%%int16' using 0:1 with lines title '%s'\\\" | gnuplot\"",
      img_name, raw_name, name);
  ret = system (cmd);

  g_free (cmd);
  g_free (img_name);
  g_free (raw_name);
  g_free (base_name);
  g_free (name);
  return ret;
}

//-- fixtures

static void
case_setup (void)
{
  BT_CASE_START;
}

static void
case_teardown (void)
{
}

//-- tests

static void
test_create_obj (BT_TEST_ARGS)
{
  BT_TEST_START;
  GstBtOscSynth *osc;

  GST_INFO ("-- arrange --");
  GST_INFO ("-- act --");
  osc = gstbt_osc_synth_new ();

  GST_INFO ("-- assert --");
  fail_unless (osc != NULL, NULL);
  fail_unless (G_OBJECT (osc)->ref_count == 1, NULL);

  GST_INFO ("-- cleanup --");
  ck_g_object_final_unref (osc);
  BT_TEST_END;
}

// cp /tmp/lt-bt_gst_*.svg docs/reference/bt-gst/images/
static void
test_waves_not_silent (BT_TEST_ARGS)
{
  BT_TEST_START;
  GstBtOscSynth *osc;
  gint16 data[WAVE_SIZE];

  GST_INFO ("-- arrange --");
  osc = gstbt_osc_synth_new ();
  // plot 1 cycle
  g_object_set (osc, "wave", _i, "sample-rate", WAVE_SIZE, "frequency", 1.0,
      NULL);

  GST_INFO ("-- act --");
  osc->process (osc, WAVE_SIZE, data);

  GST_INFO ("-- assert --");
  GEnumClass *enum_class = g_type_class_peek_static (GSTBT_TYPE_OSC_SYNTH_WAVE);
  GEnumValue *enum_value = g_enum_get_value (enum_class, _i);
  plot_data (data, WAVE_SIZE, enum_value->value_name);

  if (_i != GSTBT_OSC_SYNTH_WAVE_SILENCE) {
    gint j;
    for (j = 0; j < WAVE_SIZE; j++) {
      if (data[j] != 0)
        break;
    }
    fail_if (j == WAVE_SIZE, "in %s all samples are 0", enum_value->value_name);
  }

  GST_INFO ("-- cleanup --");
  ck_g_object_final_unref (osc);
  BT_TEST_END;
}

TCase *
gst_buzztrax_osc_synth_example_case (void)
{
  TCase *tc = tcase_create ("GstBtOscSynthExamples");

  tcase_add_test (tc, test_create_obj);
  tcase_add_loop_test (tc, test_waves_not_silent, 0,
      GSTBT_OSC_SYNTH_WAVE_COUNT);
  // test each wave with a volume and frequency decay env
  // test that for a larger wave, summing up all values should be ~0
  // test that for non noise waves, we should get min/max
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
