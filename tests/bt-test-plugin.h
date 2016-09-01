/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@buzztrax.org>
 *
 * gstreamer test plugin for unit tests
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

#ifndef BT_TEST_PLUGIN_H
#define BT_TEST_PLUGIN_H

#include <glib.h>
#include <glib-object.h>
#include <gst/gst.h>

//-- test_sparse_enum

typedef enum
{
  BT_TEST_SPARSE_ENUM_ZERO = 0,
  BT_TEST_SPARSE_ENUM_ONE = 1,
  BT_TEST_SPARSE_ENUM_TEN = 0xa,
  BT_TEST_SPARSE_ENUM_TWENTY = 0x14,
  BT_TEST_SPARSE_ENUM_TWENTY_ONE = 0x15,
  BT_TEST_SPARSE_ENUM_TWO_HUNDRED = 0xc8,
} BtTestSparseEnum;

//-- test_no_arg_mono_source

#define BT_TYPE_TEST_NO_ARG_MONO_SOURCE            (bt_test_no_arg_mono_source_get_type ())
#define BT_TEST_NO_ARG_MONO_SOURCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_TEST_NO_ARG_MONO_SOURCE, BtTestNoArgMonoSource))
#define BT_TEST_NO_ARG_MONO_SOURCE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_TEST_NO_ARG_MONO_SOURCE, BtTestNoArgMonoSourceClass))
#define BT_IS_TEST_NO_ARG_MONO_SOURCE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_TEST_NO_ARG_MONO_SOURCE))
#define BT_IS_TEST_NO_ARG_MONO_SOURCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_TEST_NO_ARG_MONO_SOURCE))
#define BT_TEST_NO_ARG_MONO_SOURCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_TEST_NO_ARG_MONO_SOURCE, BtTestNoArgMonoSourceClass))

/* type macros */

typedef struct _BtTestNoArgMonoSource BtTestNoArgMonoSource;
typedef struct _BtTestNoArgMonoSourceClass BtTestNoArgMonoSourceClass;

/* monophonic source element */
struct _BtTestNoArgMonoSource {
  GstElement parent;
};
/* structure of the test_mono_source class */
struct _BtTestNoArgMonoSourceClass {
  GstElementClass parent_class;
};

GType bt_test_no_arg_mono_source_get_type(void);

//-- test_mono_source

#define BT_TYPE_TEST_MONO_SOURCE            (bt_test_mono_source_get_type ())
#define BT_TEST_MONO_SOURCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_TEST_MONO_SOURCE, BtTestMonoSource))
#define BT_TEST_MONO_SOURCE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_TEST_MONO_SOURCE, BtTestMonoSourceClass))
#define BT_IS_TEST_MONO_SOURCE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_TEST_MONO_SOURCE))
#define BT_IS_TEST_MONO_SOURCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_TEST_MONO_SOURCE))
#define BT_TEST_MONO_SOURCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_TEST_MONO_SOURCE, BtTestMonoSourceClass))

/* type macros */

typedef struct _BtTestMonoSource BtTestMonoSource;
typedef struct _BtTestMonoSourceClass BtTestMonoSourceClass;

/* monophonic source element */
struct _BtTestMonoSource {
  GstElement parent;

  // test properties
  gulong ulong_val;
  gdouble double_val;
  gboolean switch_val;
  GstBtNote note_val;
  BtTestSparseEnum sparse_enum_val;
  gint wave_val;
  gint int_val;

  // audio tempo context
  guint bpm, tpb, stpb;
};
/* structure of the test_mono_source class */
struct _BtTestMonoSourceClass {
  GstElementClass parent_class;
};

GType bt_test_mono_source_get_type(void);

//-- test_poly_source

#define BT_TYPE_TEST_POLY_SOURCE            (bt_test_poly_source_get_type ())
#define BT_TEST_POLY_SOURCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_TEST_POLY_SOURCE, BtTestPolySource))
#define BT_TEST_POLY_SOURCE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_TEST_POLY_SOURCE, BtTestPolySourceClass))
#define BT_IS_TEST_POLY_SOURCE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_TEST_POLY_SOURCE))
#define BT_IS_TEST_POLY_SOURCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_TEST_POLY_SOURCE))
#define BT_TEST_POLY_SOURCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_TEST_POLY_SOURCE, BtTestPolySourceClass))

/* type macros */

typedef struct _BtTestPolySource BtTestPolySource;
typedef struct _BtTestPolySourceClass BtTestPolySourceClass;

/* polyphonic source element */
struct _BtTestPolySource {
  GstElement parent;

  // test properties
  gulong ulong_val;
  gdouble double_val;
  gboolean switch_val;
  GstBtNote note_val;
  BtTestSparseEnum sparse_enum_val;

  // audio tempo context
  guint bpm, tpb, stpb;

  // the number of voices the plugin has
  gulong num_voices;
  GList *voices;
};
/* structure of the test_poly_source class */
struct _BtTestPolySourceClass {
  GstElementClass parent_class;
};

GType bt_test_poly_source_get_type(void);

//-- test_mono_processor

//-- test_poly_processor

#endif /* BT_TEST_PLUGIN_H */
