/* GStreamer
 * Copyright (C) 2015 Stefan Sauer <ensonic@users.sf.net>
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

#include <string.h>
#include <gst/fft/gstffts16.h>

#include "gst/filter-svf.h"

//-- globals

#define WAVE_SIZE 1024

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
  GstBtFilterSVF *filter;

  GST_INFO ("-- arrange --");
  GST_INFO ("-- act --");
  filter = gstbt_filter_svf_new ();

  GST_INFO ("-- assert --");
  fail_unless (filter != NULL, NULL);
  fail_unless (G_OBJECT (filter)->ref_count == 1, NULL);

  GST_INFO ("-- cleanup --");
  ck_gst_object_final_unref (filter);
  BT_TEST_END;
}

static void
test_filter (BT_TEST_ARGS)
{
  BT_TEST_START;
  GstBtFilterSVF *filter;
  gint16 data[WAVE_SIZE * 2];
  gdouble freq[WAVE_SIZE];
  GstFFTS16 *fft;
  GstFFTS16Complex freqdata[WAVE_SIZE];
  guint nfft = 2 * WAVE_SIZE - 2;
  gdouble nfft2 = (gdouble) nfft * (gdouble) nfft;
  gint j;
  gchar name[30];

  GST_INFO ("-- arrange --");
  filter = gstbt_filter_svf_new ();
  g_object_set (filter, "filter", _i, "cut-off", 0.5, "resonance", 0.0, NULL);
  // unit impulse (delta function)
  memset (data, 0, WAVE_SIZE * sizeof (gint16));
  data[0] = G_MAXINT16;

  GST_INFO ("-- act --");
  gstbt_filter_svf_process (filter, WAVE_SIZE * 2, data);
  // test filter response
  // calculate fft on data
  fft = gst_fft_s16_new (nfft, FALSE);
  // 
  gst_fft_s16_window (fft, data, GST_FFT_WINDOW_RECTANGULAR);
  gst_fft_s16_fft (fft, data, freqdata);
  for (j = 0; j < WAVE_SIZE; j++) {
    gdouble val = freqdata[j].r * freqdata[j].r + freqdata[j].i * freqdata[j].i;
    freq[j] = 10.0 * log10 (val / nfft2);
  }

  GST_INFO ("-- plot --");
  GEnumClass *enum_class =
      g_type_class_peek_static (GSTBT_TYPE_FILTER_SVF_TYPE);
  GEnumValue *enum_value = g_enum_get_value (enum_class, _i);
  sprintf (name, "%s %4.2f", enum_value->value_name, 0.5);
  check_plot_data_double (freq, WAVE_SIZE, "filter-svf", name,
      "set logscale x; set autoscale y;");

  GST_INFO ("-- cleanup --");
  gst_fft_s16_free (fft);
  ck_gst_object_final_unref (filter);
  BT_TEST_END;
}


TCase *
gst_buzztrax_filter_svf_example_case (void)
{
  TCase *tc = tcase_create ("GstBtFilterSVFExamples");

  tcase_add_test (tc, test_create_obj);
  tcase_add_loop_test (tc, test_filter, GSTBT_FILTER_SVF_LOWPASS,
      GSTBT_FILTER_SVF_COUNT);
  // test gstbt_filter_svf_trigger()
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
