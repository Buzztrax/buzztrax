/* Buzztard
 * Copyright (C) 2012 Buzztard team <buzztard-devel@lists.sf.net>
 *
 * ui testing helpers
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

#ifndef BT_CHECK_UI_H
#define BT_CHECK_UI_H

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

//-- glib/gobject
#include <glib.h>
//-- gtk/gdk
#include <gtk/gtk.h>


void check_setup_test_server(void);
void check_setup_test_display(void);
void check_shutdown_test_display(void);
void check_shutdown_test_server(void);

enum _BtCheckWidgetScreenshotRegionsMatch {
  BT_CHECK_WIDGET_SCREENSHOT_REGION_MATCH_NONE = 0,
  BT_CHECK_WIDGET_SCREENSHOT_REGION_MATCH_TYPE = (1<<0),
  BT_CHECK_WIDGET_SCREENSHOT_REGION_MATCH_NAME = (1<<1),
  BT_CHECK_WIDGET_SCREENSHOT_REGION_MATCH_LABEL = (1<<2),
};

typedef enum _BtCheckWidgetScreenshotRegionsMatch BtCheckWidgetScreenshotRegionsMatch;

struct _BtCheckWidgetScreenshotRegions {
  BtCheckWidgetScreenshotRegionsMatch match;
  gchar *name;
  gchar *label;
  GType type;
  GtkPositionType pos;
};
typedef struct _BtCheckWidgetScreenshotRegions BtCheckWidgetScreenshotRegions;

void check_make_widget_screenshot(GtkWidget *widget, const gchar *name);
void check_make_widget_screenshot_with_highlight(GtkWidget *widget, const gchar *name, BtCheckWidgetScreenshotRegions *regions);

void check_send_key(GtkWidget *widget, guint keyval, guint16 hardware_keycode);
void check_send_click(GtkWidget *widget, guint button, gdouble x, gdouble y);

#endif /* BT_CHECK_UI_H */
