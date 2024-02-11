/* Buzztrax
 * Copyright (C) 2022 David Beswick <dlbeswick@gmail.com>
 *
 * lib/gst/ui.h: relating to UI functions of plugin machines.
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

#ifndef __LIB_GST_UI_H__
#define __LIB_GST_UI_H__

#include <glib-object.h>

G_BEGIN_DECLS

/**
 * GstBtUiCustomGfxResponse:
 * @version: set to 0
 * @width: width of returned image in pixels
 * @height: height of returned image in pixels
 * @data: ABGR data, width*height guints
 *
 * Data returned by GstBtUiCustomGfxInterface::request
 */
typedef struct GstBtUiCustomGfxResponse {
  guint version; // set to 0
  guint width;
  guint height;
  guint32* data;   // ABGR data, width*height guints
} GstBtUiCustomGfxResponse;


#define GSTBT_UI_TYPE_CUSTOM_GFX gstbt_ui_custom_gfx_get_type ()
G_DECLARE_INTERFACE (GstBtUiCustomGfx, gstbt_ui_custom_gfx, GSTBT_UI, CUSTOM_GFX,
                     GObject)

/**
 * GstBtUiCustomGfx:
 *
 * Opaque interface handle.
 */
/**
 * GstBtUiCustomGfxInterface:
 * @parent: parent type
 * @request: Return the machine's current custom gfx image.
 *           Ownership of the returned object isn't transferred, so cached data
 *           can be used (both response structure and image data.)
 *
 *           The actual rendering of the image may be done here.
 *
 *           May return NULL; in that case, Buzztrax's standard gfx are used.
 *
 * Interface structure.
 */
struct _GstBtUiCustomGfxInterface {
  GTypeInterface parent;

  const GstBtUiCustomGfxResponse *(*request) (GstBtUiCustomGfx *self);
};

const GstBtUiCustomGfxResponse *gstbt_ui_custom_gfx_request (GstBtUiCustomGfx *self);

G_END_DECLS

#endif
