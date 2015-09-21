/* Buzztrax
 * Copyright (C) 2015 Stefan Sauer <ensonic@users.sf.net>
 *
 * combine.h: combine/mixing module
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

#ifndef __GSTBT_COMBINE_H__
#define __GSTBT_COMBINE_H__

#include <gst/gst.h>

G_BEGIN_DECLS

#define GSTBT_TYPE_COMBINE_TYPE (gstbt_combine_type_get_type())

/**
 * GstBtCombineType:
 * @GSTBT_COMBINE_MIX: add both signals (A+B)
 * @GSTBT_COMBINE_MUL: multiply signals (A*B) (amplitude modulation)
 * @GSTBT_COMBINE_SUB: subtract signals (A-B)
 * @GSTBT_COMBINE_MAX: max of both signals (max(A,B))
 * @GSTBT_COMBINE_MIN: min of both signals (min(A,B))
 * @GSTBT_COMBINE_AND: logical and of both signals (A&B)
 * @GSTBT_COMBINE_OR: logical or of both signals (A|B)
 * @GSTBT_COMBINE_XOR: logical xor of both signals (A^B)
 * @GSTBT_COMBINE_COUNT: number of combine modes, this can change with new
 * releases
 *
 * Combine types.
 */
typedef enum
{
  GSTBT_COMBINE_MIX,
  GSTBT_COMBINE_MUL,
  GSTBT_COMBINE_SUB,
  GSTBT_COMBINE_MAX,
  GSTBT_COMBINE_MIN,
  GSTBT_COMBINE_AND,
  GSTBT_COMBINE_OR,
  GSTBT_COMBINE_XOR,
  GSTBT_COMBINE_COUNT
} GstBtCombineType;

GType gstbt_combine_type_get_type(void);


#define GSTBT_TYPE_COMBINE            (gstbt_combine_get_type())
#define GSTBT_COMBINE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GSTBT_TYPE_COMBINE,GstBtCombine))
#define GSTBT_IS_COMBINE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GSTBT_TYPE_COMBINE))
#define GSTBT_COMBINE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass) ,GSTBT_TYPE_COMBINE,GstBtCombineClass))
#define GSTBT_IS_COMBINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass) ,GSTBT_TYPE_COMBINE))
#define GSTBT_COMBINE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj) ,GSTBT_TYPE_COMBINE,GstBtCombineClass))

typedef struct _GstBtCombine GstBtCombine;
typedef struct _GstBtCombineClass GstBtCombineClass;

/**
 * GstBtCombine:
 * @type: combine type
 *
 * Class instance data.
 */
struct _GstBtCombine {
  GstObject parent;
  /* < private > */
  gboolean dispose_has_run;		/* validate if dispose has run */

  /* < public > */
  /* parameters */
  GstBtCombineType type;

  /* < private > */
  guint64 offset;

  /* < private > */
  void (*process) (GstBtCombine *, guint, gint16 *, gint16 *);
};

struct _GstBtCombineClass {
  GstObjectClass parent_class;
};

GType gstbt_combine_get_type(void);

GstBtCombine *gstbt_combine_new(void);

void gstbt_combine_trigger(GstBtCombine *self);
void gstbt_combine_process(GstBtCombine *self, guint size, gint16 *d1, gint16 *d2);

G_END_DECLS
#endif /* __GSTBT_COMBINE_H__ */
