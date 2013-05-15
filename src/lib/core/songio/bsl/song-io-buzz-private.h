/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@buzztrax.org>
 *
 * class for buzz song input and output, private data structures
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

#ifndef BT_SONG_IO_BUZZ_PRIVATE_H
#define BT_SONG_IO_BUZZ_PRIVATE_H

typedef struct {
  gchar name[4];
  guint32 offset;
  guint32 size;
} BmxSectionEntry;

typedef struct {
  guint16 wSum1;
  guint16 wSum2;
  guint16 wResult;
  guint16 *lpwTempData;
} BmxCompressionValues;

typedef struct {
  gchar *key;
  guint32 value;
} BmxMachineAttributes;

typedef struct {
  gchar *name;
  guint8 type; // 0 = master, 1 = generator, 2 = effect
  gchar *dllname; // name of DLL file if type is 1 or 2
  gfloat xpos;
  gfloat ypos;
  guint16 number_of_inputs; // will be calculated by reading CONN section
  guint16 number_of_attributes;
  BmxMachineAttributes *attributes;
  guint16 *global_parameter_state;
  guint16 number_of_tracks;    
  guint16 **track_parameter_state;
} BmxMachSection;

typedef struct { // almost like CMachineParameter
  guint8 type;
  gchar *name;
  gint minvalue;
  gint maxvalue;
  gint novalue;
  gint flags;
  gint defvalue;
} BmxParameter;

typedef struct {
  gchar *name;
  gchar *long_name;
  guint32 number_of_global_params;
  guint32 number_of_track_params;
  gint size;
  BmxParameter *global_params;
  BmxParameter *track_params;
} BmxParaSection;


#endif // BT_SONG_IO_BUZZ_PRIVATE_H
