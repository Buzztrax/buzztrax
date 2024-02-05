/* Buzztrax
 * Copyright (C) 2010 Buzztrax team <buzztrax-devel@buzztrax.org>
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
 * SECTION:btchangelogger
 * @short_description: interface for the editor action journaling
 *
 * Defines undo/redo interface.
 */

#define BT_EDIT
#define BT_CHANGE_LOGGER_C

#include "bt-edit.h"

//-- the iface

G_DEFINE_INTERFACE (BtChangeLogger, bt_change_logger, 0);

//-- common helper

/**
 * bt_change_logger_match_method:
 * @change_logger_methods: array of change log methods
 * @data: the string to match agains
 * @match_info: resulting parameter on a positive match
 *
 * Matches commands registered in the @change_logger_methods against @data. If
 * a match is found the parameters are returned in @match_info.
 *
 * Returns: the command id from @change_logger_methods or -1 for no match. Free
 * the match_info on positive matches.
 */
gint
bt_change_logger_match_method (BtChangeLoggerMethods * change_logger_methods,
    const gchar * data, GMatchInfo ** match_info)
{
  gint i = 0, res = -1;
  BtChangeLoggerMethods *clm = change_logger_methods;

  while (clm->name) {
    if (!strncmp (data, clm->name, clm->name_len)) {
      if (!clm->regex) {
        clm->regex = g_regex_new (clm->regex_str, 0, 0, NULL);
      }
      if (g_regex_match_full (clm->regex, data, -1, clm->name_len, 0,
              match_info, NULL)) {
        res = i;
      } else {
        GST_WARNING ("no match for command %s in regexp \"%s\" for data \"%s\"",
            clm->name, g_regex_get_pattern (clm->regex), &data[clm->name_len]);
      }
      break;
    }
    i++;
    clm++;
  }
  return res;
}

//-- wrapper

/**
 * bt_change_logger_change:
 * @self: an object that implements logging changes
 * @data: serialised data of the action to apply
 *
 * Run the editor action pointed to by @data.
 *
 * Returns: %TRUE for success.
 */
gboolean
bt_change_logger_change (BtChangeLogger * self, const gchar * data)
{
  g_return_val_if_fail (BT_IS_CHANGE_LOGGER (self), FALSE);

  return BT_CHANGE_LOGGER_GET_INTERFACE (self)->change (self, data);
}

//-- interface internals

static void
bt_change_logger_default_init (BtChangeLoggerInterface * klass)
{
}
