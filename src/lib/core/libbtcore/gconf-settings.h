/* $Id: gconf-settings.h,v 1.1 2004-09-26 01:50:08 ensonic Exp $
 * gconf based implementation sub class for buzztard settings handling
 */

#ifndef BT_GCONF_SETTINGS_H
#define BT_GCONF_SETTINGS_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_GCONF_SETTINGS		        (bt_gconf_settings_get_type ())
#define BT_GCONF_SETTINGS(obj)		        (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_GCONF_SETTINGS, BtGConfSettings))
#define BT_GCONF_SETTINGS_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_GCONF_SETTINGS, BtGConfSettingsClass))
#define BT_IS_GCONF_SETTINGS(obj)	        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_GCONF_SETTINGS))
#define BT_IS_GCONF_SETTINGS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_GCONF_SETTINGS))
#define BT_GCONF_SETTINGS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_GCONF_SETTINGS, BtGConfSettingsClass))

/* type macros */

typedef struct _BtGConfSettings BtGConfSettings;
typedef struct _BtGConfSettingsClass BtGConfSettingsClass;
typedef struct _BtGConfSettingsPrivate BtGConfSettingsPrivate;

/**
 * BtGConfSettings:
 *
 * gconf based implementation object for a buzztard based settings
 */
struct _BtGConfSettings {
  BtSettings parent;
  
  /* private */
  BtGConfSettingsPrivate *private;
};
/* structure of the gconf-settings class */
struct _BtGConfSettingsClass {
  BtSettingsClass parent_class;
  
};

/* used by GCONF_SETTINGS_TYPE */
GType bt_gconf_settings_get_type(void);

#endif // BT_GCONF_SETTINGS_H

