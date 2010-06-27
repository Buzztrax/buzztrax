/* $Id$
 *
 * Buzztard
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef BT_BIN_H__
#define BT_BIN_H__

#include <gst/gst.h>
#include <gst/base/gstadapter.h>
#include <libbuzztard-core/core.h>

G_BEGIN_DECLS


#define BT_TYPE_BIN            (bt_bin_get_type())
#define BT_BIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),BT_TYPE_BIN,BtBin))
#define BT_IS_BIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),BT_TYPE_BIN))
#define BT_BIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass) ,BT_TYPE_BIN,BtBinClass))
#define BT_IS_BIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass) ,BT_TYPE_BIN))
#define BT_BIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj) ,BT_TYPE_BIN,BtBinClass))

typedef struct _BtBin BtBin;
typedef struct _BtBinClass BtBinClass;

/**
 * BtBin:
 *
 * Class instance data.
 */
struct _BtBin {
  GstBin parent;
  
  GstPad *sinkpad, *srcpad;

  /* raw song data */
  GstAdapter *adapter;
  guint64 offset;

  BtApplication *app;
  BtSong *song;
  GstBin *bin;
};

struct _BtBinClass {
  GstBinClass parent_class;
};

GType bt_bin_get_type(void);

G_END_DECLS

#endif /*BT_BIN_H__ */