/* $Id$
 *
 * Buzztard
 * Copyright (C) 2010 Buzztard team <buzztard-devel@lists.sf.net>
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

#ifndef BT_CHANGE_LOG_H
#define BT_CHANGE_LOG_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_CHANGE_LOG            (bt_change_log_get_type ())
#define BT_CHANGE_LOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_CHANGE_LOG, BtChangeLog))
#define BT_CHANGE_LOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_CHANGE_LOG, BtChangeLogClass))
#define BT_IS_CHANGE_LOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_CHANGE_LOG))
#define BT_IS_CHANGE_LOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_CHANGE_LOG))
#define BT_CHANGE_LOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_CHANGE_LOG, BtChangeLogClass))

/* type macros */

typedef struct _BtChangeLog BtChangeLog;
typedef struct _BtChangeLogClass BtChangeLogClass;
typedef struct _BtChangeLogPrivate BtChangeLogPrivate;

/**
 * BtChangeLog:
 *
 * Editor action journal.
 */
struct _BtChangeLog {
  GObject parent;

  /*< private >*/
  BtChangeLogPrivate *priv;
};
/* structure of the ui-resources class */
struct _BtChangeLogClass {
  GObjectClass parent;
};

typedef struct _BtChangeLogFile BtChangeLogFile;
struct _BtChangeLogFile {
  gchar *log_name;
  gchar *song_file_name;
  gchar *change_ts;
};

/* used by CHANGE_LOG_TYPE */
GType bt_change_log_get_type(void) G_GNUC_CONST;

#endif // BT_CHANGE_LOG_H
