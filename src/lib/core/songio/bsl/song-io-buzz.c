/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@buzztrax.org>
 *
 * class for buzz song input and output
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
/**
 * SECTION:btsongiobuzz
 * @short_description: class for song input in buzz bmx and bmw format
 *
 * This #BtSongIO plugin implements loading and of songs made using Buzz. Both
 * songs with and without embedded waveforms are supported. Most aspects of the
 * file-format are implemented.
 */
/* TODO(ensonic): pdlg and midi chunks
 */

#define BT_SONG_IO_BUZZ_C

#include "bsl.h"
#include "song-io-buzz-private.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
//-- glib/gobject
#include <glib/gprintf.h>
//-- gstbuzztrax
#include "gst/toneconversion.h"
#include "gst/propertymeta.h"

struct _BtSongIOBuzzPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the buzz song file handle */
  gchar *data;
  gsize data_length;
  gulong data_pos;
  gboolean io_error;

  guint32 number_of_sections;
  BmxSectionEntry *entries;

  guint32 number_of_machines;
  BmxMachSection *mach_section;
  BmxParaSection *para_section;
};

//-- the class

G_DEFINE_TYPE_WITH_CODE (BtSongIOBuzz, bt_song_io_buzz, BT_TYPE_SONG_IO, 
    G_ADD_PRIVATE(BtSongIOBuzz));


typedef enum
{
  PT_NOTE = 0,
  PT_SWITCH,
  PT_BYTE,
  PT_WORD,
  PT_ENUM,
  PT_ATTR
} GstBMLParameterTypes;

typedef enum
{
  PF_WAVE = (1 << 0),
  PF_STATE = (1 << 1),
  PF_TICK_ON_EDIT = (1 << 2)
} GstBMLParameterFlags;


typedef enum
{
  MT_MASTER = 0,
  MT_GENERATOR,
  MT_EFFECT
} GstBMLMachineTypes;

//-- memory io helper methods

static gint
mem_seek (const BtSongIOBuzz * self, glong offset, gint mode)
{
  switch (mode) {
    case SEEK_SET:
      if (offset <= self->priv->data_length) {
        self->priv->data_pos = offset;
        return 0;
      }
      break;
    case SEEK_CUR:{
      glong new_pos = (glong) self->priv->data_pos + offset;
      if (new_pos > 0 && new_pos <= self->priv->data_length) {
        self->priv->data_pos = new_pos;
        return 0;
      }
      break;
    }
  }
  errno = ESPIPE;
  return -1;
}

static gsize
mem_read (const BtSongIOBuzz * self, gpointer ptr, gsize size, gsize n_items)
{
  gsize bytes = size * n_items;

  if (self->priv->data_pos + bytes <= self->priv->data_length) {
    memcpy (ptr, &self->priv->data[self->priv->data_pos], bytes);
    self->priv->data_pos += bytes;
    return n_items;
  }
  errno = EIO;
  return 0;
}

//-- wave decompression methods

typedef struct
{
  guint16 sum1;
  guint16 sum2;
  guint16 result;

  guint16 *temp;
} CompressionValues;

#define MAXPACKEDBUFFER 2048

typedef struct
{
  guint8 packed_buf[MAXPACKEDBUFFER];
  guint32 cur_index;
  guint32 cur_bit;

  guint32 bytes_in_buffer;
  guint32 max_bytes;
  guint32 bytes_in_file_remain;

  gboolean error;

  const BtSongIOBuzz *self;
} CompressionCtx;

static CompressionCtx ctxt = { {0,}, 0 };

static guint32
unpack_bits (guint32 amount)
{
  guint32 ret = 0, shift = 0;
  guint32 size, mask, val;
  guint32 amount_needed, amount_got;
  guint32 max_bits = 8;

  GST_LOG ("unpack_bits(%d)", amount);

  if ((ctxt.bytes_in_file_remain == 0) && (ctxt.cur_index == MAXPACKEDBUFFER)) {
    GST_WARNING ("unpack_bits().1 = 0 : eof");
    ctxt.error = TRUE;
    return 0;
  }

  while (amount > 0) {
    //check to see if we need to update buffer and/or index
    if ((ctxt.cur_bit == max_bits) || (ctxt.bytes_in_buffer == 0)) {
      ctxt.cur_bit = 0;
      ctxt.cur_index++;
      if (ctxt.cur_index >= ctxt.bytes_in_buffer) {
        //run out of buffer... read more file into buffer
        amount_needed =
            (ctxt.bytes_in_file_remain >
            ctxt.max_bytes) ? ctxt.max_bytes : ctxt.bytes_in_file_remain;

        amount_got = mem_read (ctxt.self, ctxt.packed_buf, 1, amount_needed);
        GST_LOG ("reading %u bytes at pos %ld and got %u bytes",
            amount_needed, ctxt.self->priv->data_pos, amount_got);

        ctxt.bytes_in_file_remain -= amount_got;
        ctxt.bytes_in_buffer = amount_got;
        ctxt.cur_index = 0;

        //if we didnt read anything then exit now
        if (amount_got == 0) {
          //make sure nothing else is read
          ctxt.bytes_in_file_remain = 0;
          ctxt.cur_index = MAXPACKEDBUFFER;
          ctxt.error = TRUE;
          if (amount_needed == 0) {
            GST_WARNING
                ("got 0 bytes, wanted 0 bytes, %u bytes in file remain, fpos %ld",
                ctxt.bytes_in_file_remain, ctxt.self->priv->data_pos);
          } else {
            GST_WARNING ("got 0 bytes, wanted %u bytes", amount_needed);
          }
          return 0;
        }
      }
    }
    //calculate size to read from current guint32
    size =
        ((amount + ctxt.cur_bit) > max_bits) ? max_bits - ctxt.cur_bit : amount;

    //calculate bitmask
    mask = (1 << size) - 1;

    //Read value from buffer
    val = ctxt.packed_buf[ctxt.cur_index];
    val = val >> ctxt.cur_bit;

    //apply mask to value
    val &= mask;

    //shift value to correct position
    val = val << shift;

    //update return value
    ret |= val;

    //update info
    ctxt.cur_bit += size;
    shift += size;
    amount -= size;
  }
  GST_LOG ("unpack_bits() = %d", ret);
  return ret;
}

static guint32
count_zero_bits (void)
{
  guint32 bit;
  guint32 count = 0;

  GST_LOG ("count_zero_bits()");

  bit = unpack_bits (1);
  while ((bit == 0) && (!ctxt.error)) {
    count++;
    bit = unpack_bits (1);
  }
  GST_LOG ("count_zero_bits() = %u", count);
  return count;
}

static void
init_compression_values (CompressionValues * cv, guint32 block_size)
{
  cv->result = 0;
  cv->sum1 = 0;
  cv->sum2 = 0;

  //If block size is given, then allocate specfied temporary data
  if (block_size > 0) {
    cv->temp = g_new (guint16, block_size);
  } else {
    cv->temp = NULL;
  }
}

static void
free_compression_values (CompressionValues * cv)
{
  //if there is temporary data - then free it.
  g_free (cv->temp);
  cv->temp = NULL;
}

static gboolean
decompress_samples (CompressionValues * cv, guint16 * outbuf,
    guint32 block_size)
{
  guint32 switch_value, bits, size, zero_count;
  guint32 val;

  GST_LOG ("  decompress_samples(ptr=%p,size=%d)", outbuf, block_size);

  if (!block_size)
    return FALSE;

  //Get compression method
  switch_value = unpack_bits (2);

  //read size (in bits) of compressed values
  bits = unpack_bits (4);

  size = block_size;
  while ((size > 0) && (!ctxt.error)) {
    //read compressed value
    val = (guint16) unpack_bits (bits);

    //count zeros
    zero_count = count_zero_bits ();

    //construct
    val = (guint16) ((zero_count << bits) | val);

    //is value supposed to be positive or negative?
    if ((val & 1) == 0) {       //its positive
      val = val >> 1;
    } else {                    //its negative. Convert into a negative value.
      val++;
      val = val >> 1;
      val = ~val;               //invert bits
      val++;                    //add one to make 2's compliment
    }

    //Now do stuff depending on which method we're using....
    switch (switch_value) {
      case 0:
        cv->sum2 = ((val - cv->result) - cv->sum1);
        cv->sum1 = val - cv->result;
        cv->result = val;
        break;
      case 1:
        cv->sum2 = val - cv->sum1;
        cv->sum1 = val;
        cv->result += val;
        break;
      case 2:
        cv->sum2 = val;
        cv->sum1 += val;
        cv->result += cv->sum1;
        break;
      case 3:
        cv->sum2 += val;
        cv->sum1 += cv->sum2;
        cv->result += cv->sum1;
        break;
      default:                 //error
        GST_INFO ("wrong switch value %d", switch_value);
        return FALSE;
    }

    //store value into output buffer and prepare for next loop...
    *outbuf++ = cv->result;
    size--;
  }
  GST_LOG ("decompress_samples() = %d", !ctxt.error);
  return !ctxt.error;
}

static gboolean
decompress_wave (guint16 * outbuf, guint32 num_samples, guint channels)
{
  guint32 zero_count, shift, block_size, last_block_size, num_blocks;
  guint32 result_shift, count, i, j;
  guint8 sum_channels;
  CompressionValues cv1, cv2;

  GST_DEBUG ("decomp samples:%d channels:%d", num_samples, channels);

  if (!outbuf)
    return FALSE;

  zero_count = count_zero_bits ();
  if (zero_count) {
    GST_WARNING ("Unknown wave data compression %d\n", zero_count);
    return FALSE;
  }
  //get size shifter
  shift = unpack_bits (4);

  //get size of compressed blocks
  block_size = 1 << shift;

  //get number of compressed blocks
  num_blocks = num_samples >> shift;

  //get size of last compressed block
  last_block_size = (block_size - 1) & num_samples;

  //get result shifter value (used to shift data after decompression)
  result_shift = unpack_bits (4);

  GST_DEBUG ("before decomp loop: "
      " shift = %u"
      " block_size = %u"
      " num_blocks = %u"
      " last_block_size = %u"
      " result_shift = %u",
      shift, block_size, num_blocks, last_block_size, result_shift);

  if (channels == 1) {          // mono handling
    //zero internal compression values
    init_compression_values (&cv1, 0);

    //If there's a remainder... then handle number of blocks + 1
    count = (last_block_size == 0) ? num_blocks : num_blocks + 1;
    while (count > 0) {
      //check to see if we are handling the last block
      if ((count == 1) && (last_block_size != 0)) {
        //we are... set block size to size of last block
        block_size = last_block_size;
      }

      if (!decompress_samples (&cv1, outbuf, block_size)) {
        GST_WARNING ("abort with %d remaining blocks", count);
        return FALSE;
      }

      if (result_shift) {
        for (i = 0; i < block_size; i++) {
          //shift end result
          outbuf[i] = outbuf[i] << result_shift;
        }
      }
      //proceed to next block...
      outbuf += block_size;
      count--;
    }
  } else {                      // stereo handling
    //read "channel sum" flag
    sum_channels = (guint8) unpack_bits (1);

    //zero internal compression values and alloc some temporary space
    init_compression_values (&cv1, block_size);
    init_compression_values (&cv2, block_size);

    //if there's a remainder... then handle number of blocks + 1
    count = (last_block_size == 0) ? num_blocks : num_blocks + 1;
    while (count > 0) {
      //check to see if we are handling the last block
      if ((count == 1) && (last_block_size != 0)) {
        //we are... set block size to size of last block
        block_size = last_block_size;
      }
      //decompress both channels into temporary area
      if ((!decompress_samples (&cv1, cv1.temp, block_size)) ||
          (!decompress_samples (&cv2, cv2.temp, block_size)))
        return FALSE;

      for (i = 0; i < block_size; i++) {
        //store channel 1 and apply result shift
        j = i * 2;
        outbuf[j] = cv1.temp[i] << result_shift;

        //store channel 2
        j++;
        outbuf[j] = cv2.temp[i];

        //if sum_channels flag is set then the 2nd channel is the sum of both channels
        if (sum_channels != 0) {
          outbuf[j] += cv1.temp[i];
        }
        //apply result shift to channel 2
        outbuf[j] = outbuf[j] << result_shift;
      }

      //proceed to next block
      outbuf += block_size * 2;
      count--;
    }

    //tidy
    free_compression_values (&cv1);
    free_compression_values (&cv2);
  }
  GST_DEBUG ("decomp done");
  return TRUE;
}

//-- io helper methods

static gchar *
read_section_name (const BtSongIOBuzz * self, gchar * section)
{
  static gchar empty[] = "";

  g_assert (section);

  return ((mem_read (self, section, sizeof (char), 4) == 4) ? section : empty);
}

static guint32
read_dword (const BtSongIOBuzz * self)
{
  guint32 buffer;

  gint r = mem_read (self, &buffer, sizeof (buffer), 1);
  if (r != 1) {
    GST_WARNING ("can't read from file : %d : %s", errno, strerror (errno));
    self->priv->io_error = TRUE;
  }
  return buffer;
}

static guint16
read_word (const BtSongIOBuzz * self)
{
  guint16 buffer;

  gint r = mem_read (self, &buffer, sizeof (buffer), 1);
  if (r != 1) {
    GST_WARNING ("can't read from file : %d : %s", errno, strerror (errno));
    self->priv->io_error = TRUE;
  }
  return buffer;
}

static guint8
read_byte (const BtSongIOBuzz * self)
{
  guint8 buffer;

  gint r = mem_read (self, &buffer, sizeof (buffer), 1);
  if (r != 1) {
    GST_WARNING ("can't read from file : %d : %s", errno, strerror (errno));
    self->priv->io_error = TRUE;
  }
  return buffer;
}

static gint
read_int (const BtSongIOBuzz * self)
{
  gint buffer;

  gint r = mem_read (self, &buffer, sizeof (buffer), 1);
  if (r != 1) {
    GST_WARNING ("can't read from file : %d : %s", errno, strerror (errno));
    self->priv->io_error = TRUE;
  }
  return buffer;
}

static gfloat
read_float (const BtSongIOBuzz * self)
{
  gfloat buffer;

  gint r = mem_read (self, &buffer, sizeof (buffer), 1);
  if (r != 1) {
    GST_WARNING ("can't read from file : %d : %s", errno, strerror (errno));
    self->priv->io_error = TRUE;
  }
  return buffer;
}

static gchar *
read_string (const BtSongIOBuzz * self, guint32 len)
{
  GST_INFO ("reading string of len=%u", (guint) len);

  if (!len) {
    return NULL;
  } else {
    gchar *buffer = g_new (gchar, len + 1);
    gchar *res;
    gint r = mem_read (self, buffer, len, 1);
    if (r != 1) {
      GST_WARNING ("can't read from file : %d : %s", errno, strerror (errno));
      self->priv->io_error = TRUE;
    }
    buffer[len] = '\0';
    res = g_convert (buffer, -1, "UTF-8", "WINDOWS-1252", NULL, NULL, NULL);
    g_free (buffer);
    return res;
  }
}

static gchar *
read_null_string (const BtSongIOBuzz * self, guint32 len)
{
  gchar *buffer = g_new (gchar, len + 1);
  gchar *ptr = buffer, *res = NULL;
  gint8 c;
  guint32 ct = 0;

  GST_INFO ("reading string of len=%u", (guint) len);

  do {
    c = read_byte (self);
    if (!self->priv->io_error) {
      *ptr++ = (gchar) c;
      ct++;
    }
  } while (!self->priv->io_error && c && ct < len);
  *ptr = '\0';
  if (ct) {
    GST_INFO ("read len=%u", (guint) len);
    res = g_convert (buffer, -1, "UTF-8", "WINDOWS-1252", NULL, NULL, NULL);
  }
  g_free (buffer);
  return res;
}

static BmxParameter *
read_parameters (const BtSongIOBuzz * self, guint32 number,
    guint32 max_str_size)
{
  BmxParameter *params, *param;
  guint j;

  if (!number)
    return NULL;

  params = g_new0 (BmxParameter, number);
  for (j = 0; ((j < number) && !self->priv->io_error); j++) {
    param = &params[j];
    param->type = read_byte (self);
    param->name = read_null_string (self, max_str_size);
    param->minvalue = read_int (self);
    param->maxvalue = read_int (self);
    param->novalue = read_int (self);
    param->flags = read_int (self);
    param->defvalue = read_int (self);
    GST_DEBUG ("%u : %s : min/max/no/def = %d,%d,%d,%d", j, param->name,
        param->minvalue, param->maxvalue, param->novalue, param->defvalue);
  }
  return params;
}

static BmxSectionEntry *
get_section_entry (const BtSongIOBuzz * self, const gchar * key)
{
  guint i = 0;
  BmxSectionEntry *entry = self->priv->entries;

  while (i < self->priv->number_of_sections) {
    if (!g_ascii_strncasecmp (entry->name, key, 4))
      break;
    i++;
    entry++;
  }
  return ((i < self->priv->number_of_sections) ? entry : NULL);
}

static GQuark gst_bml_property_meta_quark_type = 0;

static void
fill_machine_parameter (const BtSongIOBuzz * self, BmxParameter * param,
    GParamSpec * pspec, gboolean no_para)
{
  // this is hacky :/ (this one is only used in the bml plugin)
  if (G_UNLIKELY (!gst_bml_property_meta_quark_type)) {
    gst_bml_property_meta_quark_type =
        g_quark_from_string ("GstBMLPropertyMeta::type");
  }

  param->type =
      GPOINTER_TO_INT (g_param_spec_get_qdata (pspec,
          gst_bml_property_meta_quark_type));
  if (no_para) {
    GST_DEBUG ("file BmxParameter from machine qdata");
    param->minvalue =
        GPOINTER_TO_INT (g_param_spec_get_qdata (pspec,
            gstbt_property_meta_quark_min_val));
    param->maxvalue =
        GPOINTER_TO_INT (g_param_spec_get_qdata (pspec,
            gstbt_property_meta_quark_max_val));
    param->defvalue =
        GPOINTER_TO_INT (g_param_spec_get_qdata (pspec,
            gstbt_property_meta_quark_def_val));
    param->novalue =
        GPOINTER_TO_INT (g_param_spec_get_qdata (pspec,
            gstbt_property_meta_quark_no_val));
    param->flags = 0;
    if (pspec->flags & G_PARAM_READABLE) {
      param->flags |= GSTBT_PROPERTY_META_STATE;
    }
    if (pspec->value_type == GSTBT_TYPE_WAVE_INDEX) {
      param->flags |= GSTBT_PROPERTY_META_WAVE;
    }
  } else {
    gint val;
    GST_DEBUG ("sync BmxParameter with machine qdata");
    val =
        GPOINTER_TO_INT (g_param_spec_get_qdata (pspec,
            gstbt_property_meta_quark_min_val));
    if (val != param->minvalue) {
      if (param->minvalue != -1) {
        GST_WARNING
            ("file has diffent min-value for parameter %s, file=%d != machine=%d",
            param->name, param->minvalue, val);
      }
      param->minvalue = val;
    }
    val =
        GPOINTER_TO_INT (g_param_spec_get_qdata (pspec,
            gstbt_property_meta_quark_max_val));
    if (val != param->maxvalue) {
      if (param->maxvalue != -1) {
        GST_WARNING
            ("file has diffent max-value for parameter %s, file=%d != machine=%d",
            param->name, param->maxvalue, val);
      }
      param->maxvalue = val;
    }
    val =
        GPOINTER_TO_INT (g_param_spec_get_qdata (pspec,
            gstbt_property_meta_quark_def_val));
    if (val != param->defvalue) {
      GST_WARNING
          ("file has diffent def-value for parameter %s, file=%d != machine=%d",
          param->name, param->defvalue, val);
      param->defvalue = val;
    }
    val =
        GPOINTER_TO_INT (g_param_spec_get_qdata (pspec,
            gstbt_property_meta_quark_no_val));
    if (val != param->novalue) {
      GST_WARNING
          ("file has diffent no-value for parameter %s, file=%d != machine=%d",
          param->name, param->novalue, val);
      param->novalue = val;
    }
  }
}

static gboolean
registry_filter (GstPluginFeature * const feature, gpointer user_data)
{
  const gchar *name = (const gchar *) user_data;

  if (GST_IS_ELEMENT_FACTORY (feature)) {
    const gchar *long_name =
        gst_element_factory_get_metadata (GST_ELEMENT_FACTORY (feature),
        GST_ELEMENT_METADATA_LONGNAME);
    if (!strcasecmp (long_name, name)) {
      return TRUE;
    }

    const gchar *desc =
        gst_element_factory_get_metadata (GST_ELEMENT_FACTORY (feature),
        GST_ELEMENT_METADATA_DESCRIPTION);
    if (!strcasecmp (desc, name)) {
      return TRUE;
    }
  }
  return FALSE;
}

static const gchar *
find_machine_factory_by_name (const gchar * name)
{
  GList *list = gst_registry_feature_filter (gst_registry_get (),
      registry_filter, TRUE, (gpointer) name);
  if (list) {
    const gchar *factory_name = gst_plugin_feature_get_name (
        (GstPluginFeature *) (list->data));
    gst_plugin_feature_list_free (list);

    GST_WARNING ("name='%s' -> elem_name='%s'", name, factory_name);
    return factory_name;
  }
  return NULL;
}

//-- helper methods

static gboolean
read_section_table (const BtSongIOBuzz * self)
{
  gboolean result = TRUE;
  guint i;

  self->priv->number_of_sections = read_dword (self);
  GST_INFO ("song file has %d sections", self->priv->number_of_sections);
  // alocate section table
  self->priv->entries =
      g_new0 (BmxSectionEntry, self->priv->number_of_sections);
  // read n section entries (name, offset, size)
  for (i = 0; i < self->priv->number_of_sections; i++) {
    BmxSectionEntry *entry = &self->priv->entries[i];
    read_section_name (self, entry->name);
    entry->offset = read_dword (self);
    entry->size = read_dword (self);
    GST_INFO ("  section %2u : %c%c%c%c off=%d size=%d", i, entry->name[0],
        entry->name[1], entry->name[2], entry->name[3], entry->offset,
        entry->size);
  }
  return result;
}

static gboolean
read_bver_section (const BtSongIOBuzz * self, const BtSong * song)
{
  gboolean result = TRUE;
  BmxSectionEntry *entry = get_section_entry (self, "BVER");
  gchar *version;

  // this section is optional
  if (!entry)
    return TRUE;

  mem_seek (self, entry->offset, SEEK_SET);
  GST_INFO ("reading BVER section: 0x%x, %u", entry->offset, entry->size);
  version = read_null_string (self, entry->size);
  GST_INFO ("  version:\"%s\"", version);
  g_free (version);
  if (self->priv->io_error)
    result = FALSE;
  GST_INFO ("read BVER section %d: off+size=%u, pos=%lu", result,
      entry->offset + entry->size, self->priv->data_pos);
  return result;
}

static gboolean
read_para_section (const BtSongIOBuzz * self, const BtSong * song)
{
  gboolean result = TRUE;
  BmxSectionEntry *entry = get_section_entry (self, "PARA");
  guint32 i;

  // this section is optional
  if (!entry)
    return TRUE;

  mem_seek (self, entry->offset, SEEK_SET);
  GST_INFO ("reading PARA section: 0x%x, %u", entry->offset, entry->size);

  self->priv->number_of_machines = read_dword (self);
  GST_INFO ("  number of machines : %d", self->priv->number_of_machines);
  self->priv->para_section =
      g_new0 (BmxParaSection, self->priv->number_of_machines);

  for (i = 0; ((i < self->priv->number_of_machines) && !self->priv->io_error);
      i++) {
    BmxParaSection *para = &self->priv->para_section[i];

    para->name = read_null_string (self, entry->size);
    para->long_name = read_null_string (self, entry->size);
    para->number_of_global_params = read_dword (self);
    para->number_of_track_params = read_dword (self);

    GST_INFO ("  %u : \"%s\" \"%s\" : %d, %d", i, para->name, para->long_name,
        para->number_of_global_params, para->number_of_track_params);

    para->global_params =
        read_parameters (self, para->number_of_global_params, entry->size);
    para->track_params =
        read_parameters (self, para->number_of_track_params, entry->size);
  }
  if (self->priv->io_error)
    result = FALSE;
  GST_INFO ("read PARA section %d: off+size=%u, pos=%lu", result,
      entry->offset + entry->size, self->priv->data_pos);
  return result;
}

static gboolean
read_mach_section (const BtSongIOBuzz * self, const BtSong * song)
{
  gboolean result = TRUE;
  BmxSectionEntry *entry = get_section_entry (self, "MACH");
  guint32 i, j, k;
  gboolean no_para = FALSE;
  gchar *str;
  guint32 data_size, value;
  BtSetup *setup;
  BtMachine *machine;
  GstElement *element = NULL;
  GObjectClass *class = NULL;
  gchar *name;
  gchar plugin_name[256 + 5];   // a dll-name can't be more than 256 chars
  const gchar *elem_name;
  GError *err = NULL;

  if (!entry)
    return FALSE;

  g_object_get (G_OBJECT (song), "setup", &setup, NULL);

  mem_seek (self, entry->offset, SEEK_SET);
  GST_INFO ("reading MACH section: 0x%x, %u", entry->offset, entry->size);

  self->priv->number_of_machines = read_word (self);
  GST_INFO ("  number of machines : %d", self->priv->number_of_machines);
  self->priv->mach_section =
      g_new0 (BmxMachSection, self->priv->number_of_machines);
  if (!self->priv->para_section) {
    self->priv->para_section =
        g_new0 (BmxParaSection, self->priv->number_of_machines);
    no_para = TRUE;
  }

  for (i = 0; ((i < self->priv->number_of_machines) && !self->priv->io_error);
      i++) {
    BmxMachSection *mach = &self->priv->mach_section[i];
    BmxParaSection *para = &self->priv->para_section[i];
    gulong number_of_global_params, number_of_track_params;

    str = read_null_string (self, entry->size);
    mach->name =
        g_convert_with_fallback (str, -1, "ASCII", "UTF-8", "-", NULL, NULL,
        NULL);
    g_strstrip (mach->name);
    mach->type = read_byte (self);
    if ((mach->type == MT_GENERATOR) || (mach->type == MT_EFFECT)) {
      mach->dllname = read_null_string (self, entry->size);
    } else {
      mach->dllname = NULL;
    }

    mach->xpos = read_float (self);
    mach->ypos = read_float (self);

    GST_INFO ("  %d : \"%s\" %d -> \"%s\"  %f,%f", i, mach->name, mach->type,
        safe_string (mach->dllname), mach->xpos, mach->ypos);

    data_size = read_dword (self);
    if (data_size) {
      /* TODO(ensonic): this contains machine specific init data, see
       * FSM/Infector for a machine that can use this
       * we need to pass data_size + fileposition to bsl
       * g_object_set(machine, "blob-size", data_size, "blob-data", data, NULL);
       *
       * problem: we need this in gst_bml_(src|transform)_init, so it needs to
       * sent there somehow earlier :(
       * 1.) accessing bml directly ?
       *     bml_set_machine_data_input(data_size,data) ?
       * 2.) moving the code from _init to _constructed
       *
       * see aöso: gst-buzztrax/src/bml/util.c:gstbml_class_prepare_properties()
       */
      GST_INFO ("    skipping machine data : %d (0x%x) bytes", data_size,
          data_size);
      mem_seek (self, data_size, SEEK_CUR);
    }
    // create machine
    name = bt_setup_get_unique_machine_id (setup, mach->name);
    machine = NULL;
    if ((mach->type == MT_GENERATOR) || (mach->type == MT_EFFECT)) {
      // look for machine by factory->long_name
      GST_WARNING ("searching long_name='%s'", mach->dllname);
      if (!(elem_name = find_machine_factory_by_name (mach->dllname))) {
        gchar *ptr1, *ptr2;

        snprintf (plugin_name, sizeof(plugin_name), "bml-%s", mach->dllname);
        g_strcanon (plugin_name, G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS "-+",
            '-');

        ptr1 = ptr2 = plugin_name;
        // remove double '-'
        while (*ptr2) {
          if (*ptr2 == '-') {
            while (ptr2[1] == '-')
              ptr2++;
          }
          if (ptr1 != ptr2)
            *ptr1 = *ptr2;
          ptr1++;
          ptr2++;
        }
        if (ptr1 != ptr2)
          *ptr1 = '\0';
        // remove trailing '-'
        ptr1--;
        while (*ptr1 == '-')
          *ptr1++ = '\0';

        GST_DEBUG ("    name: %s, plugin_name: %s", name, plugin_name);
        GST_WARNING ("long_name='%s' not found, trying as elem_name",
            plugin_name);
        elem_name = plugin_name;
      }
    } else {
      plugin_name[0] = '\0';
      elem_name = "\0";
      GST_DEBUG ("    name: %s, plugin_name: -", name);
    }
    switch (mach->type) {
      case 0:
        machine = BT_MACHINE (bt_sink_machine_new (song, name, &err));
        break;
      case 1:
        machine = BT_MACHINE (bt_source_machine_new (song, name, elem_name,
                /*voices */ 1, &err));
        break;
      case 2:
        machine = BT_MACHINE (bt_processor_machine_new (song, name, elem_name,
                /*voices */ 1, &err));
        // some buzz src register as FX to be stereo, try again
        if (err) {
          GError *err2 = NULL;
          GST_WARNING ("failed to create processor machine '%s', try as source",
              plugin_name);
          machine = BT_MACHINE (bt_source_machine_new (song, name, plugin_name,
                  /*voices */ 1, &err2));
          if (err2) {
            GST_WARNING ("failed to create processor machine '%s' as source",
                plugin_name);
            g_error_free (err2);
          } else {
            mach->type = 1;
            g_error_free (err);
            err = NULL;
          }
        }
        break;
    }
    g_free (name);
    name = NULL;

    if (err == NULL) {
      g_object_get (machine,
          "global-params", &number_of_global_params,
          "voice-params", &number_of_track_params, NULL);
      if (no_para) {
        // take machine details from plugin
        if (mach->type) {
          para->number_of_global_params = number_of_global_params;
          para->number_of_track_params = number_of_track_params;
        } else {
          para->number_of_global_params = 3;
          para->number_of_track_params = 0;
        }
        GST_DEBUG ("    number of params : %d,%d",
            para->number_of_global_params, para->number_of_track_params);
        para->global_params =
            g_new0 (BmxParameter, para->number_of_global_params);
        para->track_params =
            g_new0 (BmxParameter, para->number_of_track_params);
      } else {
        if (number_of_global_params != para->number_of_global_params) {
          GST_WARNING
              ("file has diffent number of global parameters than machine %s, file=%u != machine=%lu",
              plugin_name, para->number_of_global_params,
              number_of_global_params);
        }
        if (number_of_track_params != para->number_of_track_params) {
          GST_WARNING
              ("file has diffent number of track parameters than machine %s, file=%u != machine=%lu",
              plugin_name, para->number_of_track_params,
              number_of_track_params);
        }
      }
      GST_DEBUG ("    number of params : %d,%d", para->number_of_global_params,
          para->number_of_track_params);
      // get machine types in all cases from plugin (as we map types)
      if (mach->type) {
        GParamSpec *pspec;
        BtParameterGroup *pg;
        gulong num_params;

        if ((num_params =
                MIN (number_of_global_params, para->number_of_global_params))) {
          pg = bt_machine_get_global_param_group (machine);
          for (j = 0; j < num_params; j++) {
            if ((pspec = bt_parameter_group_get_param_spec (pg, j))) {
              BmxParameter *param = &para->global_params[j];
              GST_DEBUG ("pspec for global param %2d : %s / %s", j, pspec->name,
                  g_type_name (pspec->value_type));
              // ev. overwite param names as we need to use the sanitized version that works as a gobject property name
              g_free (param->name);
              param->name = g_strdup (pspec->name);
              fill_machine_parameter (self, param, pspec, no_para);
            } else {
              GST_WARNING ("no pspec for global param %2d", j);
            }
          }
        }
        if ((num_params =
                MIN (number_of_track_params, para->number_of_track_params))) {
          pg = bt_machine_get_voice_param_group (machine, 0);
          for (j = 0; j < num_params; j++) {
            if ((pspec = bt_parameter_group_get_param_spec (pg, j))) {
              BmxParameter *param = &para->track_params[j];
              GST_DEBUG ("pspec for voice param %2d : %s / %s", j, pspec->name,
                  g_type_name (pspec->value_type));
              // ev. overwite param names as we need to use the sanitized version that works as a gobject property name
              g_free (param->name);
              param->name = g_strdup (pspec->name);
              fill_machine_parameter (self, param, pspec, no_para);
            } else {
              GST_WARNING ("no pspec for voice param %2d", j);
            }
          }
        }
      } else {
        if (no_para) {
          BmxParameter *param;
          // volume: 0x0->0db, 0x4000->-80db
          param = &para->global_params[0];
          param->type = 3;
          param->name = g_strdup ("volume");
          param->minvalue = 0;
          param->maxvalue = 0x4000;
          param->novalue = param->defvalue = 0; // FIXME
          // bpm   : 0x10, 0x200
          param = &para->global_params[1];
          param->type = 3;
          param->name = g_strdup ("bpm");
          param->minvalue = 0x10;
          param->maxvalue = 0x200;
          param->novalue = param->defvalue = 0; // FIXME
          // tpb   : 0x1, 0x20
          param = &para->global_params[2];
          param->type = 2;
          param->name = g_strdup ("tpb");
          param->minvalue = 0x1;
          param->maxvalue = 0x20;
          param->novalue = param->defvalue = 0; // FIXME
        }
      }
      g_object_get (machine, "machine", &element, NULL);
      class = G_OBJECT_GET_CLASS (element);
    } else {
      g_error_free (err);
      err = NULL;
      element = NULL;
      g_object_unref (machine);
      machine = NULL;
      if (no_para) {
        // collect failed machines
        GST_WARNING ("failed to create machine: %s", plugin_name);
        bt_setup_remember_missing_machine (setup, g_strdup (plugin_name));
        // we need to abort in no_para mode if we don't have a machine as we
        // don't know how much to skip
        // TODO(ensonic): need a database of machine:num-{attr,global,track}
        result = FALSE;
        break;
      }
    }

    mach->number_of_attributes = read_word (self);
    GST_DEBUG ("    number of attributes : %d", mach->number_of_attributes);
    if (mach->number_of_attributes) {
      gchar *tstr, *ptr1, *ptr2;

      for (j = 0; ((j < mach->number_of_attributes) && !self->priv->io_error);
          j++) {
        tstr = read_null_string (self, entry->size);

        // same code as in: gstbml/src/common.c::gstbml_convert_names()
        str = g_convert (tstr, -1, "ASCII", "WINDOWS-1252", NULL, NULL, NULL);
        g_free (tstr);
        if (str) {
          g_strcanon (str, G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS "-_", '-');

          // remove leading '-'
          ptr1 = ptr2 = str;
          while (*ptr2 == '-')
            ptr2++;
          // remove double '-'
          while (*ptr2) {
            if (*ptr2 == '-') {
              while (ptr2[1] == '-')
                ptr2++;
            }
            if (ptr1 != ptr2)
              *ptr1 = *ptr2;
            ptr1++;
            ptr2++;
          }
          if (ptr1 != ptr2)
            *ptr1 = '\0';
          // remove trailing '-'
          ptr1--;
          while (*ptr1 == '-')
            *ptr1++ = '\0';

          // name must begin with a char
          if (!g_ascii_isalpha (str[0])) {
            gchar *old_name = str;
            str = g_strconcat ("Par_", old_name, NULL);
            g_free (old_name);
          }
        }

        value = read_dword (self);
        GST_DEBUG ("    attribute %d : \"%s\" %d", j, str, value);
        if (element) {
          // probe if the attribute exists
          if (g_object_class_find_property (class, str)) {
            g_object_set (element, str, value, NULL);
          } else {
            GST_WARNING ("machine %s has no attribute %s", mach->name, str);
          }
        }
        g_free (str);
      }
    }
    // read global params
    if (para->number_of_global_params) {
      mach->global_parameter_state =
          g_new0 (guint16, para->number_of_global_params);
      for (j = 0;
          ((j < para->number_of_global_params) && !self->priv->io_error); j++) {
        BmxParameter *param = &para->global_params[j];
        guint16 val = 0;

        GST_DEBUG ("    global-param %d (name=%s,type=%d,flag=%d)", j,
            param->name, param->type, param->flags);
        if ((param->type == PT_NOTE) || (param->type == PT_SWITCH)
            || (param->type == PT_BYTE) || (param->type == PT_ENUM)) {
          val = (guint16) read_byte (self);
          // TODO(ensonic): note params need to be converted:
          // valuestr=gstbt_tone_conversion_note_number_2_string(par);
        } else if (param->type == PT_WORD) {
          val = read_word (self);
        }
        if (param->flags & PF_STATE) {
          // only trigger params need no_val handling
          GST_DEBUG
              ("      no-value check: %u == no-value %d ? => def-value %d", val,
              param->novalue, param->defvalue);
          if (val == param->novalue) {
            val = param->defvalue;
          }
        }
        if (val != param->defvalue && val != param->novalue) {
          GST_DEBUG ("      min/max-value check: %d <= %u <= %d?",
              param->minvalue, val, param->maxvalue);
          if ((param->minvalue != -1) && (val < param->minvalue)) {
            val = param->minvalue;
          }
          if ((param->maxvalue != -1) && (val > param->maxvalue)) {
            val = param->maxvalue;
          }
        }
        mach->global_parameter_state[j] = val;

        // set parameter
        //if(element && (param->flags&PF_STATE) && (param->type!=PT_NOTE)) {
        if (element && (param->flags & PF_STATE)
            && (j < number_of_global_params)) {
          if (mach->type) {
            GST_DEBUG ("    global-param %d (name=%s), %u", j, param->name,
                val);
            g_object_set (element, param->name, val, NULL);
          }
        }
      }
      // master settings
      if (!mach->type) {
        BtSongInfo *song_info;

        g_object_get (G_OBJECT (song), "song-info", &song_info, NULL);
        g_object_set (G_OBJECT (song_info),
            "bpm", (gulong) mach->global_parameter_state[1],
            "tpb", (gulong) mach->global_parameter_state[2],
            // I belive this is hardcoded in buzz, beats = bars/tpb;
            "bars", (gulong) (mach->global_parameter_state[2] * 4), NULL);
        GST_DEBUG ("setting bpm : %d, tpb : %d",
            mach->global_parameter_state[1], mach->global_parameter_state[2]);
        g_object_unref (song_info);
        if (element) {
          // map master volume: 0x0->1.0, 0x4000(=16384)->-80db
          gdouble db =
              ((gdouble) mach->global_parameter_state[0] / 16384.0) * (-80.0);
          gdouble vol = (db == 0.0) ? 1.0 : (gdouble) (pow (10.0, db / 20.0));
          g_object_set (element, "master-volume", vol, NULL);
        }
      }
    }

    mach->number_of_tracks = read_word (self);
    if (machine) {
      g_object_set (machine, "voices", mach->number_of_tracks, NULL);
    }
    GST_DEBUG ("    number of tracks : %d", mach->number_of_tracks);
    // read track params
    if (mach->number_of_tracks) {
      GObject *track;

      mach->track_parameter_state = g_new0 (guint16 *, mach->number_of_tracks);
      for (j = 0; ((j < mach->number_of_tracks) && !self->priv->io_error); j++) {
        track =
            element ?
            gst_child_proxy_get_child_by_index (GST_CHILD_PROXY (element),
            j) : NULL;

        mach->track_parameter_state[j] =
            g_new0 (guint16, para->number_of_track_params);
        for (k = 0;
            ((k < para->number_of_track_params) && !self->priv->io_error);
            k++) {
          BmxParameter *param = &para->track_params[k];
          guint16 val = 0;

          GST_DEBUG ("    voice-param %d (name=%s,type=%d,flag=%d)", k,
              param->name, param->type, param->flags);
          if ((param->type == PT_NOTE) || (param->type == PT_SWITCH)
              || (param->type == PT_BYTE) || (param->type == PT_ENUM)) {
            val = (guint16) read_byte (self);
            // TODO(ensonic): note params need to be converted
            //if(track && (param->flags&PF_STATE) && (param->type!=PT_NOTE)) {
          } else if (param->type == PT_WORD) {
            val = read_word (self);
          }
          if (param->flags & PF_STATE) {
            // only trigger params need no_val handling
            GST_DEBUG
                ("      no-value check: %u == no-value %d ? => def-value %d",
                val, param->novalue, param->defvalue);
            if (val == param->novalue) {
              val = param->defvalue;
            }
          }
          if (val != param->defvalue && val != param->novalue) {
            GST_DEBUG ("      min/max-value check: %d <= %u <= %d?",
                param->minvalue, val, param->maxvalue);
            if ((param->minvalue != -1) && (val < param->minvalue)) {
              val = param->minvalue;
            }
            if ((param->maxvalue != -1) && (val > param->maxvalue)) {
              val = param->maxvalue;
            }
          }
          mach->track_parameter_state[j][k] = val;

          // set parameter
          if (track && (param->flags & PF_STATE)
              && (k < number_of_track_params)) {
            GST_DEBUG ("    voice-param %d (name=%s), %d", k, param->name, val);
            g_object_set (track, param->name, val, NULL);
          }
        }
        g_object_try_unref (track);
      }
    }
    if (machine) {              // add machine properties
      GHashTable *properties;
      gchar str[G_ASCII_DTOSTR_BUF_SIZE];

      g_object_get (machine, "properties", &properties, NULL);
      if (properties) {
        g_hash_table_insert (properties, g_strdup ("xpos"),
            g_strdup (g_ascii_dtostr (str, G_ASCII_DTOSTR_BUF_SIZE,
                    mach->xpos)));
        g_hash_table_insert (properties, g_strdup ("ypos"),
            g_strdup (g_ascii_dtostr (str, G_ASCII_DTOSTR_BUF_SIZE,
                    mach->ypos)));
      }
      gst_object_unref (element);
    } else {
      // collect failed machines
      GST_WARNING ("failed to create machine: %s", plugin_name);
      bt_setup_remember_missing_machine (setup, g_strdup (plugin_name));
    }
  }

  g_object_try_unref (setup);
  if (self->priv->io_error)
    result = FALSE;
  GST_INFO ("read MACH section %d: off+size=%u, pos=%lu", result,
      entry->offset + entry->size, self->priv->data_pos);
  return result;
}

static gboolean
read_conn_section (const BtSongIOBuzz * self, const BtSong * song)
{
  gboolean result = TRUE;
  BmxSectionEntry *entry = get_section_entry (self, "CONN");
  BtSetup *setup;
  BtWire *wire;
  BtMachine *src, *dst;
  guint32 i;
  guint16 number_of_wires, id_src, id_dst, amp, pan;
  GError *err = NULL;

  if (!entry)
    return FALSE;

  g_object_get (G_OBJECT (song), "setup", &setup, NULL);

  mem_seek (self, entry->offset, SEEK_SET);
  GST_INFO ("reading CONN section: 0x%x, %u", entry->offset, entry->size);

  number_of_wires = read_word (self);
  GST_INFO ("  number of wires : %d", number_of_wires);

  for (i = 0; ((i < number_of_wires) && !self->priv->io_error); i++) {
    id_src = read_word (self);
    id_dst = read_word (self);
    amp = read_word (self);
    pan = read_word (self);
    src =
        bt_setup_get_machine_by_id (setup,
        self->priv->mach_section[id_src].name);
    dst =
        bt_setup_get_machine_by_id (setup,
        self->priv->mach_section[id_dst].name);
    self->priv->mach_section[id_dst].number_of_inputs++;
    if (src && dst) {
      wire = bt_wire_new (song, src, dst, &err);
      if (err == NULL) {
        GstElement *a_elem, *p_elem;
        GST_INFO ("  creating wire %u, src=%p, dst=%p, amp=%u, pan=%u", i, src,
            dst, amp, pan);
        g_object_get (wire, "gain", &a_elem, "pan", &p_elem, NULL);
        // TODO(ensonic): buzz says 0x0=0% ... 0x4000=100%
        // does this ev. take number of connections into account?
        g_object_set (a_elem, "volume", (gdouble) amp / 16384.0, NULL);
        if (p_elem) {
          g_object_set (p_elem, "panorama", ((gdouble) pan / 16384.0) - 1.0,
              NULL);
          g_object_unref (p_elem);
        }
      } else {
        g_error_free (err);
        err = NULL;
      }
    } else {
      GST_WARNING ("skipping wire %d, src=%p, dst=%p", i, src, dst);
    }
    g_object_try_unref (src);
    g_object_try_unref (dst);
  }

  g_object_try_unref (setup);
  if (self->priv->io_error)
    result = FALSE;
  GST_INFO ("read CONN section %d: off+size=%u, pos=%lu", result,
      entry->offset + entry->size, self->priv->data_pos);
  return result;
}

static gboolean
read_patt_section (const BtSongIOBuzz * self, const BtSong * song)
{
  gboolean result = TRUE;
  BmxSectionEntry *entry = get_section_entry (self, "PATT");
  BtSetup *setup;
  BtPattern *pattern = NULL;
  BtMachine *machine, *src_machine;
  BtWire *wire;
  guint32 i, j, k, l, p;
  guint16 number_of_patterns, number_of_tracks, number_of_ticks;
  gulong number_of_global_params = 0, number_of_track_params = 0;
  gchar *name;
  guint16 par = 0, amp, pan, id_src;
  gchar value[G_ASCII_DTOSTR_BUF_SIZE + 1];
  const gchar *valuestr;
  gulong wire_params;
  BtParameterGroup *pg;
  BtValueGroup *vg = NULL;
  //BmxParameter *params;

  // DEBUG
  //gchar *str1,*str2;
  // DEBUG

  if (!entry)
    return FALSE;

  g_object_get (G_OBJECT (song), "setup", &setup, NULL);

  mem_seek (self, entry->offset, SEEK_SET);
  GST_INFO ("reading PATT section: 0x%x, %u", entry->offset, entry->size);

  for (i = 0; ((i < self->priv->number_of_machines) && !self->priv->io_error);
      i++) {
    BmxParaSection *para = &self->priv->para_section[i];
    BmxMachSection *mach = &self->priv->mach_section[i];

    if ((machine = bt_setup_get_machine_by_id (setup, mach->name))) {
      g_object_get (machine,
          "global-params", &number_of_global_params,
          "voice-params", &number_of_track_params, NULL);
    }

    number_of_patterns = read_word (self);
    number_of_tracks = read_word (self);
    GST_INFO
        ("  machine: %d \"%s\" patterns: %d  tracks: %d  global_params: %d  track_params: %d",
        i, mach->name, number_of_patterns, number_of_tracks,
        para->number_of_global_params, para->number_of_track_params);
    for (j = 0; ((j < number_of_patterns) && !self->priv->io_error); j++) {
      name = read_null_string (self, entry->size);
      number_of_ticks = read_word (self);
      GST_DEBUG ("    pattern: %d  name: \"%s\"  ticks: %d  inputs: %d", j,
          name, number_of_ticks, mach->number_of_inputs);

      if (machine) {
        pattern = bt_pattern_new (song, name, number_of_ticks, machine);
      } else
        pattern = NULL;

      // wire params
      for (l = 0; ((l < mach->number_of_inputs) && !self->priv->io_error); l++) {
        id_src = read_word (self);
        //str1=g_strdup_printf("      input: %2d  machine: %2d  amp,pan:",l,id_src);
        GST_DEBUG ("      get src-machine by name %d -> %s", id_src,
            self->priv->mach_section[id_src].name);
        src_machine =
            bt_setup_get_machine_by_id (setup,
            self->priv->mach_section[id_src].name);
        if (src_machine && machine && pattern
            && (wire =
                bt_machine_get_wire_by_dst_machine (src_machine, machine))) {
          g_object_get (G_OBJECT (wire), "num-params", &wire_params, NULL);
          vg = bt_pattern_get_wire_group (pattern, wire);
        } else {
          wire = NULL;
          wire_params = 0;
        }
        for (k = 0; ((k < number_of_ticks) && !self->priv->io_error); k++) {
          amp = read_word (self);
          pan = read_word (self);
          //str2=g_strdup_printf("%s  %4x %4x",str1,amp,pan);g_free(str1);str1=str2;
          //GST_LOG("    wire-pattern data 0x%04x,0x%04x",amp,pan);
          if (wire_params > 0) {
            if (amp != 0xFFFF) {
              GST_INFO ("    wire-pattern data 0x%04x", amp);
              // TODO(ensonic): buzz says 0x0=0% ... 0x4000=100%
              g_ascii_dtostr (value, G_ASCII_DTOSTR_BUF_SIZE,
                  (gdouble) amp / 16384.0);
              bt_value_group_set_event (vg, k, 0, value);
            }
            if (wire_params > 1) {
              if (pan != 0xFFFF) {
                g_ascii_dtostr (value, G_ASCII_DTOSTR_BUF_SIZE,
                    ((gdouble) pan / 16384.0) - 1.0);
                bt_value_group_set_event (vg, k, 1, value);
              }
            }
          }
        }
        //GST_INFO(str1);g_free(str1);
        g_object_try_unref (wire);
        g_object_try_unref (src_machine);
      }
      // global params
      if (machine) {
        pg = bt_machine_get_global_param_group (machine);
        vg = bt_pattern_get_global_group (pattern);
      } else {
        pg = NULL;
        vg = NULL;
      }
      for (k = 0; ((k < number_of_ticks) && !self->priv->io_error); k++) {
        //str1=g_strdup_printf("      tick: %2d  global:",k);
        for (p = 0; p < para->number_of_global_params; p++) {
          BmxParameter *param = &para->global_params[p];

          valuestr = NULL;
          if ((param->type == PT_NOTE) || (param->type == PT_SWITCH)
              || (param->type == PT_BYTE) || (param->type == PT_ENUM)) {
            par = read_byte (self);
            //str2=g_strdup_printf("%s %2x",str1,par);g_free(str1);str1=str2;
            if (param->type == PT_NOTE) {
              // convert buzz note format
              valuestr = gstbt_tone_conversion_note_number_2_string (par);
            } else if (param->type == PT_SWITCH) {
              if (par == 1) {
                valuestr = strcpy (value, "1");
              } else
                valuestr = "";
              // TODO(ensonic): what about switch-off (0), switch-no (255)
              GST_LOG ("mapped global switch from %d to '%s'", par, valuestr);
            } else if (param->type == PT_ENUM) {
              GParamSpec *pspec;

              // this should be no-val
              valuestr = "";
              // now try to map to real value
              if (pg && (pspec = bt_parameter_group_get_param_spec (pg, p))) {
                GParamSpecEnum *enum_property = G_PARAM_SPEC_ENUM (pspec);
                GEnumClass *enum_class = enum_property->enum_class;
                GEnumValue *enum_value;

                if ((enum_value = g_enum_get_value (enum_class, par))) {
                  valuestr = enum_value->value_nick;
                }
              } else
                valuestr = "";
              GST_LOG ("mapped global enum from %d to '%s'", par, valuestr);
            }
          } else if (param->type == PT_WORD) {
            par = read_word (self);
            //str2=g_strdup_printf("%s %4x",str1,par);g_free(str1);str1=str2;
          } else {
            par = 0;
            GST_WARNING ("unknown parameter type %d for global param %d",
                param->type, p);
          }
          if (vg && p < number_of_global_params) {
            if (mach->type) {
              if (!valuestr) {
                snprintf (value, sizeof(value), "%d", par);
                valuestr = value;
              }
            } else {
              if (par != 0xffff) {
                // map master volume: 0x0->1.0, 0x4000(=16384)->-80db
                gdouble db = ((gdouble) par / 16384.0) * (-80.0);
                gdouble vol =
                    (db == 0.0) ? 1.0 : (gdouble) (pow (10.0, db / 20.0));
                g_ascii_dtostr (value, G_ASCII_DTOSTR_BUF_SIZE, vol);
                //g_ascii_dtostr(value,G_ASCII_DTOSTR_BUF_SIZE,1.0-(par/16384.0));
                //g_ascii_dtostr(value,G_ASCII_DTOSTR_BUF_SIZE,(par/16384.0));
                valuestr = value;
                GST_LOG ("    master volume data 0x%04x -> %s", par, value);
              } else
                valuestr = "";
            }
            //GST_LOG("setting global parameter %d,%d: %s",k,p,valuestr);
            bt_value_group_set_event (vg, k, p, valuestr);
          }
          //else {
          //  GST_WARNING("skip global parameter machine %p, pattern %p, param %d",machine,pattern,p);
          //}
        }
        //GST_INFO(str1);g_free(str1);
      }
      // track params
      for (l = 0; ((l < number_of_tracks) && !self->priv->io_error); l++) {
        //str2=g_strdup_printf("%s  ",str1);g_free(str1);str1=str2;
        if (machine) {
          pg = bt_machine_get_voice_param_group (machine, l);
          vg = bt_pattern_get_voice_group (pattern, l);
        } else {
          pg = NULL;
          vg = NULL;
        }
        for (k = 0; ((k < number_of_ticks) && !self->priv->io_error); k++) {
          //str1=g_strdup_printf("      tick: %2d  track:",k);
          for (p = 0; p < para->number_of_track_params; p++) {
            BmxParameter *param = &para->track_params[p];

            valuestr = NULL;
            if ((param->type == PT_NOTE) || (param->type == PT_SWITCH)
                || (param->type == PT_BYTE) || (param->type == PT_ENUM)) {
              par = read_byte (self);
              //str2=g_strdup_printf("%s %2x",str1,par);g_free(str1);str1=str2;
              if (param->type == PT_NOTE) {
                // convert buzz note format
                valuestr = gstbt_tone_conversion_note_number_2_string (par);
              } else if (param->type == PT_SWITCH) {
                if (par == 1) {
                  valuestr = strcpy (value, "1");
                } else
                  valuestr = "";
                // TODO(ensonic): what about switch-off (0), switch-no (255)
                GST_LOG ("mapped voice switch from %d to '%s'", par, valuestr);
              } else if (param->type == PT_ENUM) {
                GParamSpec *pspec;

                // this should be no-val
                valuestr = "";
                // now try to map to real value
                if (pg && (pspec = bt_parameter_group_get_param_spec (pg, p))) {
                  GParamSpecEnum *enum_property = G_PARAM_SPEC_ENUM (pspec);
                  GEnumClass *enum_class = enum_property->enum_class;
                  GEnumValue *enum_value;

                  if ((enum_value = g_enum_get_value (enum_class, par))) {
                    valuestr = enum_value->value_nick;
                  }
                } else
                  valuestr = "";
                GST_LOG ("mapped voice enum from %d to '%s'", par, valuestr);
              }
            } else if (param->type == PT_WORD) {
              par = read_word (self);
              //str2=g_strdup_printf("%s %4x",str1,par);g_free(str1);str1=str2;
            } else {
              par = 0;
              GST_WARNING ("unknown parameter type %d for voice %d param %d",
                  param->type, l, p);
            }
            if (vg && p < number_of_track_params) {
              if (!valuestr) {
                snprintf (value, sizeof(value), "%d", par);
                valuestr = value;
              }
              //GST_DEBUG("setting voice parameter %d,%d,%d: %s",k,l,p,valuestr);
              bt_value_group_set_event (vg, k, p, valuestr);
            }
          }
        }
        //GST_INFO(str1);g_free(str1);
      }
      g_free (name);
      g_object_try_unref (pattern);
    }
    g_object_try_unref (machine);
  }

  g_object_try_unref (setup);
  if (self->priv->io_error)
    result = FALSE;
  GST_INFO ("read PATT section %d: off+size=%u, pos=%lu", result,
      entry->offset + entry->size, self->priv->data_pos);
  return result;
}

static gboolean
read_sequ_section (const BtSongIOBuzz * self, const BtSong * song)
{
  gboolean result = TRUE;
  BmxSectionEntry *entry = get_section_entry (self, "SEQU");
  guint32 i, j;
  guint32 end_of_song, begin_of_loop, end_of_loop;
  guint16 number_of_sequences;
  guint16 machine_index;
  guint32 number_of_events;
  guint32 track = 0;
  guint8 bpep, bpe;
  guint32 position = 0, event = 0;
  gboolean loop = FALSE;
  BtSequence *sequence;
  BtSetup *setup;
  BtMachine *machine;
  BtCmdPattern *pattern;

  if (!entry)
    return FALSE;

  g_object_get (G_OBJECT (song), "sequence", &sequence, "setup", &setup, NULL);

  mem_seek (self, entry->offset, SEEK_SET);
  GST_INFO ("reading SEQU section: 0x%x, %u", entry->offset, entry->size);

  end_of_song = read_dword (self);
  begin_of_loop = read_dword (self);
  end_of_loop = read_dword (self);
  number_of_sequences = read_word (self);       // columns
  GST_INFO ("  len: %d  loop: %d .. %d  number of sequences: %d", end_of_song,
      begin_of_loop, end_of_loop, number_of_sequences);

  g_object_set (G_OBJECT (sequence), "length", end_of_song, NULL);

  for (i = 0; ((i < number_of_sequences) && !self->priv->io_error); i++) {
    machine_index = read_word (self);
    number_of_events = read_dword (self);
    if (number_of_events) {
      bpep = read_byte (self);
      bpe = read_byte (self);
    } else
      bpep = bpe = 0;
    GST_INFO ("  machine: %u  number of events: %u  bytes: %u,%u",
        machine_index, number_of_events, bpep, bpe);

    if ((machine =
            bt_setup_get_machine_by_id (setup,
                self->priv->mach_section[machine_index].name))) {
      bt_sequence_add_track (sequence, machine, -1);
    }

    for (j = 0; ((j < number_of_events) && !self->priv->io_error); j++) {
      switch (bpep) {
        case 1:
          position = read_byte (self);
          break;
        case 2:
          position = read_word (self);
          break;
        case 4:
          position = read_dword (self);
          break;
        default:
          GST_WARNING ("unhandled bytes per event position : %d", bpep);
      }
      switch (bpe) {
        case 1:
          event = read_byte (self);
          loop = (event & 0x80) ? TRUE : FALSE;
          event &= 0x7F;
          break;
        case 2:
          event = read_word (self);
          loop = (event & 0x8000) ? TRUE : FALSE;
          event &= 0x7FFF;
          break;
        case 4:
          event = read_dword (self);
          loop = (event & 0x80000000) ? TRUE : FALSE;
          event &= 0x7FFFFFFF;
          break;
        default:
          GST_WARNING ("unhandled bytes per event : %d", bpe);
      }
      GST_DEBUG ("  position: %d  event: %d  loop: %d", position, event, loop);
      // add event to sequence
      if (machine) {
        pattern = NULL;
        switch (event) {
          case 0:              // mute
            if (BT_IS_SOURCE_MACHINE (machine))
              pattern =
                  bt_machine_get_pattern_by_index (machine,
                  BT_SOURCE_MACHINE_PATTERN_INDEX_MUTE);
            else if (BT_IS_PROCESSOR_MACHINE (machine))
              pattern =
                  bt_machine_get_pattern_by_index (machine,
                  BT_PROCESSOR_MACHINE_PATTERN_INDEX_MUTE);
            else if (BT_IS_SINK_MACHINE (machine))
              pattern =
                  bt_machine_get_pattern_by_index (machine,
                  BT_SINK_MACHINE_PATTERN_INDEX_MUTE);
            break;
          case 1:              // break;
            if (BT_IS_SOURCE_MACHINE (machine))
              pattern =
                  bt_machine_get_pattern_by_index (machine,
                  BT_SOURCE_MACHINE_PATTERN_INDEX_BREAK);
            else if (BT_IS_PROCESSOR_MACHINE (machine))
              pattern =
                  bt_machine_get_pattern_by_index (machine,
                  BT_PROCESSOR_MACHINE_PATTERN_INDEX_BREAK);
            else if (BT_IS_SINK_MACHINE (machine))
              pattern =
                  bt_machine_get_pattern_by_index (machine,
                  BT_SINK_MACHINE_PATTERN_INDEX_BREAK);
            break;
          case 2:              // thru
            if (BT_IS_PROCESSOR_MACHINE (machine))
              pattern =
                  bt_machine_get_pattern_by_index (machine,
                  BT_PROCESSOR_MACHINE_PATTERN_INDEX_BYPASS);
            break;
          default:
            if (event > 15) {
              event -= 16;      // real pattern id
              // offset with internal pattern number
              if (BT_IS_SOURCE_MACHINE (machine))
                event += BT_SOURCE_MACHINE_PATTERN_INDEX_OFFSET;
              else if (BT_IS_PROCESSOR_MACHINE (machine))
                event += BT_PROCESSOR_MACHINE_PATTERN_INDEX_OFFSET;
              else if (BT_IS_SINK_MACHINE (machine))
                event += BT_SINK_MACHINE_PATTERN_INDEX_OFFSET;
              // get pattern
              pattern = bt_machine_get_pattern_by_index (machine, event);
            } else {
              GST_ERROR ("  invalid pattern index \"%d\"", event);
            }
            break;
        }
        if (pattern) {
          bt_sequence_set_pattern_quick (sequence, position, track, pattern);
          g_object_unref (pattern);
        } else
          GST_WARNING ("  no pattern for ix=%d", event);
      }
    }
    if (machine) {
      g_object_unref (machine);
      track++;
    }
  }

  g_object_try_unref (sequence);
  g_object_try_unref (setup);
  if (self->priv->io_error)
    result = FALSE;
  GST_INFO ("read SEQU section %d: off+size=%u, pos=%lu", result,
      entry->offset + entry->size, self->priv->data_pos);
  return result;
}

static gboolean
read_wavt_section (const BtSongIOBuzz * self, const BtSong * song)
{
  gboolean result = TRUE;
  BmxSectionEntry *entry = get_section_entry (self, "WAVT");
  guint16 index;
  guint16 number_of_waves;
  guint16 number_of_envelopes;
  guint16 number_of_points;
  guint8 number_of_levels;
  guint16 i, j;
  gchar *file_name, *name;
  guint8 flags;
  gfloat volume;
  guint32 number_of_samples;
  guint32 loop_start, loop_end;
  guint32 samples_per_sec;
  guint8 root_note;
  guint channels;
  BtWave *wave;
  BtWavelevel *wavelevel;
  BtWaveLoopMode loop_mode;

  if (!entry)
    return FALSE;

  mem_seek (self, entry->offset, SEEK_SET);
  GST_INFO ("reading WAVT section: 0x%x, %u", entry->offset, entry->size);

  number_of_waves = read_word (self);
  GST_INFO ("  number of waves: %u", number_of_waves);

  for (i = 0; i < number_of_waves; i++) {
    // reader header
    index = read_word (self);
    file_name = read_null_string (self, entry->size);
    name = read_null_string (self, entry->size);
    volume = read_float (self);
    flags = read_byte (self);

    GST_INFO
        ("%02d: index=%2d volume=%f flags=0x%x name=\"%s\" file_name=\"%s\"", i,
        index, volume, flags, name, file_name);
    /*  bit 0: loop
       bit 1: don't save
       bit 2: floating point memory format
       bit 3: stereo (since 1.2)
       bit 4: bidirectional loop (since 1.2)
       bit 7: envelopes follow (since alpha 1.4)
     */
    channels = (flags & (1 << 3)) ? 2 : 1;

    if (flags & 1) {
      loop_mode =
          (flags & 16) ? BT_WAVE_LOOP_MODE_PINGPONG : BT_WAVE_LOOP_MODE_FORWARD;
    } else {
      loop_mode = BT_WAVE_LOOP_MODE_OFF;
    }

    // and need to avoid loading the file from disk (uri=NULL)
    // should be bmx only
    wave =
        bt_wave_new (song, name, NULL, index + 1, volume, loop_mode, channels);

    // need to remap the file-names
    //g_object_set(wave,"uri",file_name,NULL);

    if (flags & (1 << 7)) {
      // read envelopes
      number_of_envelopes = read_word (self);
      GST_DEBUG ("      number of envelopes: %d", number_of_envelopes);
      //wavtable->envelopes = new BmxWavtableEnvelope[wavtable->numberOfEnvelopes];

      for (j = 0; j < number_of_envelopes; j++) {
        //BmxWavtableEnvelope *envelope = &wavtable->envelopes[j];
        mem_seek (self, 10, SEEK_CUR);
        /*
           envelope->attack = read_word();
           envelope->decay = read_word();
           envelope->sustain = read_word();
           envelope->release = read_word();
           envelope->subdiv = read_byte();
           envelope->flags = read_byte();
         */
        number_of_points = read_word (self);
        //envelope->disabled=(envelope->number_of_points&0x8000)?1:0;
        number_of_points &= 0x7FFF;
        GST_DEBUG ("      number of envelope-points: %d", number_of_points);

        if (number_of_points) {
          mem_seek (self, (number_of_points * 5), SEEK_CUR);
          /*
             envelope->points = new BmxWavtableEnvelopePoints[envelope->numberOfPoints];

             for (unsigned int k = 0; k < envelope->numberOfPoints; k++) {
             BmxWavtableEnvelopePoints *point = &envelope->points[k];

             point->x = read_word();
             point->y = read_word();
             point->flags = read_byte();
             }
           */
        }
      }
    }
    // read levels
    number_of_levels = read_byte (self);
    GST_DEBUG ("      number of levels: %d", number_of_levels);

    for (j = 0; j < number_of_levels; j++) {
      number_of_samples = read_dword (self);
      loop_start = read_dword (self);
      loop_end = read_dword (self);
      samples_per_sec = read_dword (self);
      root_note = read_byte (self);

      GST_DEBUG ("%02d.%02d: samples=%6d loop=%6d,%6d,rate=%5d root=%d", i, j,
          number_of_samples, loop_start, loop_end, samples_per_sec, root_note);

      wavelevel =
          bt_wavelevel_new (song, wave, root_note, number_of_samples,
          loop_start, loop_end, samples_per_sec, NULL);
      g_object_unref (wavelevel);
    }

    g_object_unref (wave);
  }

  //g_object_try_unref(wavetable);
  if (self->priv->io_error)
    result = FALSE;
  GST_INFO ("read WAVT section %d: off+size=%u, pos=%lu", result,
      entry->offset + entry->size, self->priv->data_pos);
  return result;
}

static gboolean
read_cwav_section (const BtSongIOBuzz * self, const BtSong * song)
{
  gboolean result = TRUE;
  BmxSectionEntry *entry = get_section_entry (self, "CWAV");
  guint16 number_of_waves;
  guint16 i;
  guint16 index;
  guint8 format;
  BtWavetable *wavetable;
  BtWave *wave;
  BtWavelevel *wavelevel;
  GList *list, *node;
  gulong bytes, length, remain;
  guint channels;

  // this section is optional
  if (!entry)
    entry = get_section_entry (self, "WAVE");
  if (!entry)
    return TRUE;

  g_object_get (G_OBJECT (song), "wavetable", &wavetable, NULL);

  mem_seek (self, entry->offset, SEEK_SET);
  GST_INFO ("reading CWAV section: 0x%x, %u", entry->offset, entry->size);

  number_of_waves = read_word (self);
  GST_INFO ("  number of waves: %u", number_of_waves);

  remain = entry->size - 2;
  // decompression context
  ctxt.self = self;

  for (i = 0; i < number_of_waves; i++) {
    // reader header
    index = read_word (self);
    format = read_byte (self);  // 0 raw, 1 compressed
    remain -= 3;

#if 0
    // quirk mode to find next wavestart if we lost it
    if ((index > 200) || (format > 1)) {
      GList *waves;
      BtWave *wave;
      gulong wave_index;
      gchar buffer[4];
      glong fpos1 = self->priv->data_pos, fpos2, fskip;
      gboolean scanning = TRUE;

      GST_WARNING
          ("%02d: lost sync at file pos, 0x%x, index=%2d format=%d, remaining=%lu",
          i, fpos1, index, format, remain);
      g_object_get (wavetable, "waves", &waves, NULL);

      wave = g_list_nth_data (waves, i);
      g_object_get (G_OBJECT (wave), "index", &wave_index, NULL);
      wave_index--;             // the index in the file is one less
      GST_INFO ("%02d: scanning for 0x%x 00 01 ..", i, wave_index);
      g_list_free (waves);

      // now seek for $index 00 00|01
      mem_seek (self, -2, SEEK_CUR);
      remain += 2;
      while (scanning && remain > 4) {
        if (mem_read (self, &buffer, 4, 1) == 1) {
          // search for next index and assume its compressed
          // buffer[0],buffer[1] is the index
          // buffer[2] is compressed uncompressed
          // buffer[3] needs to start with no 0 bits and then have 6 (as a shift)
          //if((buffer[0]==wave_index) && (buffer[1]==0) && (buffer[2]==1)) {
          //if((buffer[0]==wave_index) && (buffer[1]==0) && (buffer[2]==1) && ((buffer[3]&0x80)==0x80)) {
          if ((buffer[0] == wave_index) && (buffer[1] == 0) && (buffer[2] == 1)
              && ((buffer[3] & 0x1F) == 0x0D)) {
            index = wave_index;
            format = buffer[2];
            scanning = FALSE;
            mem_seek (self, -1, SEEK_CUR);
            fpos2 = self->priv->data_pos;
            fskip = fpos2 - fpos1;
            remain -= 3;
            GST_INFO ("resynced at 0x%x from 0x%x after skipping %ld bytes",
                fpos2 - 3, fpos1 - 3, fskip);
          } else {
            mem_seek (self, -3, SEEK_CUR);
            remain--;
          }
        } else {
          GST_WARNING ("reach end of block in quirk scan");
          return FALSE;
        }
      }
    }
#endif

    GST_INFO ("%02d: index=%2d format=%d, remaining=%lu", i, index, format,
        remain);

    if (!format) {
      (void) read_dword (self);
      remain -= 4;
    } else {
      // decompression context
      ctxt.max_bytes = MAXPACKEDBUFFER;
      ctxt.bytes_in_file_remain = remain;
      ctxt.cur_bit = 0;
      // set up so that call to unpack_bits() will force an immediate read from file
      ctxt.cur_index = MAXPACKEDBUFFER;
      ctxt.bytes_in_buffer = 0;
      // needed so the unpack_bits() can signal an error
      ctxt.error = FALSE;
    }

    // get wave
    if ((wave = bt_wavetable_get_wave_by_index (wavetable, (index + 1)))) {
      g_object_get (wave, "wavelevels", &list, "channels", &channels, NULL);
      for (node = list; node; node = g_list_next (node)) {
        gpointer data;

        wavelevel = BT_WAVELEVEL (node->data);
        g_object_get (wavelevel, "length", &length, NULL);

        bytes = length * channels * sizeof (gint16);
        GST_DEBUG ("%02u.00: bytes=%lu", i, bytes);

        if ((data = g_try_malloc (bytes))) {
          if (!format) {
            if ((mem_read (self, data, bytes, 1)) != 1) {
              g_free (data);
              GST_WARNING ("error reading data");
              break;
            }
            remain -= bytes;
          } else {
            decompress_wave ((guint16 *) data, length, channels);
          }
          g_object_set (wavelevel, "data", data, NULL);
          // DEBUG
          // plot [0:] [-35000:35000] '/tmp/sample.raw' binary format="%short" using 1 with lines
          //g_file_set_contents("/tmp/sample.raw",(gchar *)data,bytes,NULL);
          // DEBUG
        } else {
          // skip this and try next
          GST_WARNING ("out of memory for this wavelevel");
        }
      }
      if (format == 1) {
        /* assume
           we read 100 bytes (bytes_in_buffer)
           we used  20 bytes (cur_index)
           we need to seek back 80 (padding)
           whats the +1?
         */
        gint32 padding =
            (gint32) (ctxt.cur_index + 1) - (gint32) ctxt.bytes_in_buffer;
        if (padding) {
          GST_DEBUG ("seek back remaining %d bytes = (%u+1)-%u", padding,
              ctxt.cur_index, ctxt.bytes_in_buffer);
          GST_DEBUG ("file position : > 0x%lx", self->priv->data_pos);
          ctxt.bytes_in_file_remain -= padding;
          if ((mem_seek (self, padding, SEEK_CUR)) == -1) {
            GST_WARNING ("failed to seek: %s", strerror (errno));
          }
          GST_DEBUG ("file position : < 0x%lx", self->priv->data_pos);
        }
        remain = ctxt.bytes_in_file_remain;
      }
      g_list_free (list);
      g_object_unref (wave);
    } else {
      GST_WARNING ("no wave for index: %d", index);
    }
  }

  g_object_try_unref (wavetable);
  if (self->priv->io_error)
    result = FALSE;
  GST_INFO ("read CWAV section %d: off+size=%u, pos=%lu", result,
      entry->offset + entry->size, self->priv->data_pos);
  return result;
}

static gboolean
read_blah_section (const BtSongIOBuzz * self, const BtSong * song)
{
  gboolean result = TRUE;
  BmxSectionEntry *entry = get_section_entry (self, "BLAH");
  BtSongInfo *song_info;
  BtSetup *setup;
  GHashTable *properties;
  guint32 len;
  gchar *str;

  if (!entry)
    return FALSE;

  g_object_get (G_OBJECT (song), "song-info", &song_info, "setup", &setup,
      NULL);

  mem_seek (self, entry->offset, SEEK_SET);
  GST_INFO ("reading BLAH section: 0x%x, %u", entry->offset, entry->size);

  len = read_dword (self);
  str = len ? read_string (self, len) : NULL;
  g_object_set (G_OBJECT (song_info), "info", str, "author", NULL, NULL);

  // if len!=0 mark "info" page as active
  if (len) {
    g_object_get (G_OBJECT (setup), "properties", &properties, NULL);
    g_hash_table_insert (properties, g_strdup ("active-page"), g_strdup ("4"));
  }

  g_free (str);
  g_object_unref (setup);
  g_object_unref (song_info);
  if (self->priv->io_error)
    result = FALSE;
  GST_INFO ("read BLAH section %d: off+size=%u, pos=%lu", result,
      entry->offset + entry->size, self->priv->data_pos);
  return result;
}

static gboolean
read_pdlg_section (const BtSongIOBuzz * self, const BtSong * song)
{
  gboolean result = TRUE;
  BmxSectionEntry *entry = get_section_entry (self, "PDLG");
  guint8 flags;
  gchar *name;
  typedef struct _WINDOWPLACEMENT
  {
    unsigned int length;
    unsigned int flags;
    unsigned int showCmd;
    int min_x, min_y;
    int max_x, max_y;
    int pos[4];                 // long left,top,bottom,rigth
  } WINDOWPLACEMENT;
  WINDOWPLACEMENT win;

  // this section is optional
  if (!entry)
    return TRUE;

  mem_seek (self, entry->offset, SEEK_SET);
  GST_INFO ("reading PDLG section: 0x%x, %u", entry->offset, entry->size);

  /*
   * byte               flags: bit 1: dialogs visible
   * list of positions followed by terminating zero byte:
   *   asciiz           name of machine
   *   WINDOWPLACEMENT  win32 window placement structure http://msdn.microsoft.com/en-us/library/ms632611.aspx
   */
  // flags, bit 1: dialogs visible
  flags = read_byte (self);
  GST_DEBUG ("  dialogs visible: %d", flags & 1);

  name = read_null_string (self, entry->size);
  while (name && *name) {
    // read windowplacement
    if (mem_read (self, &win, sizeof (win), 1)) {
      //GST_MEMDUMP ("window pos: ", (guint8 *)&win, sizeof (win));
      GST_DEBUG ("  window pos '%s': %d,%d-%d,%d", name,
          GUINT_FROM_LE (win.pos[0]), GUINT_FROM_LE (win.pos[1]),
          GUINT_FROM_LE (win.pos[2]), GUINT_FROM_LE (win.pos[3]));
    }
    name = read_null_string (self, entry->size);
  }

  if (self->priv->io_error)
    result = FALSE;
  GST_INFO ("read PDLG section %d: off+size=%u, pos=%lu", result,
      entry->offset + entry->size, self->priv->data_pos);
  return result;
}

static gboolean
read_midi_section (const BtSongIOBuzz * self, const BtSong * song)
{
  gboolean result = TRUE;
  BmxSectionEntry *entry = get_section_entry (self, "MIDI");
#if 0
  gchar *name;
#endif

  // this section is optional
  if (!entry)
    return TRUE;

  mem_seek (self, entry->offset, SEEK_SET);
  GST_INFO ("reading MIDI section: 0x%x, %u", entry->offset, entry->size);

  /*
   * list of bindings followed by terminating zero byte:
   *
   * asciiz     name of machine
   * byte               parameter group
   * byte               parameter track
   * byte               parameter number
   * byte               midi channel
   * byte               midi controller number
   */

#if 0
  name = read_null_string (self, entry->size);
  while (name && *name) {
    // read assignment

    name = read_null_string (self, entry->size);
  }
#endif
  if (self->priv->io_error)
    result = FALSE;
  GST_INFO ("read MIDI section %d: off+size=%u, pos=%lu", result,
      entry->offset + entry->size, self->priv->data_pos);
  return result;
}

//-- methods

static gboolean
bt_song_io_buzz_load (gconstpointer const _self, const BtSong * const song,
    GError ** err)
{
  const BtSongIOBuzz *self = BT_SONG_IO_BUZZ (_self);
  gboolean result = FALSE;
  gchar *const file_name;
  guint len;
  gpointer data;

  g_object_get ((gpointer) self, "file-name", &file_name, "data", &data,
      "data-len", &len, NULL);
  GST_INFO ("buzz loader will now load song from \"%s\"", file_name);

  if (file_name) {
    GError *e = NULL;
    if (!g_file_get_contents (file_name, &self->priv->data,
            &self->priv->data_length, &e)) {
      GST_WARNING ("failed to read song file \"%s\" : %s", file_name,
          e->message);
      g_propagate_error (err, e);
    }
  } else {
    self->priv->data = data;
    self->priv->data_length = len;
    if (!data && !len) {
      g_set_error (err, G_IO_ERROR, G_IO_ERROR_FAILED, "Empty data block.");
    }
  }
  self->priv->data_pos = 0;
  if (self->priv->data && self->priv->data_length) {
    gchar tmp[4];

    if ((!g_ascii_strncasecmp (read_section_name (self, tmp), "Buzz", 4)) &&
        read_section_table (self)
        ) {
      if (!get_section_entry (self, "PARA")) {
        GST_WARNING
            ("This file does not comply with Buzz v1.2 fileformat (no PARA section)");
      }
      // parse sections
      if (read_bver_section (self, song) &&
          read_para_section (self, song) &&
          read_mach_section (self, song) &&
          read_conn_section (self, song) &&
          read_patt_section (self, song) &&
          read_sequ_section (self, song) &&
          read_wavt_section (self, song) &&
          read_cwav_section (self, song) &&
          read_blah_section (self, song) &&
          read_pdlg_section (self, song) && read_midi_section (self, song)
          ) {
        if (file_name) {
          gchar *str, *name;

          // file_name is last part from file_path
          str = strrchr (file_name, G_DIR_SEPARATOR);
          if (str) {
            name = g_strdup (&str[1]);
          } else {
            name = g_strdup (file_name);
          }
          str = strrchr (name, '.');
          if (str)
            *str = '\0';

          bt_child_proxy_set ((GObject *) (song), "song-info::name", name,
              NULL);
        }
        result = TRUE;
      } else {
        GST_WARNING ("Error reading a buzz file section.");
        g_set_error (err, BT_SONG_IO_ERROR, BT_SONG_IO_ERROR_INVALID_FORMAT,
            _("Error reading a buzz file section."));
      }
    } else {
      GST_WARNING ("is not a buzz file: \"%c%c%c%c\"!=\"Buzz\"", tmp[0],
          tmp[1], tmp[2], tmp[3]);
      g_set_error (err, BT_SONG_IO_ERROR, BT_SONG_IO_ERROR_INVALID_FORMAT,
          _("Is not a buzz file."));
    }
  }
  if (file_name) {
    g_free (self->priv->data);
    g_free (file_name);
  }
  return result;
}

//-- wrapper

//-- class internals

static void
bt_song_io_buzz_finalize (GObject * object)
{
  BtSongIOBuzz *self = BT_SONG_IO_BUZZ (object);
  gint32 i, j;

  GST_DEBUG ("!!!! self=%p", self);

  if (self->priv->mach_section) {
    for (i = 0; i < self->priv->number_of_machines; i++) {
      BmxMachSection *mach = &self->priv->mach_section[i];
      g_free (mach->name);
      g_free (mach->dllname);
      g_free (mach->global_parameter_state);
      if (mach->track_parameter_state) {
        for (j = 0; j < mach->number_of_tracks; j++) {
          g_free (mach->track_parameter_state[j]);
        }
        g_free (mach->track_parameter_state);
      }
    }
    g_free (self->priv->mach_section);
  }
  if (self->priv->para_section) {
    for (i = 0; i < self->priv->number_of_machines; i++) {
      BmxParaSection *para = &self->priv->para_section[i];
      g_free (para->name);
      g_free (para->long_name);
      if (para->global_params) {
        for (j = 0; j < para->number_of_global_params; j++) {
          g_free (para->global_params[j].name);
        }
        g_free (para->global_params);
      }
      if (para->track_params) {
        for (j = 0; j < para->number_of_track_params; j++) {
          g_free (para->track_params[j].name);
        }
        g_free (para->track_params);
      }
    }
    g_free (self->priv->para_section);
  }

  g_free (self->priv->entries);

  G_OBJECT_CLASS (bt_song_io_buzz_parent_class)->finalize (object);
}

static void
bt_song_io_buzz_init (BtSongIOBuzz * self)
{
  self->priv = bt_song_io_buzz_get_instance_private(self);
}

static void
bt_song_io_buzz_class_init (BtSongIOBuzzClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BtSongIOClass *base_class = BT_SONG_IO_CLASS (klass);

  gobject_class->finalize = bt_song_io_buzz_finalize;

  /* implement virtual class function. */
  base_class->load = bt_song_io_buzz_load;
}
