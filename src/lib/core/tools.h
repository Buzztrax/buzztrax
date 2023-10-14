/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@buzztrax.org>
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

#ifndef BT_TOOLS_H
#define BT_TOOLS_H

#include <gst/gst.h>

//-- init/cleanup
// Typically called by bt_init/deinit
void bt_g_object_idle_add_init();
void bt_g_object_idle_add_cleanup();

//-- gst registry

GList *bt_gst_registry_get_element_factories_matching_all_categories(const gchar *class_filter);

GstPadTemplate * bt_gst_element_factory_get_pad_template(GstElementFactory *factory, const gchar *name);
gboolean bt_gst_element_factory_can_sink_media_type(GstElementFactory *factory,const gchar *name);

GList *bt_gst_check_elements(GList *list);
GList *bt_gst_check_core_elements(void);

gboolean bt_gst_try_element(GstElementFactory *factory, const gchar *format);

//-- gst safe linking

gboolean bt_bin_activate_tee_chain(GstBin *bin, GstElement *tee, GList* elements, gboolean is_playing);
gboolean bt_bin_deactivate_tee_chain(GstBin *bin, GstElement *tee, GList* elements, gboolean is_playing);

//-- gst debugging

const gchar *bt_gst_debug_pad_link_return(GstPadLinkReturn link_res,GstPad *src_pad,GstPad *sink_pad);

//-- gst element messages

gdouble bt_gst_level_message_get_aggregated_field(const GstStructure *structure, const gchar *field_name, gdouble default_value);
GstClockTime bt_gst_analyzer_get_waittime(GstElement *analyzer, const GstStructure *structure, gboolean endtime_is_running_time);

gboolean bt_gst_log_message_error(GstDebugCategory *cat, const gchar *file, const gchar *func, const int line, GstMessage *msg, gchar **err_msg, gchar ** err_desc);
gboolean bt_gst_log_message_warning(GstDebugCategory *cat, const gchar *file, const gchar *func, const int line, GstMessage *msg, gchar **warn_msg, gchar ** warn_desc);
/**
 * BT_GST_LOG_MESSAGE_ERROR:
 * @msg: a #GstMessage of type %GST_MESSAGE_ERROR
 * @err_msg: optional output variable for the error message
 * @err_desc: optional output variable for the error description
 *
 * Helper to logs the message to the debug log. If @description is
 * given, it is set to the error detail (free after use).
 *
 * Returns: %TRUE if the message could be parsed
 */
/**
 * BT_GST_LOG_MESSAGE_WARNING:
 * @msg: a #GstMessage of type %GST_MESSAGE_WARNING
 * @warn_msg: optional output variable for the error message
 * @warn_desc: optional output variable for the error description
 *
 * Helper to logs the message to the debug log. If @description is
 * given, it is set to the error detail (free after use).
 *
 * Returns: %TRUE if the message could be parsed
 */
#ifndef GST_DISABLE_GST_DEBUG
#define BT_GST_LOG_MESSAGE_ERROR(msg, err_msg, err_desc) \
  bt_gst_log_message_error(GST_CAT_DEFAULT,  __FILE__, GST_FUNCTION, __LINE__, msg, err_msg, err_desc)
#define BT_GST_LOG_MESSAGE_WARNING(msg, warn_msg, warn_desc) \
  bt_gst_log_message_warning(GST_CAT_DEFAULT,  __FILE__, GST_FUNCTION, __LINE__, msg, warn_msg, warn_desc)
#else
#define BT_GST_LOG_MESSAGE_ERROR(msg, err_msg, err_desc)
#define BT_GST_LOG_MESSAGE_WARNING(msg, warn_msg, warn_desc)
#endif
//-- gst compat

//-- glib compat & helper

GType bt_g_type_get_base_type(const GType type);

guint bt_g_object_idle_add(GObject *obj, gint pri, GSourceFunc func);

gulong bt_g_signal_connect_object(gpointer instance, const gchar * detailed_signal, GCallback c_handler, gpointer data, GConnectFlags connect_flags);

//-- cpu load monitoring

guint bt_cpu_load_get_current(void);

//-- glib compat & helper
/**
 * return_if_disposed:
 *
 * Checks <code>self->priv->dispose_has_run</code> and
 * if %TRUE returns.
 * This macro is handy to use at the start of all class routines
 * such as #GObjectClass.get_property(), #GObjectClass.set_property(),
 * #GObjectClass.dispose().
 */
#define return_if_disposed() if(self->priv->dispose_has_run) return
/**
 * return_val_if_disposed:
 * @a: return value
 *
 * Checks <code>self->priv->dispose_has_run</code> and
 * if %TRUE returns with the supplied arg @a.
 * This macro is handy to use at the start of e.g. idle handlers.
 */
#define return_val_if_disposed(a) if(self->priv->dispose_has_run) return(a)


/**
 * BT_IS_GVALUE:
 * @v: pointer to a GValue
 *
 * checks if the supplied gvalue is initialized (not all fields zero).
 */
#define BT_IS_GVALUE(v) (G_VALUE_TYPE(v)!=G_TYPE_INVALID)

/**
 * BT_IS_STRING:
 * @a: string pointer
 *
 * Checks if the supplied string pointer is not %NULL and contains not just '\0'
 */
#define BT_IS_STRING(a) (a && *a)

/**
 * safe_string:
 * @a: string pointer
 *
 * Pass the supplied string through or return an empty string when it is %NULL.
 *
 * Returns: the given string or an empty string in the case of a %NULL argument
 */
#define safe_string(a) ((gchar *)(a)?(gchar *)(a):"")

/*
@idea g_alloca_printf

#define g_alloca_printf(str,format,...) \
sprintf((str=alloca(g_printf_string_upper_bound(format, args)),format, args)
*/

//-- gobject ref-count debugging & helpers

/**
 * g_object_try_weak_ref:
 * @obj: the object to reference
 *
 * If the supplied object is not %NULL then reference it via
 * g_object_add_weak_pointer().
 */
#define g_object_try_weak_ref(obj) \
  if(obj) g_object_add_weak_pointer(G_OBJECT(obj),(gpointer *)&obj);

/**
 * g_object_try_weak_unref:
 * @obj: the object to release the reference
 *
 * If the supplied object is not %NULL then release the reference via
 * g_object_remove_weak_pointer().
 */
#define g_object_try_weak_unref(obj) \
  if(obj) g_object_remove_weak_pointer(G_OBJECT(obj),(gpointer *)&obj);

/**
 * G_OBJECT_REF_COUNT:
 * @obj: the object (may be %NULL)
 *
 * Read the objects reference counter. Implemented as a macro, so don't use
 * expressions for @obj.
 *
 * Returns: the reference counter.
 */
#define G_OBJECT_REF_COUNT(obj) ((obj)?((G_OBJECT(obj))->ref_count):0)

/**
 * G_OBJECT_REF_COUNT_FMT:
 *
 * Printf format string for #G_OBJECT_LOG_REF_COUNT.
 */
#define G_OBJECT_REF_COUNT_FMT "p,ref_ct=%d,floating=%d"

/**
 * G_OBJECT_LOG_REF_COUNT:
 * @o: the object (may be %NULL)
 *
 * Logs an object pointer together with its refcount value and the floating
 * flag. Use with #G_OBJECT_REF_COUNT_FMT.
 */
#define G_OBJECT_LOG_REF_COUNT(o) \
  (o), G_OBJECT_REF_COUNT ((o)), (o ? g_object_is_floating((gpointer)o) : 0)

/**
 * g_object_try_ref:
 * @obj: the object to reference
 *
 * If the supplied object is not %NULL then reference it via
 * g_object_ref().
 *
 * Returns: the referenced object or %NULL
 */
#define g_object_try_ref(obj) (obj)?g_object_ref(obj):NULL

/**
 * g_object_try_unref:
 * @obj: the object to release the reference
 *
 * If the supplied object is not %NULL then release the reference via
 * g_object_unref().
 */
#define g_object_try_unref(obj) if(obj) g_object_unref(obj)

// string formatting helper
const gchar *bt_str_format_uchar(const guchar val);
const gchar *bt_str_format_long(const glong val);
const gchar *bt_str_format_ulong(const gulong val);
const gchar *bt_str_format_double(const gdouble val);
const gchar *bt_str_format_enum(GType enum_type,gint value);

// string parsing helper
gint bt_str_parse_enum(GType enum_type,const gchar *str);

// gvalue helper
gboolean bt_str_parse_gvalue(GValue* const gvalue, const gchar * const svalue);
gchar *bt_str_format_gvalue(GValue * const gvalue);

#endif // !BT_TOOLS_H
