/* $Id: tools.h,v 1.12 2006-08-10 20:02:31 ensonic Exp $
 */

#ifndef BT_TOOLS_H
#define BT_TOOLS_H

#ifndef BT_TOOLS_C
extern GList *bt_gst_registry_get_element_names_by_class(gchar *class_filter);

extern void gst_element_dbg_pads(GstElement *elem);

#ifndef HAVE_GLIB_2_8
extern gpointer g_try_malloc0(gulong n_bytes);
#endif

#ifndef HAVE_GLIB_2_8
#define g_try_new(struct_type, n_structs) \
  g_try_malloc(sizeof(struct_type)*n_structs)

#define g_try_new0(struct_type, n_structs) \
  g_try_malloc0(sizeof(struct_type)*n_structs)
#endif

extern GType bt_g_type_get_base_type(GType type);

#endif

extern void bt_log_mark(const char *format, ...);
#ifdef APP_DEBUG
#define BT_LOG_MARK_FUNCTION(str) bt_log_mark("%s : %s",__FUNCTION__,str);
#else
#define BT_LOG_MARK_FUNCTION(str)
#endif

extern guint bt_cpu_load_get_current(void);

#endif // BT_TOOLS_H
