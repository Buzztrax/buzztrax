/* $Id: tools.h,v 1.7 2005-03-12 12:11:42 ensonic Exp $
 */

#ifndef BT_TOOLS_H
#define BT_TOOLS_H

#ifndef BT_TOOLS_C
extern GList *bt_gst_registry_get_element_names_by_class(gchar *class_filter);

extern gpointer g_try_malloc0(gulong n_bytes);
#endif

#define g_try_new(struct_type, n_structs) \
  g_try_malloc(sizeof(struct_type)*n_structs)

#define g_try_new0(struct_type, n_structs) \
  g_try_malloc0(sizeof(struct_type)*n_structs)

#endif // BT_TOOLS_H
