/* GStreamer
 * Copyright (C) 2005 Stefan Kost <ensonic@users.sf.net>
 *
 * note2frequency.c: helper class header for unit conversion
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

#ifndef __GSTBT_TONE_CONVERSION_H__
#define __GSTBT_TONE_CONVERSION_H__

#include <glib.h>
#include <glib-object.h>
#include <gst/gst.h>

#include "musicenums.h"

G_BEGIN_DECLS

#define GSTBT_TYPE_TONE_CONVERSION_TUNING     (gstbt_tone_conversion_tuning_get_type())

/**
 * GstBtToneConversionTuning:
 * @GSTBT_TONE_CONVERSION_EQUAL_TEMPERAMENT: 12 tones with equal distance
 * @GSTBT_TONE_CONVERSION_JUST_INTONATION: 12 tones with just intonation
 * @GSTBT_TONE_CONVERSION_PYTHAGOREAN_TUNING: 12 tones with pythagorean tuning
 * @GSTBT_TONE_CONVERSION_COUNT: number of tunings
 *
 * Supported tuning types.
 * see http://en.wikipedia.org/wiki/Musical_tuning
 */
typedef enum {
  GSTBT_TONE_CONVERSION_EQUAL_TEMPERAMENT=0,
  GSTBT_TONE_CONVERSION_JUST_INTONATION,
  GSTBT_TONE_CONVERSION_PYTHAGOREAN_TUNING,
  /* TODO(ensonic): add more tunings
   * PYTHAGOREAN_TUNING
   * MEANTONE_TEMPERAMENT
   * WELL_TEMPERAMENT
   * SYNTONIC_TEMPERAMENT
   * has variants for the tempered perfect fifth (P5):
   * - EQUAL_TEMPERAMENT  P5 is 700 cents wide
   * - 1/4 COMMA_MEANTONE P5 is 696.6 cents wide
   * - PYTHAGOREAN_TUNING P5 is 702 cents wide
   * - 5-equal            P5 is 720 cents wide
   * - 7-equal            P5 is 686 cents wide
   *
   * diatonic-scale : only white keys (C-Dur, A-Moll)
   * chromatic-scale: all 12 semitones
   * major-scale    : Dur
   * minor-scale    : Moll
   *
   * there are also tonal systems using 19, 24 and 31 keys per octave
   */
   GSTBT_TONE_CONVERSION_COUNT
} GstBtToneConversionTuning;

#define GSTBT_TYPE_TONE_CONVERSION            (gstbt_tone_conversion_get_type ())
#define GSTBT_TONE_CONVERSION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSTBT_TYPE_TONE_CONVERSION, GstBtToneConversion))
#define GSTBT_TONE_CONVERSION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GSTBT_TYPE_TONE_CONVERSION, GstBtToneConversionClass))
#define GSTBT_IS_TONE_CONVERSION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSTBT_TYPE_TONE_CONVERSION))
#define GSTBT_IS_TONE_CONVERSION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GSTBT_TYPE_TONE_CONVERSION))
#define GSTBT_TONE_CONVERSION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GSTBT_TYPE_TONE_CONVERSION, GstBtToneConversionClass))

typedef struct _GstBtToneConversion GstBtToneConversion;
typedef struct _GstBtToneConversionClass GstBtToneConversionClass;

/**
 * GstBtToneConversion:
 *
 * Opaque object instance.
 */
struct _GstBtToneConversion {
  GObject parent;
  
  /*< private >*/
  GstBtToneConversionTuning tuning;
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  /* translate callback */
  gdouble (*translate)(GstBtToneConversion*, guint, guint);
};

struct _GstBtToneConversionClass {
  GObjectClass parent;
  
};

GType gstbt_tone_conversion_get_type(void) G_GNUC_CONST;
GType gstbt_tone_conversion_tuning_get_type(void) G_GNUC_CONST;

GstBtToneConversion *gstbt_tone_conversion_new(GstBtToneConversionTuning tuning);
gdouble gstbt_tone_conversion_translate_from_string(GstBtToneConversion *self,gchar *note);
gdouble gstbt_tone_conversion_translate_from_number(GstBtToneConversion *self,guint note);

guint gstbt_tone_conversion_note_string_2_number(const gchar *note);
const gchar *gstbt_tone_conversion_note_number_2_string(guint note);

guint gstbt_tone_conversion_note_number_offset(guint note, guint semitones);

G_END_DECLS

#endif /* __GSTBT_TONE_CONVERSION_H__ */
