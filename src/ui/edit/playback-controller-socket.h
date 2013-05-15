/* Buzztrax
 * Copyright (C) 2007 Buzztrax team <buzztrax-devel@buzztrax.org>
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

#ifndef BT_PLAYBACK_CONTROLLER_SOCKET_H
#define BT_PLAYBACK_CONTROLLER_SOCKET_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_PLAYBACK_CONTROLLER_SOCKET            (bt_playback_controller_socket_get_type ())
#define BT_PLAYBACK_CONTROLLER_SOCKET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_PLAYBACK_CONTROLLER_SOCKET, BtPlaybackControllerSocket))
#define BT_PLAYBACK_CONTROLLER_SOCKET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_PLAYBACK_CONTROLLER_SOCKET, BtPlaybackControllerSocketClass))
#define BT_IS_PLAYBACK_CONTROLLER_SOCKET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_PLAYBACK_CONTROLLER_SOCKET))
#define BT_IS_PLAYBACK_CONTROLLER_SOCKET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_PLAYBACK_CONTROLLER_SOCKET))
#define BT_PLAYBACK_CONTROLLER_SOCKET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_PLAYBACK_CONTROLLER_SOCKET, BtPlaybackControllerSocketClass))

/* type macros */

typedef struct _BtPlaybackControllerSocket BtPlaybackControllerSocket;
typedef struct _BtPlaybackControllerSocketClass BtPlaybackControllerSocketClass;
typedef struct _BtPlaybackControllerSocketPrivate BtPlaybackControllerSocketPrivate;

/**
 *BtPlaybackControllerSocket:
 *
 * the root window for the editor application
 */
struct _BtPlaybackControllerSocket {
  GObject parent;
  
  /*< private >*/
  BtPlaybackControllerSocketPrivate *priv;
};

struct _BtPlaybackControllerSocketClass {
  GObjectClass parent;
  
};

GType bt_playback_controller_socket_get_type(void) G_GNUC_CONST;

BtPlaybackControllerSocket *bt_playback_controller_socket_new(void);

#endif // BT_PLAYBACK_CONTROLLER_SOCKET_H
