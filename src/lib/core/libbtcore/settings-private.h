/* $Id: settings-private.h,v 1.8 2006-08-13 20:24:33 ensonic Exp $
 * defines all private methods of the settings class
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
  BT_SETTINGS_SYSTEM_TOOLBAR_SIZE  gconf:gnome/interface/toolbar_size
    */
};

#endif // BT_SETTINGS_PRIVATE_H
