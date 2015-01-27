/* GStreamer
 * Copyright (C) 2007 Stefan Kost <ensonic@users.sf.net>
 *
 * musicenums.c: enum types for gstreamer elements
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
 * SECTION:musicenums
 * @title: GstBtMusicEnums
 * @include: libgstbuzztrax/musicenums.h
 * @short_description: various enum types
 *
 * Music/Audio related enumerations.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "musicenums.h"

GType
gstbt_trigger_switch_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (!type)) {
    static const GEnumValue values[] = {
      {GSTBT_TRIGGER_SWITCH_OFF, "OFF", "0"},
      {GSTBT_TRIGGER_SWITCH_ON, "ON", "1"},
      {GSTBT_TRIGGER_SWITCH_EMPTY, "EMPTY", ""},
      {0, NULL, NULL},
    };
    type = g_enum_register_static ("GstBtTriggerSwitch", values);
  }
  return type;
}

GType
gstbt_note_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (!type)) {
    static const GEnumValue values[] = {
      {GSTBT_NOTE_OFF, "OFF", "off"},
      {GSTBT_NOTE_NONE, "NONE", ""},
      /* 0 */
      {GSTBT_NOTE_C_0, "C_0", "c-0"},
      {GSTBT_NOTE_CIS_0, "CIS_0", "c#0"},
      {GSTBT_NOTE_D_0, "D_0", "d-0"},
      {GSTBT_NOTE_DIS_0, "DIS_0", "d#0"},
      {GSTBT_NOTE_E_0, "E_0", "e-0"},
      {GSTBT_NOTE_F_0, "F_0", "f-0"},
      {GSTBT_NOTE_FIS_0, "FIS_0", "f#0"},
      {GSTBT_NOTE_G_0, "G_0", "g-0"},
      {GSTBT_NOTE_GIS_0, "GIS_0", "g#0"},
      {GSTBT_NOTE_A_0, "A_0", "a-0"},
      {GSTBT_NOTE_AIS_0, "AIS_0", "a#0"},
      {GSTBT_NOTE_B_0, "B_0", "b-0"},
      /* 1 */
      {GSTBT_NOTE_C_1, "C_1", "c-1"},
      {GSTBT_NOTE_CIS_1, "CIS_1", "c#1"},
      {GSTBT_NOTE_D_1, "D_1", "d-1"},
      {GSTBT_NOTE_DIS_1, "DIS_1", "d#1"},
      {GSTBT_NOTE_E_1, "E_1", "e-1"},
      {GSTBT_NOTE_F_1, "F_1", "f-1"},
      {GSTBT_NOTE_FIS_1, "FIS_1", "f#1"},
      {GSTBT_NOTE_G_1, "G_1", "g-1"},
      {GSTBT_NOTE_GIS_1, "GIS_1", "g#1"},
      {GSTBT_NOTE_A_1, "A_1", "a-1"},
      {GSTBT_NOTE_AIS_1, "AIS_1", "a#1"},
      {GSTBT_NOTE_B_1, "B_1", "b-1"},
      /* 2 */
      {GSTBT_NOTE_C_2, "C_2", "c-2"},
      {GSTBT_NOTE_CIS_2, "CIS_2", "c#2"},
      {GSTBT_NOTE_D_2, "D_2", "d-2"},
      {GSTBT_NOTE_DIS_2, "DIS_2", "d#2"},
      {GSTBT_NOTE_E_2, "E_2", "e-2"},
      {GSTBT_NOTE_F_2, "F_2", "f-2"},
      {GSTBT_NOTE_FIS_2, "FIS_2", "f#2"},
      {GSTBT_NOTE_G_2, "G_2", "g-2"},
      {GSTBT_NOTE_GIS_2, "GIS_2", "g#2"},
      {GSTBT_NOTE_A_2, "A_2", "a-2"},
      {GSTBT_NOTE_AIS_2, "AIS_2", "a#2"},
      {GSTBT_NOTE_B_2, "B_2", "b-2"},
      /* 3 */
      {GSTBT_NOTE_C_3, "C_3", "c-3"},
      {GSTBT_NOTE_CIS_3, "CIS_3", "c#3"},
      {GSTBT_NOTE_D_3, "D_3", "d-3"},
      {GSTBT_NOTE_DIS_3, "DIS_3", "d#3"},
      {GSTBT_NOTE_E_3, "E_3", "e-3"},
      {GSTBT_NOTE_F_3, "F_3", "f-3"},
      {GSTBT_NOTE_FIS_3, "FIS_3", "f#3"},
      {GSTBT_NOTE_G_3, "G_3", "g-3"},
      {GSTBT_NOTE_GIS_3, "GIS_3", "g#3"},
      {GSTBT_NOTE_A_3, "A_3", "a-3"},
      {GSTBT_NOTE_AIS_3, "AIS_3", "a#3"},
      {GSTBT_NOTE_B_3, "B_3", "b-3"},
      /* 4 */
      {GSTBT_NOTE_C_4, "C_4", "c-4"},
      {GSTBT_NOTE_CIS_4, "CIS_4", "c#4"},
      {GSTBT_NOTE_D_4, "D_4", "d-4"},
      {GSTBT_NOTE_DIS_4, "DIS_4", "d#4"},
      {GSTBT_NOTE_E_4, "E_4", "e-4"},
      {GSTBT_NOTE_F_4, "F_4", "f-4"},
      {GSTBT_NOTE_FIS_4, "FIS_4", "f#4"},
      {GSTBT_NOTE_G_4, "G_4", "g-4"},
      {GSTBT_NOTE_GIS_4, "GIS_4", "g#4"},
      {GSTBT_NOTE_A_4, "A_4", "a-4"},
      {GSTBT_NOTE_AIS_4, "AIS_4", "a#4"},
      {GSTBT_NOTE_B_4, "B_4", "b-4"},
      /* 5 */
      {GSTBT_NOTE_C_5, "C_5", "c-5"},
      {GSTBT_NOTE_CIS_5, "CIS_5", "c#5"},
      {GSTBT_NOTE_D_5, "D_5", "d-5"},
      {GSTBT_NOTE_DIS_5, "DIS_5", "d#5"},
      {GSTBT_NOTE_E_5, "E_5", "e-5"},
      {GSTBT_NOTE_F_5, "F_5", "f-5"},
      {GSTBT_NOTE_FIS_5, "FIS_5", "f#5"},
      {GSTBT_NOTE_G_5, "G_5", "g-5"},
      {GSTBT_NOTE_GIS_5, "GIS_5", "g#5"},
      {GSTBT_NOTE_A_5, "A_5", "a-5"},
      {GSTBT_NOTE_AIS_5, "AIS_5", "a#5"},
      {GSTBT_NOTE_B_5, "B_5", "b-5"},
      /* 6 */
      {GSTBT_NOTE_C_6, "C_6", "c-6"},
      {GSTBT_NOTE_CIS_6, "CIS_6", "c#6"},
      {GSTBT_NOTE_D_6, "D_6", "d-6"},
      {GSTBT_NOTE_DIS_6, "DIS_6", "d#6"},
      {GSTBT_NOTE_E_6, "E_6", "e-6"},
      {GSTBT_NOTE_F_6, "F_6", "f-6"},
      {GSTBT_NOTE_FIS_6, "FIS_6", "f#6"},
      {GSTBT_NOTE_G_6, "G_6", "g-6"},
      {GSTBT_NOTE_GIS_6, "GIS_6", "g#6"},
      {GSTBT_NOTE_A_6, "A_6", "a-6"},
      {GSTBT_NOTE_AIS_6, "AIS_6", "a#6"},
      {GSTBT_NOTE_B_6, "B_6", "b-6"},
      /* 7 */
      {GSTBT_NOTE_C_7, "C_7", "c-7"},
      {GSTBT_NOTE_CIS_7, "CIS_7", "c#7"},
      {GSTBT_NOTE_D_7, "D_7", "d-7"},
      {GSTBT_NOTE_DIS_7, "DIS_7", "d#7"},
      {GSTBT_NOTE_E_7, "E_7", "e-7"},
      {GSTBT_NOTE_F_7, "F_7", "f-7"},
      {GSTBT_NOTE_FIS_7, "FIS_7", "f#7"},
      {GSTBT_NOTE_G_7, "G_7", "g-7"},
      {GSTBT_NOTE_GIS_7, "GIS_7", "g#7"},
      {GSTBT_NOTE_A_7, "A_7", "a-7"},
      {GSTBT_NOTE_AIS_7, "AIS_7", "a#7"},
      {GSTBT_NOTE_B_7, "B_7", "b-7"},
      /* 8 */
      {GSTBT_NOTE_C_8, "C_8", "c-8"},
      {GSTBT_NOTE_CIS_8, "CIS_8", "c#8"},
      {GSTBT_NOTE_D_8, "D_8", "d-8"},
      {GSTBT_NOTE_DIS_8, "DIS_8", "d#8"},
      {GSTBT_NOTE_E_8, "E_8", "e-8"},
      {GSTBT_NOTE_F_8, "F_8", "f-8"},
      {GSTBT_NOTE_FIS_8, "FIS_8", "f#8"},
      {GSTBT_NOTE_G_8, "G_8", "g-8"},
      {GSTBT_NOTE_GIS_8, "GIS_8", "g#8"},
      {GSTBT_NOTE_A_8, "A_8", "a-8"},
      {GSTBT_NOTE_AIS_8, "AIS_8", "a#8"},
      {GSTBT_NOTE_B_8, "B_8", "b-8"},
      /* 9 */
      {GSTBT_NOTE_C_9, "C_9", "c-9"},
      {GSTBT_NOTE_CIS_9, "CIS_9", "c#9"},
      {GSTBT_NOTE_D_9, "D_9", "d-9"},
      {GSTBT_NOTE_DIS_9, "DIS_9", "d#9"},
      {GSTBT_NOTE_E_9, "E_9", "e-9"},
      {GSTBT_NOTE_F_9, "F_9", "f-9"},
      {GSTBT_NOTE_FIS_9, "FIS_9", "f#9"},
      {GSTBT_NOTE_G_9, "G_9", "g-9"},
      {GSTBT_NOTE_GIS_9, "GIS_9", "g#9"},
      {GSTBT_NOTE_A_9, "A_9", "a-9"},
      {GSTBT_NOTE_AIS_9, "AIS_9", "a#9"},
      {GSTBT_NOTE_B_9, "B_9", "b-9"},
      {0, NULL, NULL},
    };
    type = g_enum_register_static ("GstBtNote", values);
  }
  return type;
}
