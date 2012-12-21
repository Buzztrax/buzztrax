/* Buzztard
 * Copyright (C) 2012 Buzztard team <buzztard-devel@lists.sf.net>
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

#ifndef BT_PLAYBACK_CONTROLLER_MIDI_H
#define BT_PLAYBACK_CONTROLLER_MIDI_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_PLAYBACK_CONTROLLER_MIDI            (bt_playback_controller_midi_get_type ())
#define BT_PLAYBACK_CONTROLLER_MIDI(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_PLAYBACK_CONTROLLER_MIDI, BtPlaybackControllerMidi))
#define BT_PLAYBACK_CONTROLLER_MIDI_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_PLAYBACK_CONTROLLER_MIDI, BtPlaybackControllerMidiClass))
#define BT_IS_PLAYBACK_CONTROLLER_MIDI(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_PLAYBACK_CONTROLLER_MIDI))
#define BT_IS_PLAYBACK_CONTROLLER_MIDI_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_PLAYBACK_CONTROLLER_MIDI))
#define BT_PLAYBACK_CONTROLLER_MIDI_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_PLAYBACK_CONTROLLER_MIDI, BtPlaybackControllerMidiClass))

/* type macros */

typedef struct _BtPlaybackControllerMidi BtPlaybackControllerMidi;
typedef struct _BtPlaybackControllerMidiClass BtPlaybackControllerMidiClass;
typedef struct _BtPlaybackControllerMidiPrivate BtPlaybackControllerMidiPrivate;

/**
 *BtPlaybackControllerMidi:
 *
 * the root window for the editor application
 */
struct _BtPlaybackControllerMidi {
  GObject parent;
  
  /*< private >*/
  BtPlaybackControllerMidiPrivate *priv;
};

struct _BtPlaybackControllerMidiClass {
  GObjectClass parent;
  
};

GType bt_playback_controller_midi_get_type(void) G_GNUC_CONST;

BtPlaybackControllerMidi *bt_playback_controller_midi_new(void);

#endif // BT_PLAYBACK_CONTROLLER_MIDI_H
