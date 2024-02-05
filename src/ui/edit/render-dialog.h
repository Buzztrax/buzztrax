/* Buzztrax
 * Copyright (C) 2007 Buzztrax team <buzztrax-devel@buzztrax.org>
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

#ifndef BT_RENDER_DIALOG_H
#define BT_RENDER_DIALOG_H

#include <glib.h>
#include <glib-object.h>


/**
 * BtRenderDialog:
 *
 * the song rendering dialog for the editor application
 */
G_DECLARE_FINAL_TYPE(BtRenderDialog, bt_render_dialog, BT, RENDER_DIALOG, AdwWindow);

#define BT_TYPE_RENDER_DIALOG            (bt_render_dialog_get_type ())

#define BT_TYPE_RENDER_MODE       (bt_render_mode_get_type())

/**
 * BtRenderMode:
 * @BT_RENDER_MODE_MIXDOWN: mix to one track
 * @BT_RENDER_MODE_SINGLE_TRACKS: record one track for each source
 *
 * Different modes of operation for the #BtSong rendering.
 */
typedef enum {
  BT_RENDER_MODE_MIXDOWN=0,
  BT_RENDER_MODE_SINGLE_TRACKS,
} BtRenderMode;


GType bt_render_mode_get_type(void) G_GNUC_CONST;

BtRenderDialog *bt_render_dialog_new(void);

#endif // BT_RENDER_DIALOG_H
