/* Buzztrax
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
#include <gst/fft/gstfftf64.h>

#include "gst/filter-svf.h"

//-- globals

#define WAVE_SIZE 8192
#define FILTER_MODES (GSTBT_FILTER_SVF_COUNT - GSTBT_FILTER_SVF_LOWPASS)

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

START_TEST (test_create_obj)
{
  BT_TEST_START;
  GstBtFilterSVF *filter;

  GST_INFO ("-- arrange --");
  GST_INFO ("-- act --");
  filter = gstbt_filter_svf_new ();

  GST_INFO ("-- assert --");
  ck_assert (filter != NULL);
  ck_assert (G_OBJECT (filter)->ref_count == 1);

  GST_INFO ("-- cleanup --");
  ck_gst_object_final_unref (filter);
  BT_TEST_END;
}
END_TEST

// cp /tmp/lt-bt_gst_filter-svf_*.svg docs/reference/bt-gst/images/
START_TEST (test_filter)
{
  BT_TEST_START;
  GstBtFilterSVF *filter;
  gdouble freq[WAVE_SIZE];
  GstFFTF64 *fft;
  GstFFTF64Complex ffres[WAVE_SIZE];
  const guint nfft = 2 * WAVE_SIZE - 2;
  const gdouble nfft2 = (gdouble) nfft * (gdouble) nfft;
  gint16 data[nfft];
  gdouble fdata[nfft];
  gint j;
  gchar name[40];
  guint filter_mode = GSTBT_FILTER_SVF_LOWPASS;
  gdouble resonance = 0.7;

  // run all modes twice with resonance off and on
  if (_i >= FILTER_MODES) {
    filter_mode += (_i - FILTER_MODES);
    resonance = 10.0;
  } else {
    filter_mode += _i;
  }

  GST_INFO ("-- arrange --");
  filter = gstbt_filter_svf_new ();
  g_object_set (filter, "filter", filter_mode, "cut-off", 0.5, "resonance",
      resonance, NULL);
  // unit impulse (delta function)
  memset (data, 0, nfft * sizeof (gint16));
  data[0] = G_MAXINT16;

  GST_INFO ("-- act --");
  gstbt_filter_svf_process (filter, nfft, data);
  // test filter response using fft on filtered data
  for (j = 0; j < nfft; j++) {
    fdata[j] =
        (data[j] >
        0) ? ((gdouble) data[j] / (gdouble) G_MAXINT16) : ((gdouble) data[j] /
        (gdouble) - G_MININT16);
  }
  fft = gst_fft_f64_new (nfft, FALSE);
  // when using actual window function we get mangled curves
  gst_fft_f64_window (fft, fdata, GST_FFT_WINDOW_RECTANGULAR);
  gst_fft_f64_fft (fft, fdata, ffres);
  for (j = 0; j < WAVE_SIZE; j++) {
    gdouble val = ffres[j].r * ffres[j].r + ffres[j].i * ffres[j].i;
    freq[j] = 10.0 * log10 (val / nfft2);
  }

  GST_INFO ("-- plot --");
  // inspect data file
  // hexdump -v -e '/8 "%f\n"' /tmp/lt-bt_gst_filter-svf_hipass_0.50.raw | more
  GEnumClass *enum_class =
      g_type_class_peek_static (GSTBT_TYPE_FILTER_SVF_TYPE);
  GEnumValue *enum_value = g_enum_get_value (enum_class, filter_mode);
  sprintf (name, "%s cut-off=0.5 resonance=%.1lf", enum_value->value_name,
      resonance);
  // TODO(ensonc): without logscale it might look better
  // right now x is the data-index anyway, should be frequency
  check_plot_data_double (freq, WAVE_SIZE, "filter-svf", name,
      "set logscale x; set autoscale y;");

  GST_INFO ("-- cleanup --");
  gst_fft_f64_free (fft);
  ck_gst_object_final_unref (filter);
  BT_TEST_END;
}
END_TEST


TCase *
gst_buzztrax_filter_svf_example_case (void)
{
  TCase *tc = tcase_create ("GstBtFilterSVFExamples");

  tcase_add_test (tc, test_create_obj);
  tcase_add_loop_test (tc, test_filter, 0, FILTER_MODES * 2);
  // test gstbt_filter_svf_trigger()
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
