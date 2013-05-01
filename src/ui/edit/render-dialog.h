/* Buzztrax
 * Copyright (C) 2007 Buzztrax team <buzztrax-devel@lists.sf.net>
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

#define BT_TYPE_RENDER_DIALOG            (bt_render_dialog_get_type ())
#define BT_RENDER_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_RENDER_DIALOG, BtRenderDialog))
#define BT_RENDER_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_RENDER_DIALOG, BtRenderDialogClass))
#define BT_IS_RENDER_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_RENDER_DIALOG))
#define BT_IS_RENDER_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_RENDER_DIALOG))
#define BT_RENDER_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_RENDER_DIALOG, BtRenderDialogClass))

/* type macros */

typedef struct _BtRenderDialog BtRenderDialog;
typedef struct _BtRenderDialogClass BtRenderDialogClass;
typedef struct _BtRenderDialogPrivate BtRenderDialogPrivate;

/**
 * BtRenderDialog:
 *
 * the song rendering dialog for the editor application
 */
struct _BtRenderDialog {
  GtkDialog parent;

  /*< private >*/
  BtRenderDialogPrivate *priv;
};

struct _BtRenderDialogClass {
  GtkDialogClass parent;

};

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


GType bt_render_dialog_get_type(void) G_GNUC_CONST;
GType bt_render_mode_get_type(void) G_GNUC_CONST;

BtRenderDialog *bt_render_dialog_new(void);

#endif // BT_RENDER_DIALOG_H
