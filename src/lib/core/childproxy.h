/* Buzztard
 * Copyright (C) 2005 Buzztard team <buzztard-devel@lists.sf.net>
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

#ifndef BT_CHILD_PROXY_H
#define BT_CHILD_PROXY_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define BT_TYPE_CHILD_PROXY               (bt_child_proxy_get_type ())
#define BT_CHILD_PROXY(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_CHILD_PROXY, BtChildProxy))
#define BT_IS_CHILD_PROXY(obj)	          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_CHILD_PROXY))
#define BT_CHILD_PROXY_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BT_TYPE_CHILD_PROXY, BtChildProxyInterface))

/**
 * BtChildProxy:
 *
 * Opaque interface handle.
 */
typedef struct _BtChildProxy BtChildProxy;
typedef struct _BtChildProxyInterface BtChildProxyInterface;

/**
 * BtChildProxyInterface:
 * @get_child_by_name: virtual method to fetch the child by name
 * @get_child_by_index: virtual method to fetch the child by index
 * @get_children_count: virtual method to get the children count
 *
 * #BtChildProxy interface.
 */
struct _BtChildProxyInterface {
  /*< private >*/
  GTypeInterface parent;

  /*< public >*/
  /* virtual methods */
  GObject *(*get_child_by_name)(BtChildProxy *parent,const gchar *name);
  GObject *(*get_child_by_index) (BtChildProxy *parent,guint index);
  guint (*get_children_count) (BtChildProxy *parent);
};

GType bt_child_proxy_get_type(void) G_GNUC_CONST;

GObject *bt_child_proxy_get_child_by_name(BtChildProxy *parent,const gchar *name);
GObject *bt_child_proxy_get_child_by_index(BtChildProxy *parent,guint index);
guint bt_child_proxy_get_children_count(BtChildProxy *parent);

gboolean bt_child_proxy_lookup(GObject *object,const gchar *name,GObject **target,GParamSpec **pspec);
void bt_child_proxy_get_property(GObject *object,const gchar *name,GValue *value);
void bt_child_proxy_get_valist(GObject *object,const gchar *first_property_name,va_list var_args);
void bt_child_proxy_get(gpointer object,const gchar *first_property_name,...) G_GNUC_NULL_TERMINATED;
void bt_child_proxy_set_property(GObject *object,const gchar *name,const GValue *value);
void bt_child_proxy_set_valist(GObject *object,const gchar *first_property_name,va_list var_args);
void bt_child_proxy_set(gpointer object,const gchar *first_property_name,...) G_GNUC_NULL_TERMINATED;

G_END_DECLS

#endif // BT_CHILD_PROXY_H */
