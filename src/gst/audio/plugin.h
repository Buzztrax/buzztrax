/* GStreamer
 * Copyright (C) 2015 Stefan Sauer <ensonic@users.sf.net>
 *
 * plugin.h: common code
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

#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#include <glib-object.h>

#define bt_g_param_spec_override_range(type, ps, mi, ma, def) do { \
  ((type *)(ps))->minimum = mi; \
  ((type *)(ps))->maximum = ma; \
  ((type *)(ps))->default_value = def; \
} while (0)

G_BEGIN_DECLS

GParamSpec * bt_g_param_spec_clone (GObjectClass * src_class, const gchar * src_name);
GParamSpec * bt_g_param_spec_clone_as (GObjectClass * src_class, const gchar * src_name, gchar * new_name);

G_END_DECLS
#endif /* __PLUGIN_H__ */
