/* GStreamer
 * Copyright (C) 2014 Stefan Sauer <ensonic@users.sf.net>
 *
 * delay.h: delay line object
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

#ifndef __GSTBT_DELAY_H__
#define __GSTBT_DELAY_H__

#include <gst/gst.h>

G_BEGIN_DECLS

#define GSTBT_TYPE_DELAY            (gstbt_delay_get_type())
#define GSTBT_DELAY(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GSTBT_TYPE_DELAY,GstBtDelay))
#define GSTBT_IS_DELAY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GSTBT_TYPE_DELAY))
#define GSTBT_DELAY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass) ,GSTBT_TYPE_DELAY,GstBtDelayClass))
#define GSTBT_IS_DELAY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass) ,GSTBT_TYPE_DELAY))
#define GSTBT_DELAY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj) ,GSTBT_TYPE_DELAY,GstBtDelayClass))

typedef struct _GstBtDelay GstBtDelay;
typedef struct _GstBtDelayClass GstBtDelayClass;

/**
 * GstBtDelay:
 *
 * Class instance data.
 */
struct _GstBtDelay {
  GObject parent;
  /* < private > */
  
  /* properties */
  guint delaytime;

  gint samplerate;
  gint16 *ring_buffer;
  guint max_delaytime;
  guint rb_ptr;
};

struct _GstBtDelayClass {
  GObjectClass parent_class;
};

GType gstbt_delay_get_type (void);

GstBtDelay *gstbt_delay_new (void);

void gstbt_delay_start (GstBtDelay *self, gint samplerate);
void gstbt_delay_flush (GstBtDelay *self);
void gstbt_delay_stop (GstBtDelay *self);

/**
 * GSTBT_DELAY_BEFORE:
 * @self: the delay
 * @rb_in: the write position of the ring-buffer
 * @rb_out: the read position of the ring-buffer
 *
 * Initialize read/write pointers.
 */
#define GSTBT_DELAY_BEFORE(self,rb_in,rb_out) G_STMT_START {    \
  guint delaytime = (self->delaytime * self->samplerate) / 100; \
  rb_in = self->rb_ptr;                                         \
  rb_out = (rb_in >= delaytime)                                 \
      ? rb_in - delaytime                                       \
      : (rb_in + self->max_delaytime) - delaytime;              \
} G_STMT_END

/**
 * GSTBT_DELAY_AFTER:
 * @self: the delay
 * @rb_in: the write position of the ring-buffer
 * @rb_out: the read position of the ring-buffer
 *
 * Store read/write pointers.
 */
#define GSTBT_DELAY_AFTER(self,rb_in,rb_out) G_STMT_START { \
  self->rb_ptr = rb_in;                                     \
} G_STMT_END

/**
 * GSTBT_DELAY_READ:
 * @self: the delay
 * @rb_out: the read position of the ring-buffer
 * @v: the value from the ring-buffer
 *
 * Read from ring-buffer and advance the position.
 */
#define GSTBT_DELAY_READ(self,rb_out,v) G_STMT_START { \
  v = self->ring_buffer[rb_out++];                     \
  if (rb_out == self->max_delaytime) rb_out = 0;       \
} G_STMT_END

/**
 * GSTBT_DELAY_WRITE:
 * @self: the delay
 * @rb_in: the write position of the ring-buffer
 * @v: the value to write to the ring-buffer
 *
 * Write to @v the ring-buffer and advance the position.
 */
#define GSTBT_DELAY_WRITE(self,rb_in,v) G_STMT_START { \
  self->ring_buffer[rb_in++] = (gint16) v;             \
  if (rb_in == self->max_delaytime) rb_in = 0;         \
} G_STMT_END

G_END_DECLS

#endif /* __GSTBT_DELAY_H__ */
