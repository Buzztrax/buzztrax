/* $Id: settings-private.h,v 1.5 2005-04-21 16:13:28 ensonic Exp $
 * defines all private methods of the settings class
 */

#ifndef BT_SETTINGS_PRIVATE_H
#define BT_SETTINGS_PRIVATE_H

#include "settings.h"

enum {
  BT_SETTINGS_AUDIOSINK=1,
	BT_SETTINGS_MENU_TOOLBAR_HIDE,
	BT_SETTINGS_MACHINE_VIEW_GRID_DENSITY,
	/* @todo additional application settings
	*/
  BT_SETTINGS_SYSTEM_AUDIOSINK,
	BT_SETTINGS_SYSTEM_TOOLBAR_STYLE
	/* @todo additional system settings
	BT_SETTINGS_SYSTEM_TOOLBAR_SIZE	gconf:gnome/interface/toolbar_size
 	 */
};

#endif // BT_SETTINGS_PRIVATE_H
