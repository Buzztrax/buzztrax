/* GStreamer
 * Copyright (C) 2005 Stefan Kost <ensonic at user.sf.net>
 *
 * common.h: Header for functions shared among all native and wrapped elements
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
/* < private_header > */

#ifndef __GST_BML_COMMON_H__
#define __GST_BML_COMMON_H__

#include "gstbml.h"

G_BEGIN_DECLS

//-- preset iface

gchar** gstbml_preset_get_preset_names(GstBML *bml, GstBMLClass *klass);
gboolean gstbml_preset_load_preset(GstObject *self, GstBML *bml, GstBMLClass *klass, const gchar *name);
gboolean gstbml_preset_save_preset(GstObject *self, GstBML *bml, GstBMLClass *klass, const gchar *name);
gboolean gstbml_preset_rename_preset(GstBMLClass *klass, const gchar *old_name, const gchar *new_name);
gboolean gstbml_preset_delete_preset(GstBMLClass *klass, const gchar *name);
gboolean gstbml_preset_set_meta(GstBMLClass *klass, const gchar *name ,const gchar *tag, const gchar *value);
gboolean gstbml_preset_get_meta(GstBMLClass *klass, const gchar *name, const gchar *tag, gchar **value);
void gstbml_preset_finalize(GstBMLClass *klass);

//-- common class functions

void gstbml_convert_names(GObjectClass *klass, gchar *tmp_name, gchar *tmp_desc, gchar **name, gchar **nick, gchar **desc);
GParamSpec *gstbml_register_param(GObjectClass *klass,gint prop_id, GstBMLParameterTypes type, GType enum_type, gchar *name, gchar *nick, gchar *desc, gint flags, gint min_val, gint max_val, gint no_val, gint def_val);

//-- common element functions

void gstbml_set_param(GstBMLParameterTypes type,gint val,GValue *value);
gint gstbml_get_param(GstBMLParameterTypes type,const GValue *value);
guint gstbml_calculate_buffer_size(GstBML * bml);
void gstbml_calculate_buffer_frames(GstBML *bml);

void gstbml_dispose(GstBML *bml);

gboolean gstbml_fix_data(GstElement *elem,GstMapInfo *info,gboolean has_data);

G_END_DECLS

#endif /* __GST_BML_COMMON_H__ */
