/* Buzztrax
 * Copyright (C) 2022 David Beswick <dlbeswick@gmail.com>
 *
 * lib/gst/ui.c: relating to UI functions of plugin machines.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/**
 * SECTION:ui
 * @title: GstBtUiCustomGfxInterface
 * @include: libtbuzztrax-gst/ui.h
 * @short_description: custom gfx in machine view
 *
 * Allows a machine to supply its own graphics to be drawn over the machine in
 * the Machine View.
 */

#include "ui.h"

G_DEFINE_INTERFACE(GstBtUiCustomGfx, gstbt_ui_custom_gfx, G_TYPE_OBJECT)

static void gstbt_ui_custom_gfx_default_init (GstBtUiCustomGfxInterface *iface) {
  /**
   * GstBtUiCustomGfx::gstbt-ui-custom-gfx-invalidated:
   *
   * Machines generally don't have to do anything in response to this signal, but
   * they should emit it when something happens that should change the image,
   * i.e. by the machine on a parameter change.
   */
  g_signal_new (
    "gstbt-ui-custom-gfx-invalidated",
    G_OBJECT_CLASS_TYPE(iface),
    G_SIGNAL_ACTION | G_SIGNAL_RUN_LAST,
    0 /* offset */,
    NULL /* accumulator */,
    NULL /* accumulator data */,
    NULL /* C marshaller */,
    G_TYPE_NONE /* return_type */,
    0     /* n_params */);
}

/**
 * gstbt_ui_custom_gfx_request:
 * @self: an object that implements custom graphics
 *
 * Request a graphics update.
 *
 * Returns: the graphics.
 */
const GstBtUiCustomGfxResponse *
gstbt_ui_custom_gfx_request (GstBtUiCustomGfx *self)
{
  GstBtUiCustomGfxInterface *iface;

  g_return_val_if_fail (GSTBT_UI_IS_CUSTOM_GFX (self), 0);

  iface = GSTBT_UI_CUSTOM_GFX_GET_IFACE (self);
  g_return_val_if_fail (iface->request != NULL, 0);
  return iface->request (self);
}
