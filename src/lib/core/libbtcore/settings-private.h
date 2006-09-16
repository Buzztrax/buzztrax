/* $Id: settings-private.h,v 1.10 2006-09-16 16:28:13 ensonic Exp $
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

#ifndef BT_SETTINGS_PRIVATE_H
#define BT_SETTINGS_PRIVATE_H

#include "settings.h"

enum {
  BT_SETTINGS_AUDIOSINK=1,
  BT_SETTINGS_MENU_TOOLBAR_HIDE,
  BT_SETTINGS_MENU_TABS_HIDE,
  BT_SETTINGS_MACHINE_VIEW_GRID_DENSITY,
  BT_SETTINGS_NEWS_SEEN,
  /* @todo additional application settings
  */
  BT_SETTINGS_SYSTEM_AUDIOSINK,
  BT_SETTINGS_SYSTEM_TOOLBAR_STYLE
  /* @todo additional system settings
  BT_SETTINGS_SYSTEM_TOOLBAR_DETACHABLE <gboolean> gconf:gnome/interface/toolbar_detachable
  BT_SETTINGS_SYSTEM_TOOLBAR_ICON_SIZE  <guint>    gconf:gnome/interface/toolbar_icon_size
  BT_SETTINGS_SYSTEM_MENUBAR_DETACHABLE <gboolean> gconf:gnome/interface/menubar_detachable
  BT_SETTINGS_SYSTEM_MENU_HAVE_ICONS    <gboolean> gconf:gnome/interface/menus_have_icons
  BT_SETTINGS_SYSTEM_MENU_HAVE_TEAROFF  <gboolean> gconf:gnome/interface/menus_have_tearoff
  */
};

#endif // BT_SETTINGS_PRIVATE_H
