/*  $Id$
 *
 * Buzztard
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef BT_CHILD_PROXY_METHODS_H
#define BT_CHILD_PROXY_METHODS_H

#include "childproxy.h"

G_BEGIN_DECLS

extern GObject *bt_child_proxy_get_child_by_name(BtChildProxy *parent,const gchar *name);
extern GObject *bt_child_proxy_get_child_by_index(BtChildProxy *parent,guint index);
extern guint bt_child_proxy_get_children_count(BtChildProxy *parent);

extern gboolean bt_child_proxy_lookup(GObject *object,const gchar *name,GObject **target,GParamSpec **pspec);
extern void bt_child_proxy_get_property(GObject *object,const gchar *name,GValue *value);
extern void bt_child_proxy_get_valist(GObject *object,const gchar *first_property_name,va_list var_args);
extern void bt_child_proxy_get(gpointer object,const gchar *first_property_name,...) G_GNUC_NULL_TERMINATED;
extern void bt_child_proxy_set_property(GObject *object,const gchar *name,const GValue *value);
extern void bt_child_proxy_set_valist(GObject *object,const gchar *first_property_name,va_list var_args);
extern void bt_child_proxy_set(gpointer object,const gchar *first_property_name,...) G_GNUC_NULL_TERMINATED;

G_END_DECLS

#endif // BT_CHILD_PROXY_METHODS_H
