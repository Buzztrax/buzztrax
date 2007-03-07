/* $Id: playback-controller-socket.c,v 1.2 2007-03-07 12:36:09 ensonic Exp $
 *
 * Buzztard
 * Copyright (C) 2007 Buzztard team <buzztard-devel@lists.sf.net>
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/**
 * SECTION:btplaybackcontrollersocket
 * @short_description: sockets based playback controller
 *
 * Function can be tested doing "netcat -n 127.0.0.1 7654".
 */

#define BT_EDIT
#define BT_PLAYBACK_CONTROLLER_SOCKET_C

#include "bt-edit.h"

#include <sys/types.h>
#include <sys/socket.h>

enum {
  SETTINGS_DIALOG_APP=1,
};

struct _BtPlaybackControllerSocketPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* the application */
  union {
    BtEditApplication *app;
    gpointer app_ptr;
  };
  
  /* master */
  gint master_socket,master_source;
  GIOChannel *master_channel;
  /* client */
  gint client_socket,client_source;
  GIOChannel *client_channel;
};

static GObjectClass *parent_class=NULL;

//-- helper methods

static void client_connection_free(BtPlaybackControllerSocket *self) {
  if(self->priv->client_channel) {
    GError *error=NULL;

    g_source_remove(self->priv->client_source);
    g_io_channel_unref(self->priv->client_channel);
    g_io_channel_shutdown(self->priv->client_channel,TRUE,&error);
    if(error) {
      GST_WARNING("iochannel error while shutting down client: %s",error->message);
      g_error_free(error);
    }
    g_io_channel_unref(self->priv->client_channel);
    self->priv->client_channel=NULL;
  }
  if(self->priv->client_socket>-1) {
    close(self->priv->client_socket);
    self->priv->client_socket=-1;
  }
}

//-- event handler

static gboolean client_socket_io_handler(GIOChannel *channel,GIOCondition condition,gpointer user_data) {
  BtPlaybackControllerSocket *self = BT_PLAYBACK_CONTROLLER_SOCKET(user_data);
  gboolean res=TRUE;
  gchar *str;
  gsize len,term;
  GError *error=NULL;

  GST_INFO("client io handler : %d",condition);
  if(condition & (G_IO_IN | G_IO_PRI)) {
    g_io_channel_read_line(channel,&str,&len,&term,&error);
    if(!error) {
      if(str && term>=0) str[term]='\0';
      GST_INFO("command received : %s",str);
      if(str) {
        // do something
        g_io_channel_write_chars(channel,"OKAY\n",-1,&len,&error);
        if(!error) {
          g_io_channel_flush(channel,&error);
          //g_io_channel_seek_position(channel,G_GINT64_CONSTANT(0),G_SEEK_CUR,&error);
          if(error) {
            GST_WARNING("iochannel error while flushing: %s",error->message);
            g_error_free(error);
            res=FALSE;
          }
        }
        else {
          GST_WARNING("iochannel error while writing: %s",error->message);
          g_error_free(error);
          res=FALSE;
        }
        g_free(str);
      }
      else {
        res=FALSE;
      }
    }
    else {
      GST_WARNING("iochannel error while reading: %s",error->message);
      g_error_free(error);
      res=FALSE;
    }
  }
  if(condition & (G_IO_HUP | G_IO_ERR | G_IO_NVAL)) {
    res=FALSE;
  }
  if(!res) {
    self->priv->master_source=-1;
  }
  return(res);
}

static gboolean master_socket_io_handler(GIOChannel *channel,GIOCondition condition,gpointer user_data) {
  BtPlaybackControllerSocket *self = BT_PLAYBACK_CONTROLLER_SOCKET(user_data);
  struct sockaddr addr={0,};
  socklen_t addrlen=sizeof(struct sockaddr);
  
  GST_INFO("master io handler : %d",condition);
  if(condition & (G_IO_IN | G_IO_PRI)) {
    if((self->priv->client_socket=accept(self->priv->master_socket, &addr, &addrlen))<0) {
      GST_WARNING ("accept error: %s", g_strerror (errno));
    }
    else {
      self->priv->client_channel=g_io_channel_unix_new(self->priv->client_socket);
      //g_io_channel_set_encoding(self->priv->client_channel,NULL,NULL);
      //g_io_channel_set_buffered(self->priv->client_channel,FALSE);
      self->priv->client_source=g_io_add_watch(self->priv->client_channel,G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL,client_socket_io_handler,(gpointer)self);
      GST_INFO("playback controller client connected");
    }
  }
  if(condition & (G_IO_HUP | G_IO_ERR | G_IO_NVAL)) {
    client_connection_free(self);
    GST_INFO("playback controller client disconnected");
  }
  return(TRUE);
}

//-- constructor methods

/**
 * bt_playback_controller_socket_new:
 * @app: the application to create the controller for
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtPlaybackControllerSocket *bt_playback_controller_socket_new(const BtEditApplication *app) {
  BtPlaybackControllerSocket *self;

  if(!(self=BT_PLAYBACK_CONTROLLER_SOCKET(g_object_new(BT_TYPE_PLAYBACK_CONTROLLER_SOCKET,"app",app,NULL)))) {
    goto Error;
  }  
  return(self);
Error:
  g_object_try_unref(self);
  return(NULL);
}

//-- methods

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_playback_controller_socket_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtPlaybackControllerSocket *self = BT_PLAYBACK_CONTROLLER_SOCKET(object);
  return_if_disposed();
  switch (property_id) {
    case SETTINGS_DIALOG_APP: {
      g_value_set_object(value, self->priv->app);
    } break;
    default: {
       G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_playback_controller_socket_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtPlaybackControllerSocket *self = BT_PLAYBACK_CONTROLLER_SOCKET(object);
  return_if_disposed();
  switch (property_id) {
    case SETTINGS_DIALOG_APP: {
      g_object_try_weak_unref(self->priv->app);
      self->priv->app = BT_EDIT_APPLICATION(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->app);
      //GST_DEBUG("set the app for settings_dialog: %p",self->priv->app);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_playback_controller_socket_dispose(GObject *object) {
  BtPlaybackControllerSocket *self = BT_PLAYBACK_CONTROLLER_SOCKET(object);
  GError *error=NULL;

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  g_object_try_weak_unref(self->priv->app);
  
  if(self->priv->master_channel) {
    if(self->priv->master_source>=0) {
      g_source_remove(self->priv->master_source);
      g_io_channel_unref(self->priv->master_channel);
      self->priv->master_source=-1;
    }
    g_io_channel_shutdown(self->priv->master_channel,TRUE,&error);
    if(error) {
      GST_WARNING("iochannel error while shutting down master: %s",error->message);
      g_error_free(error);
    }
    g_io_channel_unref(self->priv->master_channel);
  }
  if(self->priv->master_socket>-1) {
    close(self->priv->master_socket);
  }
  client_connection_free(self);

  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_playback_controller_socket_finalize(GObject *object) {
  //BtPlaybackControllerSocket *self = BT_PLAYBACK_CONTROLLER_SOCKET(object);

  //GST_DEBUG("!!!! self=%p",self);

  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void bt_playback_controller_socket_init(GTypeInstance *instance, gpointer g_class) {
  BtPlaybackControllerSocket *self = BT_PLAYBACK_CONTROLLER_SOCKET(instance);
  static struct sockaddr_in serv_addr={0,};;
  
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_PLAYBACK_CONTROLLER_SOCKET, BtPlaybackControllerSocketPrivate);
  
  /* new socket: internet address family, stream socket */
  if((self->priv->master_socket=socket(AF_INET,SOCK_STREAM,0))<0) {
    goto socket_error;
  }
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);// my address
  // @todo: make port a config options
  serv_addr.sin_port = htons(7654);
  if(bind(self->priv->master_socket,(struct sockaddr *)&serv_addr,sizeof(serv_addr))<0) {
    goto bind_error;
  }
  if(listen(self->priv->master_socket,64)<0) {
    goto listen_error;
  }
  self->priv->master_channel=g_io_channel_unix_new(self->priv->master_socket);
  //g_io_channel_set_encoding(self->priv->master_channel,NULL,NULL);
  //g_io_channel_set_buffered(self->priv->master_channel,FALSE);
  self->priv->master_source=g_io_add_watch(self->priv->master_channel,G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL,master_socket_io_handler,(gpointer)self);
 
  GST_INFO("playback controller running");
  return;

socket_error:
  GST_WARNING("socket allocation failed: %s",g_strerror(errno));
  return;
bind_error:
  GST_WARNING("binding the socket failed: %s",g_strerror(errno));
  return;
listen_error:
  GST_WARNING("listen failed: %s",g_strerror(errno));
  return;
}

static void bt_playback_controller_socket_class_init(BtPlaybackControllerSocketClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtPlaybackControllerSocketPrivate));
  
  gobject_class->set_property = bt_playback_controller_socket_set_property;
  gobject_class->get_property = bt_playback_controller_socket_get_property;
  gobject_class->dispose      = bt_playback_controller_socket_dispose;
  gobject_class->finalize     = bt_playback_controller_socket_finalize;

  g_object_class_install_property(gobject_class,SETTINGS_DIALOG_APP,
                                  g_param_spec_object("app",
                                     "app construct prop",
                                     "Set application object, the dialog belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

}

GType bt_playback_controller_socket_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      G_STRUCT_SIZE(BtPlaybackControllerSocketClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_playback_controller_socket_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtPlaybackControllerSocket),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_playback_controller_socket_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(G_TYPE_OBJECT,"BtPlaybackControllerSocket",&info,0);
  }
  return type;
}
