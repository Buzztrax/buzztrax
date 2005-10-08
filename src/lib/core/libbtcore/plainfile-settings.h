/* $Id: plainfile-settings.h,v 1.6 2005-10-08 18:12:13 ensonic Exp $
 * plain file based implementation sub class for buzztard settings handling
 */

#ifndef BT_PLAINFILE_SETTINGS_H
#define BT_PLAINFILE_SETTINGS_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_PLAINFILE_SETTINGS            (bt_plainfile_settings_get_type ())
#define BT_PLAINFILE_SETTINGS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_PLAINFILE_SETTINGS, BtPlainfileSettings))
#define BT_PLAINFILE_SETTINGS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_PLAINFILE_SETTINGS, BtPlainfileSettingsClass))
#define BT_IS_PLAINFILE_SETTINGS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_PLAINFILE_SETTINGS))
#define BT_IS_PLAINFILE_SETTINGS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_PLAINFILE_SETTINGS))
#define BT_PLAINFILE_SETTINGS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_PLAINFILE_SETTINGS, BtPlainfileSettingsClass))

/* type macros */

typedef struct _BtPlainfileSettings BtPlainfileSettings;
typedef struct _BtPlainfileSettingsClass BtPlainfileSettingsClass;
typedef struct _BtPlainfileSettingsPrivate BtPlainfileSettingsPrivate;

/**
 * BtPlainfileSettings:
 *
 * gconf based implementation object for a buzztard based settings
 */
struct _BtPlainfileSettings {
  BtSettings parent;
  
  /*< private >*/
  BtPlainfileSettingsPrivate *priv;
};
/* structure of the gconf-settings class */
struct _BtPlainfileSettingsClass {
  BtSettingsClass parent_class;
  
};

/* used by PLAINFILE_SETTINGS_TYPE */
GType bt_plainfile_settings_get_type(void) G_GNUC_CONST;

#endif // BT_PLAINFILE_SETTINGS_H
