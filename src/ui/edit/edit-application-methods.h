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

#ifndef BT_EDIT_APPLICATION_METHODS_H
#define BT_EDIT_APPLICATION_METHODS_H

#include "edit-application.h"

extern BtEditApplication *bt_edit_application_new(void);

extern gboolean bt_edit_application_new_song(const BtEditApplication *self);
extern gboolean bt_edit_application_load_song(const BtEditApplication *self,const char *file_name);
extern gboolean bt_edit_application_save_song(const BtEditApplication *self,const char *file_name);

extern gboolean bt_edit_application_run(const BtEditApplication *self);
extern gboolean bt_edit_application_load_and_run(const BtEditApplication *self, const gchar *input_file_name);
extern gboolean bt_edit_application_quit(const BtEditApplication *self);

extern void bt_edit_application_show_about(const BtEditApplication *self);
extern void bt_edit_application_show_tip(const BtEditApplication *self);

extern void bt_edit_application_crash_log_recover(const BtEditApplication *self);

extern void bt_edit_application_attach_child_window(const BtEditApplication *self, GtkWindow *window);

extern void bt_edit_application_ui_lock(const BtEditApplication *self);
extern void bt_edit_application_ui_unlock(const BtEditApplication *self);

#endif // BT_EDIT_APPLICATION_METHDOS_H
