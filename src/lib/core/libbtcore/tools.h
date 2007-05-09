/* $Id: tools.h,v 1.20 2007-05-09 12:50:28 ensonic Exp $
 *
 * Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
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

#ifndef BT_TOOLS_H
#define BT_TOOLS_H

#ifndef BT_TOOLS_C

//-- registry

extern GList *bt_gst_registry_get_element_names_matching_all_categories(const gchar *class_filter);
extern gboolean bt_gst_element_factory_can_sink_media_type(GstElementFactory *factory,const gchar *name);

extern GList *bt_gst_check_elements(GList *list);
extern GList *bt_gst_check_core_elements(void);

//-- debugging
extern void bt_gst_element_dbg_pads(GstElement * const elem);

//-- glib compat & helper
#ifndef HAVE_GLIB_2_8
extern gpointer g_try_malloc0(const gulong n_bytes);

#define g_try_new(struct_type, n_structs) \
  g_try_malloc(sizeof(struct_type)*n_structs)

#define g_try_new0(struct_type, n_structs) \
  g_try_malloc0(sizeof(struct_type)*n_structs)
#endif // !HAVE_GLIB_2_8

extern GType bt_g_type_get_base_type(const GType type);

//-- cpu load monitoring

extern guint bt_cpu_load_get_current(void);

#endif // !BT_TOOLS_C

#endif // !BT_TOOLS_H
