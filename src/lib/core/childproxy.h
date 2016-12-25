/* Buzztrax
 * Copyright (C) 2005 Buzztrax team <buzztrax-devel@buzztrax.org>
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

void bt_child_proxy_get_property(GObject *object,const gchar *name,GValue *value);
void bt_child_proxy_get_valist(GObject *object,const gchar *first_property_name,va_list var_args);
void bt_child_proxy_get(gpointer object,const gchar *first_property_name,...) G_GNUC_NULL_TERMINATED;
void bt_child_proxy_set_property(GObject *object,const gchar *name,const GValue *value);
void bt_child_proxy_set_valist(GObject *object,const gchar *first_property_name,va_list var_args);
void bt_child_proxy_set(gpointer object,const gchar *first_property_name,...) G_GNUC_NULL_TERMINATED;

G_END_DECLS

#endif // BT_CHILD_PROXY_H */
