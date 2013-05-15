/* Buzztrax
 * Copyright (C) 2012 Buzztrax team <buzztrax-devel@buzztrax.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BT_AUDIO_SESSION_H
#define BT_AUDIO_SESSION_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_AUDIO_SESSION            (bt_audio_session_get_type ())
#define BT_AUDIO_SESSION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_AUDIO_SESSION, BtAudioSession))
#define BT_AUDIO_SESSION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_AUDIO_SESSION, BtAudioSessionClass))
#define BT_IS_AUDIO_SESSION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_AUDIO_SESSION))
#define BT_IS_AUDIO_SESSION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_AUDIO_SESSION))
#define BT_AUDIO_SESSION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_AUDIO_SESSION, BtAudioSessionClass))

/* type macros */

typedef struct _BtAudioSession BtAudioSession;
typedef struct _BtAudioSessionClass BtAudioSessionClass;
typedef struct _BtAudioSessionPrivate BtAudioSessionPrivate;

/**
 * BtAudioSession:
 *
 * Maintains the audio connection for the life time of the application.
 */
struct _BtAudioSession {
  const GObject parent;

  /*< private >*/
  BtAudioSessionPrivate *priv;
};
/* structure of the audio_session class */
struct _BtAudioSessionClass {
  const GObjectClass parent;
};

GType bt_audio_session_get_type(void) G_GNUC_CONST;

BtAudioSession *bt_audio_session_new(void);

#endif // BT_AUDIO_SESSION_H
