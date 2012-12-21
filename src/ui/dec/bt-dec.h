/* Buzztard
 * Copyright (C) 2010 Buzztard team <buzztard-devel@lists.sf.net>
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

#ifndef BT_DEC_H__
#define BT_DEC_H__

#include <gst/gst.h>
#include <gst/base/gstadapter.h>
#include <gst/audio/audio.h>
#include "core.h"

G_BEGIN_DECLS


#define BT_TYPE_DEC            (bt_dec_get_type())
#define BT_DEC(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),BT_TYPE_DEC,BtDec))
#define BT_IS_BIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),BT_TYPE_DEC))
#define BT_DEC_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass) ,BT_TYPE_DEC,BtDecClass))
#define BT_IS_BIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass) ,BT_TYPE_DEC))
#define BT_DEC_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj) ,BT_TYPE_DEC,BtDecClass))

typedef struct _BtDec BtDec;
typedef struct _BtDecClass BtDecClass;

/**
 * BtDec:
 *
 * Class instance data.
 */
struct _BtDec {
  GstElement parent;
  
  GstPad *sinkpad, *srcpad, *binpad;

  /* raw song data */
  GstAdapter *adapter;
  guint64 offset;

  BtApplication *app;
  BtSong *song;
  GstBin *bin;
  
  GstEvent *newsegment_event;
  GstSegment segment;
};

struct _BtDecClass {
  GstElementClass parent_class;
};

GType bt_dec_get_type(void);

G_END_DECLS

#endif /*BT_DEC_H__ */