/* $Id$
 *
 * Buzztard
 * Copyright (C) 2010 Buzztard team <buzztard-devel@lists.sf.net>
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

#ifndef BT_CHANGE_LOG_METHODS_H
#define BT_CHANGE_LOG_METHODS_H

#include <libbuzztard-core/core.h>
#include "change-log.h"
#include "change-logger.h"

extern BtChangeLog *bt_change_log_new(void);

extern GList *bt_change_log_crash_check(BtChangeLog *self);
extern gboolean bt_change_log_recover(BtChangeLog *self,const gchar *entry);

extern void bt_change_log_register(BtChangeLog *self,BtChangeLogger *logger);

extern void bt_change_log_add(BtChangeLog *self,BtChangeLogger *owner,gchar *undo_data,gchar *redo_data);
extern void bt_change_log_undo(BtChangeLog *self);
extern void bt_change_log_redo(BtChangeLog *self);

#endif // BT_CHANGE_LOG_METHDOS_H
