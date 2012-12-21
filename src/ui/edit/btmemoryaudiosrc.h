/* Buzztard
 * Copyright (C) 2008 Buzztard team <buzztard-devel@lists.sf.net>
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

#ifndef __BT_MEMORY_AUDIO_SRC_H__
#define __BT_MEMORY_AUDIO_SRC_H__


#include <gst/gst.h>
#include <gst/base/gstbasesrc.h>
#include <gst/audio/audio.h>

G_BEGIN_DECLS


#define BT_TYPE_MEMORY_AUDIO_SRC \
  (bt_memory_audio_src_get_type())
#define BT_MEMORY_AUDIO_SRC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),BT_TYPE_MEMORY_AUDIO_SRC,BtMemoryAudioSrc))
#define BT_MEMORY_AUDIO_SRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),BT_TYPE_MEMORY_AUDIO_SRC,BtMemoryAudioSrcClass))
#define BT_IS_MEMORY_AUDIO_SRC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),BT_TYPE_MEMORY_AUDIO_SRC))
#define BT_IS_MEMORY_AUDIO_SRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),BT_TYPE_MEMORY_AUDIO_SRC))

typedef struct _BtMemoryAudioSrc BtMemoryAudioSrc;
typedef struct _BtMemoryAudioSrcClass BtMemoryAudioSrcClass;

/**
 * BtMemoryAudioSrc:
 *
 * audiotestsrc object structure.
 */
struct _BtMemoryAudioSrc {
  GstBaseSrc parent;

  /* parameters */
  GstCaps *caps;
  gint16 *data;
  gulong length;
  gint samples_per_buffer; /* block size ? */
    
  /* audio parameters */
  gint samplerate;
  gint channels;
  gint width;
  
  /*< private >*/
  GstClockTime running_time;            /* total running time */
  gint64 n_samples;                     /* total samples sent */
  gint64 n_samples_stop;
  gboolean check_seek_stop;
  gboolean eos_reached;
  gdouble rate;
  gint generate_samples_per_buffer;	/* used to generate a partial buffer */  
};

struct _BtMemoryAudioSrcClass {
  GstBaseSrcClass parent_class;
};

GType bt_memory_audio_src_get_type (void);

G_END_DECLS

#endif /* __BT_MEMORY_AUDIO_SRC_H__ */
