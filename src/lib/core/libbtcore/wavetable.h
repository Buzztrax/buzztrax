/* $Id: wavetable.h,v 1.4 2005-08-05 09:36:17 ensonic Exp $
 * class for wavetable
 */

#ifndef BT_WAVETABLE_H
#define BT_WAVETABLE_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_WAVETABLE             (bt_wavetable_get_type ())
#define BT_WAVETABLE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_WAVETABLE, BtWavetable))
#define BT_WAVETABLE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_WAVETABLE, BtWavetableClass))
#define BT_IS_WAVETABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_WAVETABLE))
#define BT_IS_WAVETABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_WAVETABLE))
#define BT_WAVETABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_WAVETABLE, BtWavetableClass))

/* type macros */

typedef struct _BtWavetable BtWavetable;
typedef struct _BtWavetableClass BtWavetableClass;
typedef struct _BtWavetablePrivate BtWavetablePrivate;

/**
 * BtWavetable:
 *
 * virtual hardware setup
 * (contains #BtMachine and #BtWire objects)
 */
struct _BtWavetable {
  GObject parent;
  
  /*< private >*/
  BtWavetablePrivate *priv;
};
/* structure of the setup class */
struct _BtWavetableClass {
  GObjectClass parent_class;
};

/* used by WAVETABLE_TYPE */
GType bt_wavetable_get_type(void);

#endif // BT_WAVETABLE_H
