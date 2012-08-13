/* Buzztard
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

#include <gst/gst.h>

//-- gst registry

GList *bt_gst_registry_get_element_factories_matching_all_categories(const gchar *class_filter);
GList *bt_gst_registry_get_element_names_matching_all_categories(const gchar *class_filter);
gboolean bt_gst_element_factory_can_sink_media_type(GstElementFactory *factory,const gchar *name);

GList *bt_gst_check_elements(GList *list);
GList *bt_gst_check_core_elements(void);

//-- gst safe linking

gboolean bt_bin_activate_tee_chain(GstBin *bin, GstElement *tee, GList* analyzers, gboolean is_playing);
gboolean bt_bin_deactivate_tee_chain(GstBin *bin, GstElement *tee, GList* analyzers, gboolean is_playing);

//-- gst debugging

const gchar *bt_gst_debug_pad_link_return(GstPadLinkReturn link_res,GstPad *src_pad,GstPad *sink_pad);

//-- gst element messages

gdouble bt_gst_level_message_get_aggregated_field(const GstStructure *structure, const gchar *field_name, gdouble default_value);

//-- gst compat

//-- glib compat & helper

GType bt_g_type_get_base_type(const GType type);

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
 * checks if the supplied string pointer is not %NULL and contains not just '\0'
 */
#define BT_IS_STRING(a) (a && *a)

/**
 * safe_string:
 * @a: string pointer
 *
 * passed the supplied string through or return an empty string when it is NULL
 *
 * Returns: the given string or an empty string in the case of a NULL argument
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
#define g_object_try_weak_ref(obj) if(obj) g_object_add_weak_pointer(G_OBJECT(obj),(gpointer *)&obj##_ptr);

/**
 * g_object_try_weak_unref:
 * @obj: the object to release the reference
 *
 * If the supplied object is not %NULL then release the reference via
 * g_object_remove_weak_pointer().
 */
#define g_object_try_weak_unref(obj) if(obj) g_object_remove_weak_pointer(G_OBJECT(obj),(gpointer *)&obj##_ptr);

/**
 * G_POINTER_ALIAS:
 * @type: the type
 * @var: the variable name
 *
 * Defines a anonymous union to handle gcc-4.1s type punning warning that one
 * gets when using e.g. g_object_try_weak_ref(). Since glib 2.14 api can also
 * be annotated with G_GNUC_MAY_ALIAS to avoid the union.
 *
 * Also using a union is technically wrong. It is not allowed to write to one
 * field and read from the other (or vice versa).
 */
#define G_POINTER_ALIAS(type,var) \
union { \
  type var; \
  gconstpointer var##_ptr; \
}

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

#endif // !BT_TOOLS_H
