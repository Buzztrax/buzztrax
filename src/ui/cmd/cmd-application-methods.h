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

#ifndef BT_CMD_APPLICATION_METHODS_H
#define BT_CMD_APPLICATION_METHODS_H

#include "cmd-application.h"

extern BtCmdApplication *bt_cmd_application_new(gboolean quiet);

extern gboolean bt_cmd_application_play(const BtCmdApplication *self, const gchar *input_file_name);
extern gboolean bt_cmd_application_info(const BtCmdApplication *self, const gchar *input_file_name, const gchar *output_file_name);
extern gboolean bt_cmd_application_convert(const BtCmdApplication *self, const gchar *input_file_name, const gchar *output_file_name);
extern gboolean bt_cmd_application_encode(const BtCmdApplication *self, const gchar *input_file_name, const gchar *output_file_name);

#endif // BT_CMD_APPLICATION_METHDOS_H
