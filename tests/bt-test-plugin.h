/* $Id$
 *
 * Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef BT_TEST_PLUGIN_H
#define BT_TEST_PLUGIN_H

#include <glib.h>
#include <glib-object.h>

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
};
/* structure of the test_mono_source class */
struct _BtTestMonoSourceClass {
  GstElementClass parent_class;
};

/* used by TEST_MONO_SOURCE_TYPE */
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

  // the number of voices the plugin has
  gulong num_voices;
  GList *voices;
};
/* structure of the test_poly_source class */
struct _BtTestPolySourceClass {
  GstElementClass parent_class;
};

/* used by TEST_POLY_SOURCE_TYPE */
GType bt_test_poly_source_get_type(void);

//-- test_mono_processor

//-- test_poly_processor

#endif /* BT_TEST_PLUGIN_H */
