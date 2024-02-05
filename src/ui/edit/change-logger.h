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

#ifndef BT_CHANGE_LOGGER_H
#define BT_CHANGE_LOGGER_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_CHANGE_LOGGER               (bt_change_logger_get_type())
#define BT_CHANGE_LOGGER(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_CHANGE_LOGGER, BtChangeLogger))
#define BT_IS_CHANGE_LOGGER(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_CHANGE_LOGGER))
#define BT_CHANGE_LOGGER_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BT_TYPE_CHANGE_LOGGER, BtChangeLoggerInterface))

/* type macros */

typedef struct _BtChangeLogger BtChangeLogger; /* dummy object */
typedef struct _BtChangeLoggerInterface BtChangeLoggerInterface;

struct _BtChangeLoggerInterface {
  const GTypeInterface parent;

  gboolean (*change)(BtChangeLogger *owner,const gchar *data);
};

typedef struct _BtChangeLoggerMethods BtChangeLoggerMethods;

struct _BtChangeLoggerMethods {
  const gchar *name;
  const gchar *regex_str;
  GRegex *regex;
  guint name_len;
};

/**
 * BT_CHANGE_LOGGER_METHOD:
 * @name: name of method
 * @name_len: length of the string
 * @regexp: regular expression for parsing the parameter part
 *
 * Structure entry for a change log line parser array (array of 
 * BtChangeLoggerMethods).
 */
// too bad we can't put strlen(name) below as name is const
#define BT_CHANGE_LOGGER_METHOD(name,name_len,regexp) \
{ name " ", regexp, NULL, name_len }

GType bt_change_logger_get_type(void) G_GNUC_CONST;


// common helper
gint bt_change_logger_match_method(BtChangeLoggerMethods *change_logger_methods,const gchar *data, GMatchInfo **match_info);

// wrapper
gboolean bt_change_logger_change(BtChangeLogger *self,const gchar *data);

#endif // BT_CHANGE_LOGGER_H
