/* $Id: settings-page-audiodevices.h,v 1.2 2005-01-11 16:50:50 ensonic Exp $
 * class for the editor settings audiodevices page
 */

#ifndef BT_SETTINGS_PAGE_AUDIODEVICES_H
#define BT_SETTINGS_PAGE_AUDIODEVICES_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_SETTINGS_PAGE_AUDIODEVICES		        (bt_settings_page_audiodevices_get_type ())
#define BT_SETTINGS_PAGE_AUDIODEVICES(obj)		        (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_SETTINGS_PAGE_AUDIODEVICES, BtSettingsPageAudiodevices))
#define BT_SETTINGS_PAGE_AUDIODEVICES_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_SETTINGS_PAGE_AUDIODEVICES, BtSettingsPageAudiodevicesClass))
#define BT_IS_SETTINGS_PAGE_AUDIODEVICES(obj)	        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_SETTINGS_PAGE_AUDIODEVICES))
#define BT_IS_SETTINGS_PAGE_AUDIODEVICES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_SETTINGS_PAGE_AUDIODEVICES))
#define BT_SETTINGS_PAGE_AUDIODEVICES_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_SETTINGS_PAGE_AUDIODEVICES, BtSettingsPageAudiodevicesClass))

/* type macros */

typedef struct _BtSettingsPageAudiodevices BtSettingsPageAudiodevices;
typedef struct _BtSettingsPageAudiodevicesClass BtSettingsPageAudiodevicesClass;
typedef struct _BtSettingsPageAudiodevicesPrivate BtSettingsPageAudiodevicesPrivate;

/**
 * BtSettingsPageAudiodevices:
 *
 * the root window for the editor application
 */
struct _BtSettingsPageAudiodevices {
  GtkTable parent;
  
  /*< private >*/
  BtSettingsPageAudiodevicesPrivate *priv;
};
/* structure of the settings-dialog class */
struct _BtSettingsPageAudiodevicesClass {
  GtkTableClass parent;
  
};

/* used by SETTINGS_PAGE_AUDIODEVICES_TYPE */
GType bt_settings_page_audiodevices_get_type(void);

#endif // BT_SETTINGS_PAGE_AUDIODEVICES_H

