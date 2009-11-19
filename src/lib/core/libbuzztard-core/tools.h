/* $Id$
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

#include <gst/gst.h>

//-- registry

extern GList *bt_gst_registry_get_element_names_matching_all_categories(const gchar *class_filter);
extern gboolean bt_gst_element_factory_can_sink_media_type(GstElementFactory *factory,const gchar *name);

extern GList *bt_gst_check_elements(GList *list);
extern GList *bt_gst_check_core_elements(void);

//-- debugging

extern void bt_gst_element_dbg_pads(GstElement * const elem);

//-- gst compat

//-- glib compat & helper

extern GType bt_g_type_get_base_type(const GType type);

#endif // !BT_TOOLS_C

//-- glib compat & helper
/**
 * return_if_disposed:
 * @a: return value or nothing
 *
 * checks <code>self->priv->dispose_has_run</code> and
 * if true returns with the supplied arg.
 * This macro is handy to use at the start of all class routines
 * such as _get_property(), _set_property(), _dispose().
 */
#define return_if_disposed() if(self->priv->dispose_has_run) return

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

/*
GCC 4.1 introduced this crazy warning that complains about casting between
different pointer types. The question is why this includes void* ?
Sadly they don't gave tips how they belive to get rid of the warning.

#define bt_type_pun_to_gpointer(val) \
  (((union { gpointer __a; __typeof__((val)) __b; }){__b:(val)}).__a)
*/

/**
 * g_object_try_weak_ref:
 * @obj: the object to reference
 *
 * If the supplied object is not %NULL then reference it via
 * g_object_add_weak_pointer().
 */
#define g_object_try_weak_ref(obj) if(obj) g_object_add_weak_pointer(G_OBJECT(obj),(gpointer *)&obj##_ptr);
/*
#define g_object_try_weak_ref(obj) \
  if(obj) { \
    gpointer *ptr=&bt_type_pun_to_gpointer(obj); \
    GST_DEBUG("  reffing : %p",ptr); \
    g_object_add_weak_pointer(G_OBJECT(obj),ptr); \
  }
*/

/**
 * g_object_try_weak_unref:
 * @obj: the object to release the reference
 *
 * If the supplied object is not %NULL then release the reference via
 * g_object_remove_weak_pointer().
 */
#define g_object_try_weak_unref(obj) if(obj) g_object_remove_weak_pointer(G_OBJECT(obj),(gpointer *)&obj##_ptr);
/*
#define g_object_try_weak_unref(obj) \
  if(obj) { \
    gpointer *ptr=&bt_type_pun_to_gpointer(obj); \
    GST_DEBUG("  unreffing : %p",ptr); \
    g_object_remove_weak_pointer(G_OBJECT(obj),(gpointer *)&bt_type_pun_to_gpointer(obj)); \
  }
*/

/**
 * G_POINTER_ALIAS:
 * @type: the type
 * @var: the variable name
 *
 * Defines a anonymous union to handle gcc-4.1s type punning warning that one
 * gets when using e.g. g_object_try_weak_ref()
 */
#define G_POINTER_ALIAS(type,var) \
union { \
  type var; \
  gconstpointer var##_ptr; \
}

/*
@idea g_alloca_printf

#define g_alloca_printf(str,format,...) \
sprintf((str=alloca(g_printf_string_upper_bound(format, args)),format, args)
*/

// since glib 2.13.0
#ifndef G_PARAM_STATIC_STRINGS
#define	G_PARAM_STATIC_STRINGS (G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB)
#endif

//-- cpu load monitoring

extern guint bt_cpu_load_get_current(void);

#endif // !BT_TOOLS_H
