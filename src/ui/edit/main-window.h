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

#ifndef BT_MAIN_WINDOW_H
#define BT_MAIN_WINDOW_H

#include <glib.h>
#include <glib-object.h>
#include <adwaita.h>

G_DECLARE_FINAL_TYPE(BtMainWindow, bt_main_window, BT, MAIN_WINDOW, AdwWindow);

#define BT_TYPE_MAIN_WINDOW             (bt_main_window_get_type ())

/**
 * BtMainWindow:
 *
 * the root window for the editor application
 */
struct _BtMainWindowClass {
  GtkWindowClass parent;  
};

typedef void (* BtDialogQuestionCb) (gboolean response, gpointer userdata);

typedef struct {
  BtDialogQuestionCb cb;
  gpointer user_data;
} BtDialogQuestionCbData;


BtMainWindow *bt_main_window_new(void);

void bt_main_window_check_unsaved_song(BtMainWindow *self,const gchar *title,const gchar *headline,
    BtDialogQuestionCb result_cb, gpointer user_data);
void bt_main_window_check_quit(BtMainWindow *self, BtDialogQuestionCb result_cb, gpointer user_data);
void bt_main_window_new_song(BtMainWindow *self);
void bt_main_window_open_song(BtMainWindow *self);
void bt_main_window_save_song(BtMainWindow *self);
void bt_main_window_save_song_as(BtMainWindow *self);

/* helper for simple message/question dialogs */
void bt_dialog_message(BtMainWindow *self,const gchar *title,const gchar *headline,const gchar *message);

void bt_dialog_question(BtMainWindow * self, const gchar * title,
    const gchar * headline, const gchar * message,
    BtDialogQuestionCb result_cb, gpointer user_data);

#endif // BT_MAIN_WINDOW_H
