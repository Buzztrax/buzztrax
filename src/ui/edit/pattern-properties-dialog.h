/* $Id: pattern-properties-dialog.h,v 1.1 2005-04-22 17:34:20 ensonic Exp $
 * class for the pattern properties dialog
 */

#ifndef BT_PATTERN_PROPERTIES_DIALOG_H
#define BT_PATTERN_PROPERTIES_DIALOG_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_PATTERN_PROPERTIES_DIALOG		         (bt_pattern_properties_dialog_get_type ())
#define BT_PATTERN_PROPERTIES_DIALOG(obj)		         (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_PATTERN_PROPERTIES_DIALOG, BtPatternPropertiesDialog))
#define BT_PATTERN_PROPERTIES_DIALOG_CLASS(klass)	   (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_PATTERN_PROPERTIES_DIALOG, BtPatternPropertiesDialogClass))
#define BT_IS_PATTERN_PROPERTIES_DIALOG(obj)	       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_PATTERN_PROPERTIES_DIALOG))
#define BT_IS_PATTERN_PROPERTIES_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_PATTERN_PROPERTIES_DIALOG))
#define BT_PATTERN_PROPERTIES_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_PATTERN_PROPERTIES_DIALOG, BtPatternPropertiesDialogClass))

/* type macros */

typedef struct _BtPatternPropertiesDialog BtPatternPropertiesDialog;
typedef struct _BtPatternPropertiesDialogClass BtPatternPropertiesDialogClass;
typedef struct _BtPatternPropertiesDialogPrivate BtPatternPropertiesDialogPrivate;

/**
 * BtPatternPropertiesDialog:
 *
 * the root window for the editor application
 */
struct _BtPatternPropertiesDialog {
  GtkDialog parent;
  
  /*< private >*/
  BtPatternPropertiesDialogPrivate *priv;
};
/* structure of the pattern-properties-dialog class */
struct _BtPatternPropertiesDialogClass {
  GtkDialogClass parent;
  
};

/* used by PATTERN_PROPERTIES_DIALOG_TYPE */
GType bt_pattern_properties_dialog_get_type(void);

#endif // BT_PATTERN_PROPERTIES_DIALOG_H
