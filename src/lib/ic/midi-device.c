/* $Id$
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
 * SECTION:bticmididevice
 * @short_description: buzztards interaction controller midi device
 *
 * Event handling for midi devices.
 */
/*
 * use sysex to get device ids
 * http://www.borg.com/~jglatt/tech/midispec/identity.htm
 * http://en.wikipedia.org/wiki/MIDI_Machine_Control#Identity_Request
 *
 * to test midi inputs
 * amidi -l
 * amidi -d -p hw:2,0,0
 */
#define BTIC_CORE
#define BTIC_MIDI_DEVICE_C

#include "ic_private.h"

enum {
  DEVICE_DEVNODE=1,
  DEVICE_CONTROLCHANGE
};

struct _BtIcMidiDevicePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  gchar *devnode;

  /* io channel */
  GIOChannel *io_channel;
  gint io_source;

  /* learn-mode members */
  gboolean learn_mode;
  gulong learn_key;
  gchar *control_change;

  /* type|code -> control lookup */
  GHashTable *controls;
};

static GObjectClass *parent_class=NULL;

// forward declarations
static gboolean btic_midi_device_start(gconstpointer _self);
static gboolean btic_midi_device_stop(gconstpointer _self);
static void btic_midi_device_set_property(GObject      * const object,
					  const guint          property_id,
					  const GValue * const value,
					  GParamSpec   * const pspec);
//-- helper

//-- handler

static gboolean io_handler(GIOChannel *channel,GIOCondition condition,gpointer user_data) {
  BtIcMidiDevice *self=BTIC_MIDI_DEVICE(user_data);
  BtIcControl *control;
  GError *error=NULL;
  gsize bytes_read;
  guchar midi_event[3];
  gulong key;
  gboolean res=TRUE;

  if(condition & (G_IO_IN | G_IO_PRI)) {
    g_io_channel_read_chars(self->priv->io_channel, (gchar *)midi_event, 1, &bytes_read, &error);
    if(error) {
      GST_WARNING("iochannel error when reading: %s",error->message);
      g_error_free(error);
      //res=FALSE;
    }
    else {
      //GST_DEBUG( "midi event: %2x %2x %2x", midi_event[0], midi_event[1], midi_event[2] );
      if( midi_event[0] == 0xb0 ) { // control event
        g_io_channel_read_chars(self->priv->io_channel, (gchar *)&midi_event[1], 2, &bytes_read, &error);
        if(error) {
          GST_WARNING("iochannel error when reading: %s",error->message);
          g_error_free(error);
          //res=FALSE;
        }
        else {
          GST_DEBUG( "midi event: %02x %02x %02x", midi_event[0], midi_event[1], midi_event[2] );

          key=(gulong)midi_event[1];

          if( self->priv->learn_mode == TRUE ) {
            if(self->priv->learn_key!=key) {
              gchar control_change_string[18];

              sprintf( control_change_string, "midi-control %ld", key );
              self->priv->learn_key=key;
              g_object_set(self,"device-controlchange",control_change_string,NULL);
            }
          }

          if((control=(BtIcControl *)g_hash_table_lookup(self->priv->controls,GUINT_TO_POINTER(key)))) {
            g_object_set(control,"value",midi_event[2],NULL);

          }
        }
      }
    }
  }
  if(condition & (G_IO_HUP | G_IO_ERR | G_IO_NVAL)) {
    res=FALSE;
  }
  if(!res) {
    GST_INFO("closing connection");
    self->priv->io_source=-1;
  }
  return(res);
}


//-- constructor methods

/**
 * btic_midi_device_new:
 * @udi: the udi of the device
 * @name: human readable name
 * @devnode: device node in filesystem
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtIcMidiDevice *btic_midi_device_new(const gchar *udi,const gchar *name,const gchar *devnode) {
  BtIcMidiDevice *self=BTIC_MIDI_DEVICE(g_object_new(BTIC_TYPE_MIDI_DEVICE,"udi",udi,"name",name,"devnode",devnode,NULL));
  if(!self) {
    goto Error;
  }
  return(self);
Error:
  g_object_try_unref(self);
  return(NULL);
}

//-- methods

static gboolean btic_midi_device_learn_start(gconstpointer _self) {
  BtIcMidiDevice *self=BTIC_MIDI_DEVICE(_self);

  self->priv->learn_mode=TRUE;
  btic_device_start(BTIC_DEVICE(self));

  return(TRUE);
}

static gboolean btic_midi_device_learn_stop(gconstpointer _self) {
  BtIcMidiDevice *self=BTIC_MIDI_DEVICE(_self);

  self->priv->learn_mode=FALSE;
  btic_device_stop(BTIC_DEVICE(self));

  return(TRUE);
}

static BtIcControl* btic_midi_device_register_learned_control(gconstpointer _self,
							      const gchar *name)
{
  BtIcAbsRangeControl *control=NULL;
  BtIcMidiDevice *self=BTIC_MIDI_DEVICE(_self);

  GST_INFO("registering midi control as %s", name);

  if( g_hash_table_lookup(self->priv->controls,GUINT_TO_POINTER(self->priv->learn_key)) == NULL )
  {
    control = btic_abs_range_control_new(BTIC_DEVICE(self),name,
					 0, 127, 0);
    g_hash_table_insert(self->priv->controls,GUINT_TO_POINTER(self->priv->learn_key),(gpointer)control);
  }

  return(BTIC_CONTROL(control));
}

static gboolean btic_midi_device_start(gconstpointer _self) {
  BtIcMidiDevice *self=BTIC_MIDI_DEVICE(_self);
  GError *error=NULL;

  GST_INFO( "starting the midi device" );

  // start the io-loop
  self->priv->io_channel=g_io_channel_new_file(self->priv->devnode,"r",&error);
  if(error) {
    GST_WARNING("iochannel error for open(%s): %s",self->priv->devnode,error->message);
    g_error_free(error);
    return(FALSE);
  }
  g_io_channel_set_encoding(self->priv->io_channel,NULL,&error);
  if(error) {
    GST_WARNING("iochannel error for settin encoding to NULL: %s",error->message);
    g_error_free(error);
    g_io_channel_unref(self->priv->io_channel);
    self->priv->io_channel=NULL;
    return(FALSE);
  }
/*  g_io_channel_set_flags(self->priv->io_channel,
			 g_io_channel_get_flags(self->priv->io_channel)|G_IO_FLAG_NONBLOCK,
			 &error);
  if(error) {
    GST_WARNING("iochannel error for settin to non-blocking read: %s",error->message);
    g_error_free(error);
    g_io_channel_unref(self->priv->io_channel);
    self->priv->io_channel=NULL;
    return(FALSE);
    }*/
  self->priv->io_source=g_io_add_watch_full(self->priv->io_channel,
        G_PRIORITY_LOW,
        G_IO_IN|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL,
        io_handler,
        (gpointer)self,
        NULL);

  return(TRUE);
}

static gboolean btic_midi_device_stop(gconstpointer _self) {
  BtIcMidiDevice *self=BTIC_MIDI_DEVICE(_self);

  // stop the io-loop
  if(self->priv->io_channel) {
    if(self->priv->io_source>=0) {
      g_source_remove(self->priv->io_source);
      self->priv->io_source=-1;
    }
    g_io_channel_unref(self->priv->io_channel);
    self->priv->io_channel=NULL;
  }
  return(TRUE);
}

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void btic_midi_device_get_property(GObject * const object, const guint property_id, GValue * const value, GParamSpec * const pspec) {
  const BtIcMidiDevice * const self = BTIC_MIDI_DEVICE(object);
  return_if_disposed();
  switch (property_id) {
    case DEVICE_DEVNODE: {
      g_value_set_string(value, self->priv->devnode);
    } break;
    case DEVICE_CONTROLCHANGE: {
      g_value_set_string(value, self->priv->control_change);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void btic_midi_device_set_property(GObject * const object, const guint property_id, const GValue * const value, GParamSpec * const pspec) {
  const BtIcMidiDevice * const self = BTIC_MIDI_DEVICE(object);
  return_if_disposed();
  switch (property_id) {
    case DEVICE_DEVNODE: {
      self->priv->devnode = g_value_dup_string(value);
    } break;
    case DEVICE_CONTROLCHANGE: {
      g_free(self->priv->control_change);
      self->priv->control_change=g_value_dup_string(value);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void btic_midi_device_dispose(GObject * const object) {
  const BtIcMidiDevice * const self = BTIC_MIDI_DEVICE(object);

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p, self->ref_ct=%d",self,G_OBJECT(self)->ref_count);

  GST_DEBUG("  chaining up");
  G_OBJECT_CLASS(parent_class)->dispose(object);
  GST_DEBUG("  done");
}

static void btic_midi_device_finalize(GObject * const object) {
  const BtIcMidiDevice * const self = BTIC_MIDI_DEVICE(object);

  GST_DEBUG("!!!! self=%p",self);

  g_free(self->priv->devnode);
  g_hash_table_destroy(self->priv->controls);

  GST_DEBUG("  chaining up");
  G_OBJECT_CLASS(parent_class)->finalize(object);
  GST_DEBUG("  done");
}

static void btic_midi_device_init(const GTypeInstance * const instance, gconstpointer const g_class) {
  BtIcMidiDevice * const self = BTIC_MIDI_DEVICE(instance);

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BTIC_TYPE_MIDI_DEVICE, BtIcMidiDevicePrivate);

  self->priv->controls=g_hash_table_new_full(NULL,NULL,NULL,(GDestroyNotify)g_object_unref);
}

static void btic_midi_device_class_init(BtIcMidiDeviceClass * const klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);
  BtIcDeviceClass * const bticdevice_class = BTIC_DEVICE_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtIcMidiDevicePrivate));

  gobject_class->set_property = btic_midi_device_set_property;
  gobject_class->get_property = btic_midi_device_get_property;
  gobject_class->dispose      = btic_midi_device_dispose;
  gobject_class->finalize     = btic_midi_device_finalize;

  bticdevice_class->start     = btic_midi_device_start;
  bticdevice_class->stop      = btic_midi_device_stop;

  g_object_class_install_property(gobject_class,DEVICE_DEVNODE,
                                  g_param_spec_string("devnode",
                                     "devnode prop",
                                     "device node path",
                                     NULL, /* default value */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  // override learn interface
  g_object_class_override_property(gobject_class,DEVICE_CONTROLCHANGE,"device-controlchange");
}

static void btic_midi_device_interface_init (gpointer g_iface, gpointer iface_data) {
  BtIcLearnInterface *klass = (BtIcLearnInterface *)g_iface;
  klass->learn_start = btic_midi_device_learn_start;
  klass->learn_stop  = btic_midi_device_learn_stop;
  klass->register_learned_control = btic_midi_device_register_learned_control;
}

GType btic_midi_device_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      (guint16)(sizeof(BtIcMidiDeviceClass)),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)btic_midi_device_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      (guint16)(sizeof(BtIcMidiDevice)),
      0,   // n_preallocs
      (GInstanceInitFunc)btic_midi_device_init, // instance_init
      NULL // value_table
    };

    static const GInterfaceInfo learn_info = {
      (GInterfaceInitFunc) btic_midi_device_interface_init,    /* interface_init */
      NULL,               /* interface_finalize */
      NULL                /* interface_data */
    };

    type = g_type_register_static(BTIC_TYPE_DEVICE,"BtIcMidiDevice",&info,0);
    g_type_add_interface_static (type,
                                 BTIC_TYPE_LEARN,
                                 &learn_info);
  }
  return type;
}
