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

#ifndef BT_MAIN_WINDOW_METHODS_H
#define BT_MAIN_WINDOW_METHODS_H

#include "main-window.h"
#include "edit-application.h"

extern BtMainWindow *bt_main_window_new(const BtEditApplication *app);

extern gboolean bt_main_window_run(const BtMainWindow *self);

extern gboolean bt_main_window_check_quit(const BtMainWindow *self);
extern void bt_main_window_new_song(const BtMainWindow *self);
extern void bt_main_window_open_song(const BtMainWindow *self);
extern void bt_main_window_save_song(const BtMainWindow *self);
extern void bt_main_window_save_song_as(const BtMainWindow *self);

/* helper for simple message/question dialogs */
extern void bt_dialog_message(const BtMainWindow *self,const gchar *title,const gchar *headline,const gchar *message);
extern gboolean bt_dialog_question(const BtMainWindow *self,const gchar *title,const gchar *headline,const gchar *message);

#endif // BT_MAIN_WINDOW_METHDOS_H
