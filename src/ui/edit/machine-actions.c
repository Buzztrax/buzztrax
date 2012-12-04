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
/**
 * SECTION:btmachineactions
 * @short_description: shared machine released ui actions
 *
 * Shared UI actions for machines that can be invoked from several UI classes.
 */

#define BT_EDIT
#define BT_MACHINE_ACTIONS_C

#include "bt-edit.h"

/**
 * bt_machine_action_about:
 * @machine: the machine
 * @main_window: the main window to use as parent
 *
 * Shows the about dialog for the given @machine.
 */
void
bt_machine_action_about (BtMachine * machine, BtMainWindow * main_window)
{
  GstElementFactory *element_factory;
  GstElement *element;

  GST_INFO ("context_menu about event occurred");
  // show info about machine
  g_object_get (machine, "machine", &element, NULL);
  if ((element_factory = gst_element_get_factory (element))) {
    const gchar *element_longname =
        gst_element_factory_get_longname (element_factory);
    const gchar *element_author =
        gst_element_factory_get_author (element_factory);
    const gchar *element_description =
        gst_element_factory_get_description (element_factory);
    gchar *str, *str_author, *str_desc;

    str_desc =
        g_markup_escape_text (element_description,
        strlen (element_description));
    str_author = g_markup_escape_text (element_author, strlen (element_author));
    // add a '\n' after each ',' to nicely format the message_box
    str = str_author;
    while ((str = strchr (str, ','))) {
      str++;
      if (*str == ' ')
        *str = '\n';
    }

    // format about message for machines
    str = g_strdup_printf (_("by %s\n\n%s"), str_author, str_desc);
    bt_dialog_message (main_window, _("About…"), element_longname, str);

    g_free (str);
    g_free (str_author);
    g_free (str_desc);
  }
  gst_object_unref (element);
}
