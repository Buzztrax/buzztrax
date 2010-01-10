/* $Id$
 *
 * Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
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
 * SECTION:btwave
 * @short_description: one #BtWavetable entry that keeps a list of #BtWavelevels
 *
 * Represents one instrument. Contains one or more #BtWavelevels.
 */
/* @todo: save sample file length and/or md5sum in file:
 * - if we miss files, we can do a xsesame search and use the details to verify
 * - when loading, we might also use the details as a sanity check
 * @idea: record wave-entries
 * - record wave from alsasrc
 *   - like we load & decode to tempfile, use this to record a sound
 *   - needs some dedicated ui for choosing the input, and format (m/s)
 * - recording from song playback
 *   - would be nice to have ui in song-recorder to select a wavetable slot
 *     - this would disable the file-selector, record to tempfile and load from
 *       there
 * - all recording features need some error handling when saving to plain xml
 *   song (no waves included)
 * @todo: bpm support
 * - listen for tags when loading and show BPM for waves if we have it
 * -if we don't have bpm, but have the bpm detect plugin, offer detection in
 *   context menu
 * - if we have bpm and its different from song-bpm offer adjust in the contect
 *   menu to change base_notes to match it
 * @todo: when loading a wave from filesystem we need to somehow also make a
 * copy of the original file to ensure that we can save it
 */
#define BT_CORE
#define BT_WAVE_C

#include "core_private.h"

//-- signal ids

enum {
  LOADING_DONE_EVENT,
  //WAVELEVEL_ADDED_EVENT,
  LAST_SIGNAL
};

//-- property ids

enum {
  WAVE_SONG=1,
  WAVE_WAVELEVELS,
  WAVE_INDEX,
  WAVE_NAME,
  WAVE_URI,
  WAVE_VOLUME,
  WAVE_LOOP_MODE, // OFF, FORWARD, PINGPONG
  WAVE_CHANNELS
};

struct _BtWavePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the song the wave belongs to */
  G_POINTER_ALIAS(BtSong *,song);

  /* each wave has an index number, the list of waves can have empty slots */
  gulong index;
  /* the name of the wave and the the sample file */
  gchar *name;
  gchar *uri;
  /* wave properties common to all wavelevels */
  gdouble volume;
  BtWaveLoopMode loop_mode;
  guint channels; /* number of channels (1,2) */

  GList *wavelevels;    // each entry points to a BtWavelevel
  
  /* wave loader */
  GstElement *pipeline,*fmt;
  gint fd,ext_fd;
};

static GQuark error_domain=0;

static GObjectClass *parent_class=NULL;

static guint signals[LAST_SIGNAL]={0,};

//-- enums

GType bt_wave_loop_mode_get_type(void) {
  static GType type = 0;
  if(G_UNLIKELY(type==0)) {
    static const GEnumValue values[] = {
      { BT_WAVE_LOOP_MODE_OFF,      "off",       "off" },
      { BT_WAVE_LOOP_MODE_FORWARD,  "forward",   "forward" },
      { BT_WAVE_LOOP_MODE_PINGPONG, "ping-pong", "ping-pong" },
      { 0, NULL, NULL},
    };
    type = g_enum_register_static("BtWaveLoopMode", values);
  }
  return type;
}

//-- helper

static void on_wave_io_error(const GstBus * const bus, GstMessage *message, gconstpointer user_data) {
  GError *err = NULL;
  gchar *dbg = NULL;
  
  gst_message_parse_error(message, &err, &dbg);
  GST_WARNING ("ERROR: %s (%s)", err->message, (dbg) ? dbg : "no details");
  g_error_free (err);
  g_free (dbg);
  
  //g_signal_emit(G_OBJECT(self),signals[LOADING_DONE_EVENT], 0, FALSE);
  //wave_loader_free(self);
}

static void on_wave_io_warning(const GstBus * const bus, GstMessage * message, gconstpointer user_data) {
  GError *err = NULL;
  gchar *dbg = NULL;
  
  gst_message_parse_warning(message, &err, &dbg);
  GST_WARNING ("WARNING: %s (%s)", err->message, (dbg) ? dbg : "no details");
  g_error_free (err);
  g_free (dbg);
  
  //g_signal_emit(G_OBJECT(self),signals[LOADING_DONE_EVENT], 0, FALSE);
  //wave_loader_free(self);
}

static void wave_loader_free(const BtWave *self) {
  if(self->priv->pipeline) {
    GstBus *bus;
    
    bus=gst_element_get_bus(self->priv->pipeline);
    gst_bus_remove_signal_watch(bus);
    gst_object_unref(bus);

    gst_object_unref(self->priv->pipeline);
    self->priv->pipeline=NULL;
    self->priv->fmt=NULL;
  }
  if(self->priv->ext_fd!=-1) {
    close(self->priv->ext_fd);
    self->priv->ext_fd=-1;
  }
  if(self->priv->fd!=-1) {
    close(self->priv->fd);
    self->priv->fd=-1;
  }
}

static void on_wave_loader_new_pad(GstElement *bin,GstPad *pad,gboolean islast,gpointer user_data) {
  // @todo: if we pass the pad in user_data we can use gst_pad_link()
  if(!gst_element_link(bin,GST_ELEMENT(user_data))) {
    GST_WARNING("Can't link output of wave decoder to converter.");
  }
}

static void on_wave_loader_eos(const GstBus * const bus, const GstMessage * const message, gconstpointer user_data) {
  BtWave *self=BT_WAVE(user_data);
  BtWavelevel *wavelevel;
  GstPad *pad;
  GstCaps *caps;
  gint64 duration;
  guint64 length=0;
  gint channels=1,rate=GST_AUDIO_DEF_RATE;
  GstFormat format=GST_FORMAT_TIME;
  gpointer data=NULL;
  struct stat buf={0,};
  
  GST_INFO("sample loading done, eos from %s",GST_MESSAGE_SRC_NAME(message));
  
  // query length and convert to samples
  if(!gst_element_query_duration(self->priv->pipeline, &format, &duration)) {
    GST_WARNING("getting sample duration failed");
  }
  
  // get caps for sample rate and channels 
  if((pad=gst_element_get_static_pad(self->priv->fmt,"src"))) {
    caps=GST_PAD_CAPS(pad);
    if(caps && GST_CAPS_IS_SIMPLE(caps)) {
      GstStructure *structure = gst_caps_get_structure(caps,0);
      
      gst_structure_get_int(structure,"channels",&channels);
      gst_structure_get_int(structure,"rate",&rate);
      length=gst_util_uint64_scale(duration,(guint64)rate,GST_SECOND);
    }
    else {
      GST_WARNING("No caps or format has not been fixed.");
    }
    gst_object_unref(pad);
  }

  GST_INFO("sample decoded: channels=%d, rate=%d, length=%"GST_TIME_FORMAT,
    channels, rate, GST_TIME_ARGS(duration));
  
  if(!(fstat(self->priv->fd, &buf))) {
    if((data=g_try_malloc(buf.st_size))) {
      /* mmap is unsave for removable drives :(
       * gpointer data=mmap(void *start, buf->st_size, PROT_READ, MAP_SHARED, self->priv->fd, 0);
       */
      if(lseek(self->priv->fd,0,SEEK_SET) == 0) {
        ssize_t bytes;
        
        bytes=read(self->priv->fd,data,buf.st_size);
        
        self->priv->channels=channels;
        g_object_notify(G_OBJECT(self),"channels");
    
        wavelevel=bt_wavelevel_new(self->priv->song,self,
          BT_WAVELEVEL_DEFAULT_ROOT_NOTE,(gulong)length,
          -1,-1, /* loop */
          rate, (gconstpointer)data);
        g_object_unref(wavelevel);
        /* emit signal so that UI can redraw */
        GST_INFO("sample loaded (%"G_GSSIZE_FORMAT"/%ld bytes)",bytes,buf.st_size);
        g_signal_emit(G_OBJECT(self),signals[LOADING_DONE_EVENT], 0, TRUE);
      }
      else {
        GST_WARNING("can't seek to start of sample data");
        g_signal_emit(G_OBJECT(self),signals[LOADING_DONE_EVENT], 0, FALSE);
      }
    }
    else {
      GST_WARNING("sample is too long (%ld bytes), not trying to load",buf.st_size);
      g_signal_emit(G_OBJECT(self),signals[LOADING_DONE_EVENT], 0, FALSE);
    }
  }
  else {
    GST_WARNING("can't stat() sample");
    g_signal_emit(G_OBJECT(self),signals[LOADING_DONE_EVENT], 0, FALSE);
  }
  
  gst_element_set_state(self->priv->pipeline,GST_STATE_NULL);
}

/*
 * bt_wave_load_from_uri:
 * @self: the wave to load
 * @uri: the location to load from
 *
 * Load the wavedata from the @uri.
 *
 * Returns: %TRUE if the wavedata could be loaded
 */
static gboolean bt_wave_load_from_uri(const BtWave * const self, const gchar * const uri) {
  gboolean res=TRUE;
  GstElement *src,*dec,*conv,*sink;
  GstBus *bus;
  GstCaps *caps;

  GST_INFO("about to load sample %s / %s",self->priv->uri,uri);
  // this leaks!
  //GST_INFO("current dir is %s", g_get_current_dir());

  // check if the url is valid
  // if(!uri) goto invalid_uri;

  // create loader pipeline
  self->priv->pipeline=gst_pipeline_new("wave-loader");
  src=gst_element_make_from_uri(GST_URI_SRC,uri,NULL);
  //dec=gst_element_factory_make("decodebin2",NULL);
  dec=gst_element_factory_make("decodebin",NULL);
  conv=gst_element_factory_make("audioconvert",NULL);
  self->priv->fmt=gst_element_factory_make("capsfilter",NULL);
  sink=gst_element_factory_make("fdsink",NULL);
  
  // configure elements
  caps=gst_caps_new_simple("audio/x-raw-int",
    "rate", GST_TYPE_INT_RANGE,1,G_MAXINT,
    "channels",GST_TYPE_INT_RANGE,1,2,
    "width",G_TYPE_INT,16,
    "endianness",G_TYPE_INT,G_BYTE_ORDER,
    "signed",G_TYPE_BOOLEAN,TRUE,
    NULL);
  g_object_set(self->priv->fmt,"caps",caps,NULL);
  gst_caps_unref(caps);
  
  self->priv->fd=fileno(tmpfile()); // or mkstemp("...XXXXXX")
  g_object_set(sink,"fd",self->priv->fd,"sync",FALSE,NULL);

  // add and link
  gst_bin_add_many(GST_BIN(self->priv->pipeline),src,dec,conv,self->priv->fmt,sink,NULL);
  res=gst_element_link(src,dec);
  if(!res) {
    GST_WARNING ("Can't link wave loader pipeline (src ! dec).");
    goto Error;
  }
  res=gst_element_link_many(conv,self->priv->fmt,sink,NULL);
  if(!res) {
    GST_WARNING ("Can't link wave loader pipeline (conf ! fmt ! sink).");
    goto Error;
  }
  g_signal_connect(G_OBJECT(dec),"new-decoded-pad",G_CALLBACK(on_wave_loader_new_pad),(gpointer)conv);

  /* @todo: during loading wave-data (into wavelevels)
   * - use statusbar for loader progress ("status" property like in song_io)
   * - should we do some size checks to avoid unpacking the audio track of a full
   *   video on a machine with low memory
   *   - if so, how to get real/virtual memory sizes?
   *     mallinfo() not enough, sysconf()?
   */
  
  bus=gst_element_get_bus(self->priv->pipeline);
  gst_bus_add_signal_watch_full(bus, G_PRIORITY_HIGH);
  g_signal_connect(bus, "message::error", G_CALLBACK(on_wave_io_error), (gpointer)self);
  g_signal_connect(bus, "message::warning", G_CALLBACK(on_wave_io_warning), (gpointer)self);
  g_signal_connect(bus, "message::eos", G_CALLBACK(on_wave_loader_eos), (gpointer)self);
  gst_object_unref(bus);
  
  // play and wait for EOS
  if(gst_element_set_state(self->priv->pipeline,GST_STATE_PLAYING)==GST_STATE_CHANGE_FAILURE) {
    GST_WARNING ("Can't set wave loader pipeline for %s / %s to playing",self->priv->uri,uri);
    gst_element_set_state(self->priv->pipeline,GST_STATE_NULL);
    res=FALSE;
  }
  else {
    GST_INFO("loading sample ...");
  }
  
  /* no need to *wait* for eos, we fire loading done signal */

Error:
  return(res);
}


/* bt_wave_save_to_fd:
 *
 * Write the sample data as a wav to a filedescriptor and keep it open
 */
static gboolean bt_wave_save_to_fd(const BtWave * const self) {
  gboolean res=TRUE;
  GstElement *pipeline;
  GstElement *src,*fmt,*enc,*sink;
  BtWavelevel *wavelevel;
  GstBus *bus;
  GstCaps *caps;
  //gchar *fn_wav;
  gulong srate,length,size,written;
  gint16 *data;
  gint fd;
  
  // we should have a wave loaded
  g_assert(self->priv->wavelevels->data);

  //fn_wav=g_strdup_printf("%s" G_DIR_SEPARATOR_S "XXXXXX",g_get_tmp_dir());
  fd=fileno(tmpfile());
  self->priv->ext_fd=fileno(tmpfile());
  //self->priv->ext_fd=mkstemp(fn_wav);
  //g_free(fn_wav);

  // the data is in the wave-level :/
  wavelevel=BT_WAVELEVEL(self->priv->wavelevels->data);
  g_object_get(wavelevel,
    "data",&data,
    "length",&length,
    "rate",&srate,
    NULL);
  size=length*self->priv->channels*sizeof(gint16);
  GST_INFO("about to format data as wav to fd=%d, %d bytes to write",self->priv->ext_fd, size);

  // create saver pipeline
  pipeline=gst_pipeline_new("wave-saver");
  src=gst_element_factory_make("fdsrc",NULL);
  fmt=gst_element_factory_make("capsfilter",NULL);
  enc=gst_element_factory_make("wavenc",NULL);
  sink=gst_element_factory_make("fdsink",NULL);
  GST_DEBUG("%p ! %p ! %p ! %p",src,fmt,enc,sink);

  // configure elements
  caps=gst_caps_new_simple("audio/x-raw-int",
    "rate", G_TYPE_INT,srate,
    "channels",G_TYPE_INT,self->priv->channels,
    "width",G_TYPE_INT,16,
    "endianness",G_TYPE_INT,G_BYTE_ORDER,
    "signed",G_TYPE_BOOLEAN,TRUE,
    NULL);
  g_object_set(fmt,"caps",caps,NULL);
  gst_caps_unref(caps);

  g_object_set(src,"fd",fd,"num-buffers",1,"blocksize",size,NULL);
  g_object_set(sink,"fd",self->priv->ext_fd,"sync",FALSE,NULL);

  // add and link
  gst_bin_add_many(GST_BIN(pipeline),src,fmt,enc,sink,NULL);
  res=gst_element_link_many(src,fmt,enc,sink,NULL);
  if(!res) {
    GST_WARNING ("Can't link wave loader pipeline (src ! dec).");
    goto Error;
  }

  bus=gst_element_get_bus(pipeline);
  //gst_bus_add_signal_watch_full(bus, G_PRIORITY_HIGH);
  //g_signal_connect(bus, "message::error", G_CALLBACK(on_wave_io_error), (gpointer)self);
  //g_signal_connect(bus, "message::warning", G_CALLBACK(on_wave_io_warning), (gpointer)self);

  GST_INFO("run pipeline");

  written=write(fd,data,size);
  size-=written;
  GST_INFO("wrote %d, todo %d",written, size);
  lseek(fd,0,SEEK_SET);

  // play and wait for EOS 
  if(gst_element_set_state(pipeline,GST_STATE_PLAYING)==GST_STATE_CHANGE_FAILURE) {
    GST_WARNING ("Can't set wave saver pipeline for to playing");
    gst_element_set_state(pipeline,GST_STATE_NULL);
    res=FALSE;
  }
  else {
    GstMessage *msg;

    // we have num-buffers
    //gst_element_send_event(src,gst_event_new_eos());

    // we need to run this sync'ed with eos
    GST_INFO("waiting for EOS");

    msg=gst_bus_poll(bus,GST_MESSAGE_EOS,GST_CLOCK_TIME_NONE);
    if(msg) {
      gst_message_unref(msg);
    }
  }

  gst_object_unref(bus);
  gst_element_set_state(pipeline,GST_STATE_NULL);
  close(fd);

  GST_INFO("sample saved");

Error:
  return(res);
}


static gboolean bt_wave_save_from_fd(const BtWave * const self, BtSongIONative *song_io, const gchar * const uri) {
  gchar *fn,*fp;

  // need to rewrite path for target filename
  if(!(fn=strrchr(self->priv->uri,'/')))
    fn=self->priv->uri;
  else
    fn++;

  fp=g_strdup_printf("wavetable/%s",fn);

  GST_INFO("saving external uri=%s,%s -> zip=%s",uri,self->priv->uri,fp);
  
  bt_song_io_native_bzt_copy_from_uri(BT_SONG_IO_NATIVE_BZT(song_io),fp,uri);
  g_free(fp);
  
  return(TRUE);
}


//-- constructor methods

/**
 * bt_wave_new:
 * @song: the song the new instance belongs to
 * @name: the display name for the new wave
 * @uri: the location of the sample data
 * @index: the list slot for the new wave
 * @volume: the volume of the wave
 * @loop_mode: loop playback mode
 * @channels: number of audio channels
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtWave *bt_wave_new(const BtSong * const song, const gchar * const name, const gchar * const uri, const gulong index, const gdouble volume, const  BtWaveLoopMode loop_mode, const guint channels) {
  return(BT_WAVE(g_object_new(BT_TYPE_WAVE,"song",song,"name",name,"uri",uri,"index",index,"volume",volume,"loop-mode",loop_mode,"channels",channels,NULL)));
}

//-- private methods

//-- public methods

/**
 * bt_wave_add_wavelevel:
 * @self: the wavetable to add the new wavelevel to
 * @wavelevel: the new wavelevel instance
 *
 * Add the supplied wavelevel to the wave. This is automatically done by
 * #bt_wavelevel_new().
 *
 * Returns: %TRUE for success, %FALSE otheriwse
 */
gboolean bt_wave_add_wavelevel(const BtWave * const self, const BtWavelevel * const wavelevel) {
  gboolean ret=FALSE;

  g_assert(BT_IS_WAVE(self));
  g_assert(BT_IS_WAVELEVEL(wavelevel));

  if(!g_list_find(self->priv->wavelevels,wavelevel)) {
    ret=TRUE;
    self->priv->wavelevels=g_list_append(self->priv->wavelevels,g_object_ref(G_OBJECT(wavelevel)));
    //g_signal_emit(G_OBJECT(self),signals[WAVELEVEL_ADDED_EVENT], 0, wavelevel);
    bt_song_set_unsaved(self->priv->song,TRUE);
  }
  else {
    GST_WARNING("trying to add wavelevel again");
  }
  return ret;
}

/**
 * bt_wave_get_level_by_index:
 * @self: the wave to search for the wavelevel
 * @index: the index of the wavelevel
 *
 * Search the wave for a wavelevel by the supplied index.
 * The wavelevel must have been added previously to this wave with bt_wave_add_wavelevel().
 * Unref the wavelevel, when done with it.
 *
 * Returns: #BtWavelevel instance or %NULL if not found
 */
BtWavelevel *bt_wave_get_level_by_index(const BtWave * const self,const gulong index) {
  BtWavelevel *wavelevel;
  
  if((wavelevel=g_list_nth_data(self->priv->wavelevels,index))) {
    return(g_object_ref(wavelevel));
  }
  return(NULL);
}

//-- io interface

static xmlNodePtr bt_wave_persistence_save(const BtPersistence * const persistence, const xmlNodePtr const parent_node) {
  const BtWave * const self = BT_WAVE(persistence);
  xmlNodePtr node=NULL;
  xmlNodePtr child_node;

  GST_DEBUG("PERSISTENCE::wave");

  if((node=xmlNewChild(parent_node,NULL,XML_CHAR_PTR("wave"),NULL))) {
    BtSongIONative *song_io;

    // we need to have a uri
    if(!self->priv->uri)
      self->priv->uri=g_strdup(self->priv->name);

    xmlNewProp(node,XML_CHAR_PTR("index"),XML_CHAR_PTR(bt_persistence_strfmt_ulong(self->priv->index)));
    xmlNewProp(node,XML_CHAR_PTR("name"),XML_CHAR_PTR(self->priv->name));
    xmlNewProp(node,XML_CHAR_PTR("uri"),XML_CHAR_PTR(self->priv->uri));
    xmlNewProp(node,XML_CHAR_PTR("volume"),XML_CHAR_PTR(bt_persistence_strfmt_double(self->priv->volume)));
    xmlNewProp(node,XML_CHAR_PTR("loop-mode"),XML_CHAR_PTR(bt_persistence_strfmt_enum(BT_TYPE_WAVE_LOOP_MODE,self->priv->loop_mode)));
    
    // check if we need to save external data
    g_object_get(self->priv->song,"song-io",&song_io,NULL);
    if (song_io) {
      if(BT_IS_SONG_IO_NATIVE_BZT(song_io)) {
        gchar *uri=NULL;

        if(self->priv->ext_fd==-1) {
          // need to write in memory data to a wav
          bt_wave_save_to_fd(self);
        }
        uri=g_strdup_printf("fd://%d",self->priv->ext_fd);
        bt_wave_save_from_fd(self,song_io,uri);
        g_free(uri);
      }
      g_object_unref(song_io);
    }

    // save wavelevels
    if((child_node=xmlNewChild(node,NULL,XML_CHAR_PTR("wavelevels"),NULL))) {
      bt_persistence_save_list(self->priv->wavelevels,child_node);
    }
  }
  return(node);
}

static BtPersistence *bt_wave_persistence_load(const GType type, const BtPersistence * const persistence, xmlNodePtr node, GError **err, va_list var_args) {
  BtWave *self;
  BtPersistence *result;
  BtSongIONative *song_io;
  gchar *uri=NULL;

  GST_DEBUG("PERSISTENCE::wave");
  g_assert(node);

  xmlChar * const index_str=xmlGetProp(node,XML_CHAR_PTR("index"));
  const gulong index=index_str?atol((char *)index_str):0;
  xmlChar * const name=xmlGetProp(node,XML_CHAR_PTR("name"));
  xmlChar * const uri_str=xmlGetProp(node,XML_CHAR_PTR("uri"));
  xmlChar * const volume_str=xmlGetProp(node,XML_CHAR_PTR("volume"));
  const gdouble volume=volume_str?g_ascii_strtod((char *)volume_str,NULL):0.0;
  xmlChar * const loop_mode_str=xmlGetProp(node,XML_CHAR_PTR("loop-mode"));
  gint loop_mode=bt_persistence_parse_enum(BT_TYPE_WAVE_LOOP_MODE,(char *)loop_mode_str);
  if(loop_mode==-1) loop_mode=BT_WAVE_LOOP_MODE_OFF;
  
  if(!persistence) {
    BtSong *song=NULL;
    gchar *param_name;

    // we need to get parameters from var_args (need to handle all baseclass params
    param_name=va_arg(var_args,gchar*);
    while(param_name) {
      if(!strcmp(param_name,"song")) {
        song=va_arg(var_args, gpointer);
      }
      else {
        GST_WARNING("unhandled argument: %s",param_name);
        break;
      }
      param_name=va_arg(var_args,gchar*);
    }
    
    /* @todo: ugly hack, don't pass uri_str yet, this would trigger loading
     * which we do below (see also: bt_wave_constructed) */
    self=bt_wave_new(song,(gchar*)name,NULL,index,volume,loop_mode,0);
    result=BT_PERSISTENCE(self);
    g_object_set(G_OBJECT(self),"uri",uri_str,NULL);
  }
  else {
    self=BT_WAVE(persistence);
    result=BT_PERSISTENCE(self);

    g_object_set(G_OBJECT(self),"index",index,"name",name,"uri",uri_str,"volume",volume,"loop-mode",loop_mode,NULL);
  }   
  
  xmlFree(index_str);
  xmlFree(name);
  xmlFree(volume_str);
  xmlFree(loop_mode_str);

  // check if we need to load external data
  g_object_get(self->priv->song,"song-io",&song_io,NULL);
  if (song_io) {
    gboolean unpack_failed=FALSE;
    if(BT_IS_SONG_IO_NATIVE_BZT(song_io)) {
      gchar *fn,*fp;

      // need to rewrite path
      if(!(fn=strrchr((gchar *)uri_str,'/')))
        fn=(gchar *)uri_str;
      else fn++;
      fp=g_strdup_printf("wavetable/%s",fn);

      GST_INFO("loading external uri=%s -> zip=%s",(gchar *)uri_str,fp); 
      
      // we need to copy the files from zip and change the uri to "fd://%d"
      self->priv->ext_fd=fileno(tmpfile()); // or mkstemp("...XXXXXX")
      if(bt_song_io_native_bzt_copy_to_fd(BT_SONG_IO_NATIVE_BZT(song_io),fp,self->priv->ext_fd)) {
        uri=g_strdup_printf("fd://%d",self->priv->ext_fd);
      }
      else {
        close(self->priv->ext_fd);
        self->priv->ext_fd=-1;
        unpack_failed=TRUE;
      }
      g_free(fp);
    }
    else {
      uri=g_strdup((gchar *)uri_str);
    }
    g_object_unref(song_io);
    if(unpack_failed) {
      goto WaveUnpackError;
    }
  }
  else {
    uri=g_strdup((gchar *)uri_str);
  }

  // try to load wavedata
  if(!bt_wave_load_from_uri(self,uri)) {
    goto WaveLoadingError;
  }
  else {
    GList *lnode=self->priv->wavelevels;
  
    for(node=node->children;node;node=node->next) {
      if((!xmlNodeIsText(node)) && (!strncmp((gchar *)node->name,"wavelevel\0",10))) {
        /* loading the wave already created wave-levels,
         * here we just want to override e.g. loop, sampling-rate
         */
        if(lnode) {
          BtWavelevel * const wave_level=BT_WAVELEVEL(lnode->data);

          bt_persistence_load(BT_TYPE_WAVELEVEL,BT_PERSISTENCE(wave_level),node,NULL,NULL);
          lnode=g_list_next(lnode);
        }
      }
    }
  }

Done:
  g_free(uri);
  xmlFree(uri_str);
  return(result);
WaveUnpackError:
  GST_WARNING("Failed to unpack wave %lu, uri='%s'",index,uri_str);
  if(err) {
    g_set_error(err, error_domain, /* errorcode= */0,
             "Failed to unpack wave %lu, uri='%s'",index,uri_str);
  }
  goto Done;
WaveLoadingError:
  GST_WARNING("Failed to load wave %lu, uri='%s'",index,uri);
  if(err) {
    g_set_error(err, error_domain, /* errorcode= */0,
             "Failed to load wave %lu, uri='%s'",index,uri);
  }
  goto Done;
}

static void bt_wave_persistence_interface_init(gpointer const g_iface, gpointer const iface_data) {
  BtPersistenceInterface * const iface = g_iface;

  iface->load = bt_wave_persistence_load;
  iface->save = bt_wave_persistence_save;
}

//-- wrapper

//-- g_object overrides

static void bt_wave_constructed(GObject *object) {
  BtWave *self=BT_WAVE(object);
  gboolean okay=TRUE;

  if(G_OBJECT_CLASS(parent_class)->constructed)
    G_OBJECT_CLASS(parent_class)->constructed(object);

  g_return_if_fail(BT_IS_SONG(self->priv->song));

  // some SongIO loaders load the wave themself and set the data 
  if(self->priv->uri) {
    // @todo: move stuff from bt_wave_persistence_load() into bt_wave_load_from_uri()
    // try to load wavedata
    if(!bt_wave_load_from_uri(self,self->priv->uri)) {
      GST_WARNING("Can't load wavedata from %s",self->priv->uri);
      okay=FALSE;
    }
  }
  if(okay) {
    BtWavetable *wavetable;
    // add the wave to the wavetable of the song
    g_object_get(G_OBJECT(self->priv->song),"wavetable",&wavetable,NULL);
    bt_wavetable_add_wave(wavetable,self);
    g_object_unref(wavetable);
  }
}

/* returns a property for the given property_id for this object */
static void bt_wave_get_property(GObject * const object, const guint property_id, GValue * const value, GParamSpec * const pspec) {
  const BtWave * const self = BT_WAVE(object);
  return_if_disposed();
  switch (property_id) {
    case WAVE_SONG: {
      g_value_set_object(value, self->priv->song);
    } break;
    case WAVE_WAVELEVELS: {
      g_value_set_pointer(value,g_list_copy(self->priv->wavelevels));
    } break;
    case WAVE_INDEX: {
      g_value_set_ulong(value, self->priv->index);
    } break;
    case WAVE_NAME: {
      g_value_set_string(value, self->priv->name);
    } break;
    case WAVE_URI: {
      g_value_set_string(value, self->priv->uri);
    } break;
    case WAVE_VOLUME: {
      g_value_set_double(value, self->priv->volume);
    } break;
    case WAVE_LOOP_MODE: {
      g_value_set_enum(value, self->priv->loop_mode);
    } break;    
    case WAVE_CHANNELS: {
      g_value_set_uint(value, self->priv->channels);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_wave_set_property(GObject * const object, const guint property_id, const GValue * const value, GParamSpec * const pspec) {
  const BtWave * const self = BT_WAVE(object);
  return_if_disposed();
  switch (property_id) {
    case WAVE_SONG: {
      self->priv->song = BT_SONG(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->song);
      //GST_DEBUG("set the song for wave: %p",self->priv->song);
    } break;
    case WAVE_INDEX: {
      self->priv->index = g_value_get_ulong(value);
      GST_DEBUG("set the index for wave: %lu",self->priv->index);
    } break;
    case WAVE_NAME: {
      g_free(self->priv->name);
      self->priv->name = g_value_dup_string(value);
      GST_DEBUG("set the name for wave: %s",self->priv->name);
    } break;
    case WAVE_URI: {
      g_free(self->priv->uri);
      self->priv->uri = g_value_dup_string(value);
      GST_DEBUG("set the uri for wave: %s",self->priv->uri);
    } break;
    case WAVE_VOLUME: {
      self->priv->volume = g_value_get_double(value);
    } break;
    case WAVE_LOOP_MODE: {
      self->priv->loop_mode = g_value_get_enum(value);
    } break;    
    case WAVE_CHANNELS: {
      self->priv->channels = g_value_get_uint(value);
      GST_DEBUG("set the channels for wave: %d",self->priv->channels);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_wave_dispose(GObject * const object) {
  const BtWave * const self = BT_WAVE(object);
  GList* node;

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);

  g_object_try_weak_unref(self->priv->song);
  // unref list of wavelevels
  if(self->priv->wavelevels) {
    for(node=self->priv->wavelevels;node;node=g_list_next(node)) {
      {
        const GObject * const obj=node->data;
        GST_DEBUG("  free wavelevels : %p (%d)",obj,obj->ref_count);
      }
      g_object_try_unref(node->data);
      node->data=NULL;
    }
  }
  
  wave_loader_free(self);

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_wave_finalize(GObject * const object) {
  const BtWave * const self = BT_WAVE(object);

  GST_DEBUG("!!!! self=%p",self);

  // free list of wavelevels
  if(self->priv->wavelevels) {
    g_list_free(self->priv->wavelevels);
    self->priv->wavelevels=NULL;
  }
  g_free(self->priv->name);
  g_free(self->priv->uri);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

//-- class internals

static void bt_wave_init(GTypeInstance * const instance, gconstpointer const g_class) {
  BtWave * const self = BT_WAVE(instance);

  self->priv=G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_WAVE, BtWavePrivate);
  self->priv->volume=1.0;
  self->priv->fd=-1;
  self->priv->ext_fd=-1;
}

static void bt_wave_class_init(BtWaveClass * const klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);

  // @idea: g_type_qname(BT_TYPE_WAVE);
  error_domain=g_quark_from_static_string("BtWave");

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtWavePrivate));

  gobject_class->constructed  = bt_wave_constructed;
  gobject_class->set_property = bt_wave_set_property;
  gobject_class->get_property = bt_wave_get_property;
  gobject_class->dispose      = bt_wave_dispose;
  gobject_class->finalize     = bt_wave_finalize;
  
  /**
   * BtWave::loading-done:
   * @self: the setup object that emitted the signal
   * @success: the result
   *
   * Loading the sample has finished with @result.
   */
  signals[LOADING_DONE_EVENT] = g_signal_new("loading-done",
                                        G_TYPE_FROM_CLASS(klass),
                                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                                        0,
                                        NULL, // accumulator
                                        NULL, // acc data
                                        g_cclosure_marshal_VOID__BOOLEAN,
                                        G_TYPE_NONE, // return type
                                        1, // n_params
                                        G_TYPE_BOOLEAN // param data
                                        );
  

  g_object_class_install_property(gobject_class,WAVE_SONG,
                                  g_param_spec_object("song",
                                     "song contruct prop",
                                     "Set song object, the wave belongs to",
                                     BT_TYPE_SONG, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,WAVE_WAVELEVELS,
                                  g_param_spec_pointer("wavelevels",
                                     "wavelevels list prop",
                                     "A copy of the list of wavelevels",
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,WAVE_INDEX,
                                  g_param_spec_ulong("index",
                                     "index prop",
                                     "The index of the wave in the wavtable",
                                     0,
                                     G_MAXULONG,
                                     0, /* default value */
                                     G_PARAM_CONSTRUCT|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,WAVE_NAME,
                                  g_param_spec_string("name",
                                     "name prop",
                                     "The name of the wave",
                                     "unamed wave", /* default value */
                                     G_PARAM_CONSTRUCT|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,WAVE_URI,
                                  g_param_spec_string("uri",
                                     "uri prop",
                                     "The uri of the wave",
                                     NULL, /* default value */
                                     G_PARAM_CONSTRUCT|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,WAVE_VOLUME,
                                  g_param_spec_double("volume",
                                     "volume prop",
                                     "The volume of the wave in the wavtable",
                                     0,
                                     1.0,
                                     1.0, /* default value */
                                     G_PARAM_CONSTRUCT|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,WAVE_LOOP_MODE,
                                  g_param_spec_enum("loop-mode",
                                     "loop-mode prop",
                                     "mode of loop playback",
                                     BT_TYPE_WAVE_LOOP_MODE,  /* enum type */
                                     BT_WAVE_LOOP_MODE_OFF, /* default value */
                                     G_PARAM_CONSTRUCT|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,WAVE_CHANNELS,
                                  g_param_spec_uint("channels",
                                     "channels prop",
                                     "number of channels in the sample",
                                     0,
                                     2,
                                     0,
                                     G_PARAM_CONSTRUCT|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));
}

GType bt_wave_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      sizeof(BtWaveClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_wave_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof(BtWave),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_wave_init, // instance_init
      NULL // value_table
    };
    const GInterfaceInfo persistence_interface_info = {
      (GInterfaceInitFunc) bt_wave_persistence_interface_init,  // interface_init
      NULL, // interface_finalize
      NULL  // interface_data
    };
    type = g_type_register_static(G_TYPE_OBJECT,"BtWave",&info,0);
    g_type_add_interface_static(type, BT_TYPE_PERSISTENCE, &persistence_interface_info);
  }
  return type;
}
