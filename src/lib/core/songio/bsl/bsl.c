/* Buzztard
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

#define BT_BSL_C

#include "bsl.h"

GST_DEBUG_CATEGORY (GST_CAT_DEFAULT);

static gboolean bt_song_io_init (void);

/**
 * bt_song_io_module_info:
 *
 * Buzz song loader metadata.
 */
BtSongIOModuleInfo bt_song_io_module_info = {
  bt_song_io_init,
  {
        {0, "buzz song with waves", "audio/x-bmx", "bmx"},
        {0, "buzz song without waves", "audio/x-bmw", "bmw"},
        {0, NULL, NULL, NULL}
      }
};

static gboolean
bt_song_io_init (void)
{
  static gboolean first_run = TRUE;

  if (first_run) {
#ifdef ENABLE_NLS
    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

    GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "bt-bsl", 0,
        "music production environment / buzz song io plugin");

    bt_song_io_module_info.formats[0].type = BT_TYPE_SONG_IO_BUZZ;
    bt_song_io_module_info.formats[1].type = BT_TYPE_SONG_IO_BUZZ;
    first_run = FALSE;
  }
  return (TRUE);
}
