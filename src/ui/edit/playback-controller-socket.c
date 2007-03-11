/* $Id: playback-controller-socket.c,v 1.7 2007-03-11 20:19:20 ensonic Exp $
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
  
  /* positions for each label */
  GList *playlist;
  gulong cur_pos;
  
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

static gchar *client_cmd_parse_and_process(BtPlaybackControllerSocket *self,gchar *cmd) {
  gchar *reply=NULL;
  BtSong *song;
 
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  
  if(!strcasecmp(cmd,"browse")) {
    BtSequence *sequence;
    BtSongInfo *song_info;
    gchar *str,*temp;
    gulong i,length;
    gboolean no_labels=TRUE;
    
    g_object_get(G_OBJECT(song),"sequence",&sequence,"song-info",&song_info,NULL);
    g_object_get(G_OBJECT(song_info),"name",&reply,NULL);
    g_object_get(G_OBJECT(sequence),"length",&length,NULL);
    
    if(self->priv->playlist) {
      g_list_free(self->priv->playlist);
      self->priv->playlist=NULL;
    }
    // get sequence labels
    for(i=0;i<length;i++) {
      str=bt_sequence_get_label(sequence,i);
      if(str) {
        temp=g_strconcat(reply,"|",str,NULL);
        g_free(str);
        g_free(reply);
        reply=temp;
        no_labels=FALSE;
        
        self->priv->playlist=g_list_append(self->priv->playlist,GINT_TO_POINTER(i));
      }
    }
    // if there are no labels, return the start
    if(no_labels) {
      temp=g_strconcat(reply,"|start",NULL);
      g_free(reply);
      reply=temp;
    }
    
    g_object_unref(song_info);
    g_object_unref(sequence);
  }
  else if(!strcasecmp(cmd,"play")) {
    bt_song_play(song);
  }
  else if(!strcasecmp(cmd,"stop")) {
    bt_song_stop(song);
  }
  else if(!strcasecmp(cmd,"status")) {
    BtSequence *sequence;
    gchar *state[]={"stopped","playing"};
    gchar *label,*label_str;
    gboolean is_playing;
    gulong pos;
    gulong p_msec,p_sec,p_min;
    gulong l_msec,l_sec,l_min;
    GstClockTime bar_time;

    g_object_get(G_OBJECT(song),"sequence",&sequence,"is-playing",&is_playing,"play-pos",&pos,NULL);
    bar_time=bt_sequence_get_bar_time(sequence);
  
    // calculate playtime
    p_msec=(gulong)((pos*bar_time)/G_USEC_PER_SEC);
    p_min=(gulong)(p_msec/60000);p_msec-=(p_min*60000);
    p_sec=(gulong)(p_msec/ 1000);p_msec-=(p_sec* 1000);

    // calculate length
    l_msec=(gulong)(bt_sequence_get_loop_time(sequence)/G_USEC_PER_SEC);
    l_min=(gulong)(l_msec/60000);l_msec-=(l_min*60000);
    l_sec=(gulong)(l_msec/ 1000);l_msec-=(l_sec* 1000);
    
    // get label text
    if((label=bt_sequence_get_label(sequence,self->priv->cur_pos))) {
      label_str=label;
    }
    else {
      label_str="start";
    }
    
    reply=g_strdup_printf("event|%s|%s|0:%02lu:%02lu.%03lu|0:%02lu:%02lu.%03lu",
      state[is_playing],label_str,
      p_min,p_sec,p_msec,l_min,l_sec,l_msec);
    
    g_free(label);
    g_object_unref(sequence);
  }
  else if(!strncasecmp(cmd,"goto ",5)) {
    self->priv->cur_pos=0;
    
    // get playlst entry
    if(cmd[5]) {
      // get position for ix-th label
      self->priv->cur_pos=GPOINTER_TO_INT(g_list_nth_data(self->priv->playlist,atoi(&cmd[5])));
    }
    
    // seek
    g_object_set(song,"play-pos",self->priv->cur_pos,NULL);
  }
  
  g_object_unref(song);
  return(reply);
}

static gchar *client_read(BtPlaybackControllerSocket *self) {
  gchar *str;
  gsize len,term;
  GError *error=NULL;
  
  g_io_channel_read_line(self->priv->client_channel,&str,&len,&term,&error);
  if(!error) {
    if(str && term>=0) str[term]='\0';
    GST_INFO("command received : %s",str);
  }
  else {
    GST_WARNING("iochannel error while reading: %s",error->message);
    g_error_free(error);
    if(str) {
      g_free(str);
      str=NULL;
    }
  }
  return(str);
}

static gboolean client_write(BtPlaybackControllerSocket *self,gchar *str) {
  gboolean res=FALSE;
  GError *error=NULL;
  gsize len;
  
  if(!self->priv->client_channel)
    return(FALSE);
  
  GST_INFO("sending reply : %s",str);

  g_io_channel_write_chars(self->priv->client_channel,str,-1,&len,&error);
  if(!error) {
    g_io_channel_write_chars(self->priv->client_channel,"\r\n",-1,&len,&error);
    if(!error) {
      g_io_channel_flush(self->priv->client_channel,&error);
      if(!error) {
        res=TRUE;
      }
      else {
        GST_WARNING("iochannel error while flushing: %s",error->message);
        g_error_free(error);
      }
    }
    else {
      GST_WARNING("iochannel error while writing: %s",error->message);
      g_error_free(error);
    }
  }
  else {
    GST_WARNING("iochannel error while writing: %s",error->message);
    g_error_free(error);
  }
  return(res);
}

//-- event handler

static gboolean client_socket_io_handler(GIOChannel *channel,GIOCondition condition,gpointer user_data) {
  BtPlaybackControllerSocket *self=BT_PLAYBACK_CONTROLLER_SOCKET(user_data);
  gboolean res=TRUE;
  gchar *cmd,*reply;

  GST_INFO("client io handler : %d",condition);
  if(condition & (G_IO_IN | G_IO_PRI)) {
    if((cmd=client_read(self))) {
      if((reply=client_cmd_parse_and_process(self,cmd))) {
        if(!client_write(self,reply)) {
          res=FALSE;
        }
        g_free(reply);
      }
      g_free(cmd);
    }
    else res=FALSE;
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
  BtPlaybackControllerSocket *self=BT_PLAYBACK_CONTROLLER_SOCKET(user_data);
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

#if 0
static void on_song_is_playing_notify(const BtSong *song,GParamSpec *arg,gpointer user_data) {
  BtPlaybackControllerSocket *self=BT_PLAYBACK_CONTROLLER_SOCKET(user_data);
  gboolean is_playing;

  g_assert(user_data);

  g_object_get(G_OBJECT(song),"is-playing",&is_playing,NULL);
  if(is_playing) {
    client_write(self,"playing");
  }
  else {
    client_write(self,"stopped");
  }
}
#endif

static void on_song_changed(const BtEditApplication *app,GParamSpec *arg,gpointer user_data) {
  BtPlaybackControllerSocket *self=BT_PLAYBACK_CONTROLLER_SOCKET(user_data);
  BtSong *song;

  g_assert(user_data);

  GST_INFO("song has changed : app=%p, toolbar=%p",app,user_data);
  
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  if(!song) return;
  
  self->priv->cur_pos=0;
  client_write(self,"flush");
#if 0
  g_signal_connect(G_OBJECT(song),"notify::is-playing",G_CALLBACK(on_song_is_playing_notify),(gpointer)self);
#endif
    
  g_object_unref(song);
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
      // register event handlers
      g_signal_connect(G_OBJECT(self->priv->app), "notify::song", G_CALLBACK(on_song_changed), (gpointer)self);
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
  BtPlaybackControllerSocket *self = BT_PLAYBACK_CONTROLLER_SOCKET(object);

  GST_DEBUG("!!!! self=%p",self);
  
  if(self->priv->playlist) {
    g_list_free(self->priv->playlist);
    self->priv->playlist=NULL;
  }

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
