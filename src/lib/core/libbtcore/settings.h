/* $Id: settings.h,v 1.3 2004-09-28 16:28:11 ensonic Exp $
 * base class for buzztard settings handling
 */

#ifndef BT_SETTINGS_H
#define BT_SETTINGS_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_SETTINGS		        (bt_settings_get_type ())
#define BT_SETTINGS(obj)		        (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_SETTINGS, BtSettings))
#define BT_SETTINGS_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_SETTINGS, BtSettingsClass))
#define BT_IS_SETTINGS(obj)	        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_SETTINGS))
#define BT_IS_SETTINGS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_SETTINGS))
#define BT_SETTINGS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_SETTINGS, BtSettingsClass))

/* type macros */

typedef struct _BtSettings BtSettings;
typedef struct _BtSettingsClass BtSettingsClass;
typedef struct _BtSettingsPrivate BtSettingsPrivate;

/**
 * BtSettings:
 *
 * base object for a buzztard based settings
 */
struct _BtSettings {
  GObject parent;
  
  /* private */
  BtSettingsPrivate *private;
};
/* structure of the settings class */
struct _BtSettingsClass {
  GObjectClass parent_class;

  /* class methods */
	//gboolean (*get)(const gpointer self);
	//gboolean (*set)(const gpointer self);
};

/* used by SETTINGS_TYPE */
GType bt_settings_get_type(void);

#endif // BT_SETTINGS_H

