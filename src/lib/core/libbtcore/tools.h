/* $Id: tools.h,v 1.9 2005-11-27 22:44:47 ensonic Exp $
 */

#ifndef BT_TOOLS_H
#define BT_TOOLS_H

#ifndef BT_TOOLS_C
extern GList *bt_gst_registry_get_element_names_by_class(gchar *class_filter);

extern void gst_element_dbg_pads(GstElement *elem);

#ifndef HAVE_GLIB_2_8
extern gpointer g_try_malloc0(gulong n_bytes);
#endif
#endif

#ifndef HAVE_GLIB_2_8
#define g_try_new(struct_type, n_structs) \
  g_try_malloc(sizeof(struct_type)*n_structs)

#define g_try_new0(struct_type, n_structs) \
  g_try_malloc0(sizeof(struct_type)*n_structs)
#endif

#endif // BT_TOOLS_H
