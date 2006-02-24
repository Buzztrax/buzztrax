/* $Id: wave.h,v 1.6 2006-02-24 19:51:49 ensonic Exp $
 * class for wave
 */

#ifndef BT_WAVE_H
#define BT_WAVE_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_WAVE            (bt_wave_get_type ())
#define BT_WAVE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_WAVE, BtWave))
#define BT_WAVE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_WAVE, BtWaveClass))
#define BT_IS_WAVE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_WAVE))
#define BT_IS_WAVE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_WAVE))
#define BT_WAVE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_WAVE, BtWaveClass))

/* type macros */

typedef struct _BtWave BtWave;
typedef struct _BtWaveClass BtWaveClass;
typedef struct _BtWavePrivate BtWavePrivate;

/**
 * BtWave:
 *
 * virtual hardware setup
 * (contains #BtMachine and #BtWire objects)
 */
struct _BtWave {
  GObject parent;
  
  /*< private >*/
  BtWavePrivate *priv;
};

struct _BtWaveClass {
  GObjectClass parent;
};

GType bt_wave_get_type(void) G_GNUC_CONST;


#endif // BT_WAVE_H
