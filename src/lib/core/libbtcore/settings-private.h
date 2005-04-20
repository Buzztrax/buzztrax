/* $Id: settings-private.h,v 1.4 2005-04-20 17:37:07 ensonic Exp $
 * defines all private methods of the settings class
 */

#ifndef BT_SETTINGS_PRIVATE_H
#define BT_SETTINGS_PRIVATE_H

#include "settings.h"

enum {
  BT_SETTINGS_AUDIOSINK=1,
	/* @todo additional application settings
	BT_SETTINGS_MENU_TOOLBAR_HIDE,
	BT_SETTINGS_MACHINE_VIEW_GRID_DENSITY,
	BT_SETTINGS_MACHINE_VIEW_ZOOM,
	*/
  BT_SETTINGS_SYSTEM_AUDIOSINK,
	BT_SETTINGS_SYSTEM_TOOLBAR_STYLE
	/* @todo additional system settings
	BT_SETTINGS_SYSTEM_TOOLBAR_SIZE	gconf:gnome/interface/toolbar_size
 	 */
};

#endif // BT_SETTINGS_PRIVATE_H
