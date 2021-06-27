/* Buzztrax
 * Copyright (C) 2021 David Beswick <dlbeswick@gmail.com>
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

G_BEGIN_DECLS

// Used with the "bt-gfx-request" signal to display custom machine graphics.
typedef struct BtUiCustomGfx {
  guint version; // set to 0
  guint width;
  guint height;
  guint32* data;   // ABGR data, width*height guints
} BtUiCustomGfx;

G_END_DECLS

#endif
