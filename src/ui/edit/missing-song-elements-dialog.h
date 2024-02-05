/* Buzztrax
 * Copyright (C) 2007 Buzztrax team <buzztrax-devel@buzztrax.org>
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

#ifndef BT_MISSING_SONG_ELEMENTS_DIALOG_H
#define BT_MISSING_SONG_ELEMENTS_DIALOG_H

#include <glib.h>
#include <glib-object.h>

G_DECLARE_FINAL_TYPE(BtMissingSongElementsDialog, bt_missing_song_elements_dialog,
    BT, MISSING_SONG_ELEMENTS_DIALOG, AdwMessageDialog);

#define BT_TYPE_MISSING_SONG_ELEMENTS_DIALOG            (bt_missing_song_elements_dialog_get_type ())

BtMissingSongElementsDialog *bt_missing_song_elements_dialog_new(GList *machines,GList *waves);

#endif // BT_MISSING_SONG_ELEMENTS_DIALOG_H
