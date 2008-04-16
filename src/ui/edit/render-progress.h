/* $Id$
 *
 * Buzztard
 * Copyright (C) 2007 Buzztard team <buzztard-devel@lists.sf.net>
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

#ifndef BT_RENDER_PROGRESS_H
#define BT_RENDER_PROGRESS_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_RENDER_PROGRESS            (bt_render_progress_get_type ())
#define BT_RENDER_PROGRESS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_RENDER_PROGRESS, BtRenderProgress))
#define BT_RENDER_PROGRESS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_RENDER_PROGRESS, BtRenderProgressClass))
#define BT_IS_RENDER_PROGRESS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_RENDER_PROGRESS))
#define BT_IS_RENDER_PROGRESS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_RENDER_PROGRESS))
#define BT_RENDER_PROGRESS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_RENDER_PROGRESS, BtRenderProgressClass))

/* type macros */

typedef struct _BtRenderProgress BtRenderProgress;
typedef struct _BtRenderProgressClass BtRenderProgressClass;
typedef struct _BtRenderProgressPrivate BtRenderProgressPrivate;

/**
 * BtRenderProgress:
 *
 * the song rendering progress for the editor application
 */
struct _BtRenderProgress {
  GtkDialog parent;

  /*< private >*/
  BtRenderProgressPrivate *priv;
};
/* structure of the render-progress class */
struct _BtRenderProgressClass {
  GtkDialogClass parent;

};

/* used by RENDER_PROGRESS_TYPE */
GType bt_render_progress_get_type(void) G_GNUC_CONST;

#endif // BT_RENDER_PROGRESS_H
