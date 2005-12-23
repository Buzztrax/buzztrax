/* $Id: sink-bin.h,v 1.3 2005-12-23 14:03:03 ensonic Exp $
 * class for a sink bin
 */

#ifndef BT_SINK_BIN_H
#define BT_SINK_BIN_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_SINK_BIN            (bt_sink_bin_get_type ())
#define BT_SINK_BIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_SINK_BIN, BtSinkBin))
#define BT_SINK_BIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_SINK_BIN, BtSinkBinClass))
#define BT_IS_SINK_BIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_SINK_BIN))
#define BT_IS_SINK_BIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_SINK_BIN))
#define BT_SINK_BIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_SINK_BIN, BtSinkBinClass))

/* type macros */

typedef struct _BtSinkBin BtSinkBin;
typedef struct _BtSinkBinClass BtSinkBinClass;
typedef struct _BtSinkBinPrivate BtSinkBinPrivate;

/**
 * BtSinkBin:
 *
 * Sub-class of a #GstBin that implements a signal output
 * (a machine with inputs only).
 */
struct _BtSinkBin {
  GstBin parent;
  
  /*< private >*/
  BtSinkBinPrivate *priv;
};
/* structure of the sink_bin class */
struct _BtSinkBinClass {
  GstBinClass parent;
};

#define BT_TYPE_SINK_BIN_MODE       (bt_sink_bin_mode_get_type())

/**
 * BtSinkBinMode:
 * @BT_SINK_BIN_MODE_PLAY: play the song
 * @BT_SINK_BIN_MODE_RECORD: record to file
 * @BT_SINK_BIN_MODE_PLAY_AND_RECORD: play and record together
 *
 * #BtSinkMachine supports several modes of operation. Playing is the default
 * mode.
 */
typedef enum {
  BT_SINK_BIN_MODE_PLAY=0,
  BT_SINK_BIN_MODE_RECORD,
  BT_SINK_BIN_MODE_PLAY_AND_RECORD
} BtSinkBinMode;

#define BT_TYPE_SINK_BIN_RECORD_FORMAT (bt_sink_bin_record_format_get_type())

/**
 * BtSinkBinRecordFormat:
 * @BT_SINK_BIN_RECORD_FORMAT_OGG_VORBIS: ogg vorbis
 * @BT_SINK_BIN_RECORD_FORMAT_MP3: mp3
 * @BT_SINK_BIN_RECORD_FORMAT_WAV: wav
 * @BT_SINK_BIN_RECORD_FORMAT_FLAC: flac
 * @BT_SINK_BIN_RECORD_FORMAT_RAW: raw
 *
 * #BtSinkMachine can record audio in several formats.
 */
typedef enum {
  BT_SINK_BIN_RECORD_FORMAT_OGG_VORBIS=0,
  BT_SINK_BIN_RECORD_FORMAT_MP3,
  BT_SINK_BIN_RECORD_FORMAT_WAV,
  BT_SINK_BIN_RECORD_FORMAT_FLAC,
  BT_SINK_BIN_RECORD_FORMAT_RAW
} BtSinkBinRecordFormat;


/* used by SINK_BIN_TYPE */
GType bt_sink_bin_get_type(void) G_GNUC_CONST;
/* used by SINK_BIN_MODE_TYPE */
GType bt_sink_bin_mode_get_type(void) G_GNUC_CONST;
/* used by SINK_BIN_RECORD_FORMAT_TYPE */
GType bt_sink_bin_record_format_get_type(void) G_GNUC_CONST;

#endif // BT_SINK_BIN_H
